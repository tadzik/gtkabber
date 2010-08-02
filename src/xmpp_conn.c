#include "xmpp_conn.h"
#include "ui.h"
#include "ui_roster.h"
#include "config.h"
#include "xmpp_iq.h"
#include "xmpp_mesg.h"
#include "xmpp_pres.h"
#include "xmpp_roster.h"

#include <loudmouth/loudmouth.h>

/************************/
void xmpp_connect(void);
static void connection_auth_cb(LmConnection *, gboolean, gpointer);
static void connection_disconnect_cb(LmConnection *, LmDisconnectReason,
                                     gpointer);
static void connection_open_cb(LmConnection *, gboolean, gpointer);
void xmpp_disconnect(void);
static gboolean reconnect(void);
static LmSSLResponse ssl_cb(LmSSL *, LmSSLStatus, gpointer);
/************************/

/* global variables */
static LmConnection *connection = NULL;
static int initial_presence_sent = 0;
static int wantconnection = 1;
/********************/

void
xmpp_connect() {
	GError *err = NULL;
	const gchar *conf_server;
	LmSSL *ssl;
	int use_ssl, use_tls, port;
	const gchar *jid;
	wantconnection = 1;
	if (connection && lm_connection_is_open(connection)) {
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
} /* xmpp_connect */

LmConnection *
xmpp_connection_get(void)
{
	return connection;
} /* xmpp_connection_get */

static void
connection_auth_cb(LmConnection *c, gboolean success, gpointer udata) {
	if(!success) {
		ui_print("ERROR: Authentication failed\n");
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

int
xmpp_connection_is_open(void)
{
	return connection && lm_connection_is_open(connection);
}

void
xmpp_disconnect() {
	LmMessage *m;
	const gchar *conf_server = get_settings_str(SERVER);
	wantconnection = 0;
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
} /* xmpp_disconnect */

static gboolean
reconnect(void)
{
	if(wantconnection && lm_connection_get_state(connection)
	                      == LM_CONNECTION_STATE_CLOSED) {
		xmpp_connect();
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
	xmpp_disconnect();
	if (connection) lm_connection_unref(connection);
	xmpp_roster_cleanup();
	config_cleanup();
} /* xmpp_cleanup */

void
xmpp_init(void) {
	/* TODO: Maybe we can just get rid of this? */
	xmpp_connect();
} /* xmpp_init */

void
xmpp_roster_parsed_cb(void)
{
	if(!initial_presence_sent)
		xmpp_send_status(NULL, ui_get_status(), NULL);
} /* xmpp_roster_parsed_cb */
