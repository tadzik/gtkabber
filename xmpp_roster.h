#ifndef XMPP_ROSTER_H
#define XMPP_ROSTER_H
#include "types.h"
#include <loudmouth/loudmouth.h>
void xmpp_roster_add_resource(Buddy *, Resource *);
void xmpp_roster_cleanup(void);
Buddy *xmpp_roster_find_by_jid(const char *);
Resource *xmpp_roster_find_res_by_name(Buddy *, const char *);
const char *xmpp_roster_get_best_resname(const char *);
void xmpp_roster_parse_query(LmConnection *, LmMessageNode *);
void xmpp_roster_request(LmConnection *);
#endif
