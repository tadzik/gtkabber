#include <gtk/gtk.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "xmpp.h"
#include "xmpp_roster.h"
#include "types.h"
#include "ui_roster.h"
#include "commands.h"

/* Here be everything ui-related, except the roster part,
 * which is in ui_roster.c
 * Also here the program starts and quits, as ui_setup()
 * is the first function called, and destroy() is to be the last one */

/* functions */
static void append_to_tab(Chattab *, const gchar *);
static void cbox_changed_cb(GtkComboBox *, gpointer);
static void tab_notify(Chattab *);
static void destroy(GtkWidget *, gpointer);
static Chattab *find_tab_by_jid(const gchar *);
static void free_all_tabs(void);
static void free_tab(gpointer, gpointer);
static gint match_tab_by_jid(gconstpointer, gconstpointer);
static gboolean keypress_cb(GtkWidget *, GdkEventKey *, gpointer);
static void reset_tab_title(GtkNotebook *, GtkNotebookPage *, guint, gpointer);
static void scroll_tab_down(Chattab *);
static void setup_cbox(GtkWidget *);
static void set_wm_urgency(void);
static void tab_entry_handler(GtkWidget *, gpointer);
void ui_create_tab(Chattab *);
XmppStatus ui_get_status(void);
const gchar *ui_get_status_msg(void);
void ui_setup(int *, char ***);
void ui_set_status(XmppStatus);
void ui_set_status_msg(const gchar *);
void ui_status_print(const gchar *msg, ...);
void ui_tab_print_message(const gchar *, const gchar *);
/*************/

/* global variables */
static Chattab *status_tab;
static GtkWidget *nbook, *rview, *status_cbox, *status_entry, *window;
static GSList *tabs;
/********************/

static void
append_to_tab(Chattab *t, const gchar *s)
{
	/* writing string s at the end of tab t's buffer
	 * internal function, for both status tab and chat tabs,
	 * used by ui_status_print as well as ui_tab_print_message.
	 * Just to not repeat myself */
	GtkTextIter i;
	time_t now;
	gchar tstamp[11];
	gchar *str;
	now = time(NULL);
	gtk_text_buffer_get_end_iter(t->buffer, &i);
	strftime(tstamp, sizeof(tstamp), "[%H:%M:%S]", localtime(&now));
	str = g_strdup_printf("%s %s", tstamp, s);
	gtk_text_buffer_insert(t->buffer, &i, str, strlen(str));
	g_free(str);
	scroll_tab_down(t);
} /* append_to_tab */

static void
cbox_changed_cb(GtkComboBox *e, gpointer p)
{
	/* internal handler calling external function. Needed? TODO: Check */
	xmpp_set_status(ui_get_status());
} /* cbox_changed_cb */

static void
tab_notify(Chattab *t)
{
	/* This makes all necessary notifications on the selected tab,
	 * setting wm urgency (TODO: should be defined by configuration)
	 * as well as bolding the tab title */
	gchar *markup;
	GtkWidget *activechild;
	set_wm_urgency(); /* this is done even if the tab is active */
	activechild = (gtk_notebook_get_nth_page(GTK_NOTEBOOK(nbook),
	               gtk_notebook_get_current_page(GTK_NOTEBOOK(nbook))));
	if(activechild == t->vbox)
		/* this tab's alredy active */
		return;
	markup = g_strdup_printf("<b>%s</b>", t->title);
	gtk_label_set_markup(GTK_LABEL(t->label), markup);
	g_free(markup);
} /* tab_notify */

static void
destroy(GtkWidget *widget, gpointer data)
{
	/* This occurs when "destroy" event comes to our window
	 * cleaning everything up and stuff. The whole app ends here */
	xmpp_cleanup();
	ui_roster_cleanup();
	gtk_main_quit();
	free_all_tabs();
} /* destroy */

Chattab *
find_tab_by_jid(const gchar *jid)
{
	/* Self-explanatory I guess. Internal one, used by ui_tab_print_message */
	GSList *elem;
	elem = g_slist_find_custom(tabs, jid, match_tab_by_jid);
	if(elem)
		return elem->data;
	else
		return NULL;
} /* find_tab_by_jid */

static void
free_all_tabs(void)
{
	/* No explanations needed */
	if(tabs) {
		g_slist_foreach(tabs, (GFunc)free_tab, NULL);
		g_slist_free(tabs);
	}
} /* free_all_tabs*/

static void
free_tab(gpointer t, gpointer u)
{
	/* Again, nothing to explain */
	Chattab *tab = (Chattab *)t;
	g_free(tab->jid);
	g_free(tab->title);
} /* free_tab */

static gint
match_tab_by_jid(gconstpointer elem, gconstpointer jid)
{
	/* Internal, used by find_tab_by_jid */	
	Chattab *tab = (Chattab *)elem;
	/* This alredy covers the status tab case,
	 * as g_strcmp0 handles NULL string well */
	return g_strcmp0(tab->jid, jid);
} /* match_tab_by_jid */

static gboolean
keypress_cb(GtkWidget *w, GdkEventKey *e, gpointer u)
{
	/* We react only on Ctrl+something keys */
	if(e->state & (guint)4) { /* ctrl mask */
		switch(e->keyval) {
		case 114: /* r */
			gtk_widget_grab_focus(rview);
			break;
		case 115: /* s */
			if(e->state & (guint)8) /* with alt */
				gtk_widget_grab_focus(status_entry);
			else
				gtk_widget_grab_focus(status_cbox);
			break;
		case 116: /* t */
			gtk_widget_grab_focus(nbook);
			break;
		}
	}

	return 0;
} /* keypress_cb */

static void
reset_tab_title(GtkNotebook *b, GtkNotebookPage *p, guint n, gpointer d)
{	
	/* Anything to explain? I don't think so */
	GtkWidget *child;
	Chattab *tab;
	child = gtk_notebook_get_nth_page(b, n);
	tab = (Chattab *)g_object_get_data(G_OBJECT(child), "chattab-data");
	if(tab) /* just in case */
		gtk_label_set_text(GTK_LABEL(tab->label), tab->title);
} /* reset_tab_title */

static void
scroll_tab_down(Chattab *tab)
{
	/* No explanations needed */
	GtkAdjustment *adj;
	adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(tab->scrolled));
	gtk_adjustment_set_value(adj, adj->upper);
} /* scroll_tab_down */

static void
setup_cbox(GtkWidget *cbox)
{
	/* Internal one setting up status combo box.
	 * Just here to make ui_setup() readable.
	 * TODO: Make it inline? */
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Online");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Free for chat");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Away");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Not available");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Do not disturb");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Offline");
	gtk_combo_box_set_active(GTK_COMBO_BOX(cbox), 0);
	g_signal_connect(G_OBJECT(cbox), "changed",
	                 G_CALLBACK(cbox_changed_cb), NULL);
} /* setup_cbox */

static void
set_wm_urgency()
{
	/* Nothing to explain */
	if(gtk_window_get_urgency_hint(GTK_WINDOW(window)))
		gtk_window_set_urgency_hint(GTK_WINDOW(window), FALSE);
	gtk_window_set_urgency_hint(GTK_WINDOW(window), TRUE);
} /* set_wm_urgency */

static void
tab_entry_handler(GtkWidget *e, gpointer p)
{
	/* Internal one, universal handler for tabs' "activate" signal
	 * calling commands_exec() from commands.c or sending message
	 * via xmpp_send_message(), depending on whether
	 * we are in status tab, or chat tab */
	Chattab *tab = (Chattab *)p;
	const gchar *input = gtk_entry_get_text(GTK_ENTRY(e));
	if(tab->jid) {
		gchar *str;
		xmpp_send_message(tab->jid, input);
		str = g_strdup_printf("--> %s\n", input);
		append_to_tab(tab, str);
		g_free(str);
	} else {
		if(commands_exec(input)) {
			ui_status_print("Error: unknown command\n");
		}
	}
	gtk_entry_set_text(GTK_ENTRY(e), "");
} /* tab_entry_handler */

void
ui_create_tab(Chattab *tab)
{
	/* Okay, here's a big one. jid and title are to be put
	 * into allocated tab by the caller, which may be not too good. TODO?
	 * It's called either when user clicks on the buddy in the roster
	 * (ui_roster.c), or when the new message arrives (xmpp.c,
	 * message_handler), I think there probably should be some function here,
	 * in ui.c, so we can make ui_create_tab() internal, and this handlers
	 * won't have to allocate space for Chattab. */
	GtkWidget *tview, *entry;
	/* creating GtkLabel for the tab title */
	tab->label = gtk_label_new(tab->title);
	/* creating GtkTextView for status messages */
	tview = gtk_text_view_new();
	tab->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tview));
	gtk_text_view_set_editable(GTK_TEXT_VIEW(tview), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tview), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tview), GTK_WRAP_WORD);
	/* we're putting this in a scrolled window */
	tab->scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tab->scrolled),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(tab->scrolled),
	                                      tview);
	/* setting up the entry field */
	entry = gtk_entry_new();
	/* Sending messages/running commands, depending on a tab type.
	 * Handler decides, it's universal  */
	g_signal_connect(G_OBJECT(entry), "activate",
	                 G_CALLBACK(tab_entry_handler), (void *)tab);
	/* some vbox to put it together */
	tab->vbox = gtk_vbox_new(FALSE, 0);
	/* this will help us finding Chattab struct by the child widget */
	g_object_set_data(G_OBJECT(tab->vbox), "chattab-data", tab);
	/* now let's put it all together */
	gtk_container_add(GTK_CONTAINER(tab->vbox), tab->scrolled);
	gtk_container_add(GTK_CONTAINER(tab->vbox), entry);
	gtk_box_set_child_packing(GTK_BOX(tab->vbox), entry, FALSE,
	                          FALSE, 0, GTK_PACK_START);
	gtk_widget_show_all(tab->vbox);
	/* aaand, launch! */
	gtk_notebook_append_page(GTK_NOTEBOOK(nbook), tab->vbox, tab->label);
	tabs = g_slist_prepend(tabs, tab);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(nbook),
	                              gtk_notebook_page_num(GTK_NOTEBOOK(nbook),
	                                                    tab->vbox));
	gtk_widget_grab_focus(entry);
} /* ui_create_tab */

XmppStatus
ui_get_status(void)
{
	/* A piece of code speaks a thousand words of a comment */
	switch(gtk_combo_box_get_active(GTK_COMBO_BOX(status_cbox))) {
		case 1:
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

const char
*ui_get_status_msg() {
	/* Just like this, so callers from different files
	 * don't mess with our local variables */
	return gtk_entry_get_text(GTK_ENTRY(status_entry));
} /* ui_get_status_msg */

void
ui_setup(int *argc, char **argv[])
{
	/* The first thing that occurs in the program, sets up the interface */
	GtkWidget *hpaned, *leftbox, *rwin;
	gtk_init(argc, argv);
	/* creating widgets */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	hpaned = gtk_hpaned_new();
	leftbox = gtk_vbox_new(FALSE, 0);
	rwin = gtk_scrolled_window_new(NULL, NULL);
	status_cbox = gtk_combo_box_new_text();
	nbook = gtk_notebook_new();
	rview = ui_roster_setup();
	/* setting up the more exciting ones */
	setup_cbox(status_cbox);
	status_entry = gtk_entry_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(nbook), TRUE);
	/* signal for reseting (unbolding) tab titles when switched to 'em */
	g_signal_connect(G_OBJECT(nbook), "switch-page",
					G_CALLBACK(reset_tab_title), NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(rwin),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);
	/* setting up status tab */
	status_tab = g_malloc(sizeof(Chattab));
	status_tab->jid = NULL;
	status_tab->title = g_strdup("Status");
	ui_create_tab(status_tab);
	/* packing */
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(rwin), rview);
	gtk_container_add(GTK_CONTAINER(window), hpaned);
	gtk_paned_add1(GTK_PANED(hpaned), leftbox);
	gtk_container_add(GTK_CONTAINER(leftbox), rwin);
	gtk_container_add(GTK_CONTAINER(leftbox), status_cbox);
	gtk_container_add(GTK_CONTAINER(leftbox), status_entry);
	gtk_paned_add2(GTK_PANED(hpaned), nbook);
	gtk_box_set_child_packing(GTK_BOX(leftbox), status_cbox,
	                          FALSE, FALSE, 0, GTK_PACK_START);
	gtk_box_set_child_packing(GTK_BOX(leftbox), status_entry,
	                          FALSE, FALSE, 0, GTK_PACK_START);
	/* setting up signals */
	g_signal_connect(G_OBJECT(window), "destroy",
	                 G_CALLBACK(destroy), NULL);
	g_signal_connect(G_OBJECT(status_entry), "activate",
	                 G_CALLBACK(xmpp_set_status), NULL);
	g_signal_connect(G_OBJECT(window), "key-press-event",
	                 G_CALLBACK(keypress_cb), NULL);
	/*go go go!*/
	gtk_widget_show_all(window);
} /* ui_setup */

void
ui_set_status(XmppStatus s)
{
	/* Another proxy, so remote functions don't touch our local vars */
	gtk_combo_box_set_active(GTK_COMBO_BOX(status_cbox), s);
} /* ui_set_status */

void
ui_set_status_msg(const char *m)
{
	/* Another proxy, called by commands.c on 'set status_msg' command */
	gtk_entry_set_text(GTK_ENTRY(status_entry), m);
	xmpp_set_status(ui_get_status());
} /* ui_set_status_msg */

void
ui_status_print(const char *msg, ...)
{
	/* Our printf() substitute, almost everything
	 * that happens is reported here */
	va_list ap;
	char *str;
	va_start(ap, msg);
	str = g_strdup_vprintf(msg, ap);
	append_to_tab(status_tab, str);
	va_end(ap);
	g_free(str);
} /* ui_status_print */

void
ui_tab_print_message(const char *jid, const char *msg)
{
	/* Called by xmpp_mesg_handler(), prints the incoming message
	 * to the approprate chat tab */
	Chattab *tab;
	char *str;
	tab = find_tab_by_jid(jid);
	if(!tab) {
		Buddy *sb;
		char *shortjid, *slash;
		/* We need to obtain the jid itself, w/o resource,
		 * to lookup the appropriate chat tab */
		slash = strchr(jid, '/');
		shortjid = (slash) ? g_strndup(jid, slash-jid) : g_strdup(jid);
		sb = xmpp_roster_find_by_jid(shortjid);
		if(!sb) {
			/* Something, somewhere went horribly wrong */
			g_printerr("ui_tab_print_message: Buddy %s not found.\n", shortjid);
			g_free(shortjid);
			return;
		}
		tab = g_malloc(sizeof(Chattab));
		tab->jid = g_strdup(jid);
		tab->title = g_strdup(sb->name);
		ui_create_tab(tab);
	}
	/* actual message printing - two lines of this whole function! */
	str = g_strdup_printf("<-- %s\n", msg);
	append_to_tab(tab, str);
	/* bolding tab title if it's not the status tab
	 * (the function will check whether the tab is active or not,
	 * we don't care about this) */
	if(tab->jid)
		tab_notify(tab);
	g_free(str);
} /* ui_tab_print_message */
