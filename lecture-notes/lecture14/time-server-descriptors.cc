#include <iostream>     // for cout, cett, endl
#include <ctime>        // for time, gmtime, strftim
#include <sys/socket.h> // for socket, bind, accept, listen, etc.
#include <climits>      // for USHRT_MAX
#include <cstring>      // for strlen
#include <unistd.h>     // for write
#include "server-socket.h"
#include <socket++/sockstream.h>
// sudo apt-get install libsocket++1
// sudo apt-get install libsocket++-dev
// find /usr/include -name sockstream.h
// change ldflags for including socket++ lib.

using namespace std;

static const short kDefaultPort = 12345;
static const int kWrongArgumentCount = 1;
static const int kServerStartFailure = 2;
static void publishTime(int client)
{
    time_t rawtime;
    time(&rawtime);
    struct tm *ptm = gmtime(&rawtime);
    char timestr[128]; // more than big enough
    /* size_t len = */ strftime(timestr, sizeof(timestr), "%c\n", ptm);

    sockbuf sb(client);
    iosockstream ss(&sb);
    ss << timestr << endl;
} // sockbuf destructor closes client

int main(int argc, char *argv[])
{
    int server = createServerSocket(12345);
    while (true)
    {
        int client = accept(server, NULL, NULL);
        publishTime(client);
    }

    return 0;
}