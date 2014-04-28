#include <iostream>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include "newt.hpp"

int UINewt::main(int argc, char **argv)
{
	int code = EXIT_FAILURE;

	// We do not use the arguments in this UI.
	(void) argc;
	(void) argv;

	// Check that the standard file streams are connected to a TTY.
	if(!checkTTY(STDIN_FILENO) || !checkTTY(STDOUT_FILENO) || !checkTTY(STDERR_FILENO))
	{
		return code;
	}

	if(newtInit() != 0)
	{
		printError("Could not initialize the NEWT user interface.");
		return code;
	}

	if(!setSignalInterrupt(SIGWINCH, false))
	{
		printError("Failed to stop SIGWINCH from interrupting system calls.");
		newtFinished();
		return code;
	}

	newtCls();

	newtFinished();

	return code;
}
