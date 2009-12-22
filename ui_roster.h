#ifndef UI_ROSTER_H
#define UI_ROSTER_H
#include "types.h"

void ui_roster_add(const char *, const char *);
void ui_roster_cleanup(void);
GtkWidget *ui_roster_setup(void);
void ui_roster_update(const char *, XmppStatus);

#endif
