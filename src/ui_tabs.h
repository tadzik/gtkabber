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

void ui_tabs_append_text(Chattab *, const gchar *);
void ui_tabs_append_markup(Chattab *, const gchar *);
void ui_tabs_cleanup(Notebook *);
Chattab *ui_tabs_create(Notebook *, const gchar *, const gchar *, gint);
void ui_tabs_log(Notebook *, const gchar *);
Notebook *ui_tabs_new(void);

#endif
