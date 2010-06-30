#include <gtk/gtk.h>

void mlentry_clear(GtkWidget *);
const char * mlentry_get_text(GtkWidget *);
GtkWidget * mlentry_new(void (*fun)(GtkWidget *, const char *, gpointer),
			gpointer data);
