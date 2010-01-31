#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "xmpp_roster.h"
#include "types.h"

/* Here goes everything related to roster widget in the user interface
 * It's alredy complicated enough to keep it in the separate file */

enum {
	COL_STATUS = 0,
	COL_NAME,
	NUM_COLS
};

/* vars */
static GtkTreeIter main_iter;
static GtkTreeModel *filter;
static GtkTreeSelection *selection;
static GtkTreeStore *roster;
static GtkWidget *view;
static GSList *entries, *groups;
static gint show_unavail = 0;

static GdkPixbuf *offline_icon;
static GdkPixbuf *online_icon;
static GdkPixbuf *ffc_icon;
static GdkPixbuf *away_icon;
static GdkPixbuf *xa_icon;
static GdkPixbuf *dnd_icon;
/********/

/* functions*/
static gint compare_rows(GtkTreeModel *, GtkTreeIter *,
                         GtkTreeIter *, gpointer);
static gboolean filter_func(GtkTreeModel *, GtkTreeIter *, gpointer);
static gint get_pixbuf_priority(GdkPixbuf *);
static GdkPixbuf *load_icon(const gchar *);
static void load_iconset(void);
static gint match_entry_by_iter(gconstpointer, gconstpointer);
static void row_clicked_cb(GtkTreeView *, GtkTreePath *,
                           GtkTreeViewColumn *, gpointer);
void ui_roster_add(const gchar *, const gchar *, const gchar *);
void ui_roster_cleanup(void);
GtkWidget *ui_roster_setup(void);
void ui_roster_toggle_offline(void);
void ui_roster_update(const gchar *);
/************/

static gint
get_pixbuf_priority(GdkPixbuf *p)
{
	/* Called by compare_rows when sorting entries.
	 * Determines which row is "more important", so the "free for chat" guys
	 * are kept on top of the list, and so on and so forth */
	if(p == ffc_icon) return 0;
	if(p == online_icon) return 1;
	if(p == away_icon) return 2;
	if(p == xa_icon) return 3;
	if(p == dnd_icon) return 4;
	else return 5;
} /* get_pixbuf_priority */

static gint
compare_rows(GtkTreeModel *m, GtkTreeIter *a, GtkTreeIter *b, gpointer d)
{
	/* Sorting function set in ui_roster_setup(),
	 * gets automagically called when rows are added/modified
	 * Examines first by the icon (get_pixbuf_priority() decides),
	 * then compares the the names. */
	GdkPixbuf *p1, *p2;
	gtk_tree_model_get(m, a, COL_STATUS, &p1, -1);
	gtk_tree_model_get(m, b, COL_STATUS, &p2, -1);
	if(get_pixbuf_priority(p1) < get_pixbuf_priority(p2))
		return -1;
	else if(get_pixbuf_priority(p1) > get_pixbuf_priority(p2))
		return 1;
	else {
		gchar *n1, *n2;
		gint ret;
		gtk_tree_model_get(m, a, COL_NAME, &n1, -1);
		gtk_tree_model_get(m, b, COL_NAME, &n2, -1);
		ret = g_strcmp0(n1, n2);
		g_free(n1);
		g_free(n2);
		return ret;
	}
} /* compare_rows */

static gboolean
filter_func(GtkTreeModel *m, GtkTreeIter *i, gpointer u)
{
	GdkPixbuf *p;
	if(show_unavail) return 1;
	gtk_tree_model_get(m, i, COL_STATUS, &p, -1);
	if(p == offline_icon)
		return 0;
	return 1;
}

static GdkPixbuf *
load_icon(const gchar *status)
{
	/* This one loads an icon for the specified status.
	 * Data allocated here is freed by ui_roster_cleanup(),
	 * called by destroy() handler function (ui.c) */
	GdkPixbuf *new;
	GError *err = NULL;
	gchar *path;
	/* TODO: Here there should be some iconset choosing
	 * (once I'll make it possible :])*/
	path = g_strdup_printf("icons/%s.png", status);
	new = gdk_pixbuf_new_from_file(path, &err);
	if (new == NULL) {
		ui_status_print("Error loading iconset %s: %s\n", path, err->message);
		g_error_free(err);
	}
	g_free(path);
	return new;
} /* load_icon */

static void
load_iconset()
{
	/* Loading the iconset. It's called only once,
	 * maybe it should be inline or something? TODO */
	online_icon = load_icon("online");
	offline_icon = load_icon("offline");
	ffc_icon = load_icon("ffc");
	away_icon = load_icon("away");
	dnd_icon = load_icon("dnd");
	xa_icon = load_icon("xa");
} /* load_iconset */

static int
match_entry_by_iter(gconstpointer e, gconstpointer i)
{
	/* Called by row_clicked_cb callback, to determine
	 * which row have we actually clicked */
	gint ret;
	GtkTreeIter iter;
	GtkTreeModel *m = GTK_TREE_MODEL(roster);
	GtkTreePath *p1, *p2;
	UiBuddy *sb = (UiBuddy *)e;
	gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter),
	                                                 &iter, (GtkTreeIter *)i);
	p1 = gtk_tree_model_get_path(m, &(sb->iter));
	p2 = gtk_tree_model_get_path(m, &iter);
	ret = gtk_tree_path_compare(p1, p2);
	gtk_tree_path_free(p1);
	gtk_tree_path_free(p2);
	return ret;
} /* match_entry_by_iter */

static void
row_clicked_cb(GtkTreeView *t, GtkTreePath *p, GtkTreeViewColumn *c, gpointer d)
{
	/* Callback for each row's "activate" signal. Looks up tab in the list,
	 * then creates a new chattab for ui_create_tab()
	 * and calls it to initiate chat */
	GtkTreeIter iter;
	GSList *entry;
	UiBuddy *sb;
	gchar *jid;
	gtk_tree_model_get_iter(gtk_tree_view_get_model(t), &iter, p);
	entry = g_slist_find_custom(entries, (void *)&iter, match_entry_by_iter);
	if(entry) {
		const gchar *resname;
		sb = (UiBuddy *)entry->data;
		resname = xmpp_roster_get_best_resname(sb->jid);
		if(resname == NULL)
			jid = g_strdup(sb->jid);
		else
			jid = g_strdup_printf("%s/%s", sb->jid, resname);
		ui_create_tab(jid, sb->name);
		g_free(jid);
	}
} /* row_clicked_cb */

void
ui_roster_add(const gchar *j, const gchar *n, const gchar *g)
{
	/* Adding new buddy to our roster widget.
	 * Memory allocated here is freed by ui_roster_clenaup,
	 * called by destroy() signal handler in ui.c */
	UiBuddy *newguy;
	UiGroup *group;
	GSList *elem;
	GtkTreeIter *iter = NULL;
	/* checking if our buddy belongs to some group */
	if(g) {
		/* looking for his group */
		for(elem = groups; elem; elem = elem->next) {
			group = (UiGroup *)elem->data;
			if(g_strcmp0(group->name, g) == 0) break;
		}
		/* did we even find this group? */
		if(elem) {
			group = (UiGroup *)elem->data;
		} else {
			/* we'll have to create one */
			group = g_malloc(sizeof(UiGroup));
			group->name = g_strdup(g);
			gtk_tree_store_append(roster, &main_iter, NULL);
			gtk_tree_store_set(roster, &main_iter, COL_NAME, g, -1);
			group->iter = main_iter;
			groups = g_slist_prepend(groups, group);
		}
		iter = &group->iter;
	}
	gtk_tree_store_append(roster, &main_iter, iter);
	gtk_tree_store_set(roster, &main_iter, COL_STATUS, offline_icon,
	                   COL_NAME, n, -1);
	newguy = g_malloc(sizeof(UiBuddy));
	newguy->jid = j;
	newguy->name = n;
	newguy->iter = main_iter;
	entries = g_slist_prepend(entries, newguy);
} /* ui_roster_add */

void
ui_roster_cleanup(void)
{
	/* This function, called by destroy() in ui.c frees the allocated icons
	 * and the list containing rows data */
	GSList *elem;
	if(offline_icon)
		g_object_unref(G_OBJECT(offline_icon));
	if(online_icon)
		g_object_unref(G_OBJECT(online_icon));
	if(away_icon)
		g_object_unref(G_OBJECT(away_icon));
	if(dnd_icon)
		g_object_unref(G_OBJECT(dnd_icon));
	if(ffc_icon)
		g_object_unref(G_OBJECT(ffc_icon));
	if(xa_icon)
		g_object_unref(G_OBJECT(xa_icon));
	/* There's no memory allocated in the list besides the elements itself */
	g_slist_foreach(entries, (GFunc)g_free, NULL);
	g_slist_free(entries);
	/* pardon, I'm sick of this g_slist_foreach */
	for(elem = groups; elem; elem = elem->next) {
		UiGroup *g = elem->data;
		g_free(g->name);
	}
	g_slist_free(groups);
}

GtkWidget *
ui_roster_setup(void)
{
	/* This function, called in ui_setup() prepares
	 * our roster widget for later use */
	GtkCellRenderer *txt_rend, *pix_rend;
	GtkTreeSortable *sort;
	/* tree store */
	roster = gtk_tree_store_new(NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	/* filtering */
	filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(roster), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter),
	                                       filter_func, NULL, NULL);
	/* tree view */
	view = gtk_tree_view_new_with_model(filter);
	g_object_unref(filter);
	/* renderers */
	txt_rend = gtk_cell_renderer_text_new();
	pix_rend = gtk_cell_renderer_pixbuf_new();
	/* adding columns */
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
	                                            -1, NULL, pix_rend,
	                                            "pixbuf", COL_STATUS, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
	                                            -1, NULL, txt_rend,
	                                            "text", COL_NAME, NULL);
	/* we don't need headers */
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
	/* selection  */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	/* loading iconset */
	load_iconset();
	/* sorting setup */
	sort = GTK_TREE_SORTABLE(roster);
	gtk_tree_sortable_set_sort_func(sort, COL_STATUS, compare_rows,
	                                NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(sort, COL_STATUS, GTK_SORT_ASCENDING);
	/* signals */
	g_signal_connect(G_OBJECT(view), "row-activated", 
	                 G_CALLBACK(row_clicked_cb), NULL);
	/* returning GtkTreeView so ui_setup can put it somewhere in the window */
	return view;
} /* roster_setup */

void
ui_roster_toggle_offline(void)
{
	show_unavail = !show_unavail;
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(filter));
} /* ui_roster_toggle_offline */

void
ui_roster_update(const char *jid)
{
	/* This function updates the appropriate entry, changing its status icon.
	 * The entry is determined by the given jid, which is compared to the one
	 * kept in entries GSList */
	GSList *elem;
	UiBuddy *sb = NULL;
	Resource *res;
	GdkPixbuf *icon;
	/* looking up entry by jid */
	for(elem = entries; elem; elem = elem->next) {
		UiBuddy *b = (UiBuddy *)elem->data;
		if(g_strcmp0(jid, b->jid) == 0) {
			sb = b;
			break;
		}
	}
	if(!sb) {
		/* should never happen */
		g_printerr("ui_roster_update: cannot update "
		           "non-existing entry %s\n", jid);
		return;
	}
	/* looking for the best resource */
	res = xmpp_roster_find_res_by_name(xmpp_roster_find_by_jid(jid),
	                                   xmpp_roster_get_best_resname(jid));
	if(!res) {
		g_printerr("ui_roster_update: found no resource to update %s, "
		           "assuming contact is unavailable\n", jid);
		return;
	}
	/* changing the icon */
	if(res->status == STATUS_ONLINE) icon = online_icon;
	else if(res->status == STATUS_AWAY) icon = away_icon;
	else if(res->status == STATUS_FFC) icon = ffc_icon;
	else if(res->status == STATUS_XA) icon = xa_icon;
	else if(res->status == STATUS_DND) icon = dnd_icon;
	else if(res->status == STATUS_OFFLINE) icon = offline_icon;
	gtk_tree_store_set(roster, &(sb->iter), COL_STATUS, icon, -1);
	/* well, it's a bit ugly. TODO */
	gtk_tree_view_expand_all(GTK_TREE_VIEW(view));
} /* ui_roster_update */
