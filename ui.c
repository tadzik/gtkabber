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

/* functions */
static void append_to_tab(Chattab *, const char *, ...);
static void destroy(GtkWidget *, gpointer);
static Chattab *find_tab_by_jid(const char *);
static void free_all_tabs(void);
static void free_tab(gpointer, gpointer);
static gint match_tab_by_jid(gconstpointer, gconstpointer);
static void scroll_tab_down(Chattab *);
static void setup_cbox(GtkWidget *);
static void tab_entry_handler(GtkWidget *, gpointer);
void ui_create_tab(Chattab *);
XmppStatus ui_get_status(void);
const char *ui_get_status_msg(void);
void ui_setup(int *, char ***);
void ui_status_print(const char *msg, ...);
void ui_tab_print_message(const char *, const char *);
/*************/

/* global variables */
GtkWidget *nbook, *status_cbox, *status_entry;
GtkTextBuffer *status_buffer;
GSList *tabs;
/********************/

static void
append_to_tab(Chattab *t, const char *s, ...)
{
	/* this could be the base for ui_status_print, but I haven't
	 * yet figured out any way to reuse code. TODO? */
	GtkTextIter i;
	time_t now;
	char tstamp[12];
	GString *str;
	va_list ap;
	va_start(ap, s);
	now = time(NULL);
	gtk_text_buffer_get_end_iter(t->buffer, &i);
	strftime(tstamp, sizeof(tstamp), "[%H:%M:%S] ", localtime(&now));
	str = g_string_new(tstamp);
	g_string_append_vprintf(str, s, ap);
	gtk_text_buffer_insert(t->buffer, &i, str->str, str->len);
	va_end(ap);
	g_string_free(str, TRUE);
	scroll_tab_down(t);
} /* append_to_tab */

static void
destroy(GtkWidget *widget, gpointer data)
{
	/*this occurs when "destroy" event comes to our window*/
	xmpp_cleanup();
	ui_roster_cleanup();
	gtk_main_quit();
	free_all_tabs();
} /* destroy */

Chattab *
find_tab_by_jid(const char *jid)
{
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
	if(tabs) {
		g_slist_foreach(tabs, (GFunc)free_tab, NULL);
		g_slist_free(tabs);
	}
} /* free_all_tabs*/

static void
free_tab(gpointer t, gpointer u)
{
	Chattab *tab = (Chattab *)t;
	free(tab->jid);
	free(tab->title);
} /* free_tab */

static gint
match_tab_by_jid(gconstpointer elem, gconstpointer jid)
{
	Chattab *tab = (Chattab *)elem;
	return strcmp(tab->jid, jid);
} /* match_tab_by_jid */

static void
scroll_tab_down(Chattab *tab)
{
	GtkAdjustment *adj;
	adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(tab->scrolled));
	gtk_adjustment_set_value(adj, adj->upper);
} /* scroll_tab_down */

static void
setup_cbox(GtkWidget *cbox)
{
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Online");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Free for chat");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Away");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Not available");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Do not disturb");
	gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Offline");
	gtk_combo_box_set_active(GTK_COMBO_BOX(cbox), 0);
	g_signal_connect(G_OBJECT(cbox), "changed",
					G_CALLBACK(xmpp_set_status), NULL);
} /* setup_cbox */

static void
tab_entry_handler(GtkWidget *e, gpointer p)
{
	const char *input = gtk_entry_get_text(GTK_ENTRY(e));
	if(p) {
		Chattab *tab = (Chattab *)p;
		xmpp_send_message(tab->jid, input);
		append_to_tab(tab, "--> %s\n", input);
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
	/*FIXME: All these can be probably simplified somehow. Think about it*/
	/* entry is only for status tab. Can we get rid of it? TODO */
	GtkWidget *vbox, *tview, *scrolled, *label, *entry;
	/* creating GtkLabel for the tab title */
	label = gtk_label_new((tab) ? tab->title : "Status");
	/* creating GtkTextView for status messages */
	tview = gtk_text_view_new();
	if(tab) {
		tab->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tview));
		g_printerr("Creating tab for %s\n", tab->jid);
	} else {
		status_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tview));
	}
	gtk_text_view_set_editable(GTK_TEXT_VIEW(tview), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tview), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tview), GTK_WRAP_WORD);
	/* we're putting this in a scrolled window */
	if(tab) {
		tab->scrolled = gtk_scrolled_window_new(NULL, NULL);
		scrolled = tab->scrolled;
	} else {
		scrolled = gtk_scrolled_window_new(NULL, NULL);
	}
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled),
											tview);
	/* setting up the entry field */
	if(tab) {
		tab->entry = gtk_entry_new();
		entry = tab->entry;
	} else {
		entry = gtk_entry_new();
	}
	/* Sending messages/running commands, depending on a tab type */
	if(tab) {
		g_signal_connect(G_OBJECT(entry), "activate",
						G_CALLBACK(tab_entry_handler), (void *)tab);
	} else {
		g_signal_connect(G_OBJECT(entry), "activate",
						G_CALLBACK(tab_entry_handler), NULL);
	}
	/* some vbox to put it together */
	vbox = gtk_vbox_new(FALSE, 0);
	/* now let's put it all together */
	gtk_container_add(GTK_CONTAINER(vbox), scrolled);
	gtk_container_add(GTK_CONTAINER(vbox), entry);
	gtk_box_set_child_packing(GTK_BOX(vbox), entry, FALSE, FALSE,
								0, GTK_PACK_START);
	gtk_widget_show_all(vbox);
	/* aaand, launch! */
	gtk_notebook_append_page(GTK_NOTEBOOK(nbook), vbox, label);
	if(tab) {
		tabs = g_slist_prepend(tabs, tab);
	}
} /* ui_create_tab */

XmppStatus
ui_get_status(void)
{
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
	return gtk_entry_get_text(GTK_ENTRY(status_entry));
} /* ui_get_status_msg */

void
ui_setup(int *argc, char **argv[])
{
	/*here we set up the basic interface*/
	GtkWidget *window, *hpaned, *leftbox, *rwin, *roster;
	gtk_init(argc, argv);
	/*creating*/
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	hpaned = gtk_hpaned_new();
	leftbox = gtk_vbox_new(FALSE, 0);
	rwin = gtk_scrolled_window_new(NULL, NULL);
	status_cbox = gtk_combo_box_new_text();
	status_entry = gtk_entry_new();
	nbook = gtk_notebook_new();
	roster = ui_roster_setup();
	/*setting up the more exciting ones*/
	setup_cbox(status_cbox);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(nbook), TRUE);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(rwin),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	ui_create_tab(NULL);
	/*packing*/
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(rwin), roster);
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
	/*signals*/
	g_signal_connect(G_OBJECT(window), "destroy",
					G_CALLBACK(destroy), NULL);
	g_signal_connect(G_OBJECT(status_entry), "activate",
					G_CALLBACK(xmpp_set_status), NULL);
	/*go go go!*/
	gtk_widget_show_all(window);
} /* ui_setup */

void
ui_status_print(const char *msg, ...)
{
	va_list ap;
	GtkTextIter i;
	time_t now;
	char tstamp[12];
	GString *str;
	va_start(ap, msg);
	now = time(NULL);
	gtk_text_buffer_get_end_iter(status_buffer, &i);
	strftime(tstamp, sizeof(tstamp), "[%H:%M:%S] ", localtime(&now));
	str = g_string_new(tstamp);
	g_string_append_vprintf(str, msg, ap);
	gtk_text_buffer_insert(status_buffer, &i, str->str, str->len);
	va_end(ap);
	g_string_free(str, TRUE);
} /* ui_status_print */

void
ui_tab_print_message(const char *jid, const char *msg)
{
	Chattab *tab;
	GString *str;
	tab = find_tab_by_jid(jid);
	if(!tab) {
		Buddy *sb;
		char *shortjid, *slash;
		slash = index(jid, '/');
		if(!slash) {
			g_printerr("ui_tab_print_message: "
						"How do I get message from someone with no resource? "
						"Who the fuck is %s supposed to be? Shit!\n", jid);
			return;
		}
		shortjid = strndup(jid, slash-jid);
		g_printerr("Not found, creating new, for %s\n", shortjid);
		sb = xmpp_roster_find_by_jid(shortjid);
		if(!sb) {
			g_printerr("Buddy %s not found. You've fucked pointers my friend :>\n",
						shortjid);
			return;
		}
		tab = malloc(sizeof(Chattab));
		tab->jid = strdup(jid);
		tab->title = strdup(sb->name);
		ui_create_tab(tab);
	}
	str = g_string_new("<-- ");
	g_string_append_printf(str, "%s\n", msg);
	append_to_tab(tab, str->str);
	g_string_free(str, TRUE);
} /* ui_tab_print_message */
