#include "types.h"
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
void config_cleanup(void);
gint commands_exec(const char *);
void config_init(void);
Option get_settings(Settings);
static void init_settings(void);
static gint set(const char *);
static void set_status(const char *);
/*************/

/* vars */
lua_State *lua;
Option settings[NUM_SETTINGS];
/********/

void
config_cleanup(void)
{
	int i;
	/* cleaning strings in settings[] */
	for(i=0; i != USE_SSL; i++) {
		g_free(settings[i].s);
	}
	lua_close(lua);
}

int
commands_exec(const char *command)
{
	if(g_str_has_prefix(command, "set ")) {
		/*the only case so far: setting variables*/
		if(set(&command[4])) return 1;
	} else if(g_str_has_prefix(command, "subscribe ")) {
		gchar *jid = g_strdup((&command[10]));
		xmpp_subscribe(jid);
		g_free(jid);
	} else {
		/*unkown command*/
		return 2;
	}
	return 0;
} /* commands_exec */

void
config_init(void) 
{
	/* This one opens rc file, reads its contents and passes the lines
	 * to commands_exec() */	
	char *atpos, *path;
	init_settings();
	lua = lua_open();
	luaL_openlibs(lua);
	path = g_strdup_printf("%s/.config/gtkabber.lua", g_getenv("HOME"));
	/* load the file and run it */
	if(luaL_loadfile(lua, path) || lua_pcall(lua, 0, 0, 0)) {
		ui_status_print("Couldn't parse the configuration file %s: %s",
		                lua_tostring(lua, -1));
		return;
	}
	/* TODO: It looks like a... nevermind, think about it */
	lua_getglobal(lua, "server");
	settings[SERVER].s = g_strdup(lua_tostring(lua, 1));
	lua_remove(lua, 1);

	lua_getglobal(lua, "jid");
	settings[JID].s = g_strdup(lua_tostring(lua, 1));
	lua_remove(lua, 1);
	
	lua_getglobal(lua, "passwd");
	settings[PASSWD].s = g_strdup(lua_tostring(lua, 1));
	lua_remove(lua, 1);

	lua_getglobal(lua, "resource");
	settings[RESOURCE].s = g_strdup(lua_tostring(lua, 1));
	lua_remove(lua, 1);

	lua_getglobal(lua, "use_ssl");
	settings[USE_SSL].i = lua_toboolean(lua, 1);
	lua_remove(lua, 1);

	lua_getglobal(lua, "use_tls");
	settings[USE_TLS].i = lua_toboolean(lua, 1);
	lua_remove(lua, 1);

	lua_getglobal(lua, "case_sensitive_sorting");
	settings[CASESENSORT].i = lua_toboolean(lua, 1);
	lua_remove(lua, 1);

	lua_getglobal(lua, "port");
	settings[PORT].i = (int)lua_tonumber(lua, 1);
	lua_remove(lua, 1);
	/* extracting username from jid */
	if(settings[JID].s) {
		atpos = strchr(settings[JID].s, '@');
		if(atpos)
			settings[USERNAME].s = g_strndup(settings[JID].s,
	                                                 atpos - settings[JID].s);
	}
	g_printerr("Sorting option: %d\n", settings[CASESENSORT].i);
	g_free(path);
} /* config_parse_rcfile */

Option
get_settings(Settings s)
{
	return settings[s];
}

static void
init_settings(void)
{
	/* strings go NULL, ints go 0 */
	int i;
	for(i = 0; i != USE_SSL; i++) {
		settings[i].s = NULL;
	}
	settings[RESOURCE].s = g_strdup("gtkabber");
	for(i = USE_SSL; i != NUM_SETTINGS; i++) {
		settings[i].i = 0;
	}
} /* init_settings */

static int 
set(const char *params)
{
	/* This endless chain of else-ifs looks for a var to set/func to use
	 * and does what it should */
	gchar *arg;
	arg = strchr(params, ' ');
	/* if not found, exiting. If found, incrementing */
	if(!arg++)
		return 2;
	/*
	 * line: set server jabber.org
	 *           ^      ^      
	 *         params  arg
	 */
	if(strstr(params, "server") == params) {
		g_free(settings[SERVER].s);
		settings[SERVER].s = g_strdup(arg);
	} else if(strstr(params, "username") == params) {
		g_free(settings[USERNAME].s);
		settings[USERNAME].s = g_strdup(arg);
	} else if(strstr(params, "passwd") == params) {
		g_free(settings[PASSWD].s);
		settings[PASSWD].s = g_strdup(arg);
	} else if(strstr(params, "resource") == params) {
		g_free(settings[RESOURCE].s);
		settings[RESOURCE].s = g_strdup(arg);
	} else if(strstr(params, "priority") == params) {
		g_free(settings[PRIORITY].s);
		settings[PRIORITY].s = g_strdup(arg);
	} else if(strstr(params, "port") == params) {
		settings[PORT].i = atoi(arg);
	} else if(strstr(params, "status ") == params) {
		set_status(arg);
	} else if(strstr(params, "status_msg") == params) {
		ui_set_status_msg(arg);
	} else return 1;
	return 0;
	/* return values:
	 * 1: unknown command
	 * 2: not enough parameters */
} /* set */

static void
set_status(const char *s)
{
	if(g_strcmp0(s, "online") == 0) ui_set_status(STATUS_ONLINE);
	else if(g_strcmp0(s, "away") == 0) ui_set_status(STATUS_AWAY);
	else if(g_strcmp0(s, "xa") == 0) ui_set_status(STATUS_XA);
	else if(g_strcmp0(s, "ffc") == 0) ui_set_status(STATUS_FFC);
	else if(g_strcmp0(s, "dnd") == 0) ui_set_status(STATUS_DND);
	else if(g_strcmp0(s, "offline") == 0) ui_set_status(STATUS_OFFLINE);
} /* set_status */
