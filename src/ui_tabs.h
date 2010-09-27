#ifndef UI_TABS_H
#define UI_TABS_H

#include <gtk/gtk.h>

typedef struct {
    gchar *jid,
          *title;
    GtkTextBuffer *buffer;
    GtkTextMark *mk;
    GtkWidget *entry, /* TODO: mlentry object */
              *label,
              *scrolled,
              *tview,
              *vbox;
} Chattab;

typedef struct {
    GtkWidget *nbook;
    GSList *tabs;
    Chattab *status_tab;
} Notebook;

Chattab *ui_tabs_create(Notebook *, const gchar *, const gchar *, gint);
Notebook *ui_tabs_new(void);

#endif
