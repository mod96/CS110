/**
 * File: subprocess.cc
 * -------------------
 * Presents the implementation of the subprocess routine.
 */
#include <sstream>
#include "subprocess.h"
using namespace std;

void __pipe(int fds[2]) throw()
{
  if (pipe(fds) < 0)
  {
    switch (errno)
    {
    case EFAULT:
      throw SubprocessException("pipefd is not valid.");
    case EINVAL:
      throw SubprocessException("(pipe2()) Invalid value in flags.");
    case EMFILE:
      throw SubprocessException("The per-process limit on the number of open file descriptors has been reached.");
    case ENFILE:
      throw SubprocessException("The system-wide limit on the total number of open files has been reached.");
    default:
      throw SubprocessException("unknown error from pipe()");
    }
  }
}

pid_t __fork() throw()
{
  pid_t pid = fork();
  if (pid < 0)
  {
    switch (errno)
    {
    case EAGAIN:
      throw SubprocessException("A system-imposed limit on the number of threads was encountered.");
    case ENOMEM:
      throw SubprocessException("fork() failed to allocate the necessary kernel structures because memory is tight.");
    case ENOSYS:
      throw SubprocessException("fork() is not supported on this platform (for example, hardware without a Memory-Management Unit).");
    default:
      throw SubprocessException("unknown error from fork()");
    }
  }
  return pid;
}

void __close(int fd) throw()
{
  if (close(fd) < 0)
  {
    switch (errno)
    {
    case EBADF:
      throw SubprocessException("fd isn't a valid open file descriptor.");
    case EINTR:
      throw SubprocessException("The close() call was interrupted by a signal; see signal(7).");
    case EIO:
      throw SubprocessException("An I/O error occurred.");
    case EDQUOT:
      throw SubprocessException("On  NFS,  these  errors  are  not normally reported against the first write which exceeds the available storage space, but instead against a subsequent write(2), fsync(2), or close().");
    default:
      throw SubprocessException("unknown error from close()");
    }
  }
}

void __dup2(int oldfd, int newfd) throw()
{
  if (dup2(oldfd, newfd) < 0)
  {
    switch (errno)
    {
    case EBADF:
      throw SubprocessException("oldfd isn't an open file descriptor.");
    case EBUSY:
      throw SubprocessException("(Linux only) This may be returned by dup2() or dup3() during a race condition with open(2) and dup().");
    case EINTR:
      throw SubprocessException("The dup2() or dup3() call was interrupted by a signal; see signal(7).");
    case EMFILE:
      throw SubprocessException("The per-process limit on the number of open file descriptors has been reached (see  the  discussion  of RLIMIT_NOFILE in getrlimit(2)).");
    default:
      throw SubprocessException("unknown error from dup2()");
    }
  }
}

void __execvp(const char *file, char *const argv[]) throw()
{
  if (execvp(file, argv) < 0)
  {
    std::ostringstream oss;
    oss << "An error occured from execvp() with errorno : " << errno;
    throw SubprocessException(oss.str());
  }
}

subprocess_t subprocess(char *argv[], bool supplyChildInput, bool ingestChildOutput) throw()
{
  int fdsSupply[2];
  int fdsIngest[2];
  if (supplyChildInput)
  {
    __pipe(fdsSupply);
  }

  if (ingestChildOutput)
  {
    __pipe(fdsIngest);
  }
  subprocess_t process = {__fork(), kNotInUse, kNotInUse};
  if (process.pid == 0)
  { // is child
    if (supplyChildInput)
    {
      __close(fdsSupply[1]);
      __dup2(fdsSupply[0], STDIN_FILENO);
      __close(fdsSupply[0]);
    }

    if (ingestChildOutput)
    {
      __close(fdsIngest[0]);
      __dup2(fdsIngest[1], STDOUT_FILENO);
      __close(fdsIngest[1]);
    }

    __execvp(argv[0], argv);
  }
  if (supplyChildInput)
  {
    process.supplyfd = fdsSupply[1];
    __close(fdsSupply[0]);
  }
  if (ingestChildOutput)
  {
    process.ingestfd = fdsIngest[0];
    __close(fdsIngest[1]);
  }
  return process;
}
