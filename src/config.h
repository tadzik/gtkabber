#ifndef CONFIG_H 
#define CONFIG_H
#include "types.h"

void action_call(int);
const char *action_get(int);
void config_cleanup(void);
void config_init(void);
void config_reload(void);
Option get_settings(Settings);
void lua_msg_callback(const gchar *, const gchar *);
void lua_post_connect(void);
void lua_pres_callback(const gchar *, const gchar *, const gchar *);
#endif
