#include "panels/spell_book.hpp"

#include <algorithm>
#include <cstdint>

#include <fmt/format.h>

#include "control.h"
#include "engine/backbuffer_state.hpp"
#include "engine/clx_sprite.hpp"
#include "engine/load_cel.hpp"
#include "engine/load_clx.hpp"
#include "engine/rectangle.hpp"
#include "engine/render/clx_render.hpp"
#include "engine/render/primitive_render.hpp"
#include "engine/render/text_render.hpp"
#include "init.h"
#include "missiles.h"
#include "monster.h"
#include "panels/spell_icons.hpp"
#include "panels/ui_panels.hpp"
#include "player.h"
#include "spelldat.h"
#include "utils/language.h"
#include "utils/stdcompat/optional.hpp"

namespace devilution {

namespace {

OptionalOwnedClxSpriteList pSBkBtnCel;
OptionalOwnedClxSpriteList pSpellBkCel;

const size_t SpellBookPages = 6;
const size_t SpellBookPageEntries = 7;
constexpr int SpellBookTabsWidth = 305;
constexpr int SpellBookTabsHeight = 29;
constexpr Point SpellBookTabsPosition { 7, 320 };

/** Maps from spellbook page number and position to SpellID. */
const SpellID SpellPages[SpellBookPages][SpellBookPageEntries] = {
	// Miscellaneous skills and utility spells.
	{ SpellID::Null, SpellID::Healing, SpellID::HealOther, SpellID::Resurrect, SpellID::TownPortal, SpellID::Telekinesis, SpellID::Search },
	// Fire spells.
	{ SpellID::Firebolt, SpellID::Inferno, SpellID::FireWall, SpellID::Elemental, SpellID::Fireball, SpellID::FlameWave, SpellID::Guardian },
	// Advanced fire spells and thematically related destructive magic.
	{ SpellID::Golem, SpellID::Apocalypse, SpellID::Immolation, SpellID::RingOfFire, SpellID::BloodStar, SpellID::BoneSpirit, SpellID::StoneCurse },
	// Lightning spells, with Mana Shield and Nova filling the remaining slots.
	{ SpellID::ChargedBolt, SpellID::Lightning, SpellID::Flash, SpellID::ChainLightning, SpellID::LightningWall, SpellID::ManaShield, SpellID::Nova },
	// Magic spells.
	{ SpellID::HolyBolt, SpellID::Phasing, SpellID::Teleport, SpellID::Etherealize, SpellID::Warp, SpellID::Reflect, SpellID::Berserk },
	// Reserved for mod spells.
	{ SpellID::HolyNova, SpellID::Invalid, SpellID::Invalid, SpellID::Invalid, SpellID::Invalid, SpellID::Invalid, SpellID::Invalid }
};

void DrawSpellBookTabs(const Surface &out)
{
	const int sourceTabCount = gbIsHellfire ? 5 : 4;
	const int sourceTabWidth = gbIsHellfire ? 61 : 76;
	const Point tabsPosition = GetPanelPosition(UiPanels::Spell, SpellBookTabsPosition);

	// The original background contains four or five tabs. Compress those tabs and
	// repeat the final one so all six pages have an equally sized button.
	OwnedSurface originalTabs(SpellBookTabsWidth, SpellBookTabsHeight);
	originalTabs.BlitFrom(out, MakeSdlRect(tabsPosition.x, tabsPosition.y, SpellBookTabsWidth, SpellBookTabsHeight), { 0, 0 });
	for (int tab = 0; tab < static_cast<int>(SpellBookPages); ++tab) {
		const int destinationBegin = tab * SpellBookTabsWidth / SpellBookPages;
		const int destinationEnd = (tab + 1) * SpellBookTabsWidth / SpellBookPages;
		const int sourceTab = std::min(tab, sourceTabCount - 1);
		for (int y = 0; y < SpellBookTabsHeight; ++y) {
			for (int x = destinationBegin; x < destinationEnd; ++x) {
				const int sourceX = sourceTab * sourceTabWidth
				    + (x - destinationBegin) * sourceTabWidth / (destinationEnd - destinationBegin);
				out[tabsPosition + Displacement { x, y }] = originalTabs[{ sourceX, y }];
			}
		}
	}

	const ClxSprite selectedTab = (*pSBkBtnCel)[std::min(sbooktab, sourceTabCount - 1)];
	OwnedSurface originalSelectedTab(sourceTabWidth, selectedTab.height());
	ClxDraw(originalSelectedTab, { 0, selectedTab.height() - 1 }, selectedTab);
	const int destinationBegin = sbooktab * SpellBookTabsWidth / SpellBookPages;
	const int destinationEnd = (sbooktab + 1) * SpellBookTabsWidth / SpellBookPages;
	for (int y = 0; y < selectedTab.height(); ++y) {
		for (int x = destinationBegin; x < destinationEnd; ++x) {
			const uint8_t color = originalSelectedTab[{ (x - destinationBegin) * sourceTabWidth / (destinationEnd - destinationBegin), y }];
			if (color != 0)
				out[tabsPosition + Displacement { x, y }] = color;
		}
	}

	// The sixth button reuses the fifth source tab, so replace its original numeral.
	const int modTabBegin = (SpellBookPages - 1) * SpellBookTabsWidth / SpellBookPages;
	const int modTabEnd = SpellBookTabsWidth;
	const Rectangle modTabLabel {
		tabsPosition + Displacement { modTabBegin, 4 },
		Size { modTabEnd - modTabBegin, SpellBookTabsHeight - 8 }
	};
	DrawHalfTransparentRectTo(out, modTabLabel.position.x + modTabLabel.size.width / 2 - 5, modTabLabel.position.y + 3, 10, modTabLabel.size.height - 6);
	DrawHalfTransparentRectTo(out, modTabLabel.position.x + modTabLabel.size.width / 2 - 5, modTabLabel.position.y + 3, 10, modTabLabel.size.height - 6);
	DrawString(out, "6", modTabLabel, { UiFlags::ColorGold | UiFlags::AlignCenter | UiFlags::VerticalCenter });
}

SpellID GetSpellFromSpellPage(size_t page, size_t entry)
{
	assert(page < SpellBookPages && entry < SpellBookPageEntries);
	if (page == 0 && entry == 0) {
		switch (InspectPlayer->_pClass) {
		case HeroClass::Warrior:
			return SpellID::ItemRepair;
		case HeroClass::Rogue:
			return SpellID::TrapDisarm;
		case HeroClass::Sorcerer:
			return SpellID::StaffRecharge;
		case HeroClass::Necromancer:
			return SpellID::RaiseUndead;
		case HeroClass::Monk:
			return SpellID::Search;
		case HeroClass::Bard:
			return SpellID::Identify;
		case HeroClass::Barbarian:
			return SpellID::Rage;
		}
	}
	return SpellPages[page][entry];
}

constexpr Size SpellBookDescription { 250, 43 };
constexpr int SpellBookDescriptionPaddingHorizontal = 2;

void PrintSBookStr(const Surface &out, Point position, string_view text, UiFlags flags = UiFlags::None)
{
	DrawString(out, text,
	    Rectangle(GetPanelPosition(UiPanels::Spell, position + Displacement { SPLICONLENGTH, 0 }),
	        SpellBookDescription)
	        .inset({ SpellBookDescriptionPaddingHorizontal, 0 }),
	    { UiFlags::ColorWhite | flags });
}

SpellType GetSBookTrans(SpellID ii, bool townok)
{
	Player &player = *InspectPlayer;
	if ((player._pClass == HeroClass::Monk) && (ii == SpellID::Search))
		return SpellType::Skill;
	SpellType st = SpellType::Spell;
	if ((player._pISpells & GetSpellBitmask(ii)) != 0) {
		st = SpellType::Charges;
	}
	if ((player._pAblSpells & GetSpellBitmask(ii)) != 0) {
		st = SpellType::Skill;
	}
	if (st == SpellType::Spell) {
		if (CheckSpell(*InspectPlayer, ii, st, true) != SpellCheckResult::Success) {
			st = SpellType::Invalid;
		}
		if (player.GetSpellLevel(ii) == 0) {
			st = SpellType::Invalid;
		}
	}
	if (townok && leveltype == DTYPE_TOWN && st != SpellType::Invalid && !GetSpellData(ii).isAllowedInTown()) {
		st = SpellType::Invalid;
	}

	return st;
}

} // namespace

void InitSpellBook()
{
	pSpellBkCel = LoadCel("data\\spellbk", static_cast<uint16_t>(SidePanelSize.width));
	pSBkBtnCel = LoadCel("data\\spellbkb", gbIsHellfire ? 61 : 76);
	LoadSmallSpellIcons();
}

void FreeSpellBook()
{
	FreeSmallSpellIcons();
	pSBkBtnCel = std::nullopt;
	pSpellBkCel = std::nullopt;
}

void DrawSpellBook(const Surface &out)
{
	ClxDraw(out, GetPanelPosition(UiPanels::Spell, { 0, 351 }), (*pSpellBkCel)[0]);
	DrawSpellBookTabs(out);
	Player &player = *InspectPlayer;
	uint64_t spl = player._pMemSpells | player._pISpells | player._pAblSpells;

	const int lineHeight = 18;

	int yp = 12;
	const int textPaddingTop = 7;
	for (size_t pageEntry = 0; pageEntry < SpellBookPageEntries; pageEntry++) {
		SpellID sn = GetSpellFromSpellPage(sbooktab, pageEntry);
		if (IsValidSpell(sn) && (spl & GetSpellBitmask(sn)) != 0) {
			SpellType st = GetSBookTrans(sn, true);
			SetSpellTrans(st);
			const Point spellCellPosition = GetPanelPosition(UiPanels::Spell, { 11, yp + SpellBookDescription.height });
			DrawSmallSpellIcon(out, spellCellPosition, sn);
			if (sn == player._pRSpell && st == player._pRSplType && !IsInspectingPlayer()) {
				SetSpellTrans(SpellType::Skill);
				DrawSmallSpellIconBorder(out, spellCellPosition);
			}

			const Point line0 { 0, yp + textPaddingTop };
			const Point line1 { 0, yp + textPaddingTop + lineHeight };
			PrintSBookStr(out, line0, pgettext("spell", GetSpellData(sn).sNameText));
			switch (GetSBookTrans(sn, false)) {
			case SpellType::Skill:
				if (sn == SpellID::RaiseUndead) {
					PrintSBookStr(out, line1, fmt::format(fmt::runtime(_("Minions: {:d}/{:d}")), GetRaisedUndeadCount(player), GetRaisedUndeadLimit(player)));
				} else {
					PrintSBookStr(out, line1, _("Skill"));
				}
				break;
			case SpellType::Charges: {
				int charges = player.InvBody[INVLOC_HAND_LEFT]._iCharges;
				PrintSBookStr(out, line1, fmt::format(fmt::runtime(ngettext("Staff ({:d} charge)", "Staff ({:d} charges)", charges)), charges));
			} break;
			default: {
				int mana = GetManaAmount(player, sn) >> 6;
				int lvl = player.GetSpellLevel(sn);
				PrintSBookStr(out, line0, fmt::format(fmt::runtime(pgettext(/* TRANSLATORS: UI constraints, keep short please.*/ "spellbook", "Level {:d}")), lvl), UiFlags::AlignRight);
				if (lvl == 0) {
					PrintSBookStr(out, line1, _("Unusable"), UiFlags::AlignRight);
				} else {
					if (sn != SpellID::BoneSpirit) {
						int min;
						int max;
						GetDamageAmt(sn, &min, &max);
						if (min != -1) {
							if (sn == SpellID::Healing || sn == SpellID::HealOther) {
								PrintSBookStr(out, line1, fmt::format(fmt::runtime(_(/* TRANSLATORS: UI constraints, keep short please.*/ "Heals: {:d} - {:d}")), min, max), UiFlags::AlignRight);
							} else {
								PrintSBookStr(out, line1, fmt::format(fmt::runtime(_(/* TRANSLATORS: UI constraints, keep short please.*/ "Damage: {:d} - {:d}")), min, max), UiFlags::AlignRight);
							}
						}
					} else {
						PrintSBookStr(out, line1, _(/* TRANSLATORS: UI constraints, keep short please.*/ "Dmg: 1/3 target hp"), UiFlags::AlignRight);
					}
					PrintSBookStr(out, line1, fmt::format(fmt::runtime(pgettext(/* TRANSLATORS: UI constraints, keep short please.*/ "spellbook", "Mana: {:d}")), mana));
				}
			} break;
			}
		}
		yp += SpellBookDescription.height;
	}
}

void CheckSBook()
{
	// Icons are drawn in a column near the left side of the panel and aligned with the spell book description entries
	// Spell icons/buttons are 37x38 pixels, laid out from 11,18 with a 5 pixel margin between each icon. This is close
	// enough to the height of the space given to spell descriptions that we can reuse that value and subtract the
	// padding from the end of the area.
	Rectangle iconArea = { GetPanelPosition(UiPanels::Spell, { 11, 18 }), Size { 37, SpellBookDescription.height * 7 - 5 } };
	if (iconArea.contains(MousePosition) && !IsInspectingPlayer()) {
		SpellID sn = GetSpellFromSpellPage(sbooktab, (MousePosition.y - iconArea.position.y) / SpellBookDescription.height);
		Player &player = *InspectPlayer;
		uint64_t spl = player._pMemSpells | player._pISpells | player._pAblSpells;
		if (IsValidSpell(sn) && (spl & GetSpellBitmask(sn)) != 0) {
			SpellType st = SpellType::Spell;
			if ((player._pISpells & GetSpellBitmask(sn)) != 0) {
				st = SpellType::Charges;
			}
			if ((player._pAblSpells & GetSpellBitmask(sn)) != 0) {
				st = SpellType::Skill;
			}
			player._pRSpell = sn;
			player._pRSplType = st;
			RedrawEverything();
		}
		return;
	}

	// Tabs are drawn in a row near the bottom of the panel. Integer division distributes
	// the spare pixel across the six buttons in the same way as DrawSpellBookTabs().
	Rectangle tabArea = { GetPanelPosition(UiPanels::Spell, SpellBookTabsPosition), Size { SpellBookTabsWidth, SpellBookTabsHeight } };
	if (tabArea.contains(MousePosition)) {
		const int hitColumn = MousePosition.x - tabArea.position.x;
		sbooktab = hitColumn * SpellBookPages / SpellBookTabsWidth;
	}
}

} // namespace devilution
