#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/script/debug_file_name.h"
#include "game/debug/capture/debug_screenshot.h"
#include "engine/app/application.h"
#include "engine/render/renderer.h"
#include "game/save/atomic_file.h"
#include <cctype>
#include <cstdio>
#include <ctime>
#include <limits>
#include <objbase.h>
#include <wincodec.h>
#include <wrl/client.h>

// Debug-only, so Release links no imaging library.
#pragma comment(lib, "windowscodecs.lib")

using Microsoft::WRL::ComPtr;

ScreenCapture capture_stack(labrador::Application& application)
{
	labrador::Renderer& renderer = *application.renderer();
	ScreenCapture image;
	renderer.begin_frame();
	application.draw(renderer);
	renderer.submit();
	const labrador::Vector2F size = renderer.back_buffer_size();
	image.width = static_cast<int>(size.x);
	image.height = static_cast<int>(size.y);
	renderer.read_back_buffer(image.rgba);
	return image;
}

bool write_png(const std::filesystem::path& path, const ScreenCapture& image,
	std::string& error)
{
	const std::size_t expected = static_cast<std::size_t>(image.width) *
		static_cast<std::size_t>(image.height) * 4u;
	if (image.width <= 0 || image.height <= 0 || image.rgba.size() != expected)
	{
		error = "the capture is empty or the wrong size";
		return false;
	}

	// RGBA to BGR, the order WIC's PNG encoder takes without conversion.
	const UINT stride = static_cast<UINT>(image.width) * 3u;
	std::vector<unsigned char> bgr(static_cast<std::size_t>(stride) *
		static_cast<std::size_t>(image.height));
	for (std::size_t pixel = 0, count = expected / 4u; pixel < count; ++pixel)
	{
		bgr[pixel * 3u + 0u] = image.rgba[pixel * 4u + 2u];
		bgr[pixel * 3u + 1u] = image.rgba[pixel * 4u + 1u];
		bgr[pixel * 3u + 2u] = image.rgba[pixel * 4u + 0u];
	}

	const auto failed = [&error](const char* step, HRESULT result)
		{
			char buffer[96] = {};
			std::snprintf(buffer, sizeof(buffer), "%s failed (HRESULT 0x%08lX)", step,
				static_cast<unsigned long>(result));
			error = buffer;
			return false;
		};

	ComPtr<IWICImagingFactory> factory;
	HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.GetAddressOf()));
	if (FAILED(result)) { return failed("creating the WIC factory", result); }

	// Encoded in memory and written whole, so a failure never leaves a partial
	// PNG (or truncates an earlier one of the same name).
	ComPtr<IStream> stream;
	result = CreateStreamOnHGlobal(nullptr, TRUE, stream.GetAddressOf());
	if (FAILED(result)) { return failed("creating a stream", result); }

	ComPtr<IWICBitmapEncoder> encoder;
	result = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.GetAddressOf());
	if (FAILED(result)) { return failed("creating the PNG encoder", result); }
	result = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
	if (FAILED(result)) { return failed("starting the encoder", result); }

	ComPtr<IWICBitmapFrameEncode> frame;
	result = encoder->CreateNewFrame(frame.GetAddressOf(), nullptr);
	if (FAILED(result)) { return failed("creating the frame", result); }
	result = frame->Initialize(nullptr);
	if (FAILED(result)) { return failed("starting the frame", result); }
	result = frame->SetSize(static_cast<UINT>(image.width), static_cast<UINT>(image.height));
	if (FAILED(result)) { return failed("sizing the frame", result); }
	WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
	result = frame->SetPixelFormat(&format);
	if (FAILED(result) || format != GUID_WICPixelFormat24bppBGR)
	{
		return failed("choosing 24-bit BGR", result);
	}
	result = frame->WritePixels(static_cast<UINT>(image.height), stride,
		static_cast<UINT>(bgr.size()), bgr.data());
	if (FAILED(result)) { return failed("writing the pixels", result); }
	result = frame->Commit();
	if (FAILED(result)) { return failed("committing the frame", result); }
	result = encoder->Commit();
	if (FAILED(result)) { return failed("committing the file", result); }

	STATSTG status{};
	result = stream->Stat(&status, STATFLAG_NONAME);
	if (FAILED(result)) { return failed("measuring the PNG", result); }
	if (status.cbSize.QuadPart > (std::numeric_limits<ULONG>::max)())
	{
		error = "the PNG is too large";
		return false;
	}
	std::string png(static_cast<std::size_t>(status.cbSize.QuadPart), '\0');
	result = stream->Seek(LARGE_INTEGER{}, STREAM_SEEK_SET, nullptr);
	if (FAILED(result)) { return failed("rewinding the PNG", result); }
	ULONG read = 0;
	result = stream->Read(png.data(), static_cast<ULONG>(png.size()), &read);
	if (FAILED(result) || read != png.size()) { return failed("reading the PNG back", result); }
	if (!replace_file_atomically(path, png))
	{
		error = "writing the file failed";
		return false;
	}
	return true;
}

std::string screenshot_file_name(std::string_view requested,
	std::chrono::system_clock::time_point now)
{
	std::string name(requested);
	if (name.size() > 4)
	{
		std::string tail = name.substr(name.size() - 4);
		for (char& c : tail)
		{
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		if (tail == ".png")
		{
			name.resize(name.size() - 4);
		}
	}
	for (char& c : name)
	{
		c = debug_file_name::stem_character(c) ? c : '_';
	}
	if (name.empty())
	{
		const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
		const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch()).count() % 1000;
		std::tm local{};
		localtime_s(&local, &seconds);
		char buffer[64] = {};
		std::snprintf(buffer, sizeof(buffer), "colourwars-%04d%02d%02d-%02d%02d%02d-%03d",
			local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
			local.tm_hour, local.tm_min, local.tm_sec, static_cast<int>(milliseconds));
		name = buffer;
	}
	if (debug_file_name::reserved_stem(name))
	{
		name.insert(name.begin(), '_');
	}
	return name + ".png";
}
#endif
