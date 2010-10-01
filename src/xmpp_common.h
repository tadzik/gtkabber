#ifndef XMPP_COMMON_H
#define XMPP_COMMON_H
#include <glib.h>

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

#endif
