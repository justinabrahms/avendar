#include "EchoAffect.h"
#include "Forge.h"
#include "spells_fire_earth.h"
#include "spells_fire.h"

void do_forgeweapon(CHAR_DATA * ch, char * argument) {Forge::CreateWeapon(*ch, argument);}
void do_nameweapon(CHAR_DATA * ch, char * argument) {Forge::NameWeapon(*ch, argument);}

bool spell_scorchedearth(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Room check
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("There is no earth to scorch here!\n", ch);
        return false;
    }

    // Cooldown check
    for (AFFECT_DATA * paf(get_affect(ch, sn)); paf != NULL; paf = get_affect(ch, sn, paf))
    {
        if (paf->bitvector == 0)
        {
            send_to_char("You are not ready to blast the earth again.\n", ch);
            return false;
        }
    }

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.duration = 8;
    af.level    = level;
    affect_to_char(ch, &af);

    // Echoes
    act("You hurl a blast of flame at the ground, sending burning shrapnel everywhere!", ch, NULL, NULL, TO_CHAR);
    act("$n hurls a blast of flame at the ground, sending burning shrapnel everywhere!", ch, NULL, NULL, TO_ROOM);
    
    // Prepare the damage vector
    std::vector<DamageInfo> damage(2);
    damage[0].type = DAM_FIRE;
    damage[1].type = DAM_PIERCE;

    // Iterate the people in the room
    CHAR_DATA * victim_next;
    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim_next)
    {
        // Check for whether this victim should get hit
        victim_next = victim->next_in_room;
        if (is_same_group(ch, victim) || is_safe_spell(ch, victim, true))
            continue;

        if (number_bits(1) == 0)
        {
            // Calculate and deal damage
            damage[0].amount = dice(1, level);
            damage[1].amount = dice(7, level);
            damage_new(ch, victim, damage, sn, true);

            // Bail if that killed the victim
            if (!IS_VALID(victim) || victim->in_room != ch->in_room)
                continue;

            // Check whether to make bleeding wounds
            if (!saves_spell(level, ch, victim, DAM_PIERCE))
            {
                act("The shrapnel leaves behind a nasty bleeding wound!", ch, NULL, NULL, TO_ALL);
                
                AFFECT_DATA baf = {0};
                baf.where   = TO_OAFFECTS;
                baf.type    = sn;
                baf.duration = number_range(2, 8);
                baf.level   = level;
                baf.bitvector = AFF_BLEEDING;
                affect_to_char(victim, &baf);
            }
        }
        else
        {
            act("Pieces of shrapnel fly past, narrowly missing you.", victim, NULL, NULL, TO_CHAR);
            act("Pieces of shrapnel fly past $n, narrowly missing $m.", victim, NULL, NULL, TO_ROOM);
        }

        // Check for stunning
        if (victim->in_room->sector_type == SECT_UNDERGROUND && !saves_spell(level, ch, victim, DAM_SOUND))
        {
            act("You are deafened by the concussive blast as it reverberates around this place!", victim, NULL, NULL, TO_CHAR);
            act("$n seems deafened by the sound of the blast!", victim, NULL, NULL, TO_ROOM);
            
            AFFECT_DATA daf = {0};
            daf.where   = TO_OAFFECTS;
            daf.type    = sn;
            daf.level   = level;
            daf.duration = number_range(2, 8);
            daf.bitvector = AFF_DEAFEN;
            affect_to_char(victim, &daf);
        }
    }
    return true;
}

void handleUpdateLavaForge(ROOM_INDEX_DATA & room, AFFECT_DATA & lavaEffect)
{
    switch (number_range(0, 100))
    {
        case 0: act("The lava forge burbles softly, sending waves of heat from the rift.", room.people, NULL, NULL, TO_ALL); break;
        case 1: act("Faint puffs of smoke rise from the tear in the earth, curling in the air before dissipating.", room.people, NULL, NULL, TO_ALL); break;
        case 2: act("With a hiss of heat, a bit of rock melts into the lava, vanishing almost instantly.", room.people, NULL, NULL, TO_ALL); break;
        case 3: act("A ripple spreads slowly through the lava before subsiding.", room.people, NULL, NULL, TO_ALL); break;
        case 4: act("The air above the lava forge shimmers slightly, distorting the horizon.", room.people, NULL, NULL, TO_ALL); break;

        case 5: 
            if (!room_is_affected(&room, gsn_smoke))
            {
                act("A sudden burst of smoke erupts from the tear in the ground, swiftly filling the air!", room.people, NULL, NULL, TO_ALL);
                fillRoomWithSmoke(NULL, &room, lavaEffect.level, 1);
            }
            break;
    }
}

bool spell_lavaforge(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to call forth another lava forge.\n", ch);
        return false;
    }

    // Check for room type
    if (ch->in_room == NULL)
    {
        send_to_char("You cannot call a lava forge here.\n", ch);
        return false;
    }

    switch (ch->in_room->sector_type)
    {
        case SECT_HILLS:
        case SECT_MOUNTAIN:
        case SECT_UNDERGROUND:
        case SECT_DESERT:
            break;

        default:
            send_to_char("You cannot call forth a lava forge in such a place.\n", ch);
            return false;
    }

    // Check whether there is already a lava forge
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("This place is already flowing with the power of a lava forge!\n", ch);
        return false;
    }

    // Set up the handler
    struct CallbackHandler
    {
        static bool HandleMove(CHAR_DATA * ch, ROOM_INDEX_DATA *, EchoAffect *, void *)
        {
            HandleCancel(ch);
            return true;
        }

        static bool HandlePositionChange(CHAR_DATA * ch, int newPos, EchoAffect *, void *)
        {
            HandleCancel(ch);
            return true;
        }

        static bool HandleCast(CHAR_DATA * ch, int, int, void *, int, EchoAffect *, void *)
        {
            HandleCancel(ch);
            return true;
        }

        static bool CheckImprove(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            check_improve(ch, NULL, gsn_lavaforge, true, 0);
            return false;
        }

        static bool Finish(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            // Add the room effect
            check_improve(ch, NULL, gsn_lavaforge, true, 0);

            AFFECT_DATA af = {0};
            af.type     = gsn_lavaforge;
            af.level    = reinterpret_cast<int>(tag);
            af.duration = 16;
            affect_to_room(ch->in_room, &af);
            return false;
        }

        private:
            static void HandleCancel(CHAR_DATA * ch)
            {
                send_to_char("You let the power of your summoning dissipate fruitlessly, leaving you exhausted.\n", ch);
                act("$n abruptly ceases his invocation, and the gathering powers subside.", ch, NULL, NULL, TO_ROOM);
            }
    };

    // Echoes
    act("You strike the ground with an open palm, channeling your burning will into the earth!", ch, NULL, NULL, TO_CHAR);
    act("$n strikes the ground with an open palm, a look of intense focus on $s face!", ch, NULL, NULL, TO_ROOM);
 
    // Prepare the echoAffect
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    echoAff->SetMoveCallback(&CallbackHandler::HandleMove);
    echoAff->SetCastCallback(&CallbackHandler::HandleCast);

    // Add in lines   
    echoAff->AddLine(&CallbackHandler::CheckImprove,
                    "The land shakes in response, sending a tremor through this place!",
                    "The land shakes in response, sending a tremor through this place!");

    echoAff->AddLine(&CallbackHandler::CheckImprove,
                    "With a tearing sound, the ground splits suddenly open! Smoke and heat issue from the rift!",
                    "With a tearing sound, the ground splits suddenly open! Smoke and heat issue from the rift!");

    echoAff->AddLine(&CallbackHandler::Finish, 
                    "Lava pours forth, flowing neatly into a channel shaped as a forge for your work.",
                    "Lava pours forth, flowing neatly into a channel shaped as a forge.");

    // Finish applying the effect, then add a cooldown
    echoAff->SetTag(reinterpret_cast<void*>(level));
    EchoAffect::ApplyToChar(ch, echoAff);
    
    int skill(get_skill(ch, sn));
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.duration = 120 * (25 - (UMAX(0, (skill - 70)) / 6));
    af.level    = level;
    affect_to_char(ch, &af);
    return true;
}

bool spell_moltenshield(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are already encased in a molten shield\n", ch);
        return false;
    }

    // High skill bonus
    int bonus = 0;
    int skill = get_skill(ch, sn);
    if (skill >= 90) ++bonus;
    if (skill >= 100) ++ bonus;

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = level / 6;
    af.modifier = -1 * (level + (bonus * 50));
    af.location = APPLY_AC;
    affect_to_char(ch, &af);

    // Add in res physical
    af.modifier = (level / 5) + (bonus * 5);
    af.location = APPLY_RESIST_WEAPON;
    affect_to_char(ch, &af);

    // Add in res lightning
    af.location = APPLY_RESIST_LIGHTNING;
    affect_to_char(ch, &af);

    act("A flowing shell of molten rock forms about you.", ch, NULL, NULL, TO_CHAR);
    act("A flowing shell of molten rock forms about $n.", ch, NULL, NULL, TO_ROOM);
    return true;
}

bool spell_summonlavaelemental(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You don't feel you can summon another lava elemental yet.\n", ch);
        return false;
    }

    // Only one pet for now
    if (ch->pet != NULL)
    {
        send_to_char("You already have a loyal follower, and cannot bind more loyalties.\n", ch);
        return false;
    }

    // Sanity check
    if (ch->in_room == NULL)
    {
        send_to_char("An error has occurred, please contact the gods.\n", ch);
        bug("Summon lava elemental called from NULL room", 0);
        return false;
    }

    // Check location
    switch (ch->in_room->sector_type)
    {
        case SECT_HILLS:
        case SECT_MOUNTAIN:
        case SECT_UNDERGROUND:
            break;

        default:
            send_to_char("There isn't enough stone here to call forth a lava elemental.\n", ch);
            return false;
    }

    struct CallbackHandler
    {
        static bool HandleMoveFrom(CHAR_DATA * ch, ROOM_INDEX_DATA * room, EchoAffect * thisAffect, void * tag)
        {
            HandleCancel(ch, thisAffect, tag);
            return true;
        }

        static bool HandlePositionChange(CHAR_DATA * ch, int newPos, EchoAffect * thisAffect, void * tag)
        {
            switch (newPos)
            {
                case POS_DEAD:
                case POS_MORTAL:
                case POS_INCAP:
                case POS_STUNNED:
                case POS_SLEEPING:
                case POS_FIGHTING:
                    HandleCancel(ch, thisAffect, tag);
                    return true;
            }

            return false;
        }

        static bool MakeElemental(CHAR_DATA * ch, EchoAffect * thisAffect, void * tag)
        {
            int level(reinterpret_cast<int>(tag));

            // Build the elemental
            CHAR_DATA * pet(create_mobile(get_mob_index(MOB_VNUM_LAVAELEMENTAL)));
            pet->level = level;
            pet->damroll = (level / 2);
            pet->hitroll = pet->damroll;
            pet->damage[0] = 3;
            pet->damage[1] = (level / 2);
            pet->damage[2] = (level / 10);
            pet->hit = dice(level / 2, level / 2) + 500;
            pet->max_hit = pet->hit;
            char_to_room(pet, ch->in_room);
            thisAffect->SetTag(pet);

            // Apply cooldown
            AFFECT_DATA af = {0};
            af.where     = TO_AFFECTS;
            af.type      = gsn_summonlavaelemental;
            af.level     = level;
            af.duration  = 50;
            affect_to_char(ch, &af);
            return false;
        }

        static bool FinishSpell(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            CHAR_DATA * pet(static_cast<CHAR_DATA*>(tag));
            ch->pet = pet;
            pet->master = ch;
            pet->leader = ch;
            return false;
        }

        private:
            static void HandleCancel(CHAR_DATA * ch, EchoAffect * thisAffect, void * tag)
            {
                send_to_char("You sense the gathering forces of earth and fire disperse as you abandon your summoning.\n", ch);
                if (thisAffect->LinesRemaining() <= 1)
                {
                    CHAR_DATA * pet(static_cast<CHAR_DATA *>(tag));
                    act("$n flows back into the ground with a gurgle of liquid rock.", pet, NULL, NULL, TO_ROOM);
                    extract_char(pet, TRUE);
                    thisAffect->SetTag(NULL);
                }
            }
    };

    // Handle echoes
    act("$n strikes the ground with an open palm, holding it against the earth.", ch, NULL, NULL, TO_ROOM);
    act("You strike the ground with an open palm, holding it against the earth.", ch, NULL, NULL, TO_CHAR);
    
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetTag(reinterpret_cast<void*>(level));
    echoAff->SetMoveCallback(&CallbackHandler::HandleMoveFrom);
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    echoAff->AddLine(NULL,
                    "The stone around your hand ripples in concentric circles as it glows red-hot and begins to melt!",
                    "The stone around $n's hand ripples in concentric circles as it glows red-hot and begins to melt!");

    echoAff->AddLine(&CallbackHandler::MakeElemental,
                    "You punch your hand through the molten rock! As you pull it back out, the liquid stone flows up after, forming into a humanoid shape.",
                    "$n punches $s hand through the molten rock! As $e pulls it back out, the liquid stone flows up after, forming into a humanoid shape.");

    echoAff->AddLine(&CallbackHandler::FinishSpell,
                    "You bark out a final word of power, and the lava elemental falls in line behind you.",
                    "$n barks out a final word of power, and the lava elemental falls in line behind $m.");

    EchoAffect::ApplyToChar(ch, echoAff);
    return true;
}

bool spell_harrudimfire(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    act("$n hurls a flaming ball of tar at you!", ch, NULL, victim, TO_VICT);
    act("You hurl a flaming ball of tar at $N!", ch, NULL, victim, TO_CHAR);

    // Calculate damage
    int dam(dice(level, 5));
    if (saves_spell(level, ch, victim, DAM_BASH))
        dam /= 2;

    damage_old(ch, victim, dam, sn, DAM_BASH, true);
    if (!saves_spell(level, ch, victim, DAM_FIRE))
    {
        send_to_char("Some of the flaming pitch clings to the wound it opened, causing painful burns!\n", victim);
        send_to_char("Some of the flaming pitch clings to the wound it opened, causing painful burns!\n", ch);
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_aggravatewounds;
        af.level    = ch->level;
        af.duration = 2;
        affect_to_char(victim, &af);
    }
    return true;
}
