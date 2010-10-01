#include "common.h"
#include "ui.h"
#include "ui_tabs.h"

static void cbox_changed_cb(GtkComboBox *);
static void destroy(void);
static void setup_cbox(GtkWidget *);

static Ui *ui; // This is a cheat, but Ui is a singleton anyway

static void
cbox_changed_cb(GtkComboBox *e)
{
    ui_print("Status changed to %d\n", gtk_combo_box_get_active(e));
} /* cbox_changed_cb */

static void destroy(void)
{
    gtk_main_quit();
    g_free(ui->status_entry);
    ui_tabs_cleanup(ui->tabs);
    g_free(ui);
} /* destroy */

static void
setup_cbox(GtkWidget *cbox)
{
    gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Free for chat");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Online");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Away");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Not available");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Do not disturb");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), "Offline");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cbox), 1);
    g_signal_connect(G_OBJECT(cbox), "changed",
                     G_CALLBACK(cbox_changed_cb), NULL);
} /* setup_cbox */

static void
statusentry_cb(Mlentry *e, const gchar *str, gpointer u)
{
    UNUSED(e); //TODO?
    ui_print("Changing status to %s\n", str); // TODO, of course
    UNUSED(u);
} /* statusentry_cb */

Ui *
ui_init(int *argc, char **argv[])
{
    GtkWidget *hpaned, *left, *rwin, *vbox;
    g_assert(!ui); // ui is a singleton
    Ui *new = ui = g_malloc(sizeof(Ui));
    gtk_init(argc, argv);

    /* creating */
    new->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(new->window), "gtkabberng");
    gtk_widget_set_size_request(new->window, 640, 480);

    vbox = gtk_vbox_new(FALSE, 0);
    
    hpaned = gtk_hpaned_new();
    new->toolbox = gtk_vbox_new(FALSE, 0);

    left = gtk_vbox_new(FALSE, 0);
    rwin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(rwin),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    new->roster = gtk_tree_view_new(); /* TODO proper ui_roster object */
    new->status_box = gtk_combo_box_new_text();
    setup_cbox(new->status_box);

    new->status_entry = mlentry_new(statusentry_cb, NULL);

    new->tabs = ui_tabs_new();

    /* packing */
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(rwin),
                                          new->roster);

    gtk_container_add(GTK_CONTAINER(new->window), vbox);

    gtk_container_add(GTK_CONTAINER(vbox), hpaned);
    gtk_container_add(GTK_CONTAINER(vbox), new->toolbox);
    gtk_box_set_child_packing(GTK_BOX(vbox), new->toolbox,
                              FALSE, FALSE, 0, GTK_PACK_START);

    gtk_paned_add1(GTK_PANED(hpaned), left);
    gtk_paned_add2(GTK_PANED(hpaned), new->tabs->nbook);

    gtk_container_add(GTK_CONTAINER(left), rwin);
    gtk_container_add(GTK_CONTAINER(left), new->status_box);
    gtk_container_add(GTK_CONTAINER(left), new->status_entry->widget);
    gtk_box_set_child_packing(GTK_BOX(left), new->status_box,
                              FALSE, FALSE, 0, GTK_PACK_START);
    gtk_box_set_child_packing(GTK_BOX(left), new->status_entry->widget,
                              FALSE, FALSE, 0, GTK_PACK_START);

    /* signals */
    g_signal_connect(G_OBJECT(new->window), "destroy",
                     G_CALLBACK(destroy), new);
/*  g_signal_connect(G_OBJECT(window), "key-press-event",
                     G_CALLBACK(keypress_cb), NULL);
    g_signal_connect(G_OBJECT(window), "focus-in-event",
                     G_CALLBACK(focus_cb), NULL);
*/
    gtk_widget_show_all(new->window);

    return new;
} /* ui_init */

void
ui_print(const gchar *msg, ...)
{
    va_list ap;
    char *str;
    va_start(ap, msg);
    str = g_strdup_vprintf(msg, ap);
    ui_tabs_log(ui->tabs, str);
    g_printerr("%s", str);
    va_end(ap);
    g_free(str);
} /* ui_print */

void
ui_run(void)
{
    gtk_main();
} /* ui_run */
