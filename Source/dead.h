/**
 * @file dead.h
 *
 * Interface of functions for placing dead monsters.
 */
#pragma once

#include <array>
#include <cstdint>

#include "engine.h"
#include "engine/clx_sprite.hpp"
#include "engine/point.hpp"
#include "monstdat.h"

namespace devilution {

static constexpr unsigned MaxCorpses = 31;

struct Corpse {
	OptionalClxSpriteListOrSheet sprites;
	int frame;
	uint16_t width;
	uint8_t translationPaletteIndex;
	_monster_id monsterType = MT_INVALID;
	MonsterClass monsterClass = MonsterClass::Undead;
	bool canRaise = false;

	/**
	 * @brief Returns the sprite list for a given direction.
	 *
	 * @param direction One of the 16 directions. Valid range: [0, 15].
	 * @return ClxSpriteList
	 */
	[[nodiscard]] ClxSpriteList spritesForDirection(Direction direction) const
	{
		return sprites->isSheet() ? sprites->sheet()[static_cast<size_t>(direction)] : sprites->list();
	}
};

extern Corpse Corpses[MaxCorpses];
extern int8_t stonendx;

void InitCorpses();
void AddCorpse(Point tilePosition, int8_t dv, Direction ddir);
const Corpse *GetCorpseAt(Point tilePosition);
bool IsCorpseRaiseable(Point tilePosition);
void ConsumeCorpse(Point tilePosition);
void MoveLightsToCorpses();

} // namespace devilution
