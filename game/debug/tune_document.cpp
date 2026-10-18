#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/tune_document.h"
#include "game/debug/tune_scope.h"
#include <cstdio>
#include <stdexcept>

namespace
{
	bool tunable(const std::string& name, const JsonLeaf& leaf)
	{
		return leaf.kind == JsonLeaf::Kind::number || leaf.kind == JsonLeaf::Kind::boolean ||
			(leaf.kind == JsonLeaf::Kind::string && leaf.plain_path &&
				tune_colour_path(name + "." + leaf.path));
	}
}

std::uint64_t fnv1a(std::string_view bytes, std::uint64_t hash)
{
	for (const char c : bytes)
	{
		hash ^= static_cast<unsigned char>(c);
		hash *= 0x100000001b3ull;
	}
	return hash;
}

std::string hex64(std::uint64_t value)
{
	char buffer[20] = {};
	std::snprintf(buffer, sizeof(buffer), "%016llx", static_cast<unsigned long long>(value));
	return buffer;
}

TuneDocument::TuneDocument(std::string name, std::string baseline_text) :
	name_(name),
	baseline_(baseline_text, name + ".json"),
	current_(std::move(baseline_text), name + ".json")
{
}

const JsonLeaf* TuneDocument::find(std::string_view path) const
{
	const JsonLeaf* leaf = this->current_.find(path);
	return leaf != nullptr && tunable(this->name_, *leaf) ? leaf : nullptr;
}

std::vector<std::string> TuneDocument::prefixed_paths() const
{
	std::vector<std::string> paths;
	for (const JsonLeaf& leaf : this->current_.leaves())
	{
		if (this->find(leaf.path) != nullptr)
		{
			paths.push_back(this->name_ + "." + leaf.path);
		}
	}
	return paths;
}

std::string TuneDocument::edited(const JsonLeaf& leaf, std::string_view token) const
{
	const JsonLeaf* current = this->find(leaf.path);
	if (current == nullptr || current->kind != leaf.kind ||
		current->begin != leaf.begin || current->end != leaf.end)
	{
		throw std::invalid_argument(this->name_ + "." + leaf.path + ": not a current tunable leaf");
	}
	return this->current_.replaced(*current, token);
}

void TuneDocument::commit(std::string candidate_text, const std::string& path)
{
	JsonSpanDocument candidate(std::move(candidate_text), this->name_ + ".json");
	const JsonLeaf* now = candidate.find(path);
	const JsonLeaf* was = this->baseline_.find(path);
	const JsonLeaf* current = this->find(path);
	if (current == nullptr || now == nullptr || was == nullptr ||
		!tunable(this->name_, *now) || !tunable(this->name_, *was) ||
		now->kind != current->kind || was->kind != current->kind ||
		this->current_.replaced(*current, candidate.token(*now)) != candidate.text())
	{
		throw std::invalid_argument(this->name_ + "." + path +
			": a tune commit must replace only one existing tunable token of the same type");
	}
	const std::string token(candidate.token(*now));
	const std::string baseline(this->baseline_.token(*was));
	if (token == baseline)
	{
		this->edits_.erase(path);
	}
	else
	{
		this->edits_[path] = { baseline, token };
	}
	this->current_ = std::move(candidate);
}

std::string TuneDocument::reverted(const std::string& path) const
{
	if (path.empty())
	{
		return this->baseline_.text();
	}
	const JsonLeaf* leaf = this->find(path);
	const auto edit = this->edits_.find(path);
	if (leaf == nullptr || edit == this->edits_.end())
	{
		return this->current_.text();
	}
	return this->current_.replaced(*leaf, edit->second.baseline_token);
}

void TuneDocument::rebase(std::string text)
{
	JsonSpanDocument document(std::move(text), this->name_ + ".json");
	this->baseline_ = document;
	this->current_ = std::move(document);
	this->edits_.clear();
}

TuneSplice TuneDocument::splice(const std::string& disk_text, bool force) const
{
	const JsonSpanDocument disk(disk_text, this->name_ + ".json (on disk)");
	struct Target
	{
		const JsonLeaf* leaf;
		const std::string* token;
	};
	std::vector<Target> targets;
	TuneSplice result;
	for (const auto& [path, edit] : this->edits_)
	{
		const JsonLeaf* leaf = disk.find(path);
		if (leaf == nullptr || !tunable(this->name_, *leaf))
		{
			result.conflicts.push_back(this->name_ + "." + path +
				": the file on disk no longer has it");
			continue;
		}
		const std::string on_disk(disk.token(*leaf));
		if (on_disk != edit.baseline_token && !force)
		{
			result.conflicts.push_back(this->name_ + "." + path + ": changed on disk to " +
				on_disk + " since it was loaded as " + edit.baseline_token);
			continue;
		}
		targets.push_back({ leaf, &edit.current_token });
	}
	std::sort(targets.begin(), targets.end(),
		[](const Target& a, const Target& b) { return a.leaf->begin > b.leaf->begin; });
	result.text = disk_text;
	for (const Target& target : targets)
	{
		result.text.replace(target.leaf->begin, target.leaf->end - target.leaf->begin,
			*target.token);
		++result.written;
	}
	return result;
}

std::uint64_t TuneDocument::current_hash() const
{
	return fnv1a(this->current_.text());
}
#endif
