#ifndef XMPP_ROSTER_H
#define XMPP_ROSTER_H
#include <glib.h>
#include "xmpp_common.h"

typedef struct {
    gchar *name;
    gchar *jid;
    gchar *group;
    XmppSubscription subscription;
    GSList *resources;
} Buddy;

typedef struct {
	gchar *name;
	XmppStatus status;
	gchar *status_msg;
	gint priority;
} Resource;

#endif
