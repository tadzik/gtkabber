#ifndef COMMANDS_H
#define COMMANDS_H

void config_cleanup(void);
void config_parse_rcfile(void);
gint commands_exec(const char *);
#endif
