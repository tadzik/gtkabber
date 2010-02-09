#ifndef CONFIG_H 
#define CONFIG_H
#include "types.h"

void config_cleanup(void);
void config_init(void);
void config_reload(void);
Option get_settings(Settings);
void lua_msg_callback(const gchar *, const gchar *);
void lua_pres_callback(const gchar *, const gchar *, const gchar *);
#endif
