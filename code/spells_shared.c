#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "spells_spirit_earth.h"
#include "EchoAffect.h"
#include <cstring>
/* External declarations */
DECLARE_DO_FUN(do_look);

bool spell_armor( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
            send_to_char("You are already armored.\n\r",ch);
        else
            act("$N is already armored.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24 * (check_durablemagics(*ch) ? 2 : 1);
    af.modifier  = -20 - (level / 2);
    af.location  = APPLY_AC;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You feel someone protecting you.\n\r", victim );
    if ( ch != victim )
        act("$N is protected by your magic.",ch,NULL,victim,TO_CHAR);
    return TRUE;
}

bool spell_cancellation( int sn, int level, CHAR_DATA *ch, void *vo,int target ){
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool found = FALSE;
    int dam;

    level += 2;

    if (IS_NPC(ch) && !IS_NPC(victim))
    {
        send_to_char("You failed.\n\r",ch);
        return FALSE;
    }

    if (!IS_NPC(ch) && victim->master != ch && !is_same_group(ch, victim))
    {
        send_to_char("You failed.\n\r",ch);
        return FALSE;
    }

    /* unlike dispel magic, the victim gets NO save */

    /* begin running through the spells */

    if (check_dispel(level, victim, skill_lookup("fury of the inferno")))
    {	
	found = TRUE;
	act("$n's fury subsides.", victim, NULL, NULL, TO_ROOM);
    }

    if (check_dispel(level, victim, skill_lookup("durability")))
    {
	found = TRUE;
	act("$n appears to be less durable.", victim, NULL, NULL, TO_ROOM);
    }

    if (check_dispel(level, victim, skill_lookup("protection from normal missiles")))
    {
	found = TRUE;
	act("$n is no longer protected from missiles.", victim, NULL, NULL, TO_ROOM);
    }

    if (check_dispel(level, victim, skill_lookup("petrify")))
    {
	found = TRUE;
	act("$n appears able to move more freely.", victim, NULL, NULL, TO_ROOM);
    }	

    if (check_dispel(level, victim, skill_lookup("windwall")))
        {
        found = TRUE;
        act("$n's wind wall disappears.", victim, NULL, NULL, TO_ROOM);
        act("Your wind wall disappears.", victim, NULL, NULL, TO_CHAR);
        REMOVE_BIT(victim->act, PLR_FREEZE);
        }

    if (check_dispel(level, victim, skill_lookup("stabilize")))
        {
        found = TRUE;
        act("$n's link to the ground dissolves.", victim, NULL, NULL, TO_ROOM);
        act("Your link to the ground dissolves.", victim, NULL, NULL, TO_CHAR);
        REMOVE_BIT(victim->act, PLR_FREEZE);
        }

    if (check_dispel(level, victim, skill_lookup("fleshtostone")))
        {
        found = TRUE;
        act("$n's stone encasing crumbles.", victim, NULL, NULL, TO_ROOM);
        act("Your stone encasing crumbles.", victim, NULL, NULL, TO_CHAR);
        REMOVE_BIT(victim->act, PLR_FREEZE);
        }

    if (check_dispel(level, victim, skill_lookup("encase")))
    {
        found = TRUE;
        act("The ice encasing $n melts away.", victim, NULL, NULL, TO_ROOM);
        act("The ice encasing you melts away.", victim, NULL, NULL, TO_CHAR);
        REMOVE_BIT(victim->act, PLR_FREEZE);
    }

    if (check_dispel(level, victim, skill_lookup("stone shell")))
        {
        found = TRUE;
        act("Your stone shell crumbles.", victim, NULL, NULL, TO_CHAR);
        }

    if (check_dispel(level + 10, victim, skill_lookup("backfire")))
        {
        found = TRUE;
        dam = number_range(level-10, level+10 );
        if ( saves_spell( level,NULL, ch,DAM_ENERGY) )
                dam /= 2;
        damage_old( ch,ch, dam, sn, DAM_ENERGY ,TRUE);
        }

    if (check_dispel(level, victim,skill_lookup("nightfears")))
        {
        act("$n's fears of the night dissipates.", victim, NULL, NULL, TO_ROOM);        found = TRUE;
        }

    if (check_dispel(level, victim,skill_lookup("power word fear")))
        {
        act("$n's terror dissipates.", victim, NULL, NULL, TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level, victim,skill_lookup("possession")))
        {
        act("$n shakes $s head as $e regains control of $s own body.", victim,
NULL, NULL, TO_ROOM);
        act("Your spell of possession shattered, your spirit retreats to your own body.", victim, NULL, NULL, TO_CHAR);
                    if (ch->prompt != NULL)
                        {
                        free_string(ch->prompt);
                        ch->prompt = NULL;
                        }

                act("$n shakes $s head clear as $s spirit returns.", victim->desc->original, NULL, NULL, TO_ROOM);

                WAIT_STATE(victim->desc->original, UMAX(victim->desc->original->wait, 5*PULSE_VIOLENCE));
                victim->desc->original->hit /= 2;
                if (victim->desc != NULL)
                   {
                        victim->desc->character       = victim->desc->original;
                        victim->desc->original        = NULL;
                        victim->desc->character->desc = victim->desc;
                        victim->desc                  = NULL;
                   }
        found = TRUE;
        }


    if (check_dispel(level, victim,skill_lookup("agony")))
        {
        act("$n relaxes as $s agony fades.", victim, NULL, NULL, TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level, victim, skill_lookup("protection from fire")))
        {
        act("$n's aura of cold disappears.", victim, NULL, NULL, TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level, victim,skill_lookup("cloak of the void")))
        {
        act("$n shivers as $s protection from the void slips away.", victim, NULL, NULL, TO_ROOM);
        found = TRUE;
        }
    if (check_dispel(level, victim, skill_lookup("mindshell")))
        {
        act("$n's mental shield fades away.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("water breathing")))
        {
        act("$n becomes unable to breathe underwater.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("sustenance")))
        {
        act("$n looks less sustained.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("subdue")))
        {
        act("$n looks less subdued.", victim,NULL,NULL, TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("paradise")))
        {
        act("$n looks more aware of $s surroundings.",
                victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("shatter")))
        {
        act("$n's skin stops glittering.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("manabarbs")))
        {
        act("$n looks more comfortable with $s magic.",
                victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("earthbind")))
        {
        act("$n looks lighter.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("armor")))
        {
        act("$n looks less protected.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("bless")))
        {
        act("$n looks less blessed.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("immolation")))
    {
        found = TRUE;
        act("$n no longer appears as enflamed...", victim,NULL,NULL,TO_ROOM);
    }

    if (check_dispel(level,victim,skill_lookup("regeneration")))
        {
        act("$n stops regenerating so quickly.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }
/*
    if (check_dispel(level,victim,skill_lookup("fortify")))
    {
        act("$n looks less healthy.", victim,NULL,NULL,TO_ROOM);
        victim->hit = UMIN(victim->hit, victim->max_hit);
        found = TRUE;
    }
*/

    if (check_dispel(level,victim,skill_lookup("blindness")))
    {
        found = TRUE;
        act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM);
    }

    if (check_dispel(level,victim,skill_lookup("calm")))
    {
        found = TRUE;
        act("$n no longer looks so peaceful...",victim,NULL,NULL,TO_ROOM);
    }

    if (check_dispel(level,victim,skill_lookup("change sex")))
    {
        found = TRUE;
        act("$n looks more like $mself again.",victim,NULL,NULL,TO_ROOM);
    }

    if (check_dispel(level,victim,skill_lookup("charm person")))
    {
        found = TRUE;
        act("$n regains $s free will.",victim,NULL,NULL,TO_ROOM);
    }

    if (check_dispel(level,victim,skill_lookup("chill touch")))
    {
        found = TRUE;
        act("$n looks warmer.",victim,NULL,NULL,TO_ROOM);
    }

    if (check_dispel(level,victim,skill_lookup("curse")))
        {
        act("$n looks more comfortable.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("detect evil")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("detect good")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("detect invis")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("detect magic")))
        found = TRUE;
    if (check_dispel(level,victim,gsn_coronalglow))
    {
        act("$n's outline fades.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("fly")))
    {
        act("$n falls to the ground!",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("frenzy")))
    {
        act("$n no longer looks so wild.",victim,NULL,NULL,TO_ROOM);;
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("giant strength")))
    {
        act("$n no longer looks so mighty.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("haste")))
    {
        act("$n is no longer moving so quickly.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("infravision")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("invis")))
    {
        act("$n fades into existence.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("mass invis")))
    {
        act("$n fades into existence.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("pass door")))
        {
        act("$n looks more solid.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("protection evil")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("protection good")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("sanctuary")))
    {
        act("The white aura around $n's body vanishes.",
            victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("shield")))
    {
        act("The shield protecting $n vanishes.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("sleep")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("slow")))
    {
        act("$n is no longer moving so slowly.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("stone skin")))
    {
        act("$n's skin regains its normal texture.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("weaken")))
    {
        act("$n looks stronger.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (found)
        send_to_char("Ok.\n\r",ch);
    else
        send_to_char("Spell failed.\n\r",ch);

    return TRUE;
}


bool spell_water_breathing( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You can already breathe underwater.\n\r",ch);
        else
          act("$N can already breathe underwater.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    // Water breathing cancels drowning
    if (is_affected(victim, gsn_drown))
    {
        send_to_char("With a gasp of air, you begin to breathe despite your flooded lungs.\n", victim);
        affect_strip(victim, gsn_drown);
    }

    af.where     = TO_PAFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = AFF_AIRLESS;
    affect_to_char( victim, &af );

    if (ch->class_num == global_int_class_druid)
        send_to_char("Gill-like slits form on your neck, and you feel able to breathe underwater.\n\r", victim);
    else
        send_to_char( "You feel able to breathe underwater.\n\r", victim );

    if ( ch != victim )
        act("$N is now able to breathe underwater.",ch,NULL,victim,TO_CHAR);
    return TRUE;
}

bool spell_create_spring(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *spring=NULL;

    if ((ch->in_room->sector_type == SECT_WATER_SWIM)
     || (ch->in_room->sector_type == SECT_WATER_NOSWIM)
     || (ch->in_room->sector_type == SECT_AIR)
     || (ch->in_room->sector_type == SECT_UNDERWATER))
    {
        send_to_char("You cannot create a spring here.\n\r", ch);
        return FALSE;
    }
    for (spring=ch->in_room->contents;spring;spring=spring->next_content)
	if (spring->pIndexData->vnum == OBJ_VNUM_SPRING)
	{
	    send_to_char("A spring already flows here.\n\r",ch);
	    return FALSE;
	}
    spring = create_object( get_obj_index( OBJ_VNUM_SPRING ), 0 );
    spring->timer = level;

    if (ch->in_room && room_is_affected(ch->in_room, gsn_poisonspirit))
	spring->value[3] = 1;

    obj_to_room( spring, ch->in_room );
    act( "$p flows from the ground.", ch, spring, NULL, TO_ROOM );
    act( "$p flows from the ground.", ch, spring, NULL, TO_CHAR );
    return TRUE;
}

bool spell_detect_invis( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( IS_AFFECTED(victim, AFF_DETECT_INVIS) )
    {
        send_to_char("You can already see invisible.\n\r",ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level * (check_durablemagics(*ch) ? 2 : 1);
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_INVIS;
    affect_to_char( victim, &af );
    if (ch->race == global_int_race_shuddeni)
	send_to_char("Your senses tingle.\n\r", victim);
    else
        send_to_char( "Your eyes tingle.\n\r", victim );

    return TRUE;
}

bool spell_identify( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf, * poisoned;
    int i, masked_gsn,vials=0;

    act("You hold $p with both hands, and chant softly.", ch, obj, NULL, TO_CHAR);
    act("$n holds $p with both hands, and chants softly.", ch, obj, NULL, TO_ROOM);

    act("You are filled with a rush of knowledge!", ch, NULL, NULL, TO_CHAR);

    sprintf( buf,
        "Object: '%s' is type %s.\n\rExtra flags %s",
        obj->short_descr,
        item_name(obj->item_type),
        extra_bit_name(obj->extra_flags[0], 0, true)
        );

    send_to_char( buf, ch );

    if (str_cmp(extra_bit_name(obj->extra_flags[1], 1, true), "none"))
    {
	sprintf(buf, " %s", extra_bit_name(obj->extra_flags[1], 1, true));
	send_to_char(buf, ch);
    }
	if (str_cmp(extra_bit_name(obj->extra_flags[2], 2, true), "none"))
	{
		sprintf(buf, " %s", extra_bit_name(obj->extra_flags[2], 2));
		send_to_char(buf, ch);
	}

    sprintf(buf, ".\n\rWeight is %d.%d, level is %d.\n\r",
        obj->weight / 10,
	obj->weight % 10,
        obj->level );

    send_to_char(buf, ch);

    sprintf( buf,"Material is %s.\n\r", material_table[obj->material].name);
    send_to_char( buf, ch );

    switch ( obj->item_type )
    {
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_OIL:
        
	masked_gsn = -1;
	if ((poisoned = get_obj_affect(obj, gsn_subtlepoison)) != NULL)
		masked_gsn = poisoned->modifier;
	   
	sprintf( buf, "Level %d spells of:", obj->value[0] );
        send_to_char( buf, ch );

        if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
        {
            send_to_char( " '", ch );
            send_to_char( masked_gsn > 0 ? skill_table[masked_gsn].name : skill_table[obj->value[1]].name, ch );
            send_to_char( "'", ch );
        }

        if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
        {
            send_to_char( " '", ch );
            send_to_char( skill_table[obj->value[2]].name, ch );
            send_to_char( "'", ch );
        }

        if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
        {
            send_to_char( " '", ch );
            send_to_char( skill_table[obj->value[3]].name, ch );
            send_to_char( "'", ch );
        }

        if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL)
        {
            send_to_char(" '",ch);
            send_to_char(skill_table[obj->value[4]].name,ch);
            send_to_char("'",ch);
        }

        send_to_char( ".\n\r", ch );
        break;

    case ITEM_WAND:
    case ITEM_STAFF:
        sprintf( buf, "Has %d charges of level %d",
            obj->value[2], obj->value[0] );
        send_to_char( buf, ch );

        if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
        {
            send_to_char( " '", ch );
            send_to_char( skill_table[obj->value[3]].name, ch );
            send_to_char( "'", ch );
        }

        send_to_char( ".\n\r", ch );
        break;

    case ITEM_ARROW:
        buf[0] = '\0';
        for (i = 2; i < 5; i++)
        {
            if (obj->value[i] > 0)
            {
                if (buf[0] != '\0')
                    sprintf(buf, "%s, %s", buf, skill_table[obj->value[i]].name);
                else
                    sprintf(buf, "Imbued with %s", skill_table[obj->value[i]].name);
            }
        }

        if (buf[0] != '\0')
        {
            sprintf(buf, "%s.\n\r", buf);
            send_to_char(buf, ch);
        }
        break;

    case ITEM_DRINK_CON:
        sprintf(buf,"It holds %s-colored %s.\n\r",
            liq_table[obj->value[2]].liq_color,
            liq_table[obj->value[2]].liq_name);
        send_to_char(buf,ch);
        break;

    case ITEM_POTIONCONTAINER:
	vials=obj->value[0]+obj->value[1]+obj->value[2]+obj->value[3]+obj->value[4];
	if (vials == 0)
	{
	    sprintf(buf,"It can hold %i vials, and contains none.\n\r",obj->weight);
	    send_to_char(buf,ch);
	}
	else
	{
	    sprintf(buf,"It can hold %i vials, and contains %i:",obj->weight,vials);
            act(buf,ch,obj,NULL,TO_CHAR);
            buf[0] = '\0';
            if (obj->value[0] > 0)
                sprintf(buf,"%i explosive",obj->value[0]);
            if (obj->value[1] > 0)
                if (buf[0] == '\0')
                    sprintf(buf,"%i adhesive",obj->value[1]);
                else
                    sprintf(buf,"%s, %i adhesive",str_dup(buf),obj->value[1]);
            if (obj->value[2] > 0)
                if (buf[0] == '\0')
                    sprintf(buf,"%i anesthetic",obj->value[2]);
                else
                    sprintf(buf,"%s, %i anesthetic",str_dup(buf),obj->value[2]);
            if (obj->value[3] > 0)
                if (buf[0] == '\0')
                    sprintf(buf,"%i toxin",obj->value[3]);
                else
                    sprintf(buf,"%s, %i toxin",str_dup(buf),obj->value[3]);
            if (obj->value[4] > 0)
                if (buf[0] == '\0')
                    sprintf(buf,"%i suppresive",obj->value[4]);
                else
                    sprintf(buf,"%s, %i suppressive",str_dup(buf),obj->value[4]);
            strcat(buf,".\n\r");
            send_to_char(buf,ch);
        }
        break;

    case ITEM_CONTAINER:
        sprintf(buf,"Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
            obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));
        send_to_char(buf,ch);
        if (obj->value[4] != 100)
        {
            sprintf(buf,"Weight multiplier: %d%%\n\r",
                obj->value[4]);
            send_to_char(buf,ch);
        }
        break;

    case ITEM_WEAPON:
        send_to_char("Weapon type is ",ch);
        switch (obj->value[0])
        {
            case(WEAPON_KNIFE)  : send_to_char("knife.\n\r",ch);        break;
            case(WEAPON_STAFF)  : send_to_char("staff.\n\r",ch);        break;
            case(WEAPON_EXOTIC) : send_to_char("exotic.\n\r",ch);       break;
            case(WEAPON_SWORD)  : send_to_char("sword.\n\r",ch);        break;
            case(WEAPON_DAGGER) : send_to_char("dagger.\n\r",ch);       break;
            case(WEAPON_SPEAR)  : send_to_char("spear.\n\r",ch);	break;
            case(WEAPON_MACE)   : send_to_char("mace/club.\n\r",ch);    break;
            case(WEAPON_AXE)    : send_to_char("axe.\n\r",ch);          break;
            case(WEAPON_FLAIL)  : send_to_char("flail.\n\r",ch);        break;
            case(WEAPON_WHIP)   : send_to_char("whip.\n\r",ch);         break;
            case(WEAPON_POLEARM): send_to_char("polearm.\n\r",ch);      break;
            default             : send_to_char("unknown.\n\r",ch);      break;
        }
        if (obj->pIndexData->new_format)
        {
            int aveDamTenths(((1 + obj->value[2]) * obj->value[1] * 10) / 2);
            sprintf(buf,"Damage is %dd%d (average %d.%d).\n",
                obj->value[1],obj->value[2], aveDamTenths / 10, aveDamTenths % 10);
        }
        else
            sprintf( buf, "Damage is %d to %d (average %d).\n\r",
                obj->value[1], obj->value[2],
                ( obj->value[1] + obj->value[2] ) / 2 );
        send_to_char( buf, ch );
        if (obj->value[4])  /* weapon flags */
        {
            sprintf(buf,"Weapons flags: %s\n\r",weapon_bit_name(obj->value[4]));            send_to_char(buf,ch);
        }
        break;

    case ITEM_BOW:
        if (obj->pIndexData->new_format)
            sprintf(buf,"Damage is %dd%d (average %d).\n\r",
                obj->value[1],obj->value[2],
                (1 + obj->value[2]) * obj->value[1] / 2);
        else
            sprintf( buf, "Damage is %d to %d (average %d).\n\r",
                obj->value[1], obj->value[2],
                ( obj->value[1] + obj->value[2] ) / 2 );
        send_to_char(buf, ch);
        break;

    case ITEM_ARMOR:
        sprintf( buf,
        "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\n\r",
            obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
        send_to_char( buf, ch );
        break;
    }

    if (!obj->enchanted)
    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->location != APPLY_NONE && paf->location != APPLY_HIDE && paf->modifier != 0 )
        {
            sprintf( buf, "Affects %s by %d.\n\r",
                affect_loc_name( paf->location ), paf->modifier );
            send_to_char(buf,ch);
            if (paf->bitvector)
            {
                switch(paf->where)
                {
                    case TO_AFFECTS:
                        sprintf(buf,"Adds %s affect.\n",
                            affect_bit_name(paf));
                        break;
                    case TO_OBJECT:
                        sprintf(buf,"Adds %s object flag.\n",
                            extra_bit_name(paf->bitvector, 0));
                        break;
		    case TO_OBJECT1:  sprintf(buf,"Adds %s object flag.\n", extra_bit_name(paf->bitvector, 1)); break;
		    case TO_OBJECT2:  sprintf(buf,"Adds %s object flag.\n", extra_bit_name(paf->bitvector, 2)); break;
                    case TO_IMMUNE:
                        sprintf(buf,"Adds immunity to %s.\n",
                            imm_bit_name(paf->bitvector));
                        break;
                    case TO_RESIST:
                        sprintf(buf,"Adds resistance to %s.\n\r",
                            imm_bit_name(paf->bitvector));
                        break;
                    case TO_VULN:
                        sprintf(buf,"Adds vulnerability to %s.\n\r",
                            imm_bit_name(paf->bitvector));
                        break;
                    default:
                        sprintf(buf,"Unknown bit %d: %d\n\r",
                            paf->where,paf->bitvector);
                        break;
                }
                send_to_char( buf, ch );
            }
        }
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->location != APPLY_NONE && paf->location != APPLY_HIDE && paf->modifier != 0 )
        {
            sprintf( buf, "Affects %s by %d",
                affect_loc_name( paf->location ), paf->modifier );
            send_to_char( buf, ch );
            if ( paf->duration > -1)
                sprintf(buf,", %d%s hours.\n\r",
		    paf->duration,
		    ((paf->duration % 2) == 0) ? "" : ".5");
            else
                sprintf(buf,".\n\r");
            send_to_char(buf,ch);
            if (paf->bitvector)
            {
                switch(paf->where)
                {
                    case TO_AFFECTS:
                        sprintf(buf,"Adds %s affect.\n",
                            affect_bit_name(paf));
                        break;
                    case TO_OBJECT:
                        sprintf(buf,"Adds %s object flag.\n",
                            extra_bit_name(paf->bitvector, 0));
                        break;
		    case TO_OBJECT1: sprintf(buf,"Adds %s object flag.\n", extra_bit_name(paf->bitvector, 1)); break;
		    case TO_OBJECT2: sprintf(buf,"Adds %s object flag.\n", extra_bit_name(paf->bitvector, 2)); break;
                    case TO_WEAPON:
                        sprintf(buf,"Adds %s weapon flags.\n",
                            weapon_bit_name(paf->bitvector));
                        break;
                    case TO_IMMUNE:
                        sprintf(buf,"Adds immunity to %s.\n",
                            imm_bit_name(paf->bitvector));
                        break;
                    case TO_RESIST:
                        sprintf(buf,"Adds resistance to %s.\n\r",
                            imm_bit_name(paf->bitvector));
                        break;
                    case TO_VULN:
                        sprintf(buf,"Adds vulnerability to %s.\n\r",
                            imm_bit_name(paf->bitvector));
                        break;
                    default:
                        sprintf(buf,"Unknown bit %d: %d\n\r",
                            paf->where,paf->bitvector);
                        break;
                }
                send_to_char(buf,ch);
            }
        }
    }

    return TRUE;
}

bool spell_invis( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    int multiplier(1);
    if (number_percent() <= get_skill(ch, gsn_endlessfacade))
        multiplier = 2;

    /* object invisibility */
    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *) vo;

        if (IS_OBJ_STAT(obj,ITEM_INVIS))
        {
            act("$p is already invisible.",ch,obj,NULL,TO_CHAR);
            return FALSE;
        }

        af.where        = TO_OBJECT;
        af.type         = sn;
        af.level        = level;
        af.duration     = (level + 12) * multiplier;
        af.location     = APPLY_NONE;
        af.modifier     = 0;
        af.bitvector    = ITEM_INVIS;
        affect_to_obj(obj,&af);

        act("$p fades out of sight.",ch,obj,NULL,TO_ALL);
        return TRUE;
    }

    /* character invisibility */
    victim = (CHAR_DATA *) vo;

    if ( IS_AFFECTED(victim, AFF_INVISIBLE) )
    {
	if (victim==ch)
	{
	    send_to_char("You are already invisible.\n\r", ch);
	    return FALSE;
	}
	else 
	{
            act("$N is already invisible.",ch,NULL,victim,TO_CHAR);
	    return FALSE;
	}
    }
    
    if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) )
        {
        send_to_char("Invisibility can't overcome the glow.\n\r", ch);
        return FALSE;
        }

    act( "$n fades out of existence.", victim, NULL, NULL, TO_ROOM );

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = (level + 12) * multiplier;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char( victim, &af );
    send_to_char( "You fade out of existence.\n\r", victim );
    return TRUE;
}

bool spell_locate_object( int sn, int level, CHAR_DATA *ch, void *vo,int target){
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = IS_IMMORTAL(ch) ? 200 : 2 * level;

    buffer = new_buf();

    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( !can_see_obj( ch, obj ) || !is_name( target_name, obj->name )
        ||   IS_OBJ_STAT(obj,ITEM_NOLOCATE) || number_percent() > 2 * level
        ||   ch->level < obj->level)
            continue;

        found = TRUE;
        number++;

        for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj )
            ;

        if ( in_obj->carried_by != NULL && can_see(ch,in_obj->carried_by))
        {
            sprintf( buf, "%s is carried by %s\n\r",
                obj->short_descr,
                PERS(in_obj->carried_by, ch) );
        }
        else
        {
            if (IS_IMMORTAL(ch) && in_obj->in_room != NULL)
                sprintf( buf, "%s is in %s [Room %d]\n\r",
                    obj->short_descr,
                    in_obj->in_room->name, in_obj->in_room->vnum);
            else
                sprintf( buf, "%s is in %s\n\r",
                    obj->short_descr,
                    in_obj->in_room == NULL
                        ? "somewhere" : in_obj->in_room->name );
        }

        buf[0] = UPPER(buf[0]);
        add_buf(buffer,buf);

        if (number >= max_found)
            break;
    }

    if ( !found )
        send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
    else
        page_to_char(buf_string(buffer),ch);

    free_buf(buffer);

    return TRUE;
}

bool spell_pass_door( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( IS_AFFECTED(victim, AFF_PASS_DOOR) )
    {
        if (victim == ch)
          send_to_char("You are already out of phase.\n\r",ch);
        else
          act("$N is already shifted out of phase.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_fuzzy( level / 4 );
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_PASS_DOOR;
    affect_to_char( victim, &af );
    act( "$n turns translucent.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "You turn translucent.\n\r", victim );
    return TRUE;
}

bool spell_protective_shield(int sn,int level,CHAR_DATA *ch,void *vo,
        int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    if ( is_affected(victim, gsn_protective_shield))
    {
        send_to_char(
            "You are already surrounded by a protective shield.\n\r",ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/8;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You are surrounded by a protective shield.\n\r",
        victim);
    return TRUE;
}

bool spell_remove_curse( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    /* do object cases first */
    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *) vo;

        if (obj->wear_flags & ITEM_WEAR_SIGIL)
        {
            send_to_char("You cannot affect a sigil with that.\n\r", 0);
            bug ("Sigil targetted by remove curse.", 0);
        }

        if (IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_NOREMOVE))
        {
	    int dispellevel = obj->level;
	    if (obj_is_affected(obj, gsn_cursebarkja) && !room_is_affected(ch->in_room, gsn_sanctify))
                dispellevel = 70; 

            if (!IS_OBJ_STAT(obj,ITEM_NOUNCURSE)
// brazen: remove curse shouldn't affect items > caster level
	    && obj->level <= obj->level
            &&  !saves_dispel(level + 2,dispellevel,0))
            {
                REMOVE_BIT(obj->extra_flags[0],ITEM_NODROP);
                REMOVE_BIT(obj->extra_flags[0],ITEM_NOREMOVE);
		
		act("Dark mist seeps out of $p as its curse is removed.", ch, obj, NULL, TO_ALL);
            }
	    else
		act("You fail to remove the curse on $p.", ch, obj, NULL, TO_CHAR);

        }
	else
            act("There doesn't seem to be a curse on $p.",ch,obj,NULL,TO_CHAR);

        return TRUE;
    }

    /* characters */
    victim = (CHAR_DATA *) vo;

    send_to_char("A soft white glow surrounds you.\n\r", victim);
    act("A soft white glow surrounds $n.", victim, NULL, NULL, TO_ROOM);

    if (check_dispel(level,victim,gsn_curse))
    {
        act("$n looks more relaxed.",victim,NULL,NULL,TO_ROOM);
	return TRUE;
    }

    for (obj = victim->carrying; obj; obj = obj->next_content)
    {
        if ((IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_NOREMOVE))
        &&  !IS_OBJ_STAT(obj,ITEM_NOUNCURSE) && !IS_SET(obj->wear_flags, ITEM_WEAR_SIGIL)
	&&  !IS_SET(obj->wear_flags, ITEM_PROG)
// brazen: remove curse shouldn't affect items > caster level. Healer mobs
// should be level 60 to remove these curses.
        &&  obj->level <= ch->level
	&&  !saves_dispel(level, obj->level, 0))
        {
            REMOVE_BIT(obj->extra_flags[0],ITEM_NODROP);
            REMOVE_BIT(obj->extra_flags[0],ITEM_NOREMOVE);
	    act("Dark mist seeps out of your $p as its curse is removed.", victim, obj, NULL, TO_CHAR);
	    act("Dark mist seeps out of $p as its curse is removed.", victim, obj, NULL, TO_ROOM);
	    return TRUE;
        }
    }

    return TRUE;
}

bool spell_shield( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You are already shielded from harm.\n\r",ch);
        else
          act("$N is already protected by a shield.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = (8 + level) * (check_durablemagics(*ch) ? 2 : 1);
    af.location  = APPLY_AC;
    af.modifier  = -20 - (level / 2);
    af.bitvector = 0;
    affect_to_char( victim, &af );
    act( "$n is surrounded by a force shield.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "You are surrounded by a force shield.\n\r", victim );
    return TRUE;
}

bool spell_teleport( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = ch;
    ROOM_INDEX_DATA *pRoomIndex;

     if ( victim->in_room == NULL
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NOGATE)
    || is_affected(victim, gsn_matrix)
    || ( victim->in_room->area->area_flags & AREA_UNCOMPLETE )
    || ( victim != ch && IS_SET(victim->imm_flags,IMM_SUMMON))
    || (victim->move == 0))
    {
        send_to_char( "You cannot teleport from this place.\n\r", ch );
	return FALSE;
    }

    if (IS_AFFECTED(victim, AFF_CURSE))
    {
        send_to_char("A dark power binds you, preventing your escape.\n", ch);
        return false;
    }
    
    if(!IS_NPC(ch) && victim->fighting != NULL)
    {
        send_to_char("You cannot teleport while fighting.\n\r",ch);
	return FALSE;
    }

    pRoomIndex = get_random_room(victim);

    victim->move /= 2;

    act( "$n vanishes!", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, pRoomIndex );
    act( "$n slowly fades into existence.", victim, NULL, NULL, TO_ROOM );
    do_look( victim, "auto" );

    if (victim->familiar != NULL && victim->familiar->in_room == victim->in_room)
    {
        act( "$n vanishes!", victim->familiar, NULL, NULL, TO_ROOM );
        char_from_room( victim->familiar );
        char_to_room( victim->familiar, pRoomIndex );
        act( "$n slowly fades into existence.", victim->familiar, NULL, NULL, TO_ROOM );
        do_look( victim->familiar, "auto" );
    }

    return TRUE;
}

bool spell_word_of_recall( int sn, int level, CHAR_DATA *ch,void *vo,int target){
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (IS_NPC(victim))
      return FALSE;

    if (!is_same_group(ch, victim))
    {
        send_to_char("You cannot return them to their hometown.\n\r", ch);
        return FALSE;
    }

    if (is_affected(victim, sn))
    {
        if (ch == victim) send_to_char("You are already under the power of a Word.\n", ch);
        else act("$N is already under the power of a Word.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    if (victim->recall_to == NULL)
    {
        send_to_char("You have no location to recall to.\n\r", victim);
        return FALSE;
    }

    if (victim->in_room)
    {

        if (is_affected(victim, gsn_matrix))
        {
            if (check_spirit_of_freedom(victim))
                send_to_char("You feel the spirit of freedom surge within you, allowing you escape from the matrix.\n\r", victim);
            else
            {
                send_to_char("You are trapped by the powers of the matrix.\n\r", victim);
                    return TRUE;
            }
        }

        if (IS_SET(victim->in_room->room_flags,ROOM_NO_RECALL)
        || IS_AFFECTED(victim,AFF_CURSE)
        || (area_is_affected(victim->in_room->area, gsn_suppress)
        && !is_affected(victim, gsn_spirit_of_freedom))
        || (silver_state == 16))
        {
            send_to_char("The gods have forsaken you.\n\r",victim);
            return TRUE;
        }
    }

    struct CallbackHandler
    {
        static bool HandleMoveFrom(CHAR_DATA * ch, ROOM_INDEX_DATA *, EchoAffect *, void * tag)
        {
            int pulsesLeft(reinterpret_cast<int>(tag));
            if (pulsesLeft <= 0)
                return false;
        
            send_to_char("As you move you feel the power of the Word fade.\n", ch);
            affect_strip(ch, gsn_word_of_recall);
            return true;
        }

        static bool HandlePositionChange(CHAR_DATA * ch, int newPos, EchoAffect *, void *)
        {
            if (newPos == POS_FIGHTING)
                send_to_char("You feel the power of the Word slow in response to the adrenaline of combat.\n", ch);
            else if (ch->position == POS_FIGHTING)
                send_to_char("You feel the power of the Word accelerate as your heartrate slows.\n", ch);

            return false;
        }

        static bool HandlePulse(CHAR_DATA * ch, EchoAffect * echoAff, void * tag)
        {
            int pulsesLeft(reinterpret_cast<int>(tag));
            if (pulsesLeft <= 0)
            {
                // No pulses left, let the word fire
                if (ch->fighting)
                    stop_fighting_all(ch);

                send_to_char("As the power of the Word takes over, your surroundings seem to grow hazy, then vanish completely!\n", ch);
                uncamo_char(ch);
                ch->move /= 2;
                act("$n disappears.", ch, NULL, NULL, TO_ROOM);

                if (ch->familiar != NULL && ch->in_room == ch->familiar->in_room)
                {
                    act("$n disappears.", ch->familiar,NULL,NULL,TO_ROOM);
                    char_from_room(ch->familiar);
                    char_to_room(ch->familiar, ch->recall_to);
                    act("$n appears in the room.", ch->familiar, NULL, NULL, TO_ROOM);
                }
                
                char_from_room(ch);
                char_to_room(ch, ch->recall_to);
                act("$n appears in the room.", ch, NULL, NULL, TO_ROOM);
                do_look(ch, "auto");

                affect_strip(ch, gsn_word_of_recall);
                return true;
            }

            // Decrement pulse count; chance of failing this if fighting
            if (ch->position != POS_FIGHTING || number_bits(1) == 0)
                echoAff->SetTag(reinterpret_cast<void*>(--pulsesLeft));

            // Add another pulse
            echoAff->AddLine(&CallbackHandler::HandlePulse, "");
            return false;
        }
    };

    send_to_char("You feel the power of the Word begin to spread through your body.\n", victim);
    if (ch != victim)
        act("You sense the Word take hold, its power slowly spreading over $N.", ch, NULL, victim, TO_CHAR);

    // Apply the effects
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetMoveCallback(&CallbackHandler::HandleMoveFrom);
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    echoAff->SetTag(reinterpret_cast<void*>(number_range(1, 4)));
    echoAff->AddLine(&CallbackHandler::HandlePulse, "");
    EchoAffect::ApplyToChar(victim, echoAff);

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = -1;
    af.location = APPLY_HIDE;
    affect_to_char(victim, &af);
    return true;
}

