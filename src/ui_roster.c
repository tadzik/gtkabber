#include "common.h"
#include "ui.h"
#include "ui_roster.h"

enum {
    COL_STATUS = 0,
    COL_NAME,
    NUM_COLS
};

typedef struct {
    GtkTreeIter iter;
    const gchar *jid;
    const gchar *name;
} Ui_Buddy;

typedef struct {
    GtkTreeIter iter;
    gchar *name;
} Ui_Group;

static Ui_Roster *roster; // madness? THIS! IS! SINGLETON!

static gint compare_rows(GtkTreeModel *, GtkTreeIter *, GtkTreeIter *);
static gboolean filter_func(GtkTreeModel *, GtkTreeIter *, gpointer);
static GdkPixbuf *load_icon(const gchar *);
static gint pixbuf_priority(GdkPixbuf *);

static gint
compare_rows(GtkTreeModel *m, GtkTreeIter *a, GtkTreeIter *b)
{
    GdkPixbuf *p1, *p2;
    gtk_tree_model_get(m, a, COL_STATUS, &p1, -1);
    gtk_tree_model_get(m, b, COL_STATUS, &p2, -1);
    if (pixbuf_priority(p1) < pixbuf_priority(p2)) {
        return -1;
    } else if(pixbuf_priority(p1) > pixbuf_priority(p2)) {
        return 1;
    } else {
        gchar *n1, *n2;
        gint ret;
        gtk_tree_model_get(m, a, COL_NAME, &n1, -1);
        gtk_tree_model_get(m, b, COL_NAME, &n2, -1);
        ret = g_utf8_collate(n1, n2);
        g_free(n1);
        g_free(n2);
        return ret;
    }
} /* compare_rows */

static gboolean
filter_func(GtkTreeModel *m, GtkTreeIter *i, gpointer u)
{
    GdkPixbuf *p;
    if (roster->show_unavail)
        return 1;
    gtk_tree_model_get(m, i, COL_STATUS, &p, -1);
    if (p == roster->offline_icon)
        return 0;
    return 1;
    UNUSED(u);
}

static GdkPixbuf *
load_icon(const gchar *status)
{
    GdkPixbuf *new;
    GError *err = NULL;
    gchar *path;
    /* TODO: Here there should be some iconset choosing
     * (once I'll make it possible)*/
    path = g_strdup_printf("icons/%s.png", status);
    new = gdk_pixbuf_new_from_file(path, &err);
    if (new == NULL) {
        ui_print("Error loading iconset %s: %s\n", path, err->message);
        g_error_free(err);
    }
    g_free(path);
    return new;
} /* load_icon */

static gint
pixbuf_priority(GdkPixbuf *p)
{
    if (p == roster->ffc_icon)     return 0;
    if (p == roster->online_icon)  return 1;
    if (p == roster->away_icon)    return 2;
    if (p == roster->xa_icon)      return 3;
    if (p == roster->dnd_icon)     return 4;
    if (p == roster->offline_icon) return 5;
    return 6;
} /* pixbuf_priority */

void
ui_roster_add(Buddy *b)
{
    Ui_Buddy    *newguy;
    GtkTreeIter *iter, *parent;
    Ui_Group    *group;
    GSList      *elem;
    const gchar *g = b->group;
    if (g == NULL) {
        /* that looks a bit ugly, yet saves us some dynamic
         * memory allocation crap */
        char buf[] = "---";
        g = buf;
    }
    /* looking for his group */
    for(elem = roster->groups; elem; elem = elem->next) {
        group = (Ui_Group *)elem->data;
        if (g_strcmp0(group->name, g) == 0) break;
    }
    if (elem == NULL) {
        /* not found, we'll have to create one */
        group = g_malloc(sizeof(Ui_Group));
        group->name = g_strdup(g);
        gtk_tree_store_append(roster->store, &(roster->iter_top), NULL);
        gtk_tree_store_set(roster->store, &(roster->iter_top),
                           COL_NAME, g, -1);
        group->iter = roster->iter_top;
        roster->groups = g_slist_prepend(roster->groups, group);
    }
    iter = &(roster->iter_child);
    parent = &group->iter;

    gtk_tree_store_append(roster->store, iter, parent);
    gtk_tree_store_set(roster->store, iter, COL_STATUS, 
                       roster->offline_icon, COL_NAME, b->name, -1);
    newguy = g_malloc(sizeof(Ui_Buddy));
    newguy->jid = b->jid;
    newguy->name = b->name;
    newguy->iter = *iter;
    roster->entries = g_slist_prepend(roster->entries, newguy);
} /* ui_roster_add */

Ui_Roster *
ui_roster_new(void)
{
    Ui_Roster *new = roster = g_malloc(sizeof(Ui_Roster));
    GtkCellRenderer *txt_rend, *pix_rend;
    GtkTreeSortable *sort;
    GtkTreeSelection *selection;
    new->store = gtk_tree_store_new(
        NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING
    );
    new->filter = gtk_tree_model_filter_new(
        GTK_TREE_MODEL(new->store), NULL
    );
    gtk_tree_model_filter_set_visible_func(
        GTK_TREE_MODEL_FILTER(new->filter), filter_func, NULL, NULL
    );
    new->widget = gtk_tree_view_new_with_model(new->filter);
    g_object_unref(new->filter);

    txt_rend = gtk_cell_renderer_text_new();
    pix_rend = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(new->widget), -1, NULL,
        pix_rend, "pixbuf",
        COL_STATUS, NULL
    );
    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(new->widget), -1, NULL,
        txt_rend, "text",
        COL_NAME, NULL
    );
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(new->widget), FALSE);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(new->widget));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    new->online_icon = load_icon("online");
    new->offline_icon = load_icon("offline");
    new->ffc_icon = load_icon("ffc");
    new->away_icon = load_icon("away");
    new->dnd_icon = load_icon("dnd");
    new->xa_icon = load_icon("xa");
    sort = GTK_TREE_SORTABLE(new->store);
    gtk_tree_sortable_set_sort_func(
        sort, COL_STATUS,
        (GtkTreeIterCompareFunc)compare_rows,
        NULL, NULL
    );
    gtk_tree_sortable_set_sort_column_id(
        sort, COL_STATUS, GTK_SORT_ASCENDING
    );
    g_object_set(G_OBJECT(new->widget), "has-tooltip", TRUE, NULL);
    /*
    g_signal_connect(G_OBJECT(new->widget), "row-activated", 
                     G_CALLBACK(row_clicked_cb), NULL);
    g_signal_connect(G_OBJECT(new->widget), "query-tooltip",
                     G_CALLBACK(tooltip_cb), NULL);
    g_signal_connect(G_OBJECT(new->widget), "button-press-event",
                     G_CALLBACK(click_cb), NULL);
                     */
    new->show_unavail = 1; //FIXME
    return new;
} /* ui_roster_new */
