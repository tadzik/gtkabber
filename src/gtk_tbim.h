#ifndef GTK_TBIM_H
#define GTK_TBIM_H
#include <gtk/gtk.h>

void gtk_text_buffer_insert_markup(GtkTextBuffer *, GtkTextIter *,
                                   const gchar *, gint len);
#endif
