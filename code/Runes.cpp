#include "Runes.h"
#include "StringUtil.h"
#include "DisplayPanel.h"
#include <sstream>
#include <cmath>
#include <iomanip>

std::string Runes::ListRunes(CHAR_DATA & ch)
{
    // Check for PC
    if (IS_NPC(&ch))
        return "";

    // Determine the known runes
    std::vector<std::pair<DisplayPanel::Box, DisplayPanel::Box> > knownRunes;
    for (unsigned int i(0); i < Rune::Max; ++i)
    {
        // Lookup the rune
        const Rune * rune(Lookup(i));
        if (rune == NULL || !KnowsRune(ch, *rune) || rune->syllableCount < 1)
            continue;

        // Ensure sufficient space for this rune
        while (knownRunes.size() < rune->syllableCount)
        {
            // Add in the header and box
            std::ostringstream mess;
            mess << (knownRunes.size() + 1) << "-syllable";
            knownRunes.push_back(std::make_pair(DisplayPanel::Box(mess.str()), DisplayPanel::Box()));
        }

        // Add this rune
        knownRunes[rune->syllableCount - 1].second.AddLine("{W" + rune->name + "{x");
    }

    // If no runes are known, just bail out
    if (knownRunes.empty())
        return "";

    // Build the known runes board and panel
    DisplayPanel::HorizontalSplit knownRunesBoard;
    for (size_t i(0); i < knownRunes.size(); ++i)
        knownRunesBoard.Add(DisplayPanel::VerticalSplit(knownRunes[i].first, knownRunes[i].second));

    DisplayPanel::VerticalSplit knownRunesPanel(DisplayPanel::Options(DisplayPanel::Style_Collapse));
    knownRunesPanel.Add(DisplayPanel::Box("Known Runes"), knownRunesBoard);

    // Build the invoked runes
    DisplayPanel::Box runeNumberBox("#", "-");
    DisplayPanel::Box runeNameBox("Rune", "----");
    DisplayPanel::Box runeObjectBox("Carved Object", "-------------");

    // Iterate the list of invoked runes to fill out the runes panel
    std::vector<RuneInfo> invokedRunes(Runes::LookupInvokedRunes(ch));
    for (size_t i(0); i < invokedRunes.size(); ++i)
    {
        // Lookup the rune
        const Rune * rune(Lookup(invokedRunes[i].objAff->modifier));
        if (rune == NULL)
            continue;

        // List this rune
        std::ostringstream mess;
        mess << (i + 1);
        runeNumberBox.AddLine(mess.str());
        runeNameBox.AddLine("{W" + rune->name + "{x");
        runeObjectBox.AddLine("{W" + std::string(invokedRunes[i].obj->short_descr) + "{x");
    }

    DisplayPanel::HorizontalSplit invokedRunesBoard(DisplayPanel::Options(DisplayPanel::Style_None));
    invokedRunesBoard.Add(runeNumberBox, runeNameBox, runeObjectBox);

    DisplayPanel::VerticalSplit invokedRunesPanel(DisplayPanel::Options(DisplayPanel::Style_Collapse));
    if (invokedRunes.empty())
        invokedRunesPanel.Add(DisplayPanel::Box("Invoked Runes: {Wnone{x"));
    else
    {
        invokedRunesPanel.Add(DisplayPanel::Box("Invoked Runes"));
        invokedRunesPanel.Add(invokedRunesBoard);
    }

    // Build the invokable runes panel
    DisplayPanel::Box dormantRuneNumberBox("#", "-");
    DisplayPanel::Box dormantRuneCostBox("Mana", "----");
    DisplayPanel::Box dormantRuneNameBox("Rune", "----");
    DisplayPanel::Box dormantRuneObjectBox("Carved Object", "-------------");

    // Iterate the list of invoked runes to fill out the runes panel
    int attunementCount(AttunementCount(ch));
    bool makeMonoFree(UpdateRuneManaCosts(ch, attunementCount));
    std::vector<RuneInfo> invokableRunes(Runes::LookupInvokableRunes(ch));
   
    for (size_t i(0); i < invokableRunes.size(); ++i)
    {
        // Lookup the rune
        const Rune * rune(Lookup(invokableRunes[i].objAff->modifier));
        if (rune == NULL)
            continue;

        // List this rune
        std::ostringstream mess;
        mess << (i + 1);
        dormantRuneNumberBox.AddLine(mess.str());

        mess.str("");
        bool makeMonoFreeLocal(makeMonoFree);
        mess << "{W" << ManaCost(*rune, attunementCount, makeMonoFreeLocal) << "{x";
        dormantRuneCostBox.AddLine(mess.str());

        dormantRuneNameBox.AddLine("{W" + rune->name + "{x");
        dormantRuneObjectBox.AddLine("{W" + std::string(invokableRunes[i].obj->short_descr) + "{x");
    }

    DisplayPanel::HorizontalSplit dormantRunesBoard(DisplayPanel::Options(DisplayPanel::Style_None));
    dormantRunesBoard.Add(dormantRuneNumberBox, dormantRuneCostBox);
    dormantRunesBoard.Add(dormantRuneNameBox, dormantRuneObjectBox);

    DisplayPanel::VerticalSplit dormantRunesPanel(DisplayPanel::Options(DisplayPanel::Style_Collapse));
    if (invokableRunes.empty())
        dormantRunesPanel.Add(DisplayPanel::Box("Invokable Runes: {Wnone{x"));
    else
    {
        dormantRunesPanel.Add(DisplayPanel::Box("Invokable Runes"));
        dormantRunesPanel.Add(dormantRunesBoard);
    }

    // Build the final panel
    DisplayPanel::VerticalSplit mainPanel;
    mainPanel.Add(knownRunesPanel, invokedRunesPanel, dormantRunesPanel);
    return DisplayPanel::Render(mainPanel);
}

void Runes::ShowObjectRunes(CHAR_DATA & ch, OBJ_DATA & obj, char * buffer)
{
    // Check for the stonecraft skill
    if (get_skill(const_cast<CHAR_DATA*>(&ch), gsn_stonecraft) <= 0)
        return;

    // Look for a rune effect on the obj
    const AFFECT_DATA * paf(get_obj_affect(&obj, gsn_stonecraft));
    if (paf == NULL)
        return;

    // Found a rune effect, lookup the rune
    const Rune * rune(Lookup(paf->modifier));
    if (rune == NULL)
        return;

    // Check for whether the rune is invoked by the caster and also worn
    bool active(InvokerOf(*paf) == &ch);
    std::ostringstream result;
    if (active) result << "{W";
    result << '[' << rune->name << ']';
    if (active) result << "{x";
    result << ' ';

    // Append the rune display to the buffer
    strcat(buffer, result.str().c_str());
}

bool Runes::IsInvoked(const OBJ_DATA & obj, Rune::Type type)
{
    const AFFECT_DATA * paf(get_obj_affect(&obj, gsn_stonecraft));
    return (paf != NULL && InvokerOf(*paf) != NULL && paf->modifier == type);
}

unsigned int Runes::InvokedCount(const CHAR_DATA & ch, Rune::Type type)
{
    // Iterate the char's carried objects
    unsigned int total(0);
    for (OBJ_DATA * obj(ch.carrying); obj != NULL; obj = obj->next_content)
    {
        // Look for an invoked stonecraft effect of matching type
        if (obj->worn_on && IsInvoked(*obj, type))
            ++total;
    }

    return total;
}

unsigned int Runes::InvokedCountHere(const CHAR_DATA & ch, Rune::Type type, unsigned int & bonusCount)
{
    // Sanity-check
    bonusCount = 0;
    if (ch.in_room == NULL)
        return 0;

    // Add up the total invoked runes amongst all people in the room
    unsigned int total(0);
    for (CHAR_DATA * runeCh(ch.in_room->people); runeCh != NULL; runeCh = runeCh->next_in_room)
    {
        unsigned int invokedCount(InvokedCount(*runeCh, type));
        if (invokedCount > 0)
        {
            total += invokedCount;
            bonusCount += BonusCount(*runeCh);
        }
    }

    return total;
}

unsigned int Runes::BonusCount(CHAR_DATA & ch)
{
    unsigned int result(0);

    // Rune craft is worth a bonus
    if (number_percent() <= get_skill(&ch, gsn_runecraft))
    {
        ++result;
        check_improve(&ch, NULL, gsn_runecraft, true, 4);
    }
    else
        check_improve(&ch, NULL, gsn_runecraft, false, 4);

    // Runes of empowerment are worth a bonus
    result += InvokedCount(ch, Rune::Empowerment);
    return result;
}

bool Runes::CarveRune(CHAR_DATA & ch, OBJ_DATA & obj, bool success, const char * argument)
{
    // Check whether there is already a rune present
    if (obj_is_affected(&obj, gsn_stonecraft))
    {
        act("There is already an earthen rune carved into $p.", &ch, &obj, NULL, TO_CHAR);
        return false;
    }

    // Determine which rune is desired
    const Rune * rune(RuneTable::Lookup(argument));
    if (rune == NULL || !KnowsRune(ch, *rune))
    {
        send_to_char("You do not know how to carve such a rune.\n", &ch);
        return false;
    }

    // Check for mana
    int manaCost(skill_table[gsn_stonecraft].min_mana * rune->syllableCount);
    if (ch.mana < manaCost)
    {
        std::ostringstream mess;
        mess << "You lack the mental focus right now to carve a rune of " << rune->name << ".\n";
        send_to_char(mess.str().c_str(), &ch);
        return false;
    }

    // Charge mana and lag
    expend_mana(&ch, manaCost);
    WAIT_STATE(&ch, UMAX(ch.wait, skill_table[gsn_stonecraft].beats));

    // Check for stoneshape skill improvement
    if (rune->syllableCount > 1)
        check_improve(&ch, NULL, gsn_stoneshape, success, 3);

    // Echoes
    std::ostringstream mess;
    mess << "Tracing your finger across $p, you concentrate earthen energy into it.\n";
    mess << "Chips of " << material_table[obj.material].name << " slowly turn to dust and fall away, crumbling at your touch.";
    act(mess.str().c_str(), &ch, &obj, NULL, TO_CHAR);

    mess.str("");
    mess << "$n traces $s finger across $p, focusing intently on it.\n";
    mess << "Chips of " << material_table[obj.material].name << " slowly turn to dust and fall away, crumbling at $s touch.";
    act(mess.str().c_str(), &ch, &obj, NULL, TO_ROOM);

    // Skill check
    if (!success)
    {
        // Failure echoes
        act("You lose focus on the delicate task, and the concentrated energy tears $p to pieces!", &ch, &obj, NULL, TO_CHAR);
        act("$p suddenly crumbles to pieces in $s hands!", &ch, &obj, NULL, TO_ROOM);
        extract_obj(&obj);
        return true;
    }

    // Completion echoes
    mess.str("");
    mess << "You complete your work in short order, leaving a rune of " << rune->name << " etched into $p!";
    act(mess.str().c_str(), &ch, &obj, NULL, TO_CHAR);

    mess.str("");
    mess << "$e completes $s work in short order, leaving an earthen rune etched into $p!";
    act(mess.str().c_str(), &ch, &obj, NULL, TO_ROOM);

    // Add the rune to the object
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = gsn_stonecraft;
    af.location = APPLY_HIDE;
    af.duration = -1;
    af.modifier = rune->type;
    affect_to_obj(&obj, &af);
    return true;
}

void Runes::InvokeRune(CHAR_DATA & ch, const char * argument)
{
    // Get the list of invokable runes and look up the specified rune within it
    std::vector<RuneInfo> runes(LookupInvokableRunes(ch));
    size_t index(LookupRune(runes, argument));
    if (index >= runes.size())
    {
        send_to_char("You can invoke no such rune.\n", &ch);
        return;
    }

    // Lookup the rune
    const Rune * rune(Lookup(runes[index].objAff->modifier));
    if (rune == NULL)
    {
        send_to_char("An error has occurred, please contact the gods.\n", &ch);
        return;
    }

    // Check for mana cost; start by updating rune costs
    int attunements(AttunementCount(ch));
    bool makeMonoFree(UpdateRuneManaCosts(ch, attunements));
    int manaCost(ManaCost(*rune, attunements, makeMonoFree));
    if (ch.max_mana < manaCost || ch.mana < manaCost)
    {
        std::ostringstream mess;
        mess << "You lack the energy to invoke a rune of " << rune->name << ".\n";
        send_to_char(mess.str().c_str(), &ch);
        return;
    }

    // Invoke the rune; add the effect to the char and set the obj's effect's pointer
    runes[index].objAff->point = &ch;
    
    AFFECT_DATA af = {0};
    af.type     = gsn_stonecraft;
    af.where    = TO_AFFECTS;
    af.location = APPLY_HIDE;
    af.duration = -1;
    af.point    = runes[index].obj;
    affect_to_char(&ch, &af);

    // Perform echoes
    std::ostringstream mess;
    mess << "You invoke the rune of " << rune->name << " on $p, empowering it with your will!";
    act(mess.str().c_str(), &ch, runes[index].obj, NULL, TO_CHAR);
    act("The earthen rune on $p glows faintly, throbbing with a low, steady power.", &ch, runes[index].obj, NULL, TO_ROOM);

    // Charge mana and lag, then update all rune mana costs
    expend_mana(&ch, manaCost);
    WAIT_STATE(&ch, PULSE_VIOLENCE);
    UpdateRuneManaCosts(ch, attunements);
}

void Runes::RevokeRune(CHAR_DATA & ch, const char * argument)
{
    // Get the list of invoked runes and look up the specified rune within it
    std::vector<RuneInfo> runes(LookupInvokedRunes(ch));
    size_t index(LookupRune(runes, argument));
    if (index >= runes.size())
    {
        send_to_char("You are invoking no such rune.\n", &ch);
        return;
    }

    // Clear the rune invocation; the object pointer will be cleaned up in affect_remove, 
    // and echoes and mana adjustments also take place there
    affect_remove(&ch, runes[index].chAff);
}

void Runes::UpdateAttunementsFor(CHAR_DATA & ch, int count)
{
    // Check for the skill to use the attunements in the first place
    if (get_skill(&ch, gsn_stonecraft) <= 0)
        return;

    // Check for existing effect
    AFFECT_DATA * paf(get_affect(&ch, gsn_markofloam));
    if (paf == NULL)
    {
        // If setting to 0 anyway, nothing to do here
        if (count == 0)
            return;

        // Add the effect
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_markofloam;
        af.level    = ch.level;
        af.duration = -1;
        af.modifier = count;
        af.location = APPLY_HIDE;
        affect_to_char(&ch, &af);
    }
    else if (paf->modifier == count) return;
    else paf->modifier = count;

    if (count == 0) send_to_char("You sense the power of your marks of dominion crumble completely!\n", &ch);
    else send_to_char("You sense the power of your marks of dominion shift.\n", &ch);
    UpdateRuneManaCosts(ch, count);
}

void Runes::UpdateManaCostsFor(CHAR_DATA & ch)
{
    UpdateRuneManaCosts(ch, AttunementCount(ch));
}

////////////////// Implementation /////////////////////////////
int Runes::AttunementCount(const CHAR_DATA & ch)
{
    AFFECT_DATA * paf(get_affect(&ch, gsn_markofloam));
    return (paf == NULL ? 0 : paf->modifier);
}

bool Runes::UpdateRuneManaCosts(CHAR_DATA & ch, int attunements)
{
    // Use this to control the otherwise recursive nature of the affect_remove call below
    static bool blockEntry(false);
    if (blockEntry)
        return false;

    // Find the existing mana effect from stonecraft
    int maxMana(ch.max_mana);
    AFFECT_DATA * paf;
    for (paf = get_affect(&ch, gsn_stonecraft); paf != NULL; paf = get_affect(&ch, gsn_stonecraft, paf))
    {
        if (paf->location == APPLY_MANA)
        {
            maxMana -= paf->modifier;
            break;
        }
    }

    // Initialize
    int totalCost(0);
    bool makeMonoFree(get_skill(&ch, gsn_runecraft) > 0);

    // Iterate the invoked runes
    std::vector<RuneInfo> runes(LookupInvokedRunes(ch));
    for (size_t i(0); i < runes.size(); ++i)
    {
        // Lookup the rune
        const Rune * rune(Lookup(runes[i].objAff->modifier));
        if (rune == NULL)
            continue;

        // Verify the mana works out
        int manaCost(ManaCost(*rune, attunements, makeMonoFree));
        if (totalCost + manaCost > maxMana)
        {
            // The cost is too high, drop all remaining runes
            blockEntry = true;
            for (size_t j(i); j < runes.size(); ++j)
                affect_remove(&ch, runes[j].chAff);
            
            blockEntry = false;
            break;
        }

        // Add in the cost
        totalCost += manaCost;
    }

    // Adjust the mana effect
    if (totalCost <= 0)
    {
        // No cost, so just make sure there is no effect
        if (paf != NULL)
            affect_remove(&ch, paf);

        return makeMonoFree;
    }
    
    // If the effect is already correct, bail out
    if (paf != NULL && paf->modifier == -totalCost)
        return makeMonoFree;

    // Effect is absent or incorrect; clear it out if incorrect
    if (paf != NULL)
        affect_remove(&ch, paf);

    // Add in the new mana cost
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_stonecraft;
    af.level    = ch.level;
    af.location = APPLY_MANA;
    af.modifier = -totalCost;
    af.duration = -1;
    affect_to_char(&ch, &af);

    return makeMonoFree;
}

size_t Runes::LookupRune(const std::vector<RuneInfo> & runes, const char * argument)
{
    // Check for a numerical argument
    if (is_number(argument))
        return static_cast<size_t>(atoi(argument) - 1);

    // Check for an object argument
    std::pair<int, std::string> values(number_argument(argument));
    for (size_t i(0); i < runes.size(); ++i)
    {
        if (is_name(values.second.c_str(), runes[i].obj->name))
        {
            --values.first;
            if (values.first == 0)
                return i;
        }
    }

    // Neither number nor name matched
    return runes.size();
}

std::vector<Runes::RuneInfo> Runes::LookupInvokedRunes(const CHAR_DATA & ch)
{
    // Check for PC
    std::vector<RuneInfo> result;
    if (IS_NPC(&ch))
        return result;

    // Iterate the char's stonecraft effects
    for (AFFECT_DATA * paf(get_affect(&ch, gsn_stonecraft)); paf != NULL; paf = get_affect(&ch, gsn_stonecraft, paf))
    {
        // Ignore cooldown effects
        if (paf->location != APPLY_HIDE)
            continue;

        // Get the object from the effect
        OBJ_DATA * obj(static_cast<OBJ_DATA*>(paf->point));
        if (obj == NULL)
        {
            bug("Null object pointed to by active rune", 0);
            continue;
        }

        // Verify that this caster is the invoker; not necessarily a bug because of how affect_remove partially 
        // tears down the effect then calls for a mana update
        AFFECT_DATA * objPaf(get_obj_affect(obj, gsn_stonecraft));
        if (objPaf == NULL || InvokerOf(*objPaf) != &ch)
            continue;

        // Add this rune to the list
        result.push_back(RuneInfo(*obj, *objPaf, paf));
    }

    return result;
}

std::vector<Runes::RuneInfo> Runes::LookupInvokableRunes(const CHAR_DATA & ch)
{
    // Check for PC
    std::vector<RuneInfo> result;
    if (IS_NPC(&ch))
        return result;

    // Determine how many syllables this char can invoke
    bool polysyllabicAllowed(get_skill(const_cast<CHAR_DATA*>(&ch), gsn_stoneshape) > 0);
    
    // Iterate the char's carried objects
    for (OBJ_DATA * obj(ch.carrying); obj != NULL; obj = obj->next_content)
    {
        // Look for an uninvoked stonecraft effect
        AFFECT_DATA * paf(get_obj_affect(obj, gsn_stonecraft));
        if (paf == NULL || InvokerOf(*paf) != NULL)
            continue;

        // Check syllable count
        const Rune * rune(Lookup(paf->modifier));
        if (rune == NULL || (!polysyllabicAllowed && rune->syllableCount > 1))
            continue;

        // Add this to the list
        result.push_back(RuneInfo(*obj, *paf, NULL));
    }

    return result;
}

const CHAR_DATA * Runes::InvokerOf(const AFFECT_DATA & af)
{
    return static_cast<CHAR_DATA*>(af.point);
}

const Rune * Runes::Lookup(unsigned int index)
{
    const Rune * rune(RuneTable::Lookup(static_cast<Rune::Type>(index)));
    if (rune == NULL)
        bug("Failed to find rune type in RuneTable lookup: %d", index);

    return rune;
}

int Runes::ManaCost(const Rune & rune, int attunements, bool & makeMonoFree)
{
    // Check for a freebie
    if (makeMonoFree && rune.syllableCount == 1)
    {
        makeMonoFree = false;
        return 0;
    }

    // Make the result according to syllable count
    int result(std::pow(2, rune.syllableCount - 1) * 100);

    // Factor in attunements
    int reduction(0);
    if (attunements > 20) {reduction += (attunements - 20); attunements = 20;}      // Attunements > 20 are worth 1
    if (attunements > 15) {reduction += (attunements - 15) * 2; attunements = 15;}  // Attunements 16 - 20 are worth 2
    if (attunements > 10) {reduction += (attunements - 10) * 4; attunements = 10;}  // Attunements 11 - 15 are worth 4
    if (attunements > 5) {reduction += (attunements - 5) * 6; attunements = 5;}     // Attunements 6 - 10 are worth 6
    reduction += (attunements * 8);                                                 // Attunements 1 - 5 are worth 8
    reduction /= 2;                                                                 // Halve all attunements
    reduction = UMIN(reduction, result / 2);                                        // Cap reduction at half total cost
    return (result - reduction);
}

// Assumes PC
bool Runes::KnowsRune(const CHAR_DATA & ch, const Rune & rune)
{
    // Check for bit set
    if (BIT_GET(ch.pcdata->bitptr, rune.learnedBit) == 0)
        return false;

    // Check for proper skill
    if (rune.syllableCount > 1 && get_skill(const_cast<CHAR_DATA*>(&ch), gsn_stoneshape) <= 0)
        return false;

    return true;
}
