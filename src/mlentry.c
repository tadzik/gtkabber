#include <gtk/gtk.h>

typedef void (*mlentry_fun)(GtkWidget *, const gchar *, gpointer);

static gboolean
keypress_cb(GtkWidget *v, GdkEventKey *e, gpointer p)
{
	GtkTextBuffer *buf;
	gpointer mldata = g_object_get_data(G_OBJECT(v), "mlentry_data");
	mlentry_fun cb = (mlentry_fun)p;
	buf = GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(v)));
	if(e->keyval == 65293 && !(e->state & GDK_SHIFT_MASK)) {
		GtkTextIter b, e;
		gtk_text_buffer_get_start_iter(buf, &b);
		gtk_text_buffer_get_end_iter(buf, &e);
		cb(v, gtk_text_buffer_get_text(buf, &b, &e, TRUE), mldata);
		return TRUE;
	}
	return FALSE;
}

void
mlentry_clear(GtkWidget *v)
{
	GtkTextBuffer *buf;
	buf = GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(v)));
	gtk_text_buffer_set_text(buf, "", -1);
}

const char *
mlentry_get_text(GtkWidget *v)
{
	GtkTextBuffer *buf;
	GtkTextIter b, e;
	buf = GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(v)));
	gtk_text_buffer_get_start_iter(buf, &b);
	gtk_text_buffer_get_end_iter(buf, &e);
	return gtk_text_buffer_get_text(buf, &b, &e, TRUE);
}

GtkWidget *
mlentry_new(void (*fun)(GtkWidget *, const char *, gpointer), gpointer data)
{
	GtkWidget *view;
	GtkTextBuffer *buffer;

	view = gtk_text_view_new();
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	g_object_set_data(G_OBJECT(view), "mlentry_data", data);
	g_signal_connect(G_OBJECT(view), "key-press-event",
			 G_CALLBACK(keypress_cb), (gpointer)fun);

	return view;
}
