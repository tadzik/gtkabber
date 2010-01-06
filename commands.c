#include "ui.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <string.h>
#include <stdlib.h> /*for this damn atoi... come on!*/

/* functions */
int commands_exec(const char *);
int config_parse_rcfile(void);
static int set(const char *);
static void set_status(const char *);
/*************/

/* global vars */
char conf_passwd[32];
int conf_port = 0;
char conf_priority[3] = "0"; /*it's char, for we are passing it as char in lm_ functions*/
char conf_resource[32] = "gtkabber";
char conf_server[32];
char conf_username[32];
/***************/

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

int
commands_parse_rcfile(void) 
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
	return 0;
} /* commands_parse_rcfile */

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
	if(strstr(params, "server") == params) {
		strncpy(conf_server, arg, sizeof(conf_server));
	} else if(strstr(params, "username") == params) {
		strncpy(conf_username, arg, sizeof(conf_username));
	} else if(strstr(params, "passwd") == params) {
		strncpy(conf_passwd, arg, sizeof(conf_passwd));
	} else if(strstr(params, "resource") == params) {
		strncpy(conf_resource, arg, sizeof(conf_resource));
	} else if(strstr(params, "priority") == params) {
		strncpy(conf_priority, arg, sizeof(conf_priority));
	} else if(strstr(params, "port") == params) {
		char *port = index(params, ' ');
		if(port)
			conf_port = atoi(++port);
	} else if(strstr(params, "status ") == params) {
		char *status = index(params, ' ');
		if(status)
			set_status(++status);
	} else if(strstr(params, "status_msg") == params) {
		char *msg = index(params, ' ');
		if(msg)
			ui_set_status_msg(++msg);
	} else
		return 1;
	return 0;
} /* set */

static void
set_status(const char *s)
{
	if(strcmp(s, "online") == 0) ui_set_status(STATUS_ONLINE);
	else if(strcmp(s, "away") == 0) ui_set_status(STATUS_AWAY);
	else if(strcmp(s, "xa") == 0) ui_set_status(STATUS_XA);
	else if(strcmp(s, "ffc") == 0) ui_set_status(STATUS_FFC);
	else if(strcmp(s, "dnd") == 0) ui_set_status(STATUS_DND);
	else if(strcmp(s, "offline") == 0) ui_set_status(STATUS_OFFLINE);
} /* set_status */
