#include <loudmouth/loudmouth.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"
#include "ui.h"
#include "ui_roster.h"
#include "xmpp_roster.h"
#include "types.h"

/* functions */
static void connection_auth_cb(LmConnection *, gboolean, gpointer);
static void connection_disconnect_cb(LmConnection *, LmDisconnectReason,
                                     gpointer);
static void connection_open_cb(LmConnection *, gboolean, gpointer);
void xmpp_cleanup(void);
static void disconnect(void);
static void connect(void);
void xmpp_init(void);
LmHandlerResult xmpp_iq_handler(LmMessageHandler *, LmConnection *,
                                LmMessage *, gpointer);
LmHandlerResult xmpp_mesg_handler(LmMessageHandler *, LmConnection *,
                                  LmMessage *, gpointer);
LmHandlerResult xmpp_pres_handler(LmMessageHandler *, LmConnection *,
                                  LmMessage *, gpointer);
void xmpp_set_status(XmppStatus);
void xmpp_send_message(const char *, const char *);
static LmSSLResponse ssl_cb(LmSSL *, LmSSLStatus, gpointer);
static char *xmpp_status_to_str(XmppStatus);
static char *xmpp_status_readable(XmppStatus);
void xmpp_roster_parsed_cb(void);
/*************/

/* global variables */
LmConnection *connection;
static int initial_presence_sent = 0;
/********************/

static void
connect() {
	GError *err = NULL;
	gchar *conf_server;
	if(lm_connection_is_open(connection)) {
		ui_status_print("connect: connection alredy opened, aborting\n");
		return;	
	}
	conf_server = get_settings(SERVER).s;
	if(!conf_server) {
		ui_status_print("ERROR: Insufficient configuration, "
		                "connecting aborted (server not set)\n");
		return;
	} else {
		if(!lm_connection_get_server(connection))
			lm_connection_set_server(connection, conf_server);
	}
	if(!lm_connection_open(connection, (LmResultFunction)connection_open_cb,
	                       NULL, NULL, &err)) {
	    ui_status_print("Error opening connection: %s\n", err->message);
	    g_error_free(err);
	} else {
		ui_status_print("Connected to %s\n", conf_server);
	}
}

static void
connection_auth_cb(LmConnection *c, gboolean success, gpointer udata) {
	if(!success) {
		ui_status_print("ERROR: Authentication failed\n");
	} else {
		LmMessageHandler *handler;
		handler = lm_message_handler_new(xmpp_iq_handler, NULL, NULL);
		lm_connection_register_message_handler(c, handler,
		                                       LM_MESSAGE_TYPE_IQ,
		                                       LM_HANDLER_PRIORITY_NORMAL);
		lm_message_handler_unref(handler);
		handler = lm_message_handler_new(xmpp_mesg_handler, NULL, NULL);
		lm_connection_register_message_handler(c, handler,
		                                       LM_MESSAGE_TYPE_MESSAGE,
		                                       LM_HANDLER_PRIORITY_NORMAL);
		lm_message_handler_unref(handler);
		handler = lm_message_handler_new(xmpp_pres_handler, NULL, NULL);
		lm_connection_register_message_handler(c, handler,
		                                       LM_MESSAGE_TYPE_PRESENCE,
		                                       LM_HANDLER_PRIORITY_NORMAL);
		lm_message_handler_unref(handler);
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
	initial_presence_sent = 0;
}


static void
connection_open_cb(LmConnection *c, gboolean success, gpointer udata) {
	GError *err = NULL;
	gchar *conf_passwd = get_settings(PASSWD).s;
	gchar *conf_res = get_settings(RESOURCE).s;
	gchar *conf_username = get_settings(USERNAME).s;
	if(success) {
		if(!conf_username) {
			ui_status_print("ERROR: Insufficient configuration, "
			                "authentication aborted (username not set)\n");
			return;
		} else if(!conf_passwd) {
			ui_status_print("ERROR: Insufficient configuration, "
			                "authentication aborted (password not set)\n");
			return;
		}
		if(!lm_connection_authenticate(c, conf_username, conf_passwd,
		                               (conf_res) ? conf_res : "gtkabber",
		                               connection_auth_cb, NULL, NULL, &err)) {
			ui_status_print("Error authenticating: %s\n", err->message);
			g_error_free(err);
		} else {
			ui_status_print("Authenticated as %s\n", conf_username);
		}
	}
}

static void
disconnect() {
	LmMessage *m;
	gchar *conf_server = get_settings(SERVER).s;
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
		ui_roster_offline();
		g_printerr("Disconnected\n");
	}
}

static LmSSLResponse
ssl_cb(LmSSL *ssl, LmSSLStatus st, gpointer u)
{
	switch(st) {
	case LM_SSL_STATUS_NO_CERT_FOUND:
		ui_status_print("SSL: No certificate found!\n");
		break;
	case LM_SSL_STATUS_UNTRUSTED_CERT:
		ui_status_print("SSL: Certificate not trusted!\n");
		break;
	case LM_SSL_STATUS_CERT_EXPIRED:
		ui_status_print("SSL: Certificate has expired!\n");
		break;
	case LM_SSL_STATUS_CERT_NOT_ACTIVATED:
		ui_status_print("SSL: Certificate has not been activated!\n");
		break;
	case LM_SSL_STATUS_CERT_HOSTNAME_MISMATCH:
		ui_status_print("SSL: Certificate hostname does not match "
		                "expected hostname!\n");
		break;
	case LM_SSL_STATUS_CERT_FINGERPRINT_MISMATCH:
		/* we're not checking fingerprints anyway. TODO */
		break;
	case LM_SSL_STATUS_GENERIC_ERROR:
		ui_status_print("SSL: Generic error!\n");
		break;
	}
	/* TODO: we should return LM_SSL_RESPONSE_STOP if the user is paranoic */
	return LM_SSL_RESPONSE_CONTINUE;	
} /* ssl_cb */

void
xmpp_cleanup() {
	disconnect();
	if(connection) lm_connection_unref(connection);
	xmpp_roster_cleanup();
	config_cleanup();
}

void
xmpp_init(void) {
	LmSSL *ssl;
	int use_ssl, use_tls, port;
	char *jid;
	config_init();
	connection = lm_connection_new(NULL);
	use_ssl = get_settings(USE_SSL).i;
	use_tls = get_settings(USE_TLS).i;
	jid = get_settings(JID).s;
	port = get_settings(PORT).i;
	if(use_ssl || use_tls) {
		if(!lm_ssl_is_supported()) {
			ui_status_print("Error: SSL not available\n");
		} else if(use_ssl && use_tls) {
			ui_status_print("Error: You can't use ssl and tls at the same time");
		} else {
			ssl = lm_ssl_new(NULL, ssl_cb, NULL, NULL);
			lm_ssl_use_starttls(ssl, !use_ssl, use_tls);
			lm_connection_set_ssl(connection, ssl);
			lm_ssl_unref(ssl);
		}
	}
	if(!port) {
		port = (use_ssl) ? LM_CONNECTION_DEFAULT_PORT_SSL
		                 : LM_CONNECTION_DEFAULT_PORT;
	}
	lm_connection_set_port(connection, port);
	lm_connection_set_jid(connection, jid);
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
		if(g_strcmp0(lm_message_node_get_attribute(query, "xmlns"),
		             "jabber:iq:roster") == 0) {
			xmpp_roster_parse_query(c, query);
		}
	}
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

LmHandlerResult
xmpp_mesg_handler(LmMessageHandler *h, LmConnection *c, LmMessage *m,
                  gpointer udata)
{
	const char *from, *body;
	LmMessageNode *node;
	from = lm_message_node_get_attribute(m->node, "from");
	node = lm_message_node_get_child(m->node, "body");
	if(node && (body = lm_message_node_get_value(node)))
		ui_tab_print_message(from, body);
	/* we're actually ignoring <subject> and <thread> elements,
	 * as I've never actually seen them being used. If you do, and you care,
	 * feel obliged to mail me and yell at me */
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
	g_printerr("Parsing presence\n");
	buf = lm_message_node_get_attribute(m->node, "from");
	sep = strchr(buf, '/');
	g_printerr("%d...", __LINE__);
	if(sep) {
		jid = g_strndup(buf, sep-buf);
		resname = g_strdup(sep+1);
	} else {
		jid = g_strdup(buf);
		resname = g_strdup("default");
	}
	g_printerr("%d...", __LINE__);
	sb = xmpp_roster_find_by_jid(jid);
	if(!sb) {
		g_free(jid);
		g_free(resname);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	g_printerr("%d...", __LINE__);
	res = xmpp_roster_find_res_by_name(sb, resname);
	if(!res) {
		/* we have to create a new resource */	
		res = g_malloc(sizeof(Resource));
		res->name = (char *)resname;
		res->status_msg = NULL;
		xmpp_roster_add_resource(sb, res);
	}
	/* checking presence type (if available) */
	g_printerr("%d...", __LINE__);
	if((buf = lm_message_node_get_attribute(m->node, "type"))) {
		if(g_strcmp0(buf, "unavailable") == 0) {
			res->status = STATUS_OFFLINE;
		} else {
			/* other presence types are not yet implemented */
			return LM_HANDLER_RESULT_REMOVE_MESSAGE;
		}
	} else {
		res->status = STATUS_ONLINE;	
	}
	/* checking for some specific status */
	g_printerr("%d...", __LINE__);
	if((child = lm_message_node_get_child(m->node, "show"))) {
		buf = lm_message_node_get_value(child);
		if(!g_strcmp0(buf, "away")) res->status = STATUS_AWAY;
		else if(!g_strcmp0(buf, "chat")) res->status = STATUS_FFC;
		else if(!g_strcmp0(buf, "xa")) res->status = STATUS_XA;
		else if(!g_strcmp0(buf, "dnd")) res->status = STATUS_DND;
		/* any other <show> is forbidden in xmpp,
		 * so we're not gonna care about it anyway */
	}
	/* checking if resource has some previous status message.
	 * If it does, purge it */
	g_printerr("%d...", __LINE__);
	if(res->status_msg) {
		g_free(res->status_msg);
		res->status_msg = NULL;
	}
	/* checking for status message */
	g_printerr("%d...", __LINE__);
	child = lm_message_node_get_child(m->node, "status");
	if (child && (buf = lm_message_node_get_value(child)))
		res->status_msg = g_strdup(buf);
	/* checking priority (if provided) */
	g_printerr("%d...", __LINE__);
	child = lm_message_node_get_child(m->node, "priority");
	if(child && (buf = lm_message_node_get_value(child)))
		res->priority = atoi(buf);
	else
		res->priority = 0;
	/* printing a message to status window and updating roster entry in ui */
	ui_status_print("%s/%s is now %s (%s)\n", sb->name, resname,
	                xmpp_status_readable(res->status),
	                (res->status_msg) ? res->status_msg : "");
	ui_roster_update(sb->jid);
	g_printerr("Done\n");
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
xmpp_set_status(XmppStatus s)
{
	LmMessage *p;
	GError *err = NULL;
	const char *status, *status_msg;
	gchar *conf_priority = get_settings(PRIORITY).s;
	if(!connection || !lm_connection_is_open(connection)) {
		ui_status_print("Not connected, connecting\n");
		connect();
		return;
	}
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
	if(g_strcmp0(status_msg, "") != 0)
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
	else return "unknown";
} /* xmpp_status_readable */

void
xmpp_roster_parsed_cb(void)
{
	if(!initial_presence_sent)
		xmpp_set_status(ui_get_status());
}
