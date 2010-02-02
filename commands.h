#ifndef COMMANDS_H
#define COMMANDS_H
#include "types.h"

void config_cleanup(void);
void config_parse_rcfile(void);
gint commands_exec(const char *);
Option get_settings(Settings);
#endif
