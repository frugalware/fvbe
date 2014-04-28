#pragma once

#include "ui.hpp"

typedef UI *(*ui_new_t) ();

typedef void (*ui_delete_t) (UI *);

extern UI *ui;
