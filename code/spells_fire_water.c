#include "spells_fire_water.h"
#include "spells_water.h"
#include <sstream>

void checkSteam(CHAR_DATA * ch)
{
    // Check for room disqualifiers
    if (ch->in_room == NULL || is_water_room(*ch->in_room))
        return;

    // Check odds; 50-75%, depending on skill
    if (number_percent() > 50 + UMAX(0, (get_skill(ch, gsn_steam) - 75)))
        return;

    // Add steam effect
    send_to_char("Hissing steam roils out from you, hanging densely in the air!\n", ch);
    act("Hissing steam roils out from $n, hanging densely in the air!", ch, NULL, NULL, TO_ROOM);

    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.type     = gsn_steam;
    af.level    = ch->level;
    af.duration = 4;
    affect_to_room(ch->in_room, &af);
}

bool spell_steam(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to call forth steam again.\n", ch);
        return false;
    }

    // Check initial steam
    send_to_char("Steam begins to issue forth from you, slowly filling the air.\n", ch);   
    checkSteam(ch);

    // Add the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 1;
    affect_to_char(ch, &af);
    return true;
}

bool spell_boilseas(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Boil seas cast from NULL room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You cannot draw upon such power again so soon.\n", ch);
        return false;
    }
    
    // Get the rooms
    std::set<ROOM_INDEX_DATA*> rooms(connected_water_rooms(*ch->in_room));
    if (rooms.empty())
    {
        send_to_char("There is no body of water here to boil.\n", ch);
        return false;
    }

    // Apply the boil seas effect
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.type     = sn;
    af.level    = level;
    af.modifier = ch->id;

    // Duration of effect varies by how many rooms being boiled
    if (rooms.size() <= 3) af.duration = 5;
    else if (rooms.size() <= 9) af.duration = 4;
    else if (rooms.size() <= 15) af.duration = 3;
    else af.duration = 2;

    for (std::set<ROOM_INDEX_DATA*>::const_iterator iter(rooms.begin()); iter != rooms.end(); ++iter)
    {
        affect_to_room(*iter, &af);
        if ((*iter)->people != NULL)
            act("The temperature of the surrounding water begins to rise, rapidly reaching boiling point!", (*iter)->people, NULL, NULL, TO_ALL);
    }

    // Apply a cooldown
    af.where    = TO_AFFECTS;
    af.duration = 60;
    affect_to_char(ch, &af);

    return true;
}

bool spell_sauna(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Sanity check
    if (ch->in_room == NULL)
        return false;

    // Cannot double up saunas
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("This place is already filled with healing mists.\n", ch);
        return false;
    }

    // Cooldown check
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to summon the healing mists again.\n", ch);
        return false;
    }

    // Sector check
    if (ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot create a sauna underwater.\n", ch);
        return false;
    }

    int duration(level / 10);
    int cooldown(15);
    act("You chant, and the air slowly fills with a soft, warm mist.", ch, NULL, NULL, TO_CHAR);
    act("$n chants, and the air slowly fills with a soft, warm mist.", ch, NULL, NULL, TO_ROOM);

    // Duration/cooldown bonuses for watery areas
    if (ch->in_room->sector_type == SECT_WATER_NOSWIM || ch->in_room->sector_type == SECT_WATER_SWIM)
    {
        send_to_char("You hardly feel exerted by the minimal effort required to summon the healing fog in this place.\n", ch);
        cooldown = 10;
    }

    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.type     = sn;
    af.level    = level;
    af.duration = duration;
    affect_to_room(ch->in_room, &af);

    // Apply cooldown
    af.where    = TO_AFFECTS;
    af.duration = cooldown;
    affect_to_char(ch, &af);
    return true;
}

bool spell_flamesofthemartyr(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    if (ch == victim)
    {
        send_to_char("You can't martyr yourself for yourself!\n", ch);
        return false;
    }

    act("$n burns away $s own life force, channeling its healing warmth towards $N!", ch, NULL, victim, TO_NOTVICT);
    act("You burn away your own life force, channeling its healing warmth towards $N!", ch, NULL, victim, TO_CHAR);
    act("$n burns away $s own life force, channeling its healing warmth towards you!", ch, NULL, victim, TO_VICT);
    
    // Calculate damage and healing independently
    int dam(dice(5, level));
    int healing(dice(level, 4) + 100);
    victim->hit = UMIN(victim->max_hit, victim->hit + healing);
    ch->hit -= dam;

    // Check for death
    update_pos(ch);
    if (ch->position == POS_DEAD)
    {
        act("$n collapses as blue flames lick at $s flesh! The fire spreads quickly, consuming $m.", ch, NULL, NULL, TO_ROOM);
        act("You collapse as blue flames lick at your flesh! The fire spreads quickly, consuming you.", ch, NULL, NULL, TO_CHAR);
        std::ostringstream mess;
        mess << ch->name << " died a martyr to save " << victim->name << " in " << ch->in_room->name << " [" << ch->in_room->vnum << "]";
        wiznet(const_cast<char*>(mess.str().c_str()), NULL, NULL, WIZ_DEATHS, 0, 0);
        log_string(mess.str().c_str());
        raw_kill(ch);
    }
    return true;
}

bool spell_martyrsfire(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are already prepared to martyr yourself.\n", ch);
        return false;
    }

    // Prepare the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.modifier = get_skill(ch, sn);
    af.duration = 6;
    affect_to_char(ch, &af);

    act("A faint red glow seems to suffuse $n for a moment, then fades.", ch, NULL, NULL, TO_ROOM);
    act("You feel a faint glow suffuse you for a moment as you prepare to martyr yourself, if necessary.", ch, NULL, NULL, TO_CHAR);
    return true;
}

bool spell_boilblood(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    // Check for blood already boiling
    if (is_affected(victim, gsn_boilblood))
    {
        act("$N's blood is already boiling.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    act("$n raises a palm towards $N and hisses an arcane word!", ch, NULL, victim, TO_NOTVICT);
    act("$n raises a palm towards you and hisses an arcane word!", ch, NULL, victim, TO_VICT);
    act("You raise a palm towards $N and hiss an arcane word!", ch, NULL, victim, TO_CHAR);

    // Check cold save; caster must pass this to target the spell
    if (saves_spell(level, ch, victim, DAM_COLD))
    {
        act("$N resists your attempt to boil $S blood.", ch, NULL, victim, TO_CHAR);
        act("You resist $n's attempt to boil your blood.", ch, NULL, victim, TO_VICT);
        return true;
    }

    // Affect is going to happen, so build it
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = level / 8;

    // Check fire save; failure here means the victim gets the full effect
    if (saves_spell(level, ch, victim, DAM_FIRE))
    {
        act("$N yelps in sudden pain as $S blood begins to boil.", ch, NULL, victim, TO_CHAR);
        act("You yelp in pain as your blood begins to boil!", ch, NULL, victim, TO_VICT);
        act("$N yelps in sudden pain!", ch, NULL, victim, TO_NOTVICT);
    }
    else
    {
        act("$N doubles over in intense pain as $S blood begins to boil hot!", ch, NULL, victim, TO_CHAR);
        act("You double over in intense pain as your blood begins to boil hot!", ch, NULL, victim, TO_VICT);
        act("$N doubles over in intense pain!", ch, NULL, victim, TO_NOTVICT);
        af.modifier = 1;
        WAIT_STATE(victim, UMAX(victim->wait, PULSE_VIOLENCE));
    }

    affect_to_char(victim, &af);
    return true;
}
