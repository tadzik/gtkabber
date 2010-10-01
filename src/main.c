#include "ui.h"
#include "ui_roster.h"
#include "xmpp_roster.h"

int main(int argc, char *argv[]) {
    Ui *gui;
	gui = ui_init(&argc, &argv);
    Buddy pal;
    pal.jid = "foo";
    pal.name = "bar";
    pal.group = "basterds";
    ui_roster_add(&pal);
    ui_run();
	return 0;
}
