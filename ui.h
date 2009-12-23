#ifndef UI_H
#define UI_H
#include "types.h"

void ui_create_tab(Chattab *);
XmppStatus ui_get_status(void);
const char *ui_get_status_msg(void);
void ui_setup(int *, char ***);
void ui_set_status(XmppStatus);
void ui_set_status_msg(const char *);
void ui_status_print(const char *, ...);
void ui_tab_print_message(const char *, const char *);
#endif
