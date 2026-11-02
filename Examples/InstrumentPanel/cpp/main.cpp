/**
 * Raven example - instrument panel.
 *
 * A small process-control dashboard, the kind of screen a SCADA tool shows:
 *
 *   - an analog gauge with ticks, a needle and a coloured arc;
 *   - a digital meter with a recessed seven-segment-style readout;
 *   - indicator LEDs (power, link, alarm);
 *   - push-button switches (pump, valve, arm);
 *   - a real-time strip chart with two signals, a grid and a legend.
 *
 * Everything is drawn by the application in custom widgets (raven_draw.h):
 * Raven provides the layout, the theme, the input and the primitives, the
 * panel provides the look.
 *
 *   ./instrument_panel                 the dashboard
 *   ./instrument_panel --test          no window, no GPU: checks only
 *   ./instrument_panel --capture a.ppm  120 frames, saved as a PPM image
 *
 * Build: see CMakeLists.txt, or the README one folder above.
 */

#include <Raven.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static const float kPi = 3.14159265358979323846f;

/* ---------------------------------------------------------------------------
 *  State
 * ------------------------------------------------------------------------- */

static const int SAMPLES = 300;          /* ten seconds at 30 Hz */
static const double SAMPLE_INTERVAL = 1.0 / 30.0;

struct Panel {
	/* the process */
	bool running = true;
	bool pump = false;
	bool valve = false;
	bool armed = false;
	double time = 0.0;
	float pressure = 2.0f;               /* bar, gauge shows 0..100 */
	float flow = 0.0f;                   /* L/min, gauge shows 0..100 */

	/* history, as a ring buffer */
	float pressure_history[SAMPLES]{};
	float flow_history[SAMPLES]{};
	int sample_count = 0;
	int sample_head = 0;
	double last_sample = 0.0;

	/* widgets */
	Rid window = R_INVALID_ID;
	Rid status = R_INVALID_ID;
	Rid alarm_label = R_INVALID_ID;
	Rid pump_button = R_INVALID_ID;
	Rid valve_button = R_INVALID_ID;
	Rid arm_button = R_INVALID_ID;
	Rid pause_button = R_INVALID_ID;

	/* what the tests count */
	int meter_paints = 0;
	int digital_paints = 0;
	int chart_paints = 0;
	int led_paints = 0;
};

static Panel g;

struct Led {
	bool on = false;
	float r = 0.3f, g = 0.8f, b = 0.4f;
};

static Led g_power{true, 0.30f, 0.90f, 0.45f};
static Led g_link{true, 0.30f, 0.65f, 1.00f};
static Led g_alarm{false, 1.00f, 0.30f, 0.25f};

static bool alarm_active() {
	return g.armed && g.pressure > 75.0f;
}

/* ---------------------------------------------------------------------------
 *  Helpers
 * ------------------------------------------------------------------------- */

struct Color {
	float r, g, b, a;
};

static Color role(Rid widget, RColorRole which) {
	Color color{0, 0, 0, 0};
	Rwidget_get_color(widget, which, &color.r, &color.g, &color.b, &color.a);
	return color;
}

static float clamp01(float value) {
	return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

/* A rough advance width: Raven has no text measuring call here, and the panel
 * only needs it to centre a few short strings. */
static float text_width(const char* text, float size) {
	return (float)std::strlen(text) * size * 0.60f;
}

static void draw_text(Rid font, RPainter* painter, float size, float x, float y,
	const char* text, Color color) {
	Rdraw_text(painter, font, size, x, y, text, color.r, color.g, color.b, color.a);
}

static void draw_text_centered(Rid font, RPainter* painter, float size,
	float center_x, float y, const char* text, Color color) {
	draw_text(font, painter, size, center_x - text_width(text, size) * 0.5f, y, text, color);
}

/* ---------------------------------------------------------------------------
 *  Simulation
 *
 * A first-order lag on each signal, plus a little ripple, so the meters and the
 * chart move the way a real process does instead of jumping to their target.
 * ------------------------------------------------------------------------- */

static void simulate(double delta) {
	if (!g.running || delta <= 0.0) return;
	g.time += delta;

	const float t = (float)g.time;
	float pressure_target = g.pump ? 68.0f : 2.0f;
	if (g.pump) pressure_target += 2.5f * std::sin(t * 1.7f);

	float flow_target = g.valve ? (g.pump ? 44.0f : 12.0f) : 0.0f;
	if (g.valve) flow_target += 3.0f * std::sin(t * 3.1f);

	g.pressure += (pressure_target - g.pressure) * (1.0f - std::exp(-(float)delta / 0.6f));
	g.flow += (flow_target - g.flow) * (1.0f - std::exp(-(float)delta / 0.35f));
	if (g.pressure < 0.0f) g.pressure = 0.0f;
	if (g.flow < 0.0f) g.flow = 0.0f;

	if (g.time - g.last_sample >= SAMPLE_INTERVAL) {
		g.last_sample = g.time;
		g.pressure_history[g.sample_head] = g.pressure;
		g.flow_history[g.sample_head] = g.flow;
		g.sample_head = (g.sample_head + 1) % SAMPLES;
		if (g.sample_count < SAMPLES) g.sample_count++;
	}

	g_alarm.on = alarm_active();
}

static void update_status() {
	char text[160];
	std::snprintf(text, sizeof(text), "P-101  %5.1f bar      F-201  %5.1f L/min      %s",
		(double)g.pressure, (double)g.flow, g.running ? "running" : "paused");
	Rlabel_set_text(g.status, text);

	if (alarm_active()) {
		Rlabel_set_text(g.alarm_label, "HIGH PRESSURE");
		Rlabel_set_color(g.alarm_label, 1.0f, 0.35f, 0.30f, 1.0f);
	} else if (!g.armed) {
		Rlabel_set_text(g.alarm_label, "interlocks off");
		Rlabel_set_color(g.alarm_label, 0.75f, 0.75f, 0.75f, 0.8f);
	} else {
		Rlabel_set_text(g.alarm_label, "interlocks armed");
		Rlabel_set_color(g.alarm_label, 0.45f, 0.85f, 0.50f, 0.9f);
	}
}

/* ---------------------------------------------------------------------------
 *  Analog meter
 * ------------------------------------------------------------------------- */

static void paint_meter(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	Panel* panel = (Panel*)user_data;
	panel->meter_paints++;

	Color panel_color = role(widget, R_COLOR_PANEL);
	Color border = role(widget, R_COLOR_BORDER);
	Color text = role(widget, R_COLOR_TEXT);
	Color accent = role(widget, R_COLOR_ACCENT);
	Color danger = role(widget, R_COLOR_DANGER);
	Color track = role(widget, R_COLOR_TRACK);

	Rdraw_rect(painter, 0, 0, width, height, 14.0f, panel_color.r, panel_color.g, panel_color.b, 0.85f);
	Rdraw_rect_outline(painter, 0.5f, 0.5f, width - 1.0f, height - 1.0f, 14.0f, 1.0f,
		border.r, border.g, border.b, 0.7f);

	const float center_x = width * 0.5f;
	const float center_y = height * 0.58f;
	const float radius = (width < height ? width : height) * 0.36f;
	const float start = 2.35619449f;              /* 135 degrees: bottom left */
	const float sweep = 4.71238898f;              /* 270 degrees, through the top */

	/* the scale: eleven ticks, longer every fifth */
	for (int i = 0; i <= 10; i++) {
		const float angle = start + sweep * (float)i / 10.0f;
		const bool major = (i % 5) == 0;
		const float inner = radius * (major ? 0.80f : 0.88f);
		Rdraw_line(painter,
			center_x + std::cos(angle) * inner, center_y + std::sin(angle) * inner,
			center_x + std::cos(angle) * radius, center_y + std::sin(angle) * radius,
			major ? 2.0f : 1.0f, text.r, text.g, text.b, major ? 0.9f : 0.4f);
	}

	/* the track, then the value in the accent colour */
	const float value = clamp01(panel->pressure / 100.0f);
	for (int i = 0; i < 60; i++) {
		const float a0 = start + sweep * (float)i / 60.0f;
		const float a1 = start + sweep * (float)(i + 1) / 60.0f;
		Color color = ((float)(i + 1) / 60.0f <= value) ? accent : track;
		Rdraw_line(painter,
			center_x + std::cos(a0) * radius * 0.92f, center_y + std::sin(a0) * radius * 0.92f,
			center_x + std::cos(a1) * radius * 0.92f, center_y + std::sin(a1) * radius * 0.92f,
			4.0f, color.r, color.g, color.b, color.a * (color.r == track.r ? 0.5f : 0.9f));
	}

	/* the needle: red past the alarm point */
	const float angle = start + sweep * value;
	Color needle = panel->pressure > 75.0f ? danger : text;
	Rdraw_line(painter, center_x, center_y,
		center_x + std::cos(angle) * radius * 0.78f, center_y + std::sin(angle) * radius * 0.78f,
		3.0f, needle.r, needle.g, needle.b, 1.0f);
	Rdraw_circle(painter, center_x, center_y, 6.0f, needle.r, needle.g, needle.b, 1.0f);
	Rdraw_circle(painter, center_x, center_y, 2.5f, panel_color.r, panel_color.g, panel_color.b, 1.0f);

	/* the texts */
	char amount[32];
	std::snprintf(amount, sizeof(amount), "%.1f bar", (double)panel->pressure);
	draw_text(R_INVALID_ID, painter, 12.0f, 14.0f, 10.0f, "PRESSURE  P-101", text);
	draw_text_centered(Rfont_get_monospace(), painter, 26.0f, center_x, height - 40.0f, amount, accent);
}

/* ---------------------------------------------------------------------------
 *  Digital meter
 * ------------------------------------------------------------------------- */

static void paint_digital(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	Panel* panel = (Panel*)user_data;
	panel->digital_paints++;

	Color panel_color = role(widget, R_COLOR_PANEL);
	Color border = role(widget, R_COLOR_BORDER);
	Color text = role(widget, R_COLOR_TEXT);
	Color accent = role(widget, R_COLOR_ACCENT);

	Rdraw_rect(painter, 0, 0, width, height, 14.0f, panel_color.r, panel_color.g, panel_color.b, 0.85f);
	Rdraw_rect_outline(painter, 0.5f, 0.5f, width - 1.0f, height - 1.0f, 14.0f, 1.0f,
		border.r, border.g, border.b, 0.7f);

	/* recessed display */
	Rdraw_rect(painter, 14.0f, 34.0f, width - 28.0f, height - 62.0f, 8.0f, 0.05f, 0.07f, 0.09f, 0.85f);
	Rdraw_rect_outline(painter, 14.5f, 34.5f, width - 29.0f, height - 63.0f, 8.0f, 1.0f,
		border.r, border.g, border.b, 0.5f);

	char amount[32];
	std::snprintf(amount, sizeof(amount), "%5.1f", (double)panel->flow);
	const float size = height * 0.26f;
	const float x = width * 0.5f;
	draw_text_centered(Rfont_get_monospace(), painter, size, x, 34.0f + (height - 62.0f - size) * 0.45f,
		amount, accent);

	draw_text(R_INVALID_ID, painter, 12.0f, 14.0f, 10.0f, "FLOW  F-201", text);
	draw_text(R_INVALID_ID, painter, 12.0f, width - 58.0f, 10.0f, "L/min", text);

	/* a thin bar under the digits */
	Rdraw_rect(painter, 20.0f, height - 18.0f, width - 40.0f, 3.0f, 1.5f,
		border.r, border.g, border.b, 0.35f);
	Rdraw_rect(painter, 20.0f, height - 18.0f, (width - 40.0f) * clamp01(panel->flow / 100.0f), 3.0f,
		1.5f, accent.r, accent.g, accent.b, 0.9f);
}

/* ---------------------------------------------------------------------------
 *  LED
 * ------------------------------------------------------------------------- */

static void paint_led(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	Led* led = (Led*)user_data;
	g.led_paints++;

	const float center_x = width * 0.5f;
	const float center_y = height * 0.5f;
	const float radius = (width < height ? width : height) * 0.30f;

	/* the glow: a few translucent circles, brighter when lit */
	const float strength = led->on ? 1.0f : 0.0f;
	for (int i = 0; i < 3; i++) {
		const float r = radius * (1.4f + 0.35f * (float)i);
		Rdraw_circle(painter, center_x, center_y, r,
			led->r, led->g, led->b, (0.10f - 0.03f * (float)i) * strength);
	}

	/* the body */
	Rdraw_circle(painter, center_x, center_y, radius,
		led->r * (0.35f + 0.65f * strength), led->g * (0.35f + 0.65f * strength),
		led->b * (0.35f + 0.65f * strength), 0.95f);

	/* a highlight so it reads as a dome */
	Rdraw_circle(painter, center_x - radius * 0.25f, center_y - radius * 0.30f, radius * 0.28f,
		1.0f, 1.0f, 1.0f, 0.18f + 0.22f * strength);
}

/* ---------------------------------------------------------------------------
 *  Real-time strip chart
 * ------------------------------------------------------------------------- */

static void paint_chart(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	Panel* panel = (Panel*)user_data;
	panel->chart_paints++;

	Color panel_color = role(widget, R_COLOR_PANEL);
	Color border = role(widget, R_COLOR_BORDER);
	Color text = role(widget, R_COLOR_TEXT);
	Color accent = role(widget, R_COLOR_ACCENT);
	Color success = role(widget, R_COLOR_SUCCESS);

	Rdraw_rect(painter, 0, 0, width, height, 14.0f, panel_color.r, panel_color.g, panel_color.b, 0.85f);
	Rdraw_rect_outline(painter, 0.5f, 0.5f, width - 1.0f, height - 1.0f, 14.0f, 1.0f,
		border.r, border.g, border.b, 0.7f);

	const float left = 52.0f, right = 18.0f, top = 34.0f, bottom = 22.0f;
	const float plot_width = width - left - right;
	const float plot_height = height - top - bottom;

	/* title and legend */
	draw_text(R_INVALID_ID, painter, 12.0f, 14.0f, 10.0f, "TREND  10 s", text);
	Rdraw_rect(painter, width - 128.0f, 14.0f, 9.0f, 9.0f, 2.0f, accent.r, accent.g, accent.b, 0.9f);
	draw_text(R_INVALID_ID, painter, 11.0f, width - 114.0f, 11.0f, "pressure", text);
	Rdraw_rect(painter, width - 52.0f, 14.0f, 9.0f, 9.0f, 2.0f, success.r, success.g, success.b, 0.9f);
	draw_text(R_INVALID_ID, painter, 11.0f, width - 38.0f, 11.0f, "flow", text);

	/* grid and Y labels */
	for (int i = 0; i <= 4; i++) {
		const float y = top + plot_height * (float)i / 4.0f;
		Rdraw_line(painter, left, y, left + plot_width, y, 1.0f, border.r, border.g, border.b, i == 4 ? 0.6f : 0.22f);
		char label[8];
		std::snprintf(label, sizeof(label), "%d", 100 - 25 * i);
		draw_text(Rfont_get_monospace(), painter, 10.0f, 16.0f, y - 6.0f, label, text);
	}
	for (int i = 0; i <= 10; i++) {
		const float x = left + plot_width * (float)i / 10.0f;
		Rdraw_line(painter, x, top, x, top + plot_height, 1.0f, border.r, border.g, border.b, 0.12f);
	}

	if (panel->sample_count < 2) return;

	/* The newest sample sits at the right edge; while the buffer fills, the
	 * trace grows from the right, as on a real strip chart. */
	const float step_x = plot_width / (float)(SAMPLES - 1);
	float previous_pressure_x = 0, previous_pressure_y = 0;
	float previous_flow_x = 0, previous_flow_y = 0;

	for (int i = 0; i < panel->sample_count; i++) {
		const int index = (panel->sample_head - panel->sample_count + i + SAMPLES) % SAMPLES;
		const float x = left + plot_width - step_x * (float)(panel->sample_count - 1 - i);
		const float pressure_y = top + plot_height * (1.0f - clamp01(panel->pressure_history[index] / 100.0f));
		const float flow_y = top + plot_height * (1.0f - clamp01(panel->flow_history[index] / 100.0f));

		if (i > 0) {
			Rdraw_line(painter, previous_pressure_x, previous_pressure_y, x, pressure_y, 1.8f,
				accent.r, accent.g, accent.b, 0.95f);
			Rdraw_line(painter, previous_flow_x, previous_flow_y, x, flow_y, 1.8f,
				success.r, success.g, success.b, 0.95f);
		}
		previous_pressure_x = x;
		previous_pressure_y = pressure_y;
		previous_flow_x = x;
		previous_flow_y = flow_y;
	}

	/* the alarm threshold, when the interlocks are armed */
	if (panel->armed) {
		const float y = top + plot_height * (1.0f - 0.75f);
		Rdraw_line(painter, left, y, left + plot_width, y, 1.5f, 0.95f, 0.35f, 0.30f, 0.5f);
		draw_text(R_INVALID_ID, painter, 10.0f, left + 6.0f, y - 14.0f, "alarm 75", text);
	}
}

/* ---------------------------------------------------------------------------
 *  UI
 * ------------------------------------------------------------------------- */

static Rid tile() {
	Rid column = Rwidget_create(R_WIDGET_VBOX);
	Rpanel_set_spacing(column, 8.0f);
	Rslot_set_padding(column, 12.0f, 12.0f, 12.0f, 12.0f);
	return column;
}

static void sync_controls() {
	Rbutton_set_text(g.pump_button, g.pump ? "PUMP  ON" : "PUMP  OFF");
	Rbutton_set_style(g.pump_button, g.pump ? R_BUTTON_PRIMARY : R_BUTTON_DEFAULT);
	Rbutton_set_text(g.valve_button, g.valve ? "VALVE  OPEN" : "VALVE  SHUT");
	Rbutton_set_style(g.valve_button, g.valve ? R_BUTTON_PRIMARY : R_BUTTON_DEFAULT);
	Rbutton_set_text(g.arm_button, g.armed ? "INTERLOCKS  ARMED" : "INTERLOCKS  OFF");
	Rbutton_set_style(g.arm_button, g.armed ? R_BUTTON_PRIMARY : R_BUTTON_DEFAULT);
	Rbutton_set_text(g.pause_button, g.running ? "PAUSE" : "RESUME");
}

static void on_pump(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	g.pump = !g.pump;
	sync_controls();
}

static void on_valve(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	g.valve = !g.valve;
	sync_controls();
}

static void on_arm(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	g.armed = !g.armed;
	sync_controls();
	update_status();
}

static void on_pause(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	g.running = !g.running;
	sync_controls();
	update_status();
}

static Rid custom_panel(RPaintCallback paint, float min_width, float min_height, void* user_data) {
	Rid widget = Rwidget_create(R_WIDGET_CUSTOM);
	Rcustom_set_paint_callback(widget, paint, user_data);
	Rwidget_set_min_size(widget, min_width, min_height);
	Rwidget_set_hit_testable(widget, R_FALSE);
	return widget;
}

static Rid build_panel(Rid window) {
	Rid root = Rwidget_create(R_WIDGET_VBOX);
	Rpanel_set_spacing(root, 12.0f);
	Rslot_set_padding(root, 16.0f, 16.0f, 16.0f, 16.0f);
	Rwindow_set_content(window, root);

	/* --- the row of instruments ------------------------------------------- */
	Rid row = Rwidget_add_child(root, Rwidget_create(R_WIDGET_HBOX));
	Rpanel_set_spacing(row, 12.0f);

	Rid gauge_tile = Rwidget_add_child(row, tile());
	Rslot_set_fill(gauge_tile, 1.0f);
	Rid gauge = Rwidget_add_child(gauge_tile, custom_panel(paint_meter, 240.0f, 240.0f, &g));
	Rslot_set_fill(gauge, 1.0f);

	Rid flow_tile = Rwidget_add_child(row, tile());
	Rslot_set_fill(flow_tile, 1.0f);
	Rid flow = Rwidget_add_child(flow_tile, custom_panel(paint_digital, 240.0f, 150.0f, &g));
	Rslot_set_fill(flow, 1.0f);

	Rid control_tile = Rwidget_add_child(row, tile());
	Rslot_set_fill(control_tile, 1.0f);

	Rid leds = Rwidget_add_child(control_tile, Rwidget_create(R_WIDGET_HBOX));
	Rpanel_set_spacing(leds, 10.0f);
	Rslot_set_halign(leds, R_HALIGN_CENTER);

	struct { Led* led; const char* name; } lamps[] = {
		{&g_power, "POWER"}, {&g_link, "LINK"}, {&g_alarm, "ALARM"},
	};
	for (auto& lamp : lamps) {
		Rid column = Rwidget_add_child(leds, Rwidget_create(R_WIDGET_VBOX));
		Rpanel_set_spacing(column, 2.0f);
		Rid led = Rwidget_add_child(column, custom_panel(paint_led, 46.0f, 46.0f, lamp.led));
		Rwidget_set_size(led, 46.0f, 46.0f);
		Rid name = Rwidget_add_child(column, Rlabel_create(lamp.name));
		Rlabel_set_text_size(name, 10.0f);
		Rslot_set_halign(name, R_HALIGN_CENTER);
	}

	Rid alarm = Rwidget_add_child(control_tile, Rlabel_create("interlocks off"));
	Rlabel_set_text_size(alarm, 12.0f);
	Rslot_set_halign(alarm, R_HALIGN_CENTER);
	g.alarm_label = alarm;

	g.pump_button = Rwidget_add_child(control_tile, Rbutton_create("PUMP  OFF"));
	Rbutton_set_clicked_callback(g.pump_button, on_pump, nullptr);
	g.valve_button = Rwidget_add_child(control_tile, Rbutton_create("VALVE  SHUT"));
	Rbutton_set_clicked_callback(g.valve_button, on_valve, nullptr);
	g.arm_button = Rwidget_add_child(control_tile, Rbutton_create("INTERLOCKS  OFF"));
	Rbutton_set_clicked_callback(g.arm_button, on_arm, nullptr);

	Rid spacer = Rwidget_add_child(control_tile, Rwidget_create(R_WIDGET_SPACER));
	Rslot_set_fill(spacer, 1.0f);

	/* --- the chart --------------------------------------------------------- */
	Rid chart_tile = Rwidget_add_child(root, tile());
	Rslot_set_fill(chart_tile, 1.0f);
	Rid chart = Rwidget_add_child(chart_tile, custom_panel(paint_chart, 480.0f, 180.0f, &g));
	Rslot_set_fill(chart, 1.0f);

	/* --- the status bar ---------------------------------------------------- */
	Rid bar = Rwidget_add_child(root, Rwidget_create(R_WIDGET_HBOX));
	Rpanel_set_spacing(bar, 12.0f);
	g.status = Rwidget_add_child(bar, Rlabel_create("starting"));
	Rslot_set_fill(g.status, 1.0f);
	Rslot_set_valign(g.status, R_VALIGN_CENTER);

	g.pause_button = Rwidget_add_child(bar, Rbutton_create("PAUSE"));
	Rbutton_set_clicked_callback(g.pause_button, on_pause, nullptr);

	sync_controls();
	return root;
}

/* ---------------------------------------------------------------------------
 *  Frames
 * ------------------------------------------------------------------------- */

static void on_frame(double delta, void* user_data) {
	(void)user_data;
	simulate(delta);
	update_status();
	Rwindow_request_redraw(g.window);
}

/* ---------------------------------------------------------------------------
 *  Start
 * ------------------------------------------------------------------------- */

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
	Rtheme_apply(R_THEME_DARK);
	g.window = Rwindow_create("Instrument panel", 1180, 760, R_WINDOW_DEFAULT);
	build_panel(g.window);
	update_status();
	Rdraw_frame();

	/* the panel painted itself */
	if (g.meter_paints == 0 || g.digital_paints == 0 || g.chart_paints == 0 || g.led_paints == 0) {
		std::printf("  FAIL: a custom widget was never painted\n");
		return 1;
	}

	/* the pump raises the pressure, the valve lets the flow through */
	const float pressure_at_rest = g.pressure;
	g.pump = true;
	for (int i = 0; i < 180; i++) simulate(1.0 / 60.0);
	if (!(g.pressure > pressure_at_rest + 20.0f)) {
		std::printf("  FAIL: the pump did not raise the pressure (%.1f)\n", (double)g.pressure);
		return 1;
	}

	g.valve = true;
	for (int i = 0; i < 120; i++) simulate(1.0 / 60.0);
	if (!(g.flow > 20.0f)) {
		std::printf("  FAIL: the valve did not let the flow through (%.1f)\n", (double)g.flow);
		return 1;
	}

	/* the chart filled, newest sample last */
	if (g.sample_count < 20) {
		std::printf("  FAIL: the chart has %d samples\n", g.sample_count);
		return 1;
	}

	/* the interlock lights the alarm lamp above 75 bar */
	g.armed = true;
	g.pressure = 80.0f;
	simulate(1.0 / 60.0);
	if (!alarm_active() || !g_alarm.on) {
		std::printf("  FAIL: the alarm did not trip\n");
		return 1;
	}

	/* a push button toggles its switch and its own look */
	g.pressure = 2.0f;
	Rdraw_frame();
	const float x = 0, y = 0;
	(void)x;
	(void)y;
	{
		Rid button = g.pump_button;
		float bx, by, bw, bh;
		Rwidget_get_rect(button, &bx, &by, &bw, &bh);
		const bool before = g.pump;
		Rwindow_inject_mouse_move(g.window, bx + bw * 0.5f, by + bh * 0.5f);
		Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_TRUE);
		Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_FALSE);
		if (g.pump == before) {
			std::printf("  FAIL: the pump button did not toggle\n");
			return 1;
		}
	}

	/* pause stops the simulation */
	g.running = false;
	const double frozen = g.time;
	simulate(1.0);
	if (g.time != frozen) {
		std::printf("  FAIL: the simulation ran while paused\n");
		return 1;
	}

	std::printf("instrument panel: simulated %.1f s, %d samples, %d meter paints, %d chart paints\n",
		g.time, g.sample_count, g.meter_paints, g.chart_paints);
	Rshutdown();
	std::printf("PASS\n");
	return 0;
}

static int run_capture(const char* path, int frames) {
	Rtheme_apply(R_THEME_DARK);
	g.window = Rwindow_create("Instrument panel", 1180, 760, R_WINDOW_DEFAULT);
	build_panel(g.window);
	g.pump = true;
	g.valve = true;
	g.armed = true;
	sync_controls();
	update_status();
	/* a fixed step, so a capture always shows the same three seconds */
	for (int i = 0; i < frames; i++) {
		simulate(1.0 / 60.0);
		update_status();
		Rwindow_request_redraw(g.window);
		Rwait_frame(0.0);
	}
	const bool saved = save_ppm(g.window, path);
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

	if (test) {
		if (!init(true)) return 1;
		return run_tests();
	}

	if (!init(false)) return 1;
	if (capture) return run_capture(capture, frames);

	Rtheme_apply(R_THEME_DARK);
	g.window = Rwindow_create("Instrument panel", 1180, 760, R_WINDOW_DEFAULT);
	build_panel(g.window);
	update_status();
	Rset_frame_callback(on_frame, nullptr);
	while (Rwait_frame(-1.0)) {
	}
	Rshutdown();
	return 0;
}
