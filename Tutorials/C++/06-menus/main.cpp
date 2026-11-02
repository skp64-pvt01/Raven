/**
 * Raven tutorial 06 - menus.
 *
 * A menu bar is a widget like the others:
 *
 *   R_WIDGET_MENU_BAR          the bar
 *   Rmenu_add_menu(bar, "File")     a title in the bar, or a sub-menu of a menu
 *   Rmenu_add_item(menu, "New")     an entry; Rmenu_add_separator for a line
 *   Rmenu_item_set_shortcut         Ctrl+N works even with the menu closed
 *   Rmenu_item_set_checkable        an entry with a check mark
 *   Rmenu_item_set_clicked_callback what the entry runs
 *
 * A right-click menu is the same thing built from R_WIDGET_MENU and attached
 * with Rwidget_set_context_menu.
 *
 * Build and test: see README.md next to this file.
 */

#include <Raven.h>

#include <cstdio>
#include <cstring>

#include "../common/tutorial.h"

static Rid g_window = R_INVALID_ID;
static Rid g_status = R_INVALID_ID;
static Rid g_show = R_INVALID_ID;
static char g_last[64] = "none";

static void on_item(Rid item, void* user_data) {
	const char* name = static_cast<const char*>(user_data);
	char text[128];
	std::snprintf(g_last, sizeof(g_last), "%s", name);

	if (std::strcmp(name, "Quit") == 0) Rquit(0);
	if (std::strcmp(name, "Dark") == 0) Rtheme_apply(R_THEME_DARK);
	if (std::strcmp(name, "Light") == 0) Rtheme_apply(R_THEME_LIGHT);

	if (std::strcmp(name, "Show status") == 0) {
		/* A checkable entry keeps its state itself: ask it, never a mirror. */
		Rlabel_set_text(g_status, Rmenu_item_is_checked(item) ? "Status shown" : "Status hidden");
	} else if (Rmenu_item_is_checked(g_show)) {
		std::snprintf(text, sizeof(text), "Menu: %s", name);
		Rlabel_set_text(g_status, text);
	}
}

int main(int argc, char** argv) {
	if (!tutorial::init(argc, argv, "Raven tutorial 06")) return 1;

	g_window = Rwindow_create("Tutorial 06 - Menus", 640, 420, R_WINDOW_DEFAULT);

	Rid column = Rwidget_create(R_WIDGET_VBOX);
	Rpanel_set_spacing(column, 0.0f);
	Rwindow_set_content(g_window, column);

	/* --- the bar ----------------------------------------------------------- */
	Rid bar = Rwidget_add_child(column, Rwidget_create(R_WIDGET_MENU_BAR));

	Rid file = Rmenu_add_menu(bar, "File");
	Rid new_item = Rmenu_add_item(file, "New");
	Rmenu_item_set_shortcut(new_item, R_KEY_N, R_MOD_CTRL);
	Rmenu_item_set_clicked_callback(new_item, on_item, const_cast<char*>("New"));

	Rid open_item = Rmenu_add_item(file, "Open...");
	Rmenu_item_set_shortcut(open_item, R_KEY_O, R_MOD_CTRL);
	Rmenu_item_set_clicked_callback(open_item, on_item, const_cast<char*>("Open"));

	Rmenu_add_separator(file);

	Rid quit_item = Rmenu_add_item(file, "Quit");
	Rmenu_item_set_shortcut(quit_item, R_KEY_Q, R_MOD_CTRL);
	Rmenu_item_set_clicked_callback(quit_item, on_item, const_cast<char*>("Quit"));

	Rid view = Rmenu_add_menu(bar, "View");
	g_show = Rmenu_add_item(view, "Show status");
	Rmenu_item_set_checkable(g_show, R_TRUE);
	Rmenu_item_set_checked(g_show, R_TRUE);
	Rmenu_item_set_shortcut(g_show, R_KEY_G, R_MOD_CTRL);
	Rmenu_item_set_clicked_callback(g_show, on_item, const_cast<char*>("Show status"));

	Rmenu_add_separator(view);
	Rid dark = Rmenu_add_item(view, "Dark");
	Rmenu_item_set_clicked_callback(dark, on_item, const_cast<char*>("Dark"));
	Rid light = Rmenu_add_item(view, "Light");
	Rmenu_item_set_clicked_callback(light, on_item, const_cast<char*>("Light"));

	/* A sub-menu: added to a menu instead of the bar. */
	Rid recent = Rmenu_add_menu(file, "Recent");
	Rid entry = Rmenu_add_item(recent, "notes.txt");
	Rmenu_item_set_clicked_callback(entry, on_item, const_cast<char*>("notes.txt"));

	/* --- the rest of the window ------------------------------------------- */
	Rid body = Rwidget_add_child(column, Rlabel_create(
		"Open the File menu, or press Ctrl+N, Ctrl+O, Ctrl+G."));
	Rslot_set_fill(body, 1.0f);
	Rslot_set_padding(body, 24.0f, 24.0f, 24.0f, 24.0f);

	g_status = Rwidget_add_child(column, Rlabel_create("Menu: none"));
	Rslot_set_padding(g_status, 12.0f, 0.0f, 12.0f, 12.0f);
	Rwidget_set_opacity(g_status, 0.8f);

	/* --- a right-click menu on the body ------------------------------------ */
	Rid context = Rwidget_create(R_WIDGET_MENU);
	Rid rename = Rmenu_add_item(context, "Rename");
	Rmenu_item_set_clicked_callback(rename, on_item, const_cast<char*>("Rename"));
	Rmenu_add_separator(context);
	Rid remove = Rmenu_add_item(context, "Remove");
	Rmenu_item_set_clicked_callback(remove, on_item, const_cast<char*>("Remove"));
	Rwidget_set_context_menu(body, context);

	if (tutorial::is_test(argc, argv)) {
		Rdraw_frame();

		/* Geometry of the menus: the bar is one control high, its rows are
		 * R_METRIC_ROW_HEIGHT. A click on a title opens it, a click below it
		 * picks the entry at that row. */
		const float bar_h = Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT);
		const float row = Rtheme_get_metric(R_METRIC_ROW_HEIGHT);

		/* File > New (the first entry). */
		tutorial::click_at(g_window, 15.0f, bar_h * 0.5f);
		tutorial::click_at(g_window, 30.0f, bar_h + 4.0f + row * 0.5f);
		TUTORIAL_CHECK(std::strcmp(g_last, "New") == 0);
		TUTORIAL_CHECK(std::strcmp(Rlabel_get_text(g_status), "Menu: New") == 0);

		/* The shortcut of the same entry, with the menu closed. */
		tutorial::key(g_window, R_KEY_O, R_MOD_CTRL);
		TUTORIAL_CHECK(std::strcmp(g_last, "Open") == 0);

		/* A checkable entry toggles itself, then reports it. */
		TUTORIAL_CHECK(Rmenu_item_is_checked(g_show) == R_TRUE);
		tutorial::key(g_window, R_KEY_G, R_MOD_CTRL);
		TUTORIAL_CHECK(std::strcmp(g_last, "Show status") == 0);
		TUTORIAL_CHECK(Rmenu_item_is_checked(g_show) == R_FALSE);
		TUTORIAL_CHECK(std::strcmp(Rlabel_get_text(g_status), "Status hidden") == 0);

		/* With the status switched off, the next entry runs but no longer writes it. */
		tutorial::key(g_window, R_KEY_O, R_MOD_CTRL);
		TUTORIAL_CHECK(std::strcmp(g_last, "Open") == 0);
		TUTORIAL_CHECK(std::strcmp(Rlabel_get_text(g_status), "Status hidden") == 0);

		/* Ctrl+Q runs Quit. */
		tutorial::key(g_window, R_KEY_Q, R_MOD_CTRL);
		TUTORIAL_CHECK(!Ris_running());

		return tutorial::finish("06-menus");
	}

	while (Rwait_frame(-1.0)) {
	}
	return tutorial::finish("06-menus");
}
