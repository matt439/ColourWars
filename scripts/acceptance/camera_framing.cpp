#include "game/pch.h"
#include "game/objects/player_camera.h"
#include "game/presentation.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/viewport_manager.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
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

	bool exact(const Camera& first, const Camera& second)
	{
		return first.translation.x == second.translation.x &&
			first.translation.y == second.translation.y && first.scale == second.scale;
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
					// This is the original height-only follow contract. Inactive
					// options must preserve its arithmetic even while moving.
					Camera scaled = previous;
					scaled.scale = 1080.0f / framing.reference_height;
					const Camera original = CameraTools{}.calculate_camera(route[step],
						scaled.visible_rectangle(panes.player_viewport(view)).size(), scaled, bounds);
					const Camera moving = follow.follow(route[step], panes.player_viewport(view),
						1080.0f, framing, previous, bounds, step % 2 == 0 ? 800.0f : -800.0f);
					previous = follow.follow(route[step], panes.player_viewport(view),
						1080.0f, framing, previous, bounds);
					require(exact(previous, original) && exact(moving, original),
						"Zero camera options changed the exact historical follow contract");
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

	void check_profile_matrix(const CameraPresentation& framing, const PlayerIdentityPresentation& identity)
	{
		const std::array<LayoutCase, 4> layouts = {{
			{ ScreenLayout::one_player, 1, {} }, { ScreenLayout::two_player, 2, {} },
			{ ScreenLayout::three_player, 3, {} }, { ScreenLayout::four_player, 4, {} }
		}};
		const std::array<Vector2I, 5> resolutions = {{ { 1280, 720 }, { 1920, 1080 },
			{ 2560, 1440 }, { 1281, 721 }, { 1600, 1200 } }};
		const Vector2F player(4000.0f, 2500.0f);
		RectangleF actor(player.x - 26.0f, player.y - 60.0f, 52.0f, 120.0f);
		if (identity.enabled)
		{
			const Vector2F badge = identity.world_size * std::max(1.0f, identity.emphasis_scale);
			const Vector2F bottom = Vector2F(player.x, player.y - 60.0f) + identity.world_offset;
			actor = RectangleF::union_of(actor, RectangleF(bottom - Vector2F(badge.x / 2.0f, badge.y), badge));
		}
		const RectangleF bounds(0.0f, 0.0f, 10000.0f, 6000.0f);
		const PlayerCamera follow{};
		ResolutionManager resolution;
		ViewportManager panes(&resolution);
		for (const LayoutCase& layout : layouts)
		{
			panes.set_layout(layout.layout);
			for (const Vector2I size : resolutions)
			{
				resolution.set_resolution_exactly(size);
				for (int view = 0; view < layout.players; ++view)
				{
					const Viewport pane(panes.player_viewport(view).pixel_rect());
					float scale = static_cast<float>(size.y) / framing.reference_height;
					if (framing.minimum_world_width > 0.0f)
					{
						scale = std::min(scale, pane.width / framing.minimum_world_width);
					}
					const Vector2F world = pane.size() / scale;
					const BorderThickness border = CameraTools::calculate_camera_scroll_border(world);
					const float lead = std::min({ 800.0f * framing.horizontal_lead_seconds,
						framing.horizontal_lead_max_distance, world.x / 4.0f });
					for (const float direction : { -1.0f, 1.0f })
					{
						// Start on the follow threshold, as after a steady approach.
						const float anchor = direction > 0.0f ? world.x - border.right : border.left;
						const Camera previous(player.x - anchor, player.y - world.y / 2.0f, scale);
						const Camera camera = follow.follow(player, pane, static_cast<float>(size.y),
							framing, previous, bounds, direction * 800.0f);
						const RectangleF visible = camera.visible_rectangle(pane);
						require(close_to(camera.scale, scale) && close_to(visible.size(), world),
							"A camera profile changed the minimum-width scale or pane aspect");
						require(close_to(camera.translation.x - previous.translation.x, direction * lead) &&
							close_to(camera.translation.y, previous.translation.y),
							"Horizontal lead lost its sign/cap or shifted the vertical focus");
						// The current 52x120 body and the authored badge at full emphasis.
						require(visible.contains(actor),
							"A practical body/badge envelope escaped a led camera pane");
						const float forward = direction > 0.0f ? visible.right() - player.x : player.x - visible.left();
						const float rear = direction > 0.0f ? player.x - visible.left() : visible.right() - player.x;
						const float old_forward = direction > 0.0f ? border.right : border.left;
						require(close_to(forward, old_forward + lead) && close_to(rear, world.x - old_forward - lead),
							"Camera lead concealed its forward/rear world-extent tradeoff");
						const Vector2F offset(direction * 350.0f, -120.0f);
						const Vector2F cursor = camera.calculate_view_position(player + offset) + pane.position();
						require(close_to(PlayerCamera::mouse_direction(player, cursor, pane, camera), offset.normalized()) &&
							PlayerCamera::mouse_direction(player, camera.calculate_view_position(player) + pane.position(), pane, camera) == Vector2F::ZERO,
							"Mouse projection used virtual lead focus instead of the physical player");
						const Camera stopped = follow.follow(player, pane, static_cast<float>(size.y), framing, camera, bounds);
						require(exact(stopped, follow.follow(player, pane, static_cast<float>(size.y), framing, stopped, bounds)),
							"A stopped or repeatedly refreshed camera drifted through its deadzone");
						const Camera reversed = follow.follow(player, pane, static_cast<float>(size.y),
							framing, stopped, bounds, -direction * 800.0f);
						require((reversed.translation.x - stopped.translation.x) * direction <= 0.002f &&
							std::abs(reversed.translation.x - stopped.translation.x) <= lead * 2.0f + 0.002f,
							"A reversal moved the camera the wrong way or beyond the lead/deadzone limit");
						const Camera partial = follow.follow(player, pane, static_cast<float>(size.y),
							framing, previous, bounds, direction * 200.0f);
						require(std::abs(partial.translation.x - previous.translation.x) <= lead + 0.002f,
							"Partial movement produced more lead than a full-speed approach");
					}
					for (const Vector2F near_edge : { Vector2F(60.0f, 120.0f), Vector2F(9940.0f, 5880.0f) })
					{
						const Camera camera = follow.follow(near_edge, pane, static_cast<float>(size.y),
							framing, Camera(), bounds, near_edge.x < player.x ? -800.0f : 800.0f);
						const RectangleF visible = camera.visible_rectangle(pane);
						require(visible.left() >= bounds.left() - 0.002f && visible.top() >= bounds.top() - 0.002f &&
							visible.right() <= bounds.right() + 0.002f && visible.bottom() <= bounds.bottom() + 0.002f &&
							visible.contains(RectangleF(near_edge.x - 26.0f, near_edge.y - 60.0f, 52.0f, 120.0f)),
							"A bounded led camera escaped the arena or clipped the actual body");
					}
					for (const Vector2F extent : { Vector2F(800, 300), Vector2F(800, 4000), Vector2F(6000, 300) })
					{
						const RectangleF small_bounds(Vector2F(100, -40), extent);
						Camera camera;
						for (int tick = 0; tick < 3; ++tick)
						{
							camera = follow.follow(small_bounds.center(), pane, static_cast<float>(size.y),
								framing, camera, small_bounds, tick == 1 ? -800.0f : 800.0f);
							const RectangleF visible = camera.visible_rectangle(pane);
							if (world.x >= small_bounds.width) { require(close_to(visible.center().x, small_bounds.center().x), "Oversized horizontal framing lost arena centering"); }
							if (world.y >= small_bounds.height) { require(close_to(visible.center().y, small_bounds.center().y), "Oversized vertical framing lost arena centering"); }
						}
					}
				}
			}
		}
	}

	template<class Invoke>
	void rejects(Invoke invoke)
	{
		bool rejected = false;
		try { invoke(); }
		catch (const std::invalid_argument&) { rejected = true; }
		require(rejected, "Invalid camera input was accepted");
	}

	void check_invalid_inputs(const CameraPresentation& framing)
	{
		const PlayerCamera follow{};
		const Viewport pane(0, 0, 1280, 720);
		const Vector2F player(3000, 1800);
		const RectangleF bounds(0, 0, 6000, 3600);
		const float infinity = std::numeric_limits<float>::infinity();
		const float nan = std::numeric_limits<float>::quiet_NaN();
		for (const float value : { -1.0f, infinity, nan })
		{
			for (float CameraPresentation::* field : { &CameraPresentation::minimum_world_width,
				&CameraPresentation::horizontal_lead_seconds, &CameraPresentation::horizontal_lead_max_distance })
			{
				CameraPresentation invalid = framing; invalid.*field = value;
				rejects([&] { return follow.follow(player, pane, 720.0f, invalid, Camera(), bounds); });
			}
		}
		for (const float value : { 0.0f, -1.0f, infinity, nan })
		{
			rejects([&] { return follow.follow(player, pane, value, framing, Camera(), bounds); });
			CameraPresentation invalid = framing; invalid.reference_height = value;
			rejects([&] { return follow.follow(player, pane, 720.0f, invalid, Camera(), bounds); });
		}
		for (const float value : { infinity, nan })
		{
			rejects([&] { return follow.follow(player, pane, 720.0f, framing, Camera(), bounds, value); });
			rejects([&] { return follow.follow({ value, player.y }, pane, 720.0f, framing, Camera(), bounds); });
			rejects([&] { return follow.follow(player, pane, 720.0f, framing, Camera(value, 0, 1), bounds); });
			rejects([&] { return follow.follow(player, Viewport(value, 0, 1280, 720), 720.0f, framing, Camera(), bounds); });
			rejects([&] { return follow.follow(player, pane, 720.0f, framing, Camera(), RectangleF(0, value, 6000, 3600)); });
		}
		rejects([&] { return follow.follow(player, Viewport(0, 0, 0, 720), 720.0f, framing, Camera(), bounds); });
		rejects([&] { return follow.follow(player, Viewport(0, 0, 0.5f, 720), 720.0f, framing, Camera(), bounds); });
		rejects([&] { return follow.follow(player, Viewport(-2e9f, 0, 4e9f, 720), 720.0f, framing, Camera(), bounds); });
		rejects([&] { return follow.follow(player, pane, 720.0f, framing, Camera(), RectangleF(0, 0, 0, 3600)); });
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
		CameraPresentation historical = presentation.camera();
		historical.minimum_world_width = 0.0f;
		historical.horizontal_lead_seconds = 0.0f;
		historical.horizontal_lead_max_distance = 0.0f;
		check_resolution_independence(historical);
		check_resize_and_aim(historical);
		check_small_arenas(historical);
		CameraPresentation velocity = historical;
		velocity.horizontal_lead_seconds = 0.30f; velocity.horizontal_lead_max_distance = 240.0f;
		CameraPresentation combined = historical;
		combined.minimum_world_width = 1280.0f;
		combined.horizontal_lead_seconds = 0.25f; combined.horizontal_lead_max_distance = 200.0f;
		CameraPresentation capped = historical;
		capped.horizontal_lead_seconds = 1.0f; capped.horizontal_lead_max_distance = 70.0f;
		for (const CameraPresentation& profile : { historical, velocity, combined, capped, presentation.camera() })
		{
			check_profile_matrix(profile, presentation.player_identity());
			check_invalid_inputs(profile);
		}
		std::cout << "PASS camera framing: exact zero-option history and loaded/lead/minimum-width profiles; "
			"1-4 players at 720p/1080p/QHD/odd/aspect sizes; bounds, body/badge space, rear tradeoffs, stop/reversal and mouse aim.\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL camera framing: " << error.what() << '\n';
		return 1;
	}
}
