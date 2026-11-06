#pragma once

namespace
{
    void check_runoff_content(const std::filesystem::path& content)
    {
        const acceptance::JsonVariants variants(content / "tuning.json", L"runoff-tuning");
        const auto load = [&](auto edit)
        {
            return GameTuning(variants.write(edit).string().c_str());
        };
        const auto rejected = [&](const char* field, auto edit)
        {
            const auto load_path = [](const std::filesystem::path& path) { const GameTuning invalid(path.string().c_str()); };
            const char* message = "Invalid runoff tuning did not fail at startup with its owning field named";
            if (field == nullptr)
            {
                acceptance::rejects_variant<std::runtime_error>(variants, edit, load_path, { "paint", "runoff" }, message);
            }
            else
            {
                acceptance::rejects_variant<std::runtime_error>(variants, edit, load_path, { "paint", "runoff", field }, message);
            }
        };
        rejected(nullptr, [](rapidjson::Document& json)
            { json["paint"]["visual"].RemoveMember("runoff"); });
        rejected(nullptr, [](rapidjson::Document& json)
            { json["paint"]["visual"]["runoff"].SetNull(); });
        rejected(nullptr, [](rapidjson::Document& json)
            { json["paint"]["visual"]["runoff"].SetInt(0); });
        struct Number { const char* key; float maximum; bool positive; };
        for (const Number number : { Number{"delay", 1, false}, {"duration", 2, true},
            {"distance", 64, true}, {"width", 16, true}, {"bead_height", 32, true},
            {"trail_length", 64, true}, {"trail_width", 16, true}, {"keyline_width", 8, true},
            {"face_inset", 16, false}, {"shade", 1, false}, {"highlight", 1, false} })
        {
            rejected(number.key, [&](rapidjson::Document& json)
                { json["paint"]["visual"]["runoff"].RemoveMember(number.key); });
            rejected(number.key, [&](rapidjson::Document& json)
                { json["paint"]["visual"]["runoff"][number.key].SetNull(); });
            rejected(number.key, [&](rapidjson::Document& json)
                { json["paint"]["visual"]["runoff"][number.key].SetString("bad", json.GetAllocator()); });
            for (const float value : { -0.01f, number.maximum + 0.01f })
                rejected(number.key, [&](rapidjson::Document& json)
                    { json["paint"]["visual"]["runoff"][number.key].SetFloat(value); });
            if (number.positive)
                rejected(number.key, [&](rapidjson::Document& json)
                    { json["paint"]["visual"]["runoff"][number.key].SetInt(0); });
        }
        rejected("width", [](rapidjson::Document& json)
            { json["paint"]["visual"]["runoff"]["width"].SetFloat(16); });
        rejected("trail_width", [](rapidjson::Document& json)
            { json["paint"]["visual"]["runoff"]["trail_width"].SetFloat(4); });
        rejected("keyline_width", [](rapidjson::Document& json)
            { json["paint"]["visual"]["runoff"]["keyline_width"].SetFloat(1.5f); });
        rejected("keyline_width", [](rapidjson::Document& json)
            { json["paint"]["visual"]["runoff"]["bead_height"].SetFloat(1.5f); });
        // A fitting width may touch the inward edge exactly, and a trail may
        // fill that width. Zero delay/inset and endpoint shade values are valid.
        for (const bool upper : { false, true })
        {
            const GameTuning valid = load([&](rapidjson::Document& json)
            {
                auto& paint = json["paint"];
                paint["thickness"].SetFloat(40);
                auto& values = paint["visual"]["runoff"];
                values["delay"].SetFloat(upper ? 1.0f : 0.0f);
                values["duration"].SetFloat(upper ? 2.0f : 0.001f);
                values["distance"].SetFloat(upper ? 64.0f : 0.001f);
                values["width"].SetFloat(16);
                values["bead_height"].SetFloat(32);
                values["trail_length"].SetFloat(upper ? 64.0f : 0.001f);
                values["trail_width"].SetFloat(16);
                values["keyline_width"].SetFloat(7.99f);
                values["face_inset"].SetFloat(upper ? 16.0f : 0.0f);
                values["shade"].SetFloat(upper ? 1.0f : 0.0f);
                values["highlight"].SetFloat(upper ? 1.0f : 0.0f);
            });
            require(valid.paint().runoff.delay == (upper ? 1.0f : 0.0f) &&
                valid.paint().runoff.trail_width == valid.paint().runoff.width,
                "Valid runoff tuning boundary was not retained");
        }
        std::cout << "PASS required runoff content, numeric bounds, cross-field fit and named startup diagnostics\n";
    }
}
