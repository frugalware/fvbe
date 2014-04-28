#include "ui.hpp"

// UI Class Start
int UI::main(int argc, char **argv)
{
	(void) argc;
	(void) argv;

	return 0;
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
