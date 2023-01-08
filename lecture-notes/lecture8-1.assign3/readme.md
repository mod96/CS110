### Problem 1: pipeline (C)

remember that
```
int fds[2];
pipe(fds);
fds[0]; <= reader
fds[1];
```

so like
```
cat testFile.txt | sort
```
, stdout of cat pipe into stdin of sort. What happened here is that cat's stdout went to `fds[1]`, stdin of sort went fo `fds[0]`.

```
void pipeline(char *argv1[], char *argv2[], pid_t pids[]);
```
`pids` have two pids. Standard output of the first is directed to the standard input of th second.

### Problem 2: subprocess (C++)

just an extend of subprocess in lecture


### Problem 3: trace (C++)

captures all the syscalls that it runs, and prints them.

`ptrace` is not a systemcall and it can make another program to stop and capture the information about that program.

In starter's code,

```cc
int main(int argc, char *argv[]) {
    bool simple = false, rebuild = false;
    int numFlags = processCommandLineFlags(simple, rebuild, argv);
    if (argc - numFlags == 1) {
        cout << "Nothing to trace... exiting." << endl;
        return 0;
    }
}
```

```
pid_t pid = fork();
if (pid == 0) {
    ptrace(PTRACE_TRACEME);
    raise(SIGSTOP);
    execvp(argv[numFlags + 1], argv + numFlags + 1);
    return 0;
}
```
before calling execvp, calls `ptrace` to inform the OS that it's happy being manipulated by the parent. And calls `raise` and awaits parental permission to continue. Since `argv[0]` is `./trace`, hurdles using `numFlags + 1`.

That tracer circumvents the code specific to the child and executes the following lines:
```cc
waitpid(pid, &status, 0);
assert(WIFSTOPPED(status));
ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD);
```
The `ptrace` line instructs the OS to set bit 7 of the signal number (to deliver `SIGTRAP | 0x80`) whenever a system call trap is executed.

These work to advance the tracee to run until it's just about to executre the body of a system call:
```cc
while (true) {
    ptrace(PTRACE_SYSCALL, pid, 0, 0);
    waitpid(pit, &status, 0);
    if (WIFSTOPPED(status) && (WSTOPSIG(status) == (SIGTRAP | 0x80))) {
        int syscall = ptrace(PTRACE_PEEKUSER, pid, ORIG_RAX * sizeof(long));
        cout << "syscall(" << syscall << ") = " << flush; 
        break;
    }
}
```
`ptrace(PTRACE_SYSCALL, pid, 0, 0);` continues a stopped tracee until it enters a system call (or is otherwise signaled). If the tracee stops because it's entering a syscall, all others will work.


### Problem 4: farm (C++)


read main function first.