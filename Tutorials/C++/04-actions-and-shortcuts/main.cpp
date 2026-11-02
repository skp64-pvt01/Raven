/**
 * Raven tutorial 04 - actions and shortcuts.
 *
 * A menu item, a button of an XML interface and a keyboard shortcut can all
 * run *the same* function if it is registered under a name:
 *
 *     Raction_bind("counter.add", on_action, nullptr);
 *     Raction_fire("counter.add", sender);          from code
 *     Rwindow_register_shortcut(window, R_KEY_I, R_MOD_CTRL, "counter.add");
 *
 * The action arrives in one callback with its name, so `strcmp` on the name is
 * the whole dispatcher of a small application.
 *
 * Build and test: see README.md next to this file.
 */

#include <Raven.h>

#include <cstdio>
#include <cstring>

#include "../common/tutorial.h"

static Rid g_count = R_INVALID_ID;
static Rid g_status = R_INVALID_ID;
static Rid g_add = R_INVALID_ID;
static Rid g_add_ten = R_INVALID_ID;
static Rid g_enabled = R_INVALID_ID;
static int g_value = 0;

static void show() {
	char text[64];
	std::snprintf(text, sizeof(text), "%d", g_value);
	Rlabel_set_text(g_count, text);
}

static void run(const char* action) {
	if (std::strcmp(action, "counter.add") == 0) {
		g_value += 1;
	} else if (std::strcmp(action, "counter.add_ten") == 0) {
		g_value += 10;
	} else if (std::strcmp(action, "counter.reset") == 0) {
		g_value = 0;
	} else {
		return;
	}
	show();

	char status[96];
	std::snprintf(status, sizeof(status), "Last action: %s", action);
	Rlabel_set_text(g_status, status);
}

/* One callback for every action of the application. */
static void on_action(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	run(action);
}

static void on_enabled(Rid checkbox, int checked, void* user_data) {
	(void)checkbox;
	(void)user_data;
	Rwidget_set_enabled(g_add, checked);
	Rwidget_set_enabled(g_add_ten, checked);
	Rlabel_set_text(g_status, checked ? "Add buttons enabled." : "Add buttons disabled.");
}

int main(int argc, char** argv) {
	if (!tutorial::init(argc, argv, "Raven tutorial 04")) return 1;

	Rid window = Rwindow_create("Tutorial 04 - Actions", 480, 400, R_WINDOW_DEFAULT);

	Rid column = Rwidget_create(R_WIDGET_VBOX);
	Rpanel_set_spacing(column, 10.0f);
	Rslot_set_padding(column, 20.0f, 20.0f, 20.0f, 20.0f);
	Rwindow_set_content(window, column);

	Rid title = Rwidget_add_child(column, Rlabel_create("Actions"));
	Rlabel_set_text_size(title, 22.0f);

	/* The three names this program knows. */
	Raction_bind("counter.add", on_action, nullptr);
	Raction_bind("counter.add_ten", on_action, nullptr);
	Raction_bind("counter.reset", on_action, nullptr);

	g_count = Rwidget_add_child(column, Rlabel_create("0"));
	Rlabel_set_text_size(g_count, 40.0f);
	Rslot_set_halign(g_count, R_HALIGN_CENTER);

	Rid row = Rwidget_add_child(column, Rwidget_create(R_WIDGET_HBOX));
	Rpanel_set_spacing(row, 8.0f);
	Rslot_set_halign(row, R_HALIGN_CENTER);

	/* The buttons only fire the action: the logic lives in run(), once. */
	g_add = Rwidget_add_child(row, Rbutton_create("+1"));
	Rbutton_set_style(g_add, R_BUTTON_PRIMARY);
	Rbutton_set_clicked_callback(g_add, [](Rid sender, void*) {
		Raction_fire("counter.add", sender);
	}, nullptr);
	Rwidget_set_tooltip(g_add, "Ctrl+I");

	g_add_ten = Rwidget_add_child(row, Rbutton_create("+10"));
	Rbutton_set_clicked_callback(g_add_ten, [](Rid sender, void*) {
		Raction_fire("counter.add_ten", sender);
	}, nullptr);

	Rid reset = Rwidget_add_child(row, Rbutton_create("Reset"));
	Rbutton_set_clicked_callback(reset, [](Rid sender, void*) {
		Raction_fire("counter.reset", sender);
	}, nullptr);
	Rwidget_set_tooltip(reset, "Ctrl+0");

	/* Keyboard shortcuts run the same actions, wherever the focus is. */
	Rwindow_register_shortcut(window, R_KEY_I, R_MOD_CTRL, "counter.add");
	Rwindow_register_shortcut(window, R_KEY_0, R_MOD_CTRL, "counter.reset");

	Rwidget_add_child(column, Rwidget_create(R_WIDGET_SEPARATOR));

	/* A checkbox that switches the two add buttons on and off. */
	g_enabled = Rwidget_add_child(column, Rcheckbox_create("Add buttons enabled"));
	Rcheckbox_set_checked(g_enabled, R_TRUE);
	Rcheckbox_set_changed_callback(g_enabled, on_enabled, nullptr);

	Rid spacer = Rwidget_add_child(column, Rwidget_create(R_WIDGET_SPACER));
	Rslot_set_fill(spacer, 1.0f);

	g_status = Rwidget_add_child(column, Rlabel_create("Ready."));
	Rwidget_set_opacity(g_status, 0.7f);

	if (tutorial::is_test(argc, argv)) {
		Rdraw_frame();

		/* A click on the button fires the action. */
		tutorial::click(window, g_add);
		TUTORIAL_CHECK(g_value == 1);
		TUTORIAL_CHECK(std::strcmp(Rlabel_get_text(g_count), "1") == 0);

		/* A shortcut fires the same action without touching the button. */
		tutorial::key(window, R_KEY_I, R_MOD_CTRL);
		TUTORIAL_CHECK(g_value == 2);

		/* An action can also be fired from code, with no widget at all. */
		Raction_fire("counter.add_ten", R_INVALID_ID);
		TUTORIAL_CHECK(g_value == 12);

		/* A disabled widget takes no clicks. */
		tutorial::click(window, g_enabled);        /* unchecks it */
		TUTORIAL_CHECK(Rcheckbox_is_checked(g_enabled) == R_FALSE);
		tutorial::click(window, g_add);
		TUTORIAL_CHECK(g_value == 12);             /* unchanged */

		/* ... but the action itself still works: the shortcut is not the button. */
		tutorial::key(window, R_KEY_I, R_MOD_CTRL);
		TUTORIAL_CHECK(g_value == 13);

		/* Reset, by its shortcut. */
		tutorial::key(window, R_KEY_0, R_MOD_CTRL);
		TUTORIAL_CHECK(g_value == 0);
		TUTORIAL_CHECK(std::strcmp(Rlabel_get_text(g_status), "Last action: counter.reset") == 0);

		return tutorial::finish("04-actions-and-shortcuts");
	}

	while (Rwait_frame(-1.0)) {
	}
	return tutorial::finish("04-actions-and-shortcuts");
}
