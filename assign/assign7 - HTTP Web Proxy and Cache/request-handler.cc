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

	// request to origin
	int originSockFd = createClientSocket(request.getServer(), request.getPort());
	sockbuf sb2(originSockFd);
	iosockstream ss2(&sb2);
	ss2 << request;
	ss2.flush();

	// response
	HTTPResponse response;
	response.ingestResponseHeader(ss2);
	response.ingestPayload(ss2);
	ss << response;
	ss.flush();
}

// the following two methods needs to be completed
// once you incorporate your HTTPCache into your HTTPRequestHandler
void HTTPRequestHandler::clearCache() {}
void HTTPRequestHandler::setCacheMaxAge(long maxAge) {}