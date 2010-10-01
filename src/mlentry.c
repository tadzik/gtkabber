#include "mlentry.h"
#include "common.h"

static gboolean
keypress_cb(GtkWidget *v, GdkEventKey *e, gpointer p)
{
    if(e->keyval == 65293 && !(e->state & GDK_SHIFT_MASK)) {
        Mlentry *obj = (Mlentry *)p;
        obj->callback(obj, mlentry_get_text(obj));
        return TRUE;
    }
    return FALSE;
    UNUSED(v);
}

void
mlentry_clear(Mlentry *o)
{
    GtkTextBuffer *buf;
    buf = GTK_TEXT_BUFFER(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(o->widget))
          );
    gtk_text_buffer_set_text(buf, "", -1);
}

const char *
mlentry_get_text(Mlentry *o)
{
    GtkTextBuffer *buf;
    GtkTextIter b, e;
    buf = GTK_TEXT_BUFFER(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(o->widget))
          );
    gtk_text_buffer_get_start_iter(buf, &b);
    gtk_text_buffer_get_end_iter(buf, &e);
    return gtk_text_buffer_get_text(buf, &b, &e, TRUE);
}

Mlentry *
mlentry_new(void (*fun)(Mlentry *, const gchar *, gpointer),
            gpointer data)
{
    Mlentry *new = g_malloc(sizeof(Mlentry));
    new->widget = gtk_text_view_new();
    new->callback = fun;
    new->udata = data;
    g_signal_connect(G_OBJECT(new->widget), "key-press-event",
                     G_CALLBACK(keypress_cb), (gpointer)new);
    return new;
}
