#pragma once

#include <memory>
#include <string>
#include <string_view>

class GameData;
class GameTuning;
class Presentation;

// The live object one content file feeds, as live tuning reaches it: build
// the real object from a candidate file (so every rule the game checks at
// startup, cross-field ones included, judges the edit), then copy the
// candidate over the live object. Copy assignment keeps the address of every
// sub-record that weapons, projectiles, the HUD and movement feedback point
// at; only the edited number differs. Debug-only.
class TuneTarget
{
public:
	virtual ~TuneTarget() = default;

	TuneTarget(const TuneTarget&) = delete;
	TuneTarget& operator=(const TuneTarget&) = delete;

	// "tuning" or "presentation".
	virtual std::string_view file() const = 0;

	// Constructs the reader's object from `candidate_file` (UTF-8) and keeps
	// it. Throws the reader's own message. `context`, when it has a running
	// application, also runs validate_game_content for edits that reach level
	// construction (`checks_levels`), and otherwise the object's own
	// validate_resources: startup's checks of numbers against the loaded
	// frames and fonts.
	virtual void validate(const std::string& candidate_file, const GameData* context,
		bool checks_levels) = 0;

	// Copies the validated candidate over the live object. `keep_live_toggle`
	// keeps the F9 movement feedback switch as it is (every edit but one of
	// movement_feedback.enabled itself).
	virtual void apply(bool keep_live_toggle) = 0;

protected:
	TuneTarget() = default;
};

class TuningTarget final : public TuneTarget
{
public:
	TuningTarget(GameTuning* live, const Presentation* presentation);
	~TuningTarget() override;

	std::string_view file() const override { return "tuning"; }
	void validate(const std::string& candidate_file, const GameData* context,
		bool checks_levels) override;
	void apply(bool keep_live_toggle) override;

private:
	GameTuning* live_ = nullptr;
	const Presentation* presentation_ = nullptr;
	std::unique_ptr<GameTuning> candidate_;
};

class PresentationTarget final : public TuneTarget
{
public:
	PresentationTarget(Presentation* live, const GameTuning* tuning);
	~PresentationTarget() override;

	std::string_view file() const override { return "presentation"; }
	void validate(const std::string& candidate_file, const GameData* context,
		bool checks_levels) override;
	void apply(bool keep_live_toggle) override;

private:
	Presentation* live_ = nullptr;
	const GameTuning* tuning_ = nullptr;
	std::unique_ptr<Presentation> candidate_;
};
