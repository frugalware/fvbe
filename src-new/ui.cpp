#include <iostream>
#include <csignal>
#include <cstring>
#include "ui.hpp"

// UI Class Start
void UI::printError(const std::string &line)
{
	std::cerr << line << std::endl;
}

bool UI::setSignalInterrupt(int sig, bool interrupt)
{
	struct sigaction sig_handler;

	memset(&sig_handler, 0, sizeof(struct sigaction));

	// Get the old signal handler.
	if(sigaction(sig, NULL, &sig_handler) == -1)
	{
		printError("Failed to get old signal handler.");
		return false;
	}

	if(interrupt && (sig_handler.sa_flags & SA_RESTART))
	{
		sig_handler.sa_flags ^= SA_RESTART;
	}
	else if(!interrupt && !(sig_handler.sa_flags & SA_RESTART))
	{
		sig_handler.sa_flags |= SA_RESTART;
	}

	// Set the new signal handler.
	if(sigaction(sig, &sig_handler, NULL) == -1)
	{
		printError("Failed to set new signal handler.");
		return false;
	}

	return true;
}
// UI Class End

// Widget Class Start
const unsigned char Widget::ALIGN_LEFT        = (1 << 0);
const unsigned char Widget::ALIGN_RIGHT       = (1 << 1);
const unsigned char Widget::ALIGN_TOP         = (1 << 2);
const unsigned char Widget::ALIGN_BOTTOM      = (1 << 3);

const unsigned char Widget::EXPAND_HORIZONTAL = (1 << 0);
const unsigned char Widget::EXPAND_VERTICAL   = (1 << 1);
// Widget Class End
