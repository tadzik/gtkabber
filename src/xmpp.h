#ifndef XMPP_H
#define XMPP_H
#include "types.h"
void xmpp_cleanup(void);
void xmpp_init(void);
void xmpp_send_message(const char *, const char *);
void xmpp_send_status(const char *, XmppStatus, const char *);
gchar *xmpp_status_readable(XmppStatus);
void xmpp_subscribe(const char *, const char *, const char *);
void xmpp_subscr_response(char *, int);
void xmpp_roster_parsed_cb(void);
#endif
