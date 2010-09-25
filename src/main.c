#include "ui.h"

int main(int argc, char *argv[]) {
    Ui *gui;
	gui = ui_init(&argc, &argv);
    ui_run();
	return 0;
}
