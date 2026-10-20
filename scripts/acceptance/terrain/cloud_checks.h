#pragma once

// The cloud drift checks: shipped clouds, the drift clock and malformed
// drift refusals. Included by terrain.cpp after its shared helpers.
namespace
{
	void check_cloud_pose(const DriftingVisual& cloud, const RectangleF& anchor,
		float distance, const RectangleF& cull, const std::string& where)
	{
		const RectangleF r = cloud.rectangle();
		require(std::isfinite(r.x) && std::abs(r.x - anchor.x) <= distance + 2.0f + EPSILON,
			where + ": cloud left its finite horizontal excursion");
		require(r.y == anchor.y && r.width == anchor.width && r.height == anchor.height,
			where + ": cloud changed its height or size");
		const float cells = (r.x - anchor.x) / 4.0f;
		require(std::abs(cells - std::round(cells)) <= EPSILON,
			where + ": cloud drift is not on the four-unit art grid");
		require(inside(r, cloud.bounds()) && inside(cloud.bounds(), cull),
			where + ": current geometry escaped its bounds or cull extent");
		require(inside(r, cull) && inside(anchor, cull),
			where + ": cloud escaped its original cull extent");
	}

	void check_shipped_clouds(World& world, const LevelObjectBuilder& builder,
		const JsonValue& decorations, const std::string& stage)
	{
		// Crossfeed deliberately uses a small static backdrop while its combat
		// geometry is evaluated. The other arenas retain their drift contract.
		const bool static_backdrop = stage == "crossfeed";
		int expected_random = 0;
		if (static_backdrop)
		{
			std::srand(8357);
			expected_random = std::rand();
			std::srand(8357);
		}
		world.preferences.reduce_effects = false;
		const auto first = builder.build_non_collision_objects(decorations);
		if (static_backdrop) { world.preferences.reduce_effects = true; }
		const auto restarted = builder.build_non_collision_objects(decorations);
		if (static_backdrop) { world.preferences.reduce_effects = false; }
		require(first.size() == decorations.size() && restarted.size() == decorations.size(),
			stage + ": decoration count changed");
		size_t clouds = 0;
		bool varied_phase = false;
		float first_phase = 0.0f;
		for (size_t index = 0; index < decorations.size(); ++index)
		{
			const JsonValue json = decorations.at(index);
			const bool is_cloud = json.string("frame_name").starts_with("cloud_");
			auto* cloud = dynamic_cast<DriftingVisual*>(first[index].get());
			if (static_backdrop)
			{
				const std::string where = stage + " " + json.string("name");
				auto* visual = dynamic_cast<Visual*>(first[index].get());
				auto* again = dynamic_cast<Visual*>(restarted[index].get());
				require(json.string("type") == "Visual" && !json.has("drift") && cloud == nullptr &&
					visual != nullptr && again != nullptr &&
					dynamic_cast<DriftingVisual*>(restarted[index].get()) == nullptr,
					where + ": static backdrop gained drift or another object type");
				const RectangleF anchor = authored_rectangle(json);
				same_rectangle(visual->bounds(), anchor, where + ": initial static bounds differ from authoring");
				same_rectangle(again->bounds(), anchor, where + ": restart changed static bounds");
				const RectangleF cull = visual->cull_bounds(1.0f);
				for (const bool reduced : { false, true, false })
				{
					world.preferences.reduce_effects = reduced;
					for (const float dt : { 0.0f, 1.0f / 60.0f, 1.0f, 60.0f, 3600.0f })
					{
						visual->update(dt);
						again->update(dt * 2.0f);
						require(visual->bounds() == anchor && again->bounds() == anchor &&
							visual->cull_bounds(0.0f) == anchor && again->cull_bounds(0.0f) == anchor &&
							visual->cull_bounds(1.0f) == cull && again->cull_bounds(1.0f) == cull,
							where + ": time or Reduce effects changed exact static bounds/culling");
					}
				}
				clouds += is_cloud ? 1 : 0;
				continue;
			}
			if (!is_cloud)
			{
				require(cloud == nullptr && !json.has("drift"),
					stage + ": drift reached scenery other than the clouds");
				continue;
			}
			const std::string where = stage + " " + json.string("name");
			require(cloud != nullptr && json.has("drift"), where + ": cloud is static");
			const JsonValue drift = json.object("drift");
			const float distance = drift.number("distance");
			const float period = drift.number("period");
			const float phase = drift.number("phase");
			if (clouds == 0) { first_phase = phase; }
			else { varied_phase |= phase != first_phase; }
			++clouds;
			const RectangleF anchor = authored_rectangle(json);
			const RectangleF initial = cloud->rectangle();
			const auto* again = dynamic_cast<const DriftingVisual*>(restarted[index].get());
			require(again != nullptr, where + ": restart changed the cloud type");
			same_rectangle(initial, again->rectangle(), where + ": restart changed its authored phase");

			// Compare the builder's authored phase against elapsed time from phase
			// zero. The clock's known quarter-cycle poses are checked separately.
			DriftingVisual phase_zero(json.string("sheet_name"), json.string("frame_name"),
				anchor, &world.render, &world.preferences, Colour::white, distance, period, 0.0f);
			phase_zero.update(period * phase);
			same_rectangle(initial, phase_zero.rectangle(), where + ": builder lost its authored phase");

			const RectangleF cull = cloud->cull_bounds(0.0f);
			for (int step = 0; step <= 72; ++step)
			{
				check_cloud_pose(*cloud, anchor, distance, cull, where);
				cloud->update(period / 72.0f);
			}
			world.preferences.reduce_effects = true;
			same_rectangle(cloud->rectangle(), anchor, where + ": reduced effects did not restore the anchor immediately");
			require(inside(anchor, cloud->bounds()) && inside(anchor, cull),
				where + ": reduced cloud escaped its bounds");
			world.preferences.reduce_effects = false;
		}
		if (static_backdrop)
		{
			require(clouds == 2, stage + ": static backdrop needs exactly two cloud Visuals");
			require(std::rand() == expected_random,
				stage + ": static backdrop construction/update consumed gameplay randomness");
			world.preferences.reduce_effects = false;
		}
		else
		{
			require(clouds >= 4 && varied_phase, stage + ": needs several clouds with staggered phases");
		}
	}

	void check_cloud_clock(World& world)
	{
		world.preferences.reduce_effects = false;
		const RectangleF anchor(100.0f, 200.0f, 256.0f, 96.0f);
		const auto make = [&](float distance = 64.0f, float phase = 0.0f)
		{
			return DriftingVisual("terrain", "cloud_big", anchor, &world.render,
				&world.preferences, Colour::white, distance, 32.0f, phase);
		};
		// Construction and visual time must not consume the gameplay rand stream.
		std::srand(7321);
		const int expected_random = std::rand();
		std::srand(7321);
		auto cloud = make();
		const RectangleF cull = cloud.cull_bounds(0.0f);
		const float offsets[]{ 0.0f, 64.0f, 0.0f, -64.0f, 0.0f };
		for (size_t quarter = 0; quarter < std::size(offsets); ++quarter)
		{
			if (quarter != 0) { cloud.update(8.0f); }
			same_rectangle(cloud.rectangle(), RectangleF(anchor.x + offsets[quarter], anchor.y,
				anchor.width, anchor.height), "cloud missed a known quarter-cycle pose");
			check_cloud_pose(cloud, anchor, 64.0f, cull, "quarter-cycle cloud");
		}
		require(std::rand() == expected_random, "cloud construction/update consumed gameplay randomness");

		auto phased = make(64.0f, 0.25f);
		same_rectangle(phased.rectangle(), RectangleF(164.0f, 200.0f, 256.0f, 96.0f),
			"initial cloud phase was delayed until its first update");
		const RectangleF still = phased.rectangle();
		for (float dt : { 0.0f, -1.0f, std::numeric_limits<float>::infinity(),
			-std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
		{
			phased.update(dt);
			same_rectangle(phased.rectangle(), still, "nonpositive/nonfinite dt moved the cloud clock");
		}

		auto large_tick = make();
		large_tick.update(1048584.0f); // 32768 complete periods and one quarter.
		same_rectangle(large_tick.rectangle(), still, "large dt did not wrap by the authored period");
		large_tick.update(std::numeric_limits<float>::max());
		check_cloud_pose(large_tick, anchor, 64.0f, cull, "maximum finite dt");
		auto whole = make();
		auto split = make();
		whole.update(7.25f);
		for (int step = 0; step < 58; ++step) { split.update(0.125f); }
		same_rectangle(split.rectangle(), whole.rectangle(), "equal elapsed time depends on frame partitioning");

		const RectangleF moving = phased.rectangle();
		world.preferences.reduce_effects = true;
		same_rectangle(phased.rectangle(), anchor, "live reduction needs an update before restoring the anchor");
		same_rectangle(phased.bounds(), anchor, "live reduction left bounds at the moving pose");
		require(inside(moving, phased.cull_bounds(0.0f)) && inside(anchor, phased.cull_bounds(0.0f)),
			"live reduction invalidated moving/static cull coverage");
		phased.update(8.0f);
		world.preferences.reduce_effects = false;
		same_rectangle(phased.rectangle(), moving, "reduced effects advanced the hidden cloud clock");
		phased.update(8.0f);
		same_rectangle(phased.rectangle(), anchor, "cloud did not resume from its held phase");
		world.preferences.reduce_effects = true;
		auto reduced_start = make(64.0f, 0.25f);
		reduced_start.update(20.0f);
		same_rectangle(reduced_start.rectangle(), anchor, "reduced initial construction did not stay static");
		world.preferences.reduce_effects = false;
		same_rectangle(reduced_start.rectangle(), moving, "reduced initial construction lost its authored phase");

		// Rounding an authored distance can exceed that distance. The cull
		// extent must include the snapped endpoints before the first update.
		auto snapped = make(63.0f);
		const RectangleF snapped_cull = snapped.cull_bounds(0.0f);
		snapped.update(8.0f);
		require(snapped.rectangle().x == 164.0f, "cloud peak did not snap to the art grid");
		check_cloud_pose(snapped, anchor, 63.0f, snapped_cull, "snapped positive peak");
		snapped.update(16.0f);
		require(snapped.rectangle().x == 36.0f, "cloud negative peak did not snap to the art grid");
		check_cloud_pose(snapped, anchor, 63.0f, snapped_cull, "snapped negative peak");
		for (float zoom : { 0.5f, 1.0f, 4.0f, 16.0f })
		{
			const RectangleF zoom_cull = snapped.cull_bounds(zoom);
			require(inside(snapped_cull, zoom_cull), "view-pixel cull padding shrank the cloud excursion");
			require(zoom_cull.left() < snapped_cull.left() && zoom_cull.right() > snapped_cull.right() &&
				zoom_cull.top() < snapped_cull.top() && zoom_cull.bottom() > snapped_cull.bottom(),
				"cloud culling omitted view-pixel snap padding");
		}
	}

	void check_cloud_rejections(World& world, const GameTuning& tuning,
		const std::filesystem::path& scratch)
	{
		std::filesystem::create_directories(scratch);
		const LevelObjectBuilder builder(&world.render, &world.audio, &world.preferences, &tuning.paint());
		const auto fixture = [&](const std::string& drift, size_t index)
		{
			const auto path = scratch / ("cloud-drift-" + std::to_string(index) + ".json");
			std::ofstream file(path, std::ios::binary | std::ios::trunc);
			file << "[{\"type\":\"Visual\",\"sheet_name\":\"terrain\",\"frame_name\":\"cloud_big\","
				"\"colour\":\"WHITE\",\"rectangle\":{\"x\":100,\"y\":200,\"width\":256,\"height\":96},"
				"\"drift\":" << drift << "}]";
			file.close();
			require(file.good(), "cannot write cloud drift fixture");
			return path;
		};
		struct Rejection { const char* drift; const char* field; };
		const Rejection rejected[]{
			{ "null", "drift" }, { "[]", "drift" },
			{ "{\"period\":32,\"phase\":0}", "distance" },
			{ "{\"distance\":0,\"period\":32,\"phase\":0}", "distance" },
			{ "{\"distance\":-1,\"period\":32,\"phase\":0}", "distance" },
			{ "{\"distance\":257,\"period\":32,\"phase\":0}", "distance" },
			{ "{\"distance\":1e100,\"period\":32,\"phase\":0}", "distance" },
			{ "{\"distance\":\"64\",\"period\":32,\"phase\":0}", "distance" },
			{ "{\"distance\":64,\"period\":0.5,\"phase\":0}", "period" },
			{ "{\"distance\":64,\"period\":3601,\"phase\":0}", "period" },
			{ "{\"distance\":64,\"period\":1e100,\"phase\":0}", "period" },
			{ "{\"distance\":64,\"period\":false,\"phase\":0}", "period" },
			{ "{\"distance\":64,\"period\":32,\"phase\":-0.01}", "phase" },
			{ "{\"distance\":64,\"period\":32,\"phase\":1}", "phase" },
			{ "{\"distance\":64,\"period\":32,\"phase\":1e100}", "phase" },
			{ "{\"distance\":64,\"period\":32,\"phase\":[]}", "phase" }
		};
		for (size_t index = 0; index < std::size(rejected); ++index)
		{
			const auto path = fixture(rejected[index].drift, index);
			const JsonDocument document = read_json_file(path.string().c_str());
			bool named = false;
			try { builder.build_non_collision_objects(document.root()); }
			catch (const std::exception& error)
			{
				const std::string message = error.what();
				named = message.find(path.filename().string()) != std::string::npos &&
					message.find("[0].drift") != std::string::npos &&
					message.find(rejected[index].field) != std::string::npos;
			}
			require(named, "malformed cloud drift must fail naming its file, object and " +
				std::string(rejected[index].field));
		}
		// Boundary values are useful authoring choices, not just refusal cases.
		const auto path = fixture("{\"distance\":256,\"period\":3600,\"phase\":0.999}", std::size(rejected));
		const JsonDocument document = read_json_file(path.string().c_str());
		const auto accepted = builder.build_non_collision_objects(document.root());
		require(accepted.size() == 1 && dynamic_cast<const DriftingVisual*>(accepted[0].get()) != nullptr,
			"valid cloud drift boundary values were rejected");
	}
}
