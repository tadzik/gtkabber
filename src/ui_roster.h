#ifndef UI_ROSTER_H
#define UI_ROSTER_H

#include <gtk/gtk.h>
#include "xmpp_roster.h"

typedef struct {
    GtkTreeIter      iter_top, iter_child;
    GtkTreeModel     *filter;
    GtkTreeSelection *selection;
    GtkTreeStore     *store;
    GtkWidget        *widget;
    /* I feel like having entries in a hashtable. TODO? */
    GSList           *entries, *groups;
    gint             show_unavail;
    GdkPixbuf        *offline_icon,
                     *online_icon,
                     *ffc_icon,
                     *away_icon,
                     *xa_icon,
                     *dnd_icon;
} Ui_Roster;

Ui_Roster * ui_roster_new(void);
void ui_roster_add(Buddy *);

#endif
