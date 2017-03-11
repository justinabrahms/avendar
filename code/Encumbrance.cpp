#include "Encumbrance.h"
#include "Runes.h"
#include <sstream>

Encumbrance::ChangeNotifier::ChangeNotifier(CHAR_DATA & ch) : m_ch(&ch), m_id(ch.id), m_level(LevelFor(ch)) {}

Encumbrance::ChangeNotifier::~ChangeNotifier()
{
    // Verify the ch is still valid
    if (!IS_VALID(m_ch) || m_ch->id != m_id)
        return;

    // Get the new level and check for a change
    Level level(LevelFor(*m_ch));
    if (level == m_level)
        return;

    // Change happened, so echo about it
    std::ostringstream mess;
    mess << "You are now " << DescriptorFor(level) << " by your equipment.\n";
    send_to_char(mess.str().c_str(), m_ch);
}

const char * Encumbrance::DescriptorFor(Level level)
{
    switch (level)
    {
        case None:      return "unencumbered";
        case Light:     return "lightly encumbered";
        case Medium:    return "encumbered";
        case Heavy:     return "heavily encumbered";
        case Max:       return "overburdened";
    }

    return "";
}

Encumbrance::Level Encumbrance::LevelFor(const CHAR_DATA & ch)
{
    // Collect the values needed for this calculation
    int currStrength(get_curr_stat(&ch, STAT_STR));
    unsigned int runeCount(Runes::InvokedCount(ch, Rune::Strength));
    unsigned int maxWeight(CarryWeightCapacity(ch, currStrength, runeCount));
    unsigned int carryWeight(get_carry_weight(ch));

    // Check carry weight thresholds
    Level result(Max);
    for (unsigned int i(Light); i <= Max; ++i)
    {
        if (carryWeight <= CarryWeightThresholdFor(ch, static_cast<Level>(i), maxWeight, runeCount))
        {
            result = static_cast<Level>(i - 1);
            break;
        }
    }

    // Check specific armor
    if (result == None && WornArmorEncumbers(ch, currStrength))
        result = Light;

    // Check for nimble trait
    if (result == Light && !IS_NPC(&ch) && BIT_GET(ch.pcdata->traits, TRAIT_NIMBLE))
        result = None;

    return result;
}

unsigned int Encumbrance::CarryCountCapacity(const CHAR_DATA & ch)
{
    if (IS_IMMORTAL(&ch)) return MaxCarryCount;             // Imms can carry everything
    if (IS_NPC(&ch) && IS_SET(ch.act, ACT_PET)) return 0;   // Pets cannot carry anything
    unsigned int result(MAX_WEAR + 6);                      // Can carry enough to wear a full set, plus 6
    result += (3 * get_curr_stat(&ch, STAT_DEX));           // Each point of DEX is worth 3 more items
    return UMIN(result, MaxCarryCount);
}

unsigned int Encumbrance::CarryWeightCapacity(const CHAR_DATA & ch)
{
    return CarryWeightCapacity(ch, get_curr_stat(&ch, STAT_STR), Runes::InvokedCount(ch, Rune::Strength)); 
}

unsigned int Encumbrance::CarryWeightCapacity(const CHAR_DATA & ch, int currStrength, unsigned int runeCount)
{
    if (IS_IMMORTAL(&ch)) return MaxCarryWeight;            // Imms can carry everything
    if (IS_NPC(&ch) && IS_SET(ch.act, ACT_PET)) return 0;   // Pets cannot carry anything

    unsigned int result(1500);                              // Base carry weight is 150.0
    result += ((currStrength * currStrength * 15) / 2);     // Add in strength bonus

    if (!IS_NPC(&ch) && BIT_GET(ch.pcdata->traits, TRAIT_PACK_HORSE))
        result = (result * 23) / 20;                        // +15% for pack horse

    result += (runeCount * 50);                             // +5.0 for each strength rune
    return UMIN(MaxCarryWeight, result);
}

unsigned int Encumbrance::CarryWeightThresholdFor(const CHAR_DATA & ch, Level level, unsigned int maxWeight, unsigned int runeCount)
{
    unsigned int thresholdMultiplier;
    switch (level)
    {
        case None:      return 0;
        case Light:     thresholdMultiplier = 70; break;
        case Medium:    thresholdMultiplier = 80; break;
        case Heavy:     thresholdMultiplier = 90; break;
        case Max:       return maxWeight;
        default:        bug("Unknown level requested in Encumbrance::CarryWeightThresholdFor [%d]", level); return 0;
    }

    // Adjust for runes
    thresholdMultiplier += (runeCount * 10);
    unsigned int result((maxWeight * thresholdMultiplier) / 100);
    return UMIN(result, maxWeight);
}

bool Encumbrance::WornArmorEncumbers(const CHAR_DATA & ch, int currStrength)
{
    // Check specific pieces of armor for interfering with spellcasting
    unsigned int maxWeight(SingleItemMax + (currStrength * 2));
    if (WornWeight(ch, WEAR_BODY) > maxWeight) return true;             // Body armor is checked directly
    if (((WornWeight(ch, WEAR_ARMS) * 3) / 2) > maxWeight) return true; // Arms armor is checked at x1.5 rate
    if ((WornWeight(ch, WEAR_HANDS) * 2) > maxWeight) return true;      // Hands armor is checked at x2 rate
    return false;
}

unsigned int Encumbrance::WornWeight(const CHAR_DATA & ch, int wearSlot)
{
    OBJ_DATA * obj(get_eq_char(&ch, wearSlot));
    if (obj == NULL) return 0;
    return get_obj_weight(obj);
}
