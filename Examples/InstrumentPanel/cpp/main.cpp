/**
 * Raven example - instrument panel (standalone).
 *
 * The instruments themselves live in panel.h / panel.cpp, so the same pane can
 * be embedded in an application: see the Raven Workbench
 * (https://github.com/skp64-pvt01/Raven-workbench), where it opens and closes
 * from a menu, the toolbar and a side bar.
 *
 *   ./instrument_panel                    the dashboard
 *   ./instrument_panel --test             no window, no GPU: checks only
 *   ./instrument_panel --capture a.ppm    180 frames, saved as a PPM image
 */

#include <Raven.h>

#include "panel.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static Panel* g_panel = nullptr;
static Rid g_window = R_INVALID_ID;
static Rid g_status = R_INVALID_ID;
static Rid g_pause = R_INVALID_ID;

static void update_status() {
	char text[200];
	std::snprintf(text, sizeof(text), "P-101  %5.1f bar      F-201  %5.1f L/min      %s%s",
		(double)panel_pressure(g_panel), (double)panel_flow(g_panel),
		panel_running(g_panel) ? "running" : "paused",
		panel_alarm(g_panel) ? "      HIGH PRESSURE" : "");
	Rlabel_set_text(g_status, text);
}

static void on_pause(Rid button, void* user_data) {
	(void)user_data;
	panel_set_running(g_panel, !panel_running(g_panel));
	Rbutton_set_text(button, panel_running(g_panel) ? "PAUSE" : "RESUME");
	update_status();
}

static void on_frame(double delta, void* user_data) {
	(void)user_data;
	panel_simulate(g_panel, delta);
	panel_refresh(g_panel);
	update_status();
	Rwindow_request_redraw(g_window);
}

static void build_window() {
	Rtheme_apply(R_THEME_DARK);
	g_window = Rwindow_create("Instrument panel", 1180, 760, R_WINDOW_DEFAULT);

	Rid root = Rwidget_create(R_WIDGET_VBOX);
	Rpanel_set_spacing(root, 12.0f);
	Rslot_set_padding(root, 16.0f, 16.0f, 16.0f, 16.0f);
	Rwindow_set_content(g_window, root);

	Rid pane = panel_build(g_panel);
	Rwidget_add_child(root, pane);
	Rslot_set_fill(pane, 1.0f);

	Rid bar = Rwidget_add_child(root, Rwidget_create(R_WIDGET_HBOX));
	Rpanel_set_spacing(bar, 12.0f);
	g_status = Rwidget_add_child(bar, Rlabel_create("starting"));
	Rslot_set_fill(g_status, 1.0f);
	Rslot_set_valign(g_status, R_VALIGN_CENTER);

	g_pause = Rwidget_add_child(bar, Rbutton_create("PAUSE"));
	Rbutton_set_clicked_callback(g_pause, on_pause, nullptr);

	panel_refresh(g_panel);
	update_status();
}

/** Moves the pointer to the middle of a widget and clicks it. */
static void click(Rid widget) {
	float x = 0, y = 0, width = 0, height = 0;
	Rwidget_get_rect(widget, &x, &y, &width, &height);
	Rwindow_inject_mouse_move(g_window, x + width * 0.5f, y + height * 0.5f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
}

static bool init(bool headless) {
	RInitDesc desc{};
	desc.backend = headless ? R_BACKEND_NULL : R_BACKEND_VULKAN;
	desc.headless = headless ? R_TRUE : R_FALSE;
	desc.app_name = "Raven instrument panel";
	if (!Rinit_ex(&desc)) {
		std::fprintf(stderr, "cannot start Raven (is a Vulkan driver installed?)\n");
		return false;
	}
	return true;
}

static bool save_ppm(Rid window, const char* path) {
	int width = 0, height = 0;
	uint8_t* pixels = Rwindow_capture(window, &width, &height);
	FILE* file = pixels ? std::fopen(path, "wb") : nullptr;
	if (!file) {
		Rpixels_free(pixels);
		return false;
	}
	std::fprintf(file, "P6\n%d %d\n255\n", width, height);
	for (int i = 0; i < width * height; i++) std::fwrite(pixels + i * 4, 1, 3, file);
	std::fclose(file);
	Rpixels_free(pixels);
	return true;
}

static int run_tests() {
	build_window();
	Rdraw_frame();

	if (!panel_paints(g_panel, PANEL_PAINT_METER) || !panel_paints(g_panel, PANEL_PAINT_DIGITAL) ||
		!panel_paints(g_panel, PANEL_PAINT_CHART) || !panel_paints(g_panel, PANEL_PAINT_LED)) {
		std::printf("  FAIL: a custom widget was never painted\n");
		return 1;
	}

	/* the pump raises the pressure */
	const float pressure_at_rest = panel_pressure(g_panel);
	click(panel_button(g_panel, PANEL_PUMP));
	for (int i = 0; i < 180; i++) panel_simulate(g_panel, 1.0 / 60.0);
	if (!(panel_pressure(g_panel) > pressure_at_rest + 20.0f)) {
		std::printf("  FAIL: the pump did not raise the pressure (%.1f)\n", (double)panel_pressure(g_panel));
		return 1;
	}

	/* the valve lets the flow through */
	click(panel_button(g_panel, PANEL_VALVE));
	for (int i = 0; i < 120; i++) panel_simulate(g_panel, 1.0 / 60.0);
	if (!(panel_flow(g_panel) > 20.0f)) {
		std::printf("  FAIL: the valve did not let the flow through (%.1f)\n", (double)panel_flow(g_panel));
		return 1;
	}

	if (panel_samples(g_panel) < 20) {
		std::printf("  FAIL: the chart has %d samples\n", panel_samples(g_panel));
		return 1;
	}

	/* reset empties the plant and the trend */
	panel_reset(g_panel);
	if (panel_samples(g_panel) != 0 || panel_pressure(g_panel) > 5.0f || panel_flow(g_panel) > 1.0f) {
		std::printf("  FAIL: the reset did not clear the plant\n");
		return 1;
	}

	/* a paused panel does not advance */
	panel_set_running(g_panel, false);
	panel_simulate(g_panel, 1.0);
	if (panel_samples(g_panel) != 0) {
		std::printf("  FAIL: the simulation ran while paused\n");
		return 1;
	}

	std::printf("instrument panel: %d meter paints, %d chart paints, plant reset\n",
		panel_paints(g_panel, PANEL_PAINT_METER), panel_paints(g_panel, PANEL_PAINT_CHART));
	Rshutdown();
	std::printf("PASS\n");
	return 0;
}

static int run_capture(const char* path, int frames) {
	build_window();
	Rdraw_frame();  /* lays the panel out, so its buttons have rects */

	click(panel_button(g_panel, PANEL_PUMP));
	click(panel_button(g_panel, PANEL_VALVE));
	click(panel_button(g_panel, PANEL_INTERLOCKS));

	for (int i = 0; i < frames; i++) {
		panel_simulate(g_panel, 1.0 / 60.0);
		panel_refresh(g_panel);
		update_status();
		Rwindow_request_redraw(g_window);
		Rwait_frame(0.0);
	}
	const bool saved = save_ppm(g_window, path);
	Rshutdown();
	if (!saved) {
		std::fprintf(stderr, "cannot write %s\n", path);
		return 1;
	}
	std::printf("saved %s\n", path);
	return 0;
}

int main(int argc, char** argv) {
	const char* capture = nullptr;
	int frames = 180;
	bool test = false;
	for (int i = 1; i < argc; i++) {
		if (std::strcmp(argv[i], "--test") == 0) test = true;
		else if (std::strcmp(argv[i], "--capture") == 0 && i + 1 < argc) capture = argv[++i];
		else if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) frames = std::atoi(argv[++i]);
	}

	if (!init(test)) return 1;
	g_panel = panel_create();

	int code = 0;
	if (test) {
		code = run_tests();
	} else if (capture) {
		code = run_capture(capture, frames);
	} else {
		build_window();
		Rset_frame_callback(on_frame, nullptr);
		while (Rwait_frame(-1.0)) {
		}
		Rshutdown();
	}

	panel_destroy(g_panel);
	return code;
}
