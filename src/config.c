#include "config.h"

#include "xmpp.h"
#include "ui.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* Here we are doing command parsing, either these typed at runtime,
 * or these supplied in a gtkabberrc file. This is also the home of
 * some configuration variables, keeping connection parameters as well
 * as various things of which user can decide */

/* functions */
static int fun_print(lua_State *);
static int fun_sendmsg(lua_State *);
static int fun_sendstatus(lua_State *);
static int getbool(const gchar *);
static int getint(const gchar *);
static gchar *getstr(const gchar *);
static void loadactions(void);
static void loadfile(GError **);
static void loadlib(void);
/*************/

/* vars */
lua_State *lua;
char *tofree = NULL;
GArray *actions;
/********/

void action_call(int i)
{
	lua_settop(lua, 0);
	lua_getglobal(lua, "actions");
	if (!lua_istable(lua, -1)) {
		ui_print("actions not a table!\n");
		lua_settop(lua, 0);
		return;
	}
	/* fetching actions[i] */
	lua_pushinteger(lua, i+1);
	lua_gettable(lua, -2);
	if (lua_isnil(lua, -1)) {
		ui_print("actions[%d] nil!\n", i+1);
		lua_settop(lua, 0);
		return;
	}
	if (!lua_istable(lua, -1)) {
		ui_print("actions[%d] not a table!\n", i+1);
		lua_settop(lua, 0);
		return;
	}
	/* fetching actions[i].action */
	lua_pushstring(lua, "action");
	lua_gettable(lua, -2);
	if (lua_pcall(lua, 0, 0, 0)) {
		ui_print("lua: error running action: %s\n",
		                lua_tostring(lua, -1));
	}
	lua_settop(lua, 0);
} /* action_call */

const char *
action_get(int n)
{
	if (n >= (int)actions->len) return NULL;
	return g_array_index(actions, gchar *, n);
} /* action_get */

void
config_cleanup(void)
{
	int i;
	lua_close(lua);
	g_free(tofree); tofree = NULL;
	for (i = 0; i < (int)actions->len; i++) {
		g_free(g_array_index(actions, gchar *, i));
	}
	g_array_free(actions, TRUE);
}

int
config_init(void) 
{
	GError *err = NULL;
	loadfile(&err);
	if (err) {
		g_printerr("%s\n", err->message);
		g_error_free(err);
		return 1;
	}
	loadlib();
	actions = g_array_new(FALSE, FALSE, sizeof(gchar *));
	loadactions();
	return 0;
} /* config_parse_rcfile */

void
config_reload(void)
{
	config_cleanup();
	config_init();
} /* config_reload */

static int
fun_print(lua_State *l)
{
	const gchar *txt;
	txt = lua_tostring(l, 1);
	if (txt)
		ui_print(txt);
	return 0;
} /* fun_print */

static int
fun_sendmsg(lua_State *l)
{
	const gchar *to, *body;
	to = lua_tostring(l, 1);
	body = lua_tostring(l, 2);
	xmpp_send_message(to, body);
	return 0;
} /* fun_sendmsg */

static int
fun_sendstatus(lua_State *l)
{
	const gchar *to, *type, *msg;
	XmppStatus st;
	to = lua_tostring(l, 1);
	type = lua_tostring(l, 2);
	msg = lua_tostring(l, 3);
	if (type[0] == 'o') st = STATUS_ONLINE;
	else if (type[0] == 'f') st = STATUS_FFC;
	else if (type[0] == 'a') st = STATUS_AWAY;
	else if (type[0] == 'x') st = STATUS_XA;
	else if (type[0] == 'd') st = STATUS_DND;
	else st = STATUS_OFFLINE;
	ui_print("Sending status '%s' to %s\n", xmpp_status_readable(st),
		 (to) ? to : "the server");
	xmpp_send_status(to, st, msg);
	return 0;
} /* fun_sendstatus */

static int
getbool(const gchar *o)
{
	/* getbool returns -1 is the option is not set */
	int ret;
	lua_getglobal(lua, o);
	if (lua_isboolean(lua, 1) == 0)
		ret = -1;
	else
		ret = lua_toboolean(lua, 1);
	lua_pop(lua, 1);
	return ret;
} /* getbool */

static int
getint(const gchar *o)
{
	int ret;
	lua_getglobal(lua, o);
	ret = (int)lua_tonumber(lua, 1);
	lua_pop(lua, 1);
	return ret;
} /* getint */

static gchar *
getstr(const gchar *o)
{
	gchar *ret;
	lua_getglobal(lua, o);
	ret = g_strdup(lua_tostring(lua, 1));
	lua_pop(lua, 1);
	return ret;
} /* getstr */

int
get_settings_int(Settings s)
{
	int tmp;
	switch (s) {
	case USE_TLS:
		tmp = getbool("use_tls");
		return (tmp == -1) ? 0 : tmp;
	case USE_SSL:
		tmp = getbool("use_ssl");
		return (tmp == -1) ? !get_settings_int(USE_TLS) : tmp;
	case CASESENSORT:
		tmp = getbool("case_sensitive_sorting");
		return (tmp == -1) ? 0 : tmp;
	case PORT:
		tmp = getint("port");
		return (tmp == -1) ? 0 : tmp;
	case PRIORITY:
		tmp = getint("priority");
		return (tmp == -1) ? 10 : tmp;
	default:
		return -1;
	}
} /* get_settings_int */

const char *
get_settings_str(Settings s)
{
	char *ptr, *atpos;
	switch (s) {
	case SERVER:
		return getstr("server");
	case JID:
		return getstr("jid");
	case PASSWD:
		return getstr("passwd");
	case RESOURCE:
		ptr = getstr("resource");
		return (ptr) ? ptr : "gtkabber";
	case USERNAME:
		ptr = getstr("jid");
		atpos = strchr(ptr, '@');
		if (atpos)
			return tofree = g_strndup(ptr, atpos - ptr);
	default:
		return NULL;
	}
} /* get_settings */

static void
loadactions(void)
{
	int i;
	lua_settop(lua, 0);
	lua_getglobal(lua, "actions");
	if (lua_istable(lua, -1) == 0) {
		lua_settop(lua, 0);
		return;
	}
	for (i = 1; ; i++) {
		gchar *name;
		lua_settop(lua, 1);
		/* fetching actions[i] */
		lua_pushinteger(lua, i);
		lua_gettable(lua, -2);
		if (lua_isnil(lua, -1)) {
			break;
		}
		if (lua_istable(lua, -1) == 0) {
			ui_print("Lua error: actions[%d] not an array\n", i);
			continue;
		}
		/* fetching actions[i].name */
		lua_pushstring(lua, "name");
		lua_gettable(lua, -2);
		if (lua_isstring(lua, -1) == 0) {
			ui_print("Lua error: actions[%d].name not a string\n", i);
			continue;
		}
		name = g_strdup(lua_tostring(lua, -1));
		g_array_append_val(actions, name);
	}
	lua_settop(lua, 0); /* popping out "actions" array */
} /* loadactions */

static void
loadfile(GError **e)
{
	char *path;
	lua = lua_open();
	luaL_openlibs(lua);
	path = g_strdup_printf("%s/.config/gtkabber.lua", g_getenv("HOME"));
	/* load the file and run it */
	if (luaL_loadfile(lua, path) || lua_pcall(lua, 0, 0, 0)) {
		*e = g_error_new(1, 1,
				"Couldn't parse the configuration file %s: %s",
		                path, lua_tostring(lua, -1));
		lua_pop(lua, 1);
	}
	g_free(path);
} /* loadfile */

static void
loadlib(void)
{
	/* here we create a table with C functions (pseudo-object)
	 * then we set it global, so one can use it in lua scripts
	 * like `gtkabber.sendmsg(blah, blah)` */
	lua_newtable(lua);

	lua_pushstring(lua, "sendmsg");
	lua_pushcfunction(lua, fun_sendmsg);
	lua_settable(lua, -3);

	lua_pushstring(lua, "sendstatus");
	lua_pushcfunction(lua, fun_sendstatus);
	lua_settable(lua, -3);

	lua_pushstring(lua, "print");
	lua_pushcfunction(lua, fun_print);
	lua_settable(lua, -3);
	/* setting our "library" global */
	lua_setglobal(lua, "gtkabber");
} /* loadlib */

void
lua_msg_callback(const gchar *f, const gchar *t, const gchar *m)
{
	if (f == NULL) f = get_settings_str(JID);
	lua_getglobal(lua, "message_cb");
	if (lua_isnil(lua, 1)) {
		lua_pop(lua, 1);
		return;
	}
	if (lua_isfunction(lua, 1) == 0) {
		ui_print("lua error: 'message_cb' is not a function!\n");
		lua_pop(lua, 1);
		return;
	}
	lua_pushstring(lua, f);
	lua_pushstring(lua, t);
	lua_pushstring(lua, m);
	if(lua_pcall(lua, 3, 0, 0)) {
		ui_print("lua: error running message_cb: %s\n",
		                lua_tostring(lua, -1));
		lua_pop(lua, 1);
	}
} /* lua_msg_callback */

gchar *
lua_msg_markup(const gchar *n, const gchar *t)
{
	gchar *ret;
	lua_getglobal(lua, "message_markup");
	if (lua_isnil(lua, 1)) {
		lua_pop(lua, 1);
		return NULL;
	}
	if (lua_isfunction(lua, 1) == 0) {
		ui_print("lua error: 'message_markup' is not a function!\n");
		lua_pop(lua, 1);
		return NULL;
	}
	lua_pushstring(lua, n);
	lua_pushstring(lua, t);
	if(lua_pcall(lua, 2, 1, 0)) {
		ui_print("lua: error running message_cb: %s\n",
		                lua_tostring(lua, -1));
		ret = NULL;
	} else {
		ret = g_strdup(lua_tostring(lua, -1));
	}
	lua_pop(lua, 1);
	return ret;
} /* lua_msg_markup */

void
lua_post_connect(void)
{
	lua_getglobal(lua, "post_connect");
	if (lua_isnil(lua, 1)) {
		lua_pop(lua, 1);
		return;
	}
	if (lua_isfunction(lua, 1) == 0) {
		ui_print("lua error: 'post_connect' is not a function!\n");
		lua_pop(lua, 1);
		return;
	}
	if (lua_pcall(lua, 0, 0, 0)) {
		ui_print("lua: error running post_connect: %s\n",
		                lua_tostring(lua, -1));
		lua_pop(lua, 1);
	}

}

void
lua_pres_callback(const gchar *j, const gchar *s, const gchar *m)
{
	lua_getglobal(lua, "presence_cb");
	if (lua_isnil(lua, 1)) {
		lua_pop(lua, 1);
		return;
	}
	if (lua_isfunction(lua, 1) == 0) {
		ui_print("lua error: 'presence_cb' is not a function!\n");
		lua_pop(lua, 1);
		return;
	}
	lua_pushstring(lua, j);
	lua_pushstring(lua, s);
	lua_pushstring(lua, m);
	if (lua_pcall(lua, 3, 0, 0)) {
		ui_print("lua: error running message_cb: %s\n",
		                lua_tostring(lua, -1));
		lua_pop(lua, 1);
	}
}
