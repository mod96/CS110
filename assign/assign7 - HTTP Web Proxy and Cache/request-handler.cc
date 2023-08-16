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
	request.ingestHeader(ss, clientIPAddress);
	request.ingestPayload(ss);
	// cout << request << endl;

	HTTPResponse response;
	if (blacklist.serverIsAllowed(request.getServer()))
	{
		if (!cache.containsCacheEntry(request, response))
		{
			// request to origin
			int originSockFd = createClientSocket(request.getServer(), request.getPort());
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