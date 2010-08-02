#ifndef XMPP_CONN_H
#define XMPP_CONN_H
#include "types.h"
#include <loudmouth/loudmouth.h>

void xmpp_init(void);
void xmpp_cleanup(void);
void xmpp_connect(void);
LmConnection * xmpp_connection_get(void);
int xmpp_connection_is_open(void);
void xmpp_disconnect(void);
void xmpp_roster_parsed_cb(void);

#endif
