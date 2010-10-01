#include "common.h"
#include "gtk_tbim.h"
#include "ui_tabs.h"

static void ui_tabs_scroll_down(Chattab *);
static void ui_tabs_append_anyhow(Chattab *, const gchar *,
                                  void (*fun)(
                                      GtkTextBuffer *, GtkTextIter *,
                                      const gchar *, gint
                                  ));

static void
ui_tabs_append_anyhow(Chattab *t, const gchar *s,
                      void (*fun)(
                          GtkTextBuffer *, GtkTextIter *,
                          const gchar *, gint
                      ))
{
    GtkTextIter i;
    g_assert(t);
    gtk_text_buffer_get_end_iter(t->buffer, &i);
    (*fun)(t->buffer, &i, s, -1);
    ui_tabs_scroll_down(t);
} /* ui_tabs_append_anyhow */

void
ui_tabs_append_text(Chattab *t, const gchar *s)
{
    ui_tabs_append_anyhow(t, s, gtk_text_buffer_insert);
} /* ui_tabs_append_text */

void
ui_tabs_append_markup(Chattab *t, const gchar *s)
{
    ui_tabs_append_anyhow(t, s, gtk_text_buffer_insert_markup);
} /* ui_tabs_append_markup */

void
ui_tabs_cleanup(Notebook *t)
{
    if(t->tabs) {
        GSList *elem;
        for(elem = t->tabs; elem; elem = elem->next) {
            Chattab *t = (Chattab *)elem->data;
            g_free(t->jid);
            g_free(t->title);
            g_free(t);
        }
        g_slist_free(t->tabs);
    }
} /* ui_tabs_cleanup */

Chattab *
ui_tabs_create(Notebook *n,
               const gchar *jid, const gchar *title,
               gint active)
{
    Chattab *tab;
    GtkWidget *evbox;
    tab = g_malloc(sizeof(Chattab));
    if (jid == NULL) {
        tab->jid = NULL;
        n->status_tab = tab;
    } else {
        tab->jid = g_strdup(jid);
    }
    tab->title = g_strdup(title);

    /* setting up a place for a tab title */
    evbox = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(evbox), FALSE);
    tab->label = gtk_label_new(tab->title);
    gtk_container_add(GTK_CONTAINER(evbox), tab->label);
    gtk_widget_show(tab->label);
    /*
    g_signal_connect(G_OBJECT(evbox), "button-press-event",
                     G_CALLBACK(tab_label_click_cb), (gpointer)tab);
    */

    /* creating GtkTextView for status messages */
    tab->tview = gtk_text_view_new();
    tab->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->tview));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tab->tview), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tab->tview), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tab->tview),
                                GTK_WRAP_WORD);
    tab->mk = gtk_text_buffer_get_mark(tab->buffer, "insert");

    /* we're putting this in a scrolled window */
    tab->scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tab->scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(
        GTK_SCROLLED_WINDOW(tab->scrolled),
        tab->tview
    );

    /* setting up the entry field */
    if (jid) {
        /*tab->entry = mlentry_new(tab_entry_handler, tab);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tab->entry),
                                    GTK_WRAP_WORD_CHAR);
        gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(tab->entry), FALSE);
        */
        tab->entry = gtk_entry_new();
    }

    /* some vbox to put it together */
    tab->vbox = gtk_vpaned_new();

    /* this will help us finding Chattab struct by its vbox */
    g_object_set_data(G_OBJECT(tab->vbox), "chattab-data", tab);

    /* now let's put it all together */
    gtk_paned_pack1(GTK_PANED(tab->vbox), tab->scrolled, TRUE, FALSE);
    if (jid) {
        gtk_paned_pack2(GTK_PANED(tab->vbox), tab->entry, FALSE, FALSE);
    }
    gtk_widget_show_all(tab->vbox);

    /* aaand, launch! */
    gtk_notebook_append_page(GTK_NOTEBOOK(n->nbook), tab->vbox, evbox);
    n->tabs = g_slist_prepend(n->tabs, tab);
    if (active && jid) {
        gtk_notebook_set_current_page(
            GTK_NOTEBOOK(n->nbook),
            gtk_notebook_page_num(GTK_NOTEBOOK(n->nbook), tab->vbox)
        );
        gtk_widget_grab_focus(tab->entry);
    }
    return tab;
} /* ui_tabs_create */

void
ui_tabs_log(Notebook *n, const gchar *s)
{
    ui_tabs_append_text(n->status_tab, s);
} /* ui_tabs_log */

Notebook *
ui_tabs_new(void)
{
    Notebook *new = g_malloc(sizeof(Notebook));
    new->nbook = gtk_notebook_new();
    new->tabs = NULL;
    /*
    g_signal_connect(G_OBJECT(new->nbook), "switch-page",
                     G_CALLBACK(tab_switch_cb), NULL);
    */
    ui_tabs_create(new, NULL, "Status", 0);
    return new;
}

static void
ui_tabs_scroll_down(Chattab *tab)
{
    GtkAdjustment *adj;
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tab->tview),
                                 tab->mk, 0.0, FALSE, 0.0, 0.0);
    adj = gtk_scrolled_window_get_vadjustment(
            GTK_SCROLLED_WINDOW(tab->scrolled)
          );
    gtk_adjustment_set_value(adj, adj->upper);
} /* tab_scroll_down */
