#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/core/file_path.h"
#include "game/debug/debug_binds.h"
#include <iostream>

namespace debug_console_acceptance
{
	namespace
	{
		// The only handle opened here belongs to a private ScratchFolder. Permit
		// reads and writes, but keep deletion blocked until this owner releases it.
		class PrivateBindReadLock
		{
		public:
			explicit PrivateBindReadLock(const std::filesystem::path& path) :
				handle_(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
					nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr))
			{
				require(this->handle_ != INVALID_HANDLE_VALUE,
					"DBG10 could not lock the private binds.cfg against deletion");
			}

			~PrivateBindReadLock()
			{
				if (this->handle_ != INVALID_HANDLE_VALUE) { CloseHandle(this->handle_); }
			}

			void release()
			{
				require(this->handle_ != INVALID_HANDLE_VALUE && CloseHandle(this->handle_) != 0,
					"DBG10 could not release its private binds.cfg handle");
				this->handle_ = INVALID_HANDLE_VALUE;
			}

			PrivateBindReadLock(const PrivateBindReadLock&) = delete;
			PrivateBindReadLock& operator=(const PrivateBindReadLock&) = delete;

		private:
			HANDLE handle_ = INVALID_HANDLE_VALUE;
		};

		bool has_warning(const DebugLog& log)
		{
			for (const DebugLogLine& line : log.lines())
			{
				if (line.severity == DebugSeverity::warning) { return true; }
			}
			return false;
		}

		void check_default_persistence(GameTuning& tuning, Presentation& presentation,
			bool reset_command)
		{
			ScratchFolder scratch(reset_command ? L"bind_reset_locked" : L"bind_converge_locked");
			const std::filesystem::path file = scratch.root / L"binds.cfg";
			TestDirector writer(tuning, presentation);
			writer.impl.set_user_directory(scratch.root);
			writer.director.run_launch_arguments({});
			require(writer.impl.execute("bind f12 echo retained-private-override").ok,
				"DBG10 could not save its private F12 override");
			const DebugBinds saved_table = writer.impl.binds();
			const std::string saved_bytes = read_text(file);
			require(saved_table != DebugBinds::defaults() &&
				saved_bytes.find("bind f12") != std::string::npos,
				"DBG10 private fixture has no persisted override");

			// Declared after scratch: every exit closes the owned handle before
			// ScratchFolder removes its private files, including a failed assertion.
			PrivateBindReadLock lock(file);
			writer.impl.log().clear();
			const char* command = reset_command ? "binds.reset" : "unbind f12";
			require(writer.impl.execute(command).ok,
				"DBG10 restoring defaults must still apply to this session");
			require(writer.impl.binds() == DebugBinds::defaults() && read_text(file) == saved_bytes,
				"DBG10 blocked removal changed the saved bytes or failed to apply current defaults");
			// The first new regression reaches the real reset and the blocked
			// private file before demanding the previously missing warning.
			const std::string expected_warning = "binds: cannot remove " + labrador::path_to_utf8(file) +
				"; the change holds for this session";
			require(log_contains(writer.impl.log(), expected_warning, DebugSeverity::warning),
				"DBG10 blocked binds.cfg removal did not warn that the change holds for this session");
			{
				TestDirector locked_reload(tuning, presentation);
				locked_reload.impl.set_user_directory(scratch.root);
				locked_reload.director.run_launch_arguments({});
				require(locked_reload.impl.binds() == saved_table && read_text(file) == saved_bytes,
					"DBG10 a new private session did not retain the still-saved overrides");
			}

			lock.release();
			writer.impl.log().clear();
			require(writer.impl.execute("binds.reset").ok &&
				writer.impl.binds() == DebugBinds::defaults() && !std::filesystem::exists(file) &&
				!has_warning(writer.impl.log()),
				"DBG10 reset did not remove the private saved overrides cleanly after release");
			{
				TestDirector cleared_reload(tuning, presentation);
				cleared_reload.impl.set_user_directory(scratch.root);
				cleared_reload.director.run_launch_arguments({});
				require(cleared_reload.impl.binds() == DebugBinds::defaults() && !std::filesystem::exists(file),
					"DBG10 a new private session did not retain the successfully reset defaults");
			}

			writer.impl.log().clear();
			require(writer.impl.execute("binds.reset").ok &&
				writer.impl.binds() == DebugBinds::defaults() && !std::filesystem::exists(file) &&
				!has_warning(writer.impl.log()),
				"DBG10 an already absent binds.cfg produced a removal warning");
		}
	}

	void check_bind_persistence_failures(GameTuning& tuning, Presentation& presentation)
	{
		check_default_persistence(tuning, presentation, true);
		check_default_persistence(tuning, presentation, false);
		std::cout << "PASS binds persistence: private locked reset and unbind-to-defaults warn, "
			"apply current defaults and preserve saved overrides; release/retry removes the file, "
			"fresh sessions reload the expected table, absent files do not warn.\n";
	}
}
#endif
