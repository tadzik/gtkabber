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
	char *name;
	char *jid;
	char *group;
	XmppSubscription subscription;
	GSList *resources;
} Buddy;

typedef struct {
	GtkTreeIter iter;
	const char *jid; /* sth like main key, pointer to Buddy.jid */
	const char *name; /* it's gonna be a pointer to Buddy.name */
} UiBuddy;

typedef struct {
	char *name;
	XmppStatus status;
	char *status_msg;
	int priority;
} Resource;

typedef struct {
	char *jid;
	char *title;
	GtkTextBuffer *buffer;
	GtkWidget *entry;
	GtkWidget *scrolled;
} Chattab;
#endif
