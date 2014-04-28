#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include "newt.hpp"

bool UINewt::checkTTY(int fd)
{
	std::string fd_name;

	switch(fd)
	{
		case STDIN_FILENO:
			fd_name = "stdin";
			break;
		
		case STDOUT_FILENO:
			fd_name = "stdout";
			break;
		
		case STDERR_FILENO:
			fd_name = "stderr";
			break;
	}

	if(!isatty(fd))
	{
		printError(fd_name + " is not a tty.");
		return false;		
	}

	return true;
}

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

	return code;
}
