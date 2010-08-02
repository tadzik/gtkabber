#include "xmpp_iq.h"

#include "xmpp_roster.h"

LmHandlerResult
xmpp_iq_handler(LmMessageHandler *h, LmConnection *c,
		LmMessage *m, gpointer userdata)
{
	LmMessageNode *query;
	query = lm_message_node_get_child(m->node, "query");
	if(query) {
		if(g_strcmp0(lm_message_node_get_attribute(query, "xmlns"),
		             "jabber:iq:roster") == 0) {
			xmpp_roster_parse_query(query);
		}
	}
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	UNUSED(c);
	UNUSED(h);
	UNUSED(userdata);
}
