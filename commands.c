#include "ui.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <string.h>
#include <stdlib.h> /*for this damn atoi... come on!*/

/* functions */
void config_cleanup(void);
int commands_exec(const char *);
void config_parse_rcfile(void);
static int set(const char *);
static void set_status(const char *);
/*************/

/* global vars */
gchar *conf_passwd = NULL;
int conf_port = 0;
gchar *conf_priority = NULL; /*it's char, for we are passing it as char in lm_ functions*/
gchar *conf_res= NULL;
gchar *conf_server = NULL;
gchar *conf_username = NULL;
/***************/

void
config_cleanup(void)
{
	/* g_free() is smart enough to return if the pointer is NULL */
	g_free(conf_passwd);
	g_free(conf_priority);
	g_free(conf_res);
	g_free(conf_server);
	g_free(conf_username);
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
	GString *path;
	gchar *contents;
	gchar **lines;
	GError *err = NULL;
	path = g_string_new(NULL);
	g_string_append_printf(path, "%s/.config/gtkabberrc", g_getenv("HOME"));
	/*loading file contents: gods bless glib*/
	if(!g_file_get_contents(path->str, &contents, NULL, &err)) {
		ui_status_print("Error opening rc file: %s\n", err->message);
		g_error_free(err);
	} else {
		/*splitting file to lines*/
		int i;
		lines = g_strsplit(contents, "\n", 0);
		for(i=0; lines[i] != NULL; i++) {
			if(lines[i][0] == '\0') continue;
			if(lines[i][0] == '#') {
				continue;	
			} else {
				int status = commands_exec(lines[i]);
				if(status == 1) ui_status_print("Line %d: parsing failed: "
				                                "%s\n", i+1, lines[i]);
				else if(status == 2) ui_status_print("Line %d: unknown command:"
				                                     " '%s'\n", i+1, lines[i]);
			}
		}
		g_strfreev(lines);
	}
	g_string_free(path, TRUE);
} /* config_parse_rcfile */

static int 
set(const char *params)
{
	/* return values:
	 * 1: unknown command
	 * 2: not enough parameters 
	 */
	char *arg;
	arg = index(params, ' ');
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
