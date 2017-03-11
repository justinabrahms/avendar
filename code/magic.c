/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@efn.org)				   *
*	    Gabrielle Taylor						   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cmath>
#include <vector>
#include <sstream>
#include "merc.h"
#include "magic.h"
#include "lookup.h"
#include "recycle.h"
#include "interp.h"
#include "olc.h"
#include "tables.h"
#include "languages.h"
#include "PyreInfo.h"
#include "spells_fire.h"
#include "spells_fire_air.h"
#include "spells_air.h"
#include "spells_earth.h"
#include "spells_spirit.h"
#include "spells_spirit_air.h"
#include "spells_void.h"
#include "ShadeControl.h"
#include "RoomPath.h"
#include "Weave.h"
#include "Luck.h"
#include "NameMaps.h"
#include "Runes.h"
#include "EchoAffect.h"
#include "Drakes.h"
#include "Forge.h"
#include "Encumbrance.h"

/* command procedures needed */
DECLARE_DO_FUN(do_look		);
DECLARE_DO_FUN(do_autoyell      );

/*
 * Local functions.
 */
void	say_spell	args( ( CHAR_DATA *ch, int sn ) );
void 	cast_mana	args( (CHAR_DATA *ch, int mana, CHAR_DATA *conduit, int wait) );

/* imported functions */
CHAR_DATA *get_conduit  args( (CHAR_DATA *ch, ROOM_INDEX_DATA *room, int distance) );
extern int get_moon_state args( ( int moon_num, bool get_size ) );
extern bool describe_spell args( (char * spell, char * element, char * strength, int level) );
extern bool protect_works;






/*
 * Lookup a skill by name.
 */
int skill_lookup( const char *name )
{
    int sn;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
	if ( skill_table[sn].name == NULL )
	    break;
	if ( LOWER(name[0]) == LOWER(skill_table[sn].name[0])
	&&   !str_prefix( name, skill_table[sn].name ) )
	    return sn;
    }
    return -1;
}

int skill_lookup_full( const char *name )
{
    int sn;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
	if ( skill_table[sn].name == NULL )
	    break;
	if ( LOWER(name[0]) == LOWER(skill_table[sn].name[0])
	&&   !str_cmp( name, skill_table[sn].name ) )
	    return sn;
    }
    return -1;
}

int material_lookup( const char *name )
{
    int num;

    for ( num = 0; num < MAX_MATERIALS; num++ )
    {
	if ( material_table[num].name == NULL )
		break;
	if ( LOWER(name[0]) == LOWER(material_table[num].name[0])
	&&  !str_prefix( name, material_table[num].name) )
		return num;
    }

    /* return -1; */
    return 0;   /* unknown */
}

int find_spell(CHAR_DATA *ch, const char *name, SPELL_FUN funType)
{
    /* finds a spell the character can cast if possible */
    int sn, found = -1;

    if (IS_NPC(ch) && find_bilocated_body(ch) == NULL)
        return skill_lookup(name);

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if (skill_table[sn].name == NULL)
            break;

        // Filter out class mismatches
        if (funType == NULL)
        {
            if (skill_table[sn].spell_fun == spell_null
            ||  skill_table[sn].spell_fun == spell_song
            ||  skill_table[sn].spell_fun == spell_form
            ||  skill_table[sn].spell_fun == spell_focus)
                continue;
        }
        else if (skill_table[sn].spell_fun != funType)
            continue;

    	if (LOWER(name[0]) == LOWER(skill_table[sn].name[0])
    	&&  !str_prefix(name,skill_table[sn].name))
	    {
	        if ( found == -1)
        		found = sn;
	        if (get_skill(ch, sn) > 0)
          		return sn;
	    }
    }

    return found;
}

/*
 * Lookup a skill by slot number.
 * Used for object loading.
 */
int slot_lookup( int slot )
{
    extern bool fBootDb;
    int sn;

    if ( slot <= 0 )
	return -1;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
	if ( slot == skill_table[sn].slot )
	    return sn;
    }

    if ( fBootDb )
    {
	bug( "Slot_lookup: bad slot %d.", slot );
	abort( );
    }

    return -1;
}



/*
 * Utter mystical words for an sn.
 */
void say_spell( CHAR_DATA *ch, int sn )
{
    char buf  [MAX_STRING_LENGTH];
    char buf2 [MAX_STRING_LENGTH];
    CHAR_DATA *rch;
    char *pName;
    int iSyl;
    int length;


    struct syl_type
    {
	char *	old;
	char *	new_syl;
    };

    static const struct syl_type syl_table[] =
    {
	{ " ",		" "		},
	{ "ar",		"abra"		},
	{ "au",		"kada"		},
	{ "bless",	"fido"		},
	{ "blind",	"nose"		},
	{ "bur",	"mosa"		},
	{ "cu",		"judi"		},
	{ "de",		"oculo"		},
	{ "en",		"unso"		},
	{ "light",	"dies"		},
	{ "lo",		"hi"		},
	{ "mor",	"zak"		},
	{ "move",	"sido"		},
	{ "ness",	"lacri"		},
	{ "ning",	"illa"		},
	{ "per",	"duda"		},
	{ "ra",		"gru"		},
	{ "fresh",	"ima"		},
	{ "re",		"candus"	},
	{ "son",	"sabru"		},
	{ "tect",	"infra"		},
	{ "tri",	"cula"		},
	{ "ven",	"nofo"		},
	{ "a", "a" }, { "b", "b" }, { "c", "q" }, { "d", "e" },
	{ "e", "z" }, { "f", "y" }, { "g", "o" }, { "h", "p" },
	{ "i", "u" }, { "j", "y" }, { "k", "t" }, { "l", "r" },
	{ "m", "w" }, { "n", "i" }, { "o", "a" }, { "p", "s" },
	{ "q", "d" }, { "r", "f" }, { "s", "g" }, { "t", "h" },
	{ "u", "j" }, { "v", "z" }, { "w", "x" }, { "x", "n" },
	{ "y", "l" }, { "z", "k" },
	{ "", "" }
    };

    bool is_spirit((IS_NPC(ch) && IS_SET(ch->nact, ACT_SHADE)) || is_affected(ch, gsn_astralprojection));
    if (get_skill(ch, *lang_data[LANG_ARCANE].sn) >= 100
     && (IS_NPC(ch) || !BIT_GET(ch->pcdata->traits, TRAIT_OBSCURE)))
    {
        int prev = ch->speaking;
	LANG_STRING* arcane;

	ch->speaking = LANG_ARCANE;
	arcane = translate_spoken_new(ch, skill_table[sn].name);
	arcane->orig_string = skill_table[sn].name;


	for (rch = ch->in_room->people; rch; rch = rch->next_in_room)
    {
	    if (ch != rch && rch->position >= POS_RESTING && (!is_spirit || can_see(rch, ch)))
	    {
	    	act("$n utters the words '$t'", ch, translate_out_new(rch, arcane), rch, TO_VICT);
		    check_improve(rch, NULL, *lang_data[LANG_ARCANE].sn, TRUE, 5);
	    }
    }

	ch->speaking = prev;
	free(arcane);
	return;
    }

    buf[0]	= '\0';
    for ( pName = skill_table[sn].name; *pName != '\0'; pName += length )
    {
        for ( iSyl = 0; (length = strlen(syl_table[iSyl].old)) != 0; iSyl++ )
        {
            if ( !str_prefix( syl_table[iSyl].old, pName ) )
            {
                strcat( buf, syl_table[iSyl].new_syl);
                break;
            }
        }

        if ( length == 0 )
            length = 1;
    }

    sprintf( buf2, "$n utters the words, '%s'.", buf );
    sprintf( buf,  "$n utters the words, '%s'.", skill_table[sn].name );

    if (!(IS_NPC(ch) && (ch->desc == NULL)))
    {
        for ( rch = ch->in_room->people; rch; rch = rch->next_in_room )
        {
            if (rch != ch && rch->position >= POS_RESTING && (!is_spirit || can_see(rch, ch)))
                act((!IS_NPC(rch) && ch->class_num==rch->class_num) ? buf : buf2, ch, NULL, rch, TO_VICT );
        }
    }
}

// Compute a saving throw; negative applys make saving throw better
bool saves_spell(int level, CHAR_DATA * ch, CHAR_DATA *victim, int dam_type)
{
    // Cancel the effects of a supplied ch if it is the same as the victim
    if (ch == victim)
        ch = NULL;

    // Check luck
    switch (ch == NULL ? Luck::Check(*victim) : Luck::CheckOpposed(*victim, *ch))
    {
        case Luck::Lucky: return true;
        case Luck::Unlucky: return false;
        default: break;
    }
    
    int save;
    AFFECT_DATA *paf;
    bool saved;

    if (IS_OAFFECTED(victim, AFF_DARKFUTURE))
    {
		AFFECT_DATA *future;

		if ((future = affect_find(victim->affected, gsn_darkfuture)) != NULL)
		{
		    future->modifier -= 1;
	    	if (future->modifier <= 0)
				affect_remove(victim, future);
		}

		return FALSE;
    }
    
    save = 45;
    save += 2 * (victim->level - level);
    save -= get_save(victim);

    // Check for flight
    if (dam_type == DAM_BASH && is_flying(victim))
    {
        if (number_percent() <= get_skill(victim, gsn_controlledflight))
            check_improve(victim, ch, gsn_controlledflight, true, 2);
        else
        {
            check_improve(victim, ch, gsn_controlledflight, false, 2);
            save -= 10;
        }
    }

    // Check for Bayyal's Realm
    if (dam_type == DAM_FIRE)
    {
        std::vector<std::pair<CHAR_DATA*, PyreInfo*> > pyreData(getPyreInfoEffectsArea(victim, PyreInfo::Effect_BayyalsRealm));
        bool any(false);
        for (size_t i(0); i < pyreData.size(); ++i)
        {
            if (pyreData[i].second->isEffectGreater())
            {
                any = true;
                --save;
            }
        }
        if (any)
            save -= 3;
    }

    // Check for masquerade
    if (dam_type == DAM_FEAR && victim->in_room != NULL)
    {
        AFFECT_DATA * masquerade(get_area_affect(victim->in_room->area, gsn_masquerade));
        if (masquerade != NULL && masquerade->modifier != victim->id)
        {
            if (number_percent() <= get_skill(victim, gsn_disillusionment))
                check_improve(victim, NULL, gsn_disillusionment, true, 12);
            else
            {
                check_improve(victim, NULL, gsn_disillusionment, false, 12);
                save -= (masquerade->level / 5);
            }
        }
    }
    
    // Check for Ionize
    if (dam_type == DAM_LIGHTNING)
    {
        AFFECT_DATA * ionize(get_affect(victim, gsn_ionize));
        if (ionize != NULL)
        {
            // Shockcraft can prevent this
            if (number_percent() <= get_skill(victim, gsn_shockcraft))
                check_improve(victim, NULL, gsn_shockcraft, true, 8);
            else
            {
                check_improve(victim, NULL, gsn_shockcraft, false, 8);
                save -= (ionize->level / 5);
            }
        }
    }

    // Check for modifications from the caster
    if (ch != NULL)
    {
        // Check for adamantine invocation
        AFFECT_DATA * adamantine(get_affect(ch, gsn_adamantineinvocation));
        if (adamantine != NULL && adamantine->modifier > 0)
        {
            save -= adamantine->modifier;
            adamantine->modifier = 0;
        }

        // Check for runes of adamantine
        unsigned int runeCount(Runes::InvokedCount(*ch, Rune::Adamantine));
        if (runeCount > 0)
        {
            save -= runeCount * 2;
            save -= (Runes::BonusCount(*ch) * 6);
        }

        // Check for endless facade
        if (dam_type == DAM_ILLUSION)
        {
            if (number_percent() <= get_skill(ch, gsn_endlessfacade))
            {
                check_improve(ch, victim, gsn_endlessfacade, true, 2);
                save -= 5;
            }
            else
                check_improve(ch, victim, gsn_endlessfacade, false, 2);
        }

        // Check for blood of the vizier
        save -= check_bloodofthevizier_count(*ch, *victim);

        // Check for unholy might
        AFFECT_DATA * unholymight(get_affect(ch, gsn_unholymight));
        if (unholymight != NULL)
            save -= UMAX(0, unholymight->modifier);

        // Check for revenant
        if (dam_type == DAM_FEAR && number_percent() <= get_skill(ch, gsn_revenant))
            save -= 5;
    }

    if (!IS_NPC(victim) && dam_type == DAM_NEGATIVE
	&& BIT_GET(victim->pcdata->traits, TRAIT_MARKED)
	&& victim->religion == god_lookup("Calaera"))
		save += 10;
		
	if (IS_AFFECTED(victim,AFF_BERSERK))
		save += victim->level/2;
   
    if (victim->in_room != NULL)
    {
        if (room_is_affected(victim->in_room, gsn_sanctify) && IS_GOOD(victim))
            save += 10;
        else if (area_is_affected(victim->in_room->area, gsn_holyground) && IS_GOOD(victim))
            save += 5;
    }

    save += get_resist(victim, TYPE_HIT + dam_type);

	// TRAIT_RESISTANT adds 12 saves
	if (!IS_NPC(victim) && BIT_GET(victim->pcdata->traits, TRAIT_RESISTANT))
		save += 12;
	
    save = URANGE( 5, save, 95 );
    saved = (number_percent( ) < save) ? TRUE : FALSE;

    if (!saved && (paf = affect_find(victim->affected,gsn_compact_of_Logor)) != NULL)
    {
        if (number_percent() < paf->modifier)
          saved = (number_percent() < save) ? TRUE : FALSE;
    }

    return saved;
}

/// RT save for dispels
bool saves_dispel( int dis_level, int spell_level, int duration)
{
    int save;
    
    if (duration == -1)
      spell_level += 5;  
      /* very hard to dispel permanent effects */

    save = 50 + (spell_level - dis_level) * 5;
    save = URANGE( 5, save, 95 );
    return number_percent( ) < save;
}

/* co-routine for dispel magic and cancellation */

bool check_dispel( int dis_level, CHAR_DATA *victim, int sn)
{
    for (AFFECT_DATA * paf(get_affect(victim, sn)); paf != NULL; paf = get_affect(victim, sn, paf))
    {
        if (saves_dispel(dis_level, paf->level, paf->duration))
        {
            --paf->level;
            continue;
        }
        
        affect_strip(victim, sn);
        if (skill_table[sn].msg_off)
        {
            send_to_char(skill_table[sn].msg_off, victim);
            send_to_char("\n", victim);
        }
        return true;
    }

    return false;
}

/* for finding mana costs -- temporary version */
int mana_cost (CHAR_DATA *ch, int min_mana, int level)
{
    if (ch->level + 2 == level)
	return 1000;
    return UMAX(min_mana,(100/(2 + ch->level - level)));
}

void cast_mana(CHAR_DATA *ch, int mana, CHAR_DATA *conduit, int wait)
{
    int cmana = 0;

    if (conduit)
    {
	cmana = mana / 2;
	mana = (mana + 1) / 2;
    }

    WAIT_STATE( ch, wait );

    if (conduit)
    {
	if (conduit->mana >= cmana)
	    expend_mana(conduit, cmana);
	else
	    expend_mana(ch, cmana);

	if (ch->mana >= mana)
	    expend_mana(ch, mana);
	else
	    expend_mana(conduit, mana);
    }
    else
        expend_mana(ch, mana);

    if (is_affected(ch, gsn_manabarbs))
    {
	damage_old(ch, ch, mana, gsn_manabarbs, DAM_MENTAL, TRUE);
	send_to_char ("You wince as the manabarbs bite into your mind.\n\r", ch);
	act("$n winces as they cast a spell.",ch,NULL,NULL, TO_ROOM);
    }
};

void do_magic_detect(CHAR_DATA * ch, int sn)
{
	DESCRIPTOR_DATA *d;
	char element[MAX_STRING_LENGTH];
	char strength[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch) || ch->in_room == NULL) 
        return;

	element[0] = '\0';
	strength[0] = '\0';
	
	//For the new and improved detect magic users
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->connected == CON_PLAYING && d->character->in_room != NULL && d->character != ch && d->character->in_room->area == ch->in_room->area)
		{
            bool show(false);

            // Show if detect magic or weavesense is up
            if (is_affected(d->character, gsn_detect_magic) || is_an_avatar(d->character)) 
                show = true;

            if (number_percent() < get_modifier(d->character->affected, gsn_weavesense))
            {
                show = true;
                check_improve(d->character, NULL, gsn_weavesense, TRUE, 8);
            }

            if (show)
            {
			    if (describe_spell(skill_table[sn].name, element, strength, ch->level))
				    sprintf(buf, "You detect a %s spell of %s being cast in the area.\n\r", strength, element);
			    else
				    sprintf(buf, "You detect a %s spell being cast in the area.\n\r", strength);
			    send_to_char(buf, d->character);
            }
		}
	}
}

/*
 * The kludgy global is for spells who want more stuff from command line.
 */
char *target_name;

bool adjust_spell_parameters(CHAR_DATA * ch, int sn, int & level, int & beats, int & mana)
{
    // Dark insight modifier
    AFFECT_DATA * paf(get_affect(ch, gsn_dark_insight));
	if (paf != NULL)
	    level += paf->modifier;

    // Luck modifier
    switch (Luck::Check(*ch))
    {
        case Luck::Lucky: ++level; break;
        case Luck::Unlucky: --level; break;
        default: break;
    }

    // Fell purpose modifier
    if (is_affected(ch, gsn_fellpurpose))
    {
        mana *= 2;

        int skill(get_skill(ch, gsn_fellpurpose));
        if (number_percent() <= skill)
        {
            check_improve(ch, NULL, gsn_fellpurpose, true, 4);
            ++level;

            // Adjust for familiars
            if (is_familiar_present(*ch, gsn_calltoad, OBJ_VNUM_FAMILIAR_TOAD)) ++level;
            if (is_familiar_present(*ch, gsn_callcat, OBJ_VNUM_FAMILIAR_CAT)) beats = (beats * 9) / 10;
        }
        else
            check_improve(ch, NULL, gsn_fellpurpose, false, 4);
    }

    // Devour essence modifier
    paf = get_affect(ch, gsn_devouressence);
    if (paf != NULL)
        level += (paf->modifier / 50);

    // Pain channel modifier; +1 level per 20% hurt, rounded up
    if (number_percent() <= get_skill(ch, gsn_painchannel))
    {
        check_improve(ch, NULL, gsn_painchannel, true, 4);
        int bonus(5 - ((ch->hit * 5) / ch->max_hit));
        level += URANGE(0, bonus, 5);
    }
    else
        check_improve(ch, NULL, gsn_painchannel, false, 4);
	
    // Quintessence rush modifier
    if (is_quintessence_rushing(ch))
        mana += 15;

	// Check for modifiers which require a room
    if (ch->in_room != NULL)
    {
        // Arctic chill modifier
        paf = get_affect(ch, gsn_arcticchill);
        if (paf != NULL && paf->modifier == 0)
        {
            // Check for temperature to adjust mana cost; the warmer, the costlier
            int mod(ch->in_room->area->w_cur.temperature + 20);
            mana += URANGE(5, mod, 45);
        }

        // Wintertide modifier; every 8 degrees below 0 is worth a level
        if (number_percent() <= get_skill(ch, gsn_wintertide))
        {
            int wintertideMod(ch->in_room->area->w_cur.temperature / -8);
            level += URANGE(0, wintertideMod, 5);
        }

        // Storm mastery modifier
        if (IS_OUTSIDE(ch))
        {
            int stormPower(0);
            if (ch->in_room->area->w_cur.storm_str >= 35) ++stormPower;
            if (ch->in_room->area->w_cur.storm_str >= 65) ++stormPower;
            if (ch->in_room->area->w_cur.storm_str >= 90) ++stormPower;
            if (ch->in_room->area->w_cur.lightning_str >= 35) ++stormPower;
            if (ch->in_room->area->w_cur.lightning_str >= 70) ++stormPower;
            if (area_is_affected(ch->in_room->area, gsn_brume)) ++stormPower;
            if (area_is_affected(ch->in_room->area, gsn_icestorm)) ++stormPower;
            if (area_is_affected(ch->in_room->area, gsn_lightning_storm)) ++stormPower;

            if (stormPower > 0)
            {
                if (number_percent() < get_skill(ch, gsn_stormmastery))
                {
                    // Determine bonuses
                    int bonus(2 + (stormPower >= 3 ? 1 : 0) + (stormPower >= 5 ? 1 : 0));
                    level += bonus;
                    mana = (mana * (10 - bonus)) / 10;
                    check_improve(ch, NULL, gsn_stormmastery, true, 2);
                    send_to_char("You draw power from the storm, infusing it into your spell!\n", ch);
                }
                else
                    check_improve(ch, NULL, gsn_stormmastery, false, 2);
            }
        }
 
        // Check for holy ground
        if (is_skill_sphere_any(sn, SPH_VOID) && area_is_affected(ch->in_room->area, gsn_holyground))
            mana *= 2;

        // Power nexus modifier
    	if (ch->in_room->room_flags & ROOM_POWER_NEXUS)
	        level += (1 + ch->level / 12);
    
        // Get the weave modifiers
        int positivePower(Weave::AbsolutePositivePower(*ch->in_room));
        int orderPower(Weave::AbsoluteOrderPower(*ch->in_room));
        int boost(0);

        // Calculate the base boost from the spheres
        for (unsigned int i(0); i < MAX_SKILL_SPHERE; ++i)
        {
            switch (skill_table[sn].spheres[i])
            {
                case SPH_NONE:      break;
                case SPH_WATER:     boost = positivePower + orderPower;     break;
                case SPH_EARTH:     boost = 2 * orderPower;                 break;
                case SPH_VOID:      boost = -positivePower + orderPower;    break;
                case SPH_SPIRIT:    boost = positivePower - orderPower;     break;
                case SPH_AIR:       boost = -2 * orderPower;                break;
                case SPH_FIRE:      boost = -positivePower - orderPower;    break;
                default:            boost = (abs(positivePower) + abs(orderPower)) / 3; break;
            }
        }

        // Check for weavecraft to cancel any negative boost
        if (boost < 0)
        {
            if (number_percent() <= get_skill(ch, gsn_weavecraft))
            {
                check_improve(ch, NULL, gsn_weavecraft, true, 2);
                boost = 0;
            }
            else
                check_improve(ch, NULL, gsn_weavecraft, false, 2);
        }
        
        // Check for weavetap; requires the presence of a line/fount and a non-negative boost
        if (Weave::HasWeave(*ch->in_room) && boost >= 0)
        {
            int skill(get_skill(ch, gsn_weavetap));
            if (number_percent() <= skill)
            {
                // Successfully tapped; determine level of boost
                check_improve(ch, NULL, gsn_weavetap, true, 2);

                int manaReduction;
                int lagReduction;
                if (boost < 2 || number_percent() > (skill / 2))
                {
                    // Basic tap; applies if power is less than 2 or fails the second skill check
                    manaReduction = 20;
                    lagReduction = 10;
                    send_to_char("You tap into the Weave, drawing its power into your spell.\n", ch);
                }
                else if (boost < 4 || number_percent() > (skill / 2))
                {
                    // Heavy tap; applies if power is less than 4 or fails the third skill check
                    manaReduction = 35;
                    lagReduction = 20;
                    ++boost;
                    send_to_char("You tap heavily into the Weave, redirecting its energy to empower your magic!\n", ch);
                }
                else
                {
                    // Super deep tap; can only reach this with a boost of 4+ and passing all three skill checks; at 100% skill this has 25% odds
                    manaReduction = 50;
                    lagReduction = 30;
                    boost += 2;
                    send_to_char("Your whole being tingles as you immerse your magic in the Weave, drawing deeply of its power!\n", ch); 
                }

                // Apply reductions
                beats = (beats * (100 - lagReduction)) / 100;
                mana = (mana * (100 - manaReduction)) / 100;
            }
            else
                check_improve(ch, NULL, gsn_weavetap, false, 2);
        }

        // Adjust the level of the spell
        level += boost;
    }

    // Check for thermal mastery
    if ((is_skill_sphere_any(sn, SPH_FIRE) || is_skill_sphere_any(sn, SPH_WATER)) && number_percent() < get_skill(ch, gsn_thermalmastery))
        ++level;

    // Check for casting-empowered weapons
    OBJ_DATA * weapon(get_eq_char(ch, WEAR_WIELD));
    if (weapon != NULL)
        level += Forge::TraitCount(*weapon, Forge::Trait_Casting);
    
    // Check for spirit-specific modifications
    if (is_skill_sphere_any(sn, SPH_SPIRIT))
    {
        // Check for bind essence headgear
        OBJ_DATA * bound(get_eq_char(ch, WEAR_HEAD));
        if (bound != NULL && obj_is_affected(bound, gsn_bindessence))
            ++level;
    
        // Check for rod of transcendence
        OBJ_DATA * rod(get_eq_char(ch, WEAR_WIELD));
        if (rod != NULL && rod->pIndexData->vnum == OBJ_VNUM_RODOFTRANSCENDENCE)
        {
            static const int chance_table[] = {10, 20, 30, 40, 50, 55, 60, 65, 69, 73, 76, 78, 80};
            static const int chance_table_count = sizeof(chance_table) / sizeof(chance_table[0]);

            // Calculate chance of reducing mana cost from the table (values are tenths of a percent); each extra boost beyond the table gives +1 more
            int chance;
            int boostCount(count_obj_affects(rod, gsn_bindessence));
            if (boostCount < chance_table_count) chance = chance_table[boostCount];
            else chance = chance_table[chance_table_count - 1] + boostCount - chance_table_count;

            // Check whether mana cost is reduced; capped at 25%
            if (number_range(0, 1000) < UMIN(250, chance))
            {
                act("$p gleams softly as it channels your magic efficiently.", ch, rod, NULL, TO_CHAR);
                mana /= 3;
            }
        }
    }

    // Check for void-specific modifications
    if (is_skill_sphere_any(sn, SPH_VOID))
    {
        // Check bonereaper
        check_bonereaper_mana(*ch, mana);

        // Check for Cowl of Night
        if (ch->in_room != NULL && number_percent() <= get_skill(ch, gsn_cowlofnight) && room_is_dark(ch->in_room))
        {
            check_improve(ch, NULL, gsn_cowlofnight, true, 2);
            int mod(aura_grade(ch));
            level += URANGE(1, mod, 3);
        }
    }

    // Check for desecration
    if (ch->in_room != NULL && room_is_affected(ch->in_room, gsn_desecration))
    {
        if (is_skill_sphere_any(sn, SPH_VOID))
        {
            send_to_char("You feel the unholy power of this place infuse your spell with dark energy!\n", ch);
            level += 2;
        }
        else
        {
            int failOdds(is_skill_sphere_any(sn, SPH_WATER) ? 40 : 20);
            if (number_percent() <= failOdds)
            {
                send_to_char("Your spell fizzles, snuffed out by the unholy powers in this place.\n", ch);
                return false;
            }
        }
    }

    // Check for water-specific modifications
    if (is_skill_sphere_any(sn, SPH_WATER))
    {
        if (ch->in_room != NULL)
        {
            // Check for waveborne
            int waveborneLagReduce(0);
            switch (ch->in_room->sector_type)
            {
                case SECT_UNDERWATER: waveborneLagReduce = 75; break;
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM: waveborneLagReduce = 85; break;
            }
        
            if (waveborneLagReduce > 0)
            {
                if (number_percent() <= get_skill(ch, gsn_waveborne))
                {
                    beats = UMAX(1, ((beats * waveborneLagReduce) / 100));
                    check_improve(ch, NULL, gsn_waveborne, true, 8);
                }
                else
                    check_improve(ch, NULL, gsn_waveborne, false, 8);
            }
        }
    }
    else if (ch->in_room != NULL)
    {
        // Not water; check for winter's stronghold
        AFFECT_DATA * stronghold(get_room_affect(ch->in_room, gsn_wintersstronghold));
        if (stronghold != NULL && stronghold->modifier == 1 && number_percent() <= 20)
        {
            send_to_char("The chill power of the stronghold engulfs your spell, negating it completely!\n", ch);
            return false;    
        }
    }

    // Check for air-specific modifications
    if (is_skill_sphere_any(sn, SPH_AIR))
    {
        // Check for windrider
        if (is_flying(ch))
        {
            if (number_percent() <= get_skill(ch, gsn_windrider))
            {
                check_improve(ch, NULL, gsn_windrider, true, 4);
                std::pair<Direction::Value, int> windInfo(checkWind(ch));
                if (windInfo.second > 0)
                    level += 1 + (windInfo.second / 14);
            }
            else
                check_improve(ch, NULL, gsn_windrider, false, 4);
        }

        // Check for conduit of the skies
        if (is_affected(ch, gsn_conduitoftheskies))
        {
            ++level;
            beats = UMAX(1, (beats * 4) / 5);
        }
    }

    // Check for earth-specific modifications
    bool isStonehaven(ch->in_room != NULL && room_is_affected(ch->in_room, gsn_stonehaven));
    if (is_skill_sphere_any(sn, SPH_EARTH))
    {
        // Check for runes of order
        unsigned int runeCount(Runes::InvokedCount(*ch, Rune::Order));
        if (runeCount > 0)
        {
            if (runeCount >= 3)
            {
                // Each rune after the first two is worth 0.5 levels
                level += ((runeCount - 2) / 2);
                if ((runeCount % 2) == 1 && number_bits(1) == 0)
                    ++level;

                runeCount = 2;
            }

            // The first two runes are worth 1 level each
            level += runeCount;

            // Bonuses decrease the beat count by 1 each up to 6
            unsigned int bonusCount(Runes::BonusCount(*ch));
            beats -= UMIN(bonusCount, 6);
            beats = UMAX(1, beats);
        }

        // Handle stonehaven
        if (isStonehaven)
        {
            level += 2;
            mana = (mana * 4) / 5;
        }

        // Handle salt of the earth
        if (number_percent() <= determine_saltoftheearth_level(*ch, SPH_EARTH))
            beats = (beats * 4) / 5;

        // Check for geomancy
        if (ch->in_room != NULL)
        {
            int geomancyBonus(0);
            switch (ch->in_room->sector_type)
            {
                case SECT_MOUNTAIN:
                case SECT_HILLS:        geomancyBonus = 1; break;
                case SECT_UNDERGROUND:  geomancyBonus = 2; break;
            }

            if (geomancyBonus > 0 && number_percent() <= get_skill(ch, gsn_geomancy))
            {
                check_improve(ch, NULL, gsn_geomancy, true, 8);
                level += geomancyBonus;
            }
        }
    }
    else if (isStonehaven && number_bits(2) == 0)
    {
        send_to_char("Your spell fizzles, snuffed out by the earthen power here.\n", ch);
        return false;
    }

    // Check for fire-specific modifications
    if (is_skill_sphere_any(sn, SPH_FIRE))
    {
        // Check for firedancer
        AFFECT_DATA * firedancerAff = get_affect(ch, gsn_firedancer);
        if (firedancerAff != NULL && firedancerAff->duration < 0)
        {
            // Firedancer is present; add to mana cost (the modifier is the % increase in cost)
            mana = (mana * (100 + firedancerAff->modifier)) / 100;
            if (number_percent() < get_skill(ch, gsn_firedancer))
            {
                // Passed the skill check, so reduce the lag by ~20%
                if (beats > 0) beats = UMAX(1, ((beats * 4) / 5));
                check_improve(ch, NULL, gsn_firedancer, TRUE, 8);
            }
            else
                check_improve(ch, NULL, gsn_firedancer, FALSE, 8);
        }

        // Check for heart of the inferno
        for (OBJ_DATA * heart = ch->carrying; heart != NULL; heart = heart->next_content)
        {
            // Only count if actually worn
            if (!heart->worn_on)
                continue;

            // Look for the affect
            AFFECT_DATA * heartAff = get_obj_affect(heart, gsn_heartoftheinferno);
            while (heartAff != NULL && heartAff->location != APPLY_NONE)
                heartAff = get_obj_affect(heart, gsn_heartoftheinferno, heartAff);

            // Did not find affect
            if (heartAff == NULL)
                continue;

            // Found affect; check for level modifications (every even level up through 10, inclusive)
            level += UMIN(5, (heartAff->modifier / 2));
            break;
        }

        // Check for geothermic-based mana reduction
        if (ch->in_room != NULL && number_percent() < get_skill(ch, gsn_geothermics))
        {
            switch (ch->in_room->sector_type)
            {
                case SECT_HILLS: 
                case SECT_MOUNTAIN:
                    mana = UMAX(1, (mana * 4) / 5);
                    check_improve(ch, NULL, gsn_geothermics, TRUE, 4);
                    break;

                case SECT_UNDERGROUND:
                    mana = UMAX(1, (mana * 2) / 3);
                    check_improve(ch, NULL, gsn_geothermics, TRUE, 2);
                    break;
            }
        }
    }
    else
    {
        // Not fire; handle Bayyal's Realm
        OBJ_DATA * pyre(lookupPyre(ch->in_room));
        if (pyre != NULL)
        {
            CHAR_DATA * pyreOwner(findCharForPyre(pyre));
            if (pyreOwner != NULL)
            {
                PyreInfo * pyreInfo(getPyreInfoEffect(pyreOwner, PyreInfo::Effect_BayyalsRealm));
                if (pyreInfo != NULL && pyreInfo->isEffectGreater())
                {
                    send_to_char("Your spell fizzles, snuffed out by the fiery powers of this place.\n", ch);
                    return false;
                }
            }
        }
    }
	
	return true;
}

// Assumes ch and victim are both non-null and in a room and the target is TARGET_CHAR
static bool adjust_for_targeted_spell(CHAR_DATA * ch, CHAR_DATA * victim, int sn, int level, int & mana)
{
    // Check for blur
	if (!is_same_group(victim, ch) && !IS_IMMORTAL(ch)
	&& ((is_affected(victim, gsn_blur) && number_bits(3) == 0)
	|| (room_is_affected(victim->in_room, gsn_smokescreen && number_bits(4) == 0))))
	{
	    if (!IS_NPC(ch) || !IS_AFFECTED(ch, AFF_WIZI))
	    {
	        act("You misjudge $S location, and your spell fizzles!", ch, NULL, victim, TO_CHAR);
	        act("$n misjudges your location, and $s spell fizzles!", ch, NULL, victim, TO_VICT);
	        act("$n misjudges $N's location, and $s spell fizzles!", ch, NULL, victim, TO_NOTVICT);
	    }
		
		return false;
	}

    // Check for drake sunderer trait
    if (number_percent() <= Drakes::SpecialCount(*victim, Drakes::Sunderer))
    {
        act("Your spell fizzles out against $N, unable to penetrate $S stony flesh.", ch, NULL, victim, TO_CHAR);
        act("$n's spell fizzles out against $N, unable to penetrate $S stony flesh.", ch, NULL, victim, TO_NOTVICT);
        act("$n's spell fizzles out against you, unable to penetrate your stony flesh.", ch, NULL, victim, TO_VICT);
        return false;
    }

    // Check for healer's touch
    for (AFFECT_DATA * paf(get_affect(victim, gsn_healerstouch)); paf != NULL; paf = get_affect(victim, gsn_healerstouch, paf))
    {
        if (paf->modifier == sn)
        {
            send_to_char("You feel the immunizing magic within you flare up, protecting you.\n", victim);
            act("$N appears to be immune to that, for now.", ch, NULL, victim, TO_CHAR);
            return false;
        }
    }

    // Check for somatic arts
    if (IS_SET(skill_table[sn].flags, SN_SOMATICBOOST) && !IS_NPC(ch))
    {
        if (number_percent() <= get_skill(ch, gsn_somaticarts))
        {
            int somaticSkill(0);
            if (ch->pcdata->somatic_arts_info != NULL)
            {
                somaticSkill = ch->pcdata->somatic_arts_info->SkillFor(victim->race);
                if (somaticSkill == SomaticArtsInfo::Unknown)
                    somaticSkill = 0;

                if (ch->pcdata->somatic_arts_info->CheckImproveRace(victim->race, ch->race == victim->race, false, get_curr_stat(ch, STAT_INT)))
                {
                    std::ostringstream mess;
                    mess << "Your understanding of " << race_table[victim->race].name << " physiology deepens!\n";
                    send_to_char(mess.str().c_str(), ch);
                }
 
            }
 
            check_improve(ch, victim, gsn_somaticarts, true, 12);
            mana = (mana * (95 - (somaticSkill / 25))) / 100;
       }
        else
            check_improve(ch, victim, gsn_somaticarts, false, 12);
    }

    return true;
}

// Assumes ch and victim are both non-null and in a room and the target is TARGET_CHAR
// This is for TAR_CHAR_OFFENSIVE or TAR_OBJ_CHAR_OFF
static bool adjust_for_offensive_spell(CHAR_DATA * ch, CHAR_DATA *& victim, int sn, int level)
{
    if (IS_NPC(victim) && (victim->pIndexData->vnum == MOB_VNUM_MIRROR_IMAGE))
	{
		act("$n turns translucent and disappears.", victim, NULL, NULL, TO_ROOM);
        extract_char(victim, TRUE);
		return false;
	}
		
	if (checkPyrokineticMirrorAttacked(ch, victim))
		return false;

	if (ShadeControl::CheckShadeAttacked(*ch, *victim))
		return false;

	// Check for countermagic; note that it is almost impossible to countermagic someone who knows countermagic
    // Subvocalize also makes it harder
    if (ch != victim && number_percent() <= ((get_skill(victim, gsn_countermagic) * 15) / 100) 
    && number_percent() > get_skill(ch, gsn_countermagic) && number_percent() > (get_skill(ch, gsn_subvocalize) / 2))
    {
        // Countermagic success; check whether it is a riposte or just a parry
        check_improve(victim, ch, gsn_countermagic, true, 1);
        if (number_bits(2) == 0)
        {
            // Riposte; set the target to be the original caster
            act("$N weaves a quick countersign from threads of energy, and your own magics turn against you!", ch, NULL, victim, TO_CHAR);
            act("You weave a quick countersign from threads of energy, turning $n's magics against $m!", ch, NULL, victim, TO_VICT);
            victim = ch;
        }
        else
        {
            // Parry; just bail out
            act("$N weaves a quick countersign from threads of energy, causing your magics to fizzle and fail.", ch, NULL, victim, TO_CHAR);
            act("You weave a quick countersign from threads of energy, causing $n's magics to fizzle and fail.", ch, NULL, victim, TO_VICT);
            return false;
        }
    }

    // Check for chaoscast
    if (ch != victim && victim->in_room != NULL && number_percent() <= ((get_skill(victim, gsn_chaoscast) * 15) / 100))
    {
        // Find a target other than ch or victim
        std::vector<CHAR_DATA*> candidates;
        for (CHAR_DATA * candidate(victim->in_room->people); candidate != NULL; candidate = candidate->next_in_room)
        {
            if (candidate != ch && !IS_AFFECTED(candidate, AFF_WIZI) && !IS_OAFFECTED(candidate, AFF_GHOST) 
            && (!IS_NPC(ch) || !IS_SET(ch->nact, ACT_SHADE)) && !is_same_group(candidate, victim))
                candidates.push_back(candidate);
        }
        
        if (!candidates.empty())
        {
            // Choose a target
            check_improve(victim, ch, gsn_chaoscast, true, 1);
            act("A flare of burning static energy leaps out from $N, redirecting your magics!", ch, NULL, victim, TO_CHAR);
            act("A flare of burning static energy leaps out from you, redirecting $n's magics!", ch, NULL, victim, TO_VICT);
            victim = candidates[number_range(0, candidates.size() - 1)];
        }
    }

	if (ch != victim)
	{
        // Check for avenging seraph; small chance of retribution if void spell cast
        if (is_skill_sphere_any(sn, SPH_VOID) && is_affected(victim, gsn_avengingseraph) && number_percent() <= 5)
        {
            act("As you cast, an arrow of burning light flies from the heavens to strike you!", ch, NULL, NULL, TO_CHAR);
            act("As $n casts, an arrow of burning light flies from the heavens to strike $m!", ch, NULL, NULL, TO_ROOM);
            sourcelessDamage(ch, "the arrow", dice(victim->level, 2), gsn_avengingseraph, DAM_LIGHT);
        }

        // Check for heatlash
		for (CHAR_DATA * gch(victim->in_room->people); gch != NULL; gch = gch->next_in_room)
		{
			// Applies if any groupmate has the effect up
			if (gch == victim || is_same_group(gch, victim))
			{
				PyreInfo * pyreInfo(getPyreInfoEffect(gch, PyreInfo::Effect_Heatlash));
				if (pyreInfo == NULL)
					continue;

				// Make sure the type is correct
				if ((is_skill_sphere_any(sn, SPH_EARTH) && pyreInfo->effectModifier() == PyreInfo::Heatlash_Modifier_Earth)
				||  (is_skill_sphere_any(sn, SPH_FIRE) && pyreInfo->effectModifier() == PyreInfo::Heatlash_Modifier_Fire))
				{
					// Check consume
					bool anyEffect = false;
					if (!is_affected(ch, gsn_consume) && !savesConsumeSpecial(gch, ch) && !saves_spell(gch->level, victim, ch, DAM_FIRE))
					{
						anyEffect = true;
						act("A flare of scalding heat flashes from you to $N, and $S skin begins to glow hot!", victim, NULL, ch, TO_CHAR);
						act("A flare of scalding heat flashes from $n to $N, and $S skin begins to glow hot!", victim, NULL, ch, TO_NOTVICT);
						act("A flare of scalding heat flashes from $n to you, and your skin begins to burn hot!", victim, NULL, ch, TO_VICT);

						AFFECT_DATA caf = {0};
						caf.where       = TO_AFFECTS;
						caf.type        = gsn_consume;
						caf.level       = gch->level;
						caf.duration    = caf.level / 5;
						affect_to_char(ch, &caf);
					}

					// Greater effect cause a burst of damage as well as the consume
					if (pyreInfo->isEffectGreater())
					{
						anyEffect = true;
						int damage = dice(2, 50);
						if (saves_spell(gch->level, victim, ch, DAM_FIRE))
							damage /= 2;

						damage_new(victim, ch, UMAX(1, damage), gsn_scorch, DAM_FIRE, true, "fiery flare");
					}

					if (!anyEffect)
					{
						act("A flare of scalding heat flashes from you to $N, but has no effect.", victim, NULL, ch, TO_CHAR);
						act("A flare of scalding heat flashes from $n to $N, but has no effect.", victim, NULL, ch, TO_NOTVICT);
						act("A flare of scalding heat flashes from $n to you, but has no effect.", victim, NULL, ch, TO_VICT);
					}
				}
			}
		}
	}
	
	// Check for crystallize magic
	if (IS_PAFFECTED(victim, AFF_CRYSTALIZE_MAGIC) && sn != gsn_soulblade)
	{
	    if (ch != victim)
	    {
			act("$n's magical force is absorbed by $N and channeled into a small crystal.", ch, NULL, victim, TO_NOTVICT);
			act("You absorb $n's magical force, channeling it down into a small crystal.", ch, NULL, victim, TO_VICT);
			act("$N absorbs your magical force, channeling it down into a small crystal.", ch, NULL, victim, TO_CHAR);
	    }
	    else
	    {
			act("$n's magical force is absorbed by $m and channeled into a small crystal.", ch, NULL, NULL, TO_ROOM);
			act("You absorb your magical force, channeling it down into a small crystal.", ch, NULL, NULL, TO_CHAR);
	    }
	
        char buf[MAX_STRING_LENGTH];	
	    REMOVE_BIT(victim->paffected_by, AFF_CRYSTALIZE_MAGIC);
	    OBJ_DATA * crystal = create_object(get_obj_index(OBJ_VNUM_MAGIC_CRYSTAL), ch->level);
	    sprintf(buf, crystal->short_descr, skill_table[sn].name);
	    free_string(crystal->short_descr);
	    crystal->short_descr = str_dup(buf);
	    sprintf(buf, crystal->name, skill_table[sn].name);
        setName(*crystal, buf);
	    crystal->value[0] = level;
	    crystal->value[1] = 1;
	    crystal->value[2] = 1;
	    crystal->value[3] = sn;
	    obj_to_char(crystal, victim);
	    return false;
	}
	
	return true;
}

// Assumes ch and victim are both non-null and in a room and the target is TARGET_CHAR
// This is for other than TAR_CHAR_OFFENSIVE or TAR_OBJ_CHAR_OFF
static bool adjust_for_defensive_targeted_spells(CHAR_DATA * ch, CHAR_DATA * victim, int sn, int level)
{
    // Check for distill magic	
    AFFECT_DATA * distill(get_affect(victim, gsn_distillmagic));
    if (distill != NULL && distill->modifier == 0)
    {
        if (ch == victim)
        {
            act("The misty aura of magic about you shimmers, then distills your magic down into a potion.", victim, NULL, NULL, TO_CHAR);
            act("The misty aura of magic about $n shimmers, then distills $s magic down into a potion.", victim, NULL, NULL, TO_ROOM);
        }
        else
        {
            act("The misty aura of magic about you shimmers, then distills $N's magic down into a potion.", victim, NULL, ch, TO_CHAR);
            act("The misty aura of magic about $n shimmers, then distills your magic down into a potion.", victim, NULL, ch, TO_VICT);
            act("The misty aura of magic about $n shimmers, then distills $N's magic down into a potion.", victim, NULL, ch, TO_NOTVICT);
        }

        // Make the potion
        char buf[MAX_STRING_LENGTH];
        OBJ_DATA * potion(create_object(get_obj_index(OBJ_VNUM_DISTILLEDPOTION), distill->level));
        
        sprintf(buf, potion->short_descr, skill_table[sn].name);
        free_string(potion->short_descr);
        potion->short_descr = str_dup(buf);

        sprintf(buf, potion->name, skill_table[sn].name);
        setName(*potion, buf);

        potion->value[0] = level;
        potion->value[1] = sn;
        obj_to_char(potion, victim);

        // Change the effect to not allow another potion
        distill->modifier = 1;
        return false;
    }

	return true;
}

bool validate_target(CHAR_DATA * ch, CHAR_DATA * victim, int sn, bool offensiveSpell)
{
	if (victim != NULL)
	{
		// No casting on others in ghost form
		if (victim != ch && IS_OAFFECTED(ch, AFF_GHOST))
		{
			send_to_char("Your ghostly form cannot channel enough power to affect others.\n", ch);
			return false;
		}
		
		// Illusion check
		if (!can_be_affected(victim, sn))
		{
			send_to_char("Illusions have no passions to excite.\n", ch);
			return false;
		}
		
		if (offensiveSpell)
		{
			// No harming ghosts
			if (IS_OAFFECTED(victim, AFF_GHOST))
			{
				act("Your magics would have no effect on $N's ghostly form.", ch, NULL, victim, TO_CHAR);
				return false;
			}
			
			// Check for own follower
			if (IS_AFFECTED(victim, AFF_CHARM) && victim->master == ch)
			{
				send_to_char( "You can't do that to your own follower.\n",ch );
				return false;
			}
			
			// General safe_spell check
			if (!IS_NPC(ch))
			{
				if (is_safe_spell(ch, victim, FALSE) && victim != ch)
					return false;
					
				check_killer(ch, victim);
			}
		}
	}
	
	return true;
}

bool determine_target(CHAR_DATA * ch, CHAR_DATA *& victim, int sn, char *& argument, char * arg, void *& vo, int & target, bool & offensiveSpell, bool & requiredStoneOfSalyra)
{
	char dummy[MAX_STRING_LENGTH];
	offensiveSpell = false;
    requiredStoneOfSalyra = false;
	
	switch (skill_table[sn].target)
    {		
		case TAR_IGNORE: 
			//argument = one_argument(argument, dummy); 
			//vo = (void *) argument; 
			return true;
		
		case TAR_CHAR_OFFENSIVE:
			argument = one_argument(argument, dummy); 
			offensiveSpell = true;
			if (arg[0] == '\0')
			{
				victim = ch->fighting;
				if (victim == NULL)
				{
					send_to_char("Cast the spell on whom?\n\r", ch);
					return false;
				}
			}
			else
			{
				victim = get_char_room(ch, arg);
				if (victim == NULL)
				{
                    // Check for dreamstalk casting
                    if (IS_SET(skill_table[sn].flags, SN_STALKCAST))
                    {
                        victim = get_char_world(ch, arg);
                        AFFECT_DATA * paf;
                        if (victim != NULL && (paf = get_affect(victim, gsn_dreamstalk)) != NULL && paf->modifier == ch->id)
                        {
                            // Found a dreamstalk victim, check for sleeping
                            if (victim->position > POS_SLEEPING)
                            {
                                act("You can only stalk $N in $S dreams.", ch, NULL, victim, TO_CHAR);
                                return false;
                            }

                            // Sleeping victim
                            vo = (void*)victim;
                            target = TARGET_CHAR;
                            return true;
                        }
                    }
					
                    send_to_char("You see no one here by that name.\n", ch);
    				return false;
				}
			}

			vo = (void *) victim;
			target = TARGET_CHAR;
			return true;

		case TAR_CHAR_DEFENSIVE:
			argument = one_argument(argument, dummy); 
			if (arg[0] == '\0')
				victim = ch;
			else
			{
				victim = get_char_room(ch, arg);
				if (victim == NULL)
				{
                    // Check for stone of salyra, which reuses the somaticboost flag since they cover the same spells
                    if (IS_SET(skill_table[sn].flags, SN_SOMATICBOOST))
                    {
                        victim = get_char_world(ch, arg);
                        if (victim != NULL)
                        {
                            // Make sure the target is wearing a stone keyed to the caster
                            bool found(false);
                            for (OBJ_DATA * stone(victim->carrying); stone != NULL; stone = stone->next_content)
                            {
                                if (!stone->worn_on) 
                                    continue;

                                AFFECT_DATA * stoneAff(get_obj_affect(stone, gsn_stoneofsalyra));
                                if (stoneAff != NULL && stoneAff->modifier == ch->id)
                                {
                                    found = true;
                                    requiredStoneOfSalyra = true;
                                    break;
                                }
                            }

                            if (!found)
                                victim = NULL;
                        }
                    }

                    if (victim == NULL)
                    {
					    send_to_char("You see no one here by that name.\n", ch);
    					return false;
                    }
				}
			}

			vo = (void *) victim;
			target = TARGET_CHAR;
			return true;

		case TAR_CHAR_SELF:
			if (arg[0] != '\0' && !is_name(arg, ch->name ))
			{
				send_to_char("You cannot cast this spell on another.\n", ch);
				return false;
			}

			vo = (void *) ch;
			target = TARGET_CHAR;
			return true;

		case TAR_OBJ_INV:
        {
			argument = one_argument(argument, dummy); 
			if (arg[0] == '\0')
			{
				send_to_char("Upon what should the spell be cast?\n", ch);
				return false;
			}

			OBJ_DATA * obj(get_obj_carry(ch, arg, ch));
			if (obj == NULL)
			{
				send_to_char("You are not carrying that.\n", ch);
				return false;
			}

			vo = (void *) obj;
			target = TARGET_OBJ;
			return true;
        }

		case TAR_OBJ_CHAR_OFF:
        {
			argument = one_argument(argument, dummy); 
			offensiveSpell = true;
			if (arg[0] == '\0')
			{
				victim = ch->fighting;
				if (victim == NULL)
				{
					send_to_char("Cast the spell on whom or what?\n", ch);
					return false;
				}
			
			    vo = (void *) victim;
				target = TARGET_CHAR;
				return true;
			}
			
			victim = get_char_room(ch, arg);
			if (victim != NULL)
			{
			    vo = (void *) victim;
				target = TARGET_CHAR;
				return true;
			}
			
			OBJ_DATA * obj(get_obj_here(ch, arg));
			if (obj == NULL)
			{
                // Check for dreamstalk casting
                if (IS_SET(skill_table[sn].flags, SN_STALKCAST))
                {
                    victim = get_char_world(ch, arg);
                    AFFECT_DATA * paf;
                    if (victim != NULL && (paf = get_affect(victim, gsn_dreamstalk)) != NULL && paf->modifier == ch->id)
                    {
                        // Found a dreamstalk victim, check for sleeping
                        if (victim->position > POS_SLEEPING)
                        {
                            act("You can only stalk $N in $S dreams.", ch, NULL, victim, TO_CHAR);
                            return false;
                        }
            
                        // Found a dreamstalk victim
                        vo = (void*)victim;
                        target = TARGET_CHAR;
                        return true;
                    }
                }

				send_to_char("You don't see that here.\n", ch);
				return false;
			}
			
			vo = (void *) obj;
			target = TARGET_OBJ;
			return true;
        }

		case TAR_OBJ_CHAR_DEF:
        {
			argument = one_argument(argument, dummy); 
			if (arg[0] == '\0')
			{
				vo = (void *) ch;
				target = TARGET_CHAR;
				return true;
			}
			
			victim = get_char_room(ch, arg);
			if (victim != NULL)
			{
				vo = (void *) victim;
				target = TARGET_CHAR;
				return true;
			}
	        
			OBJ_DATA * obj(get_obj_carry(ch, arg, ch));
			if (obj != NULL)
			{
				vo = (void *) obj;
				target = TARGET_OBJ;
				return true;
			}
	    
	        send_to_char("You don't see that here.\n", ch);
	        return false;
        }

		case TAR_OBJ_ROOM:
			argument = one_argument(argument, dummy); 
			if (arg[0] == '\0')
			{
				send_to_char("Cast the spell on what?\n", ch);
				return false;
			}

			vo = (void*)get_obj_room(ch, const_cast<char*>(arg));
			if (vo == NULL)
			{
				send_to_char("You don't see that around here.\n", ch);
				return false;
			}

			target = TARGET_OBJ;
			return true;
    }
	
	bug("do_cast: bad spell target for sn %d.", sn); 
	return false;
}

void do_cast( CHAR_DATA *ch, char *argument )
{
    perform_spellcasting(ch, argument, false);
}

static void perform_actual_casting(CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj, void * vo, int sn, int target, 
    int mana, int beats, int level, bool charAttacked, bool adamantine, bool requiredStoneOfSalyra)
{
   if (!validate_target(ch, victim, sn, charAttacked)) 
       return;

    CHAR_DATA * conduit(NULL);
    if (is_affected(ch, gsn_manaconduit))
        conduit = get_conduit(ch, ch->in_room, 0);
   
    // Stone of Salyra allows for casting certain spells at distance, but at increased mana cost by range
    if (victim != NULL && requiredStoneOfSalyra)
    {
        // Find a path to the target from the caster
        if (ch->in_room == NULL || victim->in_room == NULL)
        {
            act("Your link to $N is too faint at this distance to channel any magics.", ch, NULL, victim, TO_CHAR);
            return;
        }

        RoomPath path(*ch->in_room, *victim->in_room, ch, 100);
        if (!path.Exists())
        {
            act("Your link to $N is too faint at this distance to channel any magics.", ch, NULL, victim, TO_CHAR);
            return;
        }

        // Charge additional mana according to distance
        mana = (mana * (120 + path.StepCount())) / 100;
    }
    
    // Adjust for adamantine invocation
    if (adamantine)
    {
        // Verify target type
        if (target != TARGET_CHAR || victim == NULL)
        {
            send_to_char("You cannot bring the earth's force to bear with such a spell.\n", ch);
            return;
        }

        // Add the mana cost and ensure a minimum lag
        mana += skill_table[gsn_adamantineinvocation].min_mana;
        beats = UMAX(beats, skill_table[gsn_adamantineinvocation].beats);
    }
   
    if ((!IS_NPC(ch) || find_bilocated_body(ch) != NULL) && ch->mana < mana )
    {
	    if (!conduit || (conduit->mana < mana))
	    {
	        if (ch->mana > 0 && !IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_GIFTED))
		        conduit = NULL;
	        else
	        {
                // Check for Brimstone Conduit
                if (is_skill_sphere_any(sn, SPH_FIRE))
                {
                    PyreInfo * pyreInfo(getPyreInfoEffect(ch, PyreInfo::Effect_BrimstoneConduit));
                    if (pyreInfo != NULL && pyreInfo->isEffectGreater())
                    {
                        // Cannot help past -350
                        if (ch->mana < -350)
                        {
                            send_to_char("Not even your connection to the Inferno can sustain your ragged mind any further.\n", ch);
                            return;
                        }

                        if (ch->mana < -250)
                        {
                            // Can go past -250, but it hurts badly
                            send_to_char("You barely manage to call on the Inferno to assist you, and are too weary to reign in its destructive force!\n", ch);
                            damage_new(ch, ch, dice(5, 40), gsn_bloodpyre, DAM_FIRE, true, "infernal link");
                        }
                        else if (ch->mana < -150)
                        {
                            // Starts to hurt past -150
                            send_to_char("You draw on the Inferno to assist you, but cannot completely control it!\n", ch);
                            damage_new(ch, ch, dice(5, 10), gsn_bloodpyre, DAM_FIRE, true, "infernal link");
                        }
                        else send_to_char("You draw on your mental conduit to the Inferno for the power to cast!\n", ch);
                    }
                    else
                    {
                        send_to_char("You don't have enough mana.\n", ch);
                        return;
                    }
                }
                else
                {
		            send_to_char( "You don't have enough mana.\n\r", ch );
		            return;
                }
	        }
	    }
    }

    if (number_percent() <= get_skill(ch, gsn_subvocalize))
        check_improve(ch, NULL, gsn_subvocalize, true, 8);
    else
    {
        check_improve(ch, NULL, gsn_subvocalize, false, 8);
	    say_spell(ch, sn);
    }
      
    int chance = get_skill(ch,sn);
    char buf[MAX_STRING_LENGTH];
      
    if (victim != NULL)
	{
        if ((ch->fighting != victim) && !IS_NPC(victim) && !IS_NPC(ch) && victim->fighting != ch 
		&& ((skill_table[sn].target == TAR_CHAR_OFFENSIVE) || (skill_table[sn].target == TAR_OBJ_CHAR_OFF)) && victim != ch)
		{
			if (can_see(victim, ch))
			{
				sprintf( buf, "Help!  %s is attacking me!", PERS(ch, victim) );
				do_autoyell( victim, buf );
			}
			else
			{
				sprintf( buf, "Help! I'm being attacked!" );
				do_autoyell( victim, buf );
				do_visible(ch, "FALSE");
			}
		}
	}

    if ((sn == gsn_greaterdemonsummon) && (silver_state == SILVER_FINAL))
		chance = 100;

    // Check for failure due to encumbrance
    int failChance;
    switch (Encumbrance::LevelFor(*ch))
    {
        case Encumbrance::None:     failChance = 0;     break;
        case Encumbrance::Light:    failChance = 10;    break;
        case Encumbrance::Medium:   failChance = 25;    break;
        case Encumbrance::Heavy:    failChance = 50;    break;
        default:                    failChance = 100;   break;
    }
 
    if (number_percent() <= failChance)
    {
        send_to_char("You are encumbered by your armor, and fumble the spell.\n", ch);
		cast_mana(ch, mana / 2, conduit, beats);
        return;
    }

    if ( number_percent( ) > chance )
    {
		send_to_char( "You lost your concentration.\n\r", ch );
		check_improve(ch,NULL,sn,FALSE,1);
		cast_mana(ch, mana / 2, conduit, beats);
    }
    else
    {
        AFFECT_DATA * song;
    	if ((song = get_room_song(ch->in_room, gsn_stuttering)) != NULL)
	    {
	        int schance = 25 + song->level - ch->level;

    	    if (number_percent() <= schance)
	        {
				send_to_char("The words of the Stuttering Sage ring in your mind, and you lose your concentration.\n\r", ch);
				cast_mana(ch, mana, conduit, beats);
				return;
    	    }
	    }
		
		if (victim != NULL && victim->in_room != NULL && target == TARGET_CHAR)
		{
			bool shouldReturn(!adjust_for_targeted_spell(ch, victim, sn, level, mana));
            if (!shouldReturn)
            {
    			if (charAttacked) shouldReturn = !adjust_for_offensive_spell(ch, victim, sn, level);
	    		else shouldReturn = !adjust_for_defensive_targeted_spells(ch, victim, sn, level);
            }
			
			if (shouldReturn)
			{
				do_magic_detect(ch, sn);
				cast_mana(ch, mana, conduit, beats);
				return;
			}

            // Assign the vo in case one of the adjustments changed the victim
            vo = victim;
		}

		if (ch->in_room && ch->clan == clan_lookup("SHUNNED"))
		{
			CHAR_DATA *vch;
			AFFECT_DATA *paf;
			int clvls = 0;

			for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
			{
				if (IS_OAFFECTED(vch, AFF_COVEN))
				{
					for (paf = vch->affected; paf; paf = paf->next)
					{
						if ((paf->type == gsn_coven) && ((CHAR_DATA *) paf->point == ch))
							clvls += (vch->level / 10);
					}
				}
			}

			if (clvls > 0)
			{
				if (clvls > 20)
					clvls = 20 + ((clvls - 20) / 3);

				if (clvls > 40)
					clvls = 40;

				level += clvls;
			}
		}

		if (victim && is_an_avatar(victim) && skill_table[sn].target == TAR_CHAR_DEFENSIVE)
		{
			send_to_char("An avatar cannot receive such magic.\n\r", ch);
			return;
		}

		if (is_affected(ch, gsn_chameleon))
		{
			affect_strip(ch, gsn_chameleon);
			act("You return to your natural form.", ch, NULL, NULL, TO_CHAR);
			act("$n returns to $s natural form.", ch, NULL, NULL, TO_ROOM);
		}

        // Handle any echo affects
        AFFECT_DATA * echoAff(get_affect(ch, EchoAffect::GSN));
        if (echoAff != NULL && echoAff->point != NULL)
        {
            // Handle echo affect, including removal if appropriate
            if (static_cast<EchoAffect*>(echoAff->point)->HandleCastSpell(ch, sn, level, vo, target))
                affect_remove(ch, echoAff);
        }

        // Check for adamantine
        bool legal;
        if (adamantine)
        {
            // Perform skill check
            if (number_percent() > get_skill(ch, gsn_adamantineinvocation))
            {
                // Improvement on failure is very hard here
                check_improve(ch, victim, gsn_adamantineinvocation, false, 12);
                send_to_char("You fail to draw the indomitable power of the earth into your spell.\n", ch);
                return;
            }

            // Prepare the adamantine effect
            performAdamantineCast(ch, victim, sn, UMAX(1, level), target_name); 
            legal = true;
        }
        else
        {
            // Actually cast the spell
		    protect_works = FALSE;
    	    legal = (*skill_table[sn].spell_fun) (sn, UMAX(1, level), ch, vo, target);
	        protect_works = TRUE;
        }

	    if (legal)
	    {
            do_magic_detect(ch, sn);
            cast_mana(ch, mana, conduit, beats);

	        if (sn == gsn_nightfears || sn == gsn_riteofkaagn)
				check_improve(ch, NULL, sn, TRUE, 0);
			else
				check_improve(ch, NULL, sn, TRUE, 1);
	    }
	    ch->lastsn = sn;
    }

    if ((skill_table[sn].target == TAR_CHAR_OFFENSIVE
    ||   (skill_table[sn].target == TAR_OBJ_CHAR_OFF && target == TARGET_CHAR))
    &&   victim != ch
    &&   victim->master != ch
    &&   !victim->mercy_from
    &&   ch->in_room && victim->in_room && ch->in_room == victim->in_room
    &&   IS_VALID(ch) && IS_VALID(victim)
    &&   sn != gsn_subdue
    &&	 victim->position > POS_SLEEPING
    &&   !IS_NAFFECTED(victim, AFF_FLESHTOSTONE))
    {
		if (!victim->fighting)
		{
			check_killer(victim, ch);
			victim->fighting = ch;
			multi_hit(victim, ch, TYPE_UNDEFINED);
		}
		if (can_see(victim,ch)
	    && victim->position > POS_SLEEPING
	    && !(sn == gsn_fire_breath
	    || sn == gsn_gas_breath
	    || sn == gsn_frost_breath
	    || sn == gsn_cone_of_cold
	    || sn == gsn_ball_lightning
	    || sn == gsn_soulflare
	    || sn == gsn_chain_lightning))
		{
            // Check for ethereal brethren
            bool shouldStrike(number_percent() <= get_skill(victim, gsn_opportunism));
            bool alreadyEchoed(false);

            AFFECT_DATA * brethren(get_affect(victim, gsn_etherealbrethren));
            if (!shouldStrike && brethren != NULL && number_percent() <= 15 && brethren->duration >= 1)
            {
                brethren->duration -= 1;
                act("Your hands are guided by a newfound memory to strike $n as $e casts $s spell!", ch, NULL, victim, TO_VICT);
                shouldStrike = true;
                alreadyEchoed = true;
            }

			if (shouldStrike)
			{
                if (!alreadyEchoed)
    				act("You swiftly strike as $n casts a spell!",ch,NULL,victim,TO_VICT);
	
    			act("$N swiftly strikes as you cast your spell!",ch,NULL,victim,TO_CHAR);
				act("$N swiftly strikes as $n casts a spell!",ch,NULL,victim,TO_NOTVICT);
				one_hit(victim,ch,TYPE_UNDEFINED,HIT_OPPORTUNITY, false);
				check_improve(victim,ch,gsn_opportunism,TRUE,1);
			}	
			else
				check_improve(victim,ch,gsn_opportunism,FALSE,1);
		}
    }
}

void perform_spellcasting(CHAR_DATA *ch, char *argument, bool adamantine)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    AFFECT_DATA af;;
	af.valid = TRUE;
	af.point = NULL;
    int mana; // , cmana = 0;
    int sn;
    int level = ch->level;

    if (IS_AFFECTED(ch, AFF_CHARM) || is_affected(ch, gsn_commandword))
		return;

    if (IS_NPC(ch) && (ch->pIndexData->vnum == MOB_VNUM_SHUNNED_DEMON))
    {
		send_to_char("Your demonic form cannot perform spellcasting.\n\r", ch);
		return;
    }

    target_name = one_argument( argument, arg1 );
    one_argument( target_name, arg2 );

    if (IS_SET(ch->in_room->room_flags, ROOM_NOMAGIC))
	{
		send_to_char( "Your spell fizzles out and fails.\n\r", ch);
		act( "$n's spell fizzles out and fails.", ch, NULL, NULL, TO_ROOM);
		WAIT_STATE(ch, 2*PULSE_VIOLENCE);
		return;
	}

    if (ch->in_room && room_is_affected(ch->in_room, gsn_earthmaw))
    {
		send_to_char("The suffocating dirt and soil prevents you from properly casting.\n\r", ch);
		return;
    }

    if (is_affected(ch, gsn_wolfform) || is_affected(ch, gsn_bearform) || is_affected(ch, gsn_hawkform))
    {
		send_to_char("You cannot cast while taking the aspect of a creature.\n\r", ch);
		return;
   }

    if (is_affected(ch, gsn_bludgeon) && (number_bits(1) == 0))
    {
		send_to_char("You're too dazed to concentrate.\n\r", ch);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;
    }
    
    if (is_affected(ch, gsn_nettles) && (number_bits(1) == 0))
    {
		send_to_char("The itch is too maddening to concentrate!\n\r", ch);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;
    }

    if (is_an_avatar(ch) || is_affected(ch, gsn_shadowfiend))
    {
		send_to_char("You can't cast spells in your current form.\n\r", ch);
		return;
    }

    bool subvocalized(number_percent() <= get_skill(ch, gsn_subvocalize));
    if (IS_NAFFECTED(ch, AFF_GARROTE_VICT) && number_percent() < (subvocalized ? 25 : 50))
    {
		send_to_char("The pain in your neck prevents you from speaking!\n\r", ch);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;
    }

    if (is_affected(ch,gsn_gag) && number_percent() < (subvocalized ? 25 : 50))
    { 
       send_to_char("The gag around your mouth muffles your words.\n\r",ch);
		WAIT_STATE(ch, PULSE_VIOLENCE);
       return;
    }
    if (get_modifier(ch->affected,gsn_wrathofthevoid) == 1)
    {
		send_to_char("The green ichor still binds your mouth.\n\r",ch);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;
    }

    if (IS_PAFFECTED(ch, AFF_VOIDWALK))
    {
		send_to_char("You cannot channel magics normally while walking the void.\n\r", ch);
		return;
    }

    if ( arg1[0] == '\0' )
    {
		send_to_char( "Cast which what where?\n\r", ch );
		return;
    }

    if ((sn = find_spell(ch, arg1, NULL)) < 1 || (!IS_NPC(ch) && get_skill(ch, sn) <= 0))
    {
		send_to_char( "You don't know any spells of that name.\n\r", ch );
		return;
    }

    if (!IS_SET(skill_table[sn].flags, SN_SPIRITCAST) && is_affected(ch, gsn_astralprojection))
    {
		send_to_char("You cannot draw on such magics in your current form.\n", ch);
		return;
    }

    if (!IS_SET(skill_table[sn].flags, SN_BILOCATECAST) && is_affected(ch, gsn_bilocation) && IS_NPC(ch))
    {
        send_to_char("You cannot draw on such magics in your illusory form.\n", ch);
        return;
    }

    if (!IS_SET(skill_table[sn].flags, SN_PHYSIKERCAST) && is_affected(ch, gsn_physikersinstinct))
    {
        send_to_char("You are focused too intently on healing to cast that spell.\n", ch);
        return;
    }

    if ( ch->position < skill_table[sn].minimum_position )
    {
		send_to_char( "You can't concentrate enough.\n\r", ch );
		return;
    }

    if (sn == gsn_sanctuary && ch->position != POS_STANDING)
    {
        if (number_percent() > get_skill(ch, gsn_fieldmedicine))
        {
		    send_to_char("You can't concentrate enough.\n\r", ch);
    		return;
        }

        check_improve(ch, NULL, gsn_fieldmedicine, true, 8);
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_NOMAGIC))
	{
		send_to_char( "Your spell fizzles out and fails.\n\r", ch);
		act( "$n's spell fizzles out and fails.", ch, NULL, NULL, TO_ROOM);
		WAIT_STATE(ch, 2*PULSE_VIOLENCE);
		return;
	}

    if (IS_OAFFECTED(ch, AFF_DEAFEN) && number_percent() < (subvocalized ? 25 : 50))
    {
		send_to_char( "You can't hear yourself well enough to cast.\n\r", ch);
		if (number_percent() < 50) {WAIT_STATE(ch, PULSE_VIOLENCE);}
		else {WAIT_STATE(ch, 2*PULSE_VIOLENCE);}
		return;
    }

    if (ch->in_room != NULL && area_is_affected(ch->in_room->area, gsn_sunderweave) && number_percent() < 50)
    {
        if (number_percent() <= get_skill(ch, gsn_weavecraft))
        {
            check_improve(ch, NULL, gsn_weavecraft, true, 2);
            send_to_char("Your craft helps you find a link to the distant Weave.\n", ch);
        }
        else
        {
            send_to_char("Your connection to magic fails you as you struggle to find threads of power here.\n", ch);
            WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
            return;
        }
    }

    if (is_affected(ch, gsn_drown) && number_percent() <= (is_affected(ch, gsn_waterbreathing) ? 15 : 50))
    {
        send_to_char("With your lungs full of water, you garble the spell.\n", ch);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return;
    }

	if (IS_PAFFECTED(ch, AFF_MUTE) && number_percent() < (subvocalized ? 50 : 75))
	{
	    send_to_char("You are unable to get the words out to cast.\n\r", ch);
	    if (number_percent() < 50) {WAIT_STATE(ch, PULSE_VIOLENCE);}
  	    else {WAIT_STATE(ch, 2*PULSE_VIOLENCE);}
	    return;		
	}

    AFFECT_DATA * crush(get_affect(ch, gsn_crush));
    if (crush != NULL && number_percent() <= (crush->modifier / (subvocalized ? 2 : 1)))
    {
        send_to_char("You wheeze in pain, disrupting the spell.\n", ch);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return;
    }

    if (ch->in_room && room_is_affected(ch->in_room, gsn_sanctify) && is_skill_sphere_any(sn, SPH_VOID))
    {
		send_to_char("A holy power protects this place.\n\r",ch);
		return;
    }

    if (ch->level + 2 == skill_table[sn].skill_level[ch->class_num])
		mana = 50;
    else
    {
		if (!IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_MAGAPT))
			mana = skill_table[sn].min_mana;
		else
    	    mana = UMAX(skill_table[sn].min_mana,
	    100 / ( 2 + ch->level - skill_table[sn].skill_level[ch->class_num] ) );
    }

    if (sn == gsn_sanctuary && ch->position != POS_STANDING)
		mana = 100;

    if (is_affected(ch, gsn_dark_insight))
	{
        if (mana < 20) mana = 20;
		else mana += mana / 5;
	}
    
    if (IS_OAFFECTED(ch, AFF_GHOST) 
    && (skill_table[sn].target != TAR_CHAR_SELF)
    && (skill_table[sn].target != TAR_CHAR_DEFENSIVE))
    {
		send_to_char("You cannot cast that spell while in ghost form.\n\r", ch);
		return;
    }

    int beats = skill_table[sn].beats;
	if (!adjust_spell_parameters(ch, sn, level, beats, mana))
		return;

    /*
     * Locate targets.
     */
    CHAR_DATA * victim	= NULL;
    OBJ_DATA * obj		= NULL;
    void * vo		= NULL;
    int target	= TARGET_NONE;
    bool charAttacked(false);
    bool requiredStoneOfSalyra(false);

	if (!determine_target(ch, victim, sn, target_name, arg2, vo, target, charAttacked, requiredStoneOfSalyra)) return;
    perform_actual_casting(ch, victim, obj, vo, sn, target, mana, beats, level, charAttacked, adamantine, requiredStoneOfSalyra);

    // Check for puppetmaster (aka reshackle)
    if (target == TARGET_CHAR && victim != NULL && !IS_NPC(victim) && victim->in_room != NULL
    && (skill_table[sn].target == TAR_CHAR_OFFENSIVE || skill_table[sn].target == TAR_OBJ_CHAR_OFF)
    && number_percent() <= get_skill(ch, gsn_reshackle))
    {
        // Find all charmies in the room belonging to the victim
        CHAR_DATA * next_charmy;
        for (CHAR_DATA * charmy(victim->in_room->people); charmy != NULL; charmy = next_charmy)
        {
            next_charmy = charmy->next_in_room;
            if (IS_NPC(charmy) && IS_AFFECTED(charmy, AFF_CHARM) && charmy->master == victim)
                perform_actual_casting(ch, charmy, obj, charmy, sn, target, mana, beats, level, charAttacked, adamantine, requiredStoneOfSalyra);
        }
    }
}

void do_mprog_cast( CHAR_DATA *ch, int sn, int level, CHAR_DATA *mptarget )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    void *vo;
    int mana;
    int target;
    int chance;

    if (skill_table[sn].spell_fun == spell_form)
    {
	bug( "FORM SPELL LAMAH.", 0 );
	return;
    }

    if (skill_table[sn].spell_fun == spell_null)
    {
	bug( "NULL SPELL LAMAH.", 0 );
	return;
    }
    
    if (skill_table[sn].spell_fun == spell_song)
    {
        bug("SONG SPELL LAMAH.", 0);
        return;
    }

    if ( ch->position < skill_table[sn].minimum_position )
    {
	send_to_char( "You can't concentrate enough.\n\r", ch );
	return;
    }

	mana=0;
	
	int beats = skill_table[sn].beats;
	if (!adjust_spell_parameters(ch, sn, level, beats, mana))
		return;

    /*
     * Locate targets.
     */
    victim	= NULL;
    obj		= NULL;
    vo		= NULL;
    target	= TARGET_NONE;
    bool charAttacked(false);

    switch ( skill_table[sn].target )
    {
    default:
	bug( "Do_cast: bad target for sn %d.", sn );
	return;

    case TAR_IGNORE:
	break;

    case TAR_CHAR_OFFENSIVE:
        charAttacked = true;
	if (mptarget)
	    victim = mptarget;
	else
	    victim = ch->fighting;

	if (!victim)
	{
	    send_to_char( "Cast the spell on whom?\n\r", ch );
	    return;
	}

	vo = (void *) victim;
	target = TARGET_CHAR;
	break;

    case TAR_CHAR_DEFENSIVE:
	
	if (mptarget == NULL)
	    victim = ch;
	else
	    victim = mptarget;

	vo = (void *) victim;
	target = TARGET_CHAR;
	break;

    case TAR_CHAR_SELF:
	victim = ch; 
	vo = (void *) victim;
	break;

    case TAR_OBJ_INV:
 	return;	


    case TAR_OBJ_CHAR_OFF:
        charAttacked = true;
	if (mptarget)
	    victim = mptarget;
	else
	    victim = ch->fighting;

	if (!victim)
	{
	    send_to_char("Cast the spell on whom or what?\n\r",ch);
	    return;
	}
	
	target = TARGET_CHAR;

	if (target == TARGET_CHAR) /* check the sanity of the attack */
	    vo = (void *) victim;
	else
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}
	break; 

    case TAR_OBJ_CHAR_DEF: return;
    case TAR_OBJ_ROOM: return;
   }
   
    if (!validate_target(ch, victim, sn, charAttacked)) 
		return;
	    
    if ( !IS_NPC(ch) && ch->mana < mana )
    {
	send_to_char( "You don't have enough mana.\n\r", ch );
	return;
    }

    if (victim && (ch->fighting != victim) && !IS_NPC(victim)
     && !IS_NPC(ch)
     && victim->fighting != ch
     && ((skill_table[sn].target == TAR_CHAR_OFFENSIVE)
      || (skill_table[sn].target == TAR_OBJ_CHAR_OFF))
     && victim != ch)
    {
	if (can_see(victim, ch))
	{
	    sprintf( buf, "Help!  %s is attacking me!", PERS(ch, victim) );
	    do_autoyell( victim, buf );
	}
	else
	{
	    sprintf( buf, "Help! I'm being attacked!" );
	    do_autoyell( victim, buf );

	    if (IS_PAFFECTED(ch, AFF_VOIDWALK))
	    {  
		AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

		act("You shift back into the material world.", ch, NULL, NULL, TO_CHAR);
		act("$n shifts $s essense back into the material world.", ch, NULL, NULL, TO_ROOM);
		ch->hit = UMAX(1, ch->hit / 4);
		affect_strip(ch, gsn_voidwalk);

		af.where     = TO_PAFFECTS;
		af.type      = gsn_voidwalk;
		af.level     = level;
		af.duration  = ch->level / 4;
		af.location  = APPLY_HIT;
		af.modifier  = ch->level * -1;
		af.bitvector = 0;
		affect_to_char(ch, &af);

		WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
		send_to_char("You pause for a moment, feeling drained from the experience.\n\r", ch);
	    }
	}
    }

    WAIT_STATE( ch, skill_table[sn].beats );

    chance = get_skill(ch,sn);

    if (IS_NPC(ch))
     	chance=95;

    if ((sn == gsn_greaterdemonsummon) && (silver_state == SILVER_FINAL))
	chance = 100;
 
    if ( number_percent( ) > chance )
    {
	send_to_char( "You lost your concentration.\n\r", ch );
	expend_mana(ch, mana / 2);
    }
    else
    {
        expend_mana(ch, mana);
	if (is_affected(ch, gsn_manabarbs))
	{
	    ch->hit -= mana/2;
	    send_to_char ("You wince as the manabarbs bite into your mind.\n\r", ch);
	    act("$n winces as they cast a spell.",ch,NULL,NULL, TO_ROOM);
	}        

    if (victim != NULL && victim->in_room != NULL && target == TARGET_CHAR)
	{
        bool shouldReturn(!adjust_for_targeted_spell(ch, victim, sn, level, mana));
        if (!shouldReturn)
        {
            if (charAttacked) shouldReturn = !adjust_for_offensive_spell(ch, victim, sn, level);
            else shouldReturn = !adjust_for_defensive_targeted_spells(ch, victim, sn, level);
        }
		
		if (shouldReturn)
		{
			do_magic_detect(ch, sn);
			return;
		}
        
        // Assign the vo in case one of the adjustments changed the victim
        vo = victim;
	}

	if (is_affected(ch, gsn_chameleon))
	{
	    affect_strip(ch, gsn_chameleon);
	    act("You return to your natural form.", ch, NULL, NULL, TO_CHAR);
	    act("$n returns to $s natural form.", ch, NULL, NULL, TO_ROOM);
	}

    (*skill_table[sn].spell_fun) ( sn, level, ch, vo,target);
   }

/* Don't let wizi mobs hang people up in combat with spells for runes/etc */
   if (IS_AFFECTED(ch, AFF_WIZI))
       stop_fighting_all(ch);

   if (!IS_AFFECTED(ch, AFF_WIZI))
    if ((skill_table[sn].target == TAR_CHAR_OFFENSIVE
    ||   (skill_table[sn].target == TAR_OBJ_CHAR_OFF && target == TARGET_CHAR))
    &&   victim != ch
    &&   victim->master != ch)
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	if (ch && ch->in_room)
	    for ( vch = ch->in_room->people; vch; vch = vch_next )
	    {
	        vch_next = vch->next_in_room;
	        if ( victim == vch && victim->fighting == NULL )
	        {	
		    check_killer(victim,ch);
		    multi_hit( victim, ch, TYPE_UNDEFINED );
		    break;
	        }
	    }
    }
}

/*
 * Cast spells at targets using a magical object.
 */
void obj_cast_spell( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
    void *vo;
    int target = TARGET_NONE;
    char charAttacked(false);
    
    if ( sn <= 0 )
	return;

    if ( sn >= MAX_SKILL || skill_table[sn].spell_fun == 0 )
    {
	bug( "Obj_cast_spell: bad sn %d.", sn );
	return;
    }

    switch ( skill_table[sn].target )
    {
    default:
	bug( "Obj_cast_spell: bad target for sn %d.", sn );
	return;

    case TAR_IGNORE:
	vo = NULL;
	if (victim)
	{
	    vo = (void *) victim;  // let's try passing this if we decide to in the code
	    target = TARGET_CHAR;
	}
	else if (obj)
	{
	    vo = (void *) obj;
	    target = TARGET_OBJ;
	}
	break;

    case TAR_CHAR_OFFENSIVE:
        charAttacked = true;
	if ( victim == NULL )
	    victim = ch->fighting;
	if ( victim == NULL )
	{
	    send_to_char( "You can't do that.\n\r", ch );
	    return;
	}
	if (is_safe_spell(ch,victim,FALSE) && ch != victim)
	    return;
	vo = (void *) victim;
	target = TARGET_CHAR;
	break;

    case TAR_CHAR_DEFENSIVE:
    case TAR_CHAR_SELF:
	if ( victim == NULL )
	    victim = ch;
	vo = (void *) victim;
	target = TARGET_CHAR;
	break;

    case TAR_OBJ_INV:
	if ( obj == NULL )
	{
	    send_to_char( "You can't do that.\n\r", ch );
	    return;
	}
	vo = (void *) obj;
	target = TARGET_OBJ;
	break;

    case TAR_OBJ_CHAR_OFF:
        charAttacked = true;
        if ( victim == NULL && obj == NULL)
	    if (ch->fighting != NULL)
		victim = ch->fighting;
	    else
	    {
		send_to_char("You can't do that.\n\r",ch);
		return;
	    }

	    if (victim != NULL)
	    {
		if (is_safe_spell(ch,victim,FALSE) && ch != victim)
		    return;

		vo = (void *) victim;
		target = TARGET_CHAR;
	    }
	    else
	    {
	    	vo = (void *) obj;
	    	target = TARGET_OBJ;
	    }
        break;


    case TAR_OBJ_CHAR_DEF:
	    if (victim == NULL && obj == NULL)
    	{
	        vo = (void *) ch;
	        target = TARGET_CHAR;
    	}
	    else if (victim != NULL)
    	{
	        vo = (void *) victim;
	        target = TARGET_CHAR;
    	}
	    else
    	{
	        vo = (void *) obj;
	        target = TARGET_OBJ;
    	}
	
	    break;

    case TAR_OBJ_ROOM:
        if (obj == NULL)
        {
            send_to_char("You can't do that.\n", ch);
            return;
        }

        vo = (void*)obj;
        target = TARGET_OBJ;
        break;
    }

    if (victim != NULL && victim->in_room != NULL && target == TARGET_CHAR)
	{
        int dummy(0);
        bool shouldReturn(!adjust_for_targeted_spell(ch, victim, sn, level, dummy));
        if (!shouldReturn)
        {
            if (charAttacked) shouldReturn = !adjust_for_offensive_spell(ch, victim, sn, level);
            else shouldReturn = !adjust_for_defensive_targeted_spells(ch, victim, sn, level);
        }
		
		if (shouldReturn)
		{
			do_magic_detect(ch, sn);
			return;
		}

        // Adjust the vo in case one of the adjustments changed the victim
        vo = victim; 
	}

    target_name = "";
    (*skill_table[sn].spell_fun) ( sn, level, ch, vo,target);

    if ( (skill_table[sn].target == TAR_CHAR_OFFENSIVE
    ||   (skill_table[sn].target == TAR_OBJ_CHAR_OFF && target == TARGET_CHAR))
    &&   victim != ch
    &&   victim->master != ch )
    {
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;
		check_killer(ch,victim);

		for ( vch = ch->in_room->people; vch; vch = vch_next )
		{
			vch_next = vch->next_in_room;
			if ( victim == vch && victim->fighting == NULL )
			{
                check_killer(victim,ch);
                multi_hit( victim, ch, TYPE_UNDEFINED );
                break;
			}
		}
    }
}

