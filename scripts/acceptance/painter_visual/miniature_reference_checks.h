// Probe-only frozen reference: the retired manual miniature traversal remains
// here so the new Scene path is compared against its previous painter/cull order.
// Production never includes this file or keeps these borrowed scenery mirrors.
void draw_legacy_miniature_view(DrawList& list, const Camera& camera,
	const Viewport& viewport, std::span<GameObject* const> scenery,
	std::span<CollisionObject* const> collision_objects)
{
	list.set_viewport(viewport);
	list.set_camera(camera);
	const RectangleF visible = camera.visible_rectangle(viewport);
	const float units_per_pixel = 1.0f / camera.scale;
	for (const GameObject* object : scenery)
	{
		if (object->cull_bounds(units_per_pixel).intersects(visible))
		{
			object->draw(list);
		}
	}
	for (const CollisionObject* object : collision_objects)
	{
		if (!object->cull_bounds(units_per_pixel).intersects(visible)) { continue; }
		if (const auto* structure = dynamic_cast<const Structure*>(object))
		{
			structure->draw_miniature(list);
		}
		else
		{
			object->draw(list);
		}
	}
}

// Fixture configuration happens before begin_frame/Scene fan-out. This is
// explicit path policy, never an inference from a small camera scale.
void configure_miniature_fixture(const Scene& scene, bool miniature)
{
    for (CollisionObject* object : scene.collision_objects())
    {
        if (auto* structure = dynamic_cast<Structure*>(object))
        { structure->set_miniature(miniature); }
    }
}

std::vector<bool> miniature_fixture_modes(const Scene& scene)
{
    std::vector<bool> result;
    for (const CollisionObject* object : scene.collision_objects())
    {
        if (const auto* structure = dynamic_cast<const Structure*>(object))
        { result.push_back(structure->miniature()); }
    }
    return result;
}

// Defined after CrossfeedFixture: it borrows that real Level/Scene construction.
void miniature_path_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& output, std::ofstream& records);
