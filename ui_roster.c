#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "xmpp_roster.h"
#include "types.h"

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
static int get_pixbuf_priority(GdkPixbuf *);
static GdkPixbuf *load_icon(const char *);
static void load_iconset(void);
static int match_entry_by_iter(gconstpointer, gconstpointer);
static int match_entry_by_jid(gconstpointer, gconstpointer);
static void row_clicked_cb(GtkTreeView *, GtkTreePath *,
                           GtkTreeViewColumn *, gpointer);
void ui_roster_add(const char *, const char *);
void ui_roster_cleanup(void);
GtkWidget *ui_roster_setup(void);
void ui_roster_update(const char *, XmppStatus);
/************/

static int
get_pixbuf_priority(GdkPixbuf *p)
{
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
	GdkPixbuf *p1, *p2;
	gtk_tree_model_get(m, a, COL_STATUS, &p1, -1);
	gtk_tree_model_get(m, b, COL_STATUS, &p2, -1);
	if(get_pixbuf_priority(p1) < get_pixbuf_priority(p2))
		return -1;
	else if(get_pixbuf_priority(p1) > get_pixbuf_priority(p2))
		return 1;
	else {
		char *n1, *n2;
		int ret;
		gtk_tree_model_get(m, a, COL_NAME, &n1, -1);
		gtk_tree_model_get(m, b, COL_NAME, &n2, -1);
		ret = strcmp(n1, n2);
		free(n1);
		free(n2);
		return ret;
	}
} /* compare_rows */

static GdkPixbuf *
load_icon(const char *status)
{
	GdkPixbuf *new;
	GError *err = NULL;
	char path[20];
	/* TODO: Here there should be some iconset choosing
	 * (once I'll make it possible :])*/
	snprintf(path, sizeof(path), "icons/%s.png", status);
	new = gdk_pixbuf_new_from_file(path, &err);
	if (new == NULL) {
		ui_status_print("Error loading icon %s: %s\n", path, err->message);
		g_error_free(err);
	}
	return new;
} /* load_icon */

static void
load_iconset()
{
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
	/*TODO: Check if comparing GtkTreeIter.stamp would do the job*/
	int ret;
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
	UiBuddy *sb = (UiBuddy *)e;
	if(strcmp(sb->jid, j) == 0)
		return 0;
	return 1;
} /* match_entry_by_jid */

static void
row_clicked_cb(GtkTreeView *t, GtkTreePath *p, GtkTreeViewColumn *c, gpointer d)
{
	GtkTreeIter iter;
	GSList *entry;
	UiBuddy *sb;
	gtk_tree_model_get_iter(gtk_tree_view_get_model(t), &iter, p);
	entry = g_slist_find_custom(entries, (void *)&iter, match_entry_by_iter);
	if(entry) {
		Chattab *tab;
		const char *resname;
		GString *fulljid;
		tab = malloc(sizeof(Chattab));
		sb = (UiBuddy *)entry->data;
		resname = xmpp_roster_get_best_resname(sb->jid);
		fulljid = g_string_new(sb->jid);
		g_string_append_printf(fulljid, "/%s", resname);
		tab->jid = strdup(fulljid->str);
		tab->title = strdup(sb->name);
		ui_create_tab(tab);
		g_string_free(fulljid, TRUE);
	}
} /* row_clicked_cb */

void
ui_roster_add(const char *jid, const char *nick)
{
	UiBuddy *newguy;	
	gtk_tree_store_append(roster, &main_iter, NULL);
	gtk_tree_store_set(roster, &main_iter, COL_STATUS, offline_icon,
	                   COL_NAME, nick, -1);
	newguy = malloc(sizeof(UiBuddy));
	newguy->jid = jid;
	newguy->name = nick;
	newguy->iter = main_iter;
	entries = g_slist_prepend(entries, newguy);
} /* ui_roster_add */

void
ui_roster_cleanup(void)
{
	if(offline_icon)
		gdk_pixbuf_unref(offline_icon);
	if(online_icon)
		gdk_pixbuf_unref(online_icon);
	if(away_icon)
		gdk_pixbuf_unref(away_icon);
	if(dnd_icon)
		gdk_pixbuf_unref(dnd_icon);
	if(ffc_icon)
		gdk_pixbuf_unref(ffc_icon);
	if(xa_icon)
		gdk_pixbuf_unref(xa_icon);
	g_slist_free(entries);
}

GtkWidget *
ui_roster_setup(void)
{
	GtkWidget *view;
	GtkCellRenderer *txt_rend, *pix_rend;
	GtkTreeSortable *sort;
	/* tree store */
	roster = gtk_tree_store_new(NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	/* tree view */
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(roster));
	txt_rend = gtk_cell_renderer_text_new();
	pix_rend = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
	                                            -1, NULL, pix_rend,
	                                            "pixbuf", COL_STATUS, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
	                                            -1, NULL, txt_rend,
	                                            "text", COL_NAME, NULL);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
	/* selection */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	load_iconset();
	/* sorting */
	sort = GTK_TREE_SORTABLE(roster);
	gtk_tree_sortable_set_sort_func(sort, COL_STATUS, compare_rows,
	                                NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(sort, COL_STATUS, GTK_SORT_ASCENDING);
	/* signals */
	g_signal_connect(G_OBJECT(view), "row-activated", 
	                 G_CALLBACK(row_clicked_cb), NULL);
	return view;
} /* roster_setup */

void
ui_roster_update(const char *jid, XmppStatus s)
{
	GSList *entry;
	UiBuddy *sb;
	GdkPixbuf *icon;
	entry = g_slist_find_custom(entries, jid, match_entry_by_jid);
	if(!entry) {
		ui_status_print("ui_roster_update: cannot update "
		                "non-existing entry %s\n", jid);
		return;
	}
	sb = (UiBuddy *)entry->data;
	if(s == STATUS_ONLINE) icon = online_icon;
	else if(s == STATUS_AWAY) icon = away_icon;
	else if(s == STATUS_FFC) icon = ffc_icon;
	else if(s == STATUS_XA) icon = xa_icon;
	else if(s == STATUS_DND) icon = dnd_icon;
	else if(s == STATUS_OFFLINE) icon = offline_icon;
	else {
		ui_status_print("ui_roster_update: requested invalid status %d\n", s);
		return;
	}
	gtk_tree_store_set(roster, &(sb->iter), COL_STATUS, icon, -1);
} /* ui_roster_update */
