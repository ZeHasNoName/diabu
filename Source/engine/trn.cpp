#include "engine/trn.hpp"

#include <algorithm>
#include <cstdint>
#include <unordered_map>

#include <fmt/format.h>

#ifdef _DEBUG
#include "debug.h"
#endif
#include "engine/load_file.hpp"
#include "engine/palette.h"
#include "lighting.h"

namespace devilution {

uint8_t *GetInfravisionTRN()
{
	return InfravisionTable.data();
}

uint8_t *GetStoneTRN()
{
	return StoneTable.data();
}

uint8_t *GetPauseTRN()
{
	return PauseTable.data();
}

std::optional<std::array<uint8_t, 256>> GetClassTRN(Player &player)
{
	std::array<uint8_t, 256> trn;
	const char *path;
	bool useNecromancerPalette = false;

	switch (player._pClass) {
	case HeroClass::Warrior:
		path = "plrgfx\\warrior.trn";
		break;
	case HeroClass::Rogue:
		path = "plrgfx\\rogue.trn";
		break;
	case HeroClass::Sorcerer:
		path = "plrgfx\\sorcerer.trn";
		break;
	case HeroClass::Necromancer:
		path = "plrgfx\\necromancer.trn";
		useNecromancerPalette = true;
		break;
	case HeroClass::Monk:
		path = "plrgfx\\monk.trn";
		break;
	case HeroClass::Bard:
		path = "plrgfx\\bard.trn";
		break;
	case HeroClass::Barbarian:
		path = "plrgfx\\barbarian.trn";
		break;
	}

#ifdef _DEBUG
	if (!debugTRN.empty()) {
		path = debugTRN.c_str();
	}
#endif
	if (LoadOptionalFileInMem(path, &trn[0], 256)) {
		return trn;
	}
	if (useNecromancerPalette) {
		for (size_t i = 0; i < trn.size(); ++i)
			trn[i] = static_cast<uint8_t>(i);

		// Player graphics use the fixed red ramps for the Sorcerer's robe. The
		// fixed blue ramp is the closest palette-safe purple and remains stable
		// across dungeon palettes. Offset it toward its dark end for the
		// Necromancer, while leaving skin, equipment, and effects untouched.
		for (uint8_t i = 0; i < 16; ++i)
			trn[PAL16_RED + i] = PAL16_BLUE + std::min<uint8_t>(i + 4, 15);
		for (uint8_t i = 0; i < 8; ++i)
			trn[PAL8_RED + i] = PAL16_BLUE + std::min<uint8_t>(i * 2 + 4, 15);
		return trn;
	}
	return std::nullopt;
}

} // namespace devilution
