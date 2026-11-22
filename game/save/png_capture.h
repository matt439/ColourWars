#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace labrador
{
	class Renderer;
}

// 8-bit RGBA, top row first, as Renderer::read_back_buffer returns it.
struct ScreenCapture
{
	int width = 0;
	int height = 0;
	std::vector<unsigned char> rgba;
};

// Draws and reads one unpresented frame. The next normal render starts
// a fresh frame; this must run from update, outside any draw already in flight.
// Throws what the renderer throws (the null backend refuses readback).
ScreenCapture capture_frame(labrador::Renderer& renderer,
	const std::function<void(labrador::Renderer&)>& draw);

enum class PngWriteMode { replace, new_file };

// Encodes an opaque PNG through WIC on an already initialized COM thread.
// Writes the encoded bytes whole, leaving any previous file intact on failure.
bool write_png(const std::filesystem::path& path, const ScreenCapture& image,
	std::string& error, PngWriteMode mode = PngWriteMode::replace);

// Local time as YYYYMMDD-HHMMSS-mmm.
std::string capture_file_timestamp(std::chrono::system_clock::time_point now);
