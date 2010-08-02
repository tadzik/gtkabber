#ifndef XMPP_PRES_H
#define XMPP_PRES_H
#include "types.h"
#include <loudmouth/loudmouth.h>

LmHandlerResult xmpp_pres_handler(LmMessageHandler *h, LmConnection *c,
				LmMessage *m, gpointer udata);
void xmpp_send_status(const char *, XmppStatus, const char *);
gchar *xmpp_status_readable(XmppStatus);
void xmpp_subscribe(const char *, const char *, const char *);
void xmpp_subscr_response(char *, int);

#endif
