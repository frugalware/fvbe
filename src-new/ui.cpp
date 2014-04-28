#include <iostream>
#include "ui.hpp"

// UI Class Start
void UI::printError(const std::string &line)
{
	std::cerr << line << std::endl;
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
