#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>
#include "ui_tabs.h"
#include "mlentry.h"

typedef struct {
    GtkWidget *window,
              *roster,
              *status_box,
              *toolbox;
    Mlentry   *status_entry;
    Notebook  *tabs;
} Ui;

Ui *ui_init(int *argc, char **argv[]);
void ui_print(const gchar *, ...);
void ui_run(void);

#endif
