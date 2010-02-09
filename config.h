#ifndef CONFIG_H 
#define CONFIG_H
#include "types.h"

void config_cleanup(void);
void config_init(void);
Option get_settings(Settings);
#endif
