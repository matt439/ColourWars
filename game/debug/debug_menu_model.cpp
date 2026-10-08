#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_menu_model.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/tune_scope.h"

using Kind = DebugMenuRow::Kind;
using Form = DebugMenuTraits::Form;

DebugMenuModel::DebugMenuModel(DebugDirectorImpl& director) : director_(director)
{
}

const DebugEntry* DebugMenuModel::entry(const std::string& name) const
{
	return this->director_.registry().find(name);
}

std::vector<DebugMenuRow> DebugMenuModel::rows() const
{
	std::vector<DebugMenuRow> rows = this->collect();
	if (rows.size() > max_rows)
	{
		// The page lays out a fixed column; the rest stays reachable by typing.
		const std::size_t hidden = rows.size() - (max_rows - 1);
		rows.resize(max_rows - 1);
		rows.push_back({ Kind::note, "... " + std::to_string(hidden) + " more: use the console",
			"", "", false });
	}
	return rows;
}

std::vector<DebugMenuRow> DebugMenuModel::collect() const
{
	std::vector<DebugMenuRow> rows;
	const DebugRegistry& registry = this->director_.registry();

	if (!this->picker_.empty())
	{
		const DebugEntry* picked = this->entry(this->picker_);
		std::vector<std::string> choices;
		std::string head = this->picker_;
		std::string current;
		if (picked != nullptr)
		{
			const DebugMenuTraits& traits = picked->menu();
			try
			{
				choices = picked->suggestions(picked->is_variable() ? 0 : traits.pick_argument);
			}
			catch (const std::exception&)
			{
				choices.clear();
			}
			if (picked->is_variable())
			{
				current = static_cast<const DebugVariable*>(picked)->value_text();
			}
			else if (!traits.prefix.empty())
			{
				head += " " + traits.prefix;
			}
		}
		for (const std::string& choice : choices)
		{
			rows.push_back({ Kind::choice, choice, choice == current ? "current" : "",
				head + " " + choice, true });
		}
		if (rows.empty())
		{
			rows.push_back({ Kind::note, "(nothing to choose here)", "", "", false });
		}
		return rows;
	}

	if (this->category_ == "tuning")
	{
		return this->tuning_rows();
	}

	if (!this->category_.empty())
	{
		for (const DebugEntry* listed : registry.entries(this->category_))
		{
			DebugMenuRow row;
			row.label = listed->name();
			switch (listed->menu().form)
			{
			case Form::variable:
				row.kind = Kind::variable;
				row.value = static_cast<const DebugVariable*>(listed)->value_text();
				break;
			case Form::pick:
				row.kind = Kind::pick;
				row.value = ">";
				break;
			default:
				row.kind = Kind::run;
				break;
			}
			rows.push_back(row);
		}
		return rows;
	}

	const MatchRequest& request = this->director_.request();
	MatchRequest staged = request;
	staged.restart = false;
	if (!staged.empty())
	{
		DebugMenuRow apply{ Kind::apply, "", "", "", this->director_.can_apply() };
		if (this->director_.in_match())
		{
			apply.label = "Apply: " + request.describe();
		}
		else
		{
			apply.label = apply.enabled ? "Apply: start " + request.describe() :
				"Apply: pick a stage";
		}
		rows.push_back(apply);
		rows.push_back({ Kind::discard, "Discard queued", "", "", true });
	}
	for (const std::string& category : registry.categories())
	{
		rows.push_back({ Kind::category, category, ">", "", true });
	}
	return rows;
}

std::string DebugMenuModel::title() const
{
	std::string title = "DEBUG";
	if (this->category_ == "tuning" && !this->tune_node_.empty())
	{
		// The node's last two segments, which for a top-level group are its
		// file and its name: "DEBUG / tuning / player", "DEBUG /
		// movement_feedback / takeoff". Deeper paths would not fit the panel.
		std::string node = this->tune_node_;
		node.pop_back();
		std::vector<std::string> segments(1);
		for (const char c : node)
		{
			if (c == '.' || c == '[')
			{
				segments.emplace_back(c == '[' ? "[" : "");
				continue;
			}
			segments.back() += c;
		}
		if (this->tune_node_.back() == '[')
		{
			segments.back() += "[]";
		}
		const std::size_t first = segments.size() > 2 ? segments.size() - 2 : 0;
		for (std::size_t i = first; i < segments.size(); ++i)
		{
			title += " / " + segments[i];
		}
	}
	else if (!this->category_.empty())
	{
		title += " / " + this->category_;
	}

	if (!this->picker_.empty())
	{
		title += " / " + this->picker_;
	}
	return title;
}

std::string DebugMenuModel::queued() const
{
	MatchRequest staged = this->director_.request();
	staged.restart = false;
	return staged.empty() ? std::string() : "Queued: " + staged.describe();
}

void DebugMenuModel::enter(std::string category, std::string picker)
{
	if (category != this->category_)
	{
		this->tune_node_.clear();
		this->tune_page_ = 0;
	}
	this->category_ = std::move(category);
	this->picker_ = std::move(picker);
	this->focus_hint_ = 0;
	++this->level_;
}

void DebugMenuModel::enter_tune_node(std::string node, const std::string& focus_line)
{
	this->tune_node_ = std::move(node);
	this->tune_page_ = 0;
	this->focus_hint_ = 0;
	++this->level_;
	if (focus_line.empty())
	{
		return;
	}
	// Back lands on the row it came from, on whichever page holds it.
	for (std::size_t page = 0; page < 64; ++page)
	{
		this->tune_page_ = page;
		const std::vector<DebugMenuRow> rows = this->tuning_rows();
		for (std::size_t i = 0; i < rows.size(); ++i)
		{
			if (rows[i].line == focus_line)
			{
				this->focus_hint_ = i;
				return;
			}
		}
		if (rows.empty() || rows.back().kind != Kind::more)
		{
			break;
		}
	}
	this->tune_page_ = 0;
}

std::vector<DebugMenuRow> DebugMenuModel::tuning_rows() const
{
	std::vector<DebugMenuRow> rows;
	if (this->tune_node_.empty())
	{
		// The two files, then tune.diff, tune.reset and tune.save.
		for (const std::string& child : tune_children(this->director_.tune_paths(), ""))
		{
			DebugMenuRow row{ Kind::category, child.substr(0, child.size() - 1), ">", child, true };
			rows.push_back(row);
		}
		for (const DebugEntry* listed : this->director_.registry().entries("tuning"))
		{
			rows.push_back({ Kind::run, listed->name(), "", "", true });
		}
		if (rows.empty())
		{
			rows.push_back({ Kind::note, "(tuning.json and presentation.json could not be read)",
				"", "", false });
		}
		return rows;
	}

	std::vector<DebugMenuRow> all;
	for (const std::string& child : tune_children(this->director_.tune_paths(), this->tune_node_))
	{
		std::string label = child.substr(this->tune_node_.size());
		const char last = label.empty() ? ' ' : label.back();
		if (last == '.' || last == '[')
		{
			label.pop_back();
			if (this->tune_node_.back() == '[')
			{
				label = "[" + label;
			}
			all.push_back({ Kind::category, label + (last == '[' ? "[]" : ""), ">", child, true });
			continue;
		}
		if (this->tune_node_.back() == '[')
		{
			label = "[" + label;
		}
		const TuneScope scope = tune_scope_of(child);
		std::string value = this->director_.tune_token(child);
		if (scope != TuneScope::live)
		{
			value += std::string("  *") + tune_scope_name(scope);
		}
		all.push_back({ Kind::variable, label, value, "tune " + child, true });
	}

	if (all.size() <= max_rows)
	{
		return all;
	}
	const std::size_t per_page = max_rows - 1;
	const std::size_t pages = (all.size() + per_page - 1) / per_page;
	const std::size_t page = std::min(this->tune_page_, pages - 1);
	for (std::size_t i = page * per_page; i < all.size() && i < (page + 1) * per_page; ++i)
	{
		rows.push_back(all[i]);
	}
	rows.push_back({ Kind::more, "more (" + std::to_string(page + 1) + "/" +
		std::to_string(pages) + ")", ">", "", true });
	return rows;
}

std::size_t DebugMenuModel::row_of(const std::string& label) const
{
	const std::vector<DebugMenuRow> rows = this->rows();
	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		if (rows[i].label == label)
		{
			return i;
		}
	}
	return 0;
}

DebugMenuModel::Result DebugMenuModel::run(const std::string& line, bool staged)
{
	this->director_.log().write(DebugSeverity::echo, "[menu] " + line);
	const DebugStatus status = this->director_.execute(line);
	return status.close_console && !staged ? Result::close : Result::stay;
}

DebugMenuModel::Result DebugMenuModel::confirm(std::size_t row)
{
	const std::vector<DebugMenuRow> rows = this->rows();
	if (row >= rows.size() || !rows[row].enabled)
	{
		return Result::stay;
	}
	const DebugMenuRow chosen = rows[row];
	const std::string category = this->category_;
	switch (chosen.kind)
	{
	case Kind::apply:
		return Result::close;
	case Kind::discard:
		this->director_.drop_request("discarded in the debug menu");
		this->enter({}, {});
		return Result::stay;
	case Kind::category:
		if (category == "tuning")
		{
			this->enter_tune_node(chosen.line, {});
		}
		else
		{
			this->enter(chosen.label, {});
		}
		return Result::stay;
	case Kind::more:
		++this->tune_page_;
		if (this->tuning_rows().back().label == chosen.label)
		{
			// That was the last page: wrap.
			this->tune_page_ = 0;
		}
		this->focus_hint_ = 0;
		++this->level_;
		return Result::stay;
	case Kind::variable:
	{
		if (category == "tuning")
		{
			return this->step(row, 1);
		}
		// A choice variable opens a picker on its current value; a bool or a
		// number steps forward.
		if (dynamic_cast<const ChoiceVariable*>(this->entry(chosen.label)) != nullptr)
		{
			this->enter(category, chosen.label);
			this->focus_hint_ = this->row_of(chosen.value);
			return Result::stay;
		}
		return this->step(row, 1);
	}
	case Kind::run:
	{
		const DebugEntry* listed = this->entry(chosen.label);
		return this->run(chosen.label, listed != nullptr && listed->menu().staged);
	}
	case Kind::pick:
		this->enter(category, chosen.label);
		return Result::stay;
	case Kind::choice:
	{
		const std::string picked = this->picker_;
		const DebugEntry* listed = this->entry(picked);
		const Result result = this->run(chosen.line, listed != nullptr && listed->menu().staged);
		this->enter(category, {});
		this->focus_hint_ = this->row_of(picked);
		return result;
	}
	case Kind::note:
	default:
		return Result::stay;
	}
}

DebugMenuModel::Result DebugMenuModel::step(std::size_t row, int direction)
{
	const std::vector<DebugMenuRow> rows = this->rows();
	if (row >= rows.size() || rows[row].kind != Kind::variable)
	{
		return Result::stay;
	}
	if (this->category_ == "tuning")
	{
		// A tuning leaf: its line is "tune <path>".
		const std::string& value = rows[row].value;
		const bool flag = value.rfind("true", 0) == 0 || value.rfind("false", 0) == 0;
		return this->run(rows[row].line + " " + (flag ? "toggle" : direction > 0 ? "next" : "prev"),
			false);
	}
	const DebugEntry* listed = this->entry(rows[row].label);
	if (listed == nullptr || !listed->is_variable())
	{
		return Result::stay;
	}
	const DebugVariable* variable = static_cast<const DebugVariable*>(listed);
	return this->run(variable->name() + " " + variable->step_word(direction),
		variable->menu().staged);
}

DebugMenuModel::Result DebugMenuModel::back()
{
	if (!this->picker_.empty())
	{
		const std::string picked = this->picker_;
		const std::string category = this->category_;
		this->enter(category, {});
		this->focus_hint_ = this->row_of(picked);
		return Result::stay;
	}
	if (this->category_ == "tuning" && !this->tune_node_.empty())
	{
		// Up one segment: "tuning.player." -> "tuning.", "tuning." -> "".
		const std::string from = this->tune_node_;
		std::string node = from.substr(0, from.size() - 1);
		const std::size_t cut = node.find_last_of(".[");
		node = cut == std::string::npos ? std::string() : node.substr(0, cut + 1);
		this->enter_tune_node(node, from);
		return Result::stay;
	}
	if (!this->category_.empty())
	{
		const std::string category = this->category_;
		this->enter({}, {});
		// Back from staging lands on Apply, the obvious next step.
		const std::vector<DebugMenuRow> rows = this->rows();
		const bool apply = !rows.empty() && rows.front().kind == Kind::apply &&
			rows.front().enabled;
		this->focus_hint_ = apply ? 0 : this->row_of(category);
		return Result::stay;
	}
	return Result::close;
}
#endif
