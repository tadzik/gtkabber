#include "config.h"
#include "ui.h"
#include "ui_tabs.h"
#include "xmpp.h"
#include "xmpp_roster.h"
#include "mlentry.h"
#include "gtk_tbim.h"
#include <stdarg.h>
#include <string.h>

static GtkWidget *nbook;
static Chattab *status_tab;
static GSList *tabs;

static void tab_entry_handler(GtkWidget *mlentry, const char *t, gpointer p);
static Chattab *tab_get_content(gint n);
static gboolean tab_label_click_cb(GtkWidget *w, GdkEventButton *e, gpointer p);
static void tab_notify(Chattab *t);
static void tab_scroll_down(Chattab *);
static void tab_switch_cb(GtkNotebook *, GtkNotebookPage *, guint, gpointer);
#define ACTIVE_TAB tab_get_content(gtk_notebook_get_current_page(GTK_NOTEBOOK(nbook)));

static void
tab_entry_handler(GtkWidget *mlentry, const char *t, gpointer p)
{
	/* Sending message to the interlocutor
	 * and printing in in tab's buffer */
	Chattab *tab = (Chattab *)p;
	gchar *str, *et;
	if (*t == 0) return;
	et = g_markup_escape_text(t, -1);
	xmpp_send_message(tab->jid, t);
	str = lua_msg_markup(get_settings_str(USERNAME), et);
	if (str == NULL)
		str = g_strdup_printf("%s: %s\n", get_settings_str(USERNAME), t);
	ui_tab_append_markup(tab, str);
	g_free(str);
	g_free(et);
	mlentry_clear(mlentry);
} /* tab_entry_handler */

static Chattab *
tab_get_content(gint n)
{
	GtkWidget *child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(nbook), n);
	return (Chattab *)g_object_get_data(G_OBJECT(child), "chattab-data");
} /* tab_get_content */

static gboolean
tab_label_click_cb(GtkWidget *w, GdkEventButton *e, gpointer p)
{
	Chattab *tab;
	if(e->button != 3)
		return FALSE;
	tab = (Chattab *)p;
	ui_tab_close(tab);
	return TRUE;
	UNUSED(w);
} /* tab_label_click_cb */

static void
tab_notify(Chattab *t)
{
	gchar *markup;
	Chattab *active = ACTIVE_TAB;
	ui_set_wm_urgency();
	if(active->vbox == t->vbox)
		/* this tab's alredy active */
		return;
	markup = g_strdup_printf("<b>%s</b>", t->title);
	gtk_label_set_markup(GTK_LABEL(t->label), markup);
	g_free(markup);
} /* tab_notify */

static void
tab_scroll_down(Chattab *tab)
{
	GtkAdjustment *adj;
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tab->tview), tab->mk,
	                             0.0, FALSE, 0.0, 0.0);
	adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(tab->scrolled));
	gtk_adjustment_set_value(adj, adj->upper);
} /* tab_scroll_down */

static void
tab_switch_cb(GtkNotebook *b, GtkNotebookPage *p, guint n, gpointer d)
{	
	Chattab *tab = tab_get_content(n);
	if(tab->jid) {
		gtk_label_set_text(GTK_LABEL(tab->label), tab->title);
		gtk_widget_grab_focus(tab->entry);
	} else {
		gtk_widget_grab_focus(nbook);
	}
	UNUSED(b);
	UNUSED(p);
	UNUSED(d);
} /* tab_switch_cb */

/* end of static functions */

static void
ui_tab_append_anyhow(Chattab *t, const gchar *s, void (*fun)(GtkTextBuffer *, GtkTextIter *, const gchar *, gint))
{
	/* timestamp is always added */
	GtkTextIter i;
	time_t now;
	gchar tstamp[11];
	gchar *str;
	if (t == NULL) t = status_tab;
	now = time(NULL);
	gtk_text_buffer_get_end_iter(t->buffer, &i);
	strftime(tstamp, sizeof(tstamp), "[%H:%M:%S]", localtime(&now));
	str = g_strdup_printf("%s %s", tstamp, s);
	(*fun)(t->buffer, &i, str, -1);
	g_free(str);
	tab_scroll_down(t);
}

void
ui_tab_append_text(Chattab *t, const gchar *s)
{
	ui_tab_append_anyhow(t, s, gtk_text_buffer_insert);
} /* ui_tab_append_text */

void
ui_tab_append_markup(Chattab *t, const gchar *s)
{
	ui_tab_append_anyhow(t, s, gtk_tbim);
} /* ui_tab_append_markup */

void
ui_tab_cleanup(void)
{
	if(tabs) {
		GSList *elem;
		for(elem = tabs; elem; elem = elem->next) {
			Chattab *t = (Chattab *)elem->data;
			g_free(t->jid);
			g_free(t->title);
			g_free(t);
		}
		g_slist_free(tabs);
	}
} /* ui_tab_cleanup */

void
ui_tab_close(Chattab *t)
{
	int pageno;
	if (t == NULL) t = ACTIVE_TAB;
	if (t->jid == NULL)
		return;
	pageno = gtk_notebook_page_num(GTK_NOTEBOOK(nbook), t->vbox);
	g_free(t->jid);
	g_free(t->title);
	gtk_notebook_remove_page(GTK_NOTEBOOK(nbook), pageno);
	tabs = g_slist_remove(tabs, (gconstpointer)t);
} /* ui_tab_close */

Chattab *
ui_tab_create(const gchar *jid, const gchar *title, gint active)
{
	Chattab *tab;
	GtkWidget *evbox;
	tab = g_malloc(sizeof(Chattab));
	if(jid == NULL) {
		tab->jid = NULL;
		status_tab = tab;
	}
	else
		tab->jid = g_strdup(jid);
	tab->title = g_strdup(title);
	/* setting up a place for a tab title */
	evbox = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(evbox), FALSE);
	tab->label = gtk_label_new(tab->title);
	gtk_container_add(GTK_CONTAINER(evbox), tab->label);
	gtk_widget_show(tab->label);
	g_signal_connect(G_OBJECT(evbox), "button-press-event",
	                 G_CALLBACK(tab_label_click_cb), (gpointer)tab);
	/* creating GtkTextView for status messages */
	tab->tview = gtk_text_view_new();
	tab->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->tview));
	gtk_text_view_set_editable(GTK_TEXT_VIEW(tab->tview), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tab->tview), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tab->tview), GTK_WRAP_WORD);
	tab->mk = gtk_text_buffer_get_mark(tab->buffer, "insert");
	/* we're putting this in a scrolled window */
	tab->scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tab->scrolled),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(tab->scrolled),
	                                      tab->tview);
	/* setting up the entry field */
	if (jid) {
		tab->entry = mlentry_new(tab_entry_handler, tab);
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tab->entry), GTK_WRAP_WORD_CHAR);
		gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(tab->entry), FALSE);
	}
	/* some vbox to put it together */
	tab->vbox = gtk_vpaned_new();
	/* this will help us finding Chattab struct by some of its properties */
	g_object_set_data(G_OBJECT(tab->vbox), "chattab-data", tab);
	/* now let's put it all together */
	gtk_paned_pack1(GTK_PANED(tab->vbox), tab->scrolled, TRUE, FALSE);
	if(jid) {
		gtk_paned_pack2(GTK_PANED(tab->vbox), tab->entry, FALSE, FALSE);
	}
	gtk_widget_show_all(tab->vbox);
	/* aaand, launch! */
	gtk_notebook_append_page(GTK_NOTEBOOK(nbook), tab->vbox, evbox);
	tabs = g_slist_prepend(tabs, tab);
	if(active && jid) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(nbook),
	       	                              gtk_notebook_page_num(GTK_NOTEBOOK(nbook),
	                                                            tab->vbox));
		gtk_widget_grab_focus(tab->entry);
	}
	return tab;
} /* ui_tab_create */

void
ui_tab_focus(void)
{
	Chattab *tab = ACTIVE_TAB;
	gtk_widget_grab_focus(tab->jid ? tab->entry : nbook);
} /* ui_tab_focus */

GtkWidget *
ui_tab_init(void)
{
	nbook = gtk_notebook_new();
	g_signal_connect(G_OBJECT(nbook), "switch-page",
	                 G_CALLBACK(tab_switch_cb), NULL);
	ui_tab_create(NULL, "Status", 0);
	return nbook;
} /* ui_tab_init */

void
ui_tab_next(void)
{
	gtk_notebook_next_page(GTK_NOTEBOOK(nbook));
}

void
ui_tab_prev(void)
{
	gtk_notebook_prev_page(GTK_NOTEBOOK(nbook));
}

void
ui_tab_print_message(const char *jid, const char *msg)
{
	/* Called by xmpp_mesg_handler(), prints the incoming message
	 * to the approprate chat tab */
	Chattab *tab = NULL;
	GSList *elem;
	char *str, *shortjid, *slash, *emsg;
	Buddy *sb = NULL;
	/* We need to obtain the jid itself, w/o resource */
	slash = strchr(jid, '/');
	shortjid = (slash) ? g_strndup(jid, slash-jid) : g_strdup(jid);
	sb = xmpp_roster_find_by_jid(shortjid);
	
	for(elem = tabs; elem; elem = elem->next) {
		Chattab *t = (Chattab *)elem->data;
		if(g_strcmp0(t->jid, jid) == 0) {
			tab = t;
			break;
		}
	}

	if(!tab) {
		if (sb == NULL) {
			tab = ui_tab_create(jid, jid, 0);
		} else {
			tab = ui_tab_create(jid, sb->name, 0);
		}
		g_free(shortjid);
	}
	/* actual message printing - two lines of this whole function! */
	emsg = g_markup_escape_text(msg, -1);
	str = lua_msg_markup((sb) ? sb->name : jid, emsg);
	if (str == NULL)
		str = g_strdup_printf("<b>%s</b>: %s\n", (sb) ? sb->name : jid, msg);
	ui_tab_append_markup(tab, str);
	/* bolding tab title if it's not the status tab
	 * (the function will check whether the tab is active or not,
	 * we don't care about this) */
	if (tab->jid)
		tab_notify(tab);
	g_free(str);
	g_free(emsg);
} /* ui_tab_print_message */

void ui_tab_set_page(gint n)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(nbook), n);
}
