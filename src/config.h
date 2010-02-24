#ifndef CONFIG_H 
#define CONFIG_H
#include "types.h"

void config_cleanup(void);
void config_init(void);
void config_reload(void);
Option get_settings(Settings);
const char *action_get(int);
void lua_msg_callback(const gchar *, const gchar *);
void lua_post_connect(void);
void lua_pres_callback(const gchar *, const gchar *, const gchar *);
#endif
