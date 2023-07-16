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
#include <cassert>
#include <iostream>
#include <string>
#include <list>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h> // for fork
#include <signal.h> // for kill
#include <sys/wait.h>
using namespace std;

static bool DEBUG = false;

static STSHJobList joblist; // the one piece of global data we need so signal handlers can access it

static void waitForForegroundProcess(pid_t pid);
static void giveForegroundtc(pid_t pgid)
{
	if ((tcsetpgrp(STDIN_FILENO, pgid) == -1) && (errno != ENOTTY))
		throw STSHException("more serious problem");
}
static void resetForgroundtc()
{
	pid_t pgid = getpgid(getpid());
	giveForegroundtc(pgid);
}

static void handleFg(const pipeline &pipeline)
{
	if (pipeline.commands[0].tokens[0] == NULL)
		throw STSHException("Usage: fg <jobid>.");
	size_t jobNumber;
	try
	{
		const string &token = pipeline.commands[0].tokens[0];
		jobNumber = stoi(token);
	}
	catch (const std::exception &e)
	{
		throw STSHException("Usage: fg <jobid>.");
	}
	if (!joblist.containsJob(jobNumber))
		throw STSHException("fg " + to_string(jobNumber) + ": No such job.");
	STSHJob &job = joblist.getJob(jobNumber);
	pid_t groupID = job.getGroupID();
	if (groupID)
	{
		job.setState(kForeground);
		for (STSHProcess &process : job.getProcesses())
		{
			process.setState(kRunning);
		}
		joblist.synchronize(job);
		killpg(groupID, SIGCONT); // if it were running, it will be ignored
		giveForegroundtc(groupID);
		std::vector<STSHProcess> &processes = job.getProcesses();
		for (STSHProcess &process : processes)
		{
			waitForForegroundProcess(process.getID());
		}
		resetForgroundtc();
		return;
	}
}

static void handleBg(const pipeline &pipeline)
{
	if (pipeline.commands[0].tokens[0] == NULL)
		throw STSHException("Usage: bg <jobid>.");
	size_t jobNumber;
	try
	{
		const string &token = pipeline.commands[0].tokens[0];
		jobNumber = stoi(token);
	}
	catch (const std::exception &e)
	{
		throw STSHException("Usage: bg <jobid>.");
	}
	if (!joblist.containsJob(jobNumber))
		throw STSHException("bg " + to_string(jobNumber) + ": No such job.");
	STSHJob &job = joblist.getJob(jobNumber);
	pid_t groupID = job.getGroupID();
	if (groupID)
	{
		job.setState(kBackground);
		for (STSHProcess &process : job.getProcesses())
		{
			process.setState(kRunning);
		}
		joblist.synchronize(job);
		killpg(groupID, SIGCONT); // if it were running, it will be ignored
		return;
	}
}

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
	case 2:
	{
		handleFg(pipeline);
		break;
	}
	case 3:
	{
		handleBg(pipeline);
		break;
	}
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
			int status;
			pid_t pid = waitpid(-1, &status, WUNTRACED | WCONTINUED | WNOHANG);
    		if (pid <= 0) break;
			assert(joblist.containsProcess(pid));
			STSHJob &job = joblist.getJobWithProcess(pid);
			STSHProcess &process = job.getProcess(pid);
			if (WIFEXITED(status) || WIFSIGNALED(status)) { // exited or terminated
				process.setState(kTerminated);
				if (DEBUG) 
					std::cout << "Child " << pid << " exited or terminated" <<  std::endl;
			} else if (WIFSTOPPED(status)) {
				process.setState(kStopped);
				if (DEBUG) 
					std::cout << "Child " << pid << " stopped by signal " << WSTOPSIG(status) << std::endl;
			} else if (WIFCONTINUED(status)) {
				process.setState(kRunning);
				if (DEBUG) 
					std::cout << "Child " << pid << " continued" << std::endl;
			}
			joblist.synchronize(job);
			waitForForegroundProcess(pid);
		} });
	installSignalHandler(SIGINT, [](int sig)
						 {
							 if (joblist.hasForegroundJob())
							 {
								 pid_t groupID = joblist.getForegroundJob().getGroupID();
								 if (groupID)
								 {
									 killpg(groupID, SIGINT);
								 }
							 } }); // to exit the shell, just type 'exit' (see above)

	installSignalHandler(SIGTSTP, [](int sig)
						 { 
							if (joblist.hasForegroundJob())
							{
								pid_t groupID = joblist.getForegroundJob().getGroupID();
								if (groupID)
								{
									killpg(groupID, SIGTSTP);
								}								
							} });
}

static void toggleSIGCHLDBlock(int how)
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	sigprocmask(how, &mask, NULL);
}

static void blockSIGCHLD()
{
	toggleSIGCHLDBlock(SIG_BLOCK);
}

static void unblockSIGCHLD()
{
	toggleSIGCHLDBlock(SIG_UNBLOCK);
}

static void waitForForegroundProcess(pid_t pid)
{
	sigset_t empty;
	sigemptyset(&empty);
	while (joblist.hasForegroundJob() && joblist.containsProcess(pid) && joblist.getJobWithProcess(pid).getProcess(pid).getState() == kRunning)
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
	pid_t pgid;
	size_t commandSize = p.commands.size();
	int fds[kMaxCommandLength][2];
	for (size_t procNum = 0; procNum < commandSize; procNum++)
	{
		pipe(fds[procNum]);
	}
	for (size_t procNum = 0; procNum < commandSize; procNum++)
	{
		pid_t pid = fork();
		if (procNum == 0)
		{
			pgid = pid;
		}
		if (pid != 0) // parent
		{
			job.addProcess(STSHProcess(pid, p.commands[procNum]));
		}
		if (pid == 0) // child
		{
			unblockSIGCHLD();
			// if read file
			if (procNum == 0 && p.input.size() > 0)
			{
				int fd = open(p.input.c_str(), O_RDONLY);
				if (fd < 0)
					throw STSHException("No such file as an input");
				dup2(fd, STDIN_FILENO);
				close(fd);
			}
			// if output file
			if (procNum == commandSize - 1 && p.output.size() > 0)
			{
				int fd = open(p.output.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
				if (fd < 0)
					throw STSHException("Something went wrong with opening output file.");
				dup2(fd, STDOUT_FILENO);
				close(fd);
			}

			// writer
			if (procNum < commandSize - 1)
				dup2(fds[procNum][1], STDOUT_FILENO);
			// reader
			if (procNum > 0)
				dup2(fds[procNum - 1][0], STDIN_FILENO);
			for (size_t procNum2 = 0; procNum2 < commandSize; procNum2++)
			{
				close(fds[procNum2][0]);
				close(fds[procNum2][1]);
			}
			pid_t selfPid = getpid();
			setpgid(selfPid, pgid);

			char *argv[kMaxArguments + 2] = {NULL};
			argv[0] = const_cast<char *>(p.commands[procNum].command);
			for (unsigned int j = 0; j <= kMaxArguments && p.commands[procNum].tokens[j] != NULL; j++)
			{
				argv[j + 1] = p.commands[procNum].tokens[j];
			}
			int err = execvp(argv[0], argv);
			if (err < 0)
				throw STSHException("Command not found");
		}
	}
	for (size_t procNum = 0; procNum < commandSize; procNum++)
	{
		close(fds[procNum][0]);
		close(fds[procNum][1]);
	}
	// now, only parent exists
	if (p.background)
	{
		unblockSIGCHLD();
		job.setState(kBackground);
		cout << "[" << job.getNum() << "]";
		std::vector<STSHProcess> &processes = job.getProcesses();
		for (STSHProcess &proc : processes)
		{
			cout << " " << proc.getID();
		}
		cout << endl;
	}
	else
	{
		giveForegroundtc(pgid);
		std::vector<STSHProcess> &processes = job.getProcesses();
		for (STSHProcess &proc : processes)
		{
			waitForForegroundProcess(proc.getID());
		}
		resetForgroundtc();
	}
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
							  // cout << joblist << endl;
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