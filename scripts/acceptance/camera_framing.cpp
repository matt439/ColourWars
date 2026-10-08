#include "game/pch.h"
#include "game/objects/player_camera.h"
#include "game/presentation.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/viewport_manager.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace labrador;
using namespace mattmath;

namespace
{
	void require(bool condition, const char* message)
	{
		if (!condition)
		{
			throw std::runtime_error(message);
		}
	}

	bool close_to(float actual, float expected)
	{
		return std::abs(actual - expected) < 0.002f;
	}

	bool close_to(const Vector2F& actual, const Vector2F& expected)
	{
		return close_to(actual.x, expected.x) && close_to(actual.y, expected.y);
	}

	struct LayoutCase
	{
		ScreenLayout layout;
		int players;
		Vector2F world_size;
	};

	void check_resolution_independence(const CameraPresentation& framing)
	{
		const std::array<LayoutCase, 4> layouts = {{
			{ ScreenLayout::one_player, 1, { 1920.0f, 1080.0f } },
			{ ScreenLayout::two_player, 2, { 1920.0f, 540.0f } },
			{ ScreenLayout::three_player, 3, { 960.0f, 540.0f } },
			{ ScreenLayout::four_player, 4, { 960.0f, 540.0f } }
		}};
		const std::array<Vector2I, 3> resolutions = {{
			{ 1280, 720 }, { 1920, 1080 }, { 2560, 1440 }
		}};
		const RectangleF bounds(-120.0f, 80.0f, 6000.0f, 3600.0f);
		const std::array<Vector2F, 6> route = {{
			{ 300.0f, 600.0f }, { 2700.0f, 1800.0f },
			{ 3500.0f, 1700.0f }, { 5700.0f, 3500.0f },
			{ 0.0f, 200.0f }, { 2700.0f, 1800.0f }
		}};
		const PlayerCamera follow{};
		ResolutionManager resolution;
		ViewportManager panes(&resolution);

		for (const LayoutCase& layout : layouts)
		{
			panes.set_layout(layout.layout);
			for (int view = 0; view < layout.players; ++view)
			{
				std::array<Camera, 6> reference;
				resolution.set_resolution_exactly({ 1920, 1080 });
				Camera previous;
				for (size_t step = 0; step < route.size(); ++step)
				{
					previous = follow.follow(route[step], panes.player_viewport(view),
						1080.0f, framing, previous, bounds);
					reference[step] = previous;
				}

				for (const Vector2I& size : resolutions)
				{
					resolution.set_resolution_exactly(size);
					const Viewport pane(panes.player_viewport(view).pixel_rect());
					previous = Camera();
					for (size_t step = 0; step < route.size(); ++step)
					{
						previous = follow.follow(route[step], pane,
							static_cast<float>(size.y), framing, previous, bounds);
						const RectangleF visible = previous.visible_rectangle(pane);
						require(close_to(visible.size(), layout.world_size),
							"Resolution changed the authored world extent");
						require(close_to(previous.translation, reference[step].translation),
							"Resolution changed camera follow or bounds");
						if (step == 3)
						{
							require(close_to(visible.right(), bounds.right()) &&
								close_to(visible.bottom(), bounds.bottom()),
								"Far camera edges escaped the authored bounds");
						}
						if (step == 4)
						{
							require(close_to(visible.position(), bounds.position()),
								"Near camera edges escaped the authored bounds");
						}
					}
				}
			}
		}
	}

	void check_resize_and_aim(const CameraPresentation& framing)
	{
		ResolutionManager resolution;
		ViewportManager panes(&resolution);
		panes.set_layout(ScreenLayout::four_player);
		const PlayerCamera follow{};
		const RectangleF bounds(0.0f, 0.0f, 6000.0f, 3600.0f);
		const Vector2F player(3200.0f, 1800.0f);
		std::array<Camera, 4> cameras;
		// Exercise consecutive resize, a non-16:9 window and odd split edges.
		for (const Vector2I size : { Vector2I(1280, 720), Vector2I(2560, 1440),
			Vector2I(1600, 1200), Vector2I(1281, 721), Vector2I(1920, 1080) })
		{
			resolution.set_resolution_exactly(size);
			for (int view = 0; view < 4; ++view)
			{
				const Viewport pane = panes.player_viewport(view);
				const Viewport pixels(pane.pixel_rect());
				Camera& camera = cameras[static_cast<size_t>(view)];
				camera = follow.follow(player, pane, static_cast<float>(size.y),
					framing, camera, bounds);
				const RectangleF visible = camera.visible_rectangle(pixels);
				require(close_to(visible.height, pixels.height * 1080.0f / static_cast<float>(size.y)),
					"Aspect or odd-pixel resize changed the vertical framing policy");
				require(player.x >= visible.left() && player.x <= visible.right() &&
					player.y >= visible.top() && player.y <= visible.bottom(),
					"Resize lost the followed player");

				for (const Vector2F offset : { Vector2F(300.0f, -400.0f),
					Vector2F(-400.0f, 300.0f), Vector2F(1.0f, 9.0f) })
				{
					const Vector2F cursor = camera.calculate_view_position(player + offset) +
						pixels.position();
					require(close_to(PlayerCamera::mouse_direction(player, cursor, pane, camera),
						offset.normalized()), "Mouse aim drifted after scale, aspect or pane change");
				}
				const Vector2F on_player = camera.calculate_view_position(player) + pixels.position();
				require(PlayerCamera::mouse_direction(player, on_player, pane, camera) == Vector2F::ZERO,
					"A cursor on the player must not normalize zero or invent an aim direction");
			}
		}
	}

	void check_small_arenas(const CameraPresentation& framing)
	{
		const PlayerCamera follow{};
		const Viewport pane(0.0f, 0.0f, 1280.0f, 720.0f);
		for (const RectangleF bounds : { RectangleF(100.0f, -40.0f, 800.0f, 300.0f),
			RectangleF(100.0f, -40.0f, 800.0f, 3000.0f),
			RectangleF(100.0f, -40.0f, 3000.0f, 300.0f) })
		{
			Camera camera;
			for (int frame = 0; frame < 120; ++frame)
			{
				camera = follow.follow(bounds.position() + bounds.size() / 2.0f,
					pane, 720.0f, framing, camera, bounds);
				const RectangleF visible = camera.visible_rectangle(pane);
				if (bounds.width < visible.width)
				{
					require(close_to(visible.x + visible.width / 2.0f, bounds.x + bounds.width / 2.0f),
						"An oversized horizontal view drifted or alternated between bounds");
				}
				if (bounds.height < visible.height)
				{
					require(close_to(visible.y + visible.height / 2.0f, bounds.y + bounds.height / 2.0f),
						"An oversized vertical view drifted or alternated between bounds");
				}
			}
		}
	}
}

int main(int argc, char** argv)
{
	try
	{
		require(argc == 2, "Usage: camera_framing.exe <game/content directory>");
		const std::string path = (std::filesystem::path(argv[1]) / "presentation.json").string();
		const Presentation presentation(path.c_str());
		require(close_to(presentation.camera().reference_height, 1080.0f),
			"The camera baseline changed; review the authored framing acceptance");
		check_resolution_independence(presentation.camera());
		check_resize_and_aim(presentation.camera());
		check_small_arenas(presentation.camera());
		std::cout << "PASS camera framing: 1-4 players at 720p/1080p/QHD; resize, bounds, mouse aim.\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL camera framing: " << error.what() << '\n';
		return 1;
	}
}
