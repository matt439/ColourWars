// Included inside tune_checks.cpp's private namespace, after its includes.
// The coupled identity and shared-HUD-paper colour pairs, each in isolated LF/CRLF contexts.

	void check_identity_colour_tunes(const std::filesystem::path& game_directory)
	{
		for (const bool background : { false, true })
		{
			const std::size_t index = background ? identity_background_colour_index : identity_ink_colour_index;
			const std::size_t sibling_index = background ? identity_ink_colour_index : identity_background_colour_index;
			const std::string path = colour_paths[index];
			const std::string local = path.substr(std::string("presentation.").size());
			const std::string sibling_path = colour_paths[sibling_index];
			const std::string sibling_local = sibling_path.substr(std::string("presentation.").size());
			const auto value_of = [background](const PlayerIdentityPresentation& look)
				{ return background ? look.background_colour : look.ink_colour; };
			const auto sibling_of = [background](const PlayerIdentityPresentation& look)
				{ return background ? look.ink_colour : look.background_colour; };
			for (const bool crlf : { false, true })
			{
				ScratchContent scratch(game_directory, background ?
					(crlf ? L"identity-background-crlf" : L"identity-background-lf") :
					(crlf ? L"identity-ink-crlf" : L"identity-ink-lf"));
				const auto source = scratch.source / L"presentation.json";
				const auto staged = scratch.staged / L"presentation.json";
				std::string baseline = read_text(source);
				if (crlf)
				{
					std::string converted;
					for (const char character : baseline)
						{ converted += character == '\n' ? std::string("\r\n") : std::string(1, character); }
					baseline = std::move(converted);
					write_text(source, baseline);
					write_text(staged, baseline);
				}
				GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
				Presentation presentation(staged.string().c_str());
				TestDirector test(tuning, presentation);
				DebugDirectorImpl& impl = test.impl;
				impl.set_content_directories(scratch.source, scratch.staged);
				const PlayerIdentityPresentation* const identity = &presentation.player_identity();
				const auto initial = colour_values(presentation);
				const auto state = [&] { return colour_values(presentation); };
				const auto edits = [&]
				{
					std::vector<std::string> values;
					for (const auto& [key, edit] : impl.tune_document("presentation")->edits())
						{ values.push_back(key + "=" + edit.baseline_token + "->" + edit.current_token); }
					return values;
				};
				for (const auto& [word, colour] : std::array<std::pair<const char*, labrador::Colour>, 3>{ {
					{ "RED", { 255, 0, 0 } }, { "a1B2c3", { 161, 178, 195 } }, { "102030FF", { 16, 32, 48 } } } })
				{
					auto expected = initial;
					expected[index] = colour;
					require(impl.execute("tune " + path + " " + word).ok && colour_values(presentation) == expected &&
						&presentation.player_identity() == identity && impl.tune_token(path) == std::string("\"") + word + '"',
						"Identity colour accepts named/RGB/opaque RGBA on its borrowed record without changing other colours: " + path);
				}
				const JsonSpanDocument authored(baseline, "identity-colour-reference");
				const std::string sibling_token(authored.token(*authored.find(sibling_local)));
				for (const std::string& word : { std::string("11223380"), std::string("11223300"),
					sibling_token.substr(1, sibling_token.size() - 2) })
				{
					const auto before_state = state();
					const auto before_edits = edits();
					const std::string before_text = impl.tune_document("presentation")->current().text();
					require(!impl.execute("tune " + path + " " + word).ok && state() == before_state && edits() == before_edits &&
						impl.tune_document("presentation")->current().text() == before_text &&
						read_text(source) == baseline && read_text(staged) == baseline,
						"Translucent, invisible or sibling-matching identity colour preserves live values, pending tokens and both files: " + path);
				}
				require(impl.execute("tune.reset " + path).ok && colour_values(presentation) == initial &&
					impl.tune_document("presentation")->current().text() == baseline && edits().empty(),
					"Identity colour resets exactly to its authored token without a rebuild or file write: " + path);

				// Both identity leaves can be pending at once. Validate against the
				// currently edited sibling, and reset one without resetting the other.
				require(initial[3] != labrador::Colour(32, 48, 64) &&
					impl.execute("tune " + path + " RED").ok && impl.execute("tune " + sibling_path + " BLUE").ok &&
					impl.execute("tune presentation.debug_overlay.colour 203040").ok,
					"Both distinct opaque identity fields and an unrelated colour can be pending together: " + path);
				const auto paired_state = state();
				const auto paired_edits = edits();
				const std::string paired_text = impl.tune_document("presentation")->current().text();
				require(!impl.execute("tune " + path + " BLUE").ok && state() == paired_state && edits() == paired_edits &&
					impl.tune_document("presentation")->current().text() == paired_text &&
					read_text(source) == baseline && read_text(staged) == baseline,
					"Identity equality is checked against the pending sibling and refusal is atomic: " + path);
				auto target_reset = paired_state;
				target_reset[index] = initial[index];
				require(impl.execute("tune.reset " + path).ok && state() == target_reset &&
					impl.tune_document("presentation")->edits().count(local) == 0 &&
					impl.tune_document("presentation")->edits().count(sibling_local) == 1 &&
					impl.tune_document("presentation")->edits().count("debug_overlay.colour") == 1,
					"Target identity reset preserves its pending sibling and unrelated colour: " + path);
				require(impl.execute("tune " + path + " RED").ok && impl.execute("tune.reset " + sibling_path).ok,
					"A pending identity edit permits an independent sibling reset: " + path);
				auto sibling_reset = paired_state;
				sibling_reset[sibling_index] = initial[sibling_index];
				require(state() == sibling_reset && impl.tune_document("presentation")->edits().count(local) == 1 &&
					impl.tune_document("presentation")->edits().count(sibling_local) == 0 &&
					impl.tune_document("presentation")->edits().count("debug_overlay.colour") == 1 &&
					impl.execute("tune.reset presentation").ok && state() == initial && edits().empty() &&
					impl.tune_document("presentation")->current().text() == baseline &&
					read_text(source) == baseline && read_text(staged) == baseline,
					"Sibling reset preserves target and unrelated edits; full reset restores exact authored identity bytes: " + path);

				// Saving merges a fresh unrelated disk edit, splices only our colour,
				// then adopts and rebases the full validated file.
				const std::string external = authored.replaced(*authored.find("debug_overlay.colour"), "\"203040\"");
				require(initial[3] != labrador::Colour(32, 48, 64), "The identity save introduces a fresh unrelated external colour");
				write_text(source, external);
				const JsonSpanDocument external_document(external, "identity-colour-reference");
				const std::string saved = external_document.replaced(*external_document.find(local), "\"123456FF\"");
				auto saved_values = initial;
				saved_values[3] = labrador::Colour(32, 48, 64);
				saved_values[index] = labrador::Colour(18, 52, 86);
				require(impl.execute("tune " + path + " 123456FF").ok && impl.execute("tune.save presentation").ok &&
					read_text(source) == saved && read_text(staged) == saved && colour_values(presentation) == saved_values &&
					impl.tune_document("presentation")->baseline().text() == saved && edits().empty() &&
					colour_values(Presentation(source.string().c_str())) == saved_values &&
					count_of(saved, "\r\n") == count_of(baseline, "\r\n") && count_of(saved, "\n") == count_of(baseline, "\n"),
					"Identity save changes only its quoted LF/CRLF token, preserves the external edit, loads and rebases both copies: " + path);
				require(impl.execute("tune " + path + " a1B2c3").ok && impl.execute("tune.reset " + path).ok &&
					colour_values(presentation) == saved_values && impl.tune_document("presentation")->current().text() == saved && edits().empty(),
					"Identity reset after save restores the saved opaque spelling and adopted external baseline: " + path);

				const JsonSpanDocument saved_document(saved, "identity-colour-conflict");
				const std::string conflicting = saved_document.replaced(*saved_document.find(local), "\"FEDCBA\"");
				write_text(source, conflicting);
				require(impl.execute("tune " + path + " ABCDEF").ok, "Identity colour is pending before an on-disk token conflict: " + path);
				const auto conflict_state = state();
				const auto conflict_edits = edits();
				const std::string conflict_text = impl.tune_document("presentation")->current().text();
				require(!impl.execute("tune.save presentation").ok && read_text(source) == conflicting && read_text(staged) == saved &&
					state() == conflict_state && edits() == conflict_edits && impl.tune_document("presentation")->current().text() == conflict_text,
					"An identity colour token conflict preserves source, staging, live colours and the complete pending edit: " + path);
				const JsonSpanDocument conflicting_document(conflicting, "identity-colour-conflict");
				const std::string forced = conflicting_document.replaced(*conflicting_document.find(local), "\"ABCDEF\"");
				require(impl.execute("tune.save presentation force").ok && read_text(source) == forced && read_text(staged) == forced &&
					value_of(*identity) == labrador::Colour(171, 205, 239) && impl.tune_document("presentation")->baseline().text() == forced && edits().empty(),
					"Force resolves only the identity token conflict and rebases the validated opaque colour: " + path);

				// Each file is valid independently. Pending RED ink + external RED
				// background, and the inverse, must fail only at the merged candidate.
				require(impl.execute("tune " + path + " RED").ok, "A distinct red identity colour is pending before the coupled disk conflict: " + path);
				const JsonSpanDocument forced_document(forced, "identity-colour-merged-conflict");
				const std::string coupled = forced_document.replaced(*forced_document.find(sibling_local), "\"RED\"");
				write_text(source, coupled);
				require(sibling_of(Presentation(source.string().c_str()).player_identity()) == labrador::Colour::red &&
					sibling_of(*identity) != labrador::Colour::red, "The external red sibling is valid before merging and is not yet live: " + path);
				const auto coupled_state = state();
				const auto coupled_edits = edits();
				const std::string coupled_text = impl.tune_document("presentation")->current().text();
				for (const char* command : { "tune.save presentation", "tune.save presentation force" })
				{
					impl.log().clear();
					require(!impl.execute(command).ok && read_text(source) == coupled && read_text(staged) == forced &&
						state() == coupled_state && edits() == coupled_edits && impl.tune_document("presentation")->current().text() == coupled_text &&
						impl.tune_document("presentation")->baseline().text() == forced &&
						log_contains(impl.log(), "the written file did not load", DebugSeverity::error) &&
						log_contains(impl.log(), "; the old bytes are back", DebugSeverity::error),
						"Invalid merged identity colours restore the exact external LF/CRLF source and preserve staged/live/ledger state, even with force: " + path);
				}
				require(impl.execute("tune.reset " + path).ok && value_of(*identity) == labrador::Colour(171, 205, 239) &&
					read_text(source) == coupled && read_text(staged) == forced && edits().empty() &&
					!std::filesystem::exists(source.string() + ".tmp") && !std::filesystem::exists(source.string() + ".bak") &&
					!std::filesystem::exists(staged.string() + ".tmp"),
					"After rejected merged saves, reset still uses the saved baseline without discarding the external source or leaving recovery files: " + path);
			}
		}
	}

// Coupled shared-HUD ink/paper reader and persistence checks use isolated
// LF/CRLF contexts; a same-colour sequential sweep would invalidate the pair.
void check_hud_paper_colour_tunes(const std::filesystem::path& game_directory)
{
    for (const bool paper : { false, true })
    {
        const std::size_t index = paper ? hud_paper_paper_colour_index : hud_paper_ink_colour_index;
        const std::size_t sibling_index = paper ? hud_paper_ink_colour_index : hud_paper_paper_colour_index;
        const std::string path = colour_paths[index];
        const std::string sibling_path = colour_paths[sibling_index];
        const std::string local = path.substr(std::string("presentation.").size());
        const std::string sibling_local = sibling_path.substr(std::string("presentation.").size());
        for (const bool crlf : { false, true })
        {
            ScratchContent scratch(game_directory, paper ?
                (crlf ? L"hud-paper-paper-crlf" : L"hud-paper-paper-lf") :
                (crlf ? L"hud-paper-ink-crlf" : L"hud-paper-ink-lf"));
            const auto source = scratch.source / L"presentation.json";
            const auto staged = scratch.staged / L"presentation.json";
            std::string baseline = read_text(source);
            if (crlf)
            {
                std::string converted;
                for (const char character : baseline)
                    { converted += character == '\n' ? std::string("\r\n") : std::string(1, character); }
                baseline = std::move(converted);
                write_text(source, baseline);write_text(staged, baseline);
            }
            GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
            Presentation presentation(staged.string().c_str());
            TestDirector test(tuning, presentation);auto& impl = test.impl;
            impl.set_content_directories(scratch.source, scratch.staged);
            const HudPresentation* const hud = &presentation.hud();
            const PlayerIdentityPresentation* const badge = &presentation.player_identity();
            const auto initial = colour_values(presentation);auto expected = initial;
            const auto* const document = impl.tune_document("presentation");
            const auto ledger = [&]
            {
                std::vector<std::string> values;
                for (const auto& [key, edit] : document->edits())
                    { values.push_back(key + '=' + edit.baseline_token + "->" + edit.current_token); }
                return values;
            };
            const auto no_temporary = [&]
            {
                return !std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
                    !std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
                    !std::filesystem::exists(scratch.staged / L"presentation.json.tmp");
            };
            const auto held = [&]
            {
                require(colour_values(presentation) == expected && &presentation.hud() == hud &&
                    &presentation.player_identity() == badge && !impl.in_match() &&
                    !impl.can_apply() && impl.request().empty(),
                    "Coupled shared-paper commands retain every unrelated colour, borrowed record and idle match state");
            };
            const auto refuse = [&](const std::string& command)
            {
                const auto before_values = colour_values(presentation);
                const auto before_edits = ledger();
                const std::string before_current = document->current().text();
                const std::string before_baseline = document->baseline().text();
                const std::string before_source = read_text(source);
                const std::string before_staged = read_text(staged);
                require(!impl.execute(command).ok && colour_values(presentation) == before_values &&
                    document->current().text() == before_current && document->baseline().text() == before_baseline &&
                    ledger() == before_edits && read_text(source) == before_source && read_text(staged) == before_staged &&
                    no_temporary(), "A refused coupled shared-paper command preserves full live/documents/ledger/source/staging state: " + command);
                held();
            };
            const auto set = [&](std::size_t selected, const char* word)
            {
                expected[selected] = std::string(word) == "RED" ? labrador::Colour(255, 0, 0) :
                    std::string(word) == "BLUE" ? labrador::Colour(0, 0, 255) :
                    std::string(word) == "GREEN" ? labrador::Colour(0, 128, 0) : labrador::Colour(word);
                require(impl.execute(std::string("tune ") + colour_paths[selected] + ' ' + word).ok &&
                    impl.tune_token(colour_paths[selected]) == std::string("\"") + word + '"',
                    "A pair-safe shared-paper assignment preserves the exact quoted word");
                held();
            };
            for (const char* word : { "RED", "a1B2c3", "102030FF" }) { set(index, word); }
            for (const char* word : { "2060C080", "A532D600", "next", "prev", "toggle",
                "on", "false", "+=1", "#112233", "12345", "GGGGGG", "white", "\"RED\"", "RED;feedback on" })
                { refuse("tune " + path + ' ' + quote_word(word)); }
            expected = initial;
            require(impl.execute("tune.reset presentation").ok && document->current().text() == baseline &&
                document->baseline().text() == baseline && document->edits().empty(),
                "An isolated opaque shared-paper sweep resets exact authored bytes");held();

            // One exact quoted token, then a genuine conflict with an unrelated
            // external colour. Both pending and disk pairs remain reader-valid.
            set(index, "123456FF");
            const JsonSpanDocument authored(baseline, "hud-paper-single-save-reference");
            const std::string saved = authored.replaced(*authored.find(local), "\"123456FF\"");
            require(impl.execute("tune.save presentation").ok && read_text(source) == saved && read_text(staged) == saved &&
                document->current().text() == saved && document->baseline().text() == saved &&
                document->edits().empty() && no_temporary(),
                "Shared opaque paper Save replaces exactly one LF/CRLF token and rebases both copies");held();
            set(index, "ABCDEF");
            const JsonSpanDocument disk(saved, "hud-paper-conflict-reference");
            const std::string changed = disk.replaced(*disk.find(local), "\"FEDCBA\"");
            const JsonSpanDocument changed_reference(changed, "hud-paper-conflict-overlay-reference");
            const std::string external = changed_reference.replaced(*changed_reference.find("debug_overlay.colour"), "\"203040\"");
            write_text(source, external);
            require(colour_values(Presentation(source.string().c_str()))[index] == labrador::Colour("FEDCBA") &&
                !document->edits().contains(sibling_local),
                "The opaque conflict source is valid with a genuinely unedited distinct counterpart");
            refuse("tune.save presentation");
            const JsonSpanDocument force_reference(external, "hud-paper-force-reference");
            const std::string forced = force_reference.replaced(*force_reference.find(local), "\"ABCDEF\"");
            expected[3] = labrador::Colour("203040");
            require(impl.execute("tune.save presentation force").ok && read_text(source) == forced && read_text(staged) == forced &&
                document->current().text() == forced && document->baseline().text() == forced &&
                document->edits().empty() && colour_values(Presentation(source.string().c_str())) == expected && no_temporary() &&
                (crlf ? count_of(forced, "\r\n") == count_of(baseline, "\r\n") &&
                    count_of(forced, "\n") == count_of(baseline, "\r\n") : forced.find('\r') == std::string::npos),
                "Force overwrites only a reader-valid shared-paper conflict, adopts unrelated external bytes and preserves EOLs");held();
            const auto forced_values = expected;
            set(index, "RED");expected = forced_values;
            require(impl.execute("tune.reset " + path).ok && document->edits().empty() &&
                document->current().text() == forced && read_text(source) == forced && read_text(staged) == forced,
                "Selected reset uses the field-valid saved opaque token");held();

            // The pair is saved together using distinct literal tokens. This
            // independent span oracle cannot silently rewrite its counterpart.
            set(hud_paper_ink_colour_index, "RED");set(hud_paper_paper_colour_index, "35BCE1FF");
            require(document->edits().size() == 2, "Both distinct shared-paper material tokens are pending");
            const std::string pair_external = (crlf ? std::string("\r\n") : std::string("\n")) + forced;
            const JsonSpanDocument pair_reference(pair_external, "hud-paper-pair-first-reference");
            const std::string first = pair_reference.replaced(*pair_reference.find("hud.paper.ink"), "\"RED\"");
            const JsonSpanDocument first_reference(first, "hud-paper-pair-second-reference");
            const std::string pair_saved = first_reference.replaced(*first_reference.find("hud.paper.paper"), "\"35BCE1FF\"");
            write_text(source, pair_external);
            require(impl.execute("tune.save presentation").ok && document->edits().empty() &&
                document->current().text() == pair_saved && document->baseline().text() == pair_saved &&
                read_text(source) == pair_saved && read_text(staged) == pair_saved &&
                colour_values(Presentation(source.string().c_str())) == expected && no_temporary(),
                "A distinct shared-paper pair Save changes exactly two tokens, preserves external whitespace and rebases both ledgers");held();
            const auto pair_values = expected;
            const auto restore_pair = [&]
            {
                write_text(source, pair_saved);expected = pair_values;
                require(impl.execute("tune.reset presentation").ok && document->edits().empty() &&
                    document->current().text() == pair_saved && document->baseline().text() == pair_saved &&
                    read_text(staged) == pair_saved && no_temporary(),
                    "A whole-document reset restores the valid saved pair without adopting external content");held();
            };
            // Equal decoded values refuse even when the opposite token spells
            // the same RGBA differently. The shade has no such distinctness rule.
            refuse("tune " + path + (paper ? " FF0000FF" : " 35bCe1FF"));
            set(index, "BLUE");set(sibling_index, "GREEN");set(3, "789ABC");
            expected[index] = pair_values[index];
            require(impl.execute("tune.reset " + path).ok && document->edits().size() == 2 &&
                !document->edits().contains(local) && document->edits().contains(sibling_local) &&
                document->edits().contains("debug_overlay.colour"),
                "Successful selected paper reset retains the independent sibling and overlay ledger");held();restore_pair();

            // Current pair is valid; reverting selected would restore its saved
            // value onto a sibling now carrying that same RGBA via another lexeme.
            set(index, "BLUE");set(sibling_index, paper ? "35bCe1FF" : "FF0000FF");
            require(document->edits().size() == 2 && document->edits().contains(local) &&
                document->edits().contains(sibling_local), "Reset collision has both real pending pair edits");
            refuse("tune.reset " + path);restore_pair();

            // Disk and current are each valid, but merged selected equals a
            // genuinely unedited external counterpart. Force cannot bypass this.
            set(index, "2060C0FF");set(3, "789ABC");
            require(document->edits().size() == 2 && document->edits().contains(local) &&
                !document->edits().contains(sibling_local), "Equality merge counterpart is genuinely unedited");
            const JsonSpanDocument pair_disk(pair_saved, "hud-paper-equality-merge-reference");
            const std::string equal_disk = pair_disk.replaced(*pair_disk.find(sibling_local), "\"2060C0\"");
            write_text(source, equal_disk);
            const auto disk_values = colour_values(Presentation(source.string().c_str()));
            require(disk_values[index] == pair_values[index] && disk_values[sibling_index] == labrador::Colour("2060C0FF") &&
                disk_values[hud_paper_ink_colour_index] != disk_values[hud_paper_paper_colour_index] &&
                expected[hud_paper_ink_colour_index] != expected[hud_paper_paper_colour_index],
                "Individually valid disk and current pairs produce an invalid equal-colour merged candidate");
            for (const char* command : { "tune.save presentation", "tune.save presentation force" }) { refuse(command); }
            restore_pair();

            // After resetting selected, only sibling/overlay are pending. A valid
            // external selected token now collides with the pending sibling.
            set(index, "BLUE");set(sibling_index, "GREEN");set(3, "789ABC");
            expected[index] = pair_values[index];
            require(impl.execute("tune.reset " + path).ok && document->edits().size() == 2 &&
                !document->edits().contains(local) && document->edits().contains(sibling_local),
                "Selected reset leaves only sibling and overlay pending before external equality");held();
            const std::string selected_external = pair_disk.replaced(*pair_disk.find(local), "\"008000FF\"");
            write_text(source, selected_external);
            const auto reset_disk_values = colour_values(Presentation(source.string().c_str()));
            require(reset_disk_values[index] == labrador::Colour::green && reset_disk_values[sibling_index] == pair_values[sibling_index] &&
                reset_disk_values[hud_paper_ink_colour_index] != reset_disk_values[hud_paper_paper_colour_index] &&
                expected[sibling_index] == labrador::Colour::green &&
                expected[hud_paper_ink_colour_index] != expected[hud_paper_paper_colour_index],
                "External selected-after-reset source is valid alone; only the sibling splice creates equality");
            for (const char* command : { "tune.save presentation", "tune.save presentation force" }) { refuse(command); }
            restore_pair();
        }
    }
}
