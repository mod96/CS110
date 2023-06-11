/**
 * File: stsh.cc
 * -------------
 * Defines the entry point of the stsh executable.
 */

#include "stsh-parser/stsh-parse.h"
#include "stsh-parser/stsh-readline.h"
#include "stsh-parser/stsh-parse-exception.h"
#include "stsh-signal.h"
#include "stsh-job-list.h"
#include "stsh-job.h"
#include "stsh-process.h"
#include <cstring>
#include <iostream>
#include <string>
#include <list>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h> // for fork
#include <signal.h> // for kill
#include <sys/wait.h>
using namespace std;

static STSHJobList joblist; // the one piece of global data we need so signal handlers can access it

/**
 * Function: handleBuiltin
 * -----------------------
 * Examines the leading command of the provided pipeline to see if
 * it's a shell builtin, and if so, handles and executes it.  handleBuiltin
 * returns true if the command is a builtin, and false otherwise.
 */
static const string kSupportedBuiltins[] = {"quit", "exit", "fg", "bg", "slay", "halt", "cont", "jobs"};
static const size_t kNumSupportedBuiltins = sizeof(kSupportedBuiltins) / sizeof(kSupportedBuiltins[0]);
static bool handleBuiltin(const pipeline &pipeline)
{
	const string &command = pipeline.commands[0].command;
	auto iter = find(kSupportedBuiltins, kSupportedBuiltins + kNumSupportedBuiltins, command);
	if (iter == kSupportedBuiltins + kNumSupportedBuiltins)
		return false;
	size_t index = iter - kSupportedBuiltins;

	switch (index)
	{
	case 0:
	case 1:
		exit(0);
	case 7:
		cout << joblist;
		break;
	default:
		throw STSHException("Internal Error: Builtin command not supported."); // or not implemented yet
	}

	return true;
}

/**
 * Function: installSignalHandlers
 * -------------------------------
 * Installs user-defined signals handlers for four signals
 * (once you've implemented signal handlers for SIGCHLD,
 * SIGINT, and SIGTSTP, you'll add more installSignalHandler calls) and
 * ignores two others.
 */
static void installSignalHandlers()
{
	installSignalHandler(SIGQUIT, [](int sig)
						 { exit(0); });
	installSignalHandler(SIGTTIN, SIG_IGN);
	installSignalHandler(SIGTTOU, SIG_IGN);
	installSignalHandler(SIGCHLD, [](int sig)
						 {
		while (true) {
			pid_t pid = waitpid(-1, NULL, WNOHANG);
    		if (pid <= 0) break;
			STSHJob &job = joblist.getJobWithProcess(pid);
			job.getProcess(pid).setState(kTerminated);
			joblist.synchronize(job);
		} });
}

static void toggleSIGCHLDBlock(int how)
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	sigprocmask(how, &mask, NULL);
}

void blockSIGCHLD()
{
	toggleSIGCHLDBlock(SIG_BLOCK);
}

void unblockSIGCHLD()
{
	toggleSIGCHLDBlock(SIG_UNBLOCK);
}

static void waitForForegroundProcess(pid_t pid)
{
	sigset_t empty;
	sigemptyset(&empty);
	while (joblist.containsProcess(pid))
	{
		sigsuspend(&empty);
	}
	unblockSIGCHLD();
}

/**
 * Function: createJob
 * -------------------
 * Creates a new job on behalf of the provided pipeline.
 */
static void createJob(const pipeline &p)
{
	blockSIGCHLD();
	STSHJob &job = joblist.addJob(kForeground);
	pid_t pid = fork();
	if (pid != 0)
	{
		job.addProcess(STSHProcess(pid, p.commands[0]));
	}
	if (pid == 0)
	{
		unblockSIGCHLD();
		pid_t selfPid = getpid();
		setpgid(selfPid, selfPid);

		const char *command = p.commands[0].command;
		const char *const *tokens = p.commands[0].tokens;
		int tokensSize = 0;
		while (tokens[tokensSize] != nullptr)
			tokensSize++;

		char **argv = new char *[1 + tokensSize]();
		argv[0] = new char[strlen(command) + 1]();
		strcpy(argv[0], command);
		for (short i = 0; i < tokensSize; i++)
		{
			argv[i + 1] = new char[strlen(tokens[i]) + 1]();
			strcpy(argv[i + 1], tokens[i]);
		}
		execvp(argv[0], argv);
	}
	waitForForegroundProcess(pid);
}

/**
 * Function: main
 * --------------
 * Defines the entry point for a process running stsh.
 * The main function is little more than a read-eval-print
 * loop (i.e. a repl).
 */
int main(int argc, char *argv[])
{
	pid_t stshpid = getpid();
	installSignalHandlers();
	rlinit(argc, argv); // configures stsh-readline library so readline works properly
	while (true)
	{
		string line;
		if (!readline(line))
			break;
		if (line.empty())
			continue;
		try
		{
			pipeline p(line);
			bool builtin = handleBuiltin(p);
			if (!builtin)
				createJob(p); // createJob is initially defined as a wrapper around cout << p;
		}
		catch (const STSHException &e)
		{
			cerr << e.what() << endl;
			if (getpid() != stshpid)
				exit(0); // if exception is thrown from child process, kill it
		}
	}

	return 0;
}