#pragma once

#include "ui.hpp"

class UINewt : UI
{
	private:
		bool checkTTY(int fd);

	public:
		int main(int argc, char **argv);
};