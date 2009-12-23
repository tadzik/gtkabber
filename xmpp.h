#ifndef XMPP_H
#define XMPP_H
#include <gtk/gtk.h>
#include "types.h"
void xmpp_cleanup(void);
void xmpp_init(void);
void xmpp_send_message(const char *, const char *);
void xmpp_set_status(XmppStatus);
#endif
