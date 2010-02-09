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
void config_init(void);
void config_reload(void);
static int getbool(const gchar *);
static int getint(const gchar *);
static gchar *getstr(const gchar *);
Option get_settings(Settings);
static void init_settings(void);
static void loadfile(void);
void lua_msg_callback(const gchar *, const gchar *);
void lua_pres_callback(const gchar *, const gchar *, const gchar *);
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

void
config_init(void) 
{
	/* This one opens rc file, reads its contents and passes the lines
	 * to commands_exec() */	
	init_settings();
	loadfile();
} /* config_parse_rcfile */

void config_reload(void)
{
	lua_close(lua);
	loadfile();
} /* config_reload */

static int
getbool(const gchar *o)
{
	/* getbool returns -1 is the option is not set */
	int ret;
	lua_getglobal(lua, o);
	if(!lua_isboolean(lua, 1))
		ret = -1;
	else
		ret = lua_toboolean(lua, 1);
	lua_remove(lua, 1);
	return ret;
} /* getbool */

static int
getint(const gchar *o)
{
	int ret;
	lua_getglobal(lua, o);
	ret = (int)lua_tonumber(lua, 1);
	lua_remove(lua, 1);
	return ret;
} /* getint */

static gchar *
getstr(const gchar *o)
{
	gchar *ret;
	lua_getglobal(lua, o);
	ret = g_strdup(lua_tostring(lua, 1));
	lua_remove(lua, 1);
	return ret;
} /* getstr */

Option
get_settings(Settings s)
{
	return settings[s];
} /* get_settings */

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

static void
loadfile(void)
{
	char *atpos, *path;
	lua = lua_open();
	luaL_openlibs(lua);
	path = g_strdup_printf("%s/.config/gtkabber.lua", g_getenv("HOME"));
	/* load the file and run it */
	if(luaL_loadfile(lua, path) || lua_pcall(lua, 0, 0, 0)) {
		ui_status_print("Couldn't parse the configuration file %s: %s",
		                lua_tostring(lua, -1));
		return;
	}

	/* strings */
	settings[SERVER].s = getstr("server");
	settings[JID].s = getstr("jid");
	settings[PASSWD].s = getstr("passwd");
	if((settings[RESOURCE].s = getstr("resource")) == NULL)
		settings[RESOURCE].s = g_strdup("gtkabber");

	if((settings[USE_TLS].i = getbool("use_tls")) == -1)
		settings[USE_TLS].i = 0;

	if((settings[USE_SSL].i = getbool("use_ssl")) == -1)
		settings[USE_SSL].i = !settings[USE_TLS].i;

	if((settings[CASESENSORT].i = getbool("case_sensitive_sorting")) == -1)
		settings[CASESENSORT].i = 0;

	if((settings[PORT].i = getint("port")) == -1)
		/* so xmpp_connect will set the default port */
		settings[PORT].i = 0;	

	if((settings[PRIORITY].i = getint("priority")) == -1)
		/* high default priority, since it's unstable
		 * a lot of testing is needed :> */
		settings[PRIORITY].i = 10;

	/* extracting username from jid */
	if(settings[JID].s) {
		atpos = strchr(settings[JID].s, '@');
		if(atpos)
			settings[USERNAME].s = g_strndup(settings[JID].s,
	                                                 atpos - settings[JID].s);
	}
	g_free(path);
} /* loadfile */

void
lua_msg_callback(const gchar *j, const gchar *m)
{
	lua_getglobal(lua, "message_cb");
	if(lua_isnil(lua, 1))
		return;
	if(!lua_isfunction(lua, 1)) {
		ui_status_print("lua error: message_cb is not a function!\n");
		return;
	}
	lua_pushstring(lua, j);
	lua_pushstring(lua, m);
	if(lua_pcall(lua, 2, 0, 0)) {
		ui_status_print("lua: error running message_cb: %s\n",
		                lua_tostring(lua, -1));
		lua_pop(lua, -1);
	}
} /* lua_msg_callback */

void
lua_pres_callback(const gchar *j, const gchar *s, const gchar *m)
{
	lua_getglobal(lua, "presence_cb");
	if(lua_isnil(lua, 1))
		return;
	if(!lua_isfunction(lua, 1)) {
		ui_status_print("lua error: presence_cb is not a function!\n");
		return;
	}
	lua_pushstring(lua, j);
	lua_pushstring(lua, s);
	lua_pushstring(lua, m);
	if(lua_pcall(lua, 3, 0, 0)) {
		ui_status_print("lua: error running message_cb: %s\n",
		                lua_tostring(lua, -1));
		lua_pop(lua, -1);
	}
}
