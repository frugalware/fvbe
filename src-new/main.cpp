#include <iostream>
#include <dlfcn.h>
#include "main.hpp"

static void *ui_handle = NULL;
static ui_new_t ui_new = NULL;
static ui_delete_t ui_delete = NULL;
UI *ui = NULL;

static bool ui_load(const std::string &filename)
{
	if(
		(ui_handle = dlopen(filename.c_str(), RTLD_NOW)) == NULL          ||
		(ui_new = (ui_new_t) dlsym(ui_handle, "ui_new")) == NULL          ||
		(ui_delete = (ui_delete_t) dlsym(ui_handle, "ui_delete")) == NULL
	)
	{
		std::cerr << "Failed to open UI: " << dlerror() << std::endl;
		return false;
	}

	return true;
}

int main(int argc, char **argv)
{
	if(!ui_load("./ui_newt.so"))
	{
		return 1;
	}

	ui = ui_new();

	ui->main(argc, argv);

	ui_delete(ui);

	dlclose(ui_handle);

	return 0;
}
