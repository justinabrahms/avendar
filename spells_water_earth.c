#include "spells_water_earth.h"
#include "spells_spirit.h"
#include "Direction.h"

extern bool check_dodge args((CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj)); // from fight.c

void handle_waterwheel_destruction(OBJ_DATA & obj)
{
    // Verify a room
    ROOM_INDEX_DATA * room(get_room_for_obj(obj));
    if (room == NULL)
        return;

    // Load up the remains and transfer the contents
    OBJ_DATA * remains(create_object(get_obj_index(OBJ_VNUM_WATERWHEEL_REMAINS), 0));
    remains->level = obj.level;

    for (OBJ_DATA * item(obj.contains); item != NULL; item = obj.contains)
    {
        obj_from_obj(item);
        obj_to_obj(item, remains);
    }

    obj_to_room(remains, room);
}

void handle_waterwheel_crystal_destroyed(CHAR_DATA & ch, OBJ_DATA & obj)
{
    // Look for the effect
    AFFECT_DATA * paf(get_obj_affect(&obj, gsn_constructwaterwheel));
    if (paf == NULL)
        return;

    // Echoes
    act("As you destroy $p, a surge of energy rushes from it and into you!", &ch, &obj, NULL, TO_CHAR);
    act("As $n destroys $p, a flash of blue light shines out from it, focused on $m!", &ch, &obj, NULL, TO_ROOM);

    // Strip the effect from the char if present
    for (AFFECT_DATA * waf(get_affect(&ch, gsn_constructwaterwheel)); waf != NULL; waf = get_affect(&ch, gsn_constructwaterwheel, waf))
    {
        if (waf->location == APPLY_MANA)
        {
            affect_remove(&ch, waf);
            break;
        }
    }

    // Determine modifier
    int modifier(paf->duration);
    if (number_percent() <= get_skill(&ch, gsn_stonecraft))
        check_improve(&ch, NULL, gsn_stonecraft, true, 4);
    else
    {
        check_improve(&ch, NULL, gsn_stonecraft, false, 4);
        modifier /= 2;
    }

    // Grant the new effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_constructwaterwheel;
    af.level    = paf->level;
    af.modifier = UMAX(1, modifier);
    af.duration = af.modifier;
    af.location = APPLY_MANA;
    affect_to_char(&ch, &af);
}

void check_waterwheel_charging(OBJ_DATA & obj)
{
    static const int DurationStep(20);

    // Verify that the object is a crystal and the waterwheel is in a room
    if (obj.contains == NULL || obj.contains->material != material_lookup("crystal") || obj.contains->item_type != ITEM_GEM)
        return;

    // Verify that the waterwheel is in a room
    ROOM_INDEX_DATA * room(get_room_for_obj(obj));
    if (room == NULL)
        return;

    // Check for existing effect
    AFFECT_DATA * paf(get_obj_affect(obj.contains, gsn_constructwaterwheel));
    if (paf == NULL)
    {
        // First time, add the effect
        AFFECT_DATA af = {0};
        af.where    = TO_OBJECT;
        af.type     = gsn_constructwaterwheel;
        af.level    = obj.level;
        af.duration = DurationStep;
        af.bitvector = ITEM_HUM;
        affect_to_obj(obj.contains, &af);

        if (room != NULL)
            act("Inside the waterwheel, $p begins to emit a faint humming sound.", room->people, obj.contains, NULL, TO_ALL);

        return;
    }

    // Increase the duration
    int maxCharge(obj.level * 5);
    if (paf->duration + DurationStep < maxCharge)
    {
        paf->duration += DurationStep;
        act("$p's hum intensifies slightly as it absorbs the river's energy.", room->people, obj.contains, NULL, TO_ALL);
        return;
    }
    
    paf->duration = maxCharge;
    if (number_bits(1) == 0)
        act("From inside the waterwheel, $p hums steadily, unable to absorb any more power.", room->people, obj.contains, NULL, TO_ALL);
}

void do_constructwaterwheel(CHAR_DATA * ch, char * argument)
{
    // Check for skill
    int skill(get_skill(ch, gsn_constructwaterwheel));
    if (skill <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Check cooldown
    for (AFFECT_DATA * paf(get_affect(ch, gsn_constructwaterwheel)); paf != NULL; paf = get_affect(ch, gsn_constructwaterwheel, paf))
    {
        if (paf->location == APPLY_NONE)
        {
            send_to_char("You are not ready to construct another waterwheel yet.\n", ch);
            return;
        }
    }

    // Basic room checking
    if (ch->in_room == NULL || (ch->in_room->sector_type != SECT_WATER_SWIM && ch->in_room->sector_type != SECT_WATER_NOSWIM))
    {
        send_to_char("You may only construct a waterwheel on a river.\n", ch);
        return;
    }

    // Verify this room is a river
    unsigned int waterCount(0);
    for (unsigned int i(0); i <= Direction::Up; ++i)
    {
        ROOM_INDEX_DATA * room(Direction::Adjacent(*ch->in_room, static_cast<Direction::Value>(i)));
        if (room != NULL && (room->sector_type == SECT_WATER_SWIM || room->sector_type == SECT_WATER_NOSWIM))
            ++waterCount;
    }

    if (waterCount < 1 || waterCount > 2)
    {
        send_to_char("You may only construct a waterwheel on a river.\n", ch);
        return;
    }

    // Verify that there is not already a waterwheel here
    for (OBJ_DATA * obj(ch->in_room->contents); obj != NULL; obj = obj->next_content)
    {
        if (obj->pIndexData->vnum == OBJ_VNUM_WATERWHEEL)
        {
            send_to_char("There is already a waterwheel collecting the power of the river here.\n", ch);
            return;
        }
    }

    // Verify sufficient mana
    if (ch->mana < skill_table[gsn_constructwaterwheel].min_mana)
    {
        send_to_char("You are too weary to construct a waterwheel right now.\n", ch);
        return;
    }

    // Charge mana and lag
    expend_mana(ch, skill_table[gsn_constructwaterwheel].min_mana);
    WAIT_STATE(ch, skill_table[gsn_constructwaterwheel].beats);

    // Skill check
    if (number_percent() > skill)
    {
        check_improve(ch, NULL, gsn_constructwaterwheel, false, 1);
        act("You try to construct a waterwheel, but botch it completely.", ch, NULL, NULL, TO_CHAR);
        act("$n holds out a palm to the earth of the riverbank, but nothing happens.", ch, NULL, NULL, TO_ROOM);
        return;
    }

    check_improve(ch, NULL, gsn_constructwaterwheel, true, 1);

    // Create the waterwheel
    OBJ_DATA * wheel(create_object(get_obj_index(OBJ_VNUM_WATERWHEEL), 0));
    wheel->level = ch->level;
    wheel->timer = 200 + (ch->level * 6);
    obj_to_room(wheel, ch->in_room);

    // Echoes
    act("You seize hold of the earth of the riverbank with pure magic, reshaping it to your will!", ch, NULL, NULL, TO_CHAR);
    act("$n holds out a palm to the earth of the riverbank, which begins to reshape itself in response!", ch, NULL, NULL, TO_ROOM);
    act("In just moments there stands a functioning waterwheel, which begins to crank from the force of the river!", ch, NULL, NULL, TO_ALL);

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_constructwaterwheel;
    af.location = APPLY_NONE;
    af.level    = ch->level;
    af.duration = 360 - (UMAX(0, skill - 70) * 2);
    affect_to_char(ch, &af);
}

bool spell_causticblast(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam(dice(level, 4));

    // Blast them
    act("$n unleashes a blast of hissing acid upon $N!", ch, NULL, victim, TO_NOTVICT);
    act("You unleash a blast of hissing acid upon $N!", ch, NULL, victim, TO_CHAR);
    act("$n unleashes a blast of hissing acid upon you!", ch, NULL, victim, TO_VICT);
    
    if (saves_spell(level, ch, victim, DAM_ACID))
    {
        damage_old(ch, victim, dam / 2, sn, DAM_ACID, true);
        return true;
    }

    damage_old(ch, victim, dam, sn, DAM_ACID, true);
    if (!IS_VALID(victim) || victim->in_room != ch->in_room)
        return true;

    act("The acid eats away at you, leaving painful, ugly scars!", victim, NULL, NULL, TO_CHAR);
    act("The acid eats away at $m, leaving painful, ugly scars!", victim, NULL, NULL, TO_ROOM);
    
    // Apply -charisma
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = level / 2;
    af.location = APPLY_CHR;
    af.modifier = -1;
    affect_to_char(victim, &af);

    // Apply burning
    for (AFFECT_DATA * paf(get_affect(victim, sn)); paf != NULL; paf = get_affect(victim, sn, paf))
    {
        if (paf->location == APPLY_NONE)
        {
            paf->duration = UMAX(2, paf->duration);
            paf->modifier = UMIN(100, paf->modifier + 1);
            return true;
        }
    }

    af.duration = 2;
    af.location = APPLY_NONE;
    af.modifier = 1;
    affect_to_char(victim, &af);
    return true;
}

bool spell_hoarfrost(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity check
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("There is no solid ground here to rime over.\n", ch);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not prepared to conjure more hoarfrost yet.\n", ch);
        return false;
    }

    // Check for the room already having some
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("This place is already coated in hoarfrost.\n", ch);
        return false;
    }

    act("You spin in a slow circle, chanting softly as a thin layer of ice coats the ground.", ch, NULL, NULL, TO_CHAR);
    act("$n spins in a slow circle, chanting softly as a thin layer of ice coats the ground.", ch, NULL, NULL, TO_ROOM);

    // Add the effect
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.type     = sn;
    af.level    = level;
    af.duration = (level / 10);
    affect_to_room(ch->in_room, &af);

    // Add a cooldown
    af.duration = 8;
    affect_to_char(ch, &af);

    return true;
}

bool spell_ordainsanctum(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity check
    if (ch->in_room == NULL)
    {
        bug("Ordain sanctum called from null room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to ordain this ground.\n", ch);
        return false;
    }

    // Check for effect already present
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("This place has already been ordained as a sanctum.\n", ch);
        return false;
    }

    // Check for annointing oil
    OBJ_DATA * obj(lookup_obj_extra_flag(ch, ITEM_ANNOINTINGOIL));
    if (obj == NULL)
    {
        send_to_char("You cannot ordain this place without holy oil.\n", ch);
        return false;
    }

    // Ordain the room
    act("You murmur a soft chant, pouring out $p as you do so.", ch, obj, NULL, TO_CHAR);
    act("$n murmurs a soft chant, pouring out $p as $e does so.", ch, obj, NULL, TO_ROOM);
    act("As the last of the oil is poured out, you sense a certain protective aura fill this place.", ch, NULL, NULL, TO_ALL);

    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.type     = sn;
    af.level    = level;
    af.duration = (level / 12);
    affect_to_room(ch->in_room, &af);

    // Apply a cooldown
    af.duration = 14;
    affect_to_char(ch, &af);

    return true;
}

bool spell_corrosion(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for dodging
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    act("You spray a slurry of corroding minerals at $N!", ch, NULL, victim, TO_CHAR);
    act("$n sprays a slurry of corroding minerals at you!", ch, NULL, victim, TO_VICT);
    act("$n sprays a slurry of corroding minerals at $N!", ch, NULL, victim, TO_NOTVICT);
    if (check_dodge(ch, victim, NULL))
        return true;

    // Failed to dodge; build a list of candidate objects
    std::vector<OBJ_DATA*> wornObjects;
    for (OBJ_DATA * obj(victim->carrying); obj != NULL; obj = obj->next_content)
    {
        // Candidate objects must be worn, metal, and not already corroding
        if (obj->worn_on && !obj_is_affected(obj, sn) && material_table[obj->material].metal && !IS_SET(obj->extra_flags[0], ITEM_NODESTROY))
            wornObjects.push_back(obj);
    }

    if (wornObjects.empty())
    {
        act("The corrosive magics strike you, but can find no purchase on your equipment!", victim, NULL, NULL, TO_CHAR);
        act("The corrosive magics strike $n, but can find no purchase on $s equipment!", victim, NULL, NULL, TO_ROOM);
        return true;
    }

    // Prepare the effect
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = sn;
    af.level    = level;
    af.duration = (level / 8);

    // Hit some of the objects
    unsigned int count(number_range(2, 5));
    for (unsigned int i(0); i < count && !wornObjects.empty(); ++i)
    {
        // Apply the effect
        size_t index(number_range(0, wornObjects.empty() - 1));
        act("$p is struck by the corrosive magics!", ch, wornObjects[index], NULL, TO_ALL);
        affect_to_obj(wornObjects[index], &af);

        // Remove this object from the candidate set
        wornObjects[index] = wornObjects[wornObjects.size() - 1];
        wornObjects.pop_back();
    }
    
    return true;
}
