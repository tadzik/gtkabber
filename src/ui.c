#include "ui.h"

#include <gtk/gtk.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "xmpp.h"
#include "xmpp_roster.h"
#include "ui_roster.h"
#include "ui_tabs.h"
#include "config.h"
#include "mlentry.h"

/* functions */
static void cbox_changed_cb(GtkComboBox *, gpointer);
static void destroy(GtkWidget *, gpointer);
static void focus_cb(GtkWidget *, GdkEventFocus *, gpointer);
static void infobar_response_cb(GtkInfoBar *, gint, gpointer);
static gboolean keypress_cb(GtkWidget *, GdkEventKey *, gpointer);
static void setup_cbox(GtkWidget *);
static void status_changed(GtkWidget *, const char *, gpointer);
static void subscr_response_cb(GtkButton *, gpointer);
static void toggle_options(void);
/*************/

/* global variables */
static GtkWidget *toolbox, *rview, *status_cbox, *status_entry, *window;
/********************/

static void
action_cb(GtkButton *b, gpointer i)
{
	action_call(GPOINTER_TO_INT(i));
	UNUSED(b);
} /* action_cb */

static void
cbox_changed_cb(GtkComboBox *e, gpointer p)
{
	xmpp_send_status(NULL, ui_get_status(), NULL);
	UNUSED(e);
	UNUSED(p);
} /* cbox_changed_cb */

static void
destroy(GtkWidget *widget, gpointer data)
{
	xmpp_cleanup();
	ui_roster_cleanup();
	gtk_main_quit();
	ui_tab_cleanup();
	UNUSED(widget);
	UNUSED(data);
} /* destroy */

static void
focus_cb(GtkWidget *w, GdkEventFocus *f, gpointer p)
{
	ui_tab_focus();
	if(gtk_window_get_urgency_hint(GTK_WINDOW(window))) {
		gtk_window_set_urgency_hint(GTK_WINDOW(window), FALSE);
	}
	UNUSED(w);
	UNUSED(f);
	UNUSED(p);
} /* focus_cb */

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
	int i;
	/* alt + num: changing tabs */
	if (e->state & GDK_MOD1_MASK) {
		for (i = 49; i < 58; i++) {
			if ((int)e->keyval == i) {
				ui_tab_set_page(i - 49);
				return TRUE;
			}
		}
	}
	/* Ctrl+something actions */
	if (e->state & GDK_CONTROL_MASK) {
		switch(e->keyval) {
		case 104: /* h */
			ui_roster_toggle_offline();
			return TRUE;
		case 111: /* o */
			toggle_options();
			return TRUE;
		case 113: /* q */
			ui_tab_close(NULL);
			return TRUE;
		case 114: /* r */
			if(e->state & GDK_MOD1_MASK) { /* with shift */
				ui_print("Config file reloaded\n");
				config_reload();
			} else {
				gtk_widget_grab_focus(rview);
			}
			return TRUE;
		case 115: /* s */
			if(e->state & GDK_MOD1_MASK) /* with alt */
				gtk_widget_grab_focus(status_entry);
			else
				gtk_widget_grab_focus(status_cbox);
			return TRUE;
		case 116: /* t */
			ui_tab_focus();
			return TRUE;
		case 65365: /* PgUp */
			ui_tab_prev();
			return TRUE;
		case 65366: /* PgDn */
			ui_tab_next();
			return TRUE;
		}
	}

	return FALSE;
	UNUSED(w);
	UNUSED(u);
} /* keypress_cb */

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
status_changed(GtkWidget *w, const char *e, gpointer p)
{
	xmpp_send_status(NULL, ui_get_status(), NULL);
	ui_tab_focus();
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
	GtkWidget *hpaned, *leftbox, *nbook, *rwin, *vbox;
	gtk_init(argc, argv);
	/* creating widgets */
	hpaned = gtk_hpaned_new();
	toolbox = gtk_vbox_new(FALSE, 0);
	leftbox = gtk_vbox_new(FALSE, 0);
	nbook = ui_tab_init();
	rwin = gtk_scrolled_window_new(NULL, NULL);
	status_cbox = gtk_combo_box_new_text();
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	vbox = gtk_vbox_new(FALSE, 0);
	/* setting up the more exciting ones */
	rview = ui_roster_setup();
	setup_cbox(status_cbox);
	status_entry = mlentry_new(status_changed, NULL);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(status_entry), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(status_entry), FALSE);
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
ui_set_wm_urgency(void)
{
	if(gtk_window_is_active(GTK_WINDOW(window)))
		return;
	if(gtk_window_get_urgency_hint(GTK_WINDOW(window)))
		gtk_window_set_urgency_hint(GTK_WINDOW(window), FALSE);
	gtk_window_set_urgency_hint(GTK_WINDOW(window), TRUE);
} /* ui_set_wm_urgency */

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
	ui_set_wm_urgency();
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
	ui_tab_append_text(NULL, str);
	/* looks stupid? That's because old gcc is stupid */
	g_printerr("%s", str);
	va_end(ap);
	g_free(str);
} /* ui_print */
