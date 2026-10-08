#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace labrador
{
	class Application;
}

// The developer `screenshot` command's parts. Debug-only bodies.

// 8-bit RGBA, top row first, as Renderer::read_back_buffer returns it.
struct ScreenCapture
{
	int width = 0;
	int height = 0;
	std::vector<unsigned char> rgba;
};

// Draws the whole state stack into an unpresented frame and reads it back:
// begin_frame, Application::draw, submit, read_back_buffer, never end_frame.
// The engine allows readback only between submit and end_frame, and
// Application::render runs those back to back, so this is the frame the
// render probes read; the next Application::render begins a fresh frame.
// Engine report 10 asks for a capture of the presented frame instead.
// Call it from a state's update, never from inside a draw. Throws what the
// renderer throws (the null backend refuses readback).
ScreenCapture capture_stack(labrador::Application& application);

// A PNG through WIC, 24-bit (the back buffer's alpha means nothing). COM
// must already be initialised on this thread (Application does it).
bool write_png(const std::filesystem::path& path, const ScreenCapture& image,
	std::string& error);

// `requested` kept to [A-Za-z0-9_-] (anything else becomes '_', a trailing
// ".png" is dropped), plus ".png"; empty requests the local-time default
// colourwars-YYYYMMDD-HHMMSS-mmm.png.
std::string screenshot_file_name(std::string_view requested,
	std::chrono::system_clock::time_point now);
