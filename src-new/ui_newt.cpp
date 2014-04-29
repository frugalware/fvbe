#include <iostream>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include "main.hpp"
#include "ui_newt.hpp"

int UINewt::main(int /*argc*/, char ** /*argv*/)
{
	int code = EXIT_FAILURE;

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

extern "C" UI *ui_new()
{
	UINewt *ui = new UINewt();

	return dynamic_cast <UI *> (ui);
}

extern "C" void ui_delete(UI *p)
{
	UINewt *ui = dynamic_cast <UINewt *> (p);

	delete ui;
}

#ifndef NDEBUG
static void test_shared_exports() __attribute__((unused));

static void test_shared_exports()
{
	// Force compiler to validate the exported C functions.
	__attribute__((unused)) ui_new_t a = ui_new;
	__attribute__((unused)) ui_delete_t b = ui_delete;
}
#endif
