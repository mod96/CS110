Our version doesn't work with https.

After you have set up the proxy, you can leave it (if you browse with another browser), as long as you always ssh into the same myth machine each time you work on the assignment. If you change myth machines, you will need to update the proxy settings (always check this first if you run into issues). You should also frequently clear the browser's cache, as it can locally cache elements, too, which means that you might load a page without ever going to your proxy.


If you want to avoid the browser, you can use telnet, instead:

```console
myth51:$ telnet <proxy-server> <proxy-port>
Trying 171.64.15.30...
Connected to myth65.stanford.edu.
Escape character is '^]'.
GET http://api.ipify.org/?format=json HTTP/1.1
Host: api.ipify.org

HTTP/1.0 200 OK
content-length: 23

You're writing a proxy!Connection closed by foreign host.
```

If you want to see how the solution behaves, run `samples/proxy_soln` and then test.

## Version 1: Sequential Proxy

- You will eventually add a ThreadPool to your program, but first, write a sequential version.
- You will be changing the starter code to be a true proxy, that intercepts the requests and passes them on to the intended server. There are three HTTP methods you need to support:
    - GET: request a web page from the server
    - HEAD: exactly like GET, but only requests the headers
    - POST: send data to the website

The request line will look like this:
```
GET http://www.cornell.edu/research/ HTTP/1.1
```
For this example, your program forwards the request to www.cornell.edu, with the first line of the request as follows:
```
GET /research/ HTTP/1.1
```
You already have a fully implemented HTTPRequest class, although you will have to update the `operator<<` function at a later stage.

There are a couple of extra request headers that you will need to add when you forward the page:

- You should add a new request header entity named x­forwarded­proto and set its value to be `http`.  If `x­-forwarded­-proto` is already included in the request header, then simply add it again.
- You should add a new request header entity called `x­-forwarded­-for` and set its value to be the IP address of the requesting client.  If `x-­forwarded­-for` is already present, then you should extend its value into a comma-separated chain of IP addresses the request has passed through before arriving at your proxy. (The IP address of the machine you’re directly hearing from would be appended to the end).
- You need to be familiar with the `header.h/cc` files to utilize the functions, e.g., 
  
```cpp
string xForwardedForStr = requestHeader.getValueAsString("x­forwarded­for");
```
- You need to manually add the extra `x-­forwarded-­for` value, by the way.
  
Most of the code for the sequential version will be in `request­handler.h/cc`, and some in  `request.h/cc`. 

After you've written this version, test all the `HTTP://` sites you can find!

## Version 2: Adding blacklisting and caching

Blacklisting means to block access to certain websites

- You have a `blocked­domains.txt` file that lists domains that should not be let through your proxy. When the server in the blacklist is requested, you should return to the client a status code of `403`, and a payload of "Forbidden Content".
- You should understand the functionality of the (short) `blacklist.cc` file, e.g.,
  
```cpp
if (!blacklist.serverIsAllowed(request.getServer()) { ...
```

If you respond with your own page, use "HTTP/1.0" as the protocol.

Caching means to keep a local copy of a page, so that you do not have to re-request the page from the Internet.
- You should update your `HTTPRequestHandler` to check to see if you've already stored a copy of a request -- if you have, just return it instead of forwarding on! You can use the `HTTPCache` class to do this check (and to add sites, as well).
- If it isn't in the cache, forward on as usual, but if the response indicates that it is cacheable (e.g., `cache.shouldCache(request, response)` ), then you cache it for later.

Make sure you clear your own browser's cache often when testing this functionality, and also clear the program's cache often, as well.

```console
myth51:$ ./proxy --clear-cache
Clearing the cache... wait for it.... done!
Listening for all incoming traffic on port 19419.
```

I put together a tiny web page that simply returns the server time, but it actually allows a cache (not a good thing, but useful for testing): http://ecosimulation.com/cgibin/currentTime.php

## Version 3: Concurrent proxy with blacklisting and caching

- Now is the time to leverage your ThreadPool class (we give you a working version in case yours still has bugs)
- You will be updating the scheduler.h/cc files, which will be scheduled on a limited
amount (64) threads.
- You will be building a scheduler to handle the requests, and you will be writing the
HTTPProxyScheduler class.
    - Keep this simple and straightforward! It should have a single `HTTPRequestHandler`, which already has a single `HTTPBlacklist` and a single `HTTPCache`. You will need to go back and add synchronization directives (e.g., `mutex`es) to your prior code to ensure that you don't have race conditions.
    - You can safely leave the blacklist functions alone, as they never change, but the cache certainly needs to be locked when accessed.
  
You can only have one request open for a given request. If two threads are trying to access the same document, one must wait.

Don't lock the entire cache with a single mutex -- this is slow!
- Instead, you are going to have an array of `997 mutex`es.
    - What?
    - Yes -- every time you request a site, you will tie the request to a particular `mutex`. How, you say? Well, you will hash the request. It is easy: ```size_t requestHash = hashRequest(request);``` (use index of hash mod 997)
    - The hash you get will always be the same for a particular request, so this is how you will ensure that two threads aren't trying to download the same site concurrently. Yes, you will have some collisions with other sites, but it will be rare.
    - You should update the `cache.h/cc` files to make it easy to get the hash and the associated mutex, and then return the mutex to the calling function.

- It is fine to have different requests adding their cached information to the cache or checking the cache, because it is thread-safe in that respect. You should never have the same site trying to update the cache at the same time.

The `client­socket.h/cc` files have been updated to include thread-safe versions of their functions, so no need to worry about that.


## Version 4: Concurrent proxy with blacklisting, caching, and proxy chaining

Proxy chaining is where your proxy will itself use another proxy. 
- Is this a real thing? Yes! Some proxies rely on other proxies to do the heavy lifting, while they add more functionality (better caching, more blacklisting, etc.). Some proxies do this to further anonymize the client.
- Example:
    - On myth63, we can start a proxy as normal, on a particular port: 
  
```console
myth63:$ samples/proxy_soln --port 12345
Listening for all incoming traffic on port 12345.

```

- On myth65, we can start another proxy that will forward all requests to the myth63 proxy:
  
```console
myth65:$ samples/proxy_soln --proxy-server myth63.stanford.edu --proxy-port 12345
Listening for all incoming traffic on port 19419.
Requests will be directed toward another proxy at myth63.stanford.edu:12345.

```
Now, all requests will go through both proxies (unless cached!)

You will have to update a number of method signatures to include the possibility of a secondary proxy.

If you are going to forward to another proxy:
- Check to see if you've got a cached request -- if so, return it
- If you notice a cycle of proxies, respond with a status code of `504`. 
  - How do you know you have a chain? That's why you have the "x­-forwarded-for" header! You analyze that list to see if you are about to create a chain.
- If not, forward the request exactly, with the addition of your "x-­forwarded-proto" and "x-­forwarded­-for" headers.
  
We provide you with a `run­-proxy-­farm.py` program that can manage a chain of proxies (but it doesn't check for cycles -- you would need to modify the python code to do that).

|file|changes|
|---|---|
|cache.c|very minor|
|cache.h|very minor|
|proxy.cc|very minor|
|request.cc|minor|
|request.h|minor|
|request-handler.cc|major|
|request-handler.h|major|
|scheduler.cc|minor|
|scheduler.h|very minor|

















