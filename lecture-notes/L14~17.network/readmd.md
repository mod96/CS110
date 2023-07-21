# Lecture 15: Introduction to Networking

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
    time_t rawtime;
    time(&rawtime);
    struct tm *ptm = gmtime(&rawtime);
    char timestr[128]; // more than big enough
    /* size_t len = */ strftime(timestr, sizeof(timestr), "%c\n", ptm);
    size_t numBytesWritten = 0, numBytesToWrite = strlen(timestr);
    while (numBytesWritten < numBytesToWrite) {
        numBytesWritten += write(client, 
                                timestr + numBytesWritten,
                                numBytesToWrite - numBytesWritten);
    }
    close(client);
}
```




