#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_binds.h"
#include "game/debug/debug_script.h"
#include "game/debug/debug_tokenizer.h"
#include <array>
#include <iostream>

namespace debug_console_acceptance
{
	namespace
	{
		std::vector<std::string> info_lines(const DebugLog& log)
		{
			std::vector<std::string> result;
			for (const DebugLogLine& line : log.lines())
			{
				if (line.severity == DebugSeverity::info) { result.push_back(line.text); }
			}
			return result;
		}

		void check_saved_word(GameTuning& tuning, Presentation& presentation, std::size_t index,
			const std::string& command, const std::vector<std::vector<std::string>>& expected_words,
			const std::vector<std::string>& expected_output)
		{
			using labrador::Key;
			ScratchFolder scratch(L"bind_word_" + std::to_wstring(index));
			TestDirector writer(tuning, presentation);
			writer.impl.set_user_directory(scratch.root);
			writer.director.run_launch_arguments({});
			require(writer.impl.execute("bind f8 echo unchanged-sibling").ok &&
				writer.impl.execute("unbind f4").ok && writer.impl.execute(command).ok,
				"DBG9 actual bind command was not accepted for case " + std::to_string(index));
			const std::string* bound = writer.impl.binds().find(KeyChord{ Key::f12 });
			require(bound != nullptr, "DBG9 accepted bind did not install its F12 line");
			const auto statements = split_statements(*bound);
			require(statements.size() == expected_words.size(), "DBG9 bind changed its statement count");
			for (std::size_t statement = 0; statement < statements.size(); ++statement)
			{
				const DebugTokens tokens = tokenize(statements[statement]);
				require(tokens.ok && tokens.words == expected_words[statement],
					"DBG9 accepted bind changed its decoded words for case " + std::to_string(index));
			}
			const DebugBinds expected = writer.impl.binds();
			const std::filesystem::path file = scratch.root / L"binds.cfg";
			const std::filesystem::path bad = scratch.root / L"binds.cfg.bad";
			const std::string saved = read_text(file);
			TestDirector reloaded(tuning, presentation);
			reloaded.impl.set_user_directory(scratch.root);
			reloaded.director.run_launch_arguments({});
			// This is deliberately the first new regression: the old encoder accepts
			// the bind, then writes a file whose terminal slash consumes its quote.
			require(reloaded.impl.binds() == expected && std::filesystem::exists(file) &&
				!std::filesystem::exists(bad),
				"DBG9 accepted trailing-backslash bind did not round-trip its complete table; case " +
				std::to_string(index) + ", generated .bad=" + (std::filesystem::exists(bad) ? "yes" : "no"));
			require(read_text(file) == saved, "DBG9 loading bindings rewrote the persisted bytes");
			require(reloaded.impl.binds().find(KeyChord{ Key::f4 }) == nullptr &&
				reloaded.impl.binds().find(KeyChord{ Key::f8 }) != nullptr &&
				*reloaded.impl.binds().find(KeyChord{ Key::f8 }) == "echo unchanged-sibling",
				"DBG9 round trip lost a sibling binding or restored a deliberately removed default");
			if (!expected_output.empty())
			{
				reloaded.impl.log().clear();
				require(reloaded.impl.execute(*reloaded.impl.binds().find(KeyChord{ Key::f12 })).ok &&
					info_lines(reloaded.impl.log()) == expected_output,
					"DBG9 reloaded bind changed actual echo output or statement order");
			}
		}
	}

	void check_bind_word_round_trips(GameTuning& tuning, Presentation& presentation)
	{
		// Baseline must reach real persistence before any direct encoder check.
		check_saved_word(tuning, presentation, 0, R"(bind f12 echo C:\temp\)",
			{ { "echo", R"(C:\temp\)" } }, { R"(C:\temp\)" });
		check_saved_word(tuning, presentation, 1, R"(bind f12 echo "C:\my logs"\)",
			{ { "echo", R"(C:\my logs\)" } }, { R"(C:\my logs\)" });
		check_saved_word(tuning, presentation, 2, R"(bind f12 "echo cost $1 $$"\)",
			{ { "echo", "cost", "$1", R"($$\)" } }, { R"(cost $1 $$\)" });
		check_saved_word(tuning, presentation, 3, R"(bind f12 echo https://paint.invalid/review\)",
			{ { "echo", R"(https://paint.invalid/review\)" } }, { R"(https://paint.invalid/review\)" });
		// A one-word line is accepted even when it names no registered command.
		// It specifically exercises saved_line's forced // quoting branch.
		check_saved_word(tuning, presentation, 4, R"(bind f12 echo//note\)",
			{ { R"(echo//note\)" } }, {});
		check_saved_word(tuning, presentation, 5, R"(bind f12 "echo \"quoted value\"; echo C:\tail"\)",
			{ { "echo", "quoted value" }, { "echo", R"(C:\tail\)" } }, { "quoted value", R"(C:\tail\)" });
		check_saved_word(tuning, presentation, 6, R"(bind f12 "echo \"quoted // value\"; echo $$ $1 C:\tail"\)",
			{ { "echo", "quoted // value" }, { "echo", "$$", "$1", R"(C:\tail\)" } },
			{ "quoted // value", R"($$ $1 C:\tail\)" });

		const std::array<std::string, 8> prefixes{ "", "plain", "two words", "say \"hi\"",
			"semi;colon", "https://paint.invalid/review", "with \"quote\" and space; // text", R"(C:\interior\path)" };
		std::size_t word_cases = 0;
		for (const std::string& prefix : prefixes)
		{
			for (std::size_t slashes = 0; slashes <= 4; ++slashes)
			{
				const std::string word = prefix + std::string(slashes, '\\');
				const std::string encoded = quote_word(word);
				const DebugTokens tokens = tokenize(encoded);
				require(tokens.ok && tokens.words == std::vector<std::string>{ word },
					"DBG9 quote_word did not preserve an admitted word and terminal slash count");
				const auto statements = split_statements("echo " + encoded + "; echo after-word");
				require(statements.size() == 2 &&
					tokenize(statements[0]).words == std::vector<std::string>{ "echo", word } &&
					tokenize(statements[1]).words == std::vector<std::string>{ "echo", "after-word" },
					"DBG9 encoded word swallowed or added a statement delimiter");
				++word_cases;
			}
		}

		ScratchFolder launch_scratch(L"word_launch");
		TestDirector launch(tuning, presentation);
		launch.impl.set_user_directory(launch_scratch.root);
		launch.director.run_launch_arguments({});
		launch.impl.log().clear();
		launch.director.run_launch_arguments({ L"+echo", L"C:\\my logs\\", L"+echo", L"after-trailing-path" });
		require(info_lines(launch.impl.log()) == std::vector<std::string>{ R"(C:\my logs\)", "after-trailing-path" },
			"DBG9 wide launch argument did not preserve its spaced path and following statement");
		require(!std::filesystem::exists(launch_scratch.root / L"binds.cfg") &&
			!std::filesystem::exists(launch_scratch.root / L"binds.cfg.bad"),
			"DBG9 echo-only launch unexpectedly changed persistent bindings");

		require(!tokenize(R"(echo "unterminated)").ok && !tokenize("echo caf\xc3\xa9").ok &&
			!tokenize("echo \"tab\tvalue\"").ok &&
			tokenize("echo\tvalue").words == std::vector<std::string>{ "echo", "value" },
			"DBG9 changed malformed quote, non-ASCII or tab admission policy");
		require(!parse_script("word-refusals.cfg", "echo \"unterminated\n", {}).ok &&
			!parse_script("word-refusals.cfg", "echo caf\xc3\xa9\n", {}).ok,
			"DBG9 changed script malformed/non-ASCII refusals");
		std::cout << "PASS DBG9 bind words: 7 actual private save/reload tables, sibling/removal/byte invariants, "
			<< word_cases << " decoded word/statement round trips, wide launch echo and unchanged refusals\n";
	}
}
#endif
