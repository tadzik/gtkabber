#ifndef XMPP_MESG_H
#define XMPP_MESG_H
#include "types.h"
#include <loudmouth/loudmouth.h>

LmHandlerResult
xmpp_mesg_handler(LmMessageHandler *h, LmConnection *c,
		LmMessage *m, gpointer udata);
void xmpp_mesg_send(const char *, const char *);

#endif
