/**
 * Raven tutorial 03 - controls and state.
 *
 * A widget holds its own state; the program reads it with a getter and writes
 * it with a setter. Callbacks say *when* something changed, never *what* to
 * keep: the widget is the source of truth.
 *
 *   R_WIDGET_TEXT_INPUT   Rtext_input_get_text
 *   R_WIDGET_CHECKBOX     Rcheckbox_is_checked
 *   R_WIDGET_SLIDER       Rslider_get_value
 *   R_WIDGET_COMBO_BOX    Rcombo_box_get_selected
 *
 * Build and test: see README.md next to this file.
 */

#include <Raven.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "../common/tutorial.h"

static Rid g_name = R_INVALID_ID;
static Rid g_loud = R_INVALID_ID;
static Rid g_size = R_INVALID_ID;
static Rid g_theme = R_INVALID_ID;
static Rid g_preview = R_INVALID_ID;
static int g_updates = 0;

/** Reads every control and shows the result. Called by all four callbacks. */
static void refresh() {
	const char* typed = Rtext_input_get_text(g_name);
	std::string name = typed && typed[0] ? typed : "world";
	if (Rcheckbox_is_checked(g_loud)) {
		for (char& c : name) {
			if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
		}
	}
	char text[256];
	std::snprintf(text, sizeof(text), "Hello, %s!", name.c_str());
	Rlabel_set_text(g_preview, text);
	Rlabel_set_text_size(g_preview, Rslider_get_value(g_size));
	++g_updates;
}

static void on_changed(Rid widget, const char* text, void* user_data) {
	(void)widget;
	(void)text;
	(void)user_data;
	refresh();
}

static void on_check(Rid checkbox, int checked, void* user_data) {
	(void)checkbox;
	(void)checked;
	(void)user_data;
	refresh();
}

static void on_slide(Rid slider, float value, void* user_data) {
	(void)slider;
	(void)value;
	(void)user_data;
	refresh();
}

static void on_theme(Rid combo, int index, void* user_data) {
	(void)combo;
	(void)user_data;
	Rtheme_apply(index == 1 ? R_THEME_DARK : R_THEME_LIGHT);
}

int main(int argc, char** argv) {
	if (!tutorial::init(argc, argv, "Raven tutorial 03")) return 1;

	Rid window = Rwindow_create("Tutorial 03 - Controls", 520, 420, R_WINDOW_DEFAULT);

	Rid column = Rwidget_create(R_WIDGET_VBOX);
	Rpanel_set_spacing(column, 10.0f);
	Rslot_set_padding(column, 20.0f, 20.0f, 20.0f, 20.0f);
	Rwindow_set_content(window, column);

	Rid title = Rwidget_add_child(column, Rlabel_create("Controls"));
	Rlabel_set_text_size(title, 22.0f);

	/* --- single-line text -------------------------------------------------- */
	Rwidget_add_child(column, Rlabel_create("Your name"));
	g_name = Rwidget_add_child(column, Rwidget_create(R_WIDGET_TEXT_INPUT));
	Rtext_input_set_placeholder(g_name, "Type a name");
	Rtext_input_set_changed_callback(g_name, on_changed, nullptr);
	Rtext_input_set_committed_callback(g_name, on_changed, nullptr);

	/* --- a checkbox -------------------------------------------------------- */
	g_loud = Rwidget_add_child(column, Rcheckbox_create("Shout it"));
	Rcheckbox_set_changed_callback(g_loud, on_check, nullptr);

	/* --- a slider ---------------------------------------------------------- */
	Rwidget_add_child(column, Rlabel_create("Text size"));
	g_size = Rwidget_add_child(column, Rwidget_create(R_WIDGET_SLIDER));
	Rslider_set_range(g_size, 12.0f, 32.0f);
	Rslider_set_value(g_size, 20.0f);
	Rslider_set_changed_callback(g_size, on_slide, nullptr);

	/* --- a combo box ------------------------------------------------------- */
	Rwidget_add_child(column, Rlabel_create("Theme"));
	g_theme = Rwidget_add_child(column, Rwidget_create(R_WIDGET_COMBO_BOX));
	Rcombo_box_add_item(g_theme, "Light");
	Rcombo_box_add_item(g_theme, "Dark");
	Rcombo_box_set_selected(g_theme, 0);
	Rcombo_box_set_changed_callback(g_theme, on_theme, nullptr);

	Rwidget_add_child(column, Rwidget_create(R_WIDGET_SEPARATOR));

	/* --- the widget the state feeds --------------------------------------- */
	g_preview = Rwidget_add_child(column, Rlabel_create("Hello, world!"));
	Rlabel_set_text_size(g_preview, 20.0f);
	Rslot_set_halign(g_preview, R_HALIGN_CENTER);

	if (tutorial::is_test(argc, argv)) {
		Rdraw_frame();

		/* Typing goes to the focused input, and fires the changed callback. */
		tutorial::click(window, g_name);
		Rwindow_inject_text(window, "Ada");
		TUTORIAL_CHECK(std::strcmp(Rtext_input_get_text(g_name), "Ada") == 0);
		TUTORIAL_CHECK(std::strcmp(Rlabel_get_text(g_preview), "Hello, Ada!") == 0);

		/* The checkbox toggles, and the preview follows. */
		tutorial::click(window, g_loud);
		TUTORIAL_CHECK(Rcheckbox_is_checked(g_loud) == R_TRUE);
		TUTORIAL_CHECK(std::strcmp(Rlabel_get_text(g_preview), "Hello, ADA!") == 0);

		/* The middle of the slider is the middle of the range: (12 + 32) / 2. */
		tutorial::click(window, g_size);
		TUTORIAL_CHECK(std::fabs(Rslider_get_value(g_size) - 22.0f) < 1.0f);

		/* The combo box: a click opens the list, a click on the second row picks it. */
		float x = 0, y = 0, w = 0, h = 0;
		Rwidget_get_rect(g_theme, &x, &y, &w, &h);
		float row = Rtheme_get_metric(R_METRIC_ROW_HEIGHT);
		tutorial::click(window, g_theme);
		tutorial::click_at(window, x + 10.0f, y + h + 2.0f + row * 1.5f);
		TUTORIAL_CHECK(Rcombo_box_get_selected(g_theme) == 1);
		TUTORIAL_CHECK(Rtheme_is_dark() == R_TRUE);

		TUTORIAL_CHECK(g_updates >= 3);
		return tutorial::finish("03-controls");
	}

	while (Rwait_frame(-1.0)) {
	}
	return tutorial::finish("03-controls");
}
