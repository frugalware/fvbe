#pragma once

#include <string>

class UI
{
	public:
		void printError(const std::string &line);
		virtual int main(int argc, char **argv);
};

class Widget
{
	public:

	static const unsigned char ALIGN_LEFT;
	static const unsigned char ALIGN_RIGHT;
	static const unsigned char ALIGN_TOP;
	static const unsigned char ALIGN_BOTTOM;

	static const unsigned char EXPAND_HORIZONTAL;
	static const unsigned char EXPAND_VERTICAL;
};

class Label : Widget
{
	protected:
		std::string text;

	public:
		virtual void setText(const std::string &newText);
};

class TextBox : Widget
{
	protected:
		std::string text;

	public:
		virtual void setText(const std::string &newText);
};
