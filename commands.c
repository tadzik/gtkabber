#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ui.h" /*ui_status_print() for raising errors*/

/* functions */
int commands_exec(const char *);
int config_parse_rcfile(void);
static int set(const char *);
/*************/

/* global vars */
char conf_passwd[32];
int conf_port = 5222;
char conf_priority[3] = "0"; /*it's char, for we are passing it as char in lm_ functions*/
char conf_resource[16] = "timc";
char conf_server[20];
char conf_username[20];
/***************/

int
commands_exec(const char *command) {
	if(strncmp(command, "set ", 4) == 0) {
		/*the only case so far: setting variables*/
		if(set(&command[4])) return 1;
	} else {
		/*unkown command*/
		return 2;
	}
	return 0;
}

int
commands_parse_rcfile(void) {
	char buf[64];
	FILE *rcfile;
	snprintf(buf, sizeof(buf), "%s/.config/gtkabberrc", getenv("HOME"));
	rcfile = fopen(buf, "r");
	if(rcfile == NULL) {
		ui_status_print("ERROR: rc file could not be found!\n");
		return 1;
	}
	while(fgets(buf, sizeof(buf), rcfile)) {
		if(strncmp(buf, "#", 1) == 0) {
			continue;	
		} else {
			int status = commands_exec(buf);
			if(status == 1) ui_status_print("Command parsing failed\n");
			else if(status == 2) ui_status_print("Unknown command\n");
		}
	}
	return 0;
}

static int 
set(const char *params) {
	char arg1[16], arg2[32];
	if(sscanf(params, "%16s %32s\n", arg1, arg2) != 2) {
		return 1;
	} else {
		if(strcmp(arg1, "server") == 0) {
			strncpy(conf_server, arg2, sizeof(conf_server));
		} else if(strcmp(arg1, "username") == 0) {
			strncpy(conf_username, arg2, sizeof(conf_username));
		} else if(strcmp(arg1, "passwd") == 0) {
			strncpy(conf_passwd, arg2, sizeof(conf_passwd));
		} else if(strcmp(arg1, "resource") == 0) {
			strncpy(conf_resource, arg2, sizeof(conf_resource));
		} else if(strcmp(arg1, "priority") == 0) {
			strncpy(conf_priority, arg2, sizeof(conf_priority));
		} else if(strcmp(arg1, "port") == 0) {
			conf_port = atoi(arg2);
		}
	}
	return 0;
}
