# Lecture 09: Introduction to Threads

A thread is an independent execution sequence within a single process. Operating systems and programming languages generally allow processes to run two or more functions simultaneously via threading.

The stack segment is subdivided into multiple miniature stacks, one for each thread. The thread manager time slices and switches between threads in much the same way that the OS scheduler switches between processes. In fact, threads are often called lightweight processes.

Each thread maintains its own stack, but all threads share the same text, data, and heap segments. It's easier to support communication between threads, because they run in the same virtual address space. But there's no memory protection, since virtual address space is shared. Race conditions and deadlock threats need to be mitigated, and debugging can be diﬃcult. Many bugs are hard to reproduce, since thread scheduling isn't predictable.

ANSI C doesn't provide native support for threads. But `pthreads`, which comes with all standard UNIX and Linux installations of gcc, provides thread support, along with other related concurrency directives.

The primary `pthreads` data type is the `pthread_t`, which is an opaque type used to manage the execution of a function within its own thread of execution.
The only pthreads functions we'll need (before formally transitioning to C++ threads) are `pthread_create` and `pthread_join` (similar to waitpid). [man - pthread_create](https://man7.org/linux/man-pages/man3/pthread_create.3.html) / [man - pthread_join](https://man7.org/linux/man-pages/man3/pthread_join.3.html)

```c
static void *recharge(void *args) {
    printf("I recharge by spending time alone.\n");
    return NULL;
}

static const size_t kNumIntroverts = 6;
int main(int argc, char *argv[]) {
    printf("Let's hear from %zu introverts.\n", kNumIntroverts);
    pthread_t introverts[kNumIntroverts];
    for (size_t i = 0; i < kNumIntroverts; i++)
        pthread_create(&introverts[i], NULL, recharge, NULL);
    for (size_t i = 0; i < kNumIntroverts; i++)
        pthread_join(introverts[i], NULL);
    printf("Everyone's recharged!\n");
    return 0;
}
```
```console
cgregg@myth57$ ./recharge
Let's hear from 6 introverts.
I recharge by spending time alone.
I recharge by spending time alone.
I recharge by spending time alone.
I recharge by spending time alone.
I recharge by spending time alone.
I recharge by spending time alone.
Everyone's recharged!
cgregg@myth57$
```
```c
static const char *kFriends[] = {
    "Jack", "Michaela", "Luis", "Richard", "Jordan", "Lisa",
    "Imaginary"
};
static const size_t kNumFriends = sizeof(kFriends)/sizeof(kFriends[0]) - 1; // count excludes imaginary friend!

static void *meetup(void *args) {
    const char *name = kFriends[*(size_t *)args];
    printf("Hey, I'm %s.  Empowered to meet you.\n", name);
    return NULL;
}

int main() {
    printf("Let's hear from %zu friends.\n", kNumFriends);
    pthread_t friends[kNumFriends];
    for (size_t i = 0; i < kNumFriends; i++)
        pthread_create(&friends[i], NULL, meetup, &i);
    for (size_t j = 0; j < kNumFriends; j++)
        pthread_join(friends[j], NULL);
    printf("Is everyone accounted for?\n");
    return 0;
}
```
```console
cgregg@myth57$ ./confused-friends
Let's hear from 6 friends.
Hey, I'm Jordan.  Empowered to meet you.
Hey, I'm Lisa.  Empowered to meet you.
Hey, I'm Jordan.  Empowered to meet you.
Hey, I'm Imaginary.  Empowered to meet you.
Hey, I'm Jordan.  Empowered to meet you.
Hey, I'm Imaginary.  Empowered to meet you.
Is everyone accounted for?
cgregg@myth57$ ./confused-friends
Let's hear from 6 friends.
Hey, I'm Richard.  Empowered to meet you.
Hey, I'm Richard.  Empowered to meet you.
Hey, I'm Richard.  Empowered to meet you.
Hey, I'm Jordan.  Empowered to meet you.
Hey, I'm Lisa.  Empowered to meet you.
Hey, I'm Imaginary.  Empowered to meet you.
Is everyone accounted for?
cgregg@myth57$ ./confused-friends
Let's hear from 6 friends.
Hey, I'm Jordan.  Empowered to meet you.
Hey, I'm Lisa.  Empowered to meet you.
Hey, I'm Imaginary.  Empowered to meet you.
Hey, I'm Imaginary.  Empowered to meet you.
Hey, I'm Imaginary.  Empowered to meet you.
Hey, I'm Imaginary.  Empowered to meet you.
Is everyone accounted for?
cgregg@myth57$
```
So.. why?

Reason is that `&i` is not changed during `pthread_create` and value is changing. Fortunately, the fix is simple

```c
static const char *kFriends[] = {
  "Jack", "Michaela", "Luis", "Richard", "Jordan", "Lisa",
  "Imaginary"
};

static const size_t kNumFriends = sizeof(kFriends)/sizeof(kFriends[0]) - 1; // count excludes imaginary friend!

static void *meetup(void *args) {
  const char *name = args;
  printf("Hey, I'm %s.  Empowered to meet you.\n", name);
  return NULL;
}

int main() {
  printf("%zu friends meet.\n", kNumFriends);
  pthread_t friends[kNumFriends];
  for (size_t i = 0; i < kNumFriends; i++)
    pthread_create(&friends[i], NULL, meetup, (void *) kFriends[i]); // this line is different than before, too
  for (size_t i = 0; i < kNumFriends; i++)
    pthread_join(friends[i], NULL);
  printf("All friends are real!\n");
  return 0;
}
```
```console
cgregg@myth57$ ./friends
6 friends meet.
Hey, I'm Jack.  Empowered to meet you.
Hey, I'm Michaela.  Empowered to meet you.
Hey, I'm Luis.  Empowered to meet you.
Hey, I'm Richard.  Empowered to meet you.
Hey, I'm Jordan.  Empowered to meet you.
Hey, I'm Lisa.  Empowered to meet you.
All friends are real!
cgregg@myth57$ ./friends
6 friends meet.
Hey, I'm Jack.  Empowered to meet you.
Hey, I'm Luis.  Empowered to meet you.
Hey, I'm Michaela.  Empowered to meet you.
Hey, I'm Richard.  Empowered to meet you.
Hey, I'm Jordan.  Empowered to meet you.
Hey, I'm Lisa.  Empowered to meet you.
All friends are real!
cgregg@myth57$ ./friends
6 friends meet.
Hey, I'm Jack.  Empowered to meet you.
Hey, I'm Michaela.  Empowered to meet you.
Hey, I'm Richard.  Empowered to meet you.
Hey, I'm Jordan.  Empowered to meet you.
Hey, I'm Luis.  Empowered to meet you.
Hey, I'm Lisa.  Empowered to meet you.
All friends are real!
cgregg@myth57$
```

# Lecture 10: From C threads to C++ threads

Rather than deal with pthreads as a platform-specific extension of C, I'd rather use a thread package that's officially integrated into the language itself. As of 2011, C++ provides support for [threading](https://en.cppreference.com/w/cpp/thread/thread) and many synchronization directives.

see [here](https://gist.github.com/Wallace-dyfq/2817479ce0309791e52cd11012116849) for ostreamlock (it used mutex) - used it since operator<<, unlike printf, isn't thread-safe.

```cpp
#include <iostream>
#include <thread>
#include "ostreamlock.h"  // oslock, osunlock

static void recharge() {
    cout << oslock << "I recharge by spending time alone." << endl << osunlock; 
}
    
static const size_t kNumIntroverts = 6;
int main(int argc, char *argv[]) {
  cout << "Let's hear from " << kNumIntroverts << " introverts." << endl   ;   
  thread introverts[kNumIntroverts]; // declare array of empty thread handles
  for (thread& introvert: introverts)
     introvert = thread(recharge);    // move anonymous threads into empty handles
  for (thread& introvert: introverts)
     introvert.join();    
  cout << "Everyone's recharged!" << endl;
  return 0;
}
```

note that thread has been created and fully transplanted to the contents of the thread on the left side. (the left and right thread objects are effectively swapped.) This is an important distinction, because a traditional operator= would produce a second working copy of the same thread, and we don't want that.


Thread routines can accept any number of arguments using variable argument lists. (see [constructor of thread](https://en.cppreference.com/w/cpp/thread/thread/thread))

```cpp
static void greet(size_t id) {
  for (size_t i = 0; i < id; i++) {
    cout << oslock << "Greeter #" << id << " says 'Hello!'" << endl << osunlock;
    struct timespec ts = {
      0, random() % 1000000000
    };
    nanosleep(&ts, NULL);
  }
  cout << oslock << "Greeter #" << id << " has issued all of his hellos, " 
       << "so he goes home!" << endl << osunlock;
}

static const size_t kNumGreeters = 6;
int main(int argc, char *argv[]) {
  cout << "Welcome to Greetland!" << endl;
  thread greeters[kNumGreeters];
  for (size_t i = 0; i < kNumGreeters; i++) greeters[i] = thread(greet, i + 1);
  for (thread& greeter: greeters) greeter.join();
  cout << "Everyone's all greeted out!" << endl;
  return 0;
}

```

* Thread is not well mixed with Process. Thread has it's own id, but not like pid. Generally we don't join another thread inside a thread. We don't fork inside a thread. Maybe you can generate thread inside some child processes.

### Example: Airline

Consider the scenario where 10 ticket agents answer telephones at United Airlines to jointly sell 250 airline tickets. Each ticket agent answers the telephone, and each telephone call always leads to the sale of precisely one ticket. 

Rather than requiring each ticket agent sell 10% of the tickets, we'll account for the possibility that some ticket sales are more time consuming than others, some ticket agents need more time in between calls, etc. Instead, we'll require that all ticket agents keep answering calls and selling tickets until all have been sold.

First attempt is like this:
```cc
static void ticketAgent(size_t id, size_t& remainingTickets) {
  while (remainingTickets > 0) {
    handleCall(); // sleep for a small amount of time to emulate conversation time.
    remainingTickets--;  // this is not atomic
    cout << oslock << "Agent #" << id << " sold a ticket! (" << remainingTickets 
     << " more to be sold)." << endl << osunlock;
    if (shouldTakeBreak()) // flip a biased coin
      takeBreak();         // if comes up heads, sleep for a random time to take a break
  }
  cout << oslock << "Agent #" << id << " notices all tickets are sold, and goes home!" 
       << endl << osunlock;
}

int main(int argc, const char *argv[]) {
  thread agents[10];
  size_t remainingTickets = 250;
  for (size_t i = 0; i < 10; i++)
    agents[i] = thread(ticketAgent, 101 + i, ref(remainingTickets));
  for (thread& agent: agents) agent.join();
  cout << "End of Business Day!" << endl;
  return 0;
}
```
```console
cgregg@myth55$ ./confused-ticket-agents 
Agent #110 sold a ticket! (249 more to be sold).
Agent #104 sold a ticket! (248 more to be sold).
Agent #106 sold a ticket! (247 more to be sold).
// some 245 lines omitted for brevity 
Agent #107 sold a ticket! (1 more to be sold).
Agent #103 sold a ticket! (0 more to be sold).
Agent #105 notices all tickets are sold, and goes home!
Agent #104 notices all tickets are sold, and goes home!
Agent #108 sold a ticket! (4294967295 more to be sold).
Agent #106 sold a ticket! (4294967294 more to be sold).
Agent #102 sold a ticket! (4294967293 more to be sold).
Agent #101 sold a ticket! (4294967292 more to be sold).
// carries on for a very, very, very long time
```
**First Problem:**
If a thread evaluates remainingTickets > 0 to be true and commits to selling a ticket, the ticket might not be there by the time it executes the decrement. That's because the thread may be swapped off the
CPU after the decision to sell
but before the sale, and during
the dead time, other threads—
perhaps the nine others—all
might get the CPU and do
precisely the same thing.

**Second Problem:**
remainingTickets itself isn't even thread-safe. C++ statements aren't inherently atomic. Virtually all C++ statements—even ones as simple as remainingTickets--—compile to multiple assembly code instructions.

g++ on the myths compiles `remainingTickets--` to five assembly code instructions, as with:

```assembly
0x0000000000401a9b <+36>:    mov    -0x20(%rbp),%rax
0x0000000000401a9f <+40>:    mov    (%rax),%eax
0x0000000000401aa1 <+42>:    lea    -0x1(%rax),%edx
0x0000000000401aa4 <+45>:    mov    -0x20(%rbp),%rax
0x0000000000401aa8 <+49>:    mov    %edx,(%rax)
```

One solution: provide a directive that allows a thread to ask that it not be swapped off the CPU while it's within a block of code that should be executed transactionally. (hardware lock) **That, however, is not an option, and shouldn't be.** That would grant too much power to threads, which could abuse the option and block other threads from running for an indeterminate amount of time.

The other option is to rely on a concurrency directive that can be used to prevent more than one thread from being anywhere in the same critical region at one time. That concurrency directive is the mutex, and in C++ it looks like this:
```cc
class mutex {
public:
  mutex();        // constructs the mutex to be in an unlocked state
  void lock();    // acquires the lock on the mutex, blocking until it's unlocked
  void unlock();  // releases the lock and wakes up another threads trying to lock it
};
```

The name mutex is just a contraction of the words mutual and exclusion. It's so named because its primary use it to mark the boundaries of a critical region—that is, a stretch of code where at most one thread is permitted to be at any one moment.

The constructor initializes the mutex to be in an unlocked state. The lock method will eventually acquire a lock on the mutex. If the mutex is in an unlocked state, lock will lock it and return immediately. If multiple threads try to lock the mutex, only one can win and that thread will do the job.

If the mutex is in a locked state (presumably because another thread called lock but has yet to unlock), lock will pull the calling thread off the CPU and render it ineligible for processor time until notified the lock on the mutex was released. The unlock method will release the lock on a mutex. The only thread qualified to release the lock on the mutex is the one that holds the lock.

```cc
static void ticketAgent(size_t id, size_t& remainingTickets, mutex& ticketsLock) {
  while (true) {
    ticketsLock.lock();
    if (remainingTickets == 0) break;
    handleCall();
    remainingTickets--;
    cout << oslock << "Agent #" << id << " sold a ticket! (" << remainingTickets 
         << " more to be sold)." << endl << osunlock;
    ticketsLock.unlock();
    if (shouldTakeBreak()) takeBreak();
  }
  ticketsLock.unlock();
  cout << oslock << "Agent #" << id << " notices all tickets are sold, and goes home!" 
       << endl << osunlock;
}

int main(int argc, const char *argv[]) {
  size_t remainingTickets = 250;
  mutex ticketsLock;
  thread agents[10];
  for (size_t i = 0; i < 10; i++)
    agents[i] = thread(ticketAgent, 101 + i, ref(remainingTickets), ref(ticketsLock));
  for (thread& agent: agents) agent.join();
  cout << "End of Business Day!" << endl;
  return 0;
}
```
The way we've set it up, only one ticket agent can sell at a time! Yes, there is some parallelism with the break-taking, but the ticket-selling is serialized.
```cc
static void ticketAgent(size_t id, size_t& remainingTickets, mutex& ticketsLock) {
  while (true) {
    ticketsLock.lock();
    if (remainingTickets == 0) break;
    remainingTickets--;
    ticketsLock.unlock();
    handleCall(); // assume this sale is successful
    cout << oslock << "Agent #" << id << " sold a ticket! (" << remainingTickets 
         << " more to be sold)." << endl << osunlock;
    if (shouldTakeBreak()) takeBreak();
  }
  ticketsLock.unlock();
  cout << oslock << "Agent #" << id << " notices all tickets are sold, and goes home!" 
       << endl << osunlock;
}
```

This is much faster. Now, we allow each agent to grab an available ticket, but they must sell it! If an agent did not sell the ticket, some other agents might go home early. But, that might be okay for our model.

So, **don't wrap around the code that doesn't matter with the lock!**