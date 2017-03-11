#include "merc.h"
#include "spells_water.h"
#include "spells_air.h"

extern void weather_update (void);

void check_mistsofarcing(CHAR_DATA * ch, int level)
{
    // Mistral ward protects from the mists
    if (is_affected(ch, gsn_mistralward))
        return;

    // Make sure the effect is not already present on the character
    for (AFFECT_DATA * paf(get_affect(ch, gsn_mistsofarcing)); paf != NULL; paf = get_affect(ch, gsn_mistsofarcing, paf))
    {
        if (paf->location == APPLY_RESIST_LIGHTNING)
            return;
    }

    // Add the effect
    send_to_char("The fine, sparkling mist covers and clings to you.\n", ch);

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.level    = level;
    af.type     = gsn_mistsofarcing;
    af.duration = number_range(4, 8);
    af.location = APPLY_RESIST_LIGHTNING;
    af.modifier = -10 - (level / 5);
    affect_to_char(ch, &af);
}

bool spell_mistsofarcing(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    for (AFFECT_DATA * paf(get_affect(ch, sn)); paf != NULL; paf = get_affect(ch, sn, paf))
    {
        if (paf->location != APPLY_RESIST_LIGHTNING)
        {
            send_to_char("You are not yet ready to call forth more mists.\n", ch);
            return false;
        }
    }

    // Check for room
    if (ch->in_room == NULL || ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot summon mists here.\n", ch);
        return false;
    }

    // Check for effect already in room
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("There are already conductive mists filling this place.\n", ch);
        return false;
    }

    // Echoes
    act("You breath slowly into your closed hands, then open them.", ch, NULL, NULL, TO_CHAR);
    act("$n breathes into $s closed hands for a long moment, then opens them.", ch, NULL, NULL, TO_ROOM);
    act("A fine, sparkling mist rolls out, hanging in the air.", ch, NULL, NULL, TO_ALL);

    // Add the effect to the room
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.level    = level;
    af.type     = sn;
    af.duration = (level / 2) + number_range(1, 3);
    affect_to_room(ch->in_room, &af);

    // Add a cooldown to the caster
    af.where    = TO_AFFECTS;
    af.duration = 18 - UMAX(0, (get_skill(ch, sn) - 70) / 5);
    affect_to_char(ch, &af);

    // Check for targets to add the effect to
    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim->next_in_room)
        check_mistsofarcing(victim, level);

    return true;
}

bool spell_breathoflife(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are already taking in the breath of life.\n", ch);
        return false;
    }

    // Check for wind
    int windStrength(checkWind(ch).second);
    if (windStrength <= 0)
    {
        send_to_char("There is no wind in this place to infuse with the breath of life.\n", ch);
        return false;
    }

    send_to_char("You exhale softly, and a faint, warm wind stirs.\n", ch);
    act("$n exhales softly, and a faint, warm wind stirs.", ch, NULL, NULL, TO_ROOM);

    // Prepare the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 20;
    
    // Apply the effect
    int baseAmount(windStrength);                                       // Range: 1 - 100
    if (baseAmount > 40) baseAmount = 40 + ((baseAmount - 50) / 5);     // Range: 1 - 52
    baseAmount += 50;                                                   // Range: 51 - 102
    baseAmount *= level;                                                // Range at hero: 2601 - 5202
    baseAmount /= 10;                                                   // Range at hero: 260 - 520

    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim->next_in_room)
    {
        if (!is_same_group(victim, ch) || is_affected(victim, sn))
            continue;

        // Found a groupmate, give the effect with random amount
        af.modifier = number_range(baseAmount / 2, (baseAmount * 3) / 2);
        affect_to_char(victim, &af);
        send_to_char("You are filled with a sense of vitality as the breath of life flows over you!\n", victim);
    }

    return true;
}

bool spell_mantleofrain(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for already present
    if (is_affected(ch, sn))
    {
        send_to_char("You are already surrounded by a mantle of rain.\n", ch);
        return false;
    }

    act("You hold your hands together over your head, then slowly lower them out to the sides.", ch, NULL, NULL, TO_CHAR);
    act("A trail of rainwater forms in their wake, flowing into a protective mantle about you!", ch, NULL, NULL, TO_CHAR);
    act("$n holds $s hands together over $s head, then slowly lowers them out to the sides.", ch, NULL, NULL, TO_ROOM);
    act("A trail of rainwater forms in their wake, flowing into a protective mantle about $m!", ch, NULL, NULL, TO_ROOM);

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 4;
    affect_to_char(ch, &af);
    return true;
}

/// @return: true if the elemental has been destroyed/replaced
static void set_fogelementalpower(CHAR_DATA * elemental, int level)
{
    level = UMAX(1, level);
    elemental->damroll = level / 2;
    elemental->hitroll = level;
    elemental->damage[0] = 2;
    elemental->damage[1] = (level / 2);
    elemental->damage[2] = 3;
    elemental->max_hit = (level * 13) + elemental->mobvalue[1];
}

bool update_fogelemental(CHAR_DATA * ch)
{
    // Check for room
    if (ch->in_room == NULL)
        return false;

    // Check for underwater
    if (ch->in_room->sector_type == SECT_UNDERWATER)
    {
        // The elemental is underwater, so replace it with a water elemental
        act("The magic binding the fog elemental breaks down under the weight of all the water, which rushes in!", ch, NULL, NULL, TO_ROOM);
        act("As the water droplets fuse together, the fog solidifies into a water elemental.", ch, NULL, NULL, TO_ROOM);

        CHAR_DATA * elemental = make_water_elemental(ch->level);
        char_to_room(elemental, ch->in_room);
        if (ch->master != NULL)
        {
            ch->master->pet = elemental;
            elemental->master = ch->master;
            elemental->leader = ch->master;
        }
        extract_char(ch, true);
        return true;
    }

    // Determine the ambient moisture
    int ambient(-10); // Ambient = -10
    if (area_is_affected(ch->in_room->area, gsn_brume)) ambient += 50; // Ambient = -10 to 40 [Typical: -10]
    if (room_is_affected(ch->in_room, gsn_blaze) || room_is_affected(ch->in_room, gsn_blazinginferno)) ambient -= 50; // Ambient = -60 to 40 [Typical: -10]
    if (area_is_affected(ch->in_room->area, gsn_rainoffire)) ambient -= 50; // Ambient = -110 to 40 [Typical: -10]
    ambient += (ch->in_room->area->w_cur.cloud_cover / 4); // Cloud cover runs from 0 - 100
    // Ambient = -110 to 65 [Typical: 2.5]

    // Hot days decrease the effect
    if (ch->in_room->area->w_cur.temperature >= 10)
        ambient -= ch->in_room->area->w_cur.temperature;

    // Ambient = Unbounded negative to 65 [Typical: 0ish]
    // Calculate storm bonus; doubled if the storm type is rain (other options are snow and hail)
    int stormBonus(ch->in_room->area->w_cur.storm_str / 2); // Storm strength runs from 0 - 100
    if (ch->in_room->area->w_cur.precip_type != 0 && ch->in_room->area->w_cur.precip_type != 1)
        stormBonus *= 2;

    ambient += stormBonus;  // Ambient = Unbounded negative to 165
    ambient /= 10;          // Ambient = Unbounded negiatve to 16, likely around -13 to 16

    // Water rooms prevent negative ambient moisture
    if (is_water_room(*ch->in_room)) 
        ambient = UMAX(ambient, 0);
    
    // Ambient moisture has been determined
    // Range is -13 to 16 (temperature is unbounded, so the negative number is only a reasonable bound)
    // Natural weather range is -3 - 11 (meaning with no magic involved -- same temperature caveat)
    // Typical land room with no storm is 0
    
    // Adjust power, tracked with mobvalue 0
    // Adjust by moving 25% of the distance, plus
    int prevValue(ch->mobvalue[0]);
    int adjust((ambient - prevValue) / 4);
    if (ambient > prevValue) ++adjust;
    else if (ambient < prevValue) --adjust;

    ch->mobvalue[0] += adjust;
    ch->mobvalue[0] = URANGE(-16, ch->mobvalue[0], 16);

    // Check for complete dissipation
    if (ch->mobvalue[0] <= -8 && number_percent() <= -2 * ch->mobvalue[0])
    {
        act("$n dries out completely, vanishing in a puff of smoky mist!", ch, NULL, NULL, TO_ROOM);
        extract_char(ch, true);
        return true;
    }

    // Check for threshold crossing for echoes
    if (adjust < 0)
    {
        // Elemental got drier
        if (ch->mobvalue[0] < -5 && prevValue >= -5) act("$n grows very faint as most of its moisture is lost to the dry air.", ch, NULL, NULL, TO_ROOM);
        else if (ch->mobvalue[0] < 0 && prevValue >= 0) act("$n seems to weaken as bits of it evaporate away.", ch, NULL, NULL, TO_ROOM);
        else if (ch->mobvalue[0] < 5 && prevValue >= 5) act("$n loses some of its strength to the drier air.", ch, NULL, NULL, TO_ROOM);
    }
    else if (adjust > 0)
    {
        // Elemental got wetter
        if (ch->mobvalue[0] > -5 && prevValue <= -5) act("$n grows more solid as some of its moisture is restored.", ch, NULL, NULL, TO_ROOM);
        else if (ch->mobvalue[0] > 0 && prevValue <= 0) act("$n seems to strengthen as more moisture condenses around it.", ch, NULL, NULL, TO_ROOM);
        else if (ch->mobvalue[0] > 5 && prevValue <= 5) act("$n grows noticeably larger, drinking in power from the moist air.", ch, NULL, NULL, TO_ROOM);
    }

    // Adjust the power
    int level(ch->level + (ch->mobvalue[0] * 2));
    set_fogelementalpower(ch, level);
    return false;
}

bool spell_fogelementalsummon(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Fog elemental summon called from NULL room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not prepared to summon another fog elemental yet.\n", ch);
        return false;
    }

    // Check for existing follower
    if (ch->pet != NULL)
    {
        act("You must dismiss $N before binding additional loyalties.", ch, NULL, ch->pet, TO_CHAR);
        return false;
    }

    // Underwater results in a water elemental rather than fog
    CHAR_DATA * elemental;
    if (ch->in_room->sector_type == SECT_UNDERWATER)
    {
        act("As your magic forms it is engulfed by the surrounding water, binding the spell into the form of a water elemental!", ch, NULL, NULL, TO_CHAR);
        act("The water about you swirls, slowly forming into the shape of a translucent water elemental!", ch, NULL, NULL, TO_ROOM);
        elemental = make_water_elemental(level);
    }
    else
    {
        act("Droplets of water condense on the air, slowly forming into a vaguely humanoid shape.", ch, NULL, NULL, TO_ALL);

        elemental = create_mobile(get_mob_index(MOB_VNUM_FOGELEMENTAL));
        elemental->level = level;
        elemental->mobvalue[1] = number_range(0, 100);
        set_fogelementalpower(elemental, level);
        elemental->hit = elemental->max_hit;
    }

    char_to_room(elemental, ch->in_room);
    ch->pet = elemental;
    elemental->master = ch;
    elemental->leader = ch;

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 50;
    affect_to_char(ch, &af);

    return true;
}

static void apply_monsoon(AREA_DATA * area, AFFECT_DATA & af)
{
    // Check for existing monsoons before applying
    if (!area_is_affected(area, gsn_monsoon))
        affect_to_area(area, &af);
}

bool spell_monsoon(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Monsoon called from NULL room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet prepared to call forth another monsoon.\n", ch);
        return false;
    }

    // Check for outside
    if (!IS_OUTSIDE(ch))
    {
        send_to_char("You must be outside to call to the skies!\n", ch);
        return false;
    }

    act("You throw back your head and howl to the skies, which darken with thick clouds in response.", ch, NULL, NULL, TO_CHAR);
    act("$n throws back $s head and howls to the skies, which darken with thick clouds in response.", ch, NULL, NULL, TO_ROOM);

    // Prepare the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AREA;
    af.type     = sn;
    af.level    = level;
    af.duration = 8;
    apply_monsoon(ch->in_room->area, af);

    // Walk the alinks, looking for connections to this area
    for (ALINK_DATA * alink(alink_first); alink != NULL; alink = alink->next)
    {
        // Apply the monsoon to all connected areas
        if (alink->a1 == ch->in_room->area) apply_monsoon(alink->a2, af);
        else if (alink->a2 == ch->in_room->area) apply_monsoon(alink->a1, af);
    }

    // Apply cooldown
    af.where    = TO_AFFECTS;
    af.duration = 120;
    affect_to_char(ch, &af);

    // Force a weather update
    weather_update();
    return true;
}

bool spell_brume(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("brume called from NULL room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You cannot summon any more fog yet.\n", ch);
        return false;
    }

    // Check whether already present
    if (area_is_affected(ch->in_room->area, sn))
    {
        send_to_char("This area is already filled with a thick fog.\n", ch);
        return false;
    }

    // Sector check
    if (ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot call fog underwater.\n", ch);
        return false;
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AREA;
    af.type     = sn;
    af.level    = level;
    af.duration = 10;
    affect_to_area(ch->in_room->area, &af);

    // Apply a cooldown
    af.where    = TO_AFFECTS;
    af.duration = 60;
    affect_to_char(ch, &af);

    // Echo to PCs in the area
    for (DESCRIPTOR_DATA * d(descriptor_list); d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING && d->character->in_room != NULL && d->character->in_room->area == ch->in_room->area)
            send_to_char("A thick, heavy fog suddenly descends about you, obscuring the area.\n", d->character);
    }

    return true;
}
