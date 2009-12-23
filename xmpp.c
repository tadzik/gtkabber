#include <loudmouth/loudmouth.h>
#include <gtk/gtk.h> /*fucking callback args*/
#include <string.h> /*xmpp_iq_handler needs this, so far only it*/
#include <stdlib.h> /*malloc*/
#include "commands.h"
#include "ui.h"
#include "ui_roster.h"
#include "xmpp_roster.h"
#include "types.h"

/* fetched from commands.c */
extern char conf_passwd[];
extern int conf_port;
extern char conf_priority[];
extern char conf_resource[];
extern char conf_server[];
extern char conf_username[];
/*************************/

/* functions */
static void connection_auth_cb(LmConnection *, gboolean, gpointer);
static void connection_disconnect_cb(LmConnection *, LmDisconnectReason,
										gpointer);
static void connection_open_cb(LmConnection *, gboolean, gpointer);
void xmpp_cleanup();
static void disconnect();
static void connect();
void xmpp_init(void);
LmHandlerResult xmpp_iq_handler(LmMessageHandler *, LmConnection *,
								LmMessage *, gpointer);
LmHandlerResult xmpp_mesg_handler(LmMessageHandler *, LmConnection *,
									LmMessage *, gpointer);
LmHandlerResult xmpp_pres_handler(LmMessageHandler *, LmConnection *,
									LmMessage *, gpointer);
void xmpp_set_status(GtkEntry *, gpointer);
void xmpp_send_message(const char *, const char *);
static char *xmpp_status_to_str(XmppStatus);
static char *xmpp_status_readable(XmppStatus);
/*************/

/* global variables */
LmConnection *connection;
static LmMessageHandler *iq_handler;
static LmMessageHandler *pres_handler;
static LmMessageHandler *mesg_handler;
/********************/

static void
connect() {
	GError *error = NULL;
	if(!lm_connection_open(connection, (LmResultFunction)connection_open_cb,
				             NULL, NULL, &error)) {
	    ui_status_print("Error opening connection %s\n", error->message);
	    g_error_free(error);
	} else {
		ui_status_print("Connected to %s\n", conf_server);
	}
}

static void
connection_auth_cb(LmConnection *c, gboolean success, gpointer udata) {
	if(!success) {
		ui_status_print("ERROR: Authentication failed\n");
	} else {
		iq_handler = lm_message_handler_new(xmpp_iq_handler, NULL, NULL);
		lm_connection_register_message_handler(c, iq_handler,
												LM_MESSAGE_TYPE_IQ,
												LM_HANDLER_PRIORITY_NORMAL);
		mesg_handler = lm_message_handler_new(xmpp_mesg_handler, NULL, NULL);
		lm_connection_register_message_handler(c, mesg_handler,
												LM_MESSAGE_TYPE_MESSAGE,
												LM_HANDLER_PRIORITY_NORMAL);
		pres_handler = lm_message_handler_new(xmpp_pres_handler, NULL, NULL);
		lm_connection_register_message_handler(c, pres_handler,
												LM_MESSAGE_TYPE_PRESENCE,
												LM_HANDLER_PRIORITY_NORMAL);
		xmpp_roster_request(c);	
	}
}

static void
connection_disconnect_cb(LmConnection *c, LmDisconnectReason reason,
							gpointer udata) {
	switch(reason) {
	case LM_DISCONNECT_REASON_OK:
		ui_status_print("Connection closed as requested by user\n");
		break;
	case LM_DISCONNECT_REASON_PING_TIME_OUT:
		ui_status_print("Connection closed due to ping timeout\n");
		break;
	case LM_DISCONNECT_REASON_HUP:
		ui_status_print("Connection closed due to socket hungup\n");
		break;
	case LM_DISCONNECT_REASON_ERROR:
		ui_status_print("Connection closed due to error in transport layer\n");
		break;
	case LM_DISCONNECT_REASON_RESOURCE_CONFLICT:
		ui_status_print("Connection closed due to conflicting resources\n");
		break;
	case LM_DISCONNECT_REASON_INVALID_XML:
		ui_status_print("Connection closed due to invalid XML stream\n");
		break;
	default:
		ui_status_print("Connection closed due to unknown error\n");
	}
	if(reason != LM_DISCONNECT_REASON_OK) {
		ui_status_print("Reconnecting…\n");
		connect();
	}
}


static void
connection_open_cb(LmConnection *c, gboolean success, gpointer udata) {
	GError *error = NULL;
	if(success) {
		if(!lm_connection_authenticate(c, conf_username, conf_passwd,
										conf_resource, connection_auth_cb,
										NULL, NULL,	&error)) {
			ui_status_print("Error authenticating: %s\n", error->message);
			g_error_free(error);
		} else {
			ui_status_print("Authenticated as %s\n", conf_username);
		}
	}
}

static void
disconnect() {
	LmMessage *m;
	if(!connection)	return;
	if(lm_connection_get_state(connection)
		!= LM_CONNECTION_STATE_AUTHENTICATED) {
		m = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_PRESENCE,
										LM_MESSAGE_SUB_TYPE_UNAVAILABLE);
		lm_connection_send(connection, m, NULL);
	}
	if(lm_connection_is_open(connection)) {
		ui_status_print("Closing connection to %s\n", conf_server);
		lm_connection_close(connection, NULL);
	}
}

void
xmpp_cleanup() {
	disconnect();
	if(connection) lm_connection_unref(connection);
	xmpp_roster_cleanup();
}

void
xmpp_init(void) {
	if(commands_parse_rcfile() == 1) {
		ui_status_print("Could not establish connection: configuration missing\n");
	}
	connection = lm_connection_new(conf_server);
	/*TODO: Write this ssl thing*/
	lm_connection_set_keep_alive_rate(connection, 30);
	lm_connection_set_disconnect_function(connection, connection_disconnect_cb,
											NULL, NULL);
	connect();
}

LmHandlerResult
xmpp_iq_handler(LmMessageHandler *h, LmConnection *c, LmMessage *m,
				gpointer userdata) {
	LmMessageNode *query;
	query = lm_message_node_get_child(m->node, "query");
	if(query) {
		if(strcmp(lm_message_node_get_attribute(query, "xmlns"),
					"jabber:iq:roster") == 0) {
			xmpp_roster_parse_query(c, query);
			xmpp_set_status(NULL, NULL);
		}
	}
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

LmHandlerResult
xmpp_mesg_handler(LmMessageHandler *h, LmConnection *c, LmMessage *m,
					gpointer udata)
{
	const char *from, *body;
	LmMessageNode *bodynode;
	from = lm_message_node_get_attribute(m->node, "from");
	bodynode = lm_message_node_get_child(m->node, "body");
	if(bodynode) {
		body = lm_message_node_get_value(bodynode);
		if(body)
			ui_tab_print_message(from, body);
	}
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
} /* xmpp_mesg_handler */

LmHandlerResult
xmpp_pres_handler(LmMessageHandler *h, LmConnection *c, LmMessage *m,
					gpointer udata)
{
	const char *buf;
	char *jid, *resname, *sep;
	Buddy *sb;
	Resource *res;
	LmMessageNode *child;
	buf = lm_message_node_get_attribute(m->node, "from");
	sep = index(buf, '/');
	if(sep) {
		jid = strndup(buf, sep-buf);
		resname = strdup(sep+1);
	} else {
		jid = strdup(buf);
		resname = strdup("default");
	}
	sb = xmpp_roster_find_by_jid(jid);
	if(!sb) {
		g_printerr("Buddy %s not found in roster, exiting\n", jid);
		if(jid)
			free(jid);
		if(resname)
			free(resname);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	res = xmpp_roster_find_res_by_name(sb, resname);
	if(!res) {
		/* we have to create a new resource */	
		res = malloc(sizeof(Resource));
		res->name = (char *)resname;
		res->status_msg = NULL;
		xmpp_roster_add_resource(sb, res);
	}
	child = lm_message_node_get_child(m->node, "show");
	if(!child) {
		/* no "show" tag, so no specific status is to be set
	 	* checking whether it's online or offline */
		buf = lm_message_node_get_attribute(m->node, "type");
		if(!buf) {
			res->status = STATUS_ONLINE;	
		} else {
			if(strcmp(buf, "unavailable") == 0 || strcmp(buf, "error") == 0) {
				res->status = STATUS_OFFLINE;
			} else {
				ui_status_print("Presence type '%s', oh lawd, what to do?\n",
								buf);
				res->status = STATUS_OFFLINE;
			}
		}
	} else {
		/* it's some specific status
		 * (or at least I hope so :})*/
		buf = lm_message_node_get_value(child);
		if(!strcmp(buf, "away")) res->status = STATUS_AWAY;
		else if(!strcmp(buf, "chat")) res->status = STATUS_FFC;
		else if(!strcmp(buf, "xa")) res->status = STATUS_XA;
		else if(!strcmp(buf, "dnd")) res->status = STATUS_DND;
		else {
			ui_status_print("WHOOPS: What is show '%s' supposed to mean?\n",
							buf);
			res->status = STATUS_OFFLINE;
		}
	}
	child = lm_message_node_get_child(m->node, "priority");
	if(child) {
		res->priority = atoi(lm_message_node_get_value(child));
	} else {
		g_printerr("No priority given, using 0 as default\n");
		res->priority = 0;
	}
	if(res->status_msg) {
		free(res->status_msg);
		res->status_msg = NULL;
	}
	child = lm_message_node_get_child(m->node, "status");
	if (child) {
		buf = lm_message_node_get_value(child);
		if(buf) {
			res->status_msg = strdup(buf);
		}
	}
	ui_status_print("%s/%s is now %s (%s)\n", jid, resname,
					xmpp_status_readable(res->status),
					(res->status_msg) ? res->status_msg : "");
	ui_roster_update(sb->jid, res->status);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
} /* xmpp_pres_handler */

void
xmpp_send_message(const char *to, const char *msg)
{
	LmMessage *m;
	GError *err = NULL;
	m = lm_message_new_with_sub_type(to, LM_MESSAGE_TYPE_MESSAGE,
									LM_MESSAGE_SUB_TYPE_CHAT);
	lm_message_node_add_child(m->node, "body", msg);
	if(!lm_connection_send(connection, m, &err)) {
		ui_status_print("Error sending message: %s\n", err->message);
		g_error_free(err);
	}
	lm_message_unref(m);
}

void
xmpp_set_status(GtkEntry *entry, gpointer data)
{
	LmMessage *p;
	GError *err = NULL;
	const char *status, *status_msg;
	XmppStatus s;
	if(!connection || lm_connection_get_state(connection)
						!= LM_CONNECTION_STATE_AUTHENTICATED) {
		connect();	
		/*succesful authentication will send us here again*/
		return;
	}
	s = ui_get_status();
	status_msg = ui_get_status_msg();
	p = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_PRESENCE,
									(s == STATUS_OFFLINE)
										? LM_MESSAGE_SUB_TYPE_UNAVAILABLE
										: LM_MESSAGE_SUB_TYPE_AVAILABLE);
	lm_message_node_add_child(p->node, "priority", conf_priority);
	status = xmpp_status_to_str(s);
	if(status) {
		lm_message_node_add_child(p->node, "show", status);
	}
	if(strcmp(status_msg, "") != 0)
		lm_message_node_add_child(p->node, "status", status_msg);
	if(!lm_connection_send(connection, p, &err)) {
		ui_status_print("Error sending presence: %s\n", err->message);
		g_error_free(err);
	}
	lm_message_unref(p);
	if(s == STATUS_OFFLINE)
		disconnect();
} /* xmpp_set_status */

static char *
xmpp_status_to_str(XmppStatus status)
{
	if(status == STATUS_FFC) return "chat";
	else if(status == STATUS_AWAY) return "away";
	else if(status == STATUS_XA) return "xa";
	else if(status == STATUS_DND) return "dnd";
	else return NULL;
} /* xmpp_status_to_str */

static char *
xmpp_status_readable(XmppStatus st)
{
	if(st == STATUS_ONLINE) return "online";
	else if(st == STATUS_FFC) return "free for chat";
	else if(st == STATUS_AWAY) return "away";
	else if(st == STATUS_XA) return "not available";
	else if(st == STATUS_DND) return "do not disturb";
	else if(st == STATUS_OFFLINE) return "offline";
	else return "...what the fuck?";
}
