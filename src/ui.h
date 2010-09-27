#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>
#include "ui_tabs.h"

typedef struct {
    GtkWidget *window,
              *roster,
              *status_box,
              *status_entry,
              *toolbox;
    Notebook *tabs;
} Ui;

Ui *ui_init(int *argc, char **argv[]);
void ui_run(void);

#endif
