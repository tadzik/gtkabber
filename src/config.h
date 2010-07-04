#ifndef CONFIG_H 
#define CONFIG_H
#include "types.h"

void action_call(int);
const char *action_get(int);
void config_cleanup(void);
int config_init(void);
void config_reload(void);
int get_settings_int(Settings);
const char *get_settings_str(Settings);
void lua_msg_callback(const gchar *, const gchar *, const gchar *);
gchar *lua_msg_markup(const gchar *, const gchar *);
void lua_post_connect(void);
void lua_pres_callback(const gchar *, const gchar *, const gchar *);
#endif
