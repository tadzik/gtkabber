#include "ui.h"

#include <gtk/gtk.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "xmpp.h"
#include "xmpp_roster.h"
#include "ui_roster.h"
#include "config.h"
#include "mlentry.h"

/* Here be everything ui-related, except the roster part,
 * which is in ui_roster.c
 * Also here the program starts and quits, as ui_setup()
 * is the first function called, and destroy() is to be the last one */

/* functions */
static void append_to_tab(Chattab *, const gchar *);
static void cbox_changed_cb(GtkComboBox *, gpointer);
static void close_tab(Chattab *);
static void destroy(GtkWidget *, gpointer);
static void focus_cb(GtkWidget *, GdkEventFocus *, gpointer);
static void free_all_tabs(void);
static Chattab *get_active_tab(void);
static Chattab *get_tab_content(gint n);
static void infobar_response_cb(GtkInfoBar *, gint, gpointer);
static gboolean keypress_cb(GtkWidget *, GdkEventKey *, gpointer);
static gboolean label_click_cb(GtkWidget *, GdkEventButton *, gpointer);
static void scroll_tab_down(Chattab *);
static void setup_cbox(GtkWidget *);
static void set_wm_urgency(void);
static void status_changed(GtkWidget *, const char *, gpointer);
static void subscr_response_cb(GtkButton *, gpointer);
static void tab_entry_handler(GtkWidget *, const char *, gpointer);
static void tab_notify(Chattab *);
static void tab_switch_cb(GtkNotebook *, GtkNotebookPage *, guint, gpointer);
static void toggle_options(void);
/*************/

/* global variables */
static Chattab *status_tab;
static GtkWidget *toolbox, *nbook, *rview, 
                 *status_cbox, *status_entry, *window;
static GSList *tabs;
/********************/

static void
action_cb(GtkButton *b, gpointer i)
{
	action_call(GPOINTER_TO_INT(i));
	UNUSED(b);
} /* action_cb */

static void
append_to_tab(Chattab *t, const gchar *s)
{
	/* writing string s at the end of tab t's buffer
	 * internal function, for both status tab and chat tabs,
	 * used by ui_print as well as ui_tab_print_message.
	 * Just to not repeat myself */
	GtkTextIter i;
	time_t now;
	gchar tstamp[11];
	gchar *str;
	now = time(NULL);
	gtk_text_buffer_get_end_iter(t->buffer, &i);
	strftime(tstamp, sizeof(tstamp), "[%H:%M:%S]", localtime(&now));
	str = g_strdup_printf("%s %s", tstamp, s);
	gtk_text_buffer_insert(t->buffer, &i, str, -1);
	g_free(str);
	scroll_tab_down(t);
} /* append_to_tab */

static void
cbox_changed_cb(GtkComboBox *e, gpointer p)
{
	xmpp_send_status(NULL, ui_get_status(), NULL);
	UNUSED(e);
	UNUSED(p);
} /* cbox_changed_cb */

static void
close_tab(Chattab *t)
{
	int pageno;
	if(t->jid == NULL)
		return;
	pageno = gtk_notebook_page_num(GTK_NOTEBOOK(nbook), t->vbox);
	g_free(t->jid);
	g_free(t->title);
	gtk_notebook_remove_page(GTK_NOTEBOOK(nbook), pageno);
	tabs = g_slist_remove(tabs, (gconstpointer)t);
} /* close_tab */

static void
destroy(GtkWidget *widget, gpointer data)
{
	xmpp_cleanup();
	ui_roster_cleanup();
	gtk_main_quit();
	free_all_tabs();
	UNUSED(widget);
	UNUSED(data);
} /* destroy */

static void
focus_cb(GtkWidget *w, GdkEventFocus *f, gpointer p)
{
	Chattab *tab = get_active_tab();
	gtk_widget_grab_focus(tab->jid ? tab->entry : nbook);
	if(gtk_window_get_urgency_hint(GTK_WINDOW(window))) {
		gtk_window_set_urgency_hint(GTK_WINDOW(window), FALSE);
	}
	UNUSED(w);
	UNUSED(f);
	UNUSED(p);
} /* focus_cb */

static void
free_all_tabs(void)
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
} /* free_all_tabs*/

static Chattab *
get_active_tab(void)
{
	return get_tab_content(gtk_notebook_get_current_page(GTK_NOTEBOOK(nbook)));
}

static Chattab *
get_tab_content(gint n)
{
	GtkWidget *child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(nbook), n);
	return (Chattab *)g_object_get_data(G_OBJECT(child), "chattab-data");
} /* get_active_tab */

static void
infobar_response_cb(GtkInfoBar *i, gint r, gpointer j)
{
	gchar *jid = (gchar *)j;
	xmpp_subscr_response(jid, (r == GTK_RESPONSE_YES)
	                                 ? TRUE : FALSE);
	gtk_container_remove(GTK_CONTAINER(toolbox), GTK_WIDGET(i));
	g_free(jid);
}

static gboolean
keypress_cb(GtkWidget *w, GdkEventKey *e, gpointer u)
{
	/* We react only on Ctrl+something keys */
	if(e->state & GDK_CONTROL_MASK) {
		switch(e->keyval) {
		case 104: /* h */
			ui_roster_toggle_offline();
			break;
		case 111: /* o */
			toggle_options();
			break;
		case 113: /* q */
			close_tab(get_active_tab());
			break;
		case 114: /* r */
			if(e->state & GDK_MOD1_MASK) { /* with shift */
				ui_print("Config file reloaded\n");
				config_reload();
			} else {
				gtk_widget_grab_focus(rview);
			}
			break;
		case 115: /* s */
			if(e->state & GDK_MOD1_MASK) /* with alt */
				gtk_widget_grab_focus(status_entry);
			else
				gtk_widget_grab_focus(status_cbox);
			break;
		case 116: /* t */
			{
				Chattab *tab = get_active_tab();
				gtk_widget_grab_focus(tab->jid ? tab->entry : nbook);
			}
			break;
		}
	}

	return 0;
	UNUSED(w);
	UNUSED(u);
} /* keypress_cb */

static gboolean
label_click_cb(GtkWidget *w, GdkEventButton *e, gpointer p)
{
	Chattab *tab;
	if(e->button != 3)
		return FALSE;
	tab = (Chattab *)p;
	close_tab(tab);
	return TRUE;
	UNUSED(w);
} /* label_click_cb */

static void
scroll_tab_down(Chattab *tab)
{
	GtkAdjustment *adj;
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tab->tview), tab->mk,
	                             0.0, FALSE, 0.0, 0.0);
	adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(tab->scrolled));
	gtk_adjustment_set_value(adj, adj->upper);
} /* scroll_tab_down */

static void
setup_cbox(GtkWidget *cbox)
{
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Free for chat");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Online");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Away");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Not available");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Do not disturb");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Offline");
	gtk_combo_box_set_active(GTK_COMBO_BOX(cbox), 1);
	g_signal_connect(G_OBJECT(cbox), "changed",
	                 G_CALLBACK(cbox_changed_cb), NULL);
} /* setup_cbox */

static void
set_wm_urgency(void)
{
	if(gtk_window_is_active(GTK_WINDOW(window)))
		return;
	if(gtk_window_get_urgency_hint(GTK_WINDOW(window)))
		gtk_window_set_urgency_hint(GTK_WINDOW(window), FALSE);
	gtk_window_set_urgency_hint(GTK_WINDOW(window), TRUE);
} /* set_wm_urgency */

static void
status_changed(GtkWidget *w, const char *e, gpointer p)
{
	/* automagically focusing tab entry */
	Chattab *tab = get_active_tab();
	xmpp_send_status(NULL, ui_get_status(), NULL);
	gtk_widget_grab_focus(tab->jid ? tab->entry : nbook);
	UNUSED(w);
	UNUSED(e);
	UNUSED(p);
}

static void
subscr_response_cb(GtkButton *b, gpointer t)
{
	const gchar *type;
	GtkWidget *table, *jentry, *nentry, *gentry;
	type = g_object_get_data(G_OBJECT(b), "type");
	table = GTK_WIDGET(t);
	jentry = g_object_get_data(G_OBJECT(b), "jentry");
	nentry = g_object_get_data(G_OBJECT(b), "nentry");
	gentry = g_object_get_data(G_OBJECT(b), "gentry");
	if(g_strcmp0(type, GTK_STOCK_OK) == 0) {
		xmpp_subscribe(gtk_entry_get_text(GTK_ENTRY(jentry)),
		               gtk_entry_get_text(GTK_ENTRY(nentry)),
		               gtk_entry_get_text(GTK_ENTRY(gentry)));
	}
	gtk_container_remove(GTK_CONTAINER(toolbox), table);
} /* subscr_response_cb */

static void
tab_entry_handler(GtkWidget *mlentry, const char *t, gpointer p)
{
	/* Sending message to the interlocutor
	 * and printing in in tab's buffer */
	Chattab *tab = (Chattab *)p;
	gchar *str;
	if (*t == 0) return;
	xmpp_send_message(tab->jid, t);
	str = g_strdup_printf("--> %s\n", t);
	append_to_tab(tab, str);
	g_free(str);
	mlentry_clear(mlentry);
} /* tab_entry_handler */

static void
tab_notify(Chattab *t)
{
	gchar *markup;
	Chattab *active = get_active_tab();
	set_wm_urgency();
	if(active->vbox == t->vbox)
		/* this tab's alredy active */
		return;
	markup = g_strdup_printf("<b>%s</b>", t->title);
	gtk_label_set_markup(GTK_LABEL(t->label), markup);
	g_free(markup);
} /* tab_notify */

static void
tab_switch_cb(GtkNotebook *b, GtkNotebookPage *p, guint n, gpointer d)
{	
	Chattab *tab = get_tab_content(n);
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

static void
toggle_options(void)
{
	static int shown = 0;
	static GtkWidget *hbox;
	if(!shown) {
		int i;
		GtkWidget *sbutton;
		hbox = gtk_hbutton_box_new();
		gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_SPREAD);
		/* Subscribe */
		sbutton = gtk_button_new_with_mnemonic("_Subscribe");
		g_signal_connect(G_OBJECT(sbutton), "clicked",
		                 G_CALLBACK(ui_show_subscribe_query), NULL);
		gtk_container_add(GTK_CONTAINER(hbox), sbutton);
		/* User-defined actions */
		for(i=0; ; i++) {
			GtkWidget *button;
			const gchar *text = action_get(i);
			if(text == NULL) break;
			button = gtk_button_new_with_mnemonic(text);
			g_signal_connect(G_OBJECT(button), "clicked",
			                 G_CALLBACK(action_cb), GINT_TO_POINTER(i));
			gtk_container_add(GTK_CONTAINER(hbox), button);
		}

		gtk_box_pack_end(GTK_BOX(toolbox), hbox, FALSE, FALSE, 0);
		gtk_widget_show_all(hbox);
	} else {
		gtk_container_remove(GTK_CONTAINER(toolbox), hbox);
	}
	shown = !shown;
} /* toggle_options */

Chattab *
ui_create_tab(const gchar *jid, const gchar *title, gint active)
{
	/* Okay, here's a big one. It's called either when user clicks on the buddy
	 * in the roster (ui_roster.c), or when the new message arrives (xmpp.c,
	 * xmpp_message_handler), if supplied jid is NULL, we're creating
	 * a status_tab */
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
	                 G_CALLBACK(label_click_cb), (gpointer)tab);
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
	if (jid)
		tab->entry = mlentry_new(tab_entry_handler, tab);
	/* some vbox to put it together */
	tab->vbox = gtk_vbox_new(FALSE, 0);
	/* this will help us finding Chattab struct by some of its properties */
	g_object_set_data(G_OBJECT(tab->vbox), "chattab-data", tab);
	/* now let's put it all together */
	gtk_container_add(GTK_CONTAINER(tab->vbox), tab->scrolled);
	if(jid) {
		gtk_container_add(GTK_CONTAINER(tab->vbox), tab->entry);
		gtk_box_set_child_packing(GTK_BOX(tab->vbox), tab->entry, FALSE,
	                                  FALSE, 0, GTK_PACK_START);
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
} /* ui_create_tab */

XmppStatus
ui_get_status(void)
{
	switch(gtk_combo_box_get_active(GTK_COMBO_BOX(status_cbox))) {
		case 0:
			return STATUS_FFC;
		case 2:
			return STATUS_AWAY;
		case 3:
			return STATUS_XA;
		case 4:
			return STATUS_DND;
		case 5:
			return STATUS_OFFLINE;
		default:
			return STATUS_ONLINE;
	}
} /* ui_get_status */

const char *
ui_get_status_msg(void)
{
	return mlentry_get_text(status_entry);
} /* ui_get_status_msg */

void
ui_setup(int *argc, char **argv[])
{
	/* The first thing that occurs in the program, sets up the interface */
	GtkWidget *hpaned, *leftbox, *rwin, *vbox;
	gtk_init(argc, argv);
	/* creating widgets */
	hpaned = gtk_hpaned_new();
	toolbox = gtk_vbox_new(FALSE, 0);
	leftbox = gtk_vbox_new(FALSE, 0);
	nbook = gtk_notebook_new();
	rwin = gtk_scrolled_window_new(NULL, NULL);
	status_cbox = gtk_combo_box_new_text();
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	vbox = gtk_vbox_new(FALSE, 0);
	/* setting up status tab
	 * needs to be almost first to make room for error messages */
	ui_create_tab(NULL, "Status", 0);
	/* setting up the more exciting ones */
	rview = ui_roster_setup();
	setup_cbox(status_cbox);
	status_entry = mlentry_new(status_changed, NULL);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(nbook), TRUE);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(rwin),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);
	gtk_window_set_title(GTK_WINDOW(window), "gtkabber");
	gtk_widget_set_size_request(window, 640, 480);
	/* packing */
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(rwin), rview);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_container_add(GTK_CONTAINER(vbox), hpaned);
	gtk_container_add(GTK_CONTAINER(vbox), toolbox);
	gtk_paned_add1(GTK_PANED(hpaned), leftbox);
	gtk_container_add(GTK_CONTAINER(leftbox), rwin);
	gtk_container_add(GTK_CONTAINER(leftbox), status_cbox);
	gtk_container_add(GTK_CONTAINER(leftbox), status_entry);
	gtk_paned_add2(GTK_PANED(hpaned), nbook);
	gtk_box_set_child_packing(GTK_BOX(leftbox), status_cbox,
	                          FALSE, FALSE, 0, GTK_PACK_START);
	gtk_box_set_child_packing(GTK_BOX(leftbox), status_entry,
	                          FALSE, FALSE, 0, GTK_PACK_START);
	gtk_box_set_child_packing(GTK_BOX(vbox), toolbox,
	                          FALSE, FALSE, 0, GTK_PACK_START);
	/* setting up signals */
	g_signal_connect(G_OBJECT(window), "destroy",
	                 G_CALLBACK(destroy), NULL);
	g_signal_connect(G_OBJECT(window), "key-press-event",
	                 G_CALLBACK(keypress_cb), NULL);
	g_signal_connect(G_OBJECT(window), "focus-in-event",
	                 G_CALLBACK(focus_cb), NULL);
	g_signal_connect(G_OBJECT(nbook), "switch-page",
	                 G_CALLBACK(tab_switch_cb), NULL);
	/*go go go!*/
	gtk_widget_show_all(window);
} /* ui_setup */

void
ui_set_status(XmppStatus s)
{
	gtk_combo_box_set_active(GTK_COMBO_BOX(status_cbox), s);
} /* ui_set_status */

void
ui_set_status_msg(const char *m)
{
	gtk_entry_set_text(GTK_ENTRY(status_entry), m);
	xmpp_send_status(NULL, ui_get_status(), NULL);
} /* ui_set_status_msg */

void
ui_show_presence_query(const gchar *j)
{
	GtkWidget *conarea, *ibar, *label;
	gchar *msg, *jid;
	ibar = gtk_info_bar_new_with_buttons(GTK_STOCK_YES, GTK_RESPONSE_YES,
	                                     GTK_STOCK_NO, GTK_RESPONSE_NO,
	                                     NULL);
	msg = g_strdup_printf("Do you wish to accept presence\n"
	                      "subscription request from %s?", j);
	label = gtk_label_new(msg);
	jid = g_strdup(j);
	gtk_info_bar_set_message_type(GTK_INFO_BAR(ibar), GTK_MESSAGE_QUESTION);
	conarea = gtk_info_bar_get_content_area(GTK_INFO_BAR(ibar));
	gtk_container_add(GTK_CONTAINER(conarea), label);
	gtk_container_add(GTK_CONTAINER(toolbox), ibar);
	g_signal_connect(G_OBJECT(ibar), "response",
	                 G_CALLBACK(infobar_response_cb), (gpointer)jid);
	gtk_widget_show_all(ibar);
	set_wm_urgency();
	g_free(msg);
} /* ui_show_presence_query */

void
ui_show_subscribe_query(void)
{
	GtkWidget *jlabel, *nlabel, *glabel,
	          *jentry, *nentry, *gentry,
	          *obutton, *cbutton,
	          *table;
	jlabel = gtk_label_new("Jid:");
	nlabel = gtk_label_new("Name:");
	glabel = gtk_label_new("Group:");

	jentry = gtk_entry_new();
	nentry = gtk_entry_new();
	gentry = gtk_entry_new();

	obutton = gtk_button_new_from_stock(GTK_STOCK_OK);
	cbutton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

	table = gtk_table_new(2, 4, FALSE);
	/* first row */
	gtk_table_attach_defaults(GTK_TABLE(table), jlabel, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), nlabel, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), glabel, 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), cbutton, 3, 4, 0, 1);
	/* second row */
	gtk_table_attach_defaults(GTK_TABLE(table), jentry, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), nentry, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), gentry, 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), obutton, 3, 4, 1, 2);

	g_signal_connect(G_OBJECT(obutton), "clicked",
	                 G_CALLBACK(subscr_response_cb), (gpointer)table);
	g_signal_connect(G_OBJECT(cbutton), "clicked",
	                 G_CALLBACK(subscr_response_cb), (gpointer)table);

	g_object_set_data(G_OBJECT(cbutton), "type", GTK_STOCK_CANCEL);
	g_object_set_data(G_OBJECT(obutton), "type", GTK_STOCK_OK);
	g_object_set_data(G_OBJECT(obutton), "jentry", jentry);
	g_object_set_data(G_OBJECT(obutton), "nentry", nentry);
	g_object_set_data(G_OBJECT(obutton), "gentry", gentry);
	gtk_container_add(GTK_CONTAINER(toolbox), table);
	gtk_widget_show_all(table);
	gtk_widget_grab_focus(jentry);
} /* ui_show_subscribe_query */

void
ui_print(const char *msg, ...)
{
	/* Our printf() substitute, almost everything
	 * that happens is reported here */
	va_list ap;
	char *str;
	va_start(ap, msg);
	str = g_strdup_vprintf(msg, ap);
	append_to_tab(status_tab, str);
	/* looks stupid? That's because old gcc is stupid */
	g_printerr("%s", str);
	va_end(ap);
	g_free(str);
} /* ui_print */

void
ui_tab_print_message(const char *jid, const char *msg)
{
	/* Called by xmpp_mesg_handler(), prints the incoming message
	 * to the approprate chat tab */
	Chattab *tab = NULL;
	GSList *elem;
	char *str;
	for(elem = tabs; elem; elem = elem->next) {
		Chattab *t = (Chattab *)elem->data;
		if(g_strcmp0(t->jid, jid) == 0) {
			tab = t;
			break;
		}
	}
	if(!tab) {
		Buddy *sb;
		char *shortjid, *slash;
		/* We need to obtain the jid itself, w/o resource,
		 * to lookup the appropriate chat tab */
		slash = strchr(jid, '/');
		shortjid = (slash) ? g_strndup(jid, slash-jid) : g_strdup(jid);
		sb = xmpp_roster_find_by_jid(shortjid);
		if(!sb) {
			tab = ui_create_tab(jid, jid, 0);
		} else {
			tab = ui_create_tab(jid, sb->name, 0);
		}
		g_free(shortjid);
	}
	/* actual message printing - two lines of this whole function! */
	str = g_strdup_printf("<== %s\n", msg);
	append_to_tab(tab, str);
	/* bolding tab title if it's not the status tab
	 * (the function will check whether the tab is active or not,
	 * we don't care about this) */
	if(tab->jid)
		tab_notify(tab);
	g_free(str);
} /* ui_tab_print_message */
