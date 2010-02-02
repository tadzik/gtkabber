#include "types.h"
#include "ui.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <string.h>
#include <stdlib.h>

/* Here we are doing command parsing, either these typed at runtime,
 * or these supplied in a gtkabberrc file. This is also the home of
 * some configuration variables, keeping connection parameters as well
 * as various things of which user can decide */

/* functions */
void config_cleanup(void);
gint commands_exec(const char *);
void config_parse_rcfile(void);
Option get_settings(Settings);
static void init_settings(void);
static gint set(const char *);
static void set_status(const char *);
/*************/

Option settings[NUM_SETTINGS];

void
config_cleanup(void)
{
	int i;
	/* cleaning strings in settings[] */
	for(i=0; i != USE_SSL; i++) {
		g_free(settings[i].s);
	}
}

int
commands_exec(const char *command)
{
	if(g_str_has_prefix(command, "set ")) {
		/*the only case so far: setting variables*/
		if(set(&command[4])) return 1;
	} else {
		/*unkown command*/
		return 2;
	}
	return 0;
} /* commands_exec */

void
config_parse_rcfile(void) 
{
	/* This one opens rc file, reads its contents and passes the lines
	 * to commands_exec() */	
	char *path;
	gchar *contents;
	gchar **lines;
	GError *err = NULL;
	init_settings();
	path = g_strdup_printf("%s/.config/gtkabberrc", g_getenv("HOME"));
	/* loading file contents */
	if(!g_file_get_contents(path, &contents, NULL, &err)) {
		ui_status_print("Error opening rc file: %s\n", err->message);
		g_error_free(err);
	} else {
		/* splitting file to lines */
		gint i;
		lines = g_strsplit(contents, "\n", 0);
		for(i=0; lines[i] != NULL; i++) {
			/* skipping empty lines and comments */
			if(lines[i][0] == '\0' || lines[i][0] == '#')
				continue;
			else {
				gint status = commands_exec(lines[i]);
				if(status == 1) ui_status_print("Line %d: parsing failed: "
				                                "%s\n", i+1, lines[i]);
				else if(status == 2) ui_status_print("Line %d: unknown command:"
				                                     " '%s'\n", i+1, lines[i]);
			}
		}
		g_strfreev(lines);
	}
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
