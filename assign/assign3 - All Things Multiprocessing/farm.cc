#include <cassert>
#include <ctime>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include "subprocess.h"

using namespace std;

struct worker
{
  worker() {}
  worker(char *argv[]) : sp(subprocess(argv, true, false)), available(false) {}
  subprocess_t sp;
  bool available;
};

static const size_t kNumCPUs = sysconf(_SC_NPROCESSORS_ONLN);
// restore static keyword once you start using these, commented out to suppress compiler warning
static vector<worker> workers(kNumCPUs);
static size_t numWorkersAvailable = 0;

static void markWorkersAsAvailable(int sig)
{
  while (true)
  {
    pid_t pid = waitpid(-1, NULL, WNOHANG | WUNTRACED);
    if (pid <= 0)
      break;
    for (size_t i = 0; i < kNumCPUs; i++)
    {
      if (workers[i].sp.pid == pid)
      {
        workers[i].available = true;
        numWorkersAvailable++;
        break;
      }
    }
  }
}

// restore static keyword once you start using it, commented out to suppress compiler warning
static const char *kWorkerArguments[] = {"./factor.py", "--self-halting", NULL};
static void spawnAllWorkers()
{
  cout << "There are this many CPUs: " << kNumCPUs << ", numbered 0 through " << kNumCPUs - 1 << "." << endl;
  for (size_t i = 0; i < kNumCPUs; i++)
  {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(i, &set);
    workers[i] = worker(const_cast<char **>(kWorkerArguments));
    sched_setaffinity(workers[i].sp.pid, sizeof(cpu_set_t), &set);
    cout << "Worker " << workers[i].sp.pid << " is set to run on CPU " << i << "." << endl;
  }
}

static void toggleSIGCHLDBlock(int how)
{
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigprocmask(how, &mask, NULL);
}

// restore static keyword once you start using it, commented out to suppress compiler warning
static size_t getAvailableWorker()
{
  toggleSIGCHLDBlock(SIG_BLOCK);
  sigset_t empty;
  sigemptyset(&empty);
  while (numWorkersAvailable == 0)
  {
    sigsuspend(&empty);
  }
  size_t res = 0;
  for (size_t i = 0; i < kNumCPUs; i++)
  {
    if (workers[i].available)
    {
      numWorkersAvailable--;
      workers[i].available = false;
      res = i;
      break;
    }
  }
  toggleSIGCHLDBlock(SIG_UNBLOCK);
  return res;
}

static void broadcastNumbersToWorkers()
{
  while (true)
  {
    string line;
    getline(cin, line);
    if (cin.fail())
      break;
    size_t endpos;
    try
    {
      long long num = stoll(line, &endpos);
      if (endpos != line.size())
        break;
      size_t idx = getAvailableWorker();
      struct worker availableWorker = workers[idx];
      kill(availableWorker.sp.pid, SIGCONT);
      dprintf(availableWorker.sp.supplyfd, "%lld\n", num);
    }
    catch (invalid_argument const &ex)
    {
      std::cout << "std::invalid_argument::what(): " << ex.what() << '\n';
      return;
    }
    catch (out_of_range const &ex)
    {
      std::cout << "std::out_of_range::what(): " << ex.what() << '\n';
      return;
    }
  }
}

static void waitForAllWorkers()
{
  toggleSIGCHLDBlock(SIG_BLOCK);
  sigset_t empty;
  sigemptyset(&empty);
  while (numWorkersAvailable < kNumCPUs)
  {
    sigsuspend(&empty);
  }
  toggleSIGCHLDBlock(SIG_UNBLOCK);
}

static void closeAllWorkers()
{
  signal(SIGCHLD, SIG_DFL);
  for (size_t i = 0; i < kNumCPUs; i++)
  {
    struct worker availableWorker = workers[i];
    close(availableWorker.sp.supplyfd);
    kill(availableWorker.sp.pid, SIGCONT);
  }
  for (size_t i = 0; i < kNumCPUs; i++)
  {
    int status;
    waitpid(workers[i].sp.pid, &status, 0);
    // cout << "Worker " << workers[i].sp.pid << " exited with state " << WEXITSTATUS(status) << "." << endl;
  }
}

int main(int argc, char *argv[])
{
  signal(SIGCHLD, markWorkersAsAvailable);
  spawnAllWorkers();
  broadcastNumbersToWorkers();
  waitForAllWorkers();
  closeAllWorkers();
  return 0;
}
