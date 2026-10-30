#include "game/pch.h"
#include "game/stage_catalog.h"

#include <algorithm>
#include <iterator>
#include <utility>

using namespace labrador;

StageCatalog::StageCatalog() :
	level_infos_("Level")
{
}

const StageList& StageCatalog::stages() const
{
	return this->stages_;
}

const Registry<LevelLoadedInfo>& StageCatalog::level_infos() const
{
	return this->level_infos_;
}

const LevelLoadedInfo* StageCatalog::definition(const std::string& name) const
{
	const bool listed = std::any_of(this->stages_.begin(), this->stages_.end(),
		[&name](const Stage& stage) { return stage.name == name; });
	return listed && this->level_infos_.contains(name) ? this->level_infos_.get(name) : nullptr;
}

void StageCatalog::add(StageDefinition definition)
{
	if (definition.info != nullptr)
	{
		this->level_infos_.add(definition.stage.name, std::move(definition.info));
	}
	this->stages_.push_back(std::move(definition.stage));
}

void StageCatalog::remove_user_stages()
{
	// The user stages are the tail; the manifest's are never reread.
	this->stages_.erase(std::remove_if(this->stages_.begin(), this->stages_.end(),
		[](const Stage& stage) { return stage.user; }), this->stages_.end());
}

void StageCatalog::restore_user_stages(const StageList& previous)
{
	this->remove_user_stages();
	std::copy_if(previous.begin(), previous.end(), std::back_inserter(this->stages_),
		[](const Stage& stage) { return stage.user; });
}
