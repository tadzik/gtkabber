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
static gint set(const char *);
static void set_status(const char *);
/*************/

/* global vars */
gchar *conf_passwd = NULL;
gint conf_port = 0;
gchar *conf_priority = NULL; /*it's char, for we are passing it as char in lm_ functions*/
gchar *conf_res= NULL;
gchar *conf_server = NULL;
gchar *conf_username = NULL;
/***************/

void
config_cleanup(void)
{
	/* This one is called when the program quits, and cleans all the memory
	 * allocated here, which is configuration variables */
	g_free(conf_passwd);
	g_free(conf_priority);
	g_free(conf_res);
	g_free(conf_server);
	g_free(conf_username);
}

int
commands_exec(const char *command)
{
	/* Here we decide what to do with the given command and send it
	 * to appropirate internal function */
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
				/* We can maybe don't care about what failed.
				 * TODO: think about it later */
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
		g_free(conf_server);
		conf_server = g_strdup(arg);
	} else if(strstr(params, "username") == params) {
		g_free(conf_username);
		conf_username = g_strdup(arg);
	} else if(strstr(params, "passwd") == params) {
		g_free(conf_passwd);
		conf_passwd = g_strdup(arg);
	} else if(strstr(params, "resource") == params) {
		g_free(conf_res);
		conf_res = g_strdup(arg);
	} else if(strstr(params, "priority") == params) {
		g_free(conf_priority);
		conf_priority = g_strdup(arg);
	} else if(strstr(params, "port") == params) {
		conf_port = atoi(arg);
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
