#ifndef TYPES_H
#define TYPES_H
#include <gtk/gtk.h>

typedef enum {
	TO,
	FROM,
	BOTH,
	NONE
} XmppSubscription;

typedef enum {
	STATUS_ONLINE,
	STATUS_FFC,
	STATUS_AWAY,
	STATUS_XA,
	STATUS_DND,
	STATUS_OFFLINE
} XmppStatus;

typedef struct {
	/* Strings in name, jid and group are being allocated
	 * when the roster is parsed xmpp_roster_parse_query(),
	 * and freed at the very end, in xmpp_roster_cleanup(),
	 * which is called by xmpp_cleanup()
	 * 
	 * Resources list is automagically modified when the presence is handled,
	 * the elements are being added when the new resource arrives
	 * (xmpp_pres_handler()) and are being freed
	 * together with Buddy in xmpp_roster_cleanup() */
	gchar *name;
	gchar *jid;
	gchar *group;
	XmppSubscription subscription;
	GSList *resources;
} Buddy;

typedef struct {
	/* No memory is allocated within this structure, jid and name are
	 * just pointers to appropriate Buddy fields */
	GtkTreeIter iter;
	const gchar *jid;
	const gchar *name;
} UiBuddy;

typedef struct {
	/* Everything is freed together with owning Buddy in xmpp_roster_cleanup()*/
	gchar *name;
	XmppStatus status;
	gchar *status_msg;
	gint priority;
} Resource;

typedef struct {
	/* jid and title are allocated in ui_add_tab and freed in free_all_tabs (ui.c)
	 * when the app exits. We're also keeping pointers to a few widgets here:
	 * buffer: used for printing messages to tabs, ui_tab_print_message()
	 *         and internal tab's GtkEntry activate signal handler
	 * label: used by reset_tab_title() (ui.c), and tab_notify() (ibidem)
	 * scrolled: used in scroll_tab_down() on new message (also ui.c)
	 * vbox is GtkNotebook's child widget, and we use it to identify
	 *      tab via various gtk_notebook_ set of functions */
	gchar *jid;
	gchar *title;
	GtkTextBuffer *buffer;
	GtkWidget *label;
	GtkWidget *scrolled;
	GtkWidget *vbox;
} Chattab;
#endif
