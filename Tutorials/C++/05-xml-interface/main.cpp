/**
 * Raven tutorial 05 - an interface written in XML.
 *
 * The window of this program exists only as window.xml. The code:
 *
 *   Rwindow_create_from_xml_file   builds it
 *   Rwidget_find(window, "name")   finds a widget by its Name attribute
 *   Raction_bind("app.greet", ...)  answers OnClicked="app.greet"
 *
 * The split matters once an interface has more than a dozen widgets: the XML
 * is edited by hand or in the designer, the code stays readable.
 *
 * Build and test: see README.md next to this file.
 */

#include <Raven.h>

#include <cstdio>
#include <cstring>

#include "../common/tutorial.h"

#ifndef RAVEN_TUTORIAL_DIR
#define RAVEN_TUTORIAL_DIR "."
#endif

static Rid g_name = R_INVALID_ID;
static Rid g_greeting = R_INVALID_ID;

/* Fired by the button, and by anything else that uses the same action name. */
static void on_action(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	if (std::strcmp(action, "app.greet") != 0) return;

	const char* typed = Rtext_input_get_text(g_name);
	char text[256];
	std::snprintf(text, sizeof(text), "Hello, %s!", typed && typed[0] ? typed : "world");
	Rlabel_set_text(g_greeting, text);
}

int main(int argc, char** argv) {
	if (!tutorial::init(argc, argv, "Raven tutorial 05")) return 1;

	Raction_bind("app.greet", on_action, nullptr);
	/* Uncomment to keep the XML loaded again every time it is saved:
	 * Rxml_set_live_reload(R_TRUE); */

	Rid window = Rwindow_create_from_xml_file(RAVEN_TUTORIAL_DIR "/window.xml");
	if (window == R_INVALID_ID) {
		std::printf("cannot read window.xml\n");
		return tutorial::finish("05-xml-interface");
	}

	/* Name="name" and Name="greeting" in the XML. */
	g_name = Rwidget_find(window, "name");
	g_greeting = Rwidget_find(window, "greeting");
	TUTORIAL_CHECK(Rwidget_is_valid(g_name));
	TUTORIAL_CHECK(Rwidget_is_valid(g_greeting));
	Rid greet = Rwidget_find(window, "greet");

	if (tutorial::is_test(argc, argv)) {
		Rdraw_frame();

		tutorial::click(window, g_name);
		Rwindow_inject_text(window, "Grace");
		tutorial::click(window, greet);
		TUTORIAL_CHECK(std::strcmp(Rlabel_get_text(g_greeting), "Hello, Grace!") == 0);

		/* The action is what the XML names, so it can be fired without the button. */
		Rtext_input_set_text(g_name, "");
		Raction_fire("app.greet", R_INVALID_ID);
		TUTORIAL_CHECK(std::strcmp(Rlabel_get_text(g_greeting), "Hello, world!") == 0);

		return tutorial::finish("05-xml-interface");
	}

	while (Rwait_frame(-1.0)) {
	}
	return tutorial::finish("05-xml-interface");
}
