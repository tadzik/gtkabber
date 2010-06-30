#include "xmpp.h"

#include <loudmouth/loudmouth.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "ui.h"
#include "ui_roster.h"
#include "xmpp_roster.h"

/* functions */
static void connect(void);
static void connection_auth_cb(LmConnection *, gboolean, gpointer);
static void connection_disconnect_cb(LmConnection *, LmDisconnectReason,
                                     gpointer);
static void connection_open_cb(LmConnection *, gboolean, gpointer);
static void disconnect(void);
static LmHandlerResult iq_handler(LmMessageHandler *, LmConnection *,
                                  LmMessage *, gpointer);
static LmHandlerResult mesg_handler(LmMessageHandler *, LmConnection *,
                                    LmMessage *, gpointer);
static void parse_err_presence(LmMessage *);
static void parse_status_presence(LmMessage *);
static void parse_subscr_presence(LmMessage *);
static LmHandlerResult pres_handler(LmMessageHandler *, LmConnection *,
                                    LmMessage *, gpointer);
static gboolean reconnect(void);
static LmSSLResponse ssl_cb(LmSSL *, LmSSLStatus, gpointer);
static gchar *xmpp_status_to_str(XmppStatus);
/*************/

/* global variables */
static LmConnection *connection = NULL;
static int initial_presence_sent = 0;
static int wantconnection = 1;
/********************/

static void
connect() {
	GError *err = NULL;
	const gchar *conf_server;
	LmSSL *ssl;
	int use_ssl, use_tls, port;
	const gchar *jid;
	wantconnection = 1;
	if (lm_connection_is_open(connection)) {
		ui_print("connect: connection alredy opened, aborting\n");
		return;	
	}
	if (connection == NULL)
		connection = lm_connection_new(NULL);
	use_ssl = get_settings_int(USE_SSL);
	use_tls = get_settings_int(USE_TLS);
	jid = get_settings_str(JID);
	port = get_settings_int(PORT);
	if (use_ssl || use_tls) {
		if (!lm_ssl_is_supported()) {
			ui_print("Error: SSL not available\n");
		} else if (use_ssl && use_tls) {
			ui_print("Error: You can't use ssl and tls at the same time");
		} else {
			ssl = lm_ssl_new(NULL, ssl_cb, NULL, NULL);
			lm_ssl_use_starttls(ssl, !use_ssl, use_tls);
			lm_connection_set_ssl(connection, ssl);
			lm_ssl_unref(ssl);
		}
	}
	if (!port) {
		port = (use_ssl) ? LM_CONNECTION_DEFAULT_PORT_SSL
		                 : LM_CONNECTION_DEFAULT_PORT;
	}
	lm_connection_set_port(connection, port);
	lm_connection_set_jid(connection, jid);
	lm_connection_set_keep_alive_rate(connection, 30);
	lm_connection_set_disconnect_function(connection,
					      connection_disconnect_cb,
	                                      NULL, NULL);

	conf_server = get_settings_str(SERVER);
	if (!conf_server) {
		ui_print("ERROR: Insufficient configuration, "
		         "connecting aborted (server not set)\n");
		return;
	} else {
		if(!lm_connection_get_server(connection))
			lm_connection_set_server(connection, conf_server);
	}
	ui_print("Opening connection to %s\n", conf_server);
	if(!lm_connection_open(connection, (LmResultFunction)connection_open_cb,
	                       NULL, NULL, &err)) {
	    ui_print("Error opening connection: %s\n", err->message);
	    g_error_free(err);
	}
} /* connect */

static void
connection_auth_cb(LmConnection *c, gboolean success, gpointer udata) {
	if(!success) {
		ui_print("ERROR: Authentication failed\n");
	} else {
		LmMessageHandler *handler;
		handler = lm_message_handler_new(iq_handler, NULL, NULL);
		lm_connection_register_message_handler(c, handler,
		                                       LM_MESSAGE_TYPE_IQ,
		                                       LM_HANDLER_PRIORITY_NORMAL);
		lm_message_handler_unref(handler);
		handler = lm_message_handler_new(mesg_handler, NULL, NULL);
		lm_connection_register_message_handler(c, handler,
		                                       LM_MESSAGE_TYPE_MESSAGE,
		                                       LM_HANDLER_PRIORITY_NORMAL);
		lm_message_handler_unref(handler);
		handler = lm_message_handler_new(pres_handler, NULL, NULL);
		lm_connection_register_message_handler(c, handler,
		                                       LM_MESSAGE_TYPE_PRESENCE,
		                                       LM_HANDLER_PRIORITY_NORMAL);
		lm_message_handler_unref(handler);
		xmpp_roster_request(c);	
		g_timeout_add(60000, (GSourceFunc)reconnect, NULL);
	}
	lua_post_connect();
	UNUSED(udata);
} /* connection_auth_cb */

static void
connection_disconnect_cb(LmConnection *c, LmDisconnectReason reason,
                         gpointer udata) {
	switch(reason) {
	case LM_DISCONNECT_REASON_OK:
		ui_print("Connection closed as requested by user\n");
		break;
	case LM_DISCONNECT_REASON_PING_TIME_OUT:
		ui_print("Connection closed due to ping timeout\n");
		break;
	case LM_DISCONNECT_REASON_HUP:
		ui_print("Connection closed due to socket hungup\n");
		break;
	case LM_DISCONNECT_REASON_ERROR:
		ui_print("Connection closed due to error in transport layer\n");
		break;
	case LM_DISCONNECT_REASON_RESOURCE_CONFLICT:
		ui_print("Connection closed due to conflicting resources\n");
		break;
	case LM_DISCONNECT_REASON_INVALID_XML:
		ui_print("Connection closed due to invalid XML stream\n");
		break;
	default:
		ui_print("Connection closed due to unknown error\n");
	}
	initial_presence_sent = 0;
	ui_roster_offline();
	UNUSED(c);
	UNUSED(udata);
} /* connection_disconnect_cb */

static void
connection_open_cb(LmConnection *c, gboolean success, gpointer udata) {
	GError *err = NULL;
	const gchar *conf_passwd = get_settings_str(PASSWD);
	const gchar *conf_res = get_settings_str(RESOURCE);
	const gchar *conf_username = get_settings_str(USERNAME);
	if(success) {
		ui_print("Connection established\n");
		if(!conf_username) {
			ui_print("ERROR: Insufficient configuration, "
			                "authentication aborted (username not set)\n");
			return;
		} else if(!conf_passwd) {
			ui_print("ERROR: Insufficient configuration, "
			                "authentication aborted (password not set)\n");
			return;
		}
		if(!lm_connection_authenticate(c, conf_username, conf_passwd,
		                               (conf_res) ? conf_res : "gtkabber",
		                               connection_auth_cb, NULL, NULL, &err)) {
			ui_print("Error authenticating: %s\n", err->message);
			g_error_free(err);
		} else {
			ui_print("Authenticated as %s\n", conf_username);
		}
	} else {
		ui_print("Could not connect to server\n");
	}
	UNUSED(udata);
} /* connection_open_cb */

static void
disconnect() {
	LmMessage *m;
	const gchar *conf_server = get_settings_str(SERVER);
	if (connection == NULL)
		return;
	if (lm_connection_get_state(connection)
	   == LM_CONNECTION_STATE_AUTHENTICATED) {
		m = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_PRESENCE,
		                                 LM_MESSAGE_SUB_TYPE_UNAVAILABLE);
		lm_connection_send(connection, m, NULL);
	}
	if(lm_connection_is_open(connection)) {
		ui_print("Closing connection to %s\n", conf_server);
		lm_connection_close(connection, NULL);
		g_printerr("Disconnected\n");
	}
	lm_connection_unref(connection);
	connection = NULL;
} /* disconnect */

static LmHandlerResult
iq_handler(LmMessageHandler *h, LmConnection *c, LmMessage *m,
           gpointer userdata) {
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

static LmHandlerResult
mesg_handler(LmMessageHandler *h, LmConnection *c, LmMessage *m,
             gpointer udata)
{
	const char *from, *body;
	LmMessageNode *node;
	from = lm_message_node_get_attribute(m->node, "from");
	node = lm_message_node_get_child(m->node, "body");
	if(node && (body = lm_message_node_get_value(node))) {
		ui_tab_print_message(from, body);
		lua_msg_callback(from, body);
	}
	/* we're actually ignoring <subject> and <thread> elements,
	 * as I've never actually seen them being used. If you do, and you care,
	 * feel obliged to mail me and yell at me */
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	UNUSED(h);
	UNUSED(c);
	UNUSED(udata);
} /* xmpp_mesg_handler */

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

static LmHandlerResult
pres_handler(LmMessageHandler *h, LmConnection *c, LmMessage *m,
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

static gboolean
reconnect(void)
{
	if(wantconnection && lm_connection_get_state(connection)
	                      == LM_CONNECTION_STATE_CLOSED) {
		connect();
	}
	return TRUE;
} /* reconnect */

static LmSSLResponse
ssl_cb(LmSSL *ssl, LmSSLStatus st, gpointer u)
{
	switch(st) {
	case LM_SSL_STATUS_NO_CERT_FOUND:
		ui_print("SSL: No certificate found!\n");
		break;
	case LM_SSL_STATUS_UNTRUSTED_CERT:
		ui_print("SSL: Certificate not trusted!\n");
		break;
	case LM_SSL_STATUS_CERT_EXPIRED:
		ui_print("SSL: Certificate has expired!\n");
		break;
	case LM_SSL_STATUS_CERT_NOT_ACTIVATED:
		ui_print("SSL: Certificate has not been activated!\n");
		break;
	case LM_SSL_STATUS_CERT_HOSTNAME_MISMATCH:
		ui_print("SSL: Certificate hostname does not match "
		                "expected hostname!\n");
		break;
	case LM_SSL_STATUS_CERT_FINGERPRINT_MISMATCH:
		/* we're not checking fingerprints anyway. TODO */
		break;
	case LM_SSL_STATUS_GENERIC_ERROR:
		ui_print("SSL: Generic error!\n");
		break;
	}
	/* TODO: we should return LM_SSL_RESPONSE_STOP if the user is paranoic */
	return LM_SSL_RESPONSE_CONTINUE;	
	UNUSED(ssl);
	UNUSED(u);
} /* ssl_cb */

void
xmpp_cleanup() {
	disconnect();
	if (connection) lm_connection_unref(connection);
	xmpp_roster_cleanup();
	config_cleanup();
} /* xmpp_cleanup */

void
xmpp_init(void) {
	/* TODO: Maybe we can just get rid of this? */
	connect();
} /* xmpp_init */

void
xmpp_send_message(const char *to, const char *msg)
{
	LmMessage *m;
	GError *err = NULL;
	m = lm_message_new_with_sub_type(to, LM_MESSAGE_TYPE_MESSAGE,
	                                 LM_MESSAGE_SUB_TYPE_CHAT);
	lm_message_node_add_child(m->node, "body", msg);
	if(!lm_connection_send(connection, m, &err)) {
		ui_print("Error sending message: %s\n", err->message);
		g_error_free(err);
	}
	lm_message_unref(m);
} /* xmpp_send_message */

void
xmpp_send_status(const gchar *to, XmppStatus s, const gchar *msg)
{
	LmMessage *p;
	GError *err = NULL;
	const gchar *status, *status_msg;
	gchar *conf_priority;
	if (s == STATUS_OFFLINE) {
		wantconnection = 0;
		disconnect();
		return;
	}
	if (!connection || !lm_connection_is_open(connection)) {
		ui_print("Not connected, connecting\n");
		connect();
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
	status = xmpp_status_to_str(s);
	if(status) {
		lm_message_node_add_child(p->node, "show", status);
	}
	if(g_strcmp0(status_msg, "") != 0)
		lm_message_node_add_child(p->node, "status", status_msg);
	if(!lm_connection_send(connection, p, &err)) {
		ui_print("Error sending presence: %s\n", err->message);
		g_error_free(err);
	}
	lm_message_unref(p);
} /* xmpp_send_status */

static char *
xmpp_status_to_str(XmppStatus status)
{
	if(status == STATUS_FFC) return "chat";
	else if(status == STATUS_AWAY) return "away";
	else if(status == STATUS_XA) return "xa";
	else if(status == STATUS_DND) return "dnd";
	else return NULL;
} /* xmpp_status_to_str */

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
		if(!lm_connection_send(connection, msg, &err)) {
			ui_print("Error adding buddy to roster: %s\n", err->message);
			g_error_free(err);
		}
		lm_message_unref(msg);
	}
	/* sending subscription request */	
	msg = lm_message_new_with_sub_type(j, LM_MESSAGE_TYPE_PRESENCE,
	                                   LM_MESSAGE_SUB_TYPE_SUBSCRIBE);
	if(!lm_connection_send(connection, msg, &err)) {
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
	if(!lm_connection_send(connection, msg, &err)) {
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

void
xmpp_roster_parsed_cb(void)
{
	if(!initial_presence_sent)
		xmpp_send_status(NULL, ui_get_status(), NULL);
} /* xmpp_roster_parsed_cb */
