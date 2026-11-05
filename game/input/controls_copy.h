#pragma once

#include <string>

// presentation.json's menu_text.controls: the in-match controls reminder's
// copy and the words among the control labels. MenuText reads and embeds it,
// so a locale reaches it; controls_text formats with it and never names
// Presentation. Key letters A-Z and the pad legends A/B/X/Y/LB/RB/LT/RT/LS/RS
// stay in code as legends, as digits do, and so does the " / " between a
// primary control and its alternate. Save tokens are input_bindings.h's.
struct ControlLabels
{
	std::wstring space, escape, none, start;
	std::wstring mouse_left, mouse_right, mouse_middle, mouse_x1, mouse_x2;
};

struct ControlsCopy
{
	// The reminder cards' headings.
	std::wstring keyboard_heading, gamepad_heading;
	// {left} {right} {drop} {jump} {shoot} {pause}, each exactly once.
	std::wstring keyboard_reminder;
	// {pad_jump} {pad_shoot} {pad_pause}, each exactly once: the pads' shared choices.
	std::wstring gamepad_reminder;
	// {pads} exactly once: one gamepad_pad_line per pad, LF between them,
	// when the pads in the match have different profiles.
	std::wstring gamepad_per_pad_reminder;
	// Single-line; {pad} (the slot's number), {pad_jump}, {pad_shoot} and
	// {pad_pause}, each exactly once.
	std::wstring gamepad_pad_line;
	// What the Controls help's {pad_*} tokens say when the pads' profiles differ.
	std::wstring per_pad;
	ControlLabels labels;
};
