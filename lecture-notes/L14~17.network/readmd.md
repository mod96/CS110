# Lecture 14: Introduction to Networking

Networking is simply communicating between two computers connected on a network. You can actually set up a network connection on a single computer, as well. A network requires one computer to act as the server, waiting patiently for an incoming connection from another computer, the client.

Server-side applications set up a socket that listens to a particular port. The server socket is an integer identifier associated with a local IP address, and a the port number is a `16-bit integer` with up to 0 ~ 65535 allowable ports.

You can think of a port number as a virtual process ID that the host associates with the true pid of the server application.

The port numbers in the range from 0 to 1023 are the well-known ports or system ports. Details in [here - Ports Database](https://www.speedguide.net/ports.php), summary in [here - Wikipedia](https://en.wikipedia.org/wiki/List_of_TCP_and_UDP_port_numbers). Ports 25 and 587 are the SMTP (Simple Mail Transfer Protocol), for sending and receiving email. Port 53 is the DNS (Domain Name Service) port, for associating names with IP addresses. Port 22 is the port for SSH (Secure Shell). Port 631 is for IPP (internet printing protocol).


### Example: Time Server Descriptors

```cc
int main(int argc, char *argv[]) {
    int server = createServerSocket(12345);
    while (true) {
        int client = accept(server, NULL, NULL); // the two NULLs could instead be used to
        // surface the IP address of the client
        publishTime(client);
    }
    return 0;
}
```
`accept (found in sys/socket.h)` returns a descriptor that can be written to and read from. Whatever's written is sent to the client, and whatever the client sends back is readable here. This descriptor is one end of a bidirectional pipe bridging two processes on different machines.

```cc
static void publishTime(int client) {
    // server-side computation needed for the service to produce output.
    time_t rawtime;
    time(&rawtime);
    struct tm *ptm = gmtime(&rawtime);
    char timestr[128]; // more than big enough
    /* size_t len = */ strftime(timestr, sizeof(timestr), "%c\n", ptm);

    // remaining lines publish the time string to the client socket using the raw, 
    // low-level I/O we've seen before
    size_t numBytesWritten = 0, numBytesToWrite = strlen(timestr);
    while (numBytesWritten < numBytesToWrite) {
        numBytesWritten += write(client, 
                                timestr + numBytesWritten,
                                numBytesToWrite - numBytesWritten);
    }
    close(client);
}
```

Note that the while loop for writing bytes is a bit more important now that we are networking: we are more likely to need to write multiple times on a network. The socket descriptor is bound to a network driver that may have a limited amount of space. That means write's return value could very well be less than what was supplied by the third argument.

Ideally, we'd rely on either C streams (e.g. the FILE *) or C++ streams (e.g. the iostream class hierarchy) to layer over data buffers and manage the while loop around exposed write calls for us. Fortunately, there's a stable, easy-to-use third-party library—one called `socket++` that
provides exactly this. socket++ provides iostream subclasses that respond to operator<<, operator>>, getline, endl, and so forth, just like cin, cout, and file streams do. We are going to operate as if this third-party library was just part of standard C++.

Here's the new implementation of publishTime:

```cc
static void publishTime(int client) {
    time_t rawtime;
    time(&rawtime);
    struct tm *ptm = gmtime(&rawtime);
    char timestr[128]; // more than big enough
    /* size_t len = */ strftime(timestr, sizeof(timestr), "%c", ptm);
    sockbuf sb(client);
    iosockstream ss(&sb);
    ss << timestr << endl;
} // sockbuf destructor closes client
```
This time, however, we insert that string into an iosockstream that itself layers over the client socket. Note that the intermediary sockbuf class takes ownership of the socket and closes it when its destructor is called.

Our time server can benefit from multithreading as well. The work a server needs to do in order to meet the client's request might be time consuming—so time consuming, in fact, that the server is slow to iterate and accept new client connections.

As soon as accept returns a socket descriptor, spawn a child thread—or reuse an existing one within a ThreadPool—to get any intense, time consuming computation off of the main thread. The child thread can make use of a second processor or a second core, and the main thread can quickly move on to its next accept call. Here's a new version of our time server, which uses a ThreadPool (you'll be implementing one for Assignment 6) to get the computation off the main thread.

```cc
int main(int argc, char *argv[]) {
    int server = createServerSocket(12345);
    ThreadPool pool(4);
    while (true) {
        int client = accept(server, NULL, NULL); // the two NULLs could instead be used
                                                // to surface the IP address of the client
        pool.schedule([client] { publishTime(client); });
    }
return 0;
}
```

The implementation of publishTime needs to change just a little if it's to be thread safe: we need to call a different version of gmtime. **(`gmtime` returns a pointer to a single, statically allocated global that's used by all calls)** Of course, one solution would be to use a mutex to ensure that a thread can call gmtime without competition and subsequently extract the data from the global into local copy. Another solution—one that doesn't require locking and one I think is better—makes use of a second version of the same function called gmtime_r. This second, reentrant version just requires that space for a dedicated return value be passed in.

```cc
static void publishTime(int client) {
    time_t rawtime;
    time(&rawtime);
    struct tm tm;
    gmtime_r(&rawtime, &tm); // here!
    char timestr[128]; // more than big enough
    /* size_t len = */ strftime(timestr, sizeof(timestr), "%c", &tm);
    sockbuf sb(client); // destructor closes socket
    iosockstream ss(&sb);
    ss << timestr << endl;
}
```

Client-side code is straight forward. The client connects to a specific server and port number. The server responds to the connection by publishing the current time into its own end of the connection and then hanging up. The client ingests the single line of text and then itself hangs up.

```cc
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "Usage:\n\t" << argv[0] << " server port" << endl;
        cerr << "e.g.,\n\t" << argv[0] << " myth61.stanford.edu 12345" << endl;

        return kWrongArgumentCount;
    }

    // Open a connection to the server
    int socketDescriptor = createClientSocket(argv[1], atoi(argv[2]));

    if (socketDescriptor == kClientSocketError)
    {
        cerr << "Time server could not be reached"
             << "." << endl;
        cerr << "Aborting" << endl;
        return kTimeServerInaccessible;
    }

    sockbuf sb(socketDescriptor);
    iosockstream ss(&sb);
    string timeline;
    getline(ss, timeline);
    cout << timeline << endl;

    return 0;
}
```

# Lecture 15: Networking, Clients

Telnet (short for "teletype network") is a client/server application protocol that provides access to virtual terminals of remote systems on local area networks or the Internet. User data is interspersed in-band with Telnet control information in an 8-bit byte oriented data connection over the Transmission Control Protocol (TCP).

```console
$ telnet google.com 80
GET / HTTP/1.1

HTTP/1.1 200 OK
Date: Sun, 23 Jul 2023 07:52:01 GMT
Expires: -1
Cache-Control: private, max-age=0
...
Transfer-Encoding: chunked

4eda
<!doctype html><html itemscope="" itemtype="http://schema.org/WebPage" lang="ko"><head>
...
```
Headers are seperated by the new line, and one more new line, then data.

### Example: wget

`wget` is a command line utility that, given its URL, downloads a single document (HTML document, image, video, etc.) and saves a copy of it to the current working directory.

```cc
static const string kProtocolPrefix = "http://";
static const string kDefaultPath = "/";
static pair<string, string> parseURL(string url) {
    if (startsWith(url, kProtocolPrefix))
        url = url.substr(kProtocolPrefix.size());
    size_t found = url.find('/');
    if (found == string::npos)
        return make_pair(url, kDefaultPath);
    string host = url.substr(0, found);
    string path = url.substr(found);
    return make_pair(host, path);
}

static void issueRequest(iosockstream& ss, const string& host, const string& path) {
    ss << "GET " << path << " HTTP/1.0\r\n"; // like telnet command
    ss << "Host: " << host << "\r\n"; // It's standard HTTP-protocol practice that each line, including the blank line that marks the end of the request, end in CRLF
    ss << "\r\n";
    ss.flush(); // The flush is necessary to ensure all character data is pressed over the wire and consumable at the other end.
}

/**
* skipHeader reads through and discards all of the HTTP response header lines until it
* encounters either a blank line or one that contains nothing other than a '\r'. The blank
* line is, indeed, supposed to be "\r\n", but some servers—often hand-rolled ones—are
* sloppy, so we treat the '\r' as optional. Recall that getline chews up the '\n', but it
* won't chew up the '\r'.
*/
static void skipHeader(iosockstream& ss) {
    string line;
    do {
        getline(ss, line);
    } while (!line.empty() && line != "\r");
}

static string getFileName(const string& path) {
    if (path.empty() || path[path.size() - 1] == '/') return "index.html";
    size_t found = path.rfind('/');
    return path.substr(found + 1);
}

/*
* HTTP dictates that everything beyond that blank line is payload, and that once the server
* publishes that payload, it closes its end of the connection. That server-side close is the
* client-side's EOF, and we write everything we read.
*/
static void savePayload(iosockstream& ss, const string& filename) {
    ofstream output(filename, ios::binary); // don't assume it's text
    size_t totalBytes = 0;
    while (!ss.fail()) {
        char buffer[2014] = {'\0'};
        ss.read(buffer, sizeof(buffer));
        totalBytes += ss.gcount();
        output.write(buffer, ss.gcount());
    }
    cout << "Total number of bytes fetched: " << totalBytes << endl;
}

static const unsigned short kDefaultHTTPPort = 80;
static void pullContent(const pair<string, string>& components) {
    int client = createClientSocket(components.first, kDefaultHTTPPort);
    if (client == kClientSocketError) {
        cerr << "Could not connect to host named \"" << components.first << "\"." << endl;
        return;
    }
    sockbuf sb(client);
    iosockstream ss(&sb);
    issueRequest(ss, components.first, components.second);
    skipHeader(ss);
    savePayload(ss, getFileName(components.second));
}


int main(int argc, char *argv[]) {
    pullContent(parseURL(argv[1]));
    return 0;
}
```

### Example: Lexical Word Finder

Let's implement an API server that's architecturally in line with the way Google, Twitter, Facebook, and LinkedIn architect their own API servers.

Our implementation assumes we have a standard Unix executable called scrabbleword­finder. The source code for this executable—completely unaware it'll be used in a larger networked application.

We'll expect the API call to come in the form of a URL, and we'll expect that URL to include the rack of letters. Assuming our API server is running on myth54:13133, we expect http://myth54:13133/lexical and http://myth54:13133/network to generate the following payloads, in JSON format.

We can just leverage our `subprocess_t` type and `subprocess` function from Assignment 3.

Here is the core of the main function implementing our server:
```cc
int main(int argc, char *argv[]) {
    unsigned short port = extractPort(argv[1]);
    int server = createServerSocket(port);
    cout << "Server listening on port " << port << "." << endl;

    ThreadPool pool(16); // Each request is handled by a dedicated worker thread within a ThreadPool of size 16

    map<string, vector<string>> cache;
    mutex cacheLock;

    while (true) {
        struct sockaddr_in address; // used to surface IP address of client
        socklen_t size = sizeof(address); // also used to surface client IP address
        bzero(&address, size);
        int client = accept(server, (struct sockaddr *) &address, &size);
        char str[INET_ADDRSTRLEN];
        cout << "Received a connection request from "
            << inet_ntop(AF_INET, &address.sin_addr, str, INET_ADDRSTRLEN) << "." << endl;
        pool.schedule([client, &cache, &cacheLock] {
                    publishScrabbleWords(client, cache, cacheLock);
                    });
    }
    return 0;
}
```

```cc
static void publishScrabbleWords(int client, map<string, vector<string>>& cache, mutex& cacheLock) {
    sockbuf sb(client);
    iosockstream ss(&sb);
    string letters = getLetters(ss);
    sort(letters.begin(), letters.end());

    skipHeaders(ss);

    struct timeval start;
    gettimeofday(&start, NULL); // start the clock

    cacheLock.lock();
    auto found = cache.find(letters);
    cacheLock.unlock(); // release lock immediately, iterator won't be invalidated by competing find calls

    bool cached = found != cache.end();
    vector<string> formableWords;
    if (cached) {
        formableWords = found->second;
    } else {
        const char *command[] = {"./scrabble-word-finder", letters.c_str(), NULL};
        subprocess_t sp = subprocess(const_cast<char **>(command), false, true);
        pullFormableWords(formableWords, sp.ingestfd); // get words from sp
        waitpid(sp.pid, NULL, 0);

        lock_guard<mutex> lg(cacheLock); // released after 'else' scope.
        cache[letters] = formableWords;
    }

    struct timeval end, duration;
    gettimeofday(&end, NULL); // stop the clock, server-computation of formableWords is complete
    timersub(&end, &start, &duration);
    double time = duration.tv_sec + duration.tv_usec/1000000.0;
    ostringstream payload;
    constructPayload(formableWords, cached, time, payload);

    sendResponse(ss, payload.str());
}
```

But wait.. you said we shouldn't mix threading and multiprocessing, yet in the scrabble-word-finder program you used subprocess, which uses fork/exec from a thread. What's the deal? 

As it turns out, you can use fork from a thread, but you just need to be careful. 

It's safe to fork in a multithreaded program as long as you are very careful about the code between fork and exec. You can make only re-enterant (aka asynchronous-safe) system calls in that span. In theory, you are not allowed to malloc or free there, although in practice the default Linux allocator is safe, and Linux libraries came to rely on it. End result is that you must use the default allocator. 

In other words: don't use `malloc` and `free` after you `fork`, otherwise you might end up with bad things happening.

In [here](https://pubs.opengroup.org/onlinepubs/9699919799/functions/fork.html) says:


> A process shall be created with a single thread. If a multi-threaded process calls fork(), the new process shall contain a replica of the calling thread and its entire address space, possibly including the states of mutexes and other resources. Consequently, to avoid errors, the child process may only execute async-signal-safe operations until such time as one of the exec functions is called.


```cc
static void pullFormableWords(vector<string>& formableWords, int ingestfd) {
    stdio_filebuf<char> inbuf(ingestfd, ios::in);
    istream is(&inbuf);
    while (true) {
        string word;
        getline(is, word);
        if (is.fail()) break;
        formableWords.push_back(word);
    }
}
static void sendResponse(iosockstream& ss, const string& payload) {
    ss << "HTTP/1.1 200 OK\r\n";
    ss << "Content-Type: application/javascript; charset=UTF-8\r\n";
    ss << "Content-Length: " << payload.size() << "\r\n";
    ss << "\r\n";
    ss << payload << flush;
}
```
```cc
static string getLetters(iosockstream& ss) {
    string method, path, protocol;
    ss >> method >> path >> protocol;
    string rest;
    getline(ss, rest);
    size_t pos = path.rfind("/");
    return pos == string::npos ? path : path.substr(pos + 1);
}
static void constructPayload(const vector<string>& formableWords, bool cached, double time, ostringstream& payload) {
    payload << "{" << endl;
    payload << " \"time\":" << time << "," << endl;
    payload << " \"cached\": " << boolalpha << cached << "," << endl;
    payload << " \"possibilities\": " << constructJSONArray(formableWords, 2) << endl;
    payload << "}" << endl;
}
```


# Lecture 16: Network System Calls, Library Functions

Linux C includes directives to convert host names (e.g. "www.facebook.com") to IPv4 address (e.g. "31.13.75.17") and vice versa. Functions called gethostbyname and gethostbyaddr, while technically deprecated, are still so prevalent that you should know how to use them.

```cc
struct hostent *gethostbyname(const char *name); // not thread safe!
struct hostent *gethostbyaddr(const char *addr, int len, int type); // not thread safe!
```

`gethostbyname` assumes its argument is a host name (e.g. "www.google.com").

`gethostbyaddr` assumes the first argument is a binary representation of an IP address (e.g. not the string "171.64.64.137" but the base address of a character array with ASCII values of 171, 64, 64, and 137 laid down side by side in network byte order.) For IPv4, the second argument is usually 4 (or rather, `sizeof(struct in_addr)`) and the third is typically the `AF_INET` constant.

```cc
struct in_addr {
    unsigned int s_addr // four bytes, stored in network byte order (big endian)
};
struct hostent {
    char *h_name; // official name of host
    char **h_aliases; // NULL-terminated list of aliases
    int h_addrtype; // host address type (typically AF_INET for IPv4)
    int h_length; // address length (typically 4, or sizeof(struct in_addr) for IPv4)
    char **h_addr_list; // NULL-terminated list of IP addresses(actually it needs to be void *)
}; // h_addr_list is really a struct in_addr ** when hostent contains IPv4 addresses
```

One-field struct `in_addr` has `s_addr` field packs each figure of a dotted quad (e.g. 171.64.64.136) into one of its four bytes. Each of these four numbers numbers can range from 0 up through 255.

Struct `hostent` is used for all IP addresses, not just IPv4 addresses. For non-IPv4 addresses, `h_addrtype`, `h_length`, and `h_addr_list` carry different types of data than they do for IPv4.

### Example: IPv4
```cc
/**
 * Function: publishIPAddressInfo
 * ------------------------------
 * Relies on the services of the inet_ntop and gethostbyname functions
 * to produce the official host name for the supplied one (sometimes
 * the same, and sometimes different) and all of the IP addresses
 * that the supplied host name resolves to.
 */
static void publishIPAddressInfo(const string &host)
{
    struct hostent *he = gethostbyname(host.c_str());
    if (he == NULL)
    { // NULL return value means resolution attempt failed
        cout << host << " could not be resolved to an address.  Did you mistype it?" << endl;
        return;
    }

    assert(he->h_addrtype == AF_INET);
    assert(he->h_length == sizeof(struct in_addr));

    cout << "Official name is \"" << he->h_name << "\"" << endl;
    cout << "IP Addresses: " << endl;
    struct in_addr **addressList = (struct in_addr **)he->h_addr_list;
    while (*addressList != NULL)
    {
        char str[INET_ADDRSTRLEN];
        cout << "+ " << inet_ntop(AF_INET, *addressList, str, INET_ADDRSTRLEN) << endl;
        addressList++;
    }
}
```

The `inet_ntop` (ntop: network to printable) function places a traditional C string presentation of an IP address into the provided character buffer, and returns the the base address of that buffer.

---

A more generic version of gethostbyname—inventively named `gethostbyname2`—can be used to extract IPv6 address information about a hostname.

```cc
struct hostent *gethostbyname2(const char *name, int af); // af = AF_INET or AF_INET6
```

- A call to `gethostbyname2(host, AF_INET)` is equivalent to a call to `gethostbyname(host)`

- A call to `gethostbyname2(host, AF_INET6)` still returns a `struct hostent*`, but the struct hostent is populated with different values and types:
    - `h_addrtype` field is set to `AF_INET6`
    - `h_length` field houses a 16
    - `h_addr_list` field is really an array of `struct in6_addr` pointers, where
each struct in6_addr looks like this:

```cc
struct in6_addr {
    u_int8_t s6_addr[16]; // 16 bytes (128 bits), stored in network byte order
};
```
---

### Example: IPv6

```cc
/**
 * Function: publishIPv6AddressInfo
 * ------------------------------
 * Relies on the services of the inet_ntop and gethostbyname2 functions to produce
 * the official host name for the supplied one (sometimes the same, and sometimes
 * different) and all of the IPv6 addresses that the supplied host name resolves to.
 */
static void publishIPv6AddressInfo(const string &host)
{
    struct hostent *he = gethostbyname2(host.c_str(), AF_INET6);
    if (he == NULL)
    { // NULL return value means resolution attempt failed
        cout << host << " could not be resolved to an address.  Did you mistype it?" << endl;
        return;
    }

    assert(he->h_addrtype == AF_INET6);
    assert(he->h_length == sizeof(struct in6_addr));

    cout << "Official name is \"" << he->h_name << "\"" << endl;
    cout << "IPv6 Addresses: " << endl;
    struct in6_addr **addressList = (struct in6_addr **)he->h_addr_list;
    while (*addressList != NULL)
    {
        char str[INET6_ADDRSTRLEN];
        cout << "+ " << inet_ntop(AF_INET6, *addressList, str, INET6_ADDRSTRLEN) << endl;
        addressList++;
    }
}
```

```console
myth61$ ./resolve-hostname6
Welcome to the IPv6 address resolver!
Enter a host name: www.facebook.com
Official name is "star-mini.c10r.facebook.com"
IPv6 Addresses:
+ 2a03:2880:f131:83:face:b00c:0:25de
```

---
The three data structures presented below are in place to model the IP address/port
pairs

`sockaddr` is generic type. Below are IPv4, IPv6 version of this.
```cc
struct sockaddr { // generic socket
    unsigned short sa_family; // protocol family for socket
    char sa_data[14]; // rest are just use this 14 bytes
    // address data (and defines full size to be 16 bytes)
};
```

The `sockaddr_in` is used to model IPv4 address/port pairs.
```cc
struct sockaddr_in { // IPv4 socket address record
    unsigned short sin_family; // = AF_INET
    unsigned short sin_port; // port number in network byte (big endian) order (2bytes)
    struct in_addr sin_addr; // like h_addr in struct hostent. unsigned int (4bytes)
    unsigned char sin_zero[8]; // padding. total 2 + 14 bytes!
};
```

The `sockaddr_in6` is used to model IPv6 address/port pairs.
```cc
struct sockaddr_in6 { // IPv6 socket address record
    unsigned short sin6_family; // = AF_INET6
    unsigned short sin6_port; // (2 bytes)
    unsigned int sin6_flowinfo; // beyond our scope (4bytes)
    struct in6_addr sin6_addr; // (16 bytes)
    unsigned int sin6_scope_id; // beyond our scope (4bytes)
};
```

What? why the byte doesn't match? Actually,,,size doesn't matter. They did it for `sockaddr_in`, but `sockaddr` is just used as pointer (it's like abstract class of java):

```cc
struct sockaddr_in serv_addr;
// ... fill out serv_addr ...
connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
```
```cc
struct sockaddr_in6 serv_addr;
// ... fill out serv_addr ...
connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
```
Any function that's provided this generic type will first look at first two bytes (`sa_family` field) before casting it to it's original type.

---

### Example: createClientSocket

Fundamentally, createClientSocket needs to:
- Confirm the host of interest is really on the net by checking to see if it has an IP address. `gethostbyname` does this for us.
- Allocate a new descriptor and configure it to be a socket descriptor. We'll rely on the `socket` system call to do this.
- Construct an instance of a `struct sockaddr_in` that packages the host and port number we're interested in connecting to.
- Associate the freshly allocated socket descriptor with the host/port pair. We'll rely on an aptly named system call called connect to do this.
- Return the fully configured client socket.

```cc
int createClientSocket(const string &host, unsigned short port)
{
    struct hostent *he = gethostbyname(host.c_str());
    if (he == NULL)
        return kClientSocketError;

    int s = socket(AF_INET, SOCK_STREAM, 0); // os will handle packet things..
    if (s < 0)
        return kClientSocketError;

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port); // htons: host to network short
    address.sin_addr = *((struct in_addr *)he->h_addr); // h_addr is defined as h_addr_list[0]

    if (connect(s, (struct sockaddr *)&address, sizeof(address)) == 0)
        return s;

    close(s); // if failed: s is descriptor. You need to close it.
    return kClientSocketError;
}
```
\* socket lives in file descriptor table and it's type is socket.

We call `gethostbyname` first before we call socket, because we want to confirm the
host has a registered IP address—which means it's reachable—before we allocate any
system resources. Recall that `gethostbyname` is intrinsically IPv4. If we wanted to involve IPv6 addresses instead, we would need to use `gethostbyname2`.

The call to socket finds, claims, and returns an unused descriptor. `AF_INET` configures it to be compatible with an IPv4 address, and `SOCK_STREAM` configures it to provide reliable data transport, which basically means the socket will reorder data packets and requests missing or garbled data packets to be resent so as to give the impression that data that is received in the order it's sent.

The first argument could have been `AF_INET6` had we decided to use IPv6 addresses
instead. (Other arguments are possible, but they're less common.) The second argument could have been `SOCK_DGRAM` had we preferred to collect data packets in the order they just happen to arrive and manage missing and garbled data packets ourselves. (Other arguments are possible, though they're less common.)

On big endian machines, `htons` is implemented to return the provided short without modification. On little endian machines (like the myths), `htons` returns a figure constructed by exchanging the two bytes of the incoming short. In addition to htons, Linux also provided `htonl` for four-byte longs, and it also provides `ntohs` and `ntohl` to restore host byte order from network byte ordered figures.


### Example: createServerSocket

```cc
static const int kReuseAddresses = 1;
int createServerSocket(unsigned short port, int backlog)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        return kServerSocketFailure;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &kReuseAddresses, sizeof(int)) < 0)
    {
        close(s);
        return kServerSocketFailure;
    }

    struct sockaddr_in address; // IPv4-style socket address
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (bind(s, (struct sockaddr *)&address, sizeof(address)) == 0 && listen(s, backlog) == 0)
        return s;

    close(s);
    return kServerSocketFailure;
}
```

The call to socket is precisely the same here as it was in createClientSocket. It allocates a descriptor and configures it to be a socket descriptor within the `AF_INET` namespace.

The address of type `struct sockaddr_in` here is configured in much the same way it was in createClientSocket, except that the `sin_addr.s_addr` field should be set to a local IP address, not a remote one. The constant `INADDR_ANY` is used to state that address should represent all local addresses.

The bind call simply assigns the set of local IP addresses represented by address to the provided socket `s`. Because we embedded `INADDR_ANY` within address, bind associates the supplied socket with all local IP addresses. That means once `createServerSocket` has done its job, clients can connect to any of the machine's IP addresses via the specified port.

The `listen` call is what converts the socket to be one that's willing to accept connections via `accept`. The second argument is a queue size limit, which states how many pending connection requests can accumulate and wait their turn to be accepted. If the number of outstanding requests is at the limit, additional requests are simply refused.
