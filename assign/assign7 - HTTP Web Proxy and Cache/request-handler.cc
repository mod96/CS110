/**
 * File: request-handler.cc
 * ------------------------
 * Provides the implementation for the HTTPRequestHandler class.
 */

#include "request-handler.h"
#include "response.h"
#include "request.h"
#include "client-socket.h"
#include <socket++/sockstream.h> // for sockbuf, iosockstream
#include "ostreamlock.h"
using namespace std;

HTTPRequestHandler::HTTPRequestHandler()
{
	blacklist.addToBlacklist("./slink/blocked-domains.txt");
}

void HTTPRequestHandler::serviceRequest(const pair<int, string> &connection) throw()
{
	int clientSocketFd = connection.first;
	string clientIPAddress = connection.second;

	sockbuf sb(clientSocketFd);
	iosockstream ss(&sb);

	// read
	HTTPRequest request;
	request.ingestRequestLine(ss);
	bool isNotCyclic = request.ingestHeader(ss, clientIPAddress);
	request.ingestPayload(ss);

	cout << oslock << "[" << request.getMethod() << "] "
		 << "Server: " << request.getServer() << ", Path: " << request.getPath() << endl
		 << osunlock;
	// cout << oslock << request << endl
	// 	 << osunlock;

	HTTPResponse response;
	if (blacklist.serverIsAllowed(request.getServer()))
	{
		if (!isNotCyclic)
		{
			// set gateway timeout
			response.setResponseCode(504);
			response.setProtocol("HTTP/1.0");
			response.setPayload("Gateway Timed-out");
		}
		else
		{
			cache.lock(request);
			if (!cache.containsCacheEntry(request, response))
			{
				// request to origin
				int originSockFd;
				if (usingProxy)
				{
					originSockFd = createClientSocket(proxyServer, proxyPortNumber);
				}
				else
				{
					originSockFd = createClientSocket(request.getServer(), request.getPort());
				}
				sockbuf sb2(originSockFd);
				iosockstream ss2(&sb2);
				ss2 << request;
				ss2.flush();

				// set response
				response.ingestResponseHeader(ss2);
				response.ingestPayload(ss2);
				if (cache.shouldCache(request, response))
					cache.cacheEntry(request, response);
			}
			cache.unlock(request);
		}
	}
	else
	{
		// set response forbidden
		response.setResponseCode(403);
		response.setProtocol("HTTP/1.0");
		response.setPayload("Forbbiden Content");
	}
	ss << response;
	ss.flush();
}

// the following two methods needs to be completed
// once you incorporate your HTTPCache into your HTTPRequestHandler
void HTTPRequestHandler::clearCache()
{
	cache.clear();
}
void HTTPRequestHandler::setCacheMaxAge(long maxAge)
{
	cache.setMaxAge(maxAge);
}

// set proxy
void HTTPRequestHandler::setProxy(const std::string &server, unsigned short port)
{
	proxyServer = server;
	proxyPortNumber = port;
	usingProxy = true;
}