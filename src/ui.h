#ifndef UI_H
#define UI_H
#include "types.h"

Chattab *ui_create_tab(const gchar *, const gchar *, gint);
XmppStatus ui_get_status(void);
const gchar *ui_get_status_msg(void);
void ui_setup(int *, char ***);
void ui_set_status(XmppStatus);
void ui_set_status_msg(const gchar *);
void ui_show_presence_query(const gchar *);
void ui_show_subscribe_query(void);
void ui_print(const gchar *, ...);
void ui_tab_print_message(const gchar *, const gchar *);
#endif
