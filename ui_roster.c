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
static GtkTreeSelection *selection;
static GtkTreeStore *roster;
static GSList *entries;

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
static gint get_pixbuf_priority(GdkPixbuf *);
static GdkPixbuf *load_icon(const gchar *);
static void load_iconset(void);
static gint match_entry_by_iter(gconstpointer, gconstpointer);
static gint match_entry_by_jid(gconstpointer, gconstpointer);
static void row_clicked_cb(GtkTreeView *, GtkTreePath *,
                           GtkTreeViewColumn *, gpointer);
void ui_roster_add(const gchar *, const gchar *);
void ui_roster_cleanup(void);
GtkWidget *ui_roster_setup(void);
void ui_roster_update(const gchar *, XmppStatus);
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
	/*TODO: Check if comparing GtkTreeIter.stamp would do the job*/
	gint ret;
	GtkTreePath *p1, *p2;
	GtkTreeModel *m = GTK_TREE_MODEL(roster);
	UiBuddy *sb = (UiBuddy *)e;
	p1 = gtk_tree_model_get_path(m, &(sb->iter));
	p2 = gtk_tree_model_get_path(m, (GtkTreeIter *)i);
	ret = gtk_tree_path_compare(p1, p2);
	gtk_tree_path_free(p1);
	gtk_tree_path_free(p2);
	return ret;
} /* match_entry_by_iter */

static int
match_entry_by_jid(gconstpointer e, gconstpointer j)
{
	/* Used as a comparing function in ui_roster_update(),
	 * not called directly, just a pointer passed to g_slist_find_custom */
	UiBuddy *sb = (UiBuddy *)e;
	return g_strcmp0(sb->jid, j);
} /* match_entry_by_jid */

static void
row_clicked_cb(GtkTreeView *t, GtkTreePath *p, GtkTreeViewColumn *c, gpointer d)
{
	/* Callback for each row's "activate" signal. Looks up tab in the list,
	 * then creates a new chattab for ui_create_tab()
	 * and calls it to initiate chat */
	GtkTreeIter iter;
	GSList *entry;
	UiBuddy *sb;
	gtk_tree_model_get_iter(gtk_tree_view_get_model(t), &iter, p);
	entry = g_slist_find_custom(entries, (void *)&iter, match_entry_by_iter);
	if(entry) {
		Chattab *tab;
		const gchar *resname;
		tab = g_malloc(sizeof(Chattab));
		sb = (UiBuddy *)entry->data;
		resname = xmpp_roster_get_best_resname(sb->jid);
		tab->jid = g_strdup_printf("%s/%s", sb->jid, resname);
		tab->title = g_strdup(sb->name);
		ui_create_tab(tab);
	}
} /* row_clicked_cb */

void
ui_roster_add(const gchar *jid, const gchar *nick)
{
	/* Adding new buddy to our roster widget.
	 * Memory allocated here is freed by ui_roster_clenaup,
	 * called by destroy() signal handler in ui.c */
	UiBuddy *newguy;	
	gtk_tree_store_append(roster, &main_iter, NULL);
	gtk_tree_store_set(roster, &main_iter, COL_STATUS, offline_icon,
	                   COL_NAME, nick, -1);
	newguy = g_malloc(sizeof(UiBuddy));
	newguy->jid = jid;
	newguy->name = nick;
	newguy->iter = main_iter;
	entries = g_slist_prepend(entries, newguy);
} /* ui_roster_add */

void
ui_roster_cleanup(void)
{
	/* This function, called by destroy() in ui.c frees the allocated icons
	 * and the list containing rows data */
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
	g_slist_foreach(entries, g_free, NULL);
	g_slist_free(entries);
}

GtkWidget *
ui_roster_setup(void)
{
	/* This function, called in ui_setup() prepares
	 * our roster widget for later use */
	GtkWidget *view;
	GtkCellRenderer *txt_rend, *pix_rend;
	GtkTreeSortable *sort;
	/* tree store */
	roster = gtk_tree_store_new(NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	/* tree view */
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(roster));
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
ui_roster_update(const char *jid, XmppStatus s)
{
	/* This function updates the appropriate entry, changing its status icon.
	 * The entry is determined by the given jid, which is compared to the one
	 * kept in entries GSList */
	GSList *entry;
	UiBuddy *sb;
	GdkPixbuf *icon;
	/* looking up entry by jid */
	entry = g_slist_find_custom(entries, jid, match_entry_by_jid);
	if(!entry) {
		/* should never happen */
		g_printerr("ui_roster_update: cannot update "
		           "non-existing entry %s\n", jid);
		return;
	}
	/* changing the icon */
	sb = (UiBuddy *)entry->data;
	if(s == STATUS_ONLINE) icon = online_icon;
	else if(s == STATUS_AWAY) icon = away_icon;
	else if(s == STATUS_FFC) icon = ffc_icon;
	else if(s == STATUS_XA) icon = xa_icon;
	else if(s == STATUS_DND) icon = dnd_icon;
	else if(s == STATUS_OFFLINE) icon = offline_icon;
	else {
		/* should never happen */
		g_printerr("ui_roster_update: requested invalid status %d\n", s);
		return;
	}
	gtk_tree_store_set(roster, &(sb->iter), COL_STATUS, icon, -1);
} /* ui_roster_update */
