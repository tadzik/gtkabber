#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>

typedef struct {
    GtkWidget *window,
              *roster,
              *status_box,
              *status_entry,
              *tabs,
              *toolbox;
} Ui;

Ui *ui_init(int *argc, char **argv[]);
void ui_run(void);

#endif
