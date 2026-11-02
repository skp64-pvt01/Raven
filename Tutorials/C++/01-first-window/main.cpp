/**
 * Raven tutorial 01 - a first window.
 *
 * Three ideas only:
 *   1. Rinit_ex starts the library, Rshutdown ends it.
 *   2. A window holds one tree of widgets. Rwindow_set_content puts the root.
 *   3. The frame loop belongs to the program: Raven draws when it is asked to.
 *
 * Build and test: see README.md next to this file.
 */

#include <Raven.h>

#include <cstdio>

#include "../common/tutorial.h"

/* The widgets the callbacks touch. A real program keeps these in a struct;
 * one label and one counter are small enough to live at file scope. */
static Rid g_label = R_INVALID_ID;
static int g_clicks = 0;

static void on_click(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	++g_clicks;

	char text[64];
	std::snprintf(text, sizeof(text), "Clicked %d time%s", g_clicks, g_clicks == 1 ? "" : "s");
	Rlabel_set_text(g_label, text);
}

static void on_quit(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	Rquit(0);
}

int main(int argc, char** argv) {
	if (!tutorial::init(argc, argv, "Raven tutorial 01")) return 1;

	/* 1. A window. The last argument is a set of R_WINDOW_* flags. */
	Rid window = Rwindow_create("Tutorial 01 - First window", 440, 240, R_WINDOW_DEFAULT);

	/* 2. A vertical box: it stacks its children top to bottom. */
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rpanel_set_spacing(box, 12.0f);
	Rslot_set_padding(box, 24.0f, 24.0f, 24.0f, 24.0f);
	Rslot_set_halign(box, R_HALIGN_CENTER);
	Rslot_set_valign(box, R_VALIGN_CENTER);
	Rwindow_set_content(window, box);

	/* A label, larger than the default text. */
	Rid title = Rwidget_add_child(box, Rlabel_create("Hello from C++"));
	Rlabel_set_text_size(title, 24.0f);
	Rslot_set_halign(title, R_HALIGN_CENTER);

	/* A second label, the one the button changes. */
	g_label = Rwidget_add_child(box, Rlabel_create("Press the button"));
	Rslot_set_halign(g_label, R_HALIGN_CENTER);

	/* A row for the buttons. */
	Rid row = Rwidget_add_child(box, Rwidget_create(R_WIDGET_HBOX));
	Rpanel_set_spacing(row, 8.0f);
	Rslot_set_halign(row, R_HALIGN_CENTER);

	Rid click_me = Rwidget_add_child(row, Rbutton_create("Click me"));
	Rbutton_set_style(click_me, R_BUTTON_PRIMARY);
	Rbutton_set_clicked_callback(click_me, on_click, nullptr);
	Rwidget_set_tooltip(click_me, "Counts the clicks");

	Rid quit = Rwidget_add_child(row, Rbutton_create("Quit"));
	Rbutton_set_clicked_callback(quit, on_quit, nullptr);

	/* 3. --test: the same widgets, driven by synthetic input, no window. */
	if (tutorial::is_test(argc, argv)) {
		/* One frame lays the widgets out: before it, every rect is still empty. */
		Rdraw_frame();
		tutorial::click(window, click_me);
		TUTORIAL_CHECK(g_clicks == 1);
		TUTORIAL_CHECK(std::strcmp(Rlabel_get_text(g_label), "Clicked 1 time") == 0);

		tutorial::click(window, click_me);
		TUTORIAL_CHECK(g_clicks == 2);
		TUTORIAL_CHECK(std::strcmp(Rlabel_get_text(g_label), "Clicked 2 times") == 0);

		tutorial::click(window, quit);
		TUTORIAL_CHECK(!Ris_running());
		return tutorial::finish("01-first-window");
	}

	/* The loop: Rwait_frame draws a frame, then sleeps until something happens.
	 * It returns R_FALSE once the application quits. */
	while (Rwait_frame(-1.0)) {
	}

	return tutorial::finish("01-first-window");
}
