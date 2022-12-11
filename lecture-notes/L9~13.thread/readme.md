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