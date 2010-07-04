#ifndef UI_TABS_H
#define UI_TABS_H
#include "types.h"
#include <gtk/gtk.h>

Chattab *ui_tab_create(const gchar *, const gchar *, gint);
void ui_tab_append_text(Chattab *, const gchar *);
void ui_tab_append_markup(Chattab *, const gchar *);
void ui_tab_cleanup(void);
void ui_tab_close(Chattab *);
void ui_tab_focus(void);
GtkWidget *ui_tab_init(void);
void ui_tab_next(void);
void ui_tab_prev(void);
void ui_tab_print_message(const gchar *, const gchar *);
void ui_tab_set_page(gint n);

#endif
