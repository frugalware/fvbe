#pragma once

#include <string>

class Widget
{
	public:
		static const unsigned char ALIGN_LEFT;
		static const unsigned char ALIGN_RIGHT;
		static const unsigned char ALIGN_TOP;
		static const unsigned char ALIGN_BOTTOM;

		static const unsigned char EXPAND_HORIZONTAL;
		static const unsigned char EXPAND_VERTICAL;

		virtual ~Widget();
};

class Label : public Widget
{
	public:
		virtual ~Label();
};

class TextBox : public Widget
{
	public:
		virtual ~TextBox();
};

class EntryBox : public Widget
{
	public:
		virtual ~EntryBox();
};

class CheckBox : public Widget
{
	public:
		virtual ~CheckBox();
};

class Button : public Widget
{
	public:
		virtual ~Button();
};

class ProgressBar : public Widget
{
	public:
		virtual ~ProgressBar();
};

class UI
{
	public:
		virtual ~UI();
		void printError(const std::string &line);
		bool checkTTY(int fd);
		bool setSignalInterrupt(int sig, bool interrupt);
		virtual int main(int argc, char **argv);
};
