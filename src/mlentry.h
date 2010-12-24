#ifndef MLENTRY_H
#define MLENTRY_H
#include <gtk/gtk.h>

struct _Mlentry {
    GtkWidget *widget;
    gpointer udata;
    void (*callback)(struct _Mlentry *, const char *, gpointer);
};
typedef struct _Mlentry Mlentry;

void mlentry_clear(Mlentry *);
const char * mlentry_get_text(Mlentry *);
Mlentry * mlentry_new(void (*fun)(Mlentry *, const gchar *, gpointer),
                        gpointer data);

#endif
