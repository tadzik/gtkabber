#include "config.h"
#include "ui.h"
#include "xmpp.h"
#include <gtk/gtk.h>

int main(int argc, char *argv[]) {
	config_init();
	ui_setup(&argc, &argv);
	xmpp_init();
	gtk_main();
	return 0;
}
