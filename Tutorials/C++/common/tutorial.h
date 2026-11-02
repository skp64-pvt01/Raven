/**
 * Raven tutorials - helpers shared by every step.
 *
 *   tutorial::init        starts Raven: a real window normally, headless with --test
 *   tutorial::is_test     R_TRUE when the program was run with --test
 *   TUTORIAL_CHECK(...)   one assertion; the count is printed at the end
 *   tutorial::click/key   synthetic input, so a test drives the real widgets
 *   tutorial::finish      Rshutdown, prints PASS or FAIL, returns the exit code
 *
 * Every tutorial ends with:
 *
 *     ./01-first-window            a window on the screen
 *     ./01-first-window --test     no window, no GPU, checks only
 *
 * The same two lines are what this course uses to prove each step still works.
 */

#ifndef RAVEN_TUTORIAL_H
#define RAVEN_TUTORIAL_H

#include <Raven.h>

#include <cstdio>
#include <cstring>

namespace tutorial {

inline int checks = 0;
inline int failures = 0;

inline void expect(bool ok, const char* expression, int line) {
	++checks;
	if (!ok) {
		++failures;
		std::printf("  FAIL line %d: %s\n", line, expression);
	}
}

/** Reports the errors of the library instead of losing them. */
inline void on_error(RError code, RSeverity severity, const char* detail) {
	std::printf("  [Raven] %s (%s): %s\n", Rerror_to_string(code), Rseverity_to_string(severity), detail);
}

inline bool is_test(int argc, char** argv) {
	return argc > 1 && std::strcmp(argv[1], "--test") == 0;
}

/**
 * Starts the library. In a normal run Raven opens a window with the Vulkan
 * backend; with --test the null backend and the null platform are used, so the
 * program needs no display and no GPU.
 */
inline bool init(int argc, char** argv, const char* app_name) {
	RInitDesc desc{};
	desc.app_name = app_name;
	desc.backend = R_BACKEND_VULKAN;
	if (is_test(argc, argv)) {
		desc.backend = R_BACKEND_NULL;
		desc.headless = R_TRUE;
	}
	Rerror_set_callback(on_error);
	if (!Rinit_ex(&desc)) {
		std::printf("%s: cannot start Raven (is a Vulkan driver installed?)\n", app_name);
		return false;
	}
	/* Tests read the state right after an input: no animation in between. */
	if (is_test(argc, argv)) Rset_animations_enabled(R_FALSE);
	return true;
}

/** Moves the pointer to the middle of a widget and clicks it. */
inline void click(Rid window, Rid widget) {
	float x = 0, y = 0, w = 0, h = 0;
	Rwidget_get_rect(widget, &x, &y, &w, &h);
	Rwindow_inject_mouse_move(window, x + w * 0.5f, y + h * 0.5f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
}

/** Clicks a point of the window, in logical units. */
inline void click_at(Rid window, float x, float y) {
	Rwindow_inject_mouse_move(window, x, y);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
}

/** Presses and releases a key, with modifiers (R_MOD_CTRL, R_MOD_SHIFT...). */
inline void key(Rid window, RKey k, Ruint modifiers = 0) {
	Rwindow_inject_key(window, k, R_TRUE, modifiers);
	Rwindow_inject_key(window, k, R_FALSE, modifiers);
}

/** Stops the library, prints the result, and returns the process exit code. */
inline int finish(const char* name) {
	Rshutdown();
	if (checks > 0) {
		std::printf("%s: %d checks, %d failures\n", name, checks, failures);
		std::printf("%s\n", failures == 0 ? "PASS" : "FAIL");
	}
	return failures == 0 ? 0 : 1;
}

}  // namespace tutorial

#define TUTORIAL_CHECK(expression) ::tutorial::expect((expression), #expression, __LINE__)

#endif  // RAVEN_TUTORIAL_H
