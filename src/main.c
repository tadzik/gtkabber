#include "config.h"
#include "ui.h"
#include "xmpp_conn.h"
#include <gtk/gtk.h>

int main(int argc, char *argv[]) {
	if (config_init())
		return 1;
	ui_setup(&argc, &argv);
	xmpp_init();
	gtk_main();
	return 0;
}
