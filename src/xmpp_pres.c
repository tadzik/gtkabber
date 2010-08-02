#include "xmpp_pres.h"

#include "config.h"
#include "ui.h"
#include "ui_roster.h"
#include "xmpp_conn.h"
#include "xmpp_roster.h"

#include <loudmouth/loudmouth.h>
#include <stdlib.h>
#include <string.h>

/************************/
static void parse_err_presence(LmMessage *);
static void parse_status_presence(LmMessage *);
static void parse_subscr_presence(LmMessage *);
static gchar *status_to_str(XmppStatus);
/************************/

static void
parse_err_presence(LmMessage *m)
{
	const gchar *from, *code;
	LmMessageNode *node;
	from = lm_message_node_get_attribute(m->node, "from");
	node = lm_message_node_get_child(m->node, "error");
	code = lm_message_node_get_attribute(node, "code");
	ui_print("Got presence of type 'error' from %s (error code %s)\n", from, code);
} /* parse_err_presence */

static void
parse_status_presence(LmMessage *m)
{
	const char *buf;
	char *jid, *resname, *sep;
	Buddy *sb;
	Resource *res;
	LmMessageNode *child;
	buf = lm_message_node_get_attribute(m->node, "from");
	sep = strchr(buf, '/');
	if(sep) {
		jid = g_strndup(buf, sep-buf);
		resname = g_strdup(sep+1);
	} else {
		jid = g_strdup(buf);
		resname = g_strdup("default");
	}
	sb = xmpp_roster_find_by_jid(jid);
	if(!sb) {
		g_free(jid);
		g_free(resname);
		return;
	}
	res = xmpp_roster_find_res_by_name(sb, resname);
	if(!res) {
		/* we have to create a new resource */	
		res = g_malloc(sizeof(Resource));
		res->name = (char *)resname;
		res->status_msg = NULL;
		xmpp_roster_add_resource(sb, res);
	}
	/* checking presence type (if available) */
	if((buf = lm_message_node_get_attribute(m->node, "type"))) {
		if(g_strcmp0(buf, "unavailable") == 0)
			res->status = STATUS_OFFLINE;
	} else {
		res->status = STATUS_ONLINE;	
	}
	/* checking for some specific status */
	if((child = lm_message_node_get_child(m->node, "show"))) {
		buf = lm_message_node_get_value(child);
		if(!buf) /* Do nothing, keep STATUS_ONLINE */;
		else if(buf[0] == 'a') res->status = STATUS_AWAY;
		else if(buf[0] == 'c') res->status = STATUS_FFC;
		else if(buf[0] == 'x') res->status = STATUS_XA;
		else if(buf[0] == 'd') res->status = STATUS_DND;
		else {
			ui_print("Invalid <show> in presence from %s (%s), ignoring\n",
			                jid, buf);
		}
	}
	/* checking if resource has some previous status message.
	 * If it does, purge it */
	if(res->status_msg) {
		g_free(res->status_msg);
		res->status_msg = NULL;
	}
	/* checking for status message */
	child = lm_message_node_get_child(m->node, "status");
	if (child && (buf = lm_message_node_get_value(child)))
		res->status_msg = g_strdup(buf);
	/* checking priority (if provided) */
	child = lm_message_node_get_child(m->node, "priority");
	if(child && (buf = lm_message_node_get_value(child)))
		res->priority = atoi(buf);
	else
		res->priority = 0;
	/* printing a message to status window and updating roster entry in ui */
	lua_pres_callback(sb->jid, xmpp_status_readable(res->status),
	                  res->status_msg);	
	ui_roster_update(sb->jid);
} /* parse_status_presence */

static void
parse_subscr_presence(LmMessage *m)
{
	const char *type, *jid;
	type = lm_message_node_get_attribute(m->node, "type");
	jid = lm_message_node_get_attribute(m->node, "from");
	ui_print("Got presence subscription, type '%s' from %s\n", type, jid);
	if(!type || (g_strcmp0(type, "subscribe") == 0))
		ui_show_presence_query(jid);
} /* parse_subscr_presence */

LmHandlerResult
xmpp_pres_handler(LmMessageHandler *h, LmConnection *c, LmMessage *m,
             gpointer udata)
{
	const char *type = lm_message_node_get_attribute(m->node, "type");
	if(!type || (g_strcmp0(type, "unavailable") == 0)) {
		parse_status_presence(m);
	} else {
		if(strncmp(type, "subscribe", strlen("subscribe")) == 0
		|| strncmp(type, "unsubscribe", strlen("unsubscribe")) == 0)
			parse_subscr_presence(m);
		else if(g_strcmp0(type, "error") == 0)
			parse_err_presence(m);
		else
			ui_print("Got presence of type %s\n", type);
	}
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	UNUSED(h);
	UNUSED(c);
	UNUSED(udata);
} /* pres_handler */

void
xmpp_send_status(const gchar *to, XmppStatus s, const gchar *msg)
{
	LmMessage *p;
	GError *err = NULL;
	const gchar *status, *status_msg;
	gchar *conf_priority;
	if (s == STATUS_OFFLINE) {
		xmpp_disconnect();
		return;
	}
	if (!xmpp_connection_is_open()) {
		ui_print("Not connected, connecting\n");
		xmpp_connect();
		return;
	}
	conf_priority = g_strdup_printf("%d", get_settings_int(PRIORITY));
	status_msg = (msg) ? msg : ui_get_status_msg();
	p = lm_message_new_with_sub_type(to, LM_MESSAGE_TYPE_PRESENCE,
	                                 (s == STATUS_OFFLINE)
	                                 	? LM_MESSAGE_SUB_TYPE_UNAVAILABLE
	                                 	: LM_MESSAGE_SUB_TYPE_AVAILABLE);
	lm_message_node_add_child(p->node, "priority", conf_priority);
	g_free(conf_priority);
	status = status_to_str(s);
	if(status) {
		lm_message_node_add_child(p->node, "show", status);
	}
	if(g_strcmp0(status_msg, "") != 0)
		lm_message_node_add_child(p->node, "status", status_msg);
	if(!lm_connection_send(xmpp_connection_get(), p, &err)) {
		ui_print("Error sending presence: %s\n", err->message);
		g_error_free(err);
	}
	lm_message_unref(p);
} /* xmpp_send_status */

static char *
status_to_str(XmppStatus status)
{
	if(status == STATUS_FFC) return "chat";
	else if(status == STATUS_AWAY) return "away";
	else if(status == STATUS_XA) return "xa";
	else if(status == STATUS_DND) return "dnd";
	else return NULL;
} /* status_to_str */

void xmpp_subscribe(const gchar *j, const gchar *n, const gchar *g) {
	LmMessage *msg;
	LmMessageNode *query, *item;
	GError *err = NULL;
	/* if buddy not in our roster, add him */
	if(!xmpp_roster_find_by_jid(j)) {
		msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ,
	                                           LM_MESSAGE_SUB_TYPE_SET);
		query = lm_message_node_add_child(msg->node, "query", NULL);
		lm_message_node_set_attribute(query, "xmlns", "jabber:iq:roster");
		item = lm_message_node_add_child(query, "item", NULL);
		lm_message_node_set_attribute(item, "jid", j);
		lm_message_node_set_attribute(item, "name", n);
		if(g) {
			LmMessageNode *group;	
			group = lm_message_node_add_child(item, "group", g);
		}
		if(!lm_connection_send(xmpp_connection_get(), msg, &err)) {
			ui_print("Error adding buddy to roster: %s\n", err->message);
			g_error_free(err);
		}
		lm_message_unref(msg);
	}
	/* sending subscription request */	
	msg = lm_message_new_with_sub_type(j, LM_MESSAGE_TYPE_PRESENCE,
	                                   LM_MESSAGE_SUB_TYPE_SUBSCRIBE);
	if(!lm_connection_send(xmpp_connection_get(), msg, &err)) {
		ui_print("Error requesting subscription: %s\n", err->message);
		g_error_free(err);
	}
	lm_message_unref(msg);
} /* xmpp_subscribe */

void
xmpp_subscr_response(gchar *j, gint s)
{
	LmMessage *msg;
	GError *err = NULL;
	msg = lm_message_new_with_sub_type(j, LM_MESSAGE_TYPE_PRESENCE,
	                                   (s) ? LM_MESSAGE_SUB_TYPE_SUBSCRIBED
					       : LM_MESSAGE_SUB_TYPE_UNSUBSCRIBED);
	if(!lm_connection_send(xmpp_connection_get(), msg, &err)) {
		ui_print("Error sending subscription response: %s\n", err->message);
		g_error_free(err);
	}
	lm_message_unref(msg);
} /* xmpp_subscr_response */

char *
xmpp_status_readable(XmppStatus st)
{
	if(st == STATUS_ONLINE) return "online";
	else if(st == STATUS_FFC) return "free for chat";
	else if(st == STATUS_AWAY) return "away";
	else if(st == STATUS_XA) return "not available";
	else if(st == STATUS_DND) return "do not disturb";
	else if(st == STATUS_OFFLINE) return "offline";
	else return "unknown";
} /* xmpp_status_readable */
