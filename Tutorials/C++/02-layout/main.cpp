/**
 * Raven tutorial 02 - layout.
 *
 * A panel arranges its children; each child carries a *slot* that tells the
 * panel what to do with it. Three slot settings cover most interfaces:
 *
 *   Rslot_set_padding   space around the child, inside the panel
 *   Rslot_set_fill      share of the space left over (1 = one share)
 *   Rslot_set_halign    where the child sits when it is smaller than its cell
 *
 * Rpanel_set_spacing sets the gap between the children of a panel once.
 *
 * Build and test: see README.md next to this file.
 */

#include <Raven.h>

#include <cmath>
#include <cstdio>

#include "../common/tutorial.h"

static Rid g_status = R_INVALID_ID;

static void on_button(Rid button, void* user_data) {
	const char* name = static_cast<const char*>(user_data);
	(void)button;
	/* The label takes any text, including one built at run time. */
	char text[128];
	std::snprintf(text, sizeof(text), "Pressed: %s", name);
	Rlabel_set_text(g_status, text);
}

int main(int argc, char** argv) {
	if (!tutorial::init(argc, argv, "Raven tutorial 02")) return 1;

	Rid window = Rwindow_create("Tutorial 02 - Layout", 640, 420, R_WINDOW_DEFAULT);

	/* The root: a column that fills the window by default. */
	Rid column = Rwidget_create(R_WIDGET_VBOX);
	Rpanel_set_spacing(column, 10.0f);
	Rslot_set_padding(column, 16.0f, 16.0f, 16.0f, 16.0f);
	Rwindow_set_content(window, column);

	Rid title = Rwidget_add_child(column, Rlabel_create("Layout"));
	Rlabel_set_text_size(title, 22.0f);

	Rid explain = Rwidget_add_child(column, Rlabel_create(
		"The three buttons below share the width: each one has Slot.Fill = 1."));
	Rslot_set_padding(explain, 0.0f, 0.0f, 0.0f, 6.0f);

	/* --- a row whose children share the space ------------------------------ */
	Rid equal_row = Rwidget_add_child(column, Rwidget_create(R_WIDGET_HBOX));
	Rpanel_set_spacing(equal_row, 8.0f);

	static const char* const names[] = { "One", "Two", "Three" };
	for (const char* name : names) {
		Rid button = Rwidget_add_child(equal_row, Rbutton_create(name));
		/* Every child asks for one share: the row divides its width equally. */
		Rslot_set_fill(button, 1.0f);
		Rbutton_set_clicked_callback(button, on_button, const_cast<char*>(name));
	}

	Rwidget_add_child(column, Rwidget_create(R_WIDGET_SEPARATOR));

	/* --- a row where only one child grows ---------------------------------- */
	Rid mixed_row = Rwidget_add_child(column, Rwidget_create(R_WIDGET_HBOX));
	Rpanel_set_spacing(mixed_row, 8.0f);

	Rid hint = Rwidget_add_child(mixed_row, Rlabel_create("This label grows, the button keeps its size."));
	Rslot_set_fill(hint, 1.0f);          /* takes what the button leaves */
	Rslot_set_valign(hint, R_VALIGN_CENTER);

	Rid fixed = Rwidget_add_child(mixed_row, Rbutton_create("Fixed"));
	Rwidget_set_min_size(fixed, 120.0f, 0.0f);   /* width at least 120 */
	Rbutton_set_clicked_callback(fixed, on_button, const_cast<char*>("Fixed"));

	/* --- a spacer pushes the footer to the bottom -------------------------- */
	Rid spacer = Rwidget_add_child(column, Rwidget_create(R_WIDGET_SPACER));
	Rslot_set_fill(spacer, 1.0f);        /* all the height left over */

	Rid footer = Rwidget_add_child(column, Rwidget_create(R_WIDGET_HBOX));
	Rslot_set_padding(footer, 0.0f, 6.0f, 0.0f, 0.0f);

	g_status = Rwidget_add_child(footer, Rlabel_create("Nothing pressed yet."));
	Rslot_set_fill(g_status, 1.0f);
	Rslot_set_valign(g_status, R_VALIGN_CENTER);

	Rid clear = Rwidget_add_child(footer, Rbutton_create("Clear"));
	Rbutton_set_clicked_callback(clear, [](Rid button, void*) {
		(void)button;
		Rlabel_set_text(g_status, "Nothing pressed yet.");
	}, nullptr);

	if (tutorial::is_test(argc, argv)) {
		Rdraw_frame();

		/* The three buttons of the first row have the same width, left to right. */
		Rid child = Rwidget_get_child(equal_row, 0);
		float ax, ay, aw, ah;
		Rwidget_get_rect(child, &ax, &ay, &aw, &ah);
		for (int i = 1; i < 3; ++i) {
			float bx, by, bw, bh;
			Rwidget_get_rect(Rwidget_get_child(equal_row, i), &bx, &by, &bw, &bh);
			TUTORIAL_CHECK(std::fabs(bw - aw) < 1.0f);   /* same share */
			TUTORIAL_CHECK(bx > ax);                      /* placed to the right */
			ax = bx;
		}

		/* The footer sits below the row of buttons. */
		float fx, fy, fw, fh, rx, ry, rw, rh;
		Rwidget_get_rect(footer, &fx, &fy, &fw, &fh);
		Rwidget_get_rect(equal_row, &rx, &ry, &rw, &rh);
		TUTORIAL_CHECK(fy > ry + rh);

		/* A click reaches the callback through the slot the button was given. */
		Rid one = Rwidget_get_child(equal_row, 0);
		tutorial::click(window, one);
		TUTORIAL_CHECK(std::strstr(Rlabel_get_text(g_status), "One") != nullptr);

		return tutorial::finish("02-layout");
	}

	while (Rwait_frame(-1.0)) {
	}
	return tutorial::finish("02-layout");
}
