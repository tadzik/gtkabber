#ifndef XMPP_H
#define XMPP_H
#include <gtk/gtk.h>
#include "types.h"
void xmpp_cleanup(void);
void xmpp_init(void);
void xmpp_send_message(const gchar *, const gchar *);
void xmpp_set_status(XmppStatus);
void xmpp_subscribe(gchar *);
void xmpp_subscr_response(gchar *, gint);
void xmpp_roster_parsed_cb(void);
#endif
