#include "xmpp_mesg.h"

#include "config.h"
#include "ui.h"
#include "ui_tabs.h"
#include "xmpp_conn.h"

LmHandlerResult
xmpp_mesg_handler(LmMessageHandler *h, LmConnection *c,
		LmMessage *m, gpointer udata)
{
	const char *from, *to, *body;
	LmMessageNode *node;
	from = lm_message_node_get_attribute(m->node, "from");
	to = lm_message_node_get_attribute(m->node, "to");
	node = lm_message_node_get_child(m->node, "body");
	if(node && (body = lm_message_node_get_value(node))) {
		ui_tab_print_message(from, body);
		lua_msg_callback(from, to, body);
	}
	/* we're actually ignoring <subject> and <thread> elements,
	 * as I've never actually seen them being used. If you do, and you care,
	 * feel obliged to mail me and yell at me */
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	UNUSED(h);
	UNUSED(c);
	UNUSED(udata);
} /* xmpp_mesg_handler */

void
xmpp_mesg_send(const char *to, const char *msg)
{
	LmMessage *m;
	GError *err = NULL;
	m = lm_message_new_with_sub_type(to, LM_MESSAGE_TYPE_MESSAGE,
	                                 LM_MESSAGE_SUB_TYPE_CHAT);
	lm_message_node_add_child(m->node, "body", msg);
	if(!lm_connection_send(xmpp_connection_get(), m, &err)) {
		ui_print("Error sending message: %s\n", err->message);
		g_error_free(err);
	}
	lua_msg_callback(NULL, to, msg);
	lm_message_unref(m);
} /* xmpp_send_message */

