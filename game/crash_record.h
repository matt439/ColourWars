#pragma once

// The crash record (REV#63): what last-error.log says when the process dies
// without reaching wWinMain's catch. An unhandled structured exception or a
// call to std::terminate on the main thread writes the same report_format=1
// record as a caught failure (fatal_report.h), with the crash phase, the
// exception code, the faulting module's class and the RVA that the shipped
// ColourWars.pdb symbolicates. docs/failure-reporting.md#crash-record-rev63
// has the fields, the module rule and the boundary.

#include "game/fatal_report.h"

#include <cstddef>
#include <string>

// Installs the unhandled-exception filter and the main thread's terminate
// handler, and computes everything they need, so a crash allocates nothing on
// the structured-exception path. Call once, first thing in wWinMain.
void install_crash_handlers() noexcept;

// The phase a crash record names; wWinMain moves it with the caught phase.
void set_crash_phase(FailurePhase phase) noexcept;

// game: the main image (the game and the statically linked engine, which
// the PDB tells apart). engine: one of the engine's runtime DLLs beside the
// executable. system: a module under the Windows directory, or a Microsoft
// runtime DLL beside the executable. other: anything else, never named.
enum class CrashModule
{
	game,
	engine,
	system,
	other
};

const char* crash_module_label(CrashModule module) noexcept;

// Classifies a loaded module by its full path. Pure: the handlers and the
// acceptance case share it. Paths compare without case, on whole directories.
CrashModule classify_crash_module(const wchar_t* module_path, const wchar_t* executable_path,
	const wchar_t* windows_directory) noexcept;

struct CrashDetails
{
	// "structured_exception" or "terminate".
	const char* kind = "structured_exception";
	bool has_code = false;
	unsigned long code = 0;
	CrashModule module = CrashModule::other;
	// The module's lower-case ASCII file name; written for engine and system only.
	char module_name[64]{};
	// Written for game, engine and system only: an RVA in an unnamed module
	// would say nothing a reader could use.
	bool has_rva = false;
	unsigned long long rva = 0;
};

// The whole record, allocation free. cause_fields is FatalReport::cause_fields()
// or null for none. Returns the length, or -1 when it does not fit.
int format_crash_record(char* buffer, std::size_t size, FailurePhase phase,
	const CrashDetails& details, const char* cause_fields) noexcept;

// The same record as a string, for checks.
std::string crash_record_text(FailurePhase phase, const CrashDetails& details,
	const std::string& cause_fields);
