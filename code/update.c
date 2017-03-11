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
#include <string.h>
#include <time.h>
#include <math.h>
#include <sstream>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "magic.h"
#include "olc.h"
#include "tables.h"
#include "lookup.h"
#include "demons.h"
#include "spells_fire.h"
#include "spells_fire_earth.h"
#include "spells_spirit.h"
#include "spells_spirit_air.h"
#include "spells_water.h"
#include "spells_water_air.h"
#include "spells_water_earth.h"
#include "spells_air.h"
#include "spells_air_earth.h"
#include "spells_earth_void.h"
#include "spells_earth.h"
#include "spells_void.h"
#include "EchoAffect.h"
#include "Titles.h"
#include "RoomPath.h"
#include "Weave.h"
#include "ShadeControl.h"
#include "songs.h"
#include "PhantasmInfo.h"
#include "Luck.h"
#include "Faction.h"
#include "NameMaps.h"
#include "Runes.h"
#include "Drakes.h"
#include "Oaths.h"

#if defined(macintosh) || defined(MSDOS) || defined(WIN32)
const   char    telnet_nop  [] = { '\0' };
#endif

#if defined(unix)
#include "telnet.h"
const   char    telnet_nop  [] = { IAC, NOP, '\0' };
#endif

/* command procedures needed */
DECLARE_DO_FUN(do_look          );
DECLARE_DO_FUN(do_quit		);

extern bool do_pctrackstep(CHAR_DATA *ch, char *argument);
extern void    get_player_levels       args( ( void) );
extern void    calc_limit_values       args( (void) );
extern int     limit_calc      args( (int limit_factor) );
extern void    rip_obj_pfiles  args( ( void) );
extern bool	saves_focus	args( ( int level, CHAR_DATA *ch, CHAR_DATA *victim, bool newfocus) );
extern	void	path_step	args( ( CHAR_DATA *ch ) );
extern	void 	mob_class_aggr	args( ( CHAR_DATA *ch, CHAR_DATA *victim) );

extern	void	save_room_desc	args( ( ROOM_INDEX_DATA *room ) );
extern	void	append_note	args( ( NOTE_DATA *pnote ) );
extern bool write_to_descriptor args( ( int desc, const char *txt, int length ) );

extern void pfile_maint(bool loading);

extern void area_affect_update ( void );
extern void unbind_shunned_demon(CHAR_DATA *victim);
extern char *target_name;
extern void do_cast( CHAR_DATA *ch, char *argument );
extern void pillage_update ( void );
extern CHAR_DATA *get_affinity_owner(OBJ_DATA *obj);
extern void weather_update ( void );
extern int get_moon_state ( int moon_num, bool get_size );
extern bool global_bool_check_avoid;
extern bool crumble_process;
extern bool crumble_test;
extern bool rip_test;
extern bool crumble_done;
extern bool rip_process;		/* track ongoing update */

extern void do_alchemy( CHAR_DATA *ch, char *argument, int alc_sn, int delay, double added_power);
extern void find_items_note( CHAR_DATA *ch, char *argument);
extern void destroy_maze(ROOM_INDEX_DATA *pRoomIndex, ROOM_INDEX_DATA *tRoom);
				

void	lunar_update_char	args ((CHAR_DATA *ch) );

bool rip_done = FALSE;

bool global_song_message = FALSE;

/*
 * Local functions.
 */
int	hit_gain	args( ( CHAR_DATA *ch ) );
int	mana_gain	args( ( CHAR_DATA *ch ) );
int	move_gain	args( ( CHAR_DATA *ch ) );
void	mobile_update	args( ( void ) );
void    track_update	args( ( void ) );
void	time_update	args( ( void ) );
void	char_update	args( ( void ) );
void	obj_update	args( ( void ) );
void	aggr_update	args( ( void ) );
void	room_update	args( ( void ) );
void	stone_update	args( ( void ) );
void	write_limits args( ( void ) );
void 	rip_proc	args ( (void) );

void 	rumor_update( void );
/* used for saving */

unsigned int	save_number = 0;
int     dur = 0;

static bool has_pc_corpse(const ROOM_INDEX_DATA & room)
{
    for (OBJ_DATA * obj(room.contents); obj != NULL; obj = obj->next_content)
    {
        if (obj->item_type == ITEM_CORPSE_PC)
            return true;
    }

    return false;
}

void room_update( void )
{
    ROOM_INDEX_DATA *pRoomIndex = NULL, *to_room = NULL, *room_next = NULL;
    CHAR_DATA *vch = NULL, *vch_next = NULL;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    AFFECT_DATA *paf = NULL, *paf_next = NULL;
    int iHash, x;
    bool exists;

    af.where	= TO_ROOM_AFF;
    af.type 	= gsn_blazinginferno;
    af.level 	= 60; 
    af.duration	= 3;
    af.location	= 0;
    af.modifier	= 0;
    af.bitvector = 0;
	af.type = 0;
	af.point = NULL;
   
    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = room_next )
        {
            room_next = pRoomIndex->next;
            exists = TRUE;

            if (IS_SET(pRoomIndex->room_flags, ROOM_VAULT) || IS_SET(pRoomIndex->room_flags, ROOM_SAVE) || has_pc_corpse(*pRoomIndex))
                save_room_obj(pRoomIndex);
            else
                clear_room_obj(pRoomIndex);

            if (pRoomIndex->owner && pRoomIndex->owner[0] != '\0')
                save_room_desc(pRoomIndex);

            rprog_time_trigger(pRoomIndex);
            rprog_tick_trigger(pRoomIndex);

            // Check for barrowmist
            paf = get_area_affect(pRoomIndex->area, gsn_barrowmist);
            if (paf != NULL)
                handleUpdateBarrowmist(*pRoomIndex, *paf);
 
            for ( paf = pRoomIndex->affected; paf != NULL; paf = paf_next )
            {
            	paf_next    = paf->next;
            	if ( paf->duration > 0 )
            	{
                    paf->duration--;  
                    
                    if (paf->type == gsn_cloudkill)
                    {
                        CHAR_DATA *vch;
                        for (vch = pRoomIndex->people; vch; vch = vch->next_in_room)
                        {
                            if (!IS_IMMORTAL(vch)
                            && !IS_OAFFECTED(vch,AFF_GHOST)
                            && !is_affected(vch,gsn_cloudkill)
                            && number_percent() < 10
                            && !saves_spell(paf->level, NULL, vch,DAM_POISON)
                            && (!is_affected(vch, gsn_mistralward) || number_bits(2) == 0))
                            {
                                act("You choke as you breathe in the noxious fumes of the stinking cloud!",vch,NULL,NULL,TO_CHAR);
                                act("$n chokes and gags, breathing in the noxious fumes of the stinking cloud!",vch,NULL,NULL,TO_ROOM);
                                af.where = TO_AFFECTS;
                                af.type = gsn_cloudkill;
                                af.level = paf->level;
                                af.duration = 8;
                                af.location = APPLY_STR;
                                af.modifier = -2;
                                af.bitvector = AFF_POISON;
                                af.point = paf->point;
                                affect_to_char(vch,&af);
                            }
                        }
                    }
                    else if (paf->type == gsn_lavaforge)
                        handleUpdateLavaForge(*pRoomIndex, *paf);
                }  
                else if ( paf->duration < 0 )
                {
                    if (paf->type == gsn_grimseep)
                    {
                        check_grimseep_update(*pRoomIndex, *paf);
                        continue;
                    }
                }
                else
                {
		    if (paf->type == gsn_circleofstones)
		    {
			CHAR_DATA *vch;

			for (vch = char_list; vch; vch = vch->next)
			    if ((vch->id == paf->modifier) && vch->in_room && (vch->recall_to == pRoomIndex))
			    {
				vch->recall_to = vch->recall_old;
				vch->recall_old = NULL;
				send_to_char("You feel your circle of stones crumble away.\n\r", vch);
			    }
		    }

		    if (paf->type == gsn_icyprison)
		    {
			    paf->type = 0;
    			destroy_icyprison(pRoomIndex, get_room_index(paf->modifier));
	    		exists = FALSE;
		    	break;
		    }

            if (paf->type == gsn_stasisrift)
            {
                if (paf->modifier == 0) 
                    act("The dark, swirling portal shrinks to pinpoint, then vanishes.", pRoomIndex->people, NULL, NULL, TO_ALL);
                else
                {
                    destroy_stasisrift(*pRoomIndex, paf->modifier);
                    exists = false;
                    break;
                }
            }

           	if (paf->type == gsn_findcover)
		    {
			ROOM_INDEX_DATA *recall = get_room_index(ROOM_VNUM_TEMPLE);
		 	ROOM_INDEX_DATA *old_room = NULL;
			OBJ_DATA *vobj, *vobj_next;

			for (vch = pRoomIndex->people; vch; vch = vch_next)
			{
			    vch_next = vch->next_in_room;
			    old_room = get_room_index(paf->modifier);
			    
			    send_to_char("You grow restless and abandon your cover.\n\r", vch);
			    global_linked_move = TRUE;
			    char_from_room(vch);
			    
			    if (old_room)
				char_to_room(vch, old_room);
			    else
				char_to_room(vch, recall);
			    
			    if (vch->in_room->vnum != 0)
				vch->was_in_room = NULL;
			    do_look(vch, "auto");		
			}
			for (vobj = pRoomIndex->contents; vobj; vobj = vobj_next)
			{
			    vobj_next = vobj->next_content;
			    extract_obj(vobj);
			}
				
			free_room_area(pRoomIndex);
			exists = FALSE;
			break;
		    }

		    if (paf->type == gsn_maze)
		    {
			paf->type = 0;
			destroy_maze(pRoomIndex, get_room_index(paf->modifier));
			exists = FALSE;
			break;
		    }

		    if (paf->type == gsn_spirit_sanctuary)
		    {
			ROOM_INDEX_DATA *recall = get_room_index(ROOM_VNUM_TEMPLE);
		 	ROOM_INDEX_DATA *old_room = NULL;
			for (vch = pRoomIndex->people; vch; vch = vch_next)
			{
			    vch_next = vch->next_in_room;
			    old_room = vch->was_in_room;
			    
			    send_to_char("The spirit sanctuary fades away.\n\r", vch);
			    global_linked_move = TRUE;
			    char_from_room(vch);
			    
			    if (old_room)
				char_to_room(vch, old_room);
			    else
				char_to_room(vch, recall);
			    
			    if (vch->in_room->vnum != 0)
				vch->was_in_room = NULL;
			    do_look(vch, "auto");		
			}

			exists = FALSE;
			break;
		    }
		    if (paf->type == gsn_dim)
		    {
			act("The ambient light in the room returns to normal.", pRoomIndex->people, NULL, NULL, TO_ALL);
		    }
		    if (paf->type == gsn_globedarkness)
		    {
			act("The globe of darkness recedes.", pRoomIndex->people, NULL, NULL, TO_ALL);
		    }

		    if (paf->type == gsn_etherealblaze)
		    {
			act("The flames lining your surroundings fade away.", pRoomIndex->people, NULL, NULL, TO_CHAR);
			act("The flames lining your surroundings fade away.", pRoomIndex->people, NULL, NULL, TO_ROOM);
		    }
			
	            if (paf->type == gsn_plantgrowth)
		    {
			act("The growth around you crumbles away, returning the room to its natural state.", pRoomIndex->people, NULL, NULL, TO_CHAR);
			act("The growth around you crumbles away, returning the room to its natural state.", pRoomIndex->people, NULL, NULL, TO_ROOM);
		    }

		    if (paf->type == gsn_wallofwater)
		    {
			act("The wall of water filling the area sinks back into the ground.", pRoomIndex->people, NULL, NULL, TO_CHAR);
			act("The wall of water filling the area sinks back into the ground.", pRoomIndex->people, NULL, NULL, TO_ROOM);
		    }

		    if (paf->type == gsn_flood)
		    {
                act("The flooding waters slowly recede.", pRoomIndex->people, NULL, NULL, TO_CHAR);
                act("The flooding waters slowly recede.", pRoomIndex->people, NULL, NULL, TO_ROOM);
		    }

            if (paf->type == gsn_curse) act("The unholy magic binding this place dissipates.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_webofoame) act("The threads of the dark web unravel into nothingness.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_bierofunmaking) act("The dark bier dissolves into wisps of shadow.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_maelstrom) act("The surging waters grow calm and still once more.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_markofthekaceajka) act("The icy rune on the ground dulls to a faint blue, then fades.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_glyphofulyon) act("The glyph gleams brightly a single time, then vanishes.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_steam) act("The steam hanging the air slowly disperses.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_hoarfrost) act("The frost riming this place melts away.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_boilseas) act("The water all about cools off, no longer boiling.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_unleashtwisters) act("The cyclone slows, then dissipates altogether.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_channelwind) act("The stone channel crumbles, causing the wind rushing through it to die down.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_sandstorm) act("The stinging, sand-laden wind calms, leaving this place covered in fine grains.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_figmentscage) act("The walls of energy dissolve into tiny motes of light, which wink out one by one.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_sparkingcloud) act("The sparking cloud dissipates with a final crackle of energy.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_clingingfog) act("The clinging fog slowly drifts away.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_lavaforge) act("The lava flow recedes back into the earth, which closes behind it.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_pillarofsparks) act("The metal pillar crumbles, coming apart in corroded flakes.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_stonehaven) act("The deep earthen power of this place slowly dies away.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_latticeofstone) act("The stone lattice on the ground grows silent, gradually crumbling away.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_glyphofentombment) act("The complex glyph on the ground slowly crumbles, its lines fading away.", pRoomIndex->people, NULL, NULL, TO_ALL);
            if (paf->type == gsn_stonetomud) act("The thick layer of mud on the ground solidifies, hardening into solid earth.", pRoomIndex->people, NULL, NULL, TO_ALL);
            
            if (paf->type == gsn_kaagnsplaguestone) 
            {
                for (CHAR_DATA * vch(pRoomIndex->people); vch != NULL; vch = vch->next_in_room)
                {
                    if (number_percent() <= get_skill(vch, gsn_stonecraft))
                        send_to_char("The sickly air about this place slowly vanishes as the disease leaches out of the ground.\n", vch);
                }
            }

		    if (paf->type == gsn_tangletrail)
		    {
		    	int i;

		    	for (i = 0; i < 6; i++)
			{
			    if (pRoomIndex->exit[i] && (pRoomIndex->exit[i]->exit_info & EX_ILLUSION))
				free_exit(pRoomIndex->exit[i]);
			    pRoomIndex->exit[i] = pRoomIndex->old_exit[i];
			}

		    	act("The forest around you untwists.", pRoomIndex->people, NULL, NULL, TO_CHAR);
		    	act("The forest around you untwists.", pRoomIndex->people, NULL, NULL, TO_ROOM);
	            }

		    if (paf->type == gsn_wallofvines)
		    {
		        char buf[MAX_STRING_LENGTH];

			if (IS_SET(pRoomIndex->exit[paf->modifier]->exit_info, EX_WALLOFVINES))
			{

			    REMOVE_BIT(pRoomIndex->exit[paf->modifier]->exit_info, EX_WALLOFVINES);
		        
			    if (pRoomIndex->people)
			    {
				sprintf(buf, "The wall of vines %s decays.",
	    			    ((paf->modifier == 0) ? "to the north" : (paf->modifier == 1) ? "to the east" :
				    (paf->modifier == 2) ? "to the south" : (paf->modifier == 3) ? "to the west" :
				    (paf->modifier == 4) ? "below you" : "above you"));

				act(buf, pRoomIndex->people, NULL, NULL, TO_CHAR);
				act(buf, pRoomIndex->people, NULL, NULL, TO_ROOM);
			    }
			}

		        if (pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]
		         && (pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]->u1.to_room == pRoomIndex)
			 && IS_SET(pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]->exit_info, EX_WALLOFVINES))
		        {
			    REMOVE_BIT(pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]->exit_info, EX_WALLOFVINES);
			    
			    if (pRoomIndex->exit[paf->modifier]->u1.to_room->people)
			    {
				sprintf(buf, "The wall of vines %s decays.",
	    			    ((OPPOSITE(paf->modifier) == 0) ? "to the north" : (OPPOSITE(paf->modifier) == 1) ? "to the east" :
				    (OPPOSITE(paf->modifier) == 2) ? "to the south" : (OPPOSITE(paf->modifier) == 3) ? "to the west" :
				    (OPPOSITE(paf->modifier) == 4) ? "below you" : "above you"));
			    
				act(buf, pRoomIndex->exit[paf->modifier]->u1.to_room->people, NULL, NULL, TO_CHAR);
				act(buf, pRoomIndex->exit[paf->modifier]->u1.to_room->people, NULL, NULL, TO_ROOM);
			    }
		        }
		    }

		    if (paf->type == gsn_runeofearth)
		    {
		        char buf[MAX_STRING_LENGTH];

			if (IS_SET(pRoomIndex->exit[paf->modifier]->exit_info, EX_RUNEOFEARTH))
			{
			    REMOVE_BIT(pRoomIndex->exit[paf->modifier]->exit_info, EX_RUNEOFEARTH);

			    if (pRoomIndex->people)
			    {   
				sprintf(buf, "The rune of earth %s fades.",
	    			    ((paf->modifier == 0) ? "to the north" : (paf->modifier == 1) ? "to the east" :
				    (paf->modifier == 2) ? "to the south" : (paf->modifier == 3) ? "to the west" :
				    (paf->modifier == 4) ? "below you" : "above you"));

				act(buf, pRoomIndex->people, NULL, NULL, TO_CHAR);
				act(buf, pRoomIndex->people, NULL, NULL, TO_ROOM);
			    }
			}

		        if (pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]
		         && (pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]->u1.to_room == pRoomIndex)
			 && IS_SET(pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]->exit_info, EX_RUNEOFEARTH))
		        {
			    REMOVE_BIT(pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]->exit_info, EX_RUNEOFEARTH);

			    if (pRoomIndex->exit[paf->modifier]->u1.to_room->people)
			    {
				sprintf(buf, "The rune of earth %s fades.",
	    			    ((OPPOSITE(paf->modifier) == 0) ? "to the north" : (OPPOSITE(paf->modifier) == 1) ? "to the east" :
				    (OPPOSITE(paf->modifier) == 2) ? "to the south" : (OPPOSITE(paf->modifier) == 3) ? "to the west" :
				    (OPPOSITE(paf->modifier) == 4) ? "below you" : "above you"));
			    
				act(buf, pRoomIndex->exit[paf->modifier]->u1.to_room->people, NULL, NULL, TO_CHAR);
				act(buf, pRoomIndex->exit[paf->modifier]->u1.to_room->people, NULL, NULL, TO_ROOM);
			    }
		        }
		    }

            if (paf->type == gsn_runeoffire)
                REMOVE_BIT(pRoomIndex->exit[paf->modifier]->exit_info, EX_RUNEOFFIRE);

		    if (paf->type == gsn_wallofstone)
            {
                char buf[MAX_STRING_LENGTH];

                if (IS_SET(pRoomIndex->exit[paf->modifier]->exit_info, EX_WALLED))
                {

                    REMOVE_BIT(pRoomIndex->exit[paf->modifier]->exit_info, EX_WALLED);
                    
                    if (pRoomIndex->people)
                    {
                    sprintf(buf, "The wall of stone %s crumbles to dust.",
                            ((paf->modifier == 0) ? "to the north" : (paf->modifier == 1) ? "to the east" :
                        (paf->modifier == 2) ? "to the south" : (paf->modifier == 3) ? "to the west" :
                        (paf->modifier == 4) ? "above you" : "below you"));

                    act(buf, pRoomIndex->people, NULL, NULL, TO_CHAR);
                    act(buf, pRoomIndex->people, NULL, NULL, TO_ROOM);
                    }
                }

                if (pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]
                && (pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]->u1.to_room == pRoomIndex)
                && IS_SET(pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]->exit_info, EX_WALLED))
                {
                    REMOVE_BIT(pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]->exit_info, EX_WALLED);
                    
                    if (pRoomIndex->exit[paf->modifier]->u1.to_room->people)
                    {
                    sprintf(buf, "The wall of stone %s crumbles to dust.",
                            ((OPPOSITE(paf->modifier) == 0) ? "to the north" : (OPPOSITE(paf->modifier) == 1) ? "to the east" :
                        (OPPOSITE(paf->modifier) == 2) ? "to the south" : (OPPOSITE(paf->modifier) == 3) ? "to the west" :
                        (OPPOSITE(paf->modifier) == 4) ? "below you" : "above you"));
                    
                    act(buf, pRoomIndex->exit[paf->modifier]->u1.to_room->people, NULL, NULL, TO_CHAR);
                    act(buf, pRoomIndex->exit[paf->modifier]->u1.to_room->people, NULL, NULL, TO_ROOM);
                    }
                }
            }

            if (paf->type == gsn_manifestweave)
            {
                if (pRoomIndex->exit[paf->modifier] != NULL && IS_SET(pRoomIndex->exit[paf->modifier]->exit_info, EX_WEAVEWALL))
                {
                    REMOVE_BIT(pRoomIndex->exit[paf->modifier]->exit_info, EX_WEAVEWALL);
                    if (pRoomIndex->people)
                    {
                        std::ostringstream mess;
                        mess << "The shimmering golden mesh " << Direction::DirectionalNameFor(static_cast<Direction::Value>(paf->modifier));
                        mess << " flickers, then dissolves into nothing.";
                        act(mess.str().c_str(), pRoomIndex->people, NULL, NULL, TO_ALL);
                    }
                }
            }

            if (paf->type == gsn_wintersstronghold)
            {
                for (unsigned int i(0); i < Direction::Max; ++i)
                {
                    if (pRoomIndex->exit[i] != NULL && IS_SET(pRoomIndex->exit[i]->exit_info, EX_ICEWALL))
                    {
                        REMOVE_BIT(pRoomIndex->exit[i]->exit_info, EX_ICEWALL);
                        if (pRoomIndex->people != NULL)
                        {
                            std::ostringstream mess;
                            mess << "The thick pane of ice " << Direction::DirectionalNameFor(static_cast<Direction::Value>(i));
                            mess << " melts away.";
                            act(mess.str().c_str(), pRoomIndex->people, NULL, NULL, TO_ALL);
                        }
                    }
                }
            }

		    if (paf->type == gsn_walloffire)
		    {
		        char buf[MAX_STRING_LENGTH];

			if (IS_SET(pRoomIndex->exit[paf->modifier]->exit_info, EX_WALLOFFIRE))
			{

			    REMOVE_BIT(pRoomIndex->exit[paf->modifier]->exit_info, EX_WALLOFFIRE);
		        
			    if (pRoomIndex->people)
			    {
				sprintf(buf, "The wall of fire %s runs out of fuel and is exhausted.",
	    			    ((paf->modifier == 0) ? "to the north" : (paf->modifier == 1) ? "to the east" :
				    (paf->modifier == 2) ? "to the south" : (paf->modifier == 3) ? "to the west" :
				    (paf->modifier == 4) ? "below you" : "above you"));

				act(buf, pRoomIndex->people, NULL, NULL, TO_CHAR);
				act(buf, pRoomIndex->people, NULL, NULL, TO_ROOM);
			    }
			}

		        if (pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]
		         && (pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]->u1.to_room == pRoomIndex)
			 && IS_SET(pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]->exit_info, EX_WALLOFFIRE))
		        {
			    REMOVE_BIT(pRoomIndex->exit[paf->modifier]->u1.to_room->exit[OPPOSITE(paf->modifier)]->exit_info, EX_WALLOFFIRE);
			    
			    if (pRoomIndex->exit[paf->modifier]->u1.to_room->people)
			    {
				sprintf(buf, "The wall of fire %s runs out of fuel and is exhausted.",
	    			    ((OPPOSITE(paf->modifier) == 0) ? "to the north" : (OPPOSITE(paf->modifier) == 1) ? "to the east" :
				    (OPPOSITE(paf->modifier) == 2) ? "to the south" : (OPPOSITE(paf->modifier) == 3) ? "to the west" :
				    (OPPOSITE(paf->modifier) == 4) ? "below you" : "above you"));
			    
				act(buf, pRoomIndex->exit[paf->modifier]->u1.to_room->people, NULL, NULL, TO_CHAR);
				act(buf, pRoomIndex->exit[paf->modifier]->u1.to_room->people, NULL, NULL, TO_ROOM);
			    }
		        }
		    }

                    affect_remove_room( pRoomIndex, paf );
                }
            }

	    if (!exists)
		continue;

	    if (area_is_affected(pRoomIndex->area, gsn_icestorm) && room_is_affected(pRoomIndex, gsn_blazinginferno))
	    {
	        room_affect_strip(pRoomIndex, gsn_blazinginferno);
	        for (vch = pRoomIndex->people; vch != NULL; vch = vch->next_in_room)
		    send_to_char("The blazing inferno is stiffled by the biting cold ice storm!\n\r", vch);
	    }

	    if (room_is_affected(pRoomIndex, gsn_blazinginferno) && get_modifier(pRoomIndex->affected, gsn_blazinginferno) < 12)
	    {
	        for (x = 0; x < 6; x++)
	        {
	 	    if (pRoomIndex->exit[x] == NULL)
	 	        continue;

		    if ((number_bits(1) == 0)
		     || room_is_affected(to_room = pRoomIndex->exit[x]->u1.to_room, gsn_blazinginferno) 
                     || (to_room->sector_type == SECT_AIR) 
                     || (to_room->sector_type == SECT_WATER_NOSWIM) 
		     || (to_room->sector_type == SECT_WATER_SWIM) 
                     || room_is_affected(to_room, gsn_wallofwater)
		     || (to_room->sector_type == SECT_UNDERWATER)
		     || IS_SET(to_room->room_flags, ROOM_NEWBIES_ONLY)) 
		        continue;
		
		    af.modifier = get_modifier(pRoomIndex->affected, gsn_blazinginferno) + 1;
		    affect_to_room(to_room, &af);

		    for (vch = pRoomIndex->people; vch != NULL; vch = vch->next_in_room)
		         send_to_char("The roaring blaze tears through its surroundings, spreading rapidly!\n\r", vch);

		    for (vch = to_room->people; vch != NULL; vch = vch->next_in_room)
		        send_to_char("A fire blazes in from nearby, setting the room on fire!\n\r", vch);
	         }
	    }

        // Check for contaminate
        AFFECT_DATA * contaminant(get_room_affect(pRoomIndex, gsn_contaminate));
        if (contaminant != NULL && contaminant->duration >= 2)
        {
            // Get all the water rooms adjacent to this one
            for (unsigned int i(0); i < Direction::Max; ++i)
            {
                ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*pRoomIndex, static_cast<Direction::Value>(i)));
                if (nextRoom != NULL && is_water_room(*nextRoom) && !room_is_affected(nextRoom, gsn_contaminate) && number_bits(2) == 0)
                {
                    // Spread to this adjacent room
                    if (nextRoom->people != NULL)
                        act("An oily black fluid spreads in, dispersing through the water here.", nextRoom->people, NULL, NULL, TO_ALL);

                    affect_to_room(nextRoom, contaminant);
                }
            }
        }

        // Check for unleash twister movement and object throwing
        AFFECT_DATA * twister_next;
        for (AFFECT_DATA * twister(get_room_affect(pRoomIndex, gsn_unleashtwisters)); twister != NULL; twister = twister_next)
        {
            twister_next = get_room_affect(pRoomIndex, gsn_unleashtwisters, twister);
            
            // Check for throwing objects
            std::vector<OBJ_DATA*> objs;
            for (OBJ_DATA * obj(pRoomIndex->contents); obj != NULL; obj = obj->next_content)
            {
                if (CAN_WEAR(obj, ITEM_TAKE) && !IS_OBJ_STAT(obj, ITEM_NOLONG))
                    objs.push_back(obj);
            }

            std::vector<CHAR_DATA*> victims;
            for (CHAR_DATA * victim(pRoomIndex->people); victim != NULL; victim = victim->next_in_room)
            {
                if (!IS_AFFECTED(victim, AFF_WIZI) && (!IS_NPC(victim) || !IS_SET(victim->nact, ACT_SHADE)))
                    victims.push_back(victim);
            }

            std::vector<Direction::Value> directions(Direction::ValidDirectionsFrom(*pRoomIndex));
            int odds(80);
            while (odds > number_percent() && !objs.empty())
            {
                odds /= 2;

                // Throw an item; choose an item and target
                size_t index(number_range(0, objs.size() - 1));
                if (number_percent() <= 50 && !directions.empty())
                {
                    Direction::Value direction(directions[number_range(0, directions.size() - 1)]);
                    ROOM_INDEX_DATA * targetRoom(Direction::Adjacent(*pRoomIndex, direction, EX_CLOSED|EX_WALLED|EX_ICEWALL));
                    if (targetRoom != NULL)
                    {
                        // Throw the item to the adjacent room
                        std::ostringstream mess;
                        mess << "The spinning cyclone snatches up $p and flings it " << Direction::DirectionalNameFor(direction) << "!";
                        act(mess.str().c_str(), pRoomIndex->people, objs[index], NULL, TO_ALL);
                        obj_from_room(objs[index]);
                        obj_to_room(objs[index], targetRoom);
                        act("$p is hurled into the room!", targetRoom->people, objs[index], NULL, TO_ALL);

                        // Clean up the obj list
                        objs[index] = objs[objs.size() - 1];
                        objs.pop_back();
                    }
                    continue;
                }

                // Throw the item into a character
                if (!victims.empty())
                {
                    size_t victimIndex(number_range(0, victims.size() - 1));
                    
                    // Check for windrider
                    if (is_flying(victims[victimIndex]))
                    {
                        if (number_percent() <= get_skill(victims[victimIndex], gsn_windrider))
                        {
                            check_improve(victims[victimIndex], NULL, gsn_windrider, true, 4);
                            act("The twister hurls $p at you, but you glide easily away.", victims[victimIndex], objs[index], NULL, TO_CHAR);
                            act("The twister snatches up $p and hurls it at $n, who glides deftly out of the way!", victims[victimIndex], objs[index], NULL, TO_ROOM);
                            continue;
                        }
                        check_improve(victims[victimIndex], NULL, gsn_windrider, false, 4);
                    }

                    act("The raging twister snatches up $p and hurls it into you!", victims[victimIndex], objs[index], NULL, TO_CHAR);
                    act("The raging twister snatches up $p and hurls it into $n!", victims[victimIndex], objs[index], NULL, TO_ROOM);
                    int baseDam(get_obj_weight(objs[index]));
                    sourcelessDamage(victims[victimIndex], "the flying object", number_range(baseDam / 4, baseDam), gsn_unleashtwisters, DAM_BASH);

                    // Clean up the victim list if necessary, as damage could move the victim
                    if (victims[victimIndex]->in_room != pRoomIndex)
                    {
                        victims[victimIndex] = victims[victims.size() - 1];
                        victims.pop_back();
                    }
                }
            }

            // Check for moving
            int action(number_percent());
            if (action <= 50)
            {
                // Continue in the same direction as before
                Direction::Value direction(static_cast<Direction::Value>(twister->modifier));
                ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*pRoomIndex, direction, EX_CLOSED|EX_WALLED|EX_ICEWALL));
                if (nextRoom != NULL)
                {
                    // Move the twister
                    affect_to_room(nextRoom, twister);
                    affect_remove_room(pRoomIndex, twister);

                    // Send echoes
                    std::ostringstream mess;
                    mess << "A whirling twister of air spins off " << Direction::DirectionalNameFor(direction) << "!";
                    act(mess.str().c_str(), pRoomIndex->people, NULL, NULL, TO_ALL);
                    act("A whirling twister of air spins in from nearby!", nextRoom->people, NULL, NULL, TO_ALL);
                    continue;
                }
            }

            // Check for choosing a new direction; 20% odds outright, plus the 40% if the twister wanted to move but could not
            if (action <= 70 && !directions.empty())
                twister->modifier = directions[number_range(0, directions.size() - 1)];
        }

 	    if (room_is_affected(pRoomIndex, gsn_earthmaw))
	    {
            bool escaped = FALSE;
	        for (vch = pRoomIndex->people; vch; vch = vch_next)
	        {
	     	    vch_next = vch->next_in_room;
                damage_old(vch, vch, number_range(round(vch->level*1.5), round(vch->level*2.5)), gsn_earthmaw, DAM_OTHER, TRUE);
		
                if (vch && vch->in_room && number_bits(2) == 0)
                {
                    send_to_char("You manage to desperately climb out of the earth maw.\n\r", vch);
                    act("$n desperately claws $s way out of the earth maw.", vch, NULL, NULL, TO_ROOM);
                    char_from_room(vch);
                    char_to_room(vch, pRoomIndex->exit[4]->u1.to_room);
                    do_look(vch, "auto");
                    act("$n claws $s way out of the earth below.", vch, NULL, NULL, TO_ROOM);
                    escaped = TRUE;
                    break;
                }
            }

            if (escaped)
                continue;
	    }

	    if (room_is_affected(pRoomIndex, gsn_poisonspirit))
	    {
		OBJ_DATA *fObj;
                AFFECT_DATA poison, *pspirit = affect_find(pRoomIndex->affected, gsn_poisonspirit);
poison.valid = TRUE;
poison.point = NULL;

		for (fObj = pRoomIndex->contents; fObj; fObj = fObj->next_content)
		    if (((fObj->item_type == ITEM_FOOD) || (fObj->item_type == ITEM_FOUNTAIN))
		     && (fObj->value[3] == 0) && (!obj_is_affected(fObj, gsn_poison)))
		    {
			poison.where	= TO_OBJECT;
			poison.type	= gsn_poison;
			poison.level	= pspirit->level;
			poison.duration = pspirit->duration;
			poison.modifier = 0;
                        poison.location = 0;
			poison.bitvector = 0;
			poison.point = NULL;
				
			affect_to_obj(fObj, &poison);
		    }
	    }
	
	    for (x = 0; x < 6; x++)
	    {
	    	if (pRoomIndex->exit[x] == NULL)
		    continue;

	    	if (pRoomIndex->exit[x] != NULL && (pRoomIndex->exit[x]->exit_info & EX_ILLUSION) && !room_is_affected(pRoomIndex, gsn_tangletrail) && (number_bits(5) == 0))
 	    	{
		    free_exit(pRoomIndex->exit[x]);
		    pRoomIndex->exit[x] = NULL;
			continue;
	    	}
	    }
        }
    }
}


void write_limits( void )
{
    FILE *fp;
    int vnum;
    OBJ_INDEX_DATA *pObjIndex;
    int nMatch = 0;

    if ((fp = fopen(LIMITTEMP, "w")) == NULL)
	{
	  bug("Failed to open LIMITTEMP. Unable to write limit data.",0);
	  return;
	}

    fprintf(fp, "PLevels %d\n", player_levels);
    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
    {
        if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
        {
            nMatch++;
	    if (pObjIndex->limit_factor == 0)
		continue;
	    fprintf(fp, "Item %d %d\n", pObjIndex->vnum, pObjIndex->current);
        }
    }  

    fprintf(fp, "#END\n");
    fclose(fp);
    rename(LIMITTEMP, LIMITFILE);

}

void check_heirloom_level(CHAR_DATA *ch, OBJ_DATA *obj)
{
    OBJ_DATA *pObj;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int i;

    if (IS_NPC(ch) || !BIT_GET(ch->pcdata->traits, TRAIT_HEIRLOOM))
	return;

    if (!obj)
	for (pObj = ch->carrying; pObj; pObj = pObj->next_content)
	    if (pObj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
	    {
		obj = pObj;
		break;
	    }

    if (!obj)
        return;

    // Time to upgrade...
    if (ch->level >= obj->level)
    {
	obj->level += 10;
	
	switch (obj->item_type)
	{
	    case ITEM_WEAPON:
		obj->value[2] += 1;

		af.where     = TO_OBJECT;
		af.type	     = gsn_heirloom;    
		af.level     = obj->level;
		af.duration  = -1;
		af.location  = ((number_bits(1) == 0) ? APPLY_DAMROLL : APPLY_HITROLL);
		af.modifier  = 1;
		af.bitvector = 0;
		obj_affect_join(obj, &af);

		af.level     = obj->level;
		af.modifier  = 1;
		af.location  = ((number_bits(1) == 0) ? APPLY_STR : APPLY_DEX);
		obj_affect_join(obj, &af);
		break;

	    case ITEM_ARMOR:
		obj->value[0] += 1;
		obj->value[1] += 1;
		obj->value[2] += 1;
		obj->value[3] += 1;

		af.where     = TO_OBJECT;
		af.type	     = gsn_heirloom;    
//		af.level     = obj->level;
		af.duration  = -1;
//		af.location  = ((number_bits(1) == 0) ? APPLY_DAMROLL : APPLY_HITROLL);
//		af.modifier  = 1;
		af.bitvector = 0;
//		obj_affect_join(obj, &af);

		i = number_range(1, 5);
		switch (i)
		{
		    case 1: af.location = APPLY_STR;    break;
		    case 2: af.location = APPLY_DEX;    break;
		    case 3: af.location = APPLY_CON;    break;
		    case 4: af.location = APPLY_HITROLL;break;
		    case 5: af.location = APPLY_DAMROLL;break;
		}
    
		af.modifier = 1;
		af.level = obj->level;
		obj_affect_join(obj, &af);
		break;

	    case ITEM_JEWELRY:
		af.where     = TO_OBJECT;
		af.type	     = gsn_heirloom;
		af.level     = obj->level;
		af.duration  = -1;
		af.location  = ((number_bits(1) == 0) ? APPLY_HIT : APPLY_MANA);
		af.modifier  = 5;
		af.bitvector = 0;
		obj_affect_join(obj, &af);
/*
		af.modifier = 5;
		af.level = obj->level;
		af.location  = APPLY_MANA;
		obj_affect_join(obj, &af);
*/	
		i = number_range(1, 5);

		switch (i)
		{
		    case 1:
			af.location = APPLY_DAMROLL;
			af.modifier = 1;
			break;

		    case 2:
			af.location = APPLY_HITROLL;
			af.modifier = 1;
			break;

		    case 3:
			af.location = APPLY_INT;
			af.modifier = 1;
			break;

		    case 4:
			af.location = APPLY_WIS;
			af.modifier = 1;
			break;

		    case 5:
			af.location = APPLY_HIT;
			af.modifier = 5;
			break;

		    case 6:
			af.location = APPLY_MANA;
			af.modifier = 5;
			break;

		    case 7:
			af.location = APPLY_SAVES;
			af.modifier = -2;
			break;

		}
    
		af.level = obj->level;
		obj_affect_join(obj, &af);

		break;
	}
    }

    return;
}
    
	    

// Yes... levelize!
void levelize_mobile(CHAR_DATA *mob, int level)
{
    int x=0;

    if (level > 0)
        mob->level = level;

    mob->max_hit = 20 + con_app[25].hitp * level;

    while (x++ < level)
    	mob->max_hit += number_range(class_table[global_int_class_fighter].hp_min,class_table[global_int_class_fighter].hp_max);

    x = 0;

    mob->hit = mob->max_hit;

    mob->damage[DICE_NUMBER] = round((float) mob->level / 10.0) + 1;
    mob->damroll  = mob->level/2;
    mob->damage[DICE_TYPE]   = round(((float) mob->level - (float) mob->damage[DICE_BONUS]) / (float) mob->damage[DICE_NUMBER] * 2.0);

    mob->max_mana = dice(mob->level, 10) + 100;
    mob->mana = mob->max_mana;

    mob->hitroll = mob->level / 2;

    for (x = 0; x < 3; x++)
	mob->armor[x] = (mob->level - 15) * -6;

    mob->armor[AC_EXOTIC] = (mob->level - 30) * -3;

    return;
}

/*
 * Advancement stuff.
 */
void advance_level( CHAR_DATA *ch, bool hide )
{
    char buf[MAX_STRING_LENGTH];
    int add_hp, old_hit = ch->max_hit;
    int add_mana, old_mana = ch->max_mana;
    int add_move, old_move = ch->max_move;
    int add_prac;

    ch->pcdata->last_level = 
	( ch->played + (int) (current_time - ch->logon) ) / 3600;

    //if (ch->class_num == global_int_class_druid)
	//sprintf(buf, "%s",
	//  druid_title_table[ch->pcdata->minor_sphere-SPH_FIRIEL][ch->level][ch->sex==SEX_FEMALE ? 1 : 0]);
    //else
	sprintf( buf, "%s", Titles::LookupTitle(*ch));
    if (ch->trust < 51)
    	set_title( ch, buf );

    add_hp	= number_range(class_table[ch->class_num].hp_min, class_table[ch->class_num].hp_max );

    add_mana	= UMAX(1, round((dice(2, 6) - 1) * class_table[ch->class_num].fMana / 100.0));
	
    add_move	= (ch->race == global_int_race_chaja 
		    || ch->race == global_int_race_alatharya)
		  ? number_range(7, 8) 
		  : ch->race == global_int_race_nefortu
		  ? number_range(5, 8)
		  : number_range(6, 8);

    add_prac	= pc_race_table[ch->race].pracs_level;

    add_hp   = add_hp   * 9/10;

    ch->max_hit 	+= (add_hp + con_app[get_curr_stat(ch, STAT_CON)].hitp);

    ch->max_mana	+= (add_mana + round(((int_app[get_curr_stat(ch, STAT_INT)].mana_bonus / (class_table[ch->class_num].attr_prime == STAT_CHR ? 2 : 1)) + wis_app[get_curr_stat(ch, STAT_WIS)].mana_bonus + (class_table[ch->class_num].attr_prime == STAT_CHR ? chr_app[get_curr_stat(ch, STAT_CHR)].mana_bonus : 0)) * class_table[ch->class_num].fMana / 100.0));

    ch->max_move	+= add_move;

    ch->practice	+= add_prac;
    
    if (ch->level % 5 == 0) ch->train += 1;

    ch->pcdata->perm_hit	+= add_hp;
    ch->pcdata->perm_mana	+= add_mana;
    ch->pcdata->perm_move	+= add_move;

    if (ch->class_num == global_int_class_druid)
        lunar_update_char(ch);

    if (ch->pet && ch->pet->pIndexData->vnum == MOB_VNUM_RETAINER)
	levelize_mobile(ch->pet, ch->level);

    check_heirloom_level(ch, NULL);

    if (!hide)
    {
    	sprintf(buf,
	    "You gain %d hit point%s, %d mana, %d movement, and %d practice%s.\n\r",
	    ch->max_hit - old_hit, (ch->max_hit - old_hit) == 1 ? "" : "s", ch->max_mana - old_mana, ch->max_move - old_move,
	    add_prac, add_prac == 1 ? "" : "s");
	send_to_char( buf, ch );

	if (ch->level == 11)
	    send_to_char("{RWARNING{x: You are now level 11, and are able to be injured by other players.\n\r", ch);

	if (ch->level == 11)
	    send_to_char("{WNOTICE{x: Now that you have attained level 11, you are no longer allowed inside the school of Heroes.\n\r", ch);

	if ((ch->level >= 10) && (!ch->description || (ch->description[0] == '\0')))
	    send_to_char("{WREMINDER{x: Characters are required to have a description by level 10.\n\r", ch);

	if ((ch->level >= 25) && (!ch->pcdata->background || (ch->pcdata->background[0] == '\0')))
	    send_to_char("{WREMINDER{x: Characters are required to have a written background by level 25.\n\r", ch);
    }

    if (ch->level == 5 && BIT_GET(ch->pcdata->traits, TRAIT_ARISTOCRAT))
    {
	NOTE_DATA *pnote;
	char *strtime;

	pnote = new_note();

	pnote->sender	= str_dup("Your faithful steward");
	pnote->to_list	= str_dup(ch->name);
	pnote->subject  = str_dup("Notice of inheritance");

	sprintf(buf, "%s,\n\r\n\rToday, you inherit the estates and wealth which are yours by right of birth.\n\rThe first portion of this inheritance is twenty-five gold coins, which has\n\rbeen deposited into your bank account.\n\r\n\rEach year, I will deposit any additional monies generated by your estate\n\rinto your account.\n\r\n\r- Your faithful steward\n\r", ch->name);
	pnote->text	= str_dup(buf);

	strtime				= ctime( &current_time );
	strtime[strlen(strtime)-1]	= '\0';
	pnote->date			= str_dup(strtime);
	pnote->date_stamp		= current_time;
        
	pnote->next  			= NULL;

	append_note(pnote);

	send_to_char("You have a new note waiting.\n\r", ch);

	ch->bank[C_GOLD] += 25;
    }

    return;
}   



void gain_exp( CHAR_DATA *ch, int gain )
{
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( IS_NPC(ch) || ch->level >= LEVEL_HERO )
	return;

    if ((gain > 0) && (ch->level >= 11) && (!ch->description || (ch->description[0] == '\0')))
    {
	send_to_char("You are required to have a description by the tenth rank.\n\rNo experience rewarded.\n\r", ch);
	return;
    }

    if ((gain > 0) && (ch->level >= 25) && (!ch->pcdata->background || (ch->pcdata->background[0] == '\0')))
    {
	send_to_char("You are required to have a background by the 25th rank.\n\rNo experience rewarded.\n\r", ch);
	return;
    }

    ch->exp = UMAX( 0, ch->exp + gain );
    if (ch->ep < ep_table[ch->level+1]
	&& ch->exp >= exp_on_level(ch, ch->level+1))
	ch->exp = exp_on_level(ch,ch->level+1) - 1;
    while ( ch->level < LEVEL_HERO && ch->exp >= 
        exp_on_level(ch,ch->level+1) )
    {
	send_to_char( "You are promoted!!  ", ch );
	ch->level += 1;
	player_levels++;
	sprintf(buf,"%s gained level %d",ch->name,ch->level);
	log_string(buf);
	sprintf(buf,"$N has attained level %d!",ch->level);
	wiznet(buf,ch,NULL,WIZ_LEVELS,0,0);
	advance_level(ch,FALSE);

	if (ch->familiar)
	{
	   affect_strip(ch, gsn_findfamiliar);
           af.where        = TO_AFFECTS;
           af.type         = gsn_findfamiliar;
           af.level        = ch->level;
           af.duration     = -1;
           af.location     = APPLY_HIT;
           af.modifier     = 4 * ch->level;
           af.bitvector   =  0;
           affect_to_char(ch, &af);

           af.location     = APPLY_MANA;
           af.modifier     = 3 * ch->level;
           affect_to_char(ch, &af);
	}

	save_char_obj(ch);
    }
}

/*
 * Regeneration stuff.
 */
int hit_gain( CHAR_DATA *ch )
{
    double gain;
    int number;
    AFFECT_DATA *paf;

    if (ch->in_room == NULL || is_in_stasis(*ch))
    	return 0;

    if (is_affected(ch, gsn_shadowfiend) && !room_is_dark(ch->in_room))
        return 0;

    if (IS_NAFFECTED(ch, AFF_FLESHTOSTONE) || IS_OAFFECTED(ch, AFF_ENCASE))
        return 0;

    if (IS_OAFFECTED(ch, AFF_NIGHTFEARS) && (ch->position < POS_FIGHTING))
        return 0;

    if ( IS_NPC(ch) )
    {
        if (ch->pIndexData->vnum == MOB_VNUM_CLOCKWORKGOLEM)
            return 0;

    	gain =  5 + ch->level;
 	    if (IS_AFFECTED(ch,AFF_REGENERATION))
	        gain *= 2;

        switch(ch->position)
        {
            case POS_SLEEPING: 
                switch (ch->ticks_slept)
                {
                    case 0: 
                    case 1: gain = (gain * 3) / 2; break;
                    case 2: gain = (gain * 5) / 2; break;
                    default: gain = (gain * 11) / 5; break;
                }
                break;

            case POS_RESTING:   gain = 3 * gain/2;		break;
            case POS_FIGHTING:	gain /= 3;		 	break;
        }
    }
    else
    {
        gain = UMAX(5,get_curr_stat(ch,STAT_CON) + ((ch->level*2)/3)); 
        gain += class_table[ch->class_num].hp_max - 10;
        number = number_percent();
        if (number < get_skill(ch,gsn_fast_healing))
        {
            gain += number * gain / 100;
            if (ch->hit < ch->max_hit)
            check_improve(ch,NULL,gsn_fast_healing,TRUE,8);
        }

        // Check for cryptkin
        bool adjustForPosition(true);
        if (ch->in_room != NULL && ch->in_room->sector_type == SECT_UNDERGROUND && room_is_dark(ch->in_room))
        {
            if (number_percent() <= get_skill(ch, gsn_cryptkin))
            {
                check_improve(ch, NULL, gsn_cryptkin, true, 4);
                adjustForPosition = false;
                gain = (gain * 11) / 10;
            }
            else
                check_improve(ch, NULL, gsn_cryptkin, false, 4);
        }

        // Adjust for character position, if appropriate
        if (adjustForPosition)
        {
            switch (ch->position)
            {
                default:	   	    gain /= 4;			break;
                case POS_SLEEPING: 
                    switch (ch->ticks_slept)
                    {
                        case 0:
                        case 1: gain = gain / 2; break;
                        case 2: gain = (gain * 3) / 2; break;
                        default: gain = (gain * 11) / 10; break;
                    }
                    break;

                case POS_RESTING:  	gain /= 2;			break;
                case POS_FIGHTING: 	gain /= 6;			break;
            }
        }
 
        if ( ch->pcdata->condition[COND_HUNGER]   <= 0 
        && !is_affected(ch,gsn_sustenance))
        {
            if (ch->pcdata->condition[COND_HUNGER] < -300)
            gain /= 10;
            else
                gain /= 2;
        }

        if ( ch->pcdata->condition[COND_THIRST] <= 0
            && !is_affected(ch,gsn_sustenance) )
            gain /= 2;

        if (IS_AFFECTED(ch,AFF_REGENERATION))
            if (get_skill(ch,gsn_regeneration) > 1)
            gain *= 1.5;
            else
            gain *= 2;

        // Check glyph of ulyon
        if (ch->in_room != NULL && room_is_affected(ch->in_room, gsn_glyphofulyon))
        {
            if ((IS_NPC(ch) && IS_SET(ch->act, ACT_UNDEAD)) || is_demon(ch)) gain /= 2;
            else gain = (gain * 11) / 10;
        }
 

        AFFECT_DATA * fugue(get_affect(ch, gsn_fugue));
        if (fugue != NULL && fugue->location != APPLY_NONE)
            gain /= 2;

        if (is_affected(ch, gsn_burnout))
            if (IS_OAFFECTED(ch, AFF_BURNOUT))
            gain *= 3;
            else
            gain /= 2;

        if (is_affected(ch, gsn_psalmofhealing) && can_be_affected(ch,gsn_psalmofhealing))
            gain /= 3;

        if (is_affected(ch, gsn_serenadeoflife) && can_be_affected(ch,gsn_serenadeoflife))
            gain = gain * 2 / 3;

        if (!IS_NPC(ch) && (ch->pcdata->condition[COND_HUNGER] > -300))
            for (paf = ch->affected; paf; paf = paf->next)
                if (paf->type == gsn_accelerated_healing)
                {
                    gain *= 1.4;
                gain_condition(ch, COND_HUNGER, -80);
                }

        if (is_affected(ch, gsn_nightfears) 
            && ch->position == POS_SLEEPING 
            && number_percent() > get_skill(ch,gsn_lucid) )
        {
                damage_old( ch, ch, number_range(1, ch->level), gsn_nightfears, DAM_NONE,TRUE);
            gain /= 10;
        }

        if (ch->pcdata->adrenaline < 3)
            gain *= 2;
    }

    // Dream mastery
    if (ch->position <= POS_SLEEPING)
    {
        if (number_percent() <= get_skill(ch, gsn_dreammastery))
        {
            gain = (gain * 3) / 2;
            check_improve(ch, NULL, gsn_dreammastery, true, 2);
        }
        else
            check_improve(ch, NULL, gsn_dreammastery, false, 2);
    }


    // Warmth calculation
    int warmthSkill = get_skill(ch, gsn_warmth);
    if (warmthSkill > 0)
    {
        // Warmth: every 5% means +1% hp regen, for +20% regen at 100%
        gain += (gain * warmthSkill) / 500;
        if (ch->hit < ch->max_hit)
            check_improve(ch, NULL, gsn_warmth, TRUE, 8);
    }

    // Check grimseep for a +/- 20% bonus
    switch (check_grimseep_affiliation(*ch))
    {
        case Grimseep_None: break;
        case Grimseep_Aided: gain = (gain * 6) / 5; break;
        case Grimseep_Hindered: gain = (gain * 4) / 5; break;
    }

    if (area_is_affected(ch->in_room->area, gsn_deathwalk)
     && (!IS_NPC(ch) || (ch->pIndexData->vnum != MOB_VNUM_ZOMBIE)))
	gain *= 2 / 3;

	if (ch->race == global_int_race_caladaran)
		gain = (gain * 9) / 10;

	if (ch->race == global_int_race_ethron && ch->in_room &&
	(ch->in_room->sector_type == SECT_HILLS || ch->in_room->sector_type == SECT_MOUNTAIN
	|| ch->in_room->sector_type == SECT_FOREST || ch->in_room->sector_type == SECT_SWAMP))
		gain *= 1.1;

	if (ch->race == global_int_race_ethron && ch->in_room &&
	(ch->in_room->sector_type == SECT_INSIDE || ch->in_room->sector_type == SECT_UNDERGROUND))
		gain *= .9;

    if (ch->in_room->sector_type != SECT_UNDERGROUND
     && (silver_state != SILVER_FINAL)
     && (time_info.hour >= season_table[time_info.season].sun_up)
     && (time_info.hour < season_table[time_info.season].sun_down)
     && (ch->race == global_int_race_shuddeni)
     && !room_is_affected(ch->in_room,gsn_dim)
     && !room_is_affected(ch->in_room,gsn_globedarkness))
	gain *= .9;

    gain = gain * ch->in_room->heal_rate / 100;
    
    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
	gain = gain * ch->on->value[3] / 100;

    if (room_is_affected(ch->in_room, gsn_sanctify))
        if (IS_GOOD(ch))
            gain *= 1.2;
        else if (IS_EVIL(ch))
            gain *= .8;

    if (is_affected(ch, gsn_smolder)) gain /= 2;
    if (is_affected(ch, gsn_consume)) gain /= 2;
    if (is_affected(ch, gsn_breathofelanthemir)) gain /= 2;
    if ( IS_AFFECTED(ch, AFF_POISON) ) gain /= 4;
    if (IS_AFFECTED(ch, AFF_PLAGUE)) gain /= 8;
    if (IS_AFFECTED(ch,AFF_HASTE) || IS_AFFECTED(ch,AFF_SLOW)) gain /= 2 ;

    if (ch->position == POS_INCAP)
	gain = 1;
    else if (ch->position == POS_STUNNED)
	gain = 0;

    if ((ch->in_room->area->w_cur.storm_str > 0) && IS_OUTSIDE(ch))
    {
	if (ch->in_room->area->w_cur.precip_type == 0)
	    gain -= (ch->in_room->area->w_cur.storm_str / 3);
	else if (ch->in_room->area->w_cur.precip_type == 1)
	    gain -= (ch->in_room->area->w_cur.storm_str / 5);
	else
	    gain -= (ch->in_room->area->w_cur.storm_str / 10);
	gain = UMAX(gain, 0);
    }

    if (!IS_NPC(ch) && ch->pcdata->karma > 0 && (!(time_info.hour >= season_table[time_info.season].sun_up && time_info.hour < season_table[time_info.season].sun_down)))
	    gain = round(gain * (1 + ch->pcdata->karma / (BLACKAURA * 2.5)));

    return round(UMIN(gain, ch->max_hit - ch->hit));
}

int mana_gain( CHAR_DATA *ch )
{
    double gain;
    int number;
    AFFECT_DATA *song;
    AFFECT_DATA *paf;

    if (ch->in_room == NULL || is_in_stasis(*ch))
        return 0;

    if (is_affected(ch, gsn_shadowfiend) && !room_is_dark(ch->in_room))
        return 0;

    if (IS_NAFFECTED(ch, AFF_FLESHTOSTONE) || IS_OAFFECTED(ch, AFF_ENCASE))
        return 0;

    if (is_quintessence_rushing(ch))
        return 0;

    if (is_an_avatar(ch) || is_affected(ch, gsn_astralprojection) || is_affected(ch, gsn_meldwithstone) 
    || is_affected(ch, gsn_flameunity) || is_affected(ch, gsn_shadowmastery) || IS_OAFFECTED(ch, AFF_POSCHAN) || is_affected(ch, gsn_wariness) || is_affected(ch, gsn_conduitofstonesong)) 
	return 0;

    if (IS_OAFFECTED(ch, AFF_NIGHTFEARS) && (ch->position < POS_FIGHTING))
	return 0;

    if ( IS_NPC(ch) )
    {
        gain = 5 + ch->level;
        switch (ch->position)
        {
            default:		    gain /= 2;		        break;
            case POS_RESTING:				            break;
            case POS_FIGHTING:	gain /= 3;		        break;
            case POS_SLEEPING:  
                switch (ch->ticks_slept)
                {
                    case 0: 
                    case 1: break;
                    case 2: gain = (gain * 37) / 20; break;
                    default: gain = (gain * 33) / 20; break;
                }
                break;
    	}
    }
    else
    {
	gain = ((get_curr_stat(ch,STAT_WIS) 
	      + get_curr_stat(ch,STAT_INT) + ch->level) * 2) / 3;
	number = number_percent();
	if (number < get_skill(ch,gsn_meditation))
	{
	    gain += number * gain / 100;
	    if (ch->mana < ch->max_mana)
	        check_improve(ch,NULL,gsn_meditation,TRUE,8);
	}
    
    // Warmth calculation
    int warmthSkill = get_skill(ch, gsn_warmth);
    if (warmthSkill > 0)
    {
        // Warmth: every 5% means +1% mana regen, for +20% regen at 100%
        gain += (gain * warmthSkill) / 500;
        if (ch->mana < ch->max_mana)
            check_improve(ch, NULL, gsn_warmth, TRUE, 8);
    }
	
    if (class_table[ch->class_num].fMana <= 50)
	    gain /= 2;

	if (ch->race == global_int_race_caladaran)
		gain *= 1.1;

    // Check grimseep for a +/- 40% bonus
    switch (check_grimseep_affiliation(*ch))
    {
        case Grimseep_None: break;
        case Grimseep_Aided: gain = (gain * 7) / 5; break;
        case Grimseep_Hindered: gain = (gain * 3) / 5; break;
    }

    // Check black amulet; adjust for diminishing returns
    // For the first 6 items, use +6%, +5%, +4%, etc (via summation formula)
    // Any items past 6 just add 0.5%; blessed items in excess of evil items are -5% each
    int blackAmuletCount(count_blackamulet(*ch));
    int mod(0);
    static const int SumTo6 = (6 * (6 + 1)); // 42
    if (blackAmuletCount > 6) {mod = (blackAmuletCount - 6); blackAmuletCount = 6;}
    if (blackAmuletCount >= 0) mod += SumTo6 - ((6 - blackAmuletCount) * (7 - blackAmuletCount));
    else mod = 10 * blackAmuletCount;
    gain = (gain * (200 + mod)) / 200;

	if (BIT_GET(ch->pcdata->traits, TRAIT_MARKED) && ch->religion == god_lookup("Chadraln"))
		gain *= 1.01;

	if (ch->race == global_int_race_ethron && ch->in_room &&
	(ch->in_room->sector_type == SECT_HILLS || ch->in_room->sector_type == SECT_MOUNTAIN
	|| ch->in_room->sector_type == SECT_FOREST || ch->in_room->sector_type == SECT_SWAMP))
		gain *= 1.1;

	if (ch->race == global_int_race_ethron && ch->in_room &&
	(ch->in_room->sector_type == SECT_INSIDE || ch->in_room->sector_type == SECT_UNDERGROUND))
		gain *= 0.9;

	if (ch->in_room->sector_type != SECT_UNDERGROUND
         && (silver_state != SILVER_FINAL)
         && (time_info.hour >= season_table[time_info.season].sun_up)
         && (time_info.hour < season_table[time_info.season].sun_down)
         && (ch->race == global_int_race_shuddeni)
	 && !room_is_affected(ch->in_room,gsn_dim)
	 && !room_is_affected(ch->in_room,gsn_globedarkness))
	    gain *= 0.9;

	if (get_skill(ch, gsn_trance) > 0)
	    if (number_percent() < get_skill(ch, gsn_trance))
	    {
		gain *= 1.75;
		check_improve(ch,NULL,gsn_trance,TRUE,8);
	    }
	    else
		check_improve(ch,NULL,gsn_trance,FALSE,8);
    
    // Check for cryptkin
    bool adjustForPosition(true);
    if (ch->in_room != NULL && ch->in_room->sector_type == SECT_UNDERGROUND && room_is_dark(ch->in_room))
    {
        if (number_percent() <= get_skill(ch, gsn_cryptkin))
        {
            check_improve(ch, NULL, gsn_cryptkin, true, 4);
            adjustForPosition = false;
            gain = (gain * 11) / 10;
        }
        else
            check_improve(ch, NULL, gsn_cryptkin, false, 4);
    }

    if (adjustForPosition)
    {
        switch (ch->position)
        {
            default:		    gain /= 4;		break;
            case POS_SLEEPING: 	
                switch (ch->ticks_slept)
                {
                    case 0: 
                    case 1: gain = gain / 2; break;
                    case 2: gain = (gain * 3) / 2; break;
                    default: gain = (gain * 11) / 10; break;
                }
                break;

            case POS_RESTING:	gain /= 2;		break;
            case POS_FIGHTING:	gain /= 6;		break;
        }
    }

	if ( ch->pcdata->condition[COND_HUNGER]   <= 0
		&& !is_affected(ch, gsn_sustenance) )
	{
	    if (ch->pcdata->condition[COND_HUNGER] < -300)
		gain /= 10;
	    else
	        gain /= 2;
	}

	if ( ch->pcdata->condition[COND_THIRST] <= 0
		&& !is_affected(ch, gsn_sustenance) )
	    gain /= 2;

	if (ch->pcdata->adrenaline < 3)
	    gain *= 2;
    }

    if (area_is_affected(ch->in_room->area, gsn_deathwalk)
     && (!IS_NPC(ch) || (ch->pIndexData->vnum != MOB_VNUM_ZOMBIE)))
	gain = gain * 2 / 3;

    gain = gain * ch->in_room->mana_rate / 100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
	gain = gain * ch->on->value[4] / 100;

    if (room_is_affected(ch->in_room, gsn_sanctify))
        if (IS_GOOD(ch))
            gain *= 1.2;
        else if (IS_EVIL(ch))
            gain *= 0.8;

    if ( IS_AFFECTED( ch, AFF_POISON ) )
	gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(ch,AFF_HASTE) || IS_AFFECTED(ch,AFF_SLOW))
        gain /=2 ;

	if (is_affected(ch, gsn_nightfears) 
        && ch->position == POS_SLEEPING 
        && number_percent() > get_skill(ch,gsn_lucid))
		{
		gain /= 10;
		}

    if ((paf = affect_find(ch->affected,gsn_focusmind)) != NULL)
    	gain += (gain * paf->modifier) / 100;

    // Check soul of the wind
    std::pair<Direction::Value, int> windInfo(checkWind(ch));
    if (windInfo.second > 0)
    {
        if (number_percent() <= get_skill(ch, gsn_soulofthewind))
        {
            check_improve(ch, NULL, gsn_soulofthewind, true, 12);
            gain += (gain * windInfo.second) / 150;
        }
        else
            check_improve(ch, NULL, gsn_soulofthewind, false, 12);
    }

    // Check room/area effects
    if (ch->in_room != NULL)
    {
        // Check glyph of ulyon
        if (room_is_affected(ch->in_room, gsn_glyphofulyon))
        {
            if ((IS_NPC(ch) && IS_SET(ch->act, ACT_UNDEAD)) || is_demon(ch)) gain /= 2;
            else gain = (gain * 11) / 10;
        }
        
        // Sunderweave halves mana regen
        if (area_is_affected(ch->in_room->area, gsn_sunderweave))
        {
            if (number_percent() <= get_skill(ch, gsn_weavecraft))
                check_improve(ch, NULL, gsn_weavecraft, true, 2);
            else
                gain /= 2;
        }
    }

    if ((paf = affect_find(ch->affected,gsn_demonic_focus)) != NULL)
        gain += gain * paf->modifier / 100;

    if (is_affected(ch, gsn_ordered_mind))
	gain *= 2;

    if (is_affected(ch, gsn_livingflame))
	gain /= 2;

    if (((paf = affect_find(ch->in_room->affected,gsn_encamp)) != NULL)
      && paf->modifier == ch->id)
	gain *= 1.25;

    if ((song = affect_find(ch->affected, gsn_manasong)) != NULL
      && can_be_affected(ch,gsn_manasong))
	gain = ((gain * (150 + song->level)) / 500);

    if ((ch->in_room->area->w_cur.storm_str > 0) && IS_OUTSIDE(ch))
    {
	if (ch->in_room->area->w_cur.precip_type == 0)
	    gain -= (ch->in_room->area->w_cur.storm_str / 3);
	else if (ch->in_room->area->w_cur.precip_type == 1)
	    gain -= (ch->in_room->area->w_cur.storm_str / 5);
	else
	    gain -= (ch->in_room->area->w_cur.storm_str / 10);
	gain = UMAX(gain, 0);
    }
    
    if (!IS_NPC(ch))
	if (ch->pcdata->karma > 0 && (!(time_info.hour >= season_table[time_info.season].sun_up && time_info.hour < season_table[time_info.season].sun_down)))
	    gain = round(gain * (1 + ch->pcdata->karma / (BLACKAURA * 2.5)));

    return round(UMIN(gain, ch->max_mana - ch->mana));
}

int move_gain( CHAR_DATA *ch )
{
    double gain;

    if (ch->in_room == NULL || is_in_stasis(*ch))
        return 0;

    if (IS_NAFFECTED(ch, AFF_FLESHTOSTONE) || IS_OAFFECTED(ch, AFF_ENCASE))
	return 0;

    if (IS_OAFFECTED(ch, AFF_NIGHTFEARS) && (ch->position < POS_FIGHTING))
	return 0;

    if ( IS_NPC(ch) )
    {
	gain = ch->level;
    }
    else
    {
	gain = UMAX( 15, ch->level );

    // Check for cryptkin
    bool adjustForPosition(true);
    if (ch->in_room != NULL && ch->in_room->sector_type == SECT_UNDERGROUND && room_is_dark(ch->in_room))
    {
        if (number_percent() <= get_skill(ch, gsn_cryptkin))
        {
            check_improve(ch, NULL, gsn_cryptkin, true, 4);
            adjustForPosition = false;
            gain += get_curr_stat(ch,STAT_DEX);
        }
        else
            check_improve(ch, NULL, gsn_cryptkin, false, 4);
    }

    if (adjustForPosition)
    {
        switch (ch->position)
        {
            case POS_SLEEPING: gain += get_curr_stat(ch,STAT_DEX);		break;
            case POS_RESTING:  gain += get_curr_stat(ch,STAT_DEX) / 2;	break;
        }
    }

	if ( ch->pcdata->condition[COND_HUNGER]   <= 0 
		&& !is_affected(ch, gsn_sustenance))
	{
	    if (ch->pcdata->condition[COND_HUNGER] < -300)
		gain /= 10;
	    else
	        gain /= 2;
	}

	if ( ch->pcdata->condition[COND_THIRST] <= 0
		&& !is_affected(ch, gsn_sustenance) )
	    gain /= 2;
	
	if (ch->pcdata->adrenaline < 3)
	    gain *= 2;
    }

    if (area_is_affected(ch->in_room->area, gsn_deathwalk)
     && (!IS_NPC(ch) || (ch->pIndexData->vnum != MOB_VNUM_ZOMBIE)))
	gain *= 2 / 3;

    gain = gain * ch->in_room->move_rate/100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
	gain = gain * ch->on->value[3] / 100;

    if (room_is_affected(ch->in_room, gsn_sanctify))
        if (IS_GOOD(ch))
            gain *= 1.2;
        else if (IS_EVIL(ch))
            gain *= 0.8;

    if ( IS_AFFECTED(ch, AFF_POISON) )
	gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(ch,AFF_HASTE) || IS_AFFECTED(ch,AFF_SLOW))
        gain /=2 ;

	if (is_affected(ch, gsn_nightfears) 
        && ch->position == POS_SLEEPING 
        && number_percent() > get_skill(ch,gsn_lucid))
		{
		gain /= 10;
		}

    if (get_skill(ch, gsn_endurance) > 0)
	if (number_percent() < get_skill(ch, gsn_endurance))
	{
	    gain *= 2;
	    check_improve(ch,NULL,gsn_endurance,TRUE,8);
	}
	else
	    check_improve(ch,NULL,gsn_endurance,FALSE,8);

    if (!IS_AFFECTED(ch, AFF_FLYING) && IS_AFFECTED(ch, AFF_FLY_NATURAL))
	gain = UMAX(1, UMIN(15,ch->level/3));
    
    if (!IS_NPC(ch))
	if (ch->pcdata->karma > 0 && (!(time_info.hour >= season_table[time_info.season].sun_up && time_info.hour < season_table[time_info.season].sun_down)))
	    gain = round(gain * (1 + ch->pcdata->karma / (BLACKAURA * 2.5)));

    if (is_affected(ch, gsn_breathofelanthemir))
        gain = UMIN(gain, 40);

    return round(UMIN(gain, ch->max_move - ch->move));
}



void gain_condition( CHAR_DATA *ch, int iCond, int value )
{
    int condition, dam;
    char buf[MAX_STRING_LENGTH];

    if (ch->desc == NULL)
	return;

    if ( value == 0 || IS_NPC(ch) || ch->level >= LEVEL_IMMORTAL)
	return;

    condition = ch->pcdata->condition[iCond];

    if (iCond == COND_HUNGER) 
    { 
        if ((condition == -2000) 
         || is_affected(ch, gsn_sustenance))
            return;

        if ((value < 1) && is_affected(ch, gsn_livingflame))
            value *= 2;

        if ((value < 0) && BIT_GET(ch->pcdata->traits, TRAIT_ENDURANCE))
            value /= 2;

        ch->pcdata->condition[iCond] += value;
        if (get_modifier(ch->affected,gsn_wrathofthevoid) == 2)
            ch->pcdata->condition[iCond] = UMIN(-299,ch->pcdata->condition[iCond]);

        if (ch->pcdata->condition[iCond] < -1500)
            ch->pcdata->condition[iCond] = -1500;

        // Revenant hunger never drops below 500
        if (ch->pcdata->condition[iCond] < 500 && number_percent() <= get_skill(ch, gsn_revenant))
            ch->pcdata->condition[iCond] = 500;
    }
    else if (iCond == COND_THIRST)
    {
        if ((condition == -101) 
         || is_affected(ch, gsn_sustenance))
            return;

        if ((value < 1) && is_affected(ch, gsn_livingflame))
            value *= 2;

        if (value < 0 && BIT_GET(ch->pcdata->traits, TRAIT_ENDURANCE) && number_bits(1) == 0)
            value = 0;

        ch->pcdata->condition[iCond] += value;
        if (ch->pcdata->condition[iCond] < -100)
            ch->pcdata->condition[iCond] = -100;

        // Revenant thirst never drops below 50
        if (ch->pcdata->condition[iCond] < 50 && number_percent() <= get_skill(ch, gsn_revenant))
            ch->pcdata->condition[iCond] = 50;
    }
    else if (condition == -1)
        return;

    if (((value >= 1) && (iCond == COND_THIRST)) || (iCond == COND_DRUNK))
        ch->pcdata->condition[iCond] = URANGE( 0, condition + value, 48 );

    if (is_affected(ch, gsn_devouringspirit))
    {
        if ((iCond == COND_HUNGER) && (ch->pcdata->condition[COND_HUNGER] > -200))
        {
	    if (value < 1)
	        ch->pcdata->condition[COND_HUNGER] = -200;
	    else
	        ch->pcdata->condition[COND_HUNGER] = UMIN(ch->pcdata->condition[COND_HUNGER], -1);
	}
	else if (iCond == COND_FULL)
	    ch->pcdata->condition[COND_FULL] = UMIN(39, ch->pcdata->condition[COND_FULL]);
    }

    if ( ch->pcdata->condition[iCond] <= 0 && !is_affected(ch, gsn_meldwithstone))
    {
	switch ( iCond )
	{
	case COND_HUNGER:
	    if (ch->level < 6)
		ch->pcdata->condition[iCond] = 60;
	    else
	        if(!is_affected(ch, gsn_sustenance))
	            send_to_char( "You are hungry.\n\r",  ch );
	    break;

	case COND_THIRST:
	    if (ch->level < 6)
		ch->pcdata->condition[iCond] = 60;
	    else
	        if(!is_affected(ch, gsn_sustenance))
  		    send_to_char( "You are thirsty.\n\r", ch );
	    break;

	case COND_DRUNK:
	    if ( condition != 0 )
		send_to_char( "You are sober.\n\r", ch );
	    break;
	}

	if (ch->in_room && (ch->in_room->vnum != 2)
         && (((iCond == COND_THIRST) && (ch->pcdata->condition[iCond] < -15))
           || ((iCond == COND_HUNGER) && (ch->pcdata->condition[iCond] < -300)))
          && (ch->level >= 5))
	{
            if(!is_affected(ch, gsn_sustenance))
	        if (iCond == COND_THIRST)
	            send_to_char ("You are feeling extremely dehydrated!\n\r",ch);
                else
                    send_to_char( "You are starving!\n\r", ch );

            dam = number_range(ch->pcdata->condition[iCond] / -100, ch->level);
            expend_mana(ch, dam);
            if (ch->mana < 0)
	        ch->mana = 0;

            ch->move -= dam;

            if (ch->move < 0)
	        ch->move = 0;
// brazen: Ticket #162: Character deaths to hunger/thirst should be logged
	    if (ch->hit - dam < -10) 
	    {
	        sprintf(buf, "%s died to hunger or thirst in %s [%i].", ch->name, ch->in_room->name, ch->in_room->vnum);
		log_string(buf);
	    }
	    damage_old( ch, ch, dam, TYPE_HIT, DAM_NONE,TRUE);
	}
    }
}

/*
 * track aging. In the end, it was decided that it was lame as hell
 * to have tracks last forever. However, it is actually easier to age
 * the tracks and disallow use of them by the various mortal skills past a
 * certain age than it is to remove them. This function is big, since
 * it cycles through all the rooms, then all the tracks for each room,
 * but it is only called on the tick.
 * 
 * When a person hits a redundant track, it changes the direction,
 * as was previous, and resets this time to 0.
 *
 * /mw
 */
void track_update( void )
{
TRACK_DATA *pTrack, *track_next;
ROOM_INDEX_DATA *pRoomIndex;
int iHash;

for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next)
	{
	if (pRoomIndex->tracks != NULL)
		{
		for (pTrack = pRoomIndex->tracks; pTrack != NULL; pTrack = track_next)
			{
			track_next = pTrack->next;
			pTrack->time++;
			}
		}
	}
    }
}


/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */
void mobile_update( void )
{
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af, naf;
af.valid = TRUE;
 naf.valid = TRUE;
af.point = NULL;
 naf.point = NULL;
    TRACK_DATA *pTrack = NULL;
    CHAR_DATA *ch = NULL;
    CHAR_DATA *ch_next = NULL;
    EXIT_DATA *pexit = NULL;
    OBJ_DATA *obj = NULL, *obj_next = NULL;
    int door=6;
    ROOM_INDEX_DATA *pRoomIndex = NULL, *room_next = NULL;

/* Obj prog moved here so they can happen at a normal pace */

    for (obj = obj_rand_first; obj != NULL; obj = obj_next)
    {
	obj_next = obj->next_rand;
        oprog_random_trigger(obj);
    }

    for (pRoomIndex = room_rand_first; pRoomIndex; pRoomIndex = room_next)
    {
        room_next = pRoomIndex->next_rand;
        rprog_random_trigger(pRoomIndex);        
   }

    /* Examine all mobs. */
    for ( ch = char_list; ch != NULL; ch = ch_next )
    {
        ch_next = ch->next;
        if (is_in_stasis(*ch))
            continue;

	if (ch->position == POS_FIGHTING && ch->fighting == NULL)
		switch_position(ch, POS_STANDING);

	if (!IS_NPC(ch) && IS_OAFFECTED(ch, AFF_GHOST) && is_affected(ch, gsn_phoenixfire))
	{
	    affect_strip(ch, gsn_ghost);
	    affect_strip(ch, gsn_phoenixfire);
	}
	
        if (IS_NPC(ch) && ch->pIndexData->vnum == MOB_VNUM_MIRROR_IMAGE
	  && !is_affected(ch,gsn_doppelganger))
	{
	    act("$n turns translucent and disappears.",ch,NULL,NULL,TO_ROOM);
	    extract_char(ch,TRUE);
	    continue;
	}

	if (IS_NPC(ch) && ch->pIndexData->vnum == MOB_VNUM_SEEDLING_GAMALOTH
	  && !is_affected(ch,gsn_seedofgamaloth))
	{
	    act("$n falls to the ground, decaying rapidly into nothingness.",ch,NULL,NULL,TO_ROOM);
	    extract_char(ch,TRUE);
	    continue;
	}

	if ( ch->guarding != NULL && ch->in_room && ch->guarding->in_room
	&& ch->in_room != ch->guarding->in_room)
	{
	    act("You stop protecting $N.", ch, NULL, ch->guarding, TO_CHAR);
	    act("$N stops protecting you.", ch->guarding, NULL, ch, TO_CHAR);
	    ch->guarding = NULL;
	}

	if (IS_NPC(ch) && ch->in_room && (ch->pIndexData->vnum == MOB_VNUM_SHUNNED_DEMON) && (silver_state != SILVER_FINAL) && (ch->in_room->sector_type != SECT_UNDERGROUND) && (time_info.hour >= season_table[time_info.season].sun_up) && (time_info.hour < season_table[time_info.season].sun_down))
	{
	    send_to_char("Unable to withstand the searing rays of the sun, your demonic form dissipates.\n\r", ch);
	    unbind_shunned_demon(ch);
	    extract_char(ch, TRUE);
	    continue;
	}

        if (!IS_NPC(ch))
	{
	    if (ch->in_room && (silver_state == SILVER_FINAL) && (number_bits(4) == 0) && (ch->level >= 43) && !IS_IMMORTAL(ch) && (ch->clan != clan_lookup("SHUNNED")))
	    {
		CHAR_DATA *demon = create_mobile(get_mob_index(number_range(MOB_VNUM_SILVER_DEMON_MIN, MOB_VNUM_SILVER_DEMON_MAX)));

		if (demon)
		{
		    char_to_room(demon, ch->in_room);
		    act("A dark line tears open in mid-air, extending into an inky black portal.", demon, NULL, NULL, TO_ROOM);
		    act("$n steps out of the portal, glaring about angrily.", demon, NULL, ch, TO_ROOM);
		    act("$n's eyes lock on $N, and it grins darkly.", demon, NULL, ch, TO_NOTVICT);
		    act("$n's eyes lock on you, and it grins darkly.", demon, NULL, ch, TO_VICT);
		    act("{r$n hisses 'Are you prepared to die, mortal?'{x", demon, NULL, NULL, TO_ROOM);
		    act("$n screams and attacks!", demon, NULL, NULL, TO_ROOM);
		    multi_hit(demon, ch, TYPE_UNDEFINED);
		}
		else
		    bug("Error creating veil demon.", 0);
		
		if (IS_OAFFECTED(ch, AFF_GHOST))
		    continue;
	    }

	    if (IS_OAFFECTED(ch, AFF_PARANOIA) && (number_bits(5) == 0))
	    {
		switch (number_range(0, 3))
		{
		    case 0:
			send_to_char("Someone steps out of the shadows.\n\r", ch);
			break;
		    case 1:
			send_to_char("Someone knocks you on the back of the head.\n\r", ch);
			break;
		    case 2:
			send_to_char("Someone steps out of the shadows.\n\rSomeone's dart {rmisses{x you!\n\r", ch);
			break;
		    case 3:
			send_to_char("Someone utters the words, in arcane, 'grzzs.'\n\rYou feel very sleepy ..... zzzzzz.\n\r", ch);
		}
	    }

	    if (IS_NAFFECTED(ch, AFF_PURSUE))
	   	if (ch->tracking)
		{
	    	    if (!do_pctrackstep(ch, ""))
	    	    {
	      		send_to_char("You are unable to find further tracks.\n\r", ch);
	      		affect_strip(ch, gsn_pursue);
	      		ch->tracking = NULL;
	    	    }
	    	    else
			act("You continue to pursue $N.", ch, NULL, ch->tracking, TO_CHAR);
	  	}
	        else
	  	{
	   	    affect_strip(ch, gsn_pursue);
	   	    ch->tracking = NULL;
	  	}

	    if (IS_NAFFECTED(ch, AFF_TREMBLE)
	    &&  number_percent() < get_modifier(ch->affected, gsn_tremblingpoison)
	    &&  ch->in_room)
	    {
		for (obj = ch->carrying; obj; obj = obj_next)
		{
		    obj_next = obj->next_content;

		    if (obj->worn_on && (number_bits(2) == 0)
		     && !IS_SET(obj->worn_on, WORN_SIGIL)
		     && !IS_SET(obj->worn_on, WORN_PROGSLOT)
             && !IS_SET(obj->worn_on, WORN_FAMILIAR))
		    {
	      	        act("You shake and tremble, and drop $p.", ch, obj, NULL, TO_CHAR);
	                act("$n shakes and trembles, and drops $p.", ch, obj, NULL, TO_ROOM);
	                obj_from_char(obj);
	                obj_to_room(obj, ch->in_room);
			break;
		    }
		}
	    }

	    if (is_affected(ch, gsn_agony) && number_bits(10) == 0)
	    {
		sprintf(buf, "Help! The pain is unbearable!");
		do_yell(ch, buf);
	    }
	}

	if (room_is_affected(ch->in_room, gsn_smoke) && number_bits(2) == 0)
        checkApplySmoke(ch->level, ch);

    // Check for bloodpyre cooldown
    AFFECT_DATA * pyreAff(get_affect(ch, gsn_bloodpyre));
    if (pyreAff != NULL && pyreAff->point != NULL)
        static_cast<PyreInfo*>(pyreAff->point)->checkCooldownTime(ch);

    // Check for the echo affect
    AFFECT_DATA * echoAff(get_affect(ch, EchoAffect::GSN));
    if (echoAff != NULL && echoAff->point != NULL)
    {
        // Handle this pulse and possibly strip the affect
        if (static_cast<EchoAffect*>(echoAff->point)->HandleNext(ch))
            affect_remove(ch, echoAff);
    }

    // Check for bedrock roots and mudfoot curse
    check_bedrockroots(*ch);
    check_mudfootcurse(*ch);
    if (!IS_VALID(ch))
        continue;

    // Check for salt of the earth healing
    if (ch->hit < ch->max_hit)
    {
        int saltLevel(determine_saltoftheearth_level(*ch, SPH_WATER));
        if (saltLevel > 0)
        {
            ch->hit += (saltLevel / 10) + 1;
            ch->hit = UMIN(ch->hit, ch->max_hit);
        }
    }

    // Check for fadeshroud mana cost
    if (is_affected(ch, gsn_fadeshroud))
        expend_mana(ch, skill_table[gsn_fadeshroud].min_mana);

    // Check for clockwork soul
    AFFECT_DATA * clockworkSoul(get_affect(ch, gsn_clockworksoul));
    if (clockworkSoul != NULL && clockworkSoul->modifier > 0)
    {
        if (deal_clockworksoul_damage(*ch, *clockworkSoul, 10))
            continue;
    }

    // Check for wellspring
    if (ch->move < ch->max_move && number_bits(1) == 0)
    {
        AFFECT_DATA * wellspring(get_affect(ch, gsn_wellspring));
        if (wellspring != NULL)
        {
            send_to_char("Your weariness recedes as the wellspring's pure waters refresh you.\n", ch);
            ch->move += dice(4, UMAX(1, wellspring->level / 4));
            ch->move = UMIN(ch->max_move, ch->move);
        }
    }

    // Check for boil seas
    if (ch->in_room != NULL)
    {
        AFFECT_DATA * boilseas(get_room_affect(ch->in_room, gsn_boilseas));
        if (number_bits(1) == 0 && boilseas != NULL
        && (ch->in_room->sector_type == SECT_UNDERWATER || !is_flying(ch) || is_affected(ch, gsn_earthbind))
        && (!IS_NPC(ch) || !IS_SET(ch->nact, ACT_SHADE)))
        {
            send_to_char("You are scalded by the boiling water!\n", ch);

            int effectiveLevel(boilseas->level);
            if (!IS_NPC(ch) && ch->level < 25)
                effectiveLevel = UMIN(effectiveLevel, ch->level);

            int dam(dice(effectiveLevel, 4));
            if (boilseas->modifier == ch->id)
                dam /= 3;

            sourcelessDamage(ch, "the water", dam, gsn_boilseas, DAM_FIRE);
        }
    }

    // Check for maelstrom
    if (ch->in_room != NULL)
    {
        AFFECT_DATA * maelstrom(get_room_affect(ch->in_room, gsn_maelstrom));
        if (number_bits(1) == 0 && maelstrom != NULL && !is_flying(ch) 
        && (!IS_NPC(ch) || (!IS_SET(ch->act, ACT_NOSUBDUE) && !IS_SET(ch->act, ACT_SENTINEL) && !IS_SET(ch->nact, ACT_SHADE))))
        {
            if (is_affected(ch, gsn_aquamove) || number_percent() <= get_skill(ch, gsn_waveborne))
            {
                send_to_char("The raging maelstrom churns all about, battering you forcefully!\n", ch);
                damage(ch, ch, number_range(2, 7), gsn_maelstrom, DAM_BASH, true);
            }
            else
            {
                send_to_char("The churning water seizes you bodily, slamming you around!\n", ch);
                damage(ch, ch, number_range(3, 15), gsn_maelstrom, DAM_BASH, true);

                if (!saves_spell(maelstrom->level, NULL, ch, DAM_BASH))
                    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));

                // Grab a random room exit; must check again for null room because the damage could have killed the target
                if (ch->in_room != NULL && number_bits(1) == 0)
                {
                    std::vector<Direction::Value> doors(Direction::ValidDirectionsFrom(*ch->in_room));
                    while (!doors.empty())
                    {
                        Direction::Value door(doors[number_range(0, doors.size() - 1)]);
                        ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*ch->in_room, door));
                        if (nextRoom != NULL && is_water_room(*nextRoom))
                        {
                            // Found a random water room to throw the char into
                            act("You are thrown forcefully through the water!", ch, NULL, NULL, TO_CHAR);
                            act("$n is thrown forcefully through the water!", ch, NULL, NULL, TO_ROOM);
                            move_char(ch, door, false);
                            break;
                        }
                    
                        // Not a valid room, remove it from the possibility set
                        doors[door] = doors[doors.size() - 1];
                        doors.pop_back();
                    }
                }
            }
        }
    }

    // Check for unreal incursion
    if (number_percent() <= 5 && is_affected(ch, gsn_unrealincursion))
    {
        std::ostringstream mess;
        mess << generate_unreal_entity() << " " << generate_unreal_predicate() << " ";
        switch (number_range(0, 7))
        {
            case 0: mess << "across your vision"; break;
            case 1: mess << "by"; break;
            case 2: mess << "past you"; break;
            case 3: mess << "below you"; break;
            case 4: mess << "around you"; break;
            case 5: mess << "into the ground"; break;
            case 6: mess << "into the air"; break;
            default: mess << "above you"; break;
        }

        mess << ".\n";
        std::string result(mess.str());
        result[0] = UPPER(result[0]);
        send_to_char(result.c_str(), ch);
    }

    // Check for sparking cloud
    if (ch->in_room != NULL && number_bits(1) == 0 && !IS_AFFECTED(ch, AFF_WIZI) && (!IS_NPC(ch) || !IS_SET(ch->nact, ACT_SHADE)))
    {
        AFFECT_DATA * sparking(get_room_affect(ch->in_room, gsn_sparkingcloud));
        if (sparking != NULL)
        {
            // Echoes
            act("A bolt of lightning arcs from the sparking cloud, shocking you!", ch, NULL, NULL, TO_CHAR);
            act("A bolt of lightning arcs from the sparking cloud, shocking $n!", ch, NULL, NULL, TO_ROOM);

            // Damage
            int effLevel(sparking->level);
            if (ch->level < 25) effLevel = UMIN(effLevel, ch->level);
            int dam(number_range(effLevel, effLevel * 3));
            if (saves_spell(effLevel, NULL, ch, DAM_LIGHTNING)) dam /= 2;
            sourcelessDamage(ch, "the lightning", dam, gsn_sparkingcloud, DAM_LIGHTNING);

            // Chance of item destruction
            if (IS_VALID(ch) && get_charging_effect(ch) == NULL && !saves_spell(effLevel, NULL, ch, DAM_LIGHTNING) && number_bits(2) == 0)
            {
                // Build a list of candidate objs
                std::vector<OBJ_DATA*> objs;
                for (OBJ_DATA * obj(ch->carrying); obj != NULL; obj = obj->next_content)
                {
                    if (obj->worn_on && !IS_OBJ_STAT(obj, ITEM_BURN_PROOF) 
                    && !IS_OBJ_STAT(obj, ITEM_NODESTROY) && !IS_OBJ_STAT(obj, ITEM_NOPURGE))
                        objs.push_back(obj);
                }

                // Find a random obj
                if (!objs.empty())
                {
                    OBJ_DATA * obj(objs[number_range(0, objs.size() - 1)]);
                    act("$p is fused into a worthless lump!", ch, obj, NULL, TO_CHAR);
                    act("$p on $n is fused into a worthless lump!", ch, obj, NULL, TO_ROOM);
                    extract_obj(obj);
                }
            }
        }
    }

    // Check for windiness movement
    if (ch->in_room != NULL && !IS_NPC(ch) && IS_OUTSIDE(ch) && ch->position != POS_FIGHTING 
    && !IS_SET(ch->in_room->room_flags,ROOM_NOWEATHER) && is_flying(ch)
    && !IS_AFFECTED(ch, AFF_WIZI) && !is_affected(ch, gsn_anchor)
    && (!IS_NPC(ch) || (!IS_SET(ch->act, ACT_NOSUBDUE) && !IS_SET(ch->act, ACT_SENTINEL) && !IS_SET(ch->nact, ACT_SHADE))))
    {
        Direction::Value direction(static_cast<Direction::Value>(ch->in_room->area->w_cur.wind_dir));
        ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*ch->in_room, direction, EX_CLOSED|EX_WALLED|EX_WEAVEWALL|EX_ICEWALL));
        if (nextRoom != NULL)
        {
            int chance(ch->in_room->area->w_cur.wind_mag - 60);
            chance /= 10;
            if (number_percent() <= chance)
            {
                if (number_percent() <= get_skill(ch, gsn_controlledflight))
                {
                    check_improve(ch, NULL, gsn_controlledflight, true, 4);
                    send_to_char("The winds buffet you, but you fly through them with skill.\n", ch);
                }
                else
                {
                    check_improve(ch, NULL, gsn_controlledflight, false, 4);
                    
                    // Echoes
                    std::ostringstream mess;
                    mess << "The stiff winds blow you " << Direction::NameFor(direction) << "!\n";
                    send_to_char(mess.str().c_str(), ch);

                    mess.str("");
                    mess << "The stiff winds catch $n, blowing $m " << Direction::NameFor(direction) << "!\n";
                    act(mess.str().c_str(), ch, NULL, NULL, TO_ROOM);

                    // Move the char
                    char_from_room(ch);
                    char_to_room(ch, nextRoom);
                    do_look(ch, "auto");

                    mess.str("");
                    mess << "$n is blown in " << Direction::SourceNameFor(Direction::ReverseOf(direction)) << "!";
                    act(mess.str().c_str(), ch, NULL, NULL, TO_ROOM);
                }
            }
        }
    }

    // Check for twister damage/lag
    if (ch->in_room != NULL && !IS_AFFECTED(ch, AFF_WIZI) && (!IS_NPC(ch) || !IS_SET(ch->nact, ACT_SHADE)))
    {
        ROOM_INDEX_DATA * initialRoom(ch->in_room);
        for (AFFECT_DATA * twister(get_room_affect(initialRoom, gsn_unleashtwisters)); twister != NULL && ch->in_room == initialRoom; twister = get_room_affect(initialRoom, gsn_unleashtwisters, twister))
        {  
            if (number_bits(1) == 0)
                continue;
                 
            // Check for windrider 
            if (is_flying(ch))
            {
                if (number_percent() <= get_skill(ch, gsn_windrider))
                {
                    check_improve(ch, NULL, gsn_windrider, true, 4);
                    send_to_char("The raging twister tears at you, but you ride it with skill, dancing easily through the whipping winds!\n", ch);
                    continue;
                }
                else
                    check_improve(ch, NULL, gsn_windrider, false, 4);
            }

            // Check for save
            if (saves_spell(twister->level, NULL, ch, DAM_BASH) || is_affected(ch, gsn_anchor))
            {
                send_to_char("The raging twister tears at you, but you stand firm against it!\n", ch);
                continue;
            }

            // Damage and lag
            send_to_char("You are grabbed by the whirling winds and hurled bodily about!\n", ch);
            act("$n is seized by the whirling winds and hurled bodily about!", ch, NULL, NULL, TO_ROOM);

            int effLevel(twister->level);
            if (ch->level < 25) 
                effLevel = UMIN(effLevel, ch->level);

            damage(ch, ch, number_range(effLevel / 2, effLevel * 2), gsn_unleashtwisters, DAM_BASH, true);
            WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
        }
    }

    // Check for displacement
    check_displacement(ch);

	if (IS_OAFFECTED(ch, AFF_INSCRIBE))
	{
	    AFFECT_DATA *paf;
	    OBJ_DATA *symbol;
	    int mod, stype;
            bool draw_symbol = FALSE;
	    char matstr[30];

	    for (paf = ch->affected; paf; paf = paf->next)
		if (paf->type == gsn_inscribe)
		    break;

	    mod = paf->modifier;
	    stype = paf->level;

	    switch(stype)
	    {
		case INSMAT_SILVER:
		    strcpy(matstr, "silver dust");
		    break;

		case INSMAT_BONEDUST:
		case INSMAT_BONES_UNDEAD:
		case INSMAT_BONES_DRAGON:
		    strcpy(matstr, "powdered bone");
		    break;

		case INSMAT_CHARCOAL:
		    strcpy(matstr, "charcoal");
		    break;

		case INSMAT_BLOOD_ANIMAL:
		case INSMAT_BLOOD_UNDEAD:
		case INSMAT_BLOOD_ALATHARYA:
		case INSMAT_BLOOD_CELESTIAL:
		case INSMAT_BLOOD_CASTER:
		case INSMAT_BLOOD_CHTAREN:
		    strcpy(matstr, "blood");
		    break;

		case INSMAT_SALT:
		    strcpy(matstr, "salt");
		    break;

		case INSMAT_MUD:
		    strcpy(matstr, "mud");
		    break;

		case INSMAT_HERB1:
		    strcpy(matstr, "paste made from black yarrow");
		    break;

		case INSMAT_HERB2:
		    strcpy(matstr, "bitterthorn vine");
		    break;

		case INSMAT_HERB3:
		    strcpy(matstr, "powdered Uzith-Hazhi dust");
		    break;

		default:
		    strcpy(matstr, "an unknown material");
		    break;
	    }

	    switch (mod / MAX_INSCRIBE_ECHOES)
	    {
		case 0:   /* Trigon of Binding */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "Using a bit of %s, you carefully inscribe a large equilateral triangle upon the ground.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "Using a bit of %s, $n carefully inscribes a large equilateral triangle upon the ground.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
		        send_to_char("At the north vertex, you inscribe a rune of the void.\n\r", ch);
		        act("At the north vertex, $n inscribes a rune of the void.", ch, NULL, NULL, TO_ROOM);
		        paf->modifier++;
		        break;

		    case 2:
		        send_to_char("At the east vertex, you inscribe a rune of order.\n\r", ch);
		        act("At the east vertex, $n inscribes a rune of order.", ch, NULL, NULL, TO_ROOM);
		        paf->modifier++;
		        break;

		    case 3:
		        send_to_char("At the west vertex, you inscribe a rune of will.\n\r", ch);
		        act("At the west vertex, $n inscribes a rune of will.", ch, NULL, NULL, TO_ROOM);
		        paf->modifier++;
		        break;

		    case 4:
		        send_to_char("You inscribe a circle within the interior of the triangle.\n\r", ch);
		        act("$n inscribes a circle within the interior of the triangle.", ch, NULL, NULL, TO_ROOM);
		        paf->modifier++;
		        break;

		    case 5:
		        send_to_char("You complete the trigon by inscribing a rune of binding within.\n\r", ch);
		        act("$n inscribes a rune of binding within, completing the trigon.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;

		}
		break;

		case 1:  /* Pentagram of Summoning */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "You carefully begin to outline a circle in %s.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n carefully begins to outline a circle in %s.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
		        send_to_char("You inscribe a second circle within the first.\n\r", ch);
		        act("$n inscribes a second circle within the first.", ch, NULL, NULL, TO_ROOM);
		        paf->modifier++;
		        break;

		    case 2:
		        send_to_char("Following the circumference of the symbol, you begin inscribing the runes of the ancient demon lords in the area between the inner and outer circles.\n\r", ch);
		        act("Following the circumference of the symbol, $n begins inscribing the runes of the ancient demon lords in the area between the inner and outer circles.", ch, NULL, NULL, TO_ROOM);
		        paf->modifier++;
		        break;

		    case 3:
		        send_to_char("You finish with the thirteenth sign, the symbol of Ashur, Dragon of the Void.\n\r", ch);
		        act("$n completes the runes with the symbol of Ashur, Dragon of the Void.", ch, NULL, NULL, TO_ROOM);
		        paf->modifier++;
		        break;

		    case 4:
		        send_to_char("In the interior of the circle, you inscribe the lines of a pentagram.\n\r", ch);
		        act("In the interior of the circle, $n inscribes the lines of a pentagram.", ch, NULL, NULL, TO_ROOM);
		        paf->modifier++;
		        break;

		    case 5:
		        send_to_char("You utter a word of power to complete the inscription, and the pentagram of summoning begins to glow with an eerie light.\n\r", ch);
		        act("$n utters a word of power, and the pentagram begins to flow with an eerie light.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
		        break;
		}
		break;

		case 2:  /* Circle of Protection */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "You carefully inscribe a large circle of %s upon the ground.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n carefully inscribes a large circle of %s upon the ground.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
		        send_to_char("You inscribe two perpendicular lines, dividing the circle into four equal quadrants.\n\r", ch);
		        act("$n inscribes two perpendicular lines, diving the circle into four equal quadrants.", ch, NULL, NULL, TO_ROOM);
		        paf->modifier++;
		        break;

		    case 2:
		        send_to_char("You draw a rune of warding within the northeast section.\n\r", ch);
		        act("$n draws a rune of warding within the northeast section.", ch, NULL, NULL, TO_ROOM);
		        paf->modifier++;
		        break;

		    case 3:
		        send_to_char("You draw a rune of shielding within the northwest section.\n\r", ch);
		        act("$n draws a rune of shielding within the northwest section.", ch, NULL, NULL, TO_ROOM);
		       paf->modifier++;
		        break;

		    case 4:
		        send_to_char("You draw a rune of strength within the southwest section.\n\r", ch);
		        act("$n draws a rune of strength within the southwest section.", ch, NULL, NULL, TO_ROOM);
		        paf->modifier++;
		        break;

		    case 5:
		        send_to_char("You draw a rune of protection within the southeast section.\n\r", ch);
		        act("$n draws a rune of protection within the southeast section.", ch, NULL, NULL, TO_ROOM);
		        paf->modifier++;
		        break;

		    case 6:
		        send_to_char("You inscribe a small circle, concentric with the first.\n\r", ch);
		        act("$n inscribes a small circle, concentric with the first.", ch, NULL, NULL, TO_ROOM);
		        paf->modifier++;
		        break;

		    case 7:
		        send_to_char("Within the second circle you inscribe a rune of power, completing the symbol.\n\r", ch);
		        act("Within the second circle $n inscrbies a rune of power, completing the symbol.", ch, NULL, NULL, TO_ROOM);
		        draw_symbol = TRUE;
			break;
		}
		break;

		case 3: /* Maze of Isetaton */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "Starting with a stroke of %s, you begin drawing a radiating maze of harshly-angled lines.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "Starting with a stroke of %s, $n begins drawing a radiating maze of harshly-angled lines.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("The maze extends outward, its countless pathways quickly becoming too complex to follow.\n\r", ch);
			act("The maze extends outward, its countless pathways quickly becoming too complex to follow.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("You finally trace through the Maze of Isetaton, placing small, dire-looking runes at the nexuses, almost like traps.\n\r", ch);
			act("$n finally traces through the Maze of Isetaton, placing small, dire-looking runes at the nexuses, almost like traps.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 4: /* Blasphemous Sigil of Nygothua */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "You begin the careful inscription of a single rune in %s.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n begins the careful inscription of a single rune in %s.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("The rune takes shape as a branching step, bisected with lesser runes of Void.\n\r", ch);
			act("The rune takes shape as a branching step, bisected with lesser runes of Void.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("The Blasphemous Sigil of Nygothua is completed with the Rune of the Arcanum, placed beneath the whole.\n\r", ch);
			act("$n completes the Blasphemous Sigil of Nygothua, with the Rune of the Arcanum placed beneath the whole.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 5: /* The Spiral of Bahhaoth */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "Starting with a single, sinuous glyph in %s, you begin scribing a symbol of power.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "Starting with a single, sinuous glyph in %s, $n begins scribing a symbol of power.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("More glyphs are added to the first, forming a widening spiral of twisting runes.\n\r", ch);
			act("More glyphs are added to the first, forming a widening spiral of twisting runes.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("You finish the last of one hundred blasphemous runes, completing the foul Spiral of Bahhaoth.\n\r", ch);
			act("$n finishes the last of one hundred blasphemous runes, completing the foul Spiral of Bahhaoth.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 6: /* Logorin Star */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "You carefully draw a skewed septagon on the ground in %s.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n carefully draws a skewed septagon on the ground in %s.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("Bisecting the septagon with dozens of precisely-placed, assymetric lines, you complete the warped Logorin Star.\n\r", ch);
			act("Bisecting the septagon with dozens of precisely-placed, assymetric lines, $n completes the warped Logorin Star.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 7: /* Angles of Selb-Kar */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			send_to_char("You kneel on the ground, and begin tracing out an unsettling array of vertices and lines, each forming a disturbing angle.\n\r", ch);
			act("$n kneels on the ground, and begins tracing out an unsettling array of vertices and lines, each forming a disturbing angle.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			sprintf(buf, "Unerring in creation of the figures drawn in %s, when done, you inscribe a tiny rune in each and every angle.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "Unerring in creation of the figures drawn in %s, when done, $n inscribes a tiny rune in each and every angle.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("When done, the lines and vertices jut upwards from the ground, their angles an obscene artifact of some alien geometry.\n\r", ch);
			act("When done, the lines and vertices jut upwards from the ground, their angles an obscene artifact of some alien geometry.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 8: /* Seal of the Dragon */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "You begin the inscription of a set of many-layered polygons, drawn in %s.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n begins the inscription of a set of many-layered polygons, drawn in %s.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;
		
		    case 1:
			send_to_char("After carefully constructing a pentagram atop these, you place elemental symbols at each point.\n\r", ch);
			act("After carefully constructing a pentagram atop these, $n places elemental symbols at each point.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("You complete the Seal of the Dragon with a harsh, black Dragon Rune in the center.\n\r", ch);
			act("$n completes the Seal of the Dragon with a harsh, black Dragon Rune in the center.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 9: /* The Eye of Xthjich */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "You outline a hasty pentagram in %s.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n outlines a hasty pentagram in %s.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("In the center of the pentagram, you work the image of an eye, glaring and scored with jagged lines.\n\r", ch);
			act("In the center of the pentagram, $n works the image of an eye, glaring and scored with jagged lines.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("Completing the symbol, you surround the Eye of Xthjich with radiating spikes and licks of stylized flame.\n\r", ch);
			act("Completing the symbol, $n surrounds the Eye of Xthjich with radiating spikes and licks of stylized flame.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 10: /* The Mark of the Damned */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "You score three harsh parallel marks on the ground in %s.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n scores three harsh parallel marks on the ground in %s.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("Surrounding the triplet, you inscribe a large, eight-sided star.\n\r", ch);
			act("Surrounding the triplet, $n inscribes a large, eight-sided star.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("You complete the Mark of the Damned with a series of cursed glyphs and unwholesome runes, filling the points of the star.\n\r", ch);
			act("$n completes the Mark of the Damned with a series of cursed glyphs and unwholesome runes, filling the points of the star.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 11: /* Tetragon of the Dead */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "With thick strokes of %s, you mark out a concave tetragon.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "With thick strokes of %s, $n marks out a concave tetragon.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("Now with a series of four-sided stars, interlaced diamonds and graven angles, you work the symbol inward.\n\r", ch);
			act("Now with a series of four-sided stars, interlaced diamonds and graven angles, $n works the symbol inward.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("At the very center of the symbol, you scribe a pointed cross, and so casts the Tetragon of the Dead.\n\r", ch);
			act("At the very center of the symbol, $n scribes a pointed cross, and so casts the Tetragon of the Dead.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 12: /* Vakalic Sign */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "You inscribe a small circle on the ground in %s.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n inscribes a small circle on the ground in %s.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("With extreme precision, you trace a trio of strange glyphs within the circle, completing the Vakalic Sign.\n\r", ch);
			act("With extreme precision, $n traces a trio of strange glyphs within the circle, completing the Vakalic Sign.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 13: /* The Lost Cipher of Pnakur */
		switch(mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "Carefully clearing the ground of dust and debris, you begin an inscription in %s.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "Carefully clearing the ground of dust and debri, $n begins an inscription in %s.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("You inscribe line after line in a strange, flowing script of incredible complexity.\n\r", ch);
			act("$n inscribes line after line in a strange, flowing script of incredible complexity.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("The Lost Cipher of Pnakur is completed as you wipe you hand over the whole, obliterating any meaning it held.\n\r", ch);
			act("The Lost Cipher of Pnakur is completed as $n wipes $s hand over the whole, obliterating any meaning it held.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 14: /* The Vkoren Configuration */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			send_to_char("You mark a series of points on the ground in a confusing, abstract array.\n\r", ch);
			act("$n marks a series of points on the ground in a confusing, abstract array.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			sprintf(buf, "With fine lines of %s, you connect the points with a series of overlapping triangles.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "With fine lines of %s, $n connects the points with a series of overlapping triangles.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("The Vkoren Configuration is completed as an impossible structure of interlocking triangles, suggestive of dimensions deeper than the familiar three.\n\r", ch);
			act("The Vkoren Configuration is completed as an impossible structure of interlocking triangles, suggestive of dimensions deeper than the familiar three.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 15: /* The Sigil of Logor */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "You apply strokes of %s to the ground in an uneven, upright pentagon.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n applies strokes of %s to the ground in an uneven, upright pentagon.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			sprintf(buf, "You proceed to completely blot out the center of the pentagon with %s.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n proceeds to completely blot out the center of the pentagon with %s.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("Using bare fingers, you mark out the Dragon Rune, atop a smaller Rune of Void, completing the Sigil of Logor.\n\r", ch);
			act("Using bare fingers, $n marks out the Dragon Rune, atop a smaller Rune of Void, completing the Sigil of Logor.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 16: /* The Scar of Gagaroth */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "With wide, heavy strokes of %s, you begin the drawing of a symbol of power.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "With wide, heavy strokes of %s, $n begins the drawing of a symbol of power.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("The symbol takes shape as a you inscribe a dark scar across the ground, radiating tapering arcs like creeping wounds.\n\r", ch);
			act("The symbol takes shape as $n inscribes a dark scar across the ground, radiating tapering arcs like creeping wounds.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("With a last, emphatic smear, you finish inscribing the Scar of Gagaroth.\n\r", ch);
			act("With a last, emphatic smear, $n finishes inscribing the Scar of Gagaroth.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 17: /* The Tear of Pricina */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "You apply a sinuous smear of %s to the ground with an almost caressing touch.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n applies a sinuous smear of %s to the ground with an almost caressing touch.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("A few lithsome arcs follow the first, forming a sensual, tear-shaped symbol.\n\r", ch);
			act("A few lithsome arcs follow the first, forming a sensual, tear-shaped symbol.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			sprintf(buf, "The Tear of Pricina is finished as you raise you hand, dabbing %s to your lips.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "The Tear of Pricina is finished as $n raises $s hand, dabbing %s to $s lips.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 18: /* The Mad Etchings of Kyalee */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "Using %s, you trace out a few erratic glyphs on the ground.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "Using %s, $n traces out a few erratic glyphs on the ground.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("A sudden frenzy overtakes you, and you begins to cover the area in a chaotic array of insane scrawlings.\n\r", ch);
			act("A sudden frenzy overtakes $n, and $e begins to cover the area in a chaotic array of insane scrawlings.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;
		
		    case 2:
			send_to_char("Your frenzy calming, you complete the Mad Etchings of Kyalee.\n\r", ch);
			act("$s frenzy calming, $n completes the Mad Etchings of Kyalee.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 19: /* The Cracks of Xixzyr */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "With a strained intensity, you mark out a single, jagged line in %s.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "With a strained intensity, $n marks out a single, jagged line in %s.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("The line is followed by six more parallel to it, each formed with violent, disjointed strokes.\n\r", ch);
			act("The line is followed by six more parallel to it, each formed with violent, disjointed strokes.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("You complete the inscription by clawing your smeared fingers across the Cracks of Xixzyr.\n\r", ch);
			act("$n completes the inscription by clawing $s smeared fingers across the Cracks of Xixzyr.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 20: /* The Crest of Chaigidon */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "You apply %s to create a near-perfect circle upon the ground.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n applies %s to create a near-perfect circle upon the ground.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("You draw the image of a sword, its hilt, blade and quillions bisecting the circle.\n\r", ch);
			act("$n draws the image of a sword, its hilt, blade and quillions bisecting the circle.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("You complete the Crest of Chaigidon with a flowing script, spanning the circumference of the circle.\n\r", ch);
			act("$n completes the Crest of Chaigidon with a flowing script, spanning the circumference of the circle.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 21: /* The Aklaju Hieroglyph */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "You begin an inscription in %s, forming a rigid design of vertical segments joined by short arcs.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n begins an inscription in %s, forming a rigid design of vertical segments joined by short arcs.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("A stylized eye forms the focus of the Aklaju Hieroglyph, which you complete with a slitted pupil.\n\r", ch);
			act("A stylized eye forms the focus of the Aklaju Hieroglyph, which $n completes with a slitted pupil.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		case 22: /* The Crest of Khamurn */
		switch (mod % MAX_INSCRIBE_ECHOES)
		{
		    case 0:
			sprintf(buf, "You begin an inscription in %s, forming a small, six-sided star upon the ground.\n\r", matstr);
			send_to_char(buf, ch);
			sprintf(buf, "$n begins an inscription in %s, forming a small, six-sided star upon the ground.", matstr);
			act(buf, ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 1:
			send_to_char("You surround the star with six vaguely alien hieroglyphs, each cupped in a semicircle, like stylized scales.\n\r", ch);
			act("$n surrounds the star with six vaguely alien hieroglyphs, each cupped in a semicircle, like stylized scales.", ch, NULL, NULL, TO_ROOM);
			paf->modifier++;
			break;

		    case 2:
			send_to_char("You complete the Crest of Khamurn with a large hexagon, surrounding the whole.\n\r", ch);
			act("$n completes the Crest of Khamurn with a large hexagon, surrounding the whole.", ch, NULL, NULL, TO_ROOM);
			draw_symbol = TRUE;
			break;
		}
		break;

		default:
		    send_to_char("You complete your drawing of the symbol.\n\r", ch);
		    act("$n completes the inscription of the symbol.", ch, NULL, NULL, TO_ROOM);
		    draw_symbol = TRUE;
		    break;
	    }

	    if (draw_symbol)
	    {
	        affect_remove(ch, paf);
		naf.where     = TO_AFFECTS;
		naf.type      = gsn_inscribe;
		naf.level     = ch->level;
		naf.duration  = 4;
		naf.modifier  = 0;
		naf.location  = 0;
		naf.bitvector = 0;
		affect_to_char(ch, &naf);

	        if (ch->in_room)
		{
		    symbol = create_object(get_obj_index(OBJ_VNUM_SYMBOL_FIRST + (mod / MAX_INSCRIBE_ECHOES)), ch->level);
            symbol->level = ch->level;
		    symbol->timer = 12;
		    symbol->objfocus[0] = ch;
		    symbol->value[1] = stype;
		    if (number_percent() < (get_skill(ch, gsn_inscribe) * 0.9))
		    {
		        symbol->value[0] = 1;
			check_improve(ch, NULL, gsn_inscribe, TRUE, 1);
		    }
		    else
		    {
                int detectOdds(25);
                if (number_percent() <= get_skill(ch, gsn_scriptmastery))
                {
                    check_improve(ch, NULL, gsn_scriptmastery, true, 2);
                    detectOdds += 50;
                }

		        if (number_percent() <= detectOdds)
			    {
			        send_to_char("As you inspect your work, you notice an error in its design.\n\r", ch);
    			    check_improve(ch, NULL, gsn_inscribe, FALSE, 1);
	    		}
		    	else
			        check_improve(ch, NULL, gsn_inscribe, TRUE, 1);

    			symbol->value[0] = 0;
	 	    }
		    obj_to_room(symbol, ch->in_room);

		    if ((mod / MAX_INSCRIBE_ECHOES) == 2)
		    {
			naf.where     = TO_ROOM;
			naf.duration  = 12;
			naf.bitvector = ROOM_NOSUM_TO;
			affect_to_room(ch->in_room, &naf);
			naf.bitvector = ROOM_NOGATE;
			affect_to_room(ch->in_room, &naf);
		    }
		}
	    }
	}

	if (IS_OAFFECTED(ch, AFF_NIGHTFEARS) && (ch->position == POS_SLEEPING))
	{
	    AFFECT_DATA *paf;

	    for (paf = ch->affected; paf; paf = paf->next)
		if ((paf->type == gsn_nightfears) && (paf->modifier != 0))
		    break;

	    switch (paf->modifier)
            {
		case 1:
		    send_to_char("You awake in a cold sweat.\n\r", ch);
		    paf->modifier = number_range(1, 3) * 100;
		    switch_position(ch, POS_STANDING);
		    act("$n awakens, drenched in a cold sweat.", ch, NULL, NULL, TO_ROOM);
		    break;

		case 100:
		    send_to_char("As you sleep, you dream of yourself, running across a barren, wasted plain.\n\r", ch);
		    paf->modifier++;
		    break;

		case 101:
		    send_to_char("Overhead, no light burns in the sky, but your eyes can see in the pale luminescence of desiccated fungi and lichen.\n\r", ch);
		    paf->modifier++;
		    break;

		case 102:
		    send_to_char("Behind you comes the slurping and wet rasping of some misshapen Thing, chasing you across the plains.\n\r", ch);
		    paf->modifier++;
		    break;

		case 103:
		    send_to_char("You run with all your might, but your strength ebbs, and the Thing grows closer and closer.\n\r", ch);
		    paf->modifier++;
		    break;

		case 104:
		    send_to_char("The sickening rasp of its headlong progress grows so loud you cannot even hear your own footsteps, and a dark, rubbery smell heralds its coming.\n\r", ch);
		    paf->modifier++;
		    break;

		case 105:
		    send_to_char("The edge of the plain becomes visible, but your steps falter and you stumble, and, without mercy, the Thing is upon you....\n\r", ch);
		    paf->modifier = 1;
		    break;

		case 200:
		    send_to_char("As you sleep, you dream of yourself, walking a vast, crowded street.\n\r", ch);
		    paf->modifier++;
		    break;

		case 201:
		    send_to_char("Everywhere you go, you look for your true love. Everywhere you go, your love is just out ofsight, or singing and talking just around a corner.\n\r", ch);
		    paf->modifier++;
		    break;
		case 202:
		    send_to_char("The inhabitants of the city point to where your love has gone, but you are always just a moment behind.\n\r", ch);
		    paf->modifier++;
		    break;
		case 203:
		    send_to_char("You travel the width and breadth of the city, never lost, but always looking.\n\r", ch);
		    paf->modifier++;
		    break;
		case 204:
		    send_to_char("Days pass, and hope has almost left you. Almost by chance, you stumble into an alley, and your love is there, waiting!\n\r", ch);
		    paf->modifier++;
		    break;
		case 205:
		    send_to_char("You run to embrace them, filled with thoughts of joyous reunion.\n\r", ch);
		    paf->modifier++;
		    break;
		case 206:
		    send_to_char("Your arms encircle a waist, and you kiss the lips which you have sought for so long.\n\r", ch);
		    paf->modifier++;
		    break;
		case 207:
		    send_to_char("For some reason, your love does not respond, stiff-lipped and cold in the face of your passion.\n\r", ch);
		    paf->modifier++;
		    break;
		case 208:
		    paf->modifier++;
		    break;
		case 209:
		    send_to_char("For, as you realize, your true love is dead.\n\r", ch);
		    paf->modifier = 1;
		    break;

		case 300:
		    send_to_char("As you sleep, you dream of a terrifying nightmare.\n\r", ch);
		    paf->modifier += 2;
		    break;

		case 302:
		    send_to_char("You have been exploring a vast cavern, and come to a long, cyclopean bridge of stone. Heedless of a damp condensate on the bridge, you have slipped, and begun to fall into an abyss in which no bottom is visible.\n\r", ch);
		    paf->modifier++;
		    break;
		case 303:
		    send_to_char("Terror grips you as you descend, and you gasp as the wind howls around your falling form.\n\r", ch);
		    paf->modifier++;
		    break;
		case 304:
		    send_to_char("The blur of the craggy walls of the chasm is almost hynotic, and in some small way you come to terms with your fate.\n\r", ch);
		    paf->modifier++;
		    break;
		case 305:
		    send_to_char("Eventually, the ground becomes visible far below, and you sense that death is near.\n\r", ch);
		    paf->modifier++;
		    break;
		case 306:
		    send_to_char("The rhythm of beating wings comes from above, and you look upward with a start.\n\r", ch);
		    paf->modifier++;
		    break;
		case 307:
		    send_to_char("Some flying demon-thing of leathery wings and chitinous claws soars toward you, keening with unwholesome glee.\n\r", ch);
		    paf->modifier++;
		    break;
		case 308:
		    send_to_char("As its razor-sharp teeth find purchase in your tender flesh, your last thoughts are a prayer to be taken back to the simpler terrors of just a few moments ago....\n\r", ch);
		    paf->modifier = 1;
		    break;
	    }
	}


	if ((silver_state >= 2) && IS_OAFFECTED(ch, AFF_CONSUMPTION) && (ch->wait == 0) && (get_modifier(ch->affected, gsn_consumption) == 0))
	{
	    CHAR_DATA *nch;
	    OBJ_DATA *sweap;
	    AFFECT_DATA naf;
naf.valid = TRUE;
naf.point = NULL;
	    int ynum;
	    bool nfound = FALSE;

	    switch (silver_state)
	    {
		case 2:
		    act("You raise your arms to the sky, and begin a sonorous incantation.", ch, NULL, NULL, TO_CHAR);
		    act("$n raises $s arms to the sky, and begins a sonorous incantation.", ch, NULL, NULL, TO_ROOM);
		    silver_state++;
		    ch->wait = (PULSE_MOBILE * 3) - 1;
		    break;
		case 3:
		    for (nch = ch->in_room->people; nch; nch = nch->next_in_room)
		        if (IS_OAFFECTED(nch, AFF_CONSUMPTION) && ((ynum = get_modifier(nch->affected, gsn_consumption)) > 0))
			{
			    nfound = TRUE;
			    switch (ynum)
			    {
				case 1:
				    act("You cry forth 'Xthjich, the Eternal Demon, I name thee!'", nch, NULL, NULL, TO_CHAR);
				    act("$n cries forth 'Xthjich, the Eternal Demon, I name thee!'", nch, NULL, NULL, TO_ROOM);
				    break;
				case 2:
				    act("You cry forth 'Pricina, Queen of the Succubi, I name thee!'", nch, NULL, NULL, TO_CHAR);
				    act("$n cries forth 'Pricina, Queen of the Succubi, I name thee!'", nch, NULL, NULL, TO_ROOM);
				    break;
				case 3:
				    act("You cry forth 'Sultan Isetaton, I name thee!'", nch, NULL, NULL, TO_CHAR);
				    act("$n cries forth 'Sultan Isetaton, I name thee!'", nch, NULL, NULL, TO_ROOM);
				    break;
				case 4:
				    act("You cry forth 'Barkja, the Infernal Sage, I name thee!'", nch, NULL, NULL, TO_CHAR);
				    act("$n cries forth 'Barkja, the Infernal Sage, I name thee!'", nch, NULL, NULL, TO_ROOM);
				    break;
				case 5:
				    act("You cry forth 'Nyogthua, Sorceror of the Arcanium, I name thee!'", nch, NULL, NULL, TO_CHAR);
				    act("$n cries forth 'Nyogthua, Sorceror of the Arcanium, I name thee!'", nch, NULL, NULL, TO_ROOM);
				    break;
				case 6:
				    act("You cry forth 'Agduk, Scion of Xthjich, I name thee!'", nch, NULL, NULL, TO_CHAR);
				    act("$n cries forth 'Agduk, Scion of Xthjich, I name thee!'", nch, NULL, NULL, TO_ROOM);
				    break;
				case 7:
				    act("You cry forth 'Vaeshir, Vizier of Logor, I name thee!'", nch, NULL, NULL, TO_CHAR);
				    act("$n cries forth 'Vaeshir, Vizier of Logor, I name thee!'", nch, NULL, NULL, TO_ROOM);
				    break;
				case 8:
				    act("You cry forth 'Khamurn, Magistrate of the Void, I name thee!'", nch, NULL, NULL, TO_CHAR);
				    act("$n cries forth 'Khamurn, Magistrate of the Void, I name thee!'", nch, NULL, NULL, TO_ROOM);
				    break;
				case 9:
				    act("You cry forth 'Bahhaoth, Queen of the Dead Seas, I name thee!'", nch, NULL, NULL, TO_CHAR);
				    act("$n cries forth 'Bahhaoth, Queen of the Dead Seas, I name thee!'", nch, NULL, NULL, TO_ROOM);
				    break;
				case 10:
				    act("You cry forth 'Tzakmeqiel the Desecrator, I name thee!'", nch, NULL, NULL, TO_CHAR);
				    act("$n cries forth 'Tzakmeqiel the Desecrator, I name thee!'", nch, NULL, NULL, TO_ROOM);
				    break;
				case 11:
				    act("You cry forth 'Igray'iitu, Lord of the Wastes of Oame, I name thee!'", nch, NULL, NULL, TO_CHAR);
				    act("$n cries forth 'Igray'iitu, Lord of the Wastes of Oame, I name thee!'", nch, NULL, NULL, TO_ROOM);
				    break;
				case 12:
				    act("You cry forth 'An'akarta, Flame of Logor, I name thee!'", nch, NULL, NULL, TO_CHAR);
				    act("$n cries forth 'An'akarta, Flame of Logor, I name thee!'", nch, NULL, NULL, TO_ROOM);
				    break;
			    }
			    ch->wait = (PULSE_MOBILE * 3) - 1;
			    affect_strip(nch, gsn_consumption);
			    naf.where	 = TO_OAFFECTS;
			    naf.type     = gsn_consumption;
			    naf.level    = nch->level;
			    naf.duration = -1;
			    naf.modifier = -1;
			    naf.location = APPLY_HIDE;
			    naf.bitvector = AFF_CONSUMPTION;
			    affect_to_char(nch, &naf);
			    break;
			}

		    if (!nfound)
		    {
			act("You scream the words 'Ashur, Great Dragon of the Void, I name thee!'", ch, NULL, NULL, TO_CHAR);
			act("$n screams the words 'Ashur, Great Dragon of the Void, I name thee!'", ch, NULL, NULL, TO_ROOM);
			ch->wait = PULSE_MOBILE - 1;
			silver_state++;
		    }
		    break;

		case 4:
		    act("As you invoke the last unholy name, you lower your arms, and cease incanting.", ch, NULL, NULL, TO_CHAR);
		    act("As $n invokes the last unholy name, $s arms lower, and $s incantation ceases.", ch, NULL, NULL, TO_ROOM);
		    ch->wait = (PULSE_MOBILE * 2) - 1;
		    silver_state ++;
		    break;

		case 5:
		    do_say(ch, "By the power of these names infernal, I call upon the Outer Void.");
		    ch->wait = (PULSE_MOBILE * 2) - 1;
		    silver_state++;
		    break;

		case 6:
		    do_say(ch, "Accept the blood of our sacrifice, and break the Veil which bounds this world!");
		    ch->wait = PULSE_MOBILE - 1;
		    silver_state++;
		    break;

		case 7:
		    for (sweap = ch->in_room->contents; sweap; sweap = sweap->next_content)
		    {
			if (sweap->item_type == ITEM_WEAPON)
			{
			    naf.where	  = TO_OBJECT;
			    naf.type      = gsn_consumption;
			    naf.level     = sweap->level;
			    naf.duration  = 12;
			    naf.modifier  = 0;
			    naf.location  = 0;
			    naf.bitvector = 0;
			    affect_to_obj(sweap, &naf);
			    act("You gesture towards $p.", ch, sweap, NULL, TO_CHAR);
			    act("$n gestures towards $p.", ch, sweap, NULL, TO_ROOM);
			    nfound = TRUE;
			    ch->wait = PULSE_MOBILE - 1;
			    silver_state++;
			    break;
			}
		    }

		    if (!nfound)
		    {
			act("You end the ritual, as there is no weapon for consecration.", ch, NULL, NULL, TO_CHAR);
			act("The ritual ends as there is no weapon for consecration.", ch, NULL, NULL, TO_ROOM);
			for (nch = ch->in_room->people; nch; nch = nch->next_in_room)
			    affect_strip(nch, gsn_consumption);
			silver_state = 0;
		    }
		    break;

		case 8:
		    for (sweap = ch->in_room->contents; sweap; sweap = sweap->next_content)
			if (obj_is_affected(sweap, gsn_consumption))
			{
			    c_act("{YYou say, 'I consecrate $p to the Outer Darkness!'{x", ch, sweap, NULL, TO_CHAR);
			    c_act("{Y$n says, 'I consecrate $p to the Outer Darkness!'{x", ch, sweap, NULL, TO_ROOM);
			    nfound = TRUE;
			    ch->wait = (PULSE_MOBILE * 2) - 1;
			    silver_state++;
			    break;
			}

		    if (!nfound)
		    {
			act("The ritual ends as the consecrated weapon disappears.", ch, NULL, NULL, TO_CHAR);
			act("The ritual ends as the consecrated weapon disappears.", ch, NULL, NULL, TO_ROOM);
			for (nch = ch->in_room->people; nch; nch = nch->next_in_room)
			    affect_strip(nch, gsn_consumption);
			silver_state = 0;
		    }
		    break;			

		case 9:
		    act("Shadows flow inward from the surrounding night, and faces of terror form and dissipate in the seething darkness.", ch, NULL, NULL, TO_CHAR);
		    act("Shadows flow inward from the surrounding night, and faces of terror form and dissipate in the seething darkness.", ch, NULL, NULL, TO_ROOM);
		    ch->wait = (PULSE_MOBILE * 3) - 1;
		    silver_state++;
		    break;

		case 10:
		    act("The shadows seeth about the coven, embracing each in tendrils of darkness.", ch, NULL, NULL, TO_CHAR);
		    act("The shadows seeth about the coven, embracing each in tendrils of darkness.", ch, NULL, NULL, TO_ROOM);
		    ch->wait = (PULSE_MOBILE * 2) - 1;
		    silver_state++;
		    break;

		case 11:
		    for (sweap = ch->in_room->contents; sweap; sweap = sweap->next_content)
			if (obj_is_affected(sweap, gsn_consumption))
			{
			    act("With an unworldly shrieking, as of the death-cries of a thousand men, the shadows stream into $p.", ch, sweap, NULL, TO_CHAR);
			    act("With an unworldly shrieking, as of the death-cries of a thousand men, the shadows stream into $p.", ch, sweap, NULL, TO_ROOM);
			    nfound = TRUE;
			    ch->wait = (PULSE_MOBILE * 4) - 1;
			    silver_state++;
			    break;
			}

		    if (!nfound)
		    {
			act("The ritual ends as the consecrated weapon disappears.", ch, NULL, NULL, TO_CHAR);
			act("The ritual ends as the consecrated weapon disappears.", ch, NULL, NULL, TO_ROOM);
			for (nch = ch->in_room->people; nch; nch = nch->next_in_room)
			    affect_strip(nch, gsn_consumption);
			silver_state = 0;
		    }
		    break;			

		case 12:
		    do_say(ch, "When the sentient soul is fed to the night through the violence of this weapon, then this world will become one with the Void.");
		    ch->wait = PULSE_MOBILE - 1;
		    silver_state++;
		    break;

		case 13:
		    for (sweap = ch->in_room->contents; sweap; sweap = sweap->next_content)
			if (obj_is_affected(sweap, gsn_consumption))
			{
		    	    act("The night quietens as the ritual approaches its ending.", ch, NULL, NULL, TO_CHAR);
		    	    act("The night quietens as the ritual approaches its ending.", ch, NULL, NULL, TO_ROOM);
			    object_affect_strip(sweap, gsn_consumption);
			    naf.where     = TO_OBJECT;
			    naf.type      = gsn_consumption;
			    naf.level     = sweap->level;
			    naf.duration  = 12;
			    naf.modifier  = 0;
			    for (nch = ch->in_room->people; nch; nch = nch->next_in_room)
				if (is_affected(nch, gsn_consumption))
			 	{
				    naf.modifier++;
				    affect_strip(nch, gsn_consumption);
				}
			    naf.location  = 0;
			    naf.bitvector = 0;
			    affect_to_obj(sweap, &naf);
		    	    silver_state++;
			    nfound = TRUE;
			    break;
			}
		    		   
		    if (!nfound)
		    {
			act("The ritual fails to complete as the consecrated weapon disappears.", ch, NULL, NULL, TO_CHAR);
			act("The ritual fails to complete as the consecrated weapon disappears.", ch, NULL, NULL, TO_ROOM);
			for (nch = ch->in_room->people; nch; nch = nch->next_in_room)
			    affect_strip(nch, gsn_consumption);
			silver_state = 0;
		    }
		    break;		
	    }
	}


	if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && ch->in_room && area_is_affected(ch->in_room->area, gsn_icestorm) && !IS_SET(ch->in_room->room_flags,ROOM_INDOORS) && number_bits(4) == 0)
	    damage_old( ch, ch,number_range(ch->level/2, round(ch->level*1.5)),gsn_icestorm,DAM_COLD,TRUE);
	
	if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && ch->in_room && number_bits(2) == 0)
    {
        AFFECT_DATA * burn(get_room_affect(ch->in_room, gsn_blazinginferno));
        if (burn == NULL) burn = get_room_affect(ch->in_room, gsn_blaze);
        if (burn != NULL)
        {
            int burnLevel(burn->level);
            if (ch->level <= 25) 
                burnLevel = UMIN(burnLevel, ch->level);

            damage_old(ch, ch, dice(5, burnLevel), burn->type, DAM_FIRE, true);
        }
    }

	if (ch->in_room && area_is_affected(ch->in_room->area, gsn_rainoffire) && number_bits(5) == 0 && !is_affected(ch, gsn_consume) && !IS_NPC(ch) && !(ch->invis_level > 0) && !IS_SET(ch->in_room->room_flags, ROOM_INDOORS))
	{
        if (savesConsumeSpecial(NULL, ch))
        {
	        act("$n is struck by the strange raining fire, but seems unfazed.", ch, NULL, NULL, TO_ROOM);
    	    act("You are struck by the strange raining fire, but your connection to the flames protects you.", ch, NULL, NULL, TO_CHAR);
        }
        else
        {
	        act("$n is struck by the strange raining fire, which seems to be pulled into $s skin!", ch, NULL, NULL, TO_ROOM);
    	    act("You are struck by the strange raining fire, which seems to be pulled into your skin!", ch, NULL, NULL, TO_CHAR);
	        af.where		= TO_AFFECTS;
            af.type 		= gsn_consume;
            af.level 		= ch->level; 
            af.duration 	= 5;
            af.location		= 0;
            af.modifier 	= 0;
            af.bitvector 	= 0;
	        affect_to_char(ch, &af);
        }
	}

	if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && !IS_OAFFECTED(ch, AFF_GHOST)
         && ch->in_room && area_is_affected(ch->in_room->area, gsn_gravitywell)
         && is_flying(ch) && !is_affected(ch, gsn_stabilize))
	{
	    act("$n is pulled to the ground and painfully crashes down!", ch, NULL, NULL, TO_ROOM);
	    act("You are pulled to the ground and painfully crash down!", ch, NULL, NULL, TO_CHAR);
	    damage( ch,ch,number_range(ch->level/2,ch->level), gsn_gravitywell,DAM_OTHER,TRUE);
        stop_flying(*ch);
	}

	if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && ch->in_room && IS_OUTSIDE(ch) && number_bits(is_affected(ch, gsn_ionize) ? 3 : 5) == 0)
    {
        AFFECT_DATA * storm(affect_find(ch->in_room->area->affected, gsn_lightning_storm));
        if (storm == NULL) storm = affect_find(ch->in_room->area->affected, gsn_electricalstorm);
        if (storm != NULL)
        {
            act("You are struck by lightning from the storm!", ch, NULL, NULL, TO_CHAR);
            act("A bolt of lightning lances from the heavens, forking into $n!", ch, NULL, NULL, TO_ROOM);
            int effLevel(storm->level);
            if (ch->level < 25) effLevel = UMIN(ch->level, effLevel);
            sourcelessDamage(ch, "the lightning", dice(3, effLevel * 2), gsn_electricalstorm, DAM_LIGHTNING);
            check_conduitoftheskies(ch);
        }
    }

	if ((obj = get_eq_char(ch, WEAR_WIELD)) != NULL
    && !IS_NPC(ch) 
    && (get_skill(ch,gsn_augmented_strength) > 0 ?
	    get_obj_weight(obj) > (str_app[URANGE(0,get_curr_stat(ch,STAT_STR)+3,25)].wield * 10) :
	    get_obj_weight(obj) > (str_app[get_curr_stat(ch,STAT_STR)].wield * 10))
	  && !IS_OBJ_STAT(obj, ITEM_NOREMOVE))
	{
	    act("$p slips from $n's weakened hands.", ch, obj, NULL, TO_ROOM);
	    act("$p slips from your weakened hands.", ch, obj, NULL, TO_CHAR);
	    unequip_char(ch, obj);
	    oprog_remove_trigger(obj);
            obj_from_char( obj );
            obj_to_room( obj, ch->in_room );
	}

    // Check for sauna healing disqualifiers
    bool canHeal(!is_demon(ch));
    if (IS_NPC(ch))
    {
        if (IS_SET(ch->act, ACT_UNDEAD)) canHeal = false;
        else if (IS_SET(ch->act, ACT_ILLUSION)) canHeal = false;
    }

    // Check for sauna healing
    AFFECT_DATA * saunaAff;
    if (canHeal && ch->in_room != NULL && ((saunaAff = affect_find(ch->in_room->affected, gsn_sauna)) != NULL))
    {
        if (number_percent() < 10)
            send_to_char("You breathe in the warm mists, and feel the healing energy flow through your body.\n", ch);
            
        int healing(1 + (number_percent() / 20) + (saunaAff->level / 5));
        if (ch->in_room->sector_type == SECT_WATER_NOSWIM || ch->in_room->sector_type == SECT_WATER_SWIM)
            healing  = (healing * 3) / 2;

        ch->hit = UMIN(ch->max_hit, ch->hit + healing);
    }

    // Check for breath of life
    AFFECT_DATA * breathOfLife(get_affect(ch, gsn_breathoflife));
    if (canHeal && breathOfLife != NULL && breathOfLife->modifier > 0 && ch->hit < ch->max_hit)
    {
        int healing(5 + (breathOfLife->level / 10));
        healing = number_range(healing / 2, (healing * 3) / 2);
        healing = UMIN(healing, breathOfLife->modifier);
        healing = UMIN(healing, ch->max_hit - ch->hit);
        ch->hit += healing;
        breathOfLife->modifier -= healing;

        if (breathOfLife->modifier <= 0)
            send_to_char("You exhale the last vestiges of healing energy from the breath of life, and feel its power abandon you.\n", ch);
    }

    // Check for sandstorm
    if (ch->in_room != NULL && number_bits(1) == 0 && !IS_AFFECTED(ch, AFF_WIZI) && !IS_OAFFECTED(ch, AFF_GHOST) && (!IS_NPC(ch) || !IS_SET(ch->nact, ACT_SHADE)))
    {
        AFFECT_DATA * sandstorm(get_room_affect(ch->in_room, gsn_sandstorm));
        if (sandstorm != NULL)
        {
            if (is_affected(ch, gsn_mistralward))
                send_to_char("The winds pelt at you with stinging sand, but your mistral ward blows it away.\n", ch);
            else
            {
                send_to_char("The winds pelt you with stinging sand!\n", ch);
                int effLevel(sandstorm->level);
                if (ch->level < 25) effLevel = UMIN(effLevel, ch->level);
                int dam(dice(effLevel, 2));
                if (saves_spell(sandstorm->level, NULL, ch, DAM_SLASH))
                    sourcelessDamage(ch, "the pelting sand", dam / 2, gsn_sandstorm, DAM_SLASH);
                else
                {
                    sourcelessDamage(ch, "the pelting sand", dam, gsn_sandstorm, DAM_SLASH);

                    // Failed to save, so potentially blind them
                    if (number_bits(1) == 0 && !IS_SET(ch->imm_flags, IMM_BLIND) && !IS_AFFECTED(ch, AFF_BLIND) && IS_VALID(ch))
                    {
                        act("$n is blinded by the sand in $s eyes!", ch, NULL, NULL, TO_ROOM);
                        act("You are blinded by the sand in your eyes!", ch, NULL, NULL, TO_CHAR);

                        AFFECT_DATA af = {0};
                        af.where     = TO_AFFECTS;
                        af.type      = gsn_sandspray;
                        af.level     = sandstorm->level;
                        af.location  = APPLY_HITROLL;
                        af.modifier  = -4;
                        af.duration  = 1;
                        af.bitvector = AFF_BLIND;
                        affect_to_char(ch, &af);
                    }
                }
            }
        }
    }

    // Check for channel wind
    if (ch->in_room != NULL && number_bits(1) == 0 && !is_affected(ch, gsn_mistralward) && !is_affected(ch, gsn_anchor) && !IS_IMMORTAL(ch) 
    && (!IS_NPC(ch) || (!IS_SET(ch->act, ACT_NOSUBDUE) && !IS_SET(ch->act, ACT_SENTINEL) && !IS_SET(ch->nact, ACT_SHADE))))
    {
        AFFECT_DATA * channelWind(get_room_affect(ch->in_room, gsn_channelwind));
        if (channelWind != NULL)
        {
            Direction::Value direction(static_cast<Direction::Value>(channelWind->modifier));
            ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*ch->in_room, direction, EX_CLOSED|EX_WALLED|EX_WEAVEWALL|EX_ICEWALL));
            if (nextRoom != NULL && !saves_spell(channelWind->level, NULL, ch, DAM_BASH))
            {
                // Send echoes
                std::ostringstream mess;
                mess << "A rush of wind " << Direction::SourceNameFor(Direction::ReverseOf(direction)) << " sends you flying!\n";
                send_to_char(mess.str().c_str(), ch);

                mess.str("");
                mess << "A rush of wind " << Direction::SourceNameFor(Direction::ReverseOf(direction)) << " sends $n flying!";
                act(mess.str().c_str(), ch, NULL, NULL, TO_ROOM);

                // Move and lag char
                WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE / 2));
                char_from_room(ch);
                char_to_room(ch, nextRoom);
                do_look(ch, "auto");
                
                mess.str("");
                mess << "$n is blasted in " << Direction::SourceNameFor(Direction::ReverseOf(direction)) << "!";
                act(mess.str().c_str(), ch, NULL, NULL, TO_ROOM);
            }
            else
                send_to_char("A blast of wind slams into you, but you stand firm!\n", ch);
        }
    }

    // Check for call upon winds healing
    AFFECT_DATA * call(get_affect(ch, gsn_calluponwind));
    std::pair<Direction::Value, int> windInfo(checkWind(ch));
    if (call != NULL && call->modifier == Direction::West && windInfo.second > 0)
    {
        int healing(5 + (windInfo.second / (windInfo.first == Direction::West ? 5 : 15)));
        ch->hit = UMIN(ch->max_hit, ch->hit + healing);
    }

    // Check for conduit of stonesong
    AFFECT_DATA * stonesong(get_affect(ch, gsn_conduitofstonesong));
    if (stonesong != NULL && ch->in_room != NULL)
    {
        // Determine cost based on terrain
        int manaCost(10);
        if (ON_GROUND(ch))
        {
            switch (ch->in_room->sector_type)
            {
                case SECT_UNDERGROUND:  manaCost = 1; break;
                case SECT_MOUNTAIN:     manaCost = 2; break;
                case SECT_HILLS:        manaCost = 3; break;
                case SECT_SWAMP:        manaCost = 5; break;
                default:                manaCost = 4; break;
            }
        }

        // Check for sufficient mana
        if (ch->mana >= manaCost)
            expend_mana(ch, manaCost);
        else
        {
            // Insufficient mana
            affect_remove(ch, stonesong);
            send_to_char("You grow too weary to channel the power of stonesong, and it dies off within you.\n", ch);
            act("The deep humming sound resonating about $n abruptly ceases.", ch, NULL, NULL, TO_ROOM);

            // Echoes if drake is present
            if (ch->pet != NULL && ch->pet->in_room == ch->in_room && IS_NPC(ch->pet) && ch->pet->pIndexData->vnum == MOB_VNUM_DRAKE)
                act("$n seems to shrink slightly, $s claws and stony plating diminishing.", ch->pet, NULL, NULL, TO_ROOM);
        }
    }

	if (ch->song && global_song_message) 
	{
	    sprintf(buf, "$n continues to perform '%s'.", skill_table[ch->song->type].name);
	    act(buf, ch, NULL, NULL, TO_ROOM);
	    sprintf(buf, "You continue to perform '%s'.\n\r", skill_table[ch->song->type].name);
	    send_to_char(buf, ch);

	    if ((ch->song->type == gsn_psalmofhealing)
	    || (ch->song->type == gsn_serenadeoflife)
	    || (ch->song->type == gsn_manasong))
	    {
            CHAR_DATA *sch, *sch_next;
            AFFECT_DATA af = {0};

            for (sch = ch->in_room->people; sch; sch = sch_next)
            {
                sch_next = sch->next_in_room;
                if (is_an_avatar(sch))
                    continue;
 
                if (is_same_group(ch, sch) && ch->position > POS_SLEEPING && sch->position > POS_SLEEPING)
                {
                    af.where    = TO_AFFECTS;
                    af.type	    = ch->song->type;
                    af.level    = ch->song->level;
                    af.location = APPLY_NONE;

                    affect_to_char(sch, &af);

                    if (ch->song->type == gsn_manasong)
                    {
                       send_to_char("You feel renewed by the power of the fugue.\n\r", sch);
                       sch->mana += mana_gain(sch);
                    }
                    else
                    {
                        send_to_char("You feel the healing energy of the music flow through you.\n\r", sch);
                        sch->hit += hit_gain(sch);
                    }

                    affect_strip(sch, ch->song->type);
                }
            }
	    }

        if (ch->in_room != NULL)
        {
            // Handle gravebeat
            check_gravebeat_room(*ch);
            int gravebeatMana(gravebeat_total_levels(*ch) / 4);
            if (gravebeatMana > 0)
            {
                // Check for bone reaper
                check_bonereaper_mana(*ch, gravebeatMana);
                expend_mana(ch, gravebeatMana);
            }
     
            // Potentially expand the light for canticle of the lightbringer
            if (ch->song->type == gsn_canticleofthelightbringer && number_bits(1) == 0)
                expand_rystaias_light(*ch->in_room, ch->level);

            // Handle the Ilal's Last Sailing
            if (ch->song->type == gsn_theilalslastsailing)
                do_theilalslastsailing(*ch);
        }
	}

	if (ch->harmony && global_song_message) 
	{
	    sprintf(buf, "$n continues to harmonize '%s'.", skill_table[ch->harmony->type].name);
	    act(buf, ch, NULL, NULL, TO_ROOM);
	    sprintf(buf, "You continue to harmonize '%s'.\n\r", skill_table[ch->harmony->type].name);
	    send_to_char(buf, ch);

	    if ((ch->harmony->type == gsn_psalmofhealing)
	    || (ch->harmony->type == gsn_serenadeoflife)
	    || (ch->harmony->type == gsn_manasong))
        {
            CHAR_DATA *sch, *sch_next;
            AFFECT_DATA af = {0};

            for (sch = ch->in_room->people; sch; sch = sch_next)
            {
                sch_next = sch->next_in_room;
                if (is_an_avatar(sch))
                    continue;
    
                if (is_same_group(ch, sch) && (ch->position > POS_SLEEPING) && (sch->position > POS_SLEEPING))
                {
                    af.where    = TO_AFFECTS;
                    af.type	    = ch->harmony->type;
                    af.level    = ch->harmony->level;
                    af.location = APPLY_NONE;

                    affect_to_char(sch, &af);

                    if (ch->harmony->type == gsn_manasong)
                    {
                       send_to_char("You feel renewed by the power of the fugue.\n\r", sch);
                       sch->mana += mana_gain(sch);
                    }
                    else
                    {
                        send_to_char("You feel the healing energy of the music flow through you.\n\r", sch);
                        sch->hit += hit_gain(sch);
                    }

                    affect_strip(sch, ch->harmony->type);
                }
            }
        }

        // Potentially expand the light for canticle of the lightbringer
        if (ch->in_room != NULL && ch->song->type == gsn_canticleofthelightbringer && number_bits(3) == 0)
            expand_rystaias_light(*ch->in_room, ch->level);
	}


	if (!IS_NPC(ch) && ch->pcdata->performing)
	{
	    if (ch->song)
	        ch->pcdata->performing = NULL;
	    else
	   {
		char *pPoint = ch->pcdata->performing;
		char pCount = 0;
		char sbuf[MAX_STRING_LENGTH];

		while ((*pPoint != '\0') && (*pPoint != '\n') && (*pPoint != '\r'))
		{
		    pPoint++;
		    pCount++;
		}

		if (pCount > 0)
		{		
		    memset((void *) sbuf, 0, sizeof(sbuf));
		    strncpy(sbuf, ch->pcdata->performing, pCount);
		    do_sing(ch, sbuf);
		}

		while ((*pPoint == '\n') || (*pPoint == '\r'))
		    pPoint++;

		if (*pPoint == '\0')
		{
		    send_to_char("You end your performance.\n\r", ch);
		    ch->pcdata->performing = NULL;
		}
		else
		    ch->pcdata->performing = pPoint;
	    }
	}

	if (IS_NPC(ch) && IS_SET(ch->act, ACT_ILLUSION) && ch->master != NULL)
    {
        // Make sure the illusion and master are in the same room
	    if (ch->master->in_room != ch->in_room)
	    {
	        act("$n fades away.", ch, NULL, NULL, TO_ROOM);
	        stop_follower(ch);
	        continue;
	    }

        // Handle mana drain for the illusion
		expend_mana(ch->master, PhantasmInfo::totalDrainFor(*ch));
        if (!IS_VALID(ch))
		    continue;
    }

    // Move loyal drakes towards their masters
    if (ch->master != NULL && ch->master->in_room != NULL && ch->master->in_room != ch->in_room 
    && ch->in_room != NULL && number_bits(1) == 0)
    {
        int loyalCount(Drakes::SpecialCount(*ch, Drakes::Loyal));
        if (loyalCount > 0 && !StepAlongPath(ch))
        {
            // Could not step along path, look for a new one
            RoomPath roomPath(*ch->in_room, *ch->master->in_room, ch, loyalCount);
            AssignNewPath(ch, roomPath);
            StepAlongPath(ch);
        }
    }

    // Heal stonemend drakes
    if (ch->hit < ch->max_hit && ch->in_room != NULL && ch->in_room->sector_type == SECT_UNDERGROUND)
    {
        ch->hit += Drakes::SpecialCount(*ch, Drakes::Stonemend);
        ch->hit = UMIN(ch->hit, ch->max_hit);
    }

    if ( !IS_NPC(ch) || ch->in_room == NULL || (IS_AFFECTED(ch, AFF_CHARM) && ch->demontrack == NULL && !IS_SET(ch->nact, ACT_CHARMRAND)))
        continue;

	if ( ch->spec_fun != 0 )
	{
	    if ( (*ch->spec_fun) ( ch ) )
		continue;
	}

	if ( ch->position < POS_SLEEPING || ch->position == POS_FIGHTING )
	    continue;

    mprog_random_trigger( ch );

	if (IS_AFFECTED(ch, AFF_CHARM) && !ch->demontrack)
	    continue;

	if (!ch->in_room || ch->position < POS_SLEEPING)
	    continue;

	if (ch->in_room->area->empty && !IS_SET(ch->act,ACT_UPDATE_ALWAYS))
	    continue;

	/* That's all for sleeping / busy monster, and empty zones */

        /* MOBprogram random trigger */
                                                /* If ch dies or changes
                                                position due to it's random
                                                trigger, continue - Kahn */
            if ( ch->position < POS_STANDING )
                continue;

	/* Scavenge */
	if ( IS_SET(ch->act, ACT_SCAVENGER)
	&&   ch->in_room->contents != NULL
	&&   number_bits( 6 ) == 0 )
	{
	    OBJ_DATA *obj;
	    OBJ_DATA *obj_best;
	    int max;

	    max         = 1;
	    obj_best    = 0;
	    for ( obj = ch->in_room->contents; obj; obj = obj->next_content )
	    {
		if ( CAN_WEAR(obj, ITEM_TAKE) && can_loot(ch, obj)
		     && obj->cost > max  && obj->cost > 0)
		{
		    obj_best    = obj;
		    max         = obj->cost;
		}
	    }

	    if ( obj_best )
		get_obj( ch, obj_best, NULL );
	}

	/* Wander */
        if (!ch) continue;
	if ( !IS_SET(ch->act, ACT_SENTINEL) 
	&& !IS_SET(ch->act, ACT_NOWANDER)
	&& !IS_SET(ch->affected_by, AFF_CHARM)
    && !IS_OAFFECTED(ch, AFF_ENCASE)
    && !IS_NAFFECTED(ch, AFF_FLESHTOSTONE)
	&& ch->tracking == NULL
	&& !ch->desc
	&& ch->position == POS_STANDING
	&& number_bits(3) == 0
	&& !is_affected(ch, gsn_enslave)
    && !is_affected(ch, gsn_gravebeat)
	&& (( door = number_bits( 5 ) ) <= 5)
	&& ( pexit = ch->in_room->exit[door] ) != NULL
	&&   pexit->u1.to_room != NULL
	&&   !IS_SET(pexit->exit_info, EX_CLOSED|EX_FAKE|EX_WALLOFFIRE|EX_WALLOFVINES|EX_ILLUSION)
	&&   !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)
	&& ( !IS_SET(ch->act, ACT_STAY_AREA)
	||   pexit->u1.to_room->area == ch->in_room->area ) 
	&& ( !IS_SET(ch->act, ACT_OUTDOORS)
	||   !IS_SET(pexit->u1.to_room->room_flags,ROOM_INDOORS)) 
	&& ( !IS_SET(ch->act, ACT_INDOORS)
	||   IS_SET(pexit->u1.to_room->room_flags,ROOM_INDOORS)))
	{
	    move_char( ch, door, FALSE );
                                                /* If ch changes position due
                                                to it's or someother mob's
                                                movement via MOBProgs,
                                                continue - Kahn */
            if ( ch->position < POS_STANDING )
                continue;
	}
	
    // Move a shade roughly every other pulse
    if (IS_NPC(ch) && IS_SET(ch->nact, ACT_SHADE) && number_bits(1) == 0)
        StepAlongPath(ch); 

    // Return unenthralled gravebeaten undead to their homes
    AFFECT_DATA * gravebeat(get_affect(ch, gsn_gravebeat));
    if (gravebeat != NULL && ch->in_room != NULL)
    {
        if (gravebeat->modifier == 0)
        {
            // Heading home
            if (gravebeat->point != 0 && number_bits(1) == 0)
            {
                if (!StepAlongPath(ch))
                {
                    // Either path failed or was never built, so make one
                    RoomPath path(*ch->in_room, *static_cast<ROOM_INDEX_DATA*>(gravebeat->point), ch, RoomPath::Infinite, EX_WALLED|EX_WEAVEWALL|EX_ICEWALL);
                    bool needsOpening(!path.Exists());
                    if (path.Exists())

                    {
                        // Able to chart a path, so assign it and step along it
                        AssignNewPath(ch, path);
                        needsOpening = !StepAlongPath(ch);
                    }

                    if (needsOpening)
                    {
                        // No path exists or cannot step along it; open all surrounding doors and try again
                        for (unsigned int i(0); i < Direction::Max; ++i)
                        {
                            if (ch->in_room->exit[i] != NULL && IS_SET(ch->in_room->exit[i]->exit_info, EX_CLOSED))
                                do_open(ch, const_cast<char*>(Direction::NameFor(static_cast<Direction::Value>(i))));
                        }
                        
                        RoomPath open_path(*ch->in_room, *static_cast<ROOM_INDEX_DATA*>(gravebeat->point), ch, RoomPath::Infinite, EX_WALLED|EX_WEAVEWALL|EX_ICEWALL);
                        if (open_path.Exists())
                        {
                            // Able to chart a path, so assign it and step along it
                            AssignNewPath(ch, open_path);
                            StepAlongPath(ch);
                        }
                    }
                }
            }
        }
        else
        {
            // Still under control
            CHAR_DATA * controller(get_char_by_id_any_room(gravebeat->modifier, *ch->in_room));
            if (controller == NULL || get_char_song(*controller, gsn_gravebeat) == NULL)
                gravebeat->modifier = 0;
            else
                ch->tracking = NULL;
        }
    }

	/* Execute mob tracking */
	if (ch->tracking && ch->in_room && !IS_SET(ch->act, ACT_SENTINEL)
	 && !IS_SET(ch->act, ACT_NOTRACK) && !IS_SET(ch->nact, ACT_CLASSED))
	{
	    door = -1;

	    for (pTrack = ch->in_room->tracks; pTrack->ch != NULL; pTrack = pTrack->next)
		if (pTrack->ch == ch->tracking)
		{
		    door = pTrack->direction;
		    break;
		}

	    if (IS_SET(ch->nact, ACT_TRACK_OPEN) && (door >= 0) && ch->in_room->exit[door]
	     && IS_SET(ch->in_room->exit[door]->exit_info, EX_CLOSED))
		do_open(ch, dir_name[door]);

	    if (IS_SET(ch->nact, ACT_TRACK_RAM) && (door >= 0) && ch->in_room->exit[door]
	     && IS_SET(ch->in_room->exit[door]->exit_info, EX_CLOSED))
		do_ram(ch, dir_name[door]);

	    if ((number_bits(2) == 0 || IS_SET(ch->nact, ACT_TRACK_FAST))
	    && ( door >= 0 )
	    && ( pexit = ch->in_room->exit[door] ) != NULL
	    &&   pexit->u1.to_room != NULL
	    &&   !IS_SET(pexit->exit_info, EX_CLOSED)
	    &&   !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)
	    && ( !IS_SET(ch->act, ACT_STAY_AREA)
	     ||   pexit->u1.to_room->area == ch->in_room->area ) 
	    && ( !IS_SET(ch->act, ACT_OUTDOORS)
	     ||   !IS_SET(pexit->u1.to_room->room_flags,ROOM_INDOORS)) 
	    && ( !IS_SET(ch->act, ACT_INDOORS)
	     ||   IS_SET(pexit->u1.to_room->room_flags,ROOM_INDOORS)))
		move_char(ch, door, FALSE);
	}
	

	if ( (ch->act & ACT_TRACK_GATE)
	&& ch->tracking != NULL
	&& number_bits(3) == 0
	&& !IS_AFFECTED(ch, AFF_CHARM)
	&& !(ch->act & ACT_SENTINEL)
	&& !(ch->act & ACT_STAY_AREA)
	&& !(ch->tracking->in_room->room_flags & ROOM_NO_MOB)
	&& ch->fighting == NULL)
	{
	target_name = ch->tracking->name;
	spell_gate(73, ch->level, ch, NULL, 0);
	}


    }
}

/*
 * Update the weather.
 */
void time_update( void )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int i;
    bool update = FALSE;
    TIME_INFO_DATA old_time_info = time_info;

    buf[0] = '\0';

// Some time system fixes courtesy of pope ninjadyne I
//    to make one day pass/tick
//    time_info.hour = 23; 
//    time_info.half = TRUE;
    if (time_info.half)
    {
        time_info.hour++;
	time_info.half = FALSE;
    }
    else
	time_info.half = TRUE;

    if ((time_info.hour == season_table[time_info.season].sun_up) && !time_info.half)
    {
	if (silver_state == SILVER_CHARGED)
	{
	    OBJ_DATA *obj;

	    for (obj = object_list; obj; obj = obj->next)
		if (obj_is_affected(obj, gsn_consumption))
		{
		    if (obj->carried_by)
			act("With the coming of dawn, the unholy aura around $p fades.", obj->carried_by, obj, NULL, TO_CHAR);
		    object_affect_strip(obj, gsn_consumption);
		}
	}
	if (silver_state <= SILVER_CHARGED)
	    strcat( buf, "The sun rises in the east.\n\r" );
	else
	    silver_state = 0;
	CHAR_DATA *vch;
	for (vch = char_list;vch;vch=vch->next)
	    if (is_affected(vch,gsn_moonbornspeed))
	    {
		send_to_char("Your moonborn speed dissipates with the rising sun.\n\r",vch);
		affect_strip(vch,gsn_moonbornspeed);
	    }
	
    }

    if ((time_info.hour == season_table[time_info.season].sun_down) && !time_info.half)
    {
	if (silver_state != SILVER_FINAL)	
	    strcat( buf, "The sun slowly disappears in the west.\n\r" );
    }

    if (time_info.hour == NUM_HOURS_DAY)
    {
	time_info.hour = 0;
	time_info.day++;
	time_info.week++;
	time_info.day_year++;

	time_info.season = -1;
	for (i = 0; season_table[i].name; i++)
	    if (time_info.day_year >= season_table[i].first_day)
		time_info.season = i;
        if (time_info.season == -1)
	    time_info.season = 3;

	update = TRUE;
	time_info.phase_lunus = (int)trunc(time_info.day_year/4);
	time_info.phase_lunus %= 8;
	time_info.phase_rhos = (int)trunc(time_info.day_year/12);
	time_info.phase_rhos %= 8;
	int sr = time_info.day_year % 95;
	if (sr < 6)     sr = 2;
	else if (sr<18) sr = 1;
	else if (sr<46) sr = 0;
	else if (sr<58) sr = 1;
	else if (sr<68) sr = 2;
	else if (sr<71) sr = 3;
	else if (sr<76) sr = 4;
	else if (sr<83) sr = 3;
	else if (sr<88) sr = 4;
	else if (sr<92) sr = 3;
	else 		sr = 2;
	time_info.size_rhos = sr;
    }

    if (time_info.week == 7)
        time_info.week = 0;
 
    if (time_info.day >= month_table[time_info.month].num_days)
    {
        time_info.day = 0;
        time_info.month++;
    }
    
    if (!month_table[time_info.month].name)
    {
        time_info.month = 0;
	time_info.year++;
	time_info.day_year = 0;
    }
    
    if ((time_info.phase_rhos != old_time_info.phase_rhos)
      || (time_info.phase_lunus != old_time_info.phase_lunus)
      || (time_info.size_rhos != old_time_info.size_rhos))
        lunar_update();

    if (update)
    {
        char date[MAX_STRING_LENGTH];

	sprintf(date, "With the passing of midnight, it is now ");
	strcpy(date, display_date(date));

	for (d = descriptor_list; d; d = d->next)
	    if (d->connected == CON_PLAYING && d->character->in_room)
	        send_to_char(date,d->character);
    }

    return;
}

void lunar_update_char(CHAR_DATA *ch)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    affect_strip(ch, gsn_lunarinfluence);

    af.where = TO_AFFECTS;
    af.type = gsn_lunarinfluence;
    af.level = ch->level;
    af.duration = -1;
    af.bitvector = 0;
    af.location = APPLY_DAMROLL;

    switch(time_info.phase_rhos)
    {
	case (MOON_NEW):
	    af.modifier  = 1;
	    break;
	
	case (MOON_WANING_CRESCENT):
	case (MOON_WAXING_CRESCENT):
	    af.modifier  = 2;
	    break;

	case (MOON_WAXING_HALF):
	case (MOON_WANING_HALF):
	    af.modifier  = 4;
	    break;
	
	case (MOON_WAXING_GIBBOUS):
	case (MOON_WANING_GIBBOUS):
	    af.modifier  = 7;
	    break;
	
	case (MOON_FULL):
	    af.modifier  = 10;
	    break;
    }
    affect_to_char(ch,&af);

    af.location = APPLY_SAVES;
    switch(time_info.size_rhos)
    {
       case (MSIZE_TINY):
           af.modifier = -1;
	   break;
       case (MSIZE_SMALL):
           af.modifier = -2;
	   break;
	case (MSIZE_MEDIUM):
	    af.modifier = -4;
	    break;
	case (MSIZE_LARGE):
	    af.modifier = -7;
	    break;
	case (MSIZE_HUGE):
	    af.modifier = -10;
	    break;
    }    
    
    affect_to_char(ch,&af);
    
    af.location = APPLY_HIT;

    switch(time_info.phase_lunus)
    {
	case (MOON_NEW):
            af.modifier  = (int)(ch->level * 0.2);
	    break;
	    
	case (MOON_WAXING_CRESCENT):
	case (MOON_WANING_CRESCENT):
            af.modifier  = (int)(ch->level * 0.4);
	    break;

	case (MOON_WAXING_HALF):
	case (MOON_WANING_HALF):
            af.modifier  = (int)(ch->level * 0.8);
	    break;

	case (MOON_WAXING_GIBBOUS):
	case (MOON_WANING_GIBBOUS):
	    af.modifier  = (int)(ch->level * 1.4);
	    break;

	case (MOON_FULL):
            af.modifier  = ch->level*2;
	    break;
	
	default:
	    bug("Lunar change and null moon value.", 0);
	    return;
	    break;
    }
   
    affect_to_char(ch, &af);
    af.location = APPLY_MANA;
    af.modifier = (int)(ch->level * 2 + ch->level * 0.2 - af.modifier);
    affect_to_char(ch,&af);
}



void lunar_update()
{
    DESCRIPTOR_DATA *d;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    for	(d = descriptor_list; d != NULL; d = d->next)
    {
       if (d->character != NULL)
	if (d->character->class_num == global_int_class_druid)
	{
	    send_to_char("You feel the influence of the moon upon you change.\n\r", d->character);
	    lunar_update_char(d->character);
	}
       if (is_affected(d->character, gsn_lycanthropy))
       {
	       switch(d->character->in_room->sector_type)
	       {
		        case SECT_FIELD:
		        case SECT_FOREST:
		      	case SECT_MOUNTAIN:
			case SECT_HILLS:
			case SECT_AIR:
			case SECT_SWAMP:
			case SECT_ROAD:
			case SECT_DESERT:
				if (!IS_AFFECTED(d->character, AFF_BERSERK))
				{
					act("You feel the moon's pull enrage your spirit!", 
						d->character, NULL, NULL, TO_CHAR);
					act("$n's lupine eyes glaze over as rage overtakes $m!", 
						d->character, NULL, NULL, TO_ROOM);
					af.where        = TO_AFFECTS;
					af.type         = gsn_berserk;
					af.level        = d->character->level;
					af.duration     = number_fuzzy(d->character->level / 8);
					af.modifier     = UMAX(1,d->character->level/2);
					af.bitvector    = AFF_BERSERK;
					af.location     = APPLY_HITROLL;
					affect_to_char(d->character,&af);

					af.location     = APPLY_DAMROLL;
					affect_to_char(d->character,&af);

					af.modifier     = UMAX(10,10 * (d->character->level/5));
					af.location     = APPLY_AC;
					affect_to_char(d->character,&af);

					if (!IS_NPC(d->character))
						SET_BIT(d->character->act, PLR_AUTOATTACK);
				}
				break;
			default: 
				break;
	       }
       }
    }
}

/*
 * Update all chars, including mobs.
*/
void char_update( void )
{   
    int direction;
    CHAR_DATA *ch = NULL, *vch = NULL, *vch_next = NULL;
    CHAR_DATA *ch_next = NULL;
    CHAR_DATA *ch_quit = NULL;
    CHAR_DATA *pMob = NULL;
    long faff;
    char buf[MAX_STRING_LENGTH];
    ch_quit	= NULL;
    OBJ_DATA *vobj = NULL;

    /* update save counter */
    save_number++;

    if (save_number > 29)
	save_number = 0;

    for ( ch = char_list; ch != NULL; ch = ch_next )
    {
        ch_next = ch->next;
        AFFECT_DATA naf = {0};
        AFFECT_DATA *af = NULL;
        AFFECT_DATA *paf = NULL;
        AFFECT_DATA *paf_next = NULL;

        // Check scion of night updates; this must take precedence over stasis rift to avoid
        // abuse of stasis rift to keep the nighttime effects going all day, since they are only stripped
        // at the break of dawn and not otherwise
        check_scionofnight(*ch);

        // Check stasis
        if (is_in_stasis(*ch))
            continue;

#ifdef HERODAY
        if(!IS_IMMORTAL(ch) && !IS_NPC(ch))
	    if(IS_SET(ch->in_room->room_flags,ROOM_ARENA)
	    && !BIT_GET(ch->pcdata->bitptr,18653))
	        SET_BIT(ch->affected_by,AFF_WIZI);
	    else
	        REMOVE_BIT(ch->affected_by,AFF_WIZI);
#endif
    
    // Update the sleep timer
    if (ch->position <= POS_SLEEPING)
    {
        if (IS_OAFFECTED(ch, AFF_PARANOIA))
        {
            send_to_char("Your paranoia runs wild, preventing anything more than fitful slumber.\n", ch);
            ch->ticks_slept = 0;
        }
        else
        {
            ++ch->ticks_slept;
            if (ch->ticks_slept == 1)
                send_to_char("Your sleep grows deep and truly restful.\n", ch);
        }
    }
    
    // Improve revenant
    if (number_percent() <= get_skill(ch, gsn_revenant))
        check_improve(ch, NULL, gsn_revenant, true, 8);
   
	if ( ch->timer > 30 && !IS_IMMORTAL(ch) && (!IS_NPC(ch) || !IS_SET(ch->nact, ACT_SHADE)))
            ch_quit = ch;

	if (ch->demontrack)
	    ch->tracking = ch->demontrack;

	faff = race_table[ch->race].aff;

	if (!IS_SET(ch->affected_by, AFF_FLY_NATURAL))
	    REMOVE_BIT(faff, AFF_FLY_NATURAL);

	ch->affected_by |= faff;

	if (is_affected(ch, gsn_earthbind) || (ch->in_room && area_is_affected(ch->in_room->area, gsn_gravitywell) && !is_affected(ch, gsn_stabilize)))
        stop_flying(*ch);

	if (IS_SET(ch->naffected_by, AFF_FURY))
	  REMOVE_BIT(ch->naffected_by, AFF_FURY);

        if (!IS_NPC(ch)) 
	{
	    ch->pcdata->adrenaline=UMAX(0,ch->pcdata->adrenaline-1);
	
	    ch->objlevels = ch->objlevels - 20 > 0 ? ch->objlevels - 20 : 0;

// strength of Aramril
	if (!IS_NPC(ch) && ch->in_room && (ch->clan == clan_lookup("CHAMPION")) && (ch->in_room->clan == clan_lookup("CHAMPION")) && !is_affected(ch,gsn_strength_of_Aramril))
	{
	    AFFECT_DATA naf;
	    int mod = aura_grade(ch);
	    int samod = 0;
	    if (mod == -4)
	        samod = 50;
	    else if (mod == -3)
	        samod = 40;
	    else if (mod == -2)
	        samod = 30;
	    else if (mod == -1)
	        samod = 20;
	    else samod = 0;
	    
	    naf.valid = TRUE;
	    naf.point = NULL;
	    naf.where = TO_AFFECTS;
	    naf.type = gsn_strength_of_Aramril;
	    naf.level = ch->level;
	    naf.duration = -1;
	    naf.location = APPLY_HIT;
	    naf.modifier = samod;
	    naf.bitvector = 0;
	    affect_to_char(ch, &naf);
	    ch->hit += samod;
	    naf.modifier = samod / -10;
	    naf.location = APPLY_SAVES;
	    affect_to_char(ch, &naf);
	    send_to_char("The strength of Aramril surges within you as you enter the Tower grounds.\n\r",ch);
 	}
	else if (!IS_NPC(ch) && ch->in_room && (ch->clan == clan_lookup("CHAMPION")) && (ch->in_room->clan != clan_lookup("CHAMPION")) && is_affected(ch, gsn_strength_of_Aramril))
	{
	    affect_strip(ch,gsn_strength_of_Aramril);
	    send_to_char("The strength of Aramril fades as you leave the Tower grounds.\n\r",ch);
	}

	if (ch->in_room && (ch->in_room->sector_type != SECT_UNDERWATER) 
	&& is_affected(ch, gsn_gills) && !IS_OAFFECTED(ch, AFF_GHOST))
	{
		if ((paf = affect_find(ch->affected, gsn_gills)) != NULL)
		{
			switch (paf->modifier)
			{
				case 0:
				  send_to_char("You choke, unable to breathe normal air with your gills!\n\r", ch);
  				  damage(ch, ch, number_range(1, ch->level), gsn_drowning, DAM_DROWNING, TRUE);
				  break;
				default:
				  send_to_char("You choke violently, unable to breathe!\n\r", ch);
				  act("$n grabs at $s neck, writhing in pain!", ch, NULL, NULL, TO_ROOM);
				  damage(ch, ch, number_range(ch->max_hit / 2 * (paf->modifier - 1) / paf->modifier, 
						ch->max_hit / 2), gsn_drowning, DAM_DROWNING, TRUE);
				  break;
			}
			if (!IS_VALID(ch) || IS_OAFFECTED(ch, AFF_GHOST))
				continue;

			paf->modifier += 1;
		}
	}
	else
	{
		if ((paf = affect_find(ch->affected, gsn_gills)) != NULL)
			paf->modifier = 0;
	}
	    
	    if (ch->in_room && (ch->in_room->sector_type == SECT_UNDERWATER)
	     && !IS_SET(ch->imm_flags, IMM_DROWNING)
	     && !IS_PAFFECTED(ch, AFF_AIRLESS)
	     && !IS_IMMORTAL(ch)
	     && !IS_OAFFECTED(ch, AFF_GHOST))
        {
            if ((paf = affect_find(ch->affected, gsn_drowning)) != NULL)
            {
                switch (paf->modifier)
                {
                case -1:
                    send_to_char("You continue holding your breath, and your chest begins to burn.\n\r",ch);
                    break;
                case 0:
                    send_to_char("You choke, unable to breathe, as water begins to fill your lungs!\n\r", ch);
                    damage(ch, ch, number_range(1, ch->level), gsn_drowning, DAM_DROWNING, TRUE);
                    break;

                default:
                    send_to_char("You choke violently, unable to breathe!\n\r", ch);
                    damage(ch, ch, number_range(ch->max_hit / 2 * (paf->modifier - 1) / paf->modifier, ch->max_hit / 2), gsn_drowning, DAM_DROWNING, TRUE);
                    break;
                }

                if (!IS_VALID(ch) || IS_OAFFECTED(ch, AFF_GHOST))
                continue;

                paf->modifier += 1;
            }
            else
            {
                if (BIT_GET(ch->pcdata->traits,TRAIT_AQUATIC))
                send_to_char("You calmly begin holding your breath.\n\r",ch);
                else
                send_to_char("You hold your breath tightly, unable to breathe the water surrounding you.\n\r", ch);
                naf.where	  = TO_AFFECTS;
                naf.type	  = gsn_drowning;
                naf.level     = ch->level;
                naf.duration  = -1;
                naf.location  = APPLY_HIDE;
                naf.modifier  = BIT_GET(ch->pcdata->traits,TRAIT_AQUATIC) ? -1 : 0;
                naf.bitvector = 0;
                affect_to_char(ch, &naf);
            }
        }
	    else
            affect_strip(ch, gsn_drowning);
	}
        if (IS_NAFFECTED(ch, AFF_GARROTE_VICT))
        { 
	    AFFECT_DATA gaf, *gpaf;
gaf.valid = TRUE;
gaf.point = NULL;
	    CHAR_DATA *gch = NULL;

	    for (gpaf = ch->affected; gpaf; gpaf = gpaf->next)
		if ((gpaf->type == gsn_garrote) && (gpaf->bitvector == AFF_GARROTE_VICT))
		{
		    gch = (CHAR_DATA *) gpaf->point;
		    break;
		}
	    if (gpaf == NULL)
		REMOVE_BIT(ch->naffected_by,AFF_GARROTE_VICT);
	    else
		if (gch && gch->in_room == ch->in_room)
	    	{        	
        	    if (number_percent() > (get_curr_stat(ch, STAT_DEX) + get_curr_stat(ch, STAT_STR)))
          	    {  
            	    	if (gpaf->modifier >= -201)
            	    	{
            	   	    act("$n chokes and thrashes, but cannot escape!", ch, NULL, NULL, TO_ROOM);
            	    	    act("You choke and thrash, but cannot escape!", ch, NULL, NULL, TO_CHAR);
			    gpaf->location = APPLY_MANA;
              		    gpaf->duration += 2;
              		    gpaf->modifier -= 100;
			    ch->mana -= 100;
			    ch->max_mana -= 100;
			    check_improve(gch,ch,gsn_garrote,TRUE,1);
           	    	}
            	        else
            	    	{
              		    act("$n feebly thrashes a final time, and slumps to the ground.", ch, NULL, NULL, TO_ROOM);
              		    act("You feebly thrash once more, and everything fades to black.", ch, NULL, NULL, TO_CHAR);
		
			    REMOVE_BIT(ch->naffected_by,AFF_GARROTE_VICT);
			    SET_BIT(ch->affected_by,AFF_SLEEP);
			    switch_position(ch,POS_SLEEPING);
			    gpaf->bitvector = AFF_SLEEP;
			    gpaf->where = TO_AFFECTS;
			    gpaf->duration = 3;
			    for (gpaf = gch->affected; gpaf; gpaf = gpaf->next)
			    	if ((gpaf->type == gsn_garrote) && (gpaf->bitvector == AFF_GARROTE_CH))
			    	{
				    REMOVE_BIT(gch->naffected_by,AFF_GARROTE_CH);
				    break;
			    	}
			    check_improve(gch,ch,gsn_garrote,TRUE,1);
		    	}
       		    }
       		    else
       		    {
       		    	act("$n struggles away from the garrote, gasping for breath!", ch, NULL, NULL, TO_ROOM);
     		    	act("You struggle away from the garrote, gasping for breath!", ch, NULL, NULL, TO_CHAR);

		    	REMOVE_BIT(ch->naffected_by,AFF_GARROTE_VICT);
		    	gpaf->where = TO_AFFECTS;
		    	gpaf->bitvector = 0;

		    	for (gpaf = gch->affected; gpaf; gpaf = gpaf->next)
			    if ((gpaf->type == gsn_garrote) && (gpaf->bitvector == AFF_GARROTE_CH))
			    {
			    	affect_remove(gch, gpaf);
			    	break;
			    }
			one_hit(gch,ch,TYPE_UNDEFINED,HIT_PRIMARY, false);
			check_improve(gch,ch,gsn_garrote,FALSE,1);
          	    }
            	}
	    	else 
	    	{
		    send_to_char("You begin to breathe easier.\n\r", ch);
		    REMOVE_BIT(ch->naffected_by,AFF_GARROTE_VICT);
		    gpaf->where = TO_AFFECTS;
		    gpaf->bitvector = 0;
	    	}
	}
		
        if (IS_NAFFECTED(ch, AFF_SUBMISSION_VICT))
	{
	    AFFECT_DATA saf, *naf;
saf.valid = TRUE;
saf.point = NULL;
	    CHAR_DATA *gch;

	    for (gch = ch->in_room->people;gch != NULL;gch = gch->next_in_room)
		if (IS_NAFFECTED(gch, AFF_SUBMISSION_CH))
		    if ((naf = affect_find(gch->affected, gsn_submissionhold))!=NULL)
			if (naf->modifier = ch->id)
			    break;

	    if (gch==NULL)
	        affect_strip(ch,gsn_submissionhold);
	    else
	    {
	    	naf = affect_find(ch->affected, gsn_submissionhold);
	    
	    	if (number_percent() > (get_curr_stat(ch, STAT_DEX) + get_curr_stat(ch, STAT_STR)))
	    	{
		    if (naf->modifier == -4)
		    {
		    	act("$n struggles, trying to escape the submission hold!", ch, NULL, NULL, TO_ROOM);
		    	act("Sharp pains fill your aching flesh as the submission hold continues!", ch, NULL, NULL, TO_CHAR);
		    	naf->duration = 6;
		    	naf->modifier -= 2;
		    }
		    else if (naf->modifier == -2)
		    {
		    	act("A steady ache develops in your limbs as the submission hold continues.",ch,NULL,NULL,TO_CHAR);
		    	act("$n struggles, trying to escape the submission hold!", ch, NULL, NULL, TO_ROOM);
		    	naf->modifier -= 2;
		    }
		    else if (naf->modifier == 0)
		    {
		    	act("$n struggles, trying to escape the submission hold!", ch, NULL, NULL, TO_ROOM);
		    	act("You struggle, trying to escape the submission hold!", ch, NULL, NULL, TO_CHAR);
		    	naf->location = APPLY_STR;
		    	naf->modifier -= 2;
			check_improve(gch,ch,gsn_submissionhold,TRUE,1);
		    }
		    else if (naf->modifier == -6)
		    {
		    	act("You finally break the submission hold, your arms and chest in agony!",ch,NULL,NULL,TO_CHAR);
		    	act("$n struggles free of the submission hold!",ch,NULL,NULL,TO_ROOM);
		        REMOVE_BIT(ch->naffected_by, AFF_SUBMISSION_VICT);
		        REMOVE_BIT(gch->naffected_by, AFF_SUBMISSION_CH);
		        one_hit(gch,ch,TYPE_UNDEFINED,HIT_PRIMARY, false);
		        WAIT_STATE(ch,UMAX(ch->wait,PULSE_VIOLENCE*2));
		        WAIT_STATE(gch,UMAX(gch->wait,PULSE_VIOLENCE*2));
		        free_string(ch->pose);
		        free_string(gch->pose);
                check_improve(gch,ch,gsn_submissionhold,TRUE,1);
		    }
	        }
	        else
	        {
	            act("$n twists violently, slipping free of the submission hold!", ch, NULL, NULL, TO_ROOM);
	            act("You twist violently, tearing free of the submission hold!", ch, NULL, NULL, TO_CHAR);

		    REMOVE_BIT(ch->naffected_by, AFF_SUBMISSION_VICT);
		    REMOVE_BIT(gch->naffected_by, AFF_SUBMISSION_CH);
       	            naf->duration = 6;
       	            naf->location = APPLY_STR;
                    naf->modifier -= 2;
		    one_hit(ch,gch,TYPE_UNDEFINED,HIT_PRIMARY, false);
		    WAIT_STATE(ch,UMAX(ch->wait,PULSE_VIOLENCE*2));
		    WAIT_STATE(gch,UMAX(ch->wait,PULSE_VIOLENCE*2));
		    free_string(ch->pose);
		    free_string(gch->pose);
		    check_improve(gch,ch,gsn_submissionhold,FALSE,1);
	        }
	    }
	}

    // Check for fog elemental updates
    if (IS_NPC(ch) && ch->pIndexData->vnum == MOB_VNUM_FOGELEMENTAL)
    {
        if (update_fogelemental(ch))
            continue;
    }

    // Check for createfoci
    OBJ_DATA * foci(get_foci(ch, false));
    if (foci != NULL)
    {
        // Focus exists, count how many effects have already been added
        int fociEffectCount(0);
        for (AFFECT_DATA * fociEffect(get_obj_affect(foci, gsn_createfoci)); fociEffect != NULL; fociEffect = get_obj_affect(foci, gsn_createfoci, fociEffect))
        {
            if (fociEffect->location != APPLY_HIDE)
                fociEffectCount += fociEffect->modifier;
        }
        
        int bonusOdds(993 + UMIN(6, fociEffectCount / 10));
        if (number_range(1, 1000) > bonusOdds)
        {
            AFFECT_DATA faf = {0};
            faf.where    = TO_OBJECT;
            faf.type     = gsn_createfoci;
            faf.level    = ch->level;
            faf.duration = -1;
            faf.modifier = 1;
            if (number_bits(5) == 0)    faf.location = (number_bits(1) == 0 ? APPLY_HITROLL : APPLY_DAMROLL);
            else                        faf.location = number_bits(1) == 0 ? APPLY_HIT : APPLY_MANA;

            obj_affect_join(foci, &faf);
            act_new("You feel more attuned to $p.", ch, foci, NULL, TO_CHAR, POS_SLEEPING);
        }
    }

    // Check for weather effects
    if ((ch->in_room != NULL && IS_OUTSIDE(ch) && !IS_NPC(ch) && ch->level > 10 && !IS_IMMORTAL(ch))
    && ((paf = affect_find(ch->in_room->affected,gsn_encamp)) == NULL || paf->modifier != ch->id))
    {
        // Check for hail
	    if (ch->in_room->area->w_cur.storm_str > 0 && ch->in_room->area->w_cur.precip_type == 1
	    && (number_percent() < (ch->in_room->area->w_cur.storm_str / 10)))
	    {
	        if (is_affected(ch,gsn_elementalprotection))
	        {
	            act("Hail pounds the air about you, but you remain unharmed.",ch,NULL,NULL,TO_CHAR);
        		act("Hail swirls around $n, but $e remains unharmed.",ch,NULL,NULL,TO_ROOM);
	        }
	        else
	        {
		        pMob = create_mobile(get_mob_index(MOB_VNUM_HAIL));
        		char_to_room(pMob, ch->in_room);
		        global_bool_check_avoid = FALSE;
        		damage( pMob, ch, number_range(1, ch->in_room->area->w_cur.storm_str), TYPE_HIT, DAM_COLD, TRUE);
		        global_bool_check_avoid = TRUE;
        		extract_char(pMob, TRUE);
	        }
	    }

        // Check for lightning
    	if (ch->in_room->area->w_cur.lightning_str >= 50 && number_range(1, 100) <= (is_affected(ch, gsn_ionize) ? 16 : 1))
	    {
	        if (is_affected(ch,gsn_elementalprotection))
	        {
		        act("Lightning races down from the sky, forking over $n before striking the ground.",ch,NULL,NULL,TO_ROOM);
		        act("Lightning races down from the sky, forking over you before striking the ground.",ch,NULL,NULL,TO_CHAR);
	        }
	        else
	        {
                act("Lightning races down from the sky, striking you with a powerful jolt!", ch, NULL, NULL, TO_CHAR);
                act("Lightning races down from the sky, striking $n with a powerful jolt!", ch, NULL, NULL, TO_ROOM);

                int effLevel(51);
                if (ch->level < 25) effLevel = ch->level;
                sourcelessDamage(ch, "the lightning", dice(3, effLevel * 2), gsn_electricalstorm, true);
                check_conduitoftheskies(ch);
	        }
	    }

        // Determine the effective temperature
        int warmMod(room_is_affected(ch->in_room, gsn_lavaforge) ? 5 : 0);
        for (OBJ_DATA * wObj = ch->carrying; wObj; wObj = wObj->next_content)
        {
            // Each warm item increases the effective temperature
            if ((wObj->worn_on || ch->on == wObj) && IS_SET(wObj->extra_flags[0], ITEM_WARM))
                warmMod += number_range(3, 5);
        }
        int effectiveTemp((ch->in_room->area->w_cur.temperature + warmMod));
        int temp_dam(1 + ((abs(effectiveTemp) - 23) * (ch->level >= 25 ? 4 : 2)));

        // Check for cold
    	if (effectiveTemp <= -23 && !BIT_GET(ch->pcdata->traits, TRAIT_COLDNATURE))
	    {
            if (get_obj_list(ch,"campfire",ch->in_room->contents))
		        act("The campfire keeps the cold at bay.",ch,NULL,NULL,TO_CHAR);
    		else if (is_affected(ch,gsn_elementalprotection))
	    	    act("The freezing air swirls around you, but you remain unharmed.",ch,NULL,NULL,TO_CHAR);
            else if (number_percent() <= get_skill(ch, gsn_warmth))
            {
                act("The freezing air swirls about you, but cannot disturb your inner warmth.", ch, NULL, NULL, TO_CHAR);
                check_improve(ch, NULL, gsn_warmth, TRUE, 8);
            }
            else if (number_percent() <= get_skill(ch, gsn_frostkin))
            {
                act("The freezing air swirls about you, its chill offering you no harm.", ch, NULL, NULL, TO_CHAR);
                check_improve(ch, NULL, gsn_frostkin, true, 8);
            }
        	else	
		    {
    		    pMob = create_mobile(get_mob_index(MOB_VNUM_COLD));
	            char_to_room(pMob, ch->in_room);
	            global_bool_check_avoid = FALSE;
	            damage(pMob, ch, temp_dam, TYPE_HIT, DAM_COLD, TRUE);
	            global_bool_check_avoid = TRUE;
    	        extract_char(pMob, TRUE);
	    	}
	    }

        // Check for heat; don't count warm objects for now
	    if (ch->in_room->area->w_cur.temperature >= 23)
	    {
		    if (is_affected(ch,gsn_elementalprotection))
		        act("The air around you shimmers with heat, but you remain cool.",ch,NULL,NULL,TO_CHAR);
            else if (number_percent() < get_skill(ch, gsn_flameheart))
                act("The air around you shimmers with heat, but it is no match for the fire in your heart.", ch, NULL, NULL, TO_CHAR);
    		else
	    	{
	    	    pMob = create_mobile(get_mob_index(MOB_VNUM_HEAT));
	    	    char_to_room(pMob, ch->in_room);
	    	    global_bool_check_avoid = FALSE;
	    	    damage(pMob, ch, temp_dam, TYPE_HIT, DAM_FIRE, TRUE);
	            global_bool_check_avoid = TRUE;
	            extract_char(pMob, TRUE);
		    }
	    }
	}

	if (ch->in_room
	 && !IS_IMMORTAL(ch)
	 && (!IS_NPC(ch) || (!IS_AFFECTED(ch, AFF_WIZI) && !IS_SET(ch->act, ACT_SENTINEL)))
	 && room_is_affected(ch->in_room, gsn_wallofair)
	 && (number_bits(1) == 0)
	 && (ch->class_num != global_int_class_airscholar)
	 && (ch->class_num != global_int_class_airtemplar)
	 && !is_affected(ch, gsn_anchor))
	{
	    int x = number_range(0, 5);

	    if (ch->in_room->exit[x]
	     && !IS_SET(ch->in_room->exit[x]->exit_info, EX_CLOSED)
	     && !IS_SET(ch->in_room->exit[x]->exit_info, EX_WALLED)
	     && !IS_SET(ch->in_room->exit[x]->exit_info, EX_FAKE))
	    {
		send_to_char("The fierce winds from the wall of air hammer at you!\n\r", ch);
		act("The fierce winds from the wall of air hammer at $n!", ch, NULL, NULL, TO_ROOM);
	        move_char(ch, x, FALSE);
	    }
	}

	if (ch->in_room && IS_NPC(ch) && (ch->pIndexData->vnum == MOB_VNUM_ZOMBIE)
	 && ch->in_room->area && ((paf = affect_find(ch->in_room->area->affected, gsn_deathwalk)) != NULL)
	 && paf->point)
	{
	    CHAR_DATA *tree = (CHAR_DATA *) paf->point;

	    if (IS_VALID(tree) && tree->in_room && (tree->in_room != ch->in_room))
	    {
		if (paf->duration == 0)
		{
		    act("$n sinks into the ground.", ch, NULL, NULL, TO_ROOM);
		    char_from_room(ch);
		    char_to_room(ch, tree->in_room);
		    act("$n rises up from the ground.", ch, NULL, NULL, TO_ROOM);
		}
		else
		{
            if (!StepAlongPath(ch))
	        {
                RoomPath roomPath(*ch->in_room, *tree->in_room, ch, 3);
                if (roomPath.Exists())
                {
                    AssignNewPath(ch, roomPath);
                    StepAlongPath(ch);
                }
		    }
		}
	    }
	}

	if (ch->song)
    {
	    if (ch->harmony)
            expend_mana(ch, skill_table[ch->harmony->type].min_mana / 2);

	    expend_mana(ch, skill_table[ch->song->type].min_mana);
	}

	/* psionics stuff */
	if (!IS_NPC(ch))
	{
	    int x, bchance;
	    char buf[MAX_STRING_LENGTH];
	    bool bdeath = FALSE, bLink = FALSE;

	    if (ch->mindlink)
        {
            if (ch->in_room && ch->mindlink->in_room && ch->in_room->area == ch->mindlink->in_room->area)
                bLink = saves_focus(ch->level, ch, ch->mindlink, FALSE);
            else
                bLink = saves_focus(ch->level, ch, ch->mindlink, TRUE);

            if (bLink)
            {
                act("Your mindlink to $N slips away.", ch, NULL, ch->mindlink, TO_CHAR);
                act("You feel $n's mindlink leave you.", ch, NULL, ch->mindlink, TO_VICT);
                ch->mindlink = NULL;
            }
        }

	    for (x = 0; x < MAX_FOCUS; x++)
        {
            while (ch->pcdata->focus_on[x] && ch->pcdata->focus_ch[x] 
            && (ch->pcdata->focus_ch[x] != ch) 
            && (skill_table[ch->pcdata->focus_sn[x]].target != TAR_CHAR_DEFENSIVE) 
            && (ch->pcdata->focus_sn[x] != gsn_suggestion) 
            && (ch->pcdata->focus_sn[x] != gsn_cloak)
            && saves_focus(ch->level, ch, ch->pcdata->focus_ch[x], false))
            {
                sprintf(buf, "You wrest your mind free of $n's %s.", skill_table[ch->pcdata->focus_sn[x]].name);
                act(buf, ch, NULL, ch->pcdata->focus_ch[x], TO_VICT);
                sprintf(buf, "$N wrests $Mself free of your %s.", skill_table[ch->pcdata->focus_sn[x]].name);
                act(buf, ch, NULL, ch->pcdata->focus_ch[x], TO_CHAR);
                if ((bchance = get_skill(ch->pcdata->focus_ch[x], gsn_backlash)) > 0)
                {
                    if (number_percent() < bchance && (number_bits(1) == 0))
                    {
                        check_improve(ch->pcdata->focus_ch[x],NULL, gsn_backlash, TRUE, 1);
                        damage_old(ch->pcdata->focus_ch[x], ch, number_fuzzy(ch->pcdata->focus_ch[x]->level), gsn_backlash, DAM_MENTAL, TRUE);
                        if (IS_OAFFECTED(ch, AFF_GHOST))
                        {
                            bdeath = TRUE;
                            break;
                        }
                    }
                    else
                        check_improve(ch->pcdata->focus_ch[x], NULL,gsn_backlash, FALSE, 2);
                }

                if (ch->pcdata->focus_sn[x] == gsn_dominance)
                    stop_follower(ch);

                if (!bdeath)
                    unfocus(ch, x, TRUE);
            }
        
            if (bdeath)
                break;

            if (ch->pcdata->focus_on[x])
                if (ch->mana < skill_table[ch->pcdata->focus_sn[x]].min_mana)
                {
                send_to_char("Out of energy, you stop exerting your psionic abilities.\n\r", ch);
                while (ch->pcdata->focus_on[0])
                    unfocus(ch, 0, TRUE);
                ch->mana = 0;
                break;
                }
                else
                    expend_mana(ch, skill_table[ch->pcdata->focus_sn[x]].min_mana);
        }
	}

	if (is_affected(ch, gsn_infernofury))
	{
	    send_to_char("The fury of the inferno {rburns{x within you.\n\r",ch);
	    damage_old(ch, ch, number_range(ch->level*3/4, ch->level*5/4), gsn_infernofury, DAM_FIRE, FALSE);
	}

	if ( ch->valid && ch->in_room && ch->position == POS_MORTAL )
	    damage( ch, ch, 1, TYPE_UNDEFINED, DAM_NONE,FALSE);

	if ( ch->valid && ch->in_room
         && (ch->position == POS_STUNNED || ch->position == POS_INCAP))
	{
	    ch->hit++;
	    if (ch->hit > 0)
	        if (!IS_AFFECTED(ch, AFF_SLEEP))
	     	    update_pos(ch);
	   	else
		    switch_position(ch, POS_SLEEPING);
	}

	if (ch->position < POS_SLEEPING)
	    continue;

	if ( ch->position >= POS_STUNNED )
	{
            /* check to see if we need to go home */
            if (IS_NPC(ch) && ch->zone != NULL && ch->zone != ch->in_room->area
            && ch->desc == NULL &&  ch->fighting == NULL 
	    && !IS_AFFECTED(ch,AFF_CHARM) && number_percent() < 5)
            {
            	act("$n wanders on home.",ch,NULL,NULL,TO_ROOM);
            	extract_char(ch,TRUE);
            	continue;
            }

	    if (IS_NPC(ch))
		{
			mprog_time_trigger(ch);
			mprog_tick_trigger(ch);
		}

	    if (IS_AFFECTED(ch, AFF_REGENERATION))
	        spell_mendwounds(gsn_mendwounds, ch->level, ch, (void *) ch, TARGET_CHAR);

	    if ( ch->hit  < ch->max_hit )
		ch->hit  += hit_gain(ch);
	    else
		ch->hit = ch->max_hit;

	    if ( ch->mana < ch->max_mana )
		ch->mana += mana_gain(ch);
	    else
		ch->mana = ch->max_mana;

	    if ( ch->move < ch->max_move )
		ch->move += move_gain(ch);
	    else
		ch->move = ch->max_move;
	}

    // Check for fury of Ada'Chemta'Boghor
    PyreInfo * pyreInfo(getPyreInfoEffect(ch, PyreInfo::Effect_FuryOfAdaChemtaBoghor));
    if (pyreInfo != NULL)
    {
        // Make sure the pyre is still around
        OBJ_DATA * pyre(pyreInfo->pyre());
        if (verify_obj_world(pyre) && pyre->in_room != NULL)
        {
            // Check for a chance to spawn a fire elemental
            if (number_percent() < static_cast<int>(pyreInfo->effectModifier() * (pyreInfo->isEffectGreater() ? 2 : 1)))
            {
                CHAR_DATA * elemental = create_mobile(get_mob_index(MOB_VNUM_FIREELEMENTAL));
                elemental->level = ch->level;
                char_to_room(elemental, pyre->in_room);
                pyreInfo->setEffectModifier(0);

                act("The pyre flares brightly for moment as $n materializes through it!", elemental, NULL, NULL, TO_ROOM);
            }
            else
                pyreInfo->setEffectModifier(pyreInfo->effectModifier() + 1);

            // Check for a chance to fill the pyre room with smoke
            if (number_percent() < (pyreInfo->isEffectGreater() ? 3 : 2))
                fillRoomWithSmoke(ch, pyre->in_room, ch->level, 3);
        }
    }

	if ( ch->position == POS_STUNNED )
	    update_pos( ch );

        if (IS_OAFFECTED(ch, AFF_POSCHAN) && !IS_NPC(ch))
        { 
            expend_mana(ch, 50);
            if (ch->mana < 1)
            {
                ch->mana = 0;
                for ( paf = ch->affected; paf != NULL; paf = paf_next )
                    {
                        paf_next    = paf->next;
                        if (paf->bitvector == AFF_POSCHAN) 
                            dur = paf->modifier;
                    }
                if (saves_spell(ch->level, NULL, ch, DAM_HOLY))
                {
                    send_to_char("Your magic drained, you can no longer hold the channel open.\n\r",ch);
                    send_to_char("The power lent to you rushes from your body, dissipating rapidly.\n\r",ch);
                    send_to_char("Your body feels weak and tired, stripped of the power of the channel.\n\r",ch);
                    act("A soft blue glow pulses in $n, then fades away.",ch,NULL,NULL,TO_ROOM);
  	            act("$n stumbles in exhaustion, but catches $mself.",ch,NULL,NULL,TO_ROOM);
    
                    affect_strip (ch,gsn_poschan);

                    naf.where     = TO_AFFECTS;
    	            naf.type      = gsn_poschan;
    	            naf.level     = ch->level;
    	            naf.modifier  = -50*dur;
    	            naf.location  = APPLY_HIT;
    	            naf.duration  = dur;
    	            naf.bitvector = 0;
    	            affect_to_char(ch,&naf);

                    naf.modifier = -50*dur;
                    naf.location = APPLY_MANA;
                    naf.duration = dur;
                    naf.bitvector = 0;
                    affect_to_char(ch,&naf);

                }
                else
                {
                    send_to_char("Your magic drained, you can no longer hold the channel open .\n\r",ch);
                    send_to_char("The power lent to you rushes from your body, dissipating rapidly.\n\r",ch);
                    send_to_char("You collapse from the prolonged strain of holding the channel open!\n\r",ch);
                    act("A soft blue glow pulses in $n, then fades away.",ch,NULL,NULL,TO_ROOM);
     	            act("$n stumbles in exhaustion, and collapses.",ch,NULL,NULL,TO_ROOM);
                
 
                    affect_strip(ch,gsn_poschan);
                     
                    naf.where     = TO_AFFECTS;
    	            naf.type      = gsn_poschan;
    	            naf.level     = ch->level;
    	            naf.modifier  = -50*dur;
    	            naf.location  = APPLY_HIT;
    	            naf.duration  = dur;
    	            naf.bitvector = 0;
    	            affect_to_char(ch,&naf);

                    naf.modifier = -50*dur;
                    naf.location = APPLY_MANA;
                    naf.duration = dur;
                    naf.bitvector = 0;
                    affect_to_char(ch,&naf);

                    switch_position(ch, POS_SLEEPING);
		    naf.type	  = gsn_lucid;
                    naf.location  = APPLY_HIDE;
                    naf.duration  = dur/2;
                    naf.bitvector = AFF_SLEEP;
                    affect_to_char(ch,&naf); 
                }   
            }   
        } 
	if (is_affected(ch, gsn_wariness))
	{
	    expend_mana(ch, skill_table[gsn_wariness].min_mana);
	    if (ch->mana < 1)
	    {
		ch->mana = 0;
		affect_strip(ch, gsn_wariness);
		send_to_char("You're too tired to continue being wary.\n\r",ch);
	    }
	}

    int avatarType(type_of_avatar(ch));
	if (avatarType != -1)
	{
	    if (avatarType == gsn_avataroftheannointed) expend_mana(ch, 120);
        else expend_mana(ch, 100);

	    if (ch->mana < 1)
	    {
	        ch->mana = 0;
            strip_avatar(ch);
	        if (ch->long_descr)
                free_string(ch->long_descr);
	        ch->long_descr = &str_empty[0];
	        send_to_char("Out of energy, you return from avatar form, exhausted and sore.\n\r", ch);
	        ch->hit = UMAX(ch->hit/5,1);
	        if(ch->position == POS_STANDING)
            {
                switch_position(ch, POS_RESTING);
	            act("The light blazing around $n suddenly extinguishes, and $e collapses, exhausted.", ch, NULL, NULL, TO_ROOM);
            }
            else
                act("The light blazing around $n suddenly extinguishes.",ch, NULL, NULL, TO_ROOM);

	        naf.where     = TO_NAFFECTS;
	        naf.type      = avatarType;
       	 	naf.level     = ch->level;
       	 	naf.duration  = 32;
        	naf.location  = APPLY_HIT;
        	naf.modifier  = ch->max_hit / -10;
    		naf.bitvector = AFF_AVATAR;
        	affect_to_char(ch, &naf);

            naf.location  = APPLY_MANA;
            naf.modifier  = ch->max_mana / -10;
            affect_to_char(ch, &naf);

            if (number_bits(1) == 0)
            {
                send_to_char("The strain proves too much, and you slip into unconsciousness!\n", ch);
                act("$n's expression seems to seize up right before $e slips into unconsciousness!", ch, NULL, NULL, TO_ROOM);

                switch_position(ch, POS_SLEEPING);
                naf.type      = gsn_lucid;
                naf.location  = APPLY_HIDE;
                naf.duration  = number_range(1, 3);
                naf.bitvector = AFF_SLEEP;
                affect_to_char(ch,&naf);
            }
	    }
    }
	if (is_affected(ch,gsn_warpath))
	{
	    if (ch->mana == 0)
	    {
		send_to_char("You tire from your warpath.\n\r",ch);
		affect_strip(ch,gsn_warpath);
	    }
	    else
		expend_mana(ch, 10);
	}
	if ((paf = affect_find(ch->affected,gsn_huntersense)) != NULL)
	    if (ch->in_room
	      && ch->in_room->sector_type != SECT_CITY
	      && ch->in_room->sector_type != SECT_INSIDE)
	    {
	    	if (paf->point)
	            expend_mana(ch, 15);
	    	else
	            expend_mana(ch, 25);
		if (ch->mana < 0)
		{
		    ch->mana = 0;
		    affect_strip(ch,gsn_huntersense);
		    send_to_char("You stop searching for tracks.\n\r",ch);
		}
	    }

	if ((paf = affect_find(ch->affected,gsn_pursuit)) != NULL)
	    if (ch->in_room
	      && (ch->in_room->sector_type == SECT_CITY
	      || ch->in_room->sector_type == SECT_INSIDE
	      || ch->in_room->sector_type == SECT_ROAD))
	    {
	    	if (paf->point)
	            expend_mana(ch, 15);
	    	else
	            expend_mana(ch, 25);
		if (ch->mana < 0)
		{
		    ch->mana = 0;
		    affect_strip(ch,gsn_pursuit);
		    send_to_char("You stop searching for tracks.\n\r",ch);
		}
	    }

	if (is_affected(ch, gsn_astralprojection))
	{
	    expend_mana(ch, 100);
	    if (ch->mana < 1)
	    {
	        ch->mana = 0;
	        act("Out of energy, you return to your body lest you suffer death on the fringe of the astral plane.", ch, NULL, NULL, TO_CHAR);
            for (CHAR_DATA * echoChar(ch->in_room == NULL ? NULL : ch->in_room->people); echoChar != NULL; echoChar = echoChar->next_in_room)
            {
                if (echoChar != ch && can_see(echoChar, ch))
	                act("$n shivers once, then phases from view.", ch, NULL, echoChar, TO_VICT);
            }

	        extract_char(ch, TRUE);
	        return;
	    }
    }

	if (is_affected(ch, gsn_guard))
	{
	    if (ch->mana < skill_table[gsn_guard].min_mana)
	    {
	        act("$n grows tired of guarding, and relaxes.", ch, NULL, NULL, TO_ROOM);
	        act("You grow tired from guarding, and relax.", ch, NULL, NULL, TO_CHAR);
	        affect_strip(ch, gsn_guard);
	    }
	    else
	        expend_mana(ch, skill_table[gsn_guard].min_mana);
	}

    // Check the shade timer; returns true if the ch was extracted
    if (ShadeControl::CheckShadeTimer(*ch))
        continue;

	if ( !IS_NPC(ch) && ++ch->timer >= 12 && !IS_IMMORTAL(ch))
	{
	    if (is_affected(ch, gsn_meldwithstone) || is_affected(ch, gsn_shadowmastery) || is_affected(ch, gsn_maze) || is_affected(ch, gsn_flameunity))
	        ch->timer = 0;
	    else if ( ch->was_in_room == NULL && ch->in_room != NULL )
	    {
	        ch->was_in_room = ch->in_room;
	        if ( ch->fighting != NULL )
	   	    stop_fighting_all(ch);
		act( "$n disappears into the void.", ch, NULL, NULL, TO_ROOM );
		send_to_char( "You disappear into the void.\n\r", ch );
		if (ch->level > 1)
		    save_char_obj( ch );
		char_from_room( ch );
		char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) );
	    }
	}

#if defined(unix)

// I really need to make this into a less brutal hack.

	if (!IS_NPC(ch) && (ch->timer > 0) && ((ch->timer % 4) == 0)
         && IS_IMMORTAL(ch) && ch->desc)
	{
	    write_to_descriptor(ch->desc->descriptor,telnet_nop,0);
	}
#endif

	if ( !IS_NPC(ch) && ch->level < LEVEL_IMMORTAL )
	{
	    OBJ_DATA *obj, *obj_next;
	    AFFECT_DATA af;
        af.valid = TRUE;
        af.point = NULL;

	    if (ch->in_room && is_affected(ch, gsn_devouringspirit))
	    {
		bool eating = FALSE;

		if (ch->pcdata->condition[COND_HUNGER] <= (get_max_hunger(ch) / 3))
		{
		    for (obj = ch->in_room->contents; obj; obj = obj_next)
		    {
			obj_next = obj->next_content;

			if ((obj->item_type == ITEM_CORPSE_PC)
			 || (obj->item_type == ITEM_CORPSE_NPC))
			{
			    eating = TRUE;
			    act("Overwhelmed by hunger, you lunge at $p, gnawing at it!", ch, obj, NULL, TO_CHAR);
			    act("$n lunges at $p, and begins to gnaw hungrily upon it!", ch, obj, NULL, TO_ROOM);
			    gain_condition(ch, COND_HUNGER, 80);

			    if (!IS_AFFECTED(ch, AFF_POISON) && number_bits(1) == 0)
			    {
				af.where     = TO_AFFECTS;
    				af.type      = gsn_poison;
    				af.level     = ch->level;
    				af.duration  = ch->level/8;
    				af.location  = APPLY_STR;
    				af.modifier  = -2;
    				af.bitvector = AFF_POISON;
    				affect_join( ch, &af );
    				send_to_char( "You feel very sick.\n\r", ch );
    				act("$n looks very ill.",ch,NULL,NULL,TO_ROOM);
			    }
			
			    break;
			}
		    }
		}

		if (!eating && (ch->pcdata->condition[COND_HUNGER] <= (get_max_hunger(ch) / 3)))
		{
		    for (obj = ch->in_room->contents; obj; obj = obj_next)
		    {
			obj_next = obj->next_content;

			if ((obj->item_type == ITEM_FOOD)
			 && CAN_WEAR(obj, ITEM_TAKE)
			 && can_see_obj(ch, obj))
			{
			    act("Overwhelmed by hunger, you sweep up $p, devouring it!", ch, obj, NULL, TO_CHAR);
			    act("$n sweeps up $p, devouring it quickly.", ch, obj, NULL, TO_ROOM);
			    obj_from_room(obj);
			    obj_to_char(obj, ch);
			    do_eat(ch, "1.");
			    break;
			}
		    }
		}
	    }

	    gain_condition( ch, COND_DRUNK,  -1 );
//	    gain_condition( ch, COND_FULL, ch->size > SIZE_MEDIUM ? -4 : -2 );
	    if (!is_affected(ch, gsn_sustenance))
	    {
	        gain_condition( ch, COND_THIRST, -1 );
	        gain_condition( ch, COND_HUNGER, -10 * pc_race_table[ch->race].race_hunger / ch->pcdata->food_quality);
	    }
	}

	for ( paf = ch->affected; paf != NULL; paf = paf_next )
	{
	    paf_next	= paf->next;

            if ((paf->type == gsn_poschan)
             && (paf->bitvector == AFF_POSCHAN))
                    paf->modifier += 1;

	    if ( paf->duration > 0 )
	    {
		paf->duration--;

		if (paf->type == gsn_leech)
		{
		    int drain;
		    CHAR_DATA *lch;

		    drain = UMIN(number_range(1, paf->level), ch->mana);
		    expend_mana(ch, drain);
		    lch = (CHAR_DATA *) paf->point;
		    lch->mana = UMIN(lch->max_mana, lch->mana + drain);
		}

		if (paf->type == gsn_infectiousaura && ch->in_room)
		{
		    CHAR_DATA *ach=NULL;
		    AFFECT_DATA daf;
		    for (ach = ch->in_room->people;ach;ach=ach->next)
		    {
			if (!is_safe_spell(ach,ch,TRUE)
			  && !is_affected(ach,gsn_fever) 
			  && !saves_spell(paf->level, NULL, ach,DAM_DISEASE))
			{
			    daf.valid = TRUE;
			    daf.point = NULL;
			    daf.type = gsn_fever;
			    daf.level=paf->level;
			    daf.bitvector=AFF_PLAGUE;
			    daf.modifier = -2;
			    daf.location = APPLY_STR;
			    daf.duration = paf->level/10;
			    daf.where = TO_AFFECTS;
			    affect_to_char(ach,&daf);
			    act("You begin to feel weak and feverish.",ach,NULL,NULL,TO_CHAR);
			    act("$n begins sweating profusely.",ach,NULL,NULL,TO_ROOM);
			}
		    }
		}

		if ((paf->type == gsn_nova) && (paf->bitvector != AFF_NOVA_CHARGE))
		    paf->modifier -= 1;

		if (paf->type == gsn_rynathsbrew && paf->modifier > 0)
		{
			act("The acidic brew continues to eat at you!", ch, NULL, NULL, TO_CHAR);
			act("$n winces as the acidic brew on $m continues to pain $m.", ch, NULL, NULL, TO_ROOM);
			damage_old(ch, ch, paf->modifier, gsn_rynathsbrew, DAM_ACID, TRUE);
		}

        if (paf->type == gsn_causticblast && paf->modifier > 0 && paf->location == APPLY_NONE)
        {
            act("You are burned by the caustic acid covering you!", ch, NULL, NULL, TO_CHAR);
            act("$n is burned by the caustic acid covering $m!", ch, NULL, NULL, TO_ROOM);
            damage_old(ch, ch, paf->modifier, gsn_causticblast, DAM_ACID, true);
        }

		if (paf->where == TO_NAFFECTS && paf->bitvector == AFF_ASHURMADNESS)
		{
		    send_to_char("Your ravenous hunger fades.",ch);
		}

		if (paf->type == gsn_teleportcurse)
		{
			int prevmove = ch->move;
			if (number_percent() < 20)
			{
			    act("You feel the grip of the forces of chaos momentarily tighten!", ch, NULL, NULL, TO_CHAR);
			    spell_teleport(gsn_teleportcurse, ch->level, ch, NULL, TAR_CHAR_SELF);
			    ch->move = prevmove;
			}
		}

        if (paf->type == gsn_curseofeverchange && number_percent() <= 40)
        {
            // Get a target room
            ROOM_INDEX_DATA * teleRoom(get_random_room(ch));

            // Perform checks to avoid abuse and weirdness
            if (ch->in_room == NULL || teleRoom == NULL || IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
            || IS_SET(ch->in_room->room_flags, ROOM_NOGATE) || is_affected(ch, gsn_matrix)
            || (ch->in_room->area->area_flags & AREA_UNCOMPLETE) || IS_AFFECTED(ch, AFF_CURSE)
            || (IS_NPC(ch) && (IS_SET(ch->act, ACT_NOSUBDUE) || IS_SET(ch->act, ACT_SENTINEL) || IS_SET(ch->nact, ACT_SHADE))))
                send_to_char("You feel the forces of chaos tighten their grip momentarily, but nothing happens.\n", ch);
            else
            {
                // Teleport time!
                send_to_char("You feel the forces of chaos tighten their grip, whisking you away to a new place!\n", ch);
                act("A gust of wind sweeps through this place, and $n suddenly vanishes!", ch, NULL, NULL, TO_ROOM);
                char_from_room(ch);
                char_to_room(ch, teleRoom);
                act("A gust of wind sweeps through this place, and suddenly $n appears!", ch, NULL, NULL, TO_ROOM);
                do_look(ch, "auto");
            }
        }
		
		if (paf->type == gsn_cateyes)
		{
			if (is_affected(ch, gsn_detecthidden) || is_affected(ch, gsn_perception) || number_percent() < 33) 
				SET_BIT(ch->affected_by, AFF_DETECT_HIDDEN);
			else REMOVE_BIT(ch->affected_by, AFF_DETECT_HIDDEN);
		}

        // Check for ward of frost damage
        if (paf->type == gsn_wardoffrost)
            sourcelessDamage(ch, "The ward of frost", number_range(10, 30), gsn_wardoffrost, DAM_COLD); 

        // Check for Chillfire Shield cooldown (only if not fighting)
        if (paf->type == gsn_chillfireshield && paf->modifier > 0 && ch->fighting == NULL)
        {
            // Drop 10% (and less an additional 1) of the heat
            int prevMod(paf->modifier);
            paf->modifier = UMAX(0, ((paf->modifier * 9) / 10) - 1);
            if (paf->modifier == 0) send_to_char("Your chillfire shield cools completely as the last of its heat disperses.\n", ch);
            else if (prevMod >= 200 && paf->modifier < 200) send_to_char("The flames around your force shield grow noticeably cooler and dimmer.\n", ch);
            else if (prevMod >= 350 && paf->modifier < 350) send_to_char("Your force shield stops humming as the dark flames around it cool.\n", ch);
            else if (prevMod >= 500 && paf->modifier < 500)
            {
                act("The dark flames covering the force shield around $n begin to flicker more slowly, radiating only a little heat.", ch, NULL, NULL, TO_ROOM);
                act("The dark flames covering the force shield around you begin to flicker more slowly, radiating only a little heat.", ch, NULL, NULL, TO_CHAR);
            }
            else if (prevMod >= 700 && paf->modifier < 700)
            {
                act("The flames surrounding the force shield around $n grow dim once more, producing noticeably less heat.", ch, NULL, NULL, TO_ROOM);
                act("The flames surrounding the force shield around you grow dim once more, producing noticeably less heat.", ch, NULL, NULL, TO_CHAR);
            }
            else if (prevMod >= 1000 && paf->modifier < 1000)
            {
                act("The force shield around $n dims a few shades as it grow slightly cooler.", ch, NULL, NULL, TO_ROOM);
                act("The force shield around you dims a few shades as it grow slightly cooler.", ch, NULL, NULL, TO_CHAR);
            }
        }

        // Check for Brimstone Conduit
        if (paf->duration > 1 && paf->type == gsn_heartoftheinferno)
        {
            PyreInfo * pyreInfo(getPyreInfoEffect(ch, PyreInfo::Effect_BrimstoneConduit));
            if (pyreInfo != NULL)
            {
                // Potentially drop the duration
                int skill = get_skill(ch, gsn_bloodpyre);
                if (number_percent() <= (skill / 4))
                    --paf->duration;

                // Potentially drop it again
                if (paf->duration > 6 && pyreInfo->isEffectGreater() && number_percent() <= (skill / 33))
                {
                    send_to_char("You feel a rush of heat wash over you from your conduit to the Inferno, and a warm clarity fills your mind.\n", ch);
                    paf->duration -= 6;
                }
            }
        }

        // Check for drake maturation
        if (paf->duration > 1 && paf->type == gsn_wakenedstone && ch->position == POS_FIGHTING)
        {
            if (number_bits(1) == 0 || Drakes::HasStonesong(*ch))
                --paf->duration;
        }

        // Check for radiate aura mana drain
        if (paf->type == gsn_radiateaura)
        {
            switch (paf->modifier)
            {
                case Aura_Soothing: expend_mana(ch, 50); break;
                case Aura_Genesis:  expend_mana(ch, 25); break;
                case Aura_Empathy:  expend_mana(ch, 10); break;
                default: bug("Unknown radiate aura modifier in update", 0); break;
            }
            continue;
        }

        // Check for crystallize aether and aura of genesis
        if (paf->duration > 1 && paf->type == gsn_crystallizeaether)
        {
            AFFECT_DATA * spiritAura(get_affect(ch, gsn_radiateaura));
            if (spiritAura != NULL && spiritAura->modifier == Aura_Genesis)
            {
                if (number_percent() <= get_skill(ch, gsn_radiateaura))
                {
                    check_improve(ch, NULL, gsn_radiateaura, true, 2);
                    --paf->duration;
                }
                else
                    check_improve(ch, NULL, gsn_radiateaura, false, 2);
            }
        }

        // Check for seed of madness
        if (paf->type == gsn_seedofmadness && paf->location == APPLY_NONE)
            intensify_seedofmadness(*ch, *paf);

        // Check for unholy might's corrupting influence
        if (paf->type == gsn_unholymight && !IS_NPC(ch))
            modify_karma(ch, 2);

        // Check for drowse
        if (paf->type == gsn_drowse && paf->modifier > 0)
        {
            --paf->modifier;
            if (ch->position > POS_SLEEPING)
            {
                if (paf->modifier > 0) 
                    send_to_char("You fade in and out of consciousness, barely able to keep awake.\n", ch);
                else
                {
                    send_to_char("You cannot keep yourself awake any longer, and collapse into blissful sleep.\n", ch);
                    act("$n suddenly keels over as $e falls into a deep sleep!", ch, NULL, NULL, TO_ROOM);
                    switch_position(ch, POS_SLEEPING);
                }
            }
        }

        // Check for Aspect of the Inferno mana drain
        if (paf->type == gsn_aspectoftheinferno && paf->modifier == 0)
        {
            expend_mana(ch, 60);
            if (ch->mana < 0)
            {
                // Out of mana, cancel the spell
                ch->mana = 0;
                act("A massive rush of heat dissipates from you as you cease taking on the aspect of the Inferno.", ch, NULL, NULL, TO_CHAR);
                act("A rush of heat dissipates suddenly from $n, warming the area.", ch, NULL, NULL, TO_ROOM);
                ch->move = UMIN(number_percent() / 8, ch->move);
                paf->modifier = 1;
                paf->duration = 16 - UMAX(0, (get_skill(ch, gsn_aspectoftheinferno) - 80) / 5);
            }
        }

        // Check for phantasmal mirror
        if (paf->type == gsn_phantasmalmirror)
        {
            if (paf->modifier > 0 && paf->point != NULL)
            {
                // Verify the target
                CHAR_DATA * victim(static_cast<CHAR_DATA*>(paf->point));
                if (ch->in_room == NULL || !verify_char_room(victim, ch->in_room))
                {
                    finishLearningPhantasm(ch, paf);
                    continue;
                }

                // Increment the modifier
                ++paf->modifier;
                if (paf->modifier >= 5)
                {
                    // Finished completely
                    finishLearningPhantasm(ch, paf);
                    continue;
                }
                
                // Send echoes
                switch (paf->modifier)
                {
                    case 2: act("As your magics take hold, an understanding of how to mirror $N begins to form in your mind!", ch, NULL, victim, TO_CHAR); break;
                    case 3: act("Your understanding of how to mirror $N deepens, and you now feel confident you could weave a phantasm of $M!", ch, NULL, victim, TO_CHAR); break;
                    case 4: act("You grow comfortable with the finer nuances of mirroring $N.", ch, NULL, victim, TO_CHAR); break;
                }
            }
        }

        // Check for bewilderment worsening
        if (paf->type == gsn_bewilderment && !saves_spell(paf->level, NULL, ch, DAM_MENTAL))
        {
            send_to_char("Your sense of unease intensifies as your doubt in your abilities worsens!\n", ch);
            paf->modifier += number_range(1, 5);
            paf->modifier = UMIN(50, paf->modifier);
        }

        // Check for displacement save unwinding
        if (paf->type == gsn_displacement)
        {
            int saveMod(reinterpret_cast<int>(paf->point));
            if (saveMod > 0)
                paf->point = reinterpret_cast<void*>(saveMod - 1);
        }

        // Check for quintessence rush mana drain
        if (paf->type == gsn_quintessencerush && paf->modifier >= 0)
        {
            expend_mana(ch, number_range(5, 15));
            continue;
        }

        // Check for cohortsvengeance
        if (paf->type == gsn_cohortsvengeance)
        {
            if (number_percent() <= paf->modifier)
            {
                paf->modifier = 0;
                act("A glowing nimbus of golden light suddenly appears, coalescing into a seraph bent on vengeance!", ch, NULL, NULL, TO_ALL);
                summon_avenging_seraph(UMAX(1, paf->level - 4), ch);
            }
            else
                paf->modifier += 10;
        }

        // Echoes for shadeswarm
        if (paf->type == gsn_shadeswarm && number_bits(4) == 0)
        {
            switch (number_range(0, 6))
            {
                case 0: send_to_char("A cold chill creeps down your back.\n", ch); break;
                case 1: send_to_char("You shiver suddenly, your breath turning frosty as a chill fills you.\n", ch); break;
                case 2: send_to_char("From the corner of your vision you sense a haunting, pale face which vanishes when you turn to look directly at it.\n", ch); break;
                case 3: send_to_char("A soft sobbing sound echoes in your ears from no clear source.\n", ch); break;
                case 4: send_to_char("A sudden shrieking fills your senses, dazing you!\n", ch); WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE)); break;
                case 5: send_to_char("For a moment, your stomache suddenly bulges as though hands were pressing it from the inside, trying to break free!\n", ch); break;
                case 6: send_to_char("The whole world goes black, leaving nothing to your senses for a long moment before slowly fading back in.\n", ch); break;
            }
        }

		if (paf->type == gsn_delayreaction)
		{
			int mana_cost = skill_table[gsn_delayreaction].min_mana;
			int odds_failure = 30, i;
			bool failed = FALSE;
			if (ch->mana < mana_cost)
			{
				send_to_char("You are too tired to keep a wary eye on your reaction.\n\r", ch);
				failed = TRUE;
			}
			expend_mana(ch, mana_cost);
			
			//Failure odds calculus
			odds_failure += (100 - get_skill(ch, gsn_delayreaction));
			odds_failure -= (ch->level / 7);

			for (i = 0; i < 3; i++)
			{
				if (number_percent() < get_skill(ch, gsn_caution) 
					&& ch->mana >= skill_table[gsn_caution].min_mana)
				{
					odds_failure /= 2;
					expend_mana(ch, skill_table[gsn_caution].min_mana);
				}
			}
		
			//Did it fail?
			if (number_percent() < odds_failure)
			{
				act("The reaction begins to boil out of control!", ch, NULL, NULL, TO_CHAR);
				act("The reaction begins to boil out of control!", ch, NULL, NULL, TO_ROOM);
				failed = TRUE;
			}

			//Handle failure or continued success
			if (failed)
			{
				check_improve(ch, NULL, gsn_delayreaction, FALSE, 1);
				if (get_skill(ch, gsn_caution) > 0) check_improve(ch, NULL, gsn_caution, FALSE, 1);
				do_alchemy(ch, (char *)paf->point, paf->modifier, 0, (double)(-1)); //-1 signals explode 
			}
			else
			{
                                check_improve(ch, NULL, gsn_delayreaction, TRUE, 1);
				if (get_skill(ch, gsn_caution) > 0) check_improve(ch, NULL, gsn_caution, TRUE, 1);
				switch (paf->duration)
				{
				 case 5: 
				 act("You continue the arduous labor of setting up the reaction.", 
					ch, NULL, NULL, TO_CHAR);
				 act("$n putters about the laboratory, setting up a reaction.",
					ch, NULL, NULL, TO_ROOM); break;
				 case 4:
				 act("You blend in some of your ingredients, maintaining a constant heat.",
					ch, NULL, NULL, TO_CHAR);
				 act("$n blends in some of $s ingredients, maintaining a constant heat.", 
					ch, NULL, NULL, TO_ROOM); break;
				 case 3:
				 act("You carefully add a catalyst to your concoction.", ch, NULL, NULL, TO_CHAR);
				 act("$n carefully adds a catalyst to $s concoction.", ch, NULL, NULL, TO_ROOM); break;
				 case 2:
				 act("Keeping a steady eye on your work, you make minute adjustments to your mixture.", 
					ch, NULL, NULL, TO_CHAR);
				 act("$n makes minute adjustments to $s mixture.", ch, NULL, NULL, TO_ROOM); break;
				 case 1:
				 act("As your reaction approaches completion, you tweak a few proportions.", 
					ch, NULL, NULL, TO_CHAR);
				 act("As $n's reaction approaches completion, $e tweaks a few proportions.", 
					ch, NULL, NULL, TO_ROOM); break;
				 default: break;
				}
			}
		}

		if ((paf->type == gsn_demoniccontrol) && (paf->duration < 2) && ch->leader)
		    act("You feel your hold over $N weakening.", ch->leader, NULL, ch, TO_CHAR);

        }
	    else if ( paf->duration < 0 )
        {
            // Check for corpsesense mana drain
            if (paf->type == gsn_corpsesense)
            {
                expend_mana(ch, skill_table[gsn_corpsesense].min_mana);
                continue;
            }

            // Check for call familiar focus
            if (IS_NPC(ch) && ch->memfocus[0] == NULL
            && (paf->type == gsn_callfox || paf->type == gsn_callbat || paf->type == gsn_callraven
            ||  paf->type == gsn_callcat || paf->type == gsn_callserpent || paf->type == gsn_calltoad))
            {
                ch->memfocus[0] = get_char_by_id(paf->modifier);
            }

            // Increase the drain of firedancer
            if (paf->type == gsn_firedancer && number_percent() < 50)
            {
                // The drain increase accelerates as time goes on
                paf->modifier += 10 + (paf->modifier / number_range(3, 7));
                if (paf->modifier <= 50)
                    send_to_char("You feel the flame's tempo quicken as your thoughts keep time.\n", ch);
                else if (paf->modifier <= 150)
                    send_to_char("The pace of your thoughts grows even more frenetic as the wild dance continues!\n", ch);
                else
                    send_to_char("Chaos spreads through your mind as the firedance grows somehow more frenzied!\n", ch);
            }

            // Check for heartfire instant death
            if (paf->type == gsn_heartfire)
            {
                // Determine odds per modifier
                int threshold;
                switch (paf->modifier)
                {
                    case 12: threshold = 986; break; // Breakeven is around 50/85 ticks
                    case 24: threshold = 974; break; // Breakeven is around 26/34 ticks
                    case 36: threshold = 935; break; // Breakeven is around 10/11 ticks
                    default:
                        threshold = 1000;
                        bug("Invalid heartfire modifier found", 0);
                        send_to_char("There is a problem with heartfire; please inform the gods.\n", ch);
                        break;
                }

                // Give a little bonus for higher skill
                threshold += ((get_skill(ch, gsn_heartfire) - 70) / 5);

                // Check against the threshold
                srand(time(0));
                int randomValue(rand() % 1000);
                if (randomValue >= threshold)
                {
                    act("The fire in your heart grows too wild for you to control!", ch, NULL, NULL, TO_CHAR);
                    act("Boiling blood courses through your veins as flames spread over your body, consuming it greedily!", ch, NULL, NULL, TO_CHAR);
                    act("$n glows red, then suddenly bursts into wild flame!", ch, NULL, NULL, TO_ROOM);
                    act("The fire consumes $m greedily, rendering $s body to mere ash in a matter of seconds!", ch, NULL, NULL, TO_ROOM);
                    raw_kill(ch);

                    // Log this
                    std::ostringstream mess;
                    mess << ch->name << " just died from heartfire [threshold: " << threshold << "] [value: " << randomValue << "]";
                    wiznet(const_cast<char*>(mess.str().c_str()), NULL, NULL, WIZ_DEATHS, 0, 0);
                    log_string(mess.str().c_str());
                    break;
                }
                else if (randomValue >= (2 * threshold - 1000))
                {
                    send_to_char("You feel a pain in your chest as the fire within you burns almost too hot for you to handle! Your heart threatens to burst!\n", ch);
                    send_to_char("With effort, you lock your will around the raging flames, staving off death for the moment.\n", ch);
                }
                else if (randomValue >= (4 * threshold - 3000))
                    send_to_char("Your chest tightens briefly as a burning sensation spreads through it, then fades.\n", ch);
            }

            // Check for Weavesense mana drain
            if (paf->type == gsn_weavesense)
            {
                int manaCost(UMAX(50, 150 - paf->modifier));
                if (number_percent() < get_skill(ch, gsn_weavecraft))
                    manaCost /= 2;

                expend_mana(ch, manaCost);
                if (ch->mana < 0)
                {
                    // Out of mana, cancel the weavesense
                    ch->mana = 0;
                    send_to_char("You grow too weary to maintain your weavesense, and slowly the flows of Quintessence fade from your sight.\n", ch);
                    affect_remove(ch, paf);
                    continue;
                }
            }

            // Check for ward of the shieldbearer mana drain
            if (paf->type == gsn_wardoftheshieldbearer)
            {
                expend_mana(ch, 20);
                if (ch->mana < 0)
                {
                    // Out of mana, cancel the ward
                    ch->mana = 0;
                    send_to_char("You grow too weary to continue warding your groupmates, and slowly the nimbus of silvery light about you fades.\n", ch);
                    affect_remove(ch, paf);
                    continue;
                }
            }

            // Check for drown
            if (paf->type == gsn_drown)
            {
                // Chance of coughing it up
                if (number_percent() <= 30)
                {
                    act("You cough up the water in your lungs, taking in the air with big gasping breaths!", ch, NULL, NULL, TO_CHAR);
                    act("$n coughs up a mouthful of water, then starts drawing in the air in big gasping breaths!", ch, NULL, NULL, TO_ROOM);
                    affect_remove(ch, paf);
                    continue;
                }

                // Did not cough it up
                if (is_affected(ch, gsn_waterbreathing))
                {
                    if (paf->modifier != 0)
                    {
                        send_to_char("You breathe more easily despite the water in your lungs.\n", ch);
                        paf->modifier = 0;
                    }
                }
                else
                {
                    if (paf->modifier == 0)
                        send_to_char("Your lungs begin to burn from the lack of oxygen.\n", ch);
                    else
                    {
                        send_to_char("You choke and spew, but cannot cough up the water in your lungs!\n", ch);
                        act("$n chokes and gags, spitting up small bits of water.", ch, NULL, NULL, TO_ROOM);
                        damage(ch, ch, dice(paf->level, paf->modifier * paf->modifier), gsn_drown, DAM_DROWNING, true);
                    }

                    // Intensify the effect
                    ++paf->modifier;
                }
            }
		}
        else
	    {
		if ( paf_next == NULL
		||   paf_next->type != paf->type
		||   paf_next->duration > 0 )
		{
		    if ( paf->type > 0 && skill_table[paf->type].msg_off 
		      && skill_table[paf->type].msg_off[0] != '!' 
		      && !(paf->type == gsn_voidwalk && paf->bitvector == 0))
		    {
			send_to_char( skill_table[paf->type].msg_off, ch );
			send_to_char( "\n\r", ch );
		    }
		}
		if (paf->type == gsn_distract)
		{
		    if (paf->point)
			if (((CHAR_DATA *) (paf->point))->in_room == ch->in_room)
			{
			    stop_fighting(ch);
			    check_killer(ch,(CHAR_DATA *) (paf->point));
			    set_fighting(ch,(CHAR_DATA *) (paf->point));
			}
		}
		if (paf->type == gsn_ghost)
		{
		    int cdown = number_range(1, 3);

		    if (ch->perm_stat[STAT_CON] < 4 && !IS_IMMORTAL(ch))
		    {
		        send_to_char("Your spirit tears free of the mortal realm, ascending into the heavens!\n\r", ch);
    			deny_char(ch);
		        continue;
		    }
		    else if ((get_curr_stat(ch, STAT_CON) - cdown) <= 3)
		    {
			affect_strip(ch, gsn_ghost);
			send_to_char("Your ghostly form pulls completely away from this world, killing you!\n\r", ch);
//			ch->perm_stat[STAT_CON] -= 1;
// brazen: Ticket #162: logging ghost deaths
			sprintf(buf,"%s died of being a ghost too long in %s [%i].", ch->name, ch->in_room->name, ch->in_room->vnum);
			log_string(buf);
			raw_kill(ch);
//			send_to_char("You are permanently weakened by the experience.\n\r", ch);
			break;
		    }
		    else
		    {
			send_to_char("Your ghostly form begins to pull away from this world!\n\r", ch);
		        send_to_char("You feel yourself weaken!\n\r", ch);
		        send_to_char("You should return to a healer immediately.\n\r", ch);

		        paf->modifier -= cdown;
		        set_mod_stat(ch, STAT_CON, ch->mod_stat[STAT_CON] - cdown);
		    }

		    while (paf_next && (paf_next->type == gsn_ghost))
			paf_next = paf_next->next;

		    continue;
		}

		else if (paf->type == gsn_slow_cure)
		{
		    ch->hit = UMIN(ch->hit + paf->modifier, ch->max_hit);
		    send_to_char("You feel better!\n\r", ch);
		}
		    
		else if ((paf->type == gsn_abominablerune) && (paf->modifier == 0))
		    send_to_char("You feel ready to place another abominable rune.\n\r", ch);			
                else if (paf->type == gsn_endure)
		{
	    	    if (IS_PAFFECTED(ch,AFF_ENDURE))
		    {
			send_to_char("You feel less prepared to endure blows.\n\r",ch);
                        naf.where     = TO_AFFECTS;
                        naf.type      = gsn_endure;
                        naf.level     = ch->level;
                        naf.duration  = ch->level/10;
                        naf.location  = APPLY_NONE;
                        naf.modifier  = 0;
			naf.bitvector = 0;
                        affect_to_char(ch, &naf);
	    	    }
		    else
			send_to_char("You feel ready to endure blows again.\n\r",ch);
		}
		else if (paf->type == gsn_taunt)
		{
	    	    if (paf->modifier == 0)
			send_to_char("You feel ready to taunt another foe.\n\r",ch);
		    else
			send_to_char("You feel less unnerved.\n\r",ch);
		}
		else if (paf->type == gsn_cloudkill)
		{
		    if (paf->bitvector == AFF_POISON)
			send_to_char("You cough out the last of the noxious fumes.\n\r",ch);
		    else
			send_to_char("You feel ready to summon another poisonous cloud.\n\r",ch);
		}
		else if (IS_NPC(ch) && paf->type == gsn_animaltongues)
		{
		    if (!ch->master)
			continue;
		    else
		    {
			act("Losing interest in $n, $N resumes $S normal behavior.", ch->master, NULL, ch, TO_ROOM);
			act("$N loses interest in you.", ch->master, NULL, ch, TO_CHAR);
			
			stop_follower(ch);
		    }
		}
		
		else if (paf->type == gsn_aura_of_corruption) 
		    if (paf->bitvector == 0)
		    {
		        if (!IS_NPC(ch))
		        {
		            if ((IS_GOOD(ch) 
			      && ((ch->pcdata->karma + paf->modifier) <= PALEGOLDENAURA))
			    || ((IS_NEUTRAL(ch)
			      && ((ch->pcdata->karma + paf->modifier) < DARKREDAURA))))
			    {
			        send_to_char("Corruption slowly rots your soul from within.\n\r",ch);
                    modify_karma(ch, paf->modifier);
			    }
			    else if (IS_GOOD(ch)
			      && ((ch->pcdata->karma + paf->modifier) > PALEGOLDENAURA))
			    {
				    send_to_char("Corruption slowly rots your soul from within.\n\r",ch);
				    ch->pcdata->karma = PALEGOLDENAURA;
			    }
			    else if (IS_NEUTRAL(ch)
			      && ((ch->pcdata->karma + paf->modifier) >= DARKREDAURA))
			    {
			        send_to_char("Corruption slowly rots your soul from within.\n\r",ch);
				    ch->pcdata->karma = DARKREDAURA - 1;
			    }
		        }
		    }
		    else
		        send_to_char("You feel less prepared to project your sins onto others.\n\r",ch);

                else if (paf->type == gsn_coverofdarkness)
		{
		    if (paf->bitvector == AFF_SHROUD_OF_NYOGTHUA)
		    {
		        send_to_char("The shroud dissipates, exposing your presence.\n\r",ch);
// If modifier > 0 here, that means we're looking at the original caster's
// affect, so they need a recast delay.
			if (paf->modifier > 0)
			{
			    naf.where = TO_CHAR;
			    naf.duration = 24 - paf->modifier;
			    naf.type = gsn_coverofdarkness;
			    naf.modifier = 0;
			    naf.bitvector = 0;
			    naf.level = paf->level;
			    naf.location = 0;
			    affect_to_char(ch,&naf);
			}
		    }
		    else
		        send_to_char("You feel prepared to shroud yourself again.\n\r",ch);
		}
        else if (paf->type == gsn_alterself || paf->type == gsn_rearrange || paf->type == gsn_englamour)
		    act("Strips of color peel off $n, revealing $s natural form.", ch, NULL, NULL, TO_ROOM);

        else if (paf->type == gsn_chameleon)
		    act("$n shifts back into $s natural form.", ch, NULL, NULL, TO_ROOM);

		else if (paf->type == gsn_wrathofthevoid && paf->modifier == 1)
		    act("The green ichor finally dissolves.",ch, NULL, NULL, TO_CHAR);
		else if (paf->type == gsn_wrathofthevoid && paf->modifier == 2)
		    act("Your incessant hunger is at an end.",ch, NULL, NULL, TO_CHAR);

		else if (paf->type == gsn_earthbind)
		{
		    if (IS_NPC(ch) && IS_SET(race_table[ch->race].aff, AFF_FLYING))
    			SET_BIT(ch->affected_by, AFF_FLYING);
		}

		else if (paf->type == gsn_dominance)
		{
            // Lookup the char by the id
            CHAR_DATA * psion(get_char_by_id(paf->modifier));
            if (psion != NULL)
            {
                naf.where	  = TO_AFFECTS;
                naf.type	  = gsn_dominance;
                naf.level	  = paf->level;
                naf.point     = psion;
                naf.duration  = -1;
                naf.modifier  = 0;
                naf.location  = 0;
                naf.bitvector = AFF_CHARM;
                affect_to_char(ch, &naf);

                act("You break $n's spirit, completely dominating $s mind.", ch, NULL, psion, TO_VICT);
                act("$N breaks your spirit, completely dominating your mind.", ch, NULL, psion, TO_CHAR);
                if (ch->master)
                    stop_follower(ch);
                
                add_follower(ch, psion);
                ch->leader = psion;
                if (ch->fighting == psion) stop_fighting(ch);
                if (psion->fighting == ch) stop_fighting(psion);
            }
   		} 

		else if (paf->type == gsn_enslave && IS_NPC(ch))
		{
		    if (IS_AFFECTED(ch, AFF_CHARM))
		    {
			if (ch->master)
			{
			    act("You feel your control over $N fade away.", ch->master, NULL, ch, TO_CHAR);
			    act("$n roars in fury and attacks you!", ch, NULL, ch->master, TO_VICT);
			    act("$n roars in fury and attacks $N!", ch, NULL, ch->master, TO_ROOM);
			    multi_hit(ch, ch->master, TYPE_UNDEFINED);
			}
			stop_follower(ch);
		    }
		    else
		    {
			if (saves_spell(paf->level, NULL, ch, DAM_CHARM))
			{
			    for (vch = char_list; vch; vch = vch_next)
			    {
				vch_next = vch->next;

				if (vch->id == paf->modifier)
				{
			            act("$n roars in fury and attacks you!", ch, NULL, vch, TO_VICT);
				    act("$n roars in fury and attacks $N!", ch, NULL, vch, TO_ROOM);
				    REMOVE_BIT(vch->affected_by, AFF_CHARM);
				    multi_hit(ch, vch, TYPE_UNDEFINED);
				    break;
				}
			    }
			}
			else
			{
			    for (vch = char_list; vch; vch = vch->next)
				if (vch->id == paf->modifier)
				{
			    	    naf.where	 = TO_AFFECTS;
			    	    naf.type	 = gsn_enslave;
			    	    naf.level	 = paf->level;
			    	    naf.duration  = number_fuzzy(paf->level/2);
			    	    naf.location  = APPLY_NONE;
			    	    naf.modifier  = paf->modifier;
			    	    naf.bitvector = AFF_CHARM;
			    	    affect_to_char(ch, &naf);

				    if (ch->master)
					stop_follower(ch);
				    add_follower(ch, vch);
				    ch->leader = vch;

				    act("$n breaks $N's will, enslaving it to $s whim.", vch, NULL, ch, TO_ROOM);
				    act("You break $N's will, enslaving $M to your whim.", vch, NULL, ch, TO_CHAR);
				    break;
				}
			}
		    }
		}

                else if (paf->type == gsn_morph)
                {
                    free_string( ch->long_descr );
                    ch->long_descr = str_dup("");
                }
		else if (paf->type == gsn_beastform)
		{
		    if (paf->bitvector == AFF_BEASTFORM)
		    {
			AFFECT_DATA af;
			af.valid = TRUE;
			af.point = NULL;
			af.type = gsn_beastform;
			af.where = TO_AFFECTS;
			af.bitvector = 0;
			af.duration = 32;
			af.modifier = 0-ch->level/5;
			af.location = APPLY_DAMROLL;
			affect_to_char(ch,&af);
			send_to_char("Your body returns to normal as the beast departs.\n\r",ch);
			act("$n's body returns to normal.",ch,NULL,NULL,TO_ROOM);
		    }
		    else if (paf->modifier < 0)
			send_to_char("You feel ready to assume the aspect of the beast once more.\n\r",ch);
		}

                else if (paf->type == gsn_demonpos) 
                {
                    if (paf->bitvector == AFF_DEMONPOS)
                    {
                   
                        AFFECT_DATA af; 
af.valid = TRUE;
af.point = NULL;
     	            
	                send_to_char("You collapse as the demon's essence roars out of your body!\n\r",ch);
                        act("$n collapses to $s knees, clutching at $s head as a dark mist roils out of $s mouth!", ch, NULL, NULL, TO_ROOM);
// brazen: added this HP back in now that max hp adjustments also affect
// current hp.
                        ch->hit = ch->hit / 2 + 100 + ch->level;
                        ch->mana /= 2;
                        switch_position(ch, POS_RESTING); 
                 
                        af.where     = TO_AFFECTS;
                        af.type      = gsn_demonpos;
                        af.level     = ch->level;
                        af.duration  = 15;
                        af.location  = APPLY_HIT;
                        af.modifier  = -100 - ch->level ;
                        af.bitvector = 0;
                        affect_to_char(ch, &af);
                    }  

                    else if (paf->modifier == 0) 
                    {
                        send_to_char("You are ready to withstand the rigors of demonic possession again.\n\r",ch);
                    }
                }      

		else if (paf->type == gsn_toggleoffaff && !IS_NPC(ch) && paf->modifier < 65535)
		    BIT_CLEAR(ch->pcdata->bitptr, paf->modifier);
		 
		else if (paf->type == gsn_toggleonaff && !IS_NPC(ch) && paf->modifier < 65535)
		    BIT_SET(ch->pcdata->bitptr, paf->modifier);
		
		else if (paf->type == gsn_crystalizemagic)
	        {
		    if (paf->modifier == 0)
			send_to_char("The magic surrounding you fades.\n\r", ch);
		    else
			send_to_char("You feel ready to crystalize magic again.\n\r", ch);
		}

		else if (paf->type == gsn_diamondskin)
		{
		    if (paf->bitvector == IMM_WEAPON)
			send_to_char("Your flesh returns to its normal state.\n\r", ch);
		    else
			send_to_char("You feel ready to diamond skin again.\n\r", ch);
		}

		else if (paf->type == gsn_borrow_knowledge)
		{
		    send_to_char("You feel ready to mindtap again.\n\r", ch);
		}

		else if ((paf->type == gsn_naturegate) && (ch->in_room->vnum >= MIN_VNUM_NATUREGATE) && (ch->in_room->vnum <= MAX_VNUM_NATUREGATE))
		{
		    act("The naturegate senses you do not belong, and casts you out!", ch, NULL, NULL, TO_CHAR);
		    act("$n is hurled from the naturegate!", ch, NULL, NULL, TO_ROOM);
		    char_from_room(ch);
		    char_to_room(ch, get_room_index(paf->modifier));
		    if (IS_AWAKE(ch))
			do_look(ch, "auto");
		    act("$n stumbles out from the foliage!", ch, NULL, NULL, TO_ROOM);
		}

		else if (paf->type == gsn_voidwalk)
		{
		    if (paf->bitvector == AFF_VOIDWALK)
		    {
			act("You shift back into the material world.", ch, NULL, NULL, TO_CHAR);
       	 		act("$n shifts $s essence back into the material world.", ch, NULL, NULL, TO_ROOM);
       	 		ch->hit = UMAX(1, ch->hit / 4);

			naf.where     = TO_PAFFECTS;
			naf.type      = gsn_voidwalk;
			naf.level     = ch->level;
			naf.duration  = ch->level / 4;
			naf.location  = APPLY_HIT;
			naf.modifier  = ch->level * -1;
			naf.bitvector = 0;
			affect_to_char(ch, &naf);

			WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
			send_to_char("You pause for a moment, feeling drained from the experience.\n\r", ch);
		    }
		    else
			send_to_char("You feel ready to shift into the void again.\n\r", ch);
		}

		else if (paf->type == gsn_burnout)
		{
		    if (paf->bitvector == AFF_BURNOUT)
		    {
    			send_to_char("Your regenerative powers subside, leaving your body tired and sore.\n\r", ch);
	    		naf.where	= TO_AFFECTS;
		    	naf.type	= gsn_burnout;
			    naf.level	= paf->level;
    			naf.duration    = (paf->level * 2) / 15;
	    		naf.modifier    = 0;
		    	naf.location    = 0;
			    naf.bitvector   = 0;
    			affect_to_char(ch, &naf);
		    }
		    else
	    		send_to_char("You feel the lingering effects of your burnout subside.\n\r", ch);
		}

        else if (paf->type == gsn_focusmind)
        {
            if (paf->modifier > 0)
            {
                send_to_char("A dull pounding fills your head as you grow weary of focusing your thoughts.\n", ch);

                AFFECT_DATA faf(*paf);
                faf.modifier = -50;
                faf.duration = 8;
                affect_to_char(ch, &faf);
            }
            else
                send_to_char("Your thoughts clear once more as the dull pounding recedes.\n", ch);
        }

        else if (paf->type == gsn_calluponwind)
        {
            if (paf->location == APPLY_HIDE) 
                send_to_char("You feel able to call upon any wind once more.\n", ch);
            else
            {
                send_to_char("The winds stop responding to your call.\n", ch);
                AFFECT_DATA caf(*paf);
                caf.location = APPLY_HIDE;
                caf.duration = 20;
                affect_to_char(ch, &caf);
            }
        }

        else if (paf->type == gsn_breezestep)
        {
            if (paf->modifier == 1)
            {
                send_to_char("Your motions slow as you stop flitting on the breeze.\n", ch);
                AFFECT_DATA baf(*paf);
                baf.duration = 18 - UMAX(0, (get_skill(ch, gsn_breezestep) - 70) / 5);
                baf.modifier = 0;
                affect_to_char(ch, &baf);
            }
            else
                send_to_char("You feel the power of the breezes return to you.\n", ch);
        }

        // Check for a pyrokinetic mirror fading away
        else if (IS_NPC(ch) && paf->type == gsn_pyrokineticmirror && paf->modifier == 1)
        {
            if (ch->in_room != NULL)
                act("$n flickers briefly, then fades away into nothingness!", ch, NULL, NULL, TO_ROOM);

            extract_char(ch, TRUE);
            break;
        }

        else if (paf->type == gsn_bilocation)
        {
            if (IS_NPC(ch))
            {
                // Find the real body
                CHAR_DATA * body(find_bilocated_body(ch, paf));
                if (body != NULL)
                {
                    // Relocate the consciousness, if necessary
                    if (ch->desc != NULL)
                    {
                        // Inhabiting the illusory body; punt him back to the real one
                        relocate_consciousness(ch, body);
                        send_to_char("Your illusory form unravels, and with a dizzying rush your consciousness tumbles back into your real body.\n", body);
                        WAIT_STATE(body, UMAX(body->wait, 2 * PULSE_VIOLENCE));
                    }
                }

                if (ch->in_room)
                {
                    act("$n shimmers, then unravels in streams of light! In moments $e is completely gone.", ch, NULL, NULL, TO_ROOM);

                    // Make sure to get all equipment from the body first and drop it on the ground
                    OBJ_DATA * obj_next;
                    for (OBJ_DATA * obj(ch->carrying); obj != NULL; obj = obj_next)
                    {
                        obj_next = obj->next_content;
                        obj_from_char(obj);
                        obj_to_room(obj, ch->in_room);
                        act("$p falls to the floor.", ch, obj, NULL, TO_ROOM);
                    }
                }

                extract_char(ch, true);
                break;
            }

            // Expired from the PC
            send_to_char("You feel ready to once again split your consciousness.\n", ch);
        }

        else if (paf->type == gsn_clayshield)
        {
            if (paf->modifier == 0) send_to_char("The clay shield about you crumbles away.\n", ch);
            else send_to_char("You feel ready to sustain another clay shield about yourself.\n", ch);
        }

        else if (paf->type == gsn_wrathofanakarta)
        {
            if (paf->modifier == 0)
            {
                send_to_char("The unholy hatred that had filled you slowly ebbs away, leaving you disoriented.\n", ch);
                AFFECT_DATA waf = {0};
                waf.where    = TO_AFFECTS;
                waf.type     = gsn_wrathofanakarta;
                waf.duration = 10;
                waf.location = APPLY_MANA;
                waf.modifier = -100;
                waf.level    = paf->level;
                affect_to_char(ch, &waf);
            }
            else
                send_to_char("Your disorientation finally passes, and you feel once again ready to channel An'Akarta's wrath.\n", ch);
        }

        else if (paf->type == gsn_conflagration)
        {
            if (paf->modifier == 0)
            {
                send_to_char("The aura of heat abandons you, dispersing into the space around you.\n", ch);

                AFFECT_DATA waf = {0};
                waf.where    = TO_AFFECTS;
                waf.type     = paf->type;
                waf.duration = 60;
                waf.modifier = 1;
                waf.level    = paf->level;
                affect_to_char(ch, &waf);
            }
            else
                send_to_char("You feel ready to call forth another conflagration.\n", ch);
        }

        else if (paf->type == gsn_steam)
        {
            if (paf->modifier == 0)
            {
                send_to_char("The steady stream of steam abates.\n", ch);
                paf->duration = 10;
                paf->modifier = 1;
                continue;
            }
            send_to_char("You feel ready to call forth more steam.\n", ch);
        }

        else if (paf->type == gsn_borrowluck)
        {
            if (paf->modifier > 0)
            {
                send_to_char("A sinking feeling fills you as your luck changes.\n", ch);
                paf->duration = 30 - UMAX(0, (get_skill(ch, gsn_borrowluck) - 70) / 3);
                paf->modifier = -paf->modifier;
                continue;
            }

            send_to_char("The fates level out again as your ill fortune departs.\n", ch);
        }

        else if (paf->type == gsn_beckonwindfall)
        {
            if (paf->location == APPLY_NONE && paf->modifier == ch->id) 
                send_to_char("You feel ready to seek out another windfall.\n", ch);
            else 
                send_to_char("The effects of the windfall fade.\n", ch);
        }

        else if (paf->type == gsn_sonicboom)
        {
            if (paf->modifier < 0) send_to_char("The thunders return to you, leaving you ready to evoke another sonic boom.\n", ch);
            else send_to_char("You are no longer startled by the sonic boom.\n", ch);
        }

        else if (paf->type == gsn_clingingfog)
        {
            if (paf->bitvector == 0) send_to_char("You feel ready to summon another clinging fog.\n", ch);
            else send_to_char("The sparkling fog stuck to you finally evaporates.\n", ch);
        }

        else if (paf->type == gsn_wakenedstone)
        {
            if (Drakes::CheckMature(*ch, *paf))
            {
                affect_remove(ch, paf);
                continue;
            }
        }

        else if (paf->type == gsn_flood)
        {
            if (paf->modifier == 0) send_to_char("You feel ready to summon another flood.\n", ch);
            else send_to_char("The weariness from your fight through the flood dissipates.\n", ch);
        }

        else if (paf->type == gsn_solaceoftheseas)
        {
            if (paf->modifier == 0)
            {
                if (paf->bitvector == 0) send_to_char("You feel once again prepared to call upon the solace of the seas.\n", ch);
                else send_to_char("Your pulse beats normally once more as the last vestiges of the sea's serenity abandon you.\n", ch);
            }
            else
                send_to_char("You feel the solace of the seas depart from you, leaving your pulse slowed.\n", ch);
        }

        else if (paf->type == gsn_healerstouch)
        {
            std::ostringstream mess;
            mess << "Your immunity to " << skill_table[paf->modifier].name << " fades away.\n";
            send_to_char(mess.str().c_str(), ch);
        }

        else if (paf->type == gsn_scorchedearth)
        {
            if (paf->bitvector == AFF_BLEEDING) send_to_char("Your bleeding wounds close up.\n", ch);
            else if (paf->bitvector == AFF_DEAFEN) send_to_char("You can hear again.\n", ch);
            else send_to_char("You feel ready to once again scorch the earth.\n", ch);
        }

        else if (paf->type == gsn_shakestride)
        {
            if (paf->modifier == 0)
            {
                send_to_char("The power of the earth drains from your feet.\n", ch);
                int skill(get_skill(ch, gsn_shakestride));
                paf->modifier = 1;
                paf->duration = 30 - (UMAX(0, skill - 70) / 3);
                continue;
            }

            send_to_char("You feel ready to shake the earth with your steps once more.\n", ch);
        }

        else if (paf->type == gsn_constructwaterwheel)
        {
            if (paf->location == APPLY_MANA) send_to_char("The power taken from the charged crystal fades within you.\n", ch);
            else send_to_char("You feel prepared to construct another waterwheel.\n", ch);
        }

        else if (paf->type == gsn_aspectoftheinferno)
        {
            if (paf->modifier == 0)
            {
                act("A massive rush of heat dissipates from you as you cease taking on the aspect of the Inferno.", ch, NULL, NULL, TO_CHAR);
                act("A rush of heat dissipates suddenly from $n, warming the area.", ch, NULL, NULL, TO_ROOM);
                send_to_char("You feel drained from the experience.\n", ch);
                ch->mana /= 8;
                ch->move = UMIN(number_percent() / 8, ch->move);

                AFFECT_DATA waf = {0};
                waf.where    = TO_AFFECTS;
                waf.type     = paf->type;
                waf.duration = 16 - UMAX(0, (get_skill(ch, gsn_aspectoftheinferno) - 80) / 5);
                waf.modifier = 1;
                waf.level    = paf->level;
                affect_to_char(ch, &waf);
            }
            else
                send_to_char("You feel once again prepared to assume the aspect of the Inferno.\n", ch);
        }

        else if (paf->type == gsn_wreathoffear)
        {
            if (paf->location == APPLY_CHR) 
                send_to_char("The aura of trepidation surrounding you slowly dissipates.\n", ch);
            else
                send_to_char("You are once again prepared to wreath yourself in fear.\n", ch);  
        }

        else if (paf->type == gsn_bierofunmaking)
        {
            if (paf->location == APPLY_NONE)
                send_to_char("You are ready to prepare another bier of unmaking.\n", ch);
            else if (paf->location == APPLY_HIT)
                send_to_char("The unholy power granted by the bier of unmaking dissipates.\n", ch);
        }

        else if (paf->type == gsn_baneblade)
        {
            if (paf->location == APPLY_NONE)
                send_to_char("The strength of earth and void return to you, leaving you ready to empower another baneblade.\n", ch);
            else
                send_to_char("You feel less vulnerable as the effects of the baneblade recede.\n", ch);
        }

        else if (paf->type == gsn_devouressence)
        {
            if (paf->modifier < 0) send_to_char("You feel restored as your soul finishes healing.\n", ch);
            else send_to_char("You burn through the last of the devoured essence, the power dissipating.\n", ch);
        }

        else if (paf->type == gsn_harvestofsouls)
        {
            if (paf->location == APPLY_NONE)
                send_to_char("Your unworldly hunger sated, you cease drawing in spirits.\n", ch);
            else if (paf->location == APPLY_HIT)
                send_to_char("The power of the harvested souls bleeds away, leaving you feeling slightly empty.\n", ch);
        }

        else if (paf->type == gsn_webofoame)
        {
            if (paf->location == APPLY_NONE)
                send_to_char("You feel ready to spin another web of oame.\n", ch);
            else
            {
                send_to_char("The webbing dries out and breaks apart, releasing you.\n", ch);
                free_string(ch->pose);
            }
        }

        else if (paf->type == gsn_grimseep)
        {
            if (paf->location == APPLY_NONE) 
                send_to_char("You feel prepared to summon more grimseep.\n", ch);
            else if (paf->location == APPLY_HIT) 
                send_to_char("You feel less hardy as you sweat out the last of the dark silt.\n", ch);
            else if (paf->location == APPLY_RESIST_WEAPON) 
                send_to_char("The blackened grime coating you flakes away, no longer protecting you from harm.\n", ch);
        }

        else if (paf->type == gsn_nightstalk)
        {
            if (paf->modifier == 0)
                send_to_char("You are ready to stalk the shadows once more.\n", ch);
            else
                send_to_char("You cease stalking the shadows.\n", ch);
        }

        else if (paf->type == gsn_seedofmadness)
        {
            if (paf->location == APPLY_NONE)
                send_to_char("With a force of will, you banish the seed of madness from your mind!\n", ch);
            else
                send_to_char("Your thoughts seem a bit clearer as some of the madness dissipates.\n", ch);
        }

        else if (paf->type == gsn_flare)
        {
            if (paf->location == APPLY_HIT) send_to_char("You feel your inner fire fade.\n", ch);
            else send_to_char("You feel ready to take in another flare.\n", ch);
        }

        else if (paf->type == gsn_invocationofconveru)
        {
            if (paf->bitvector == 0) send_to_char("You feel ready to again channel the spirit of Converu.\n", ch);
            else send_to_char("The warrior's will departs, and your movements slow to normal.\n", ch);
        }

        else if (paf->type == gsn_quintessencerush)
        {
            if (paf->modifier < 0) send_to_char("You feel once again prepare to infuse your spirit with raw quintessence.\n", ch);
            else send_to_char("You burn away the last of the quintessence enfolding your spirit, and feel its power leave you.\n", ch);
        }

        else if (paf->type == gsn_fugue)
        {
            if (paf->location == APPLY_NONE)
                send_to_char("You feel prepared to once again enter a fugue state.\n", ch);
            else
            {
                send_to_char("The lines of the world grow sharper as you slowly emerge from your fugue state.\n", ch);
                AFFECT_DATA faf = {0};
                faf.where    = TO_AFFECTS;
                faf.type     = gsn_fugue;
                faf.level    = paf->level;
                faf.location = APPLY_NONE;
                faf.duration = 18 - UMAX(0, (get_skill(ch, gsn_fugue) - 70) / 5);
                affect_remove(ch, paf);
                affect_to_char(ch, &faf);
                continue;
            }
        }

        else if (paf->type == gsn_darkchillburst)
        {
            switch (paf->location)
            {
                case APPLY_NONE: send_to_char("You feel ready to call upon the chill of night once more.\n", ch); break;
                case APPLY_STR: send_to_char("As the night's chill leaves you, you feel your strength return.\n", ch); break;
                case APPLY_DEX: send_to_char("Your limbs begin moving fluidly again as the cold of nighttime departs.\n", ch); break;
                default: bug("Unhandled location echo for darkchill burst", 0); break;
            }
        }

        else if (paf->type == gsn_arcticchill)
        {
            if (paf->modifier == 0)
            {
                send_to_char("The chill of the arctic departs from you.\n", ch);
                paf->duration = 10 - UMAX(0, (get_skill(ch, gsn_arcticchill) - 70) / 6);
                paf->modifier = 1;
                continue;
            }
            send_to_char("You feel once again prepared to invoke the arctic's chill.\n", ch);
        }

        else if (paf->type == gsn_oceanswell && IS_NPC(ch) && ch->pIndexData->vnum == MOB_VNUM_WATER_ELEMENTAL)
        {
            act("The power holding $n together dissipates, and $e seems to pause briefly before suddenly flowing down into a shapeless puddle of water!", ch, NULL, NULL, TO_ALL);
            extract_char(ch, true);
            break;
        }

        else if (paf->type == gsn_illusion)
        {
            if (IS_NPC(ch) && ch->pIndexData->vnum == MOB_VNUM_WEAVEILLUSION)
            {
                act("$n suddenly comes apart, unravelling in strips of light and color!", ch, NULL, NULL, TO_ROOM);
                extract_char(ch, true);
                break;
            }

            send_to_char("You feel once again prepared to bind an illusion together.\n", ch);
        }

        else if (paf->type == gsn_mistsofarcing)
        {
            if (paf->location == APPLY_RESIST_LIGHTNING) send_to_char("You wipe off the last of the sparkling mists coating you.\n", ch);
            else send_to_char("You feel ready to call forth more conductive mists.\n", ch);
        }

        else if (paf->type == gsn_bloodpyre)
        {
            if (paf->point != NULL)
            {
                OBJ_DATA * pyre(static_cast<PyreInfo*>(paf->point)->pyre());
                if (verify_obj_world(pyre))
                {
                    if (pyre->in_room != NULL && pyre->in_room->people != NULL)
                    {
                        act("$p slowly burns out, leaving nothing behind, not even ash.", pyre->in_room->people, pyre, NULL, TO_CHAR);
                        act("$p slowly burns out, leaving nothing behind, not even ash.", pyre->in_room->people, pyre, NULL, TO_ROOM);
                    }
                    extract_obj(pyre);
                }
            }
        }

        else if (paf->type == gsn_aetherealcommunion)
        {
            if (paf->modifier >= 0)
            {
                // Get the members before and after changing the effect from active to cooldown
                unsigned int dummy;
                std::vector<CHAR_DATA*> membersBefore(MembersOfAetherealCommunion(ch, dummy));
                paf->modifier = -1;
                paf->duration = 10;
                std::vector<CHAR_DATA*> membersAfter(MembersOfAetherealCommunion(ch, dummy));

                // Inform the members of the lost link
                bool casterLost(false);
                for (unsigned int i(0); i < membersBefore.size(); ++i)
                {
                    // Check the list of after members to see whether this member lost link
                    bool found(false);
                    for (unsigned int j(0); j < membersAfter.size(); ++j)
                    {
                        if (membersBefore[i] == membersAfter[j])
                        {
                            found = true;
                            break;
                        }
                    }

                    // Echo if link lost
                    if (!found)
                    {
                        if (membersBefore[i] == ch) casterLost = true;
                        else send_to_char("You feel a brief sense of loss as your spirit unlinks from the aethereal communion.\n", membersBefore[i]);
                    }
                }

                // Now inform the caster
                if (membersBefore.size() <= 1) send_to_char("You are no longer prepared to host an aethereal communion.\n", ch);
                else if (casterLost) send_to_char("The aethereal communion fades, and you feel your spirit unlink from the greater Whole.\n", ch);
                else send_to_char("You cease sharing in the hosting of the aethereal communion.\n", ch);
                continue;
            }

            send_to_char("You feel once again ready to host an aethereal communion.\n", ch);
        }

        else if (paf->type == gsn_crystallizeaether)
        {
            if (paf->modifier > 0) send_to_char("The lingering effects of the living mists fade, leaving you once more ready to embrace the full potency of Quintessence.\n", ch);
            else send_to_char("You are once again prepared to draw Quintessence from the Weave.\n", ch);
        }

		else if ((paf->type == gsn_demonsummon) && (paf->modifier == 1))
		{
		    int num_casters = 0;
		    int high_level = 0;
		    CHAR_DATA *demon, *controller = ch;

		    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
			if (is_affected(vch, gsn_demonsummon)
			 && (get_modifier(vch->affected, gsn_demonsummon) > 0))
			{
			    if (vch->level > high_level)
			    {
				controller = vch;
				high_level = vch->level;
			    }
			    num_casters++;
			}

		    if (ch->fighting)
		    {
			send_to_char("You cannot concentrate enough to complete the tide of Bahhaoth.\n\r", ch);
			act("$n loses $s concentration, and the tide of Bahhaoth fails.", ch, NULL, NULL, TO_ROOM);
			for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
			    if ((vch != ch)
			     && is_affected(vch, gsn_demonsummon)
			     && (get_modifier(vch->affected, gsn_demonsummon) > 0))
				affect_strip(vch, gsn_demonsummon);
		    }
		    else if (num_casters < 3)
		    {
			for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
			    if (is_affected(vch, gsn_demonsummon)
			     && (get_modifier(vch->affected, gsn_demonsummon) > 0))
			    {
				send_to_char("Lacking enough power to complete the ritual, the tide of Bahhaoth ends.\n\r", vch);
				if (vch != ch)
				    affect_strip(vch, gsn_demonsummon);
			    }
		    }
		    else
		    {
			demon = create_mobile(get_mob_index(MOB_VNUM_SHUNNED_DEMON));
			demon->max_hit 	= num_casters * 1000;
			demon->hit 	= num_casters * 1000;
			demon->max_mana = num_casters * 500;
			demon->mana 	= num_casters * 500;
			demon->max_move = 400;
			demon->move	= 400;
			demon->level	= high_level + (4 * (num_casters - 1));
			demon->hitroll  = demon->level * 3 / 4;
			demon->damroll  = demon->level / 2;
			demon->damage[0] = demon->level / 5;
			demon->damage[1] = demon->level / 4;
			demon->armor[0] = 0 - ((demon->level * 15) / 10);
			demon->armor[1] = 0 - ((demon->level * 15) / 10);
			demon->armor[2] = 0 - ((demon->level * 15) / 10);
			demon->armor[3] = 0 - ((demon->level * 15) / 10);
			char_to_room(demon, ch->in_room);

			send_to_char("Your mouth jerks open with a agonizing crack!  A nausea wells up inside you, and you begin to heave and retch as grey, hazy tendrils erupt from between your teeth.\n\r", ch);
			send_to_char("The grey tendrils flail outward from your mouth, lightly touching all those in the coven, leeching their colour slowly away.\n\r", ch);
			send_to_char("Your corporal body suddenly dissipates into shadow, and you sense your very mind flow outward to swell the malevolent sentience which grows about you.\n\r", ch);

			act("$n's mouth jerks open with a sickening crack, and $e begins to heave and retch as grey, hazy tendrils erupt from between $s teeth.", ch, NULL, NULL, TO_ROOM); 
			act("The grey tendrils stretch forward, lightly touching each of the coven, leeching their colour slowly away.", ch, NULL, NULL, TO_ROOM);  

			for (vch = ch->in_room->people; vch; vch = vch_next)
			{
			    vch_next = vch->next_in_room;
			    if (is_affected(vch, gsn_demonsummon) && (get_modifier(vch->affected, gsn_demonsummon) > 0))
			    {
				send_to_char("As the tendrils surround you, you feel your corporal essence rapidly slipping away.\n\r", vch);
				send_to_char("Your corporal body suddenly dissipates into shadow, and you sense your very mind flow outward to swell the malevolent sentience which grows about you.\n\r", vch);  

				vch->was_in_room = vch->in_room;
				char_from_room(vch);

				if (vch != controller)
				    vch->shunned_demon = demon;
				if (vch != ch)
				    affect_strip(vch, gsn_demonsummon);
			    }
			    else
				send_to_char("The coven's forms blur, and collapse into a seething, grasping flood of shadows!\n\r", vch);
			}

			controller->desc->character 	= demon;
			controller->desc->original  	= controller;
			demon->desc			= controller->desc;
			demon->exp			= controller->exp;
			controller->desc		= NULL;
			demon->comm			= controller->comm;
			demon->lines			= controller->lines;
			
			affect_remove(ch, paf);
			continue;
		    }
		}
		
		else if (paf->type == gsn_cateyes)
		{
			if (is_affected(ch, gsn_detecthidden) || is_affected(ch, gsn_perception))
				SET_BIT(ch->affected_by, AFF_DETECT_HIDDEN);
			else REMOVE_BIT(ch->affected_by, AFF_DETECT_HIDDEN);
		}
        else if (paf->type == gsn_reveal && paf->modifier == 1)
        {
            send_to_char("The fire imps finally grow bored and flitter away, gibbering madly all the while.\n", ch);
        }

		else if (paf->type == gsn_delayreaction)
		{
			send_to_char("You finish delaying your reaction, and move to complete it.\n\r", ch);
			do_alchemy(ch, (char *)paf->point, paf->modifier, 0, (double)paf->level);
		}

		else if (paf->type == gsn_gills)
		{
			affect_strip(ch, gsn_drowning);
		}
		else if (paf->type == gsn_forcedmarch)
		{
		    if (paf->bitvector == AFF_FORCEDMARCH)
		    	send_to_char("You begin recovering from your grueling march.\n\r",ch);
		    else
			send_to_char("You feel ready to march again.\n\r",ch);
		}
		else if (paf->type == gsn_dash)
		{
		    if (paf->bitvector == AFF_DASH)
			send_to_char("Your swift getaway comes to an end.\n\r",ch);
		    else
			send_to_char("You feel rested enough for another quick getaway.\n\r",ch);
		}
		else if (paf->type == gsn_nova)
		{
		    if (paf->bitvector == AFF_NOVA_CHARGE)
		    {
			send_to_char("You finish charging your nova, prepared to unleash its power.\n\r", ch);
			naf.where	= TO_AFFECTS;
			naf.type	= paf->type;
			naf.level	= paf->level;
			naf.duration    = paf->modifier * 3;
			naf.modifier    = paf->modifier;
			naf.location    = 0;
			naf.bitvector   = 0;
			affect_to_char(ch, &naf);
		    }
		    else
			if (number_percent() < 51)
			    send_to_char("The power of your nova dissipates.\n\r", ch);
			else
			{
			    if (IS_SET(ch->in_room->room_flags,ROOM_NOMAGIC))
			    {
				send_to_char("Your spell fizzles out and fails.\n\r",ch);
				act("$n's spell fizzles out and fails.",ch,NULL,NULL,TO_ROOM);
			    }
			    else
			    {
				act("You lose control of your nova, engulfing the room in flames!",ch,NULL,NULL,TO_CHAR);
			        act("$n loses control of $s nova, engulfing the room in flames!",ch,NULL,NULL,TO_ROOM);
			        int dam = dice(ch->level,5) + ch->level/3;
			        for(vch=ch->in_room->people;vch!=NULL;vch=vch->next_in_room)
				    if (is_safe_spell(ch,vch,FALSE))
				        continue;
				    else
				        damage_old(ch,vch,dam,gsn_nova,DAM_FIRE,TRUE);
			    }
			}
		}

                else if (paf->type == gsn_setsnare)
                {
		    if (ch->class_num == global_int_class_ranger) 
			send_to_char("You feel ready to set another snare.\n\r", ch);
		    else if (paf->location == APPLY_DEX)
			send_to_char("You manage to work your way out of the snare.\n\r", ch);
		}


		else if (paf->bitvector == AFF_SLEEP && IS_NPC(ch))
		{
		    affect_remove(ch, paf);
		    do_wake(ch, "");
		    continue;
		}

	        else if (paf->type == gsn_possession && IS_NPC(ch))
		{

		    if (ch->prompt != NULL)
    		    {
        		free_string(ch->prompt);
        		ch->prompt = NULL;
    		    }

		    if (ch && ch->desc && ch->desc->original && ch->desc->original->in_room)
    		        act("$n shakes $s head clear as $s spirit returns.", ch->desc->original, NULL, ch, TO_ROOM);

		    if (ch->valid)
		        send_to_char("You feel your hold of your temporary shell slip, and you are forced back into your own body.\n\r", ch);

	            if (ch && ch->desc && ch->desc->original && ch->desc->original->in_room)
	            {
			WAIT_STATE(ch->desc->original, UMAX(ch->desc->original->wait, 5*PULSE_VIOLENCE));
			ch->desc->original->hit /= 2;

			naf.where	= TO_AFFECTS;
	        	naf.type	= gsn_possession;
			naf.level	= ch->desc->original->level;
			naf.duration	= 8;
			naf.location	= APPLY_NONE;
			naf.modifier	= 0;
			naf.bitvector	= 0;
			affect_to_char(ch->desc->original, &naf);

    			if (ch->desc != NULL)
    		   	{
    			    ch->desc->character       = ch->desc->original;
    			    ch->desc->original        = NULL;
    			    ch->desc->character->desc = ch->desc;
    			    ch->desc                  = NULL;
    		        }
		    } /* if ch && ch->desc */
		}
	
		else if (paf->type == gsn_greaterdemonsummon)
		    mprog_demon_trigger(ch);
		else if (paf->type == gsn_lesserdemonsummon)
		    mprog_demon_trigger(ch);

		else if (paf->type == gsn_warpath)
		    ch->hunting = NULL;

		else if (paf->type == gsn_fleshtostone || paf->type == gsn_bindingtentacle)
		    REMOVE_BIT(ch->act, PLR_FREEZE);

		else if (paf->type == gsn_encase && paf->where != TO_PAFFECTS)
        {
            send_to_char("The ice encasing you melts away.\n", ch);
            act("The ice encasing $n melts away.", ch, NULL, NULL, TO_ROOM);
            REMOVE_BIT(ch->act, PLR_FREEZE);
        }

		else if (paf->type == gsn_suggestion)
		{
		    DESCRIPTOR_DATA *d;
		    CHAR_DATA *fch;
		    char buf[MAX_STRING_LENGTH];
		    int x;

		    sprintf(buf, "You feel yourself compelled to '%s'\n\r",
			(char *) paf->point);
		    send_to_char(buf, ch);
		    interpret(ch, (char *) paf->point);

		    for (d = descriptor_list; d; d = d->next)
		    {
			if (d->connected == CON_PLAYING)
			{
			    fch = (d->original ? d->original : d->character);
			    for (x = 0; x < MAX_FOCUS; x++)
				if (fch->pcdata->focus_on[x]
				 && (fch->pcdata->focus_sn[x] == gsn_suggestion)
				 && (fch->pcdata->focus_ch[x] == ch))
				{
				    unfocus(fch, x, FALSE);
				    send_to_char("You sense your suggestion take effect.\n\r", fch);
				    break;
				}
			}
		    }
		}

		else if (paf->type == gsn_garrote && paf->location == APPLY_MANA)
		    send_to_char("Your neck feels better.\n\r", ch);
		else if (paf->type == gsn_garrote && paf->location == APPLY_NONE)
		    send_to_char("You feel ready to garrote someone again.\n\r", ch);

		else if (paf->type == gsn_smoke)
		{
		    if (paf->bitvector == AFF_BLIND)
			send_to_char("You can see again.\n\r", ch);
		    else
			send_to_char("You feel ready to summon another cloud of smoke.\n\r", ch);
		}

                else if (paf->type == gsn_vortex)
                {
                    if ( ch->fighting )
			stop_fighting_all(ch);

		    global_linked_move = TRUE;
                    char_from_room( ch );
                    char_to_room( ch, get_room_index( paf->modifier ));
                    act("$n steps into the room from a vortex of power.", ch,NULL,NULL,TO_ROOM);
                    do_look(ch, "auto");
                }

	        else if (paf->type == gsn_flashpowder)
		{
		    if (paf->bitvector & AFF_FLASHPOWDER)
			send_to_char("You feel able to mix flash powder again.\n\r", ch);
		    else
			send_to_char("The glaring aftereffects of the flash powder subside.\n\r", ch);
		}

		else if (paf->type == gsn_bolo)
		{
		    if (paf->bitvector & AFF_BOLO)
			send_to_char("You manage to untangle yourself from the bolo.\n\r", ch);
		    else
			send_to_char("You feel ready to throw a bolo again.\n\r", ch);
		}

		else if ((paf->type == gsn_stickstosnakes) && IS_NPC(ch) && (ch->pIndexData->vnum == MOB_VNUM_SNAKE))
		{
		    act("$n petrifies into wood, and sinks into the ground.", ch, NULL, NULL, TO_ROOM);
		    extract_char(ch, TRUE);
		    continue;
		}
	
		else if ((paf->type == gsn_consumption) && (paf->modifier == 15))
		{
		    do_echo(ch, "You feel the sense of overwhelming darkness lift.");
		    silver_state = 0;
		}

		else if ((paf->type == gsn_consumption) && (paf->modifier == 0))
		{
		    int casters = 0;
		    CHAR_DATA *nch;

		    for (nch = ch->in_room->people; nch; nch = nch->next_in_room)
			if (IS_OAFFECTED(nch, AFF_CONSUMPTION))
			    casters++;

		    if (casters < 3)
		    {
			send_to_char("Lacking the numbers to complete the ritual, you abandon its casting.\n\r", ch);
			for (nch = char_list; nch; nch = nch->next)
			    if (ch != nch)
			        affect_strip(nch, gsn_consumption);
			silver_state = 0;
		    }
		    else
		    {
			silver_state = 2;
			naf.where     = TO_OAFFECTS;
			naf.type      = gsn_consumption;
		      	naf.level     = paf->level;
                        naf.duration  = -1;
			naf.modifier  = 0;
			naf.location  = APPLY_HIDE;
			naf.bitvector = AFF_CONSUMPTION;
			affect_to_char(ch, &naf);
		    }
		}

	        else if (paf->type == gsn_readiness && get_modifier(ch->affected, gsn_readiness) < 1)
		{
		    send_to_char("You feel less prepared for combat.\n\r", ch);
		    affect_strip(ch, gsn_readiness);
		  
	    	    naf.where	  = TO_AFFECTS;
            	    naf.type 	  = gsn_readiness;
            	    naf.level 	  = ch->level; 
            	    naf.duration  = 16;
            	    naf.location  = 0;
            	    naf.modifier  = 0;
            	    naf.bitvector = 0;
		    affect_to_char(ch, &naf);
		}

		else if ((paf->type == gsn_demoniccontrol) && (paf->where == TO_AFFECTS)
		 && (paf->bitvector == AFF_CHARM) && ch->leader)
		{
		    CHAR_DATA *leader = ch->leader;

		    act("You feel your control over $N fade.", ch->leader, NULL, ch, TO_CHAR);
		    stop_follower(ch);
		    ch->tracking = leader;
		    ch->demontrack = leader;

		    if (leader->in_room == ch->in_room)
		    {
			set_fighting(ch, leader);
			act("$n screams in fury and attacks!", ch, NULL, NULL, TO_ROOM);
			multi_hit(ch, leader, TYPE_UNDEFINED);
		    }
		    if (ch->pIndexData->vnum == MOB_VNUM_GREATER_TASKMASTER)
		    {
		        int numdemons = 0;
			CHAR_DATA *demon = NULL;
			for (vch = char_list; vch; vch = vch->next)
			{
			    if (IS_NPC(vch)
			      && (vch->leader == leader)
			      && (vch != ch)
			      && is_affected(vch, gsn_demoniccontrol))
			    {
			        numdemons++;
				if (!demon
				  || (demon->level < vch->level))
				    demon = vch;
			    }
			}
			if (demon
			  && (numdemons > 3))
			{
		            act("You feel your control over $N fade.", ch->leader, NULL, demon, TO_CHAR);
		            stop_follower(demon);
		            demon->tracking = leader;
		            demon->demontrack = leader;

		            if (leader->in_room == demon->in_room)
		            {
			        set_fighting(demon, leader);
			        act("$n screams in fury and attacks!", demon, NULL, NULL, TO_ROOM);
			        multi_hit(demon, leader, TYPE_UNDEFINED);
		            }
			      

		        }
		    }
		}

		else if (paf->where == TO_AFFECTS && paf->bitvector == AFF_FLYING)
		{
		    if (IS_SET(race_table[ch->race].aff, AFF_FLY_NATURAL))
			SET_BIT(ch->affected_by, AFF_FLY_NATURAL);
		}   

		if (!ch||!ch->valid||!paf)
		    continue;

		if (paf->type == gsn_contact_agents)
		    agent_note(ch, paf->modifier, (CHAR_DATA *) (paf->point));
		
		if (paf->type == gsn_finditems)
			find_items_note(ch, (char *)paf->point);
		
		if (paf->type == gsn_inquire)
		{
		    if (paf->modifier == -1)
		    {
			send_to_char("You determine that your information inquiry will not be returning.\n\rPerhaps the messenger got lost, or worse.\n\r", ch);
			check_improve(ch, NULL, gsn_inquire, FALSE, 1);
		    }
		    else
		    {
			inquire_note(ch, paf);
			check_improve(ch, NULL, gsn_inquire, TRUE, 1);
		    }
		}

		if (paf->type == gsn_zeal || paf->type == gsn_ignore_pain || (paf->type == gsn_aspectoftheinferno && paf->modifier == 0))
		{
		    affect_remove(ch, paf);
		    update_pos(ch);
		    if (ch->position == POS_DEAD)
		    {
			send_to_char("Your grievous wounds catch up to you, and you collapse...\n\r", ch);
			act("$n is DEAD!!", ch, NULL, NULL, TO_ROOM);
			sprintf(buf,"%s died from severe wounds at %s [%i].\n\r",ch->name, ch->in_room->name, ch->in_room->vnum);
			log_string(buf);
			raw_kill(ch);
			break;
		    }
		}
		else	    
		    affect_remove( ch, paf );
	    }
	}

	if (!IS_NPC(ch) && is_affected(ch, gsn_listen))
        {
	    direction = -1;
            for (af = ch->affected ; af != NULL ; af = af->next)      
	        if (af->type == gsn_listen)
                {
                    direction = af->modifier;
                    continue;
                }

	    if (direction > -1 && ch && ch->in_room && ch->in_room->exit[direction] && ch->in_room->exit[direction]->u1.to_room)
	        for (vch = ch->in_room->exit[direction]->u1.to_room->people ; vch != NULL ; vch = vch->next_in_room)
		    if (!is_affected(vch, AFF_WIZI) && !is_affected(vch, AFF_SNEAK) && !is_affected(vch, gsn_wildmove) && (!is_affected(vch, gsn_forestwalk) || ((ch->in_room->sector_type != SECT_FOREST) && (ch->in_room->sector_type != SECT_SWAMP))))
		        send_to_char("You hear some noise from the next room.\n\r", ch);
        }

        /*
         * Careful with the damages here,
         *   MUST NOT refer to ch after damage taken,
         *   as it may be lethal damage (on NPC).
         */

        if (is_affected(ch, gsn_waspstrike) && ch != NULL && ch->in_room)
        {
            int dam;

	        act("$n moans as $e bleeds from $s wounds.", ch,NULL,NULL,TO_ROOM);
	        send_to_char("You bleed from your open wound.\n\r",ch);
	        dam = dice(get_modifier(ch->affected, gsn_waspstrike)*(-2), 4);
            checkAutoCauterize(ch, gsn_waspstrike);
    	    damage( ch,ch,dam,gsn_waspstrike,DAM_NONE,FALSE);
       	} 

    if (ch != NULL && is_affected(ch, gsn_barbs) && ch->in_room)
	{
	    act("$n moans as $e bleeds from $s wounds.", ch, NULL, NULL, TO_ROOM);
	    send_to_char("You bleed from an open wound.\n\r", ch);
        checkAutoCauterize(ch, gsn_barbs);
	    damage(ch, ch, dice(2,4), gsn_barbs, DAM_NONE, FALSE);
	}

	if (ch && is_affected(ch, gsn_slow_cure_poison))
	{
	   send_to_char("You feel the cleansing herbs continue to course through your body.\n\r", ch);
	   spell_cure_poison(gsn_cure_poison, ch->level, ch, (void *) ch, TARGET_CHAR);
	}

	if (ch && is_affected(ch, gsn_slow_cure_disease))
	{
	    send_to_char("You feel the cleansing herbs continue to course through your body.\n\r", ch);
	    spell_cure_disease(gsn_cure_disease, ch->level, ch, (void *) ch, TARGET_CHAR);
	}

	if (ch && is_affected(ch, gsn_nettles))
	    if (number_bits(2) == 0)
		act("$n scratches $s skin.",ch,NULL,NULL,TO_ROOM);

	if (ch && is_affected(ch, gsn_insectswarm))
	{
	    AFFECT_DATA naf;
naf.valid = TRUE;
naf.point = NULL;
	    if (!IS_AFFECTED(ch, AFF_BLIND) && (number_bits(4) == 0))
	    {
		send_to_char("The swarming insects continue to sting and bite, blinding you!\n\r", ch);
		act("The insects swarming around $n continue to sting and bite, blinding $m!", ch, NULL, NULL, TO_ROOM);
		
		naf.where	 = TO_AFFECTS;
		naf.type	 = gsn_insectswarm;
		naf.level	 = ch->level;
		naf.modifier	 = -4;
		naf.location	 = APPLY_HITROLL;
		naf.duration	 = 0;
		naf.bitvector	 = AFF_BLIND;
		affect_to_char(ch, &naf);
	    }
	    else
	    {
		send_to_char("The swarming insects continue to sting and bite at you!\n\r", ch);
		act("The insects swarming around $n continue to sting and bite at $m!", ch, NULL, NULL, TO_ROOM);
	    }

	    damage( ch, ch, number_range((ch->level / 10), (ch->level / 2)), gsn_insectswarm, DAM_OTHER, TRUE);
	}

        if (is_affected(ch, gsn_vitalstrike) && ch != NULL && ch->in_room)
        {
            int dam;

	    act("$n groans as $s wound bleeds.",
		ch,NULL,NULL,TO_ROOM);
	    send_to_char("You bleed from an open wound.\n\r",ch);
        checkAutoCauterize(ch, gsn_vitalstrike);
	    dam = ch->level/3+1;
	    damage( ch ,ch,dam,gsn_vitalstrike,DAM_NONE,FALSE);
       	} 

        if (is_affected(ch, gsn_impale) && ch != NULL && ch->in_room)
        {
            int dam;

	    act("$n groans as $s open wound bleeds.",
		ch,NULL,NULL,TO_ROOM);
	    send_to_char("You bleed from your open wound.\n\r",ch);
        checkAutoCauterize(ch, gsn_impale);
	    dam = ch->level/2+1;
	    expend_mana(ch, dam);
	    ch->move -= dam;
	    damage(ch,ch,dam,gsn_impale,DAM_NONE,FALSE);
       	} 
        
	if (is_affected(ch, gsn_hamstring) && ch != NULL && ch->in_room)
    {
            int dam;

        act("$n groans as $s open wound bleeds.", ch,NULL,NULL,TO_ROOM);
        send_to_char("You bleed from your open wound.\n\r",ch);
        checkAutoCauterize(ch, gsn_hamstring);
        dam = ch->level/2+1;
        expend_mana(ch, dam);
        ch->move -= dam;
        damage(ch,ch,dam,gsn_hamstring,DAM_NONE,FALSE);
    } 

    if (ch != NULL && ch->in_room != NULL)
    {
        AFFECT_DATA * scorched(get_affect(ch, gsn_scorchedearth));
        if (scorched != NULL && scorched->bitvector == AFF_BLEEDING && scorched->where == TO_OAFFECTS)
        {
            act("$n groans as $s open wound bleeds.", ch,NULL,NULL,TO_ROOM);
            send_to_char("You bleed from your open wound.\n\r",ch);
            checkAutoCauterize(ch, gsn_scorchedearth);
            int dam = ch->level / 2 + 1;
            expend_mana(ch, dam);
            ch->move -= dam;
            damage(ch, ch, dam, gsn_scorchedearth, DAM_NONE, FALSE);
        } 
    }

	if (is_affected(ch, gsn_gash) && (ch != NULL && ch->in_room))
	{
	    AFFECT_DATA *af;
	    int dam;
	    af = affect_find(ch->affected,gsn_gash);
	    act("$n groans as $s open wound bleeds.",ch,NULL,NULL,TO_ROOM);
	    send_to_char("You bleed from your open wound.\n\r",ch);
        checkAutoCauterize(ch, gsn_gash);
	    dam = af->level/2+1;
	    damage(ch,ch,dam,gsn_gash,DAM_NONE,FALSE);
	}

	if (!IS_NPC(ch) && is_affected(ch, gsn_meldwithstone))
	{
	    if (ch->mana < 40)
	    {
            send_to_char("You cannot maintain your unity with the stone.\n\r", ch);
            affect_strip(ch, gsn_meldwithstone);
            act("You grow restless in the earth, and rise up from the ground.", ch, NULL, NULL, TO_CHAR);
            act("With a low rumble, the ground seems to part as $n flows up from it and reforms.", ch, NULL, NULL, TO_ROOM);
	    }
	    expend_mana(ch, 40);
	}

    if (is_affected(ch, gsn_flameunity))
    {
        if (ch->mana >= 60)
            expend_mana(ch, 60);
        else
        {
            send_to_char("You cannot maintain your unity with the flames.\n", ch);
            affect_strip(ch, gsn_flameunity);
            act("You stir, recorporealizing from the flames.", ch, NULL, NULL, TO_CHAR);
            act("$n emerges from the flames, unharmed despite the ash which swirls about $m.", ch, NULL, NULL, TO_ROOM);
        }
    }

	if (ch)
	{
	    AFFECT_DATA *aaf, *aaf_next;

	    for (aaf = ch->affected; aaf; aaf = aaf_next)
	    {
		aaf_next = aaf->next;

		if (aaf->type == gsn_aggravatewounds)
		{
		    damage(ch, ch, number_range(aaf->level/5, aaf->level/2), gsn_aggravatewounds, DAM_FIRE, TRUE);
		    if (!ch || IS_OAFFECTED(ch, AFF_GHOST))
		        break;
		}
	    }
	}

	if (is_affected(ch, gsn_adrenaline_rush))
	    ch->move -= 50;

	if (is_affected(ch, gsn_shadowmastery))
	{
	    if (ch->mana < 10)
	    {
		send_to_char("You slip out of the concealing shadows.\n\r", ch);
		affect_strip(ch, gsn_shadowmastery);
		REMOVE_BIT(ch->oaffected_by, AFF_SHADOWMASTERY);
	    }
	    else
		    expend_mana(ch, 10);
	}

    if (is_affected(ch, gsn_consume))
	{
	    damage_old(ch, ch, number_range(ch->level/2, (ch->level*3)/2), gsn_consume,DAM_FIRE,TRUE);
	    act("$n screams as the burning in $s skin incinerates $m!", ch, NULL, NULL, TO_ROOM);
	    act("You scream as the burning in your skin incinerates you!", ch, NULL, NULL, TO_CHAR);
	}

        if (is_affected(ch, gsn_ignite))
	{
	    damage_old( ch, ch, number_range(ch->level/2, (ch->level*3)/2), gsn_ignite, DAM_FIRE, TRUE);
	    act("$n screams as flames continue to lick across $s body!", ch, NULL, NULL, TO_ROOM);
	    act("You scream as flames continue to lick across your body!", ch, NULL, NULL, TO_CHAR);
	}

	if (is_affected(ch, gsn_enflamed))
	{
	    damage_old(ch, ch, number_range(ch->level/2, (ch->level*7)/4), gsn_enflamed, DAM_FIRE, TRUE);
	    act("$n screams as flames continue to lick across $s body!", ch, NULL, NULL, TO_ROOM);
	    act("You scream as flames continue to lick across your body!", ch, NULL, NULL, TO_CHAR);
	}

        if (((af=affect_find(ch->affected,gsn_fever)) != NULL) && ch->in_room 
        && !check_group_fieldmedicine_save(af->type, af->level, ch))
        {
            CHAR_DATA *vch;
            AFFECT_DATA plague;
	    plague.point = NULL;
	    plague.valid = TRUE; 
	    int dam;
	   
	    act("$n moans as $s skin burns with fever.",ch,NULL,NULL,TO_ROOM);
	    send_to_char("You burn up with the fever.\n\r",ch);
        
            if (af->level > 1)
	    {
	        plague.where	 = TO_AFFECTS;
                plague.type 	 = gsn_fever;
                plague.level 	 = af->level - 1; 
                plague.duration  = number_range(1,2 * plague.level);
                plague.location	 = APPLY_STR;
                plague.modifier  = -5;
                plague.bitvector = AFF_PLAGUE;
                for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
                {
                    if (!saves_spell(plague.level - 2, NULL, vch,DAM_DISEASE) 
		    &&  (!IS_NPC(vch) || !IS_AFFECTED(vch, AFF_WIZI))
		    &&  !IS_IMMORTAL(vch)
		    &&  !IS_OAFFECTED(vch,AFF_GHOST)
            	    &&  !IS_AFFECTED(vch,AFF_PLAGUE) && number_bits(4) == 0)
            	    {
            	        send_to_char("You feel hot and feverish.\n\r",vch);
            	        act("$n looks pale and feverish.", vch,NULL,NULL,TO_ROOM);
            	        affect_to_char(vch,&plague);
            	    }
                }

	        dam = UMIN(ch->level,af->level/3+1);
	        expend_mana(ch, dam);
	        ch->move -= dam;
	        damage_old( ch, ch, dam, gsn_fever,DAM_DISEASE,FALSE);
	    }
        }

	if (is_affected(ch, gsn_enslave) && !IS_NPC(ch))
	{
	    for (vch = char_list; vch; vch = vch->next)
		if (IS_NPC(vch)
		 && !IS_AFFECTED(vch, AFF_CHARM)
		 && is_affected(vch, gsn_enslave)
		 && (get_modifier(vch->affected, gsn_enslave) == ch->id))
		{
		    if (vch->in_room == ch->in_room)
		    {
	    	        act("You continue exerting your will, attempting to enslave $N.", ch, NULL, vch, TO_CHAR);
			act("$n continues to stare balefully at $N, attempting to break $S will.", ch, NULL, vch, TO_ROOM);
		    }
		    else
		    {
			affect_strip(ch, gsn_enslave);
			affect_strip(vch, gsn_enslave);
		    }
		    break;
		}
	}
		
	if (ch && is_affected(ch, gsn_plague_madness))
        {
            AFFECT_DATA *af;
            int dam;

	    if (ch->in_room == NULL)
		return;
            
	    act("$n moans as blood continues to ooze from $s diseased pores.", ch,NULL,NULL,TO_ROOM);
	    send_to_char("You moan as blood continues to ooze from your diseased body.\n\r",ch);

            af = affect_find(ch->affected,gsn_plague_madness);
        
            if (af == NULL)
            {
		if (IS_AFFECTED(ch, AFF_PLAGUE))
            	REMOVE_BIT(ch->affected_by,AFF_PLAGUE);
            	return;
            }
        
            if (af->level == 1)
            	return;
        
            for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
            {
                if (!saves_spell(af->level - 2, NULL, vch,DAM_DISEASE) 
		&&  (!IS_NPC(vch) || !IS_AFFECTED(vch, AFF_WIZI))
		&&  !IS_IMMORTAL(vch)
            	&&  !IS_AFFECTED(vch,AFF_PLAGUE) && number_bits(4) == 0)
            	{
            	    send_to_char("You moan as the plague of madness infects you.\n\r",vch);
            	    act("$n moans as the plague of madness overtakes $m.",
 			vch,NULL,NULL,TO_ROOM);
            	    affect_join(vch,af);

		    add_event((void *) vch, 1, &event_plague_madness);
            	}
            }

	    dam = UMIN(ch->level,af->level/2+1);
	    expend_mana(ch, dam);
	    ch->move -= dam;
	    damage_old( ch,ch,dam,gsn_plague_madness,DAM_DISEASE,FALSE);
	}

    af = get_affect(ch, gsn_pox);
	if (af != NULL && ch->in_room != NULL && !check_group_fieldmedicine_save(af->type, af->level, ch))
    {
        int dam;

	    act("$n moans as $s illness causes intense pain.", ch, NULL, NULL, TO_ROOM);
	    send_to_char("You feel your body being eaten up inside.\n\r", ch);

        for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (!saves_spell(af->level - 2, NULL, vch,DAM_DISEASE) 
            &&  (!IS_NPC(vch) || !IS_AFFECTED(vch, AFF_WIZI))
            &&  !IS_IMMORTAL(vch)
            &&  !IS_OAFFECTED(vch, AFF_GHOST)
            &&  !IS_AFFECTED(vch,AFF_PLAGUE)
            &&  number_bits(4) == 0)
            {
                send_to_char("You feel hot and feverish.\n\r", vch);
                act("$n looks pale and feverish.", vch, NULL, NULL, TO_ROOM);
                affect_join(vch,af);
            }
        }

	    dam = UMIN(ch->level,af->level/3+1);
	    expend_mana(ch, dam);
	    ch->move -= dam;
	    damage_old( ch,ch,dam,gsn_pox,DAM_DISEASE,FALSE);
    }

	if ( IS_AFFECTED(ch, AFF_POISON) && ch != NULL
	     &&   !IS_AFFECTED(ch,AFF_SLOW))

	{
	    AFFECT_DATA *poison;

	    poison = affect_find(ch->affected,gsn_poison);
	    if (poison == NULL)
		poison = affect_find(ch->affected,gsn_cloudkill);

	    if (poison != NULL)
	    {
		int tx = poison->level > 10 ? poison->level/2 : 5;
	        act( "$n shivers and suffers.", ch, NULL, NULL, TO_ROOM );
	        send_to_char( "You shiver and suffer.\n\r", ch );
		damage_old( ch ,ch,number_range(tx-4,tx+4),gsn_poison,DAM_POISON,FALSE);
	    }
	}

	if (IS_AFFECTED(ch, AFF_PLAGUE))
    {
        AFFECT_DATA * poison(get_affect(ch, gsn_plague));
        if (poison != NULL && !check_group_fieldmedicine_save(poison->type, poison->level, ch))
        {
            AFFECT_DATA plague = {0};
            plague.where		= TO_AFFECTS;
            plague.type 		= gsn_plague;
            plague.level 		= poison->level - 1; 
            plague.duration 	= number_range(1,2 * plague.level);
            plague.location		= APPLY_STR;
            plague.modifier 	= -5;
            plague.bitvector 	= AFF_PLAGUE;
        
            for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
            {
                if (!saves_spell(plague.level - 3, NULL, vch,DAM_DISEASE) 
                &&  !IS_IMMORTAL(vch)
                &&  (!IS_NPC(vch) || !IS_AFFECTED(vch, AFF_WIZI))
                &&  !IS_AFFECTED(vch,AFF_PLAGUE) && number_bits(4) == 0)
                {
                    send_to_char("You feel sick.\n\r",vch);
                    act("$n looks very sick.", vch,NULL,NULL,TO_ROOM);
                    affect_join(vch,&plague);
                }
            }

            if (poison != NULL)
            {
                int tx = poison->level > 10 ? poison->level/2 : 5;
                act( "$n twitches in agony from the plague.", ch, NULL, NULL, TO_ROOM );
                send_to_char( "You twitch in agony from the plague.\n\r", ch );
                damage_old( ch, ch,number_range(tx-4,tx+4),gsn_plague,DAM_DISEASE,FALSE);
            }
        }
	}

	if (ch && is_affected(ch, gsn_wingsofflame))
	{
	    send_to_char("Your wings of fire {rsinge{x you slightly.\n\r", ch);
	    damage_old(ch,ch,number_range(1, ch->level/4),gsn_wingsofflame,DAM_FIRE,FALSE);
	}
	if (ch && ch->in_room && is_affected(ch, gsn_concealremains))
	{
	    for (vobj = ch->in_room->contents;vobj;vobj = vobj->next_content)
		if (vobj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC
		  || vobj->pIndexData->vnum == OBJ_VNUM_CORPSE_NPC)
		    break;
	    if (vobj == NULL)
	    {
		int x=1000;
		if (!IS_NPC(ch))
		    for (x=0;x<MAX_FOCUS;x++)
			if (ch->pcdata->focus_on[x])
			    if (*(skill_table[ch->pcdata->focus_sn[x]].pgsn) == gsn_concealremains)
				break;
		if (x < MAX_FOCUS)
		    unfocus(ch,x,TRUE);
	    }
	}
	

    /* aging check */
        if (ch)
            get_age(ch);
    }

    /*
     * Autosave and autoquit.
     * Check that these chars still exist.
     */
    for ( ch = char_list; ch != NULL; ch = ch_next )
    {
        ch_next = ch->next;

        if (ch->desc != NULL && ch->desc->descriptor % 30 == save_number)
            save_char_obj(ch);

            if ( ch == ch_quit && !IS_NPC(ch))
                do_quit( ch, "" );
        if ( ch == ch_quit && IS_NPC(ch))
        {
            act("$n retreats to $s home.", ch, NULL, NULL, TO_ROOM);
	        extract_char(ch, TRUE);
        }
    }
}


/*
 *
 * Stones are cool. Make more fall from the sky, maybe, if there aren't at least 10
 * out there somewhere.
 *
 */
void stone_update( void )
{
   OBJ_DATA *obj;
   OBJ_DATA *obj_next;
   OBJ_DATA *stone;
   int count;
   DESCRIPTOR_DATA *d;
   ROOM_INDEX_DATA *room;

    count = 0;
    for ( obj = object_list; obj != NULL; obj = obj_next )
    {
	obj_next = obj->next;
	if (obj->pIndexData->vnum == OBJ_VNUM_STONE_POWER)
		count++;
    }

    if (count < 10 && number_bits(4) == 0)
    {
        room = get_random_room_no_char();
        stone = create_object(get_obj_index(OBJ_VNUM_STONE_POWER), 1);
        obj_to_room(stone, room);
        return;
    }

    if (count > 10)
    {
	for (d = descriptor_list; d != NULL; d = d->next)
	    if (d && d->character && (d->character->level > 51))
		send_to_char("Your stomach grows queasy as you realize more than 10 stones of power are in Avendar.\n\r", d->character);
    }
}

/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update( void )
{   
    CHAR_DATA *zombie = NULL;
    CHAR_DATA *owner = NULL, *oldowner = NULL;
    OBJ_DATA *obj = NULL;
    OBJ_DATA *obj_next = NULL;
    AFFECT_DATA *paf = NULL, *paf_next = NULL;
    OBJ_DATA *tobj = NULL, *tobj_next = NULL;
    char buf[MAX_STRING_LENGTH];
    int level;

    for ( obj = object_list; obj != NULL; obj = obj_next )
    {
	    CHAR_DATA *rch;
    	char *message;
	    obj_next = obj->next;

        // Check for waterwheel charging
        if (obj->pIndexData->vnum == OBJ_VNUM_WATERWHEEL)
            check_waterwheel_charging(*obj);

        // Check for rune of empowerment
        if (obj->worn_on && number_bits(6) == 0 && Runes::IsInvoked(*obj, Rune::Empowerment))
        {
            // Adjust odds for sector type
            int chance(0);
            ROOM_INDEX_DATA * room(get_room_for_obj(*obj));
            if (room != NULL)
            {
                switch (room->sector_type)
                {
                    case SECT_AIR:
                    case SECT_WATER_NOSWIM:
                    case SECT_WATER_SWIM:
                    case SECT_UNDERWATER:
                        break;

                    case SECT_UNDERGROUND:  chance += 50; break;
                    case SECT_MOUNTAIN:     chance += 40; break;
                    case SECT_HILLS:        chance += 30; break;
                    case SECT_SWAMP:        chance += 10; break;
                    default:                chance += 20; break;
                }
            }

            // Filter out certain sector types
            if (chance > 0)
            {
                // Get the current modifier; odds are improved for lower levels of empowerment
                int currMod(get_modifier(obj->affected, gsn_empowerment));
                currMod = UMAX(0, currMod);
                if (currMod < 10) chance += (12 - currMod) * 50;
                else if (currMod > 25) chance = UMIN(chance, 10);
                
                // Check for empowerment
                if (number_range(1, 1000) <= chance)
                {
                    // Join the effect
                    AFFECT_DATA af = {0};
                    af.where    = TO_OBJECT;
                    af.type     = gsn_empowerment;
                    af.location = APPLY_MANA;
                    af.modifier = 1;
                    af.duration = -1;
                    obj_affect_join(obj, &af);

                    // Echo to the room
                    if (room != NULL)
                        act("The rune of empowerment on $p gleams brightly as it drinks in the power of the earth.", room->people, obj, NULL, TO_ALL);
                }
            }
        }

        // Handle light objects burning
        if (obj->item_type == ITEM_LIGHT && IS_SET(obj->value[1], LIGHT_ALWAYS_BURN) && obj->value[2] > 0)
        {
            CHAR_DATA * ch(obj->carried_by);
            ROOM_INDEX_DATA * room(obj->in_room);
            if (IS_SET(obj->value[1], LIGHT_ALWAYS_BURN) || (ch != NULL && get_eq_char(ch, WEAR_LIGHT) == obj))
            {
                // Light is always burn or is worn, so decrement
                --obj->value[2];
                if (obj->value[2] == 0)
                {
                    // Light has burned out; figure out a target char
                    if (ch == NULL && room != NULL) 
                        ch = room->people;

                    if (ch != NULL && ch->in_room != NULL)
                    {
                        // Remove the light
                        room = ch->in_room;
                        act("$p flickers and goes out.", ch, obj, NULL, TO_ROOM);
                        act("$p flickers and goes out.", ch, obj, NULL, TO_CHAR);
                        extract_obj(obj);
                        continue;
                    }

                    // Decrement the light in the room
                    if (room != NULL) 
                        --room->light;
                }
            }
        }

	/* go through affects and decrement */
        for ( paf = obj->affected; paf != NULL; paf = paf_next )
        {
            paf_next    = paf->next;
            if ( paf->duration > 0 )
            {
                paf->duration--;
                
                if (paf->type == gsn_seedofgamaloth && obj->in_room && obj->in_room->people)
                {
                    act("$p begins to wriggle.",obj->in_room->people,obj,NULL,TO_CHAR);
                    act("$p begins to wriggle.",obj->in_room->people,obj,NULL,TO_ROOM);
                }

                // Check for eyeblight touch
                if (paf->type == gsn_eyeblighttouch)
                    check_eyeblighttouch(*obj, *paf);
            }
            else if ( paf->duration < 0 )
            {
                // Check for mirrored illusion objects
                if (paf->type == gsn_illusion && current_time >= paf->modifier)
                {
                    CHAR_DATA * bearer(obj->carried_by);
                    if (paf->point == NULL) 
                    {
                        act("$p flickers, then suddenly crumbles into nothingness!", bearer, obj, NULL, TO_ALL);
                        extract_obj(obj);
                        break;
                    }
                    
                    act("$p flickers for a moment, then seems to grow more substantial, as though a fog were lifted from it!", bearer, obj, NULL, TO_ALL);

                    bool wearAgain(false);
                    if (obj->worn_on)
                        wearAgain = remove_obj(bearer, obj, true);

                    OBJ_DATA * newObj(static_cast<OBJ_DATA*>(paf->point));
                    clone_object(newObj, obj);
                    extract_obj(newObj);
                    paf->point = NULL;
                    affect_remove_obj(obj, paf);

                    if (wearAgain)
                        wear_obj(bearer, obj, true, false, false);

                    continue;
                }
           }
            else
            {
                if ( paf_next == NULL
                ||   paf_next->type != paf->type
                ||   paf_next->duration > 0 )
                {
                    if ( paf->type > 0 && skill_table[paf->type].msg_obj )
                    {
			if (obj->carried_by != NULL)
			{
			    rch = obj->carried_by;
			    act(skill_table[paf->type].msg_obj,
				rch,obj,NULL,TO_CHAR);
			}
			if (obj->in_room != NULL && obj->in_room->people != NULL)
			{
			    rch = obj->in_room->people;
			    act(skill_table[paf->type].msg_obj,
				rch,obj,NULL,TO_ALL);
			}

            // Check for dancing sword / hovering shield dropping
            if ( paf->type == gsn_dancingsword || paf->type == gsn_hoveringshield)
			{
			    if (obj->carried_by && obj->worn_on)
                {
                    if (!obj->carried_by->in_room)
                        continue;
                
                    act("$p loses its buoyancy, and returns to $n.", obj->carried_by, obj, NULL, TO_ROOM);
                    act("$p loses its buoyancy, and returns to you.", obj->carried_by, obj, NULL, TO_CHAR);
                    CHAR_DATA *cb = obj->carried_by;
                    unequip_char(obj->carried_by, obj);
                    obj_from_char(obj);
                    obj_to_char(obj,cb);
                }
			}

			else if (paf->type == gsn_strengthen)
			{
			    if (obj->carried_by)
				if (obj->carried_by->in_room == NULL)
				    continue;
			 	else
				    act("$p looks less resistant to damage.",obj->carried_by,obj,NULL,TO_CHAR);
			}
		        else if ( paf->type == gsn_beltoflooters )
			{
			    obj->value[0] = 0;
			    obj->value[3] = 0;
			}
			else if (paf->type == gsn_cursebarkja)
			{
                            AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

                            if (obj->carried_by && obj->worn_on && 
                                  ((obj->item_type == ITEM_JEWELRY) || (obj->item_type == ITEM_GEM) || (obj->item_type == ITEM_TREASURE)))
		            {
                                AFFECT_DATA * paf2;

			        paf2 = affect_find(obj->carried_by->affected, gsn_cursebarkja);
				if (paf2 && (paf2->location == APPLY_HIT))
			        {
				    paf2->modifier -= 2;
				    if((obj->carried_by->max_hit -= 2) <= 0)
			            {
				        sprintf(buf,"%s killed by the Curse of Barkja on %s in %s [%i].\n\r",obj->carried_by->name,obj->name,obj->carried_by->in_room->name,obj->carried_by->in_room->vnum);
					log_string(buf);
				        raw_kill(obj->carried_by);
			            }
				    else
				        obj->carried_by->hit = UMIN(obj->carried_by->hit, obj->carried_by->max_hit);
                                }
			        else 
				{
				    af.where	= TO_CHAR;
				    af.type	= gsn_cursebarkja;
			            af.level 	= obj->level;
				    af.duration	= -1;
				    af.location	= APPLY_HIT;
				    af.modifier	= -2;
				    af.bitvector= 0;
				    affect_to_char(obj->carried_by, &af);
                                }
		            }
                            af.where    = TO_OBJECT;
                            af.type     = gsn_cursebarkja;
    			    af.level    = paf->level;
    			    af.duration = 3;
    			    af.location = 0;
    			    af.modifier = paf->modifier;
    			    af.bitvector= 0;
    		            affect_to_obj(obj, &af);
			}
			else if (paf->type == gsn_seedofgamaloth)
			{
			    act("$p lurches upright.",obj->in_room->people,obj,NULL,TO_CHAR);
			    act("$p lurches upright.",obj->in_room->people,obj,NULL,TO_ROOM);
			    CHAR_DATA *seedling = NULL;
			    if ((seedling = create_mobile(get_mob_index(MOB_VNUM_SEEDLING_GAMALOTH))) == NULL) 
				bug("No seedling mob.",0);
			    else
			    {
				if (obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC)
				    levelize_mobile(seedling,obj->level+5);
				else
				    levelize_mobile(seedling,obj->level);
				AFFECT_DATA af;
				af.type = gsn_seedofgamaloth;
				af.where = TO_CHAR;
				af.modifier = 0;
				af.duration = 48;
				af.bitvector = 0;
				af.level = paf->level;
				af.location = APPLY_NONE;
				affect_to_char(seedling,&af);
				char_to_room(seedling,obj->in_room);
			    }
			}
                    }
                }

                affect_remove_obj( obj, paf );
            }
        }

	oprog_time_trigger(obj);
	oprog_tick_trigger(obj);

	if (!obj || !obj->valid)
	  continue;

        if ( obj->opactnum > 0) /* && obj->in_room->area->nplayer > 0 )*/
        {
            OPROG_ACT_LIST *tmp_act, *tmp2_act;
            for ( tmp_act = obj->opact; tmp_act != NULL; tmp_act = tmp_act->next )
	        {
                 oprog_wordlist_check( tmp_act->buf,obj, tmp_act->ch, tmp_act->obj, tmp_act->vo, ACT_PROG);
                 free_string( tmp_act->buf );
            }
            for ( tmp_act = obj->opact; tmp_act != NULL; tmp_act = tmp2_act )
	        {
                 tmp2_act = tmp_act->next;
                 free(tmp_act);
            }
            obj->opactnum = 0;
            obj->opact    = NULL;
        }

	if ((obj->pIndexData->vnum == OBJ_VNUM_STONE_POWER)
	 && !obj->carried_by
	 && obj->in_room
	 && !IS_SET(obj->in_room->room_flags, ROOM_VAULT)
	 && (number_bits(6) == 0))
	{
	    act("A Stone of Power flashes and disappears!", obj->in_room->people, NULL, NULL, TO_CHAR);
	    act("A Stone of Power flashes and disappears!", obj->in_room->people, NULL, NULL, TO_ROOM);
	    obj_from_room(obj);
	    obj_to_room(obj, get_random_room_no_char());
	}

	if ((obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_CORPSE_NPC) && obj->carried_by != NULL
	 && (area_is_affected(obj->carried_by->in_room->area, gsn_deathwalk)
	|| ((silver_state == SILVER_FINAL) && (number_bits(3) == 0))))
	{
	    zombie = create_mobile(get_mob_index(MOB_VNUM_ZOMBIE));
	    level = zombie->level = obj->level;
	    zombie->hit   = 30*level;
	    zombie->max_hit = 30*level;
	    zombie->hitroll = level;
	    zombie->damroll = level;
	    zombie->damage[0]  = level/5;
	    zombie->damage[1]  = 4;
	    zombie->damage[2]  = level/5;
	    zombie->armor[0]   = -1*level;
	    zombie->armor[1]   = -1*level;
	    zombie->armor[2]   = -1*level;
	    zombie->armor[3]   = -1*level;

	    if ((obj->value[0] <= 0) && (paf = affect_find(obj->carried_by->in_room->area->affected, gsn_deathwalk)) != NULL)
		paf->modifier += level;

	    sprintf( buf, zombie->short_descr, &obj->short_descr[IS_SET(obj->value[1], CORPSE_DESTROYED) ? 25 : 14] );
    	    free_string( zombie->short_descr );
    	    zombie->short_descr = str_dup( buf );
    	    sprintf( buf, zombie->long_descr, &obj->short_descr[IS_SET(obj->value[1], CORPSE_DESTROYED) ? 25 : 14] );
    	    free_string( zombie->long_descr);
    	    zombie->long_descr = str_dup( buf );
	    act("Malefic forces enfuse $p, and a zombie rises from the corpse!", obj->carried_by, obj, NULL, TO_ROOM);
	    char_to_room(zombie, obj->carried_by->in_room);
	    for ( tobj = obj->contains; tobj != NULL; tobj = tobj_next)
	    {
		tobj_next = tobj->next_content;
		if (tobj == NULL)
			break;
		obj_from_obj(tobj);
		obj_to_char(tobj, zombie);
	    }
	    extract_obj(obj);
	    continue;
	}
         
        if  ((obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_CORPSE_NPC) && obj->in_room != NULL
	&& (area_is_affected(obj->in_room->area, gsn_deathwalk)
	|| ((silver_state == SILVER_FINAL) && (number_bits(3) == 0))))
	{
	    zombie = create_mobile(get_mob_index(MOB_VNUM_ZOMBIE));
	    level = zombie->level = obj->level;
	    zombie->hit   = 30*level;
	    zombie->max_hit = 30*level;
	    zombie->hitroll = level;
	    zombie->damroll = level;
	    zombie->damage[0]  = level/5;
	    zombie->damage[1]  = 4;
	    zombie->damage[2]  = level/5;
	    zombie->armor[0]   = -1*level;
	    zombie->armor[1]   = -1*level;
	    zombie->armor[2]   = -1*level;
	    zombie->armor[3]   = -1*level;

	    if ((obj->value[0] <= 0) && (paf = affect_find(obj->in_room->area->affected, gsn_deathwalk)) != NULL)
		paf->modifier += level;

    	    sprintf( buf, zombie->short_descr, &obj->short_descr[IS_SET(obj->value[1], CORPSE_DESTROYED) ? 25 : 14] );
    	    free_string( zombie->short_descr );
    	    zombie->short_descr = str_dup( buf );
    	    sprintf( buf, zombie->long_descr, &obj->short_descr[IS_SET(obj->value[1], CORPSE_DESTROYED) ? 25 : 14] );
    	    free_string( zombie->long_descr);
    	    zombie->long_descr = str_dup( buf );
	    if (obj->in_room->people)
	    {
		act("Malefic forces enfuse $p, and a zombie rises from the corpse!", obj->in_room->people, obj, NULL, TO_ROOM);
		act("Malefic forces enfuse $p, and a zombie rises from the corpse!", obj->in_room->people, obj, NULL, TO_CHAR);
	    }
	    char_to_room(zombie, obj->in_room);
	    for ( tobj = obj->contains; tobj != NULL; tobj = tobj_next)
	    {
		tobj_next = tobj->next_content;
		if (tobj == NULL)
		    break;
		obj_from_obj(tobj);
		obj_to_char(tobj, zombie);
	    }
	    extract_obj(obj);
	    continue;
	}

        if (obj_is_affected(obj, gsn_cursebarkja) && (obj->item_type == ITEM_ARMOR) && obj->worn_on)
        {
            if (number_percent() == 42) // Life, the universe, and everything
            {
		AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
                int x;

                if ((x = number_percent()) < 11)
                {
                    if (!is_affected(obj->carried_by, skill_lookup("weaken")))
                    {
                        af.where     = TO_AFFECTS;
                        af.type      = skill_lookup("weaken");
                        af.level     = 60;
                        af.duration  = 60 / 2;
                        af.location  = APPLY_STR;
                        af.modifier  = -1 * (60 / 7);
                        af.bitvector = AFF_WEAKEN;
                        affect_to_char(obj->carried_by, &af);
                        act("You feel $p grow cold, drawing away your strength.", obj->carried_by, obj, NULL, TO_CHAR);
                        act("$n looks tired and weak.", obj->carried_by, NULL, NULL, TO_ROOM);
                    }
		}
		else if (!IS_AFFECTED(obj->carried_by, AFF_PLAGUE))
		{
		    if (x < 41)
	            {
                        af.where     = TO_AFFECTS;
			af.type      = gsn_pox;
			af.level     = 60 * 3/4;
			af.duration  = 60/2;
			af.location  = APPLY_STR;
			af.modifier  = (-1) * 60/10;
			af.bitvector = AFF_PLAGUE;
			affect_to_char(obj->carried_by, &af);
			act("A septic miasma oozes from $p, and you grimace as illness strikes you.", obj->carried_by, obj, NULL, TO_CHAR);
			act("$n grimaces and looks ill.", obj->carried_by, NULL, NULL, TO_ROOM);
	            }
		    else
	            {
                        af.where     = TO_AFFECTS;
                        af.type      = gsn_plague;
                        af.level     = 60 * 3/4;
                        af.duration  = 60;
                        af.location  = APPLY_STR;
                        af.modifier  = -5;
                        af.bitvector = AFF_PLAGUE;
                        affect_to_char(obj->carried_by, &af);
                        act("You scream in agony as the flesh around $p erupts in plague sores.", obj->carried_by, obj, NULL, TO_CHAR);
			act("$n screams in agony as plague sores erupt from $s skin.", obj->carried_by, NULL, NULL, TO_ROOM);
		    }
		}

            }
        }

	if (obj->extra_flags[0] & ITEM_AFFINITY)
	{
	/* translation: if object is not carried by its affinity owner */
	  if (!(obj->carried_by && is_affected(obj->carried_by, gsn_affinity)
		&& (get_modifier(obj->affected, gsn_affinity) ==
		get_modifier(obj->carried_by->affected, gsn_affinity))))
		{
		  if (!obj->carried_by && !obj->in_obj && obj->in_room && (owner = get_affinity_owner(obj)) != NULL)
		  {
		    if (obj->in_room->people)
		    {
		      act("$p glows for a moment, then simply disappears.", obj->in_room->people,
									obj, NULL, TO_ROOM);
		      act("$p glows for a moment, then simply disappears.", obj->in_room->people,
									obj, NULL, TO_CHAR);
		      
		    }
		      obj_from_room(obj);
		      obj_to_char(obj, get_affinity_owner(obj));
		      save_char_obj(get_affinity_owner(obj));
		    act("$p forms in a flash of light in midair, and drifts down to you.", owner, obj, NULL, TO_CHAR);
		    act("$p forms in a flash of light in midair, and drifts down to $n.", owner, obj, NULL, TO_ROOM);
		      continue;
	          }
		  if (!obj->carried_by && !obj->in_room && obj->in_obj && (owner = get_affinity_owner(obj)) != NULL)
		  {
		    act("$p forms in a flash of light in midair, and drifts down to you.", owner, obj, NULL, TO_CHAR);
		    act("$p forms in a flash of light in midair, and drifts down to $n.", owner, obj, NULL, TO_ROOM);
		    obj_from_obj(obj);
		    obj_to_char(obj, owner);
		    save_char_obj(get_affinity_owner(obj));
		    continue;
		  }
		  if (obj->carried_by && !obj->in_room && !obj->in_obj && (owner = get_affinity_owner(obj)) != NULL)
		  {
		    act("$p tears itself from you and disappears.", obj->carried_by, obj, NULL, TO_CHAR);
		    act("$p tears itself from $n and disappears.", obj->carried_by, obj, NULL, TO_ROOM);
		    oldowner = obj->carried_by;
		    obj_from_char(obj);
		    obj_to_char(obj, owner);
		    save_char_obj(owner);
		    save_char_obj(oldowner);
		    act("$p forms in a flash of light in midair, and drifts down to you.", owner, obj, NULL, TO_CHAR);
		    act("$p forms in a flash of light in midair, and drifts down to $n.", owner, obj, NULL, TO_ROOM);
		  }
		}

	  }
	if (obj->pIndexData->vnum == OBJ_VNUM_BLOOD_SIGIL)
	{
	    if (obj->carried_by == NULL)
	    {
		extract_obj(obj);
		continue;
	    }
	    if (!obj_is_affected(obj,gsn_bloodsigil))
	    {	
		send_to_char("Your blood sigil heals.\n\r",obj->carried_by);
	    	extract_obj(obj);
	    	continue;
	    }
	}
	if ( obj->timer <= 0 || --obj->timer > 0 )
	    continue;

	if (obj->carried_by && IS_NPC(obj->carried_by) && obj->carried_by->pIndexData->pShop)
	{
	    extract_obj(obj);
	    continue;
	}

	if (obj->pIndexData->vnum == OBJ_VNUM_TROPHY_HEAD)
	{
	    OBJ_DATA *skull;

	    skull = create_object(get_obj_index(OBJ_VNUM_TROPHY_SKULL), obj->level);
	    skull->level = obj->level;
	    sprintf(buf, skull->short_descr, &obj->short_descr[28]);
	    free_string(skull->short_descr);
	    skull->short_descr = str_dup(buf);
        setName(*skull, buf);
	    sprintf(buf, skull->description, &obj->short_descr[28]);
	    free_string(skull->description);
	    skull->description = str_dup(buf);

	    if (obj->in_obj)
		obj_to_obj(skull, obj->in_obj);
	    else if (obj->carried_by)
	    {
		obj_to_char(skull, obj->carried_by);
		act("$p decomposes, turning into a skull.", obj->carried_by, obj, NULL, TO_CHAR);
	    }
	    else if (obj->in_room)
		obj_to_room(skull, obj->in_room);
	    else
		extract_obj(skull);

	    extract_obj(obj);
	    continue;
	}	    

	switch ( obj->item_type )
    {
        default:
            if (obj->pIndexData->vnum == OBJ_VNUM_DARKTALLOW)
                message = "$p gutters darkly, then winks out and crumbles away.";
            else
                message = "$p crumbles into dust.";  
            break;
    
        case ITEM_ARROW:      
            message = "$p is consumed by flames and burns away.";
            if (obj->carried_by != NULL) 
                    obj->carried_by->nocked = NULL; 
                break;
        case ITEM_FOUNTAIN:   message = "$p dries up.";         break;
        case ITEM_CORPSE_NPC: message = "$p decays into dust."; break;
        case ITEM_CORPSE_PC:  message = "$p decays into dust."; break;
        case ITEM_LIGHT:      message = "$p flickers and goes out."; break;
        case ITEM_FOOD:       message = "$p decomposes.";	break;
        case ITEM_PILL:	      message = "$p decays and crumbles."; break;
        case ITEM_POTION:     message = "$p has evaporated from disuse.";	
                                    break;
        case ITEM_OIL:        message = "$p has dissolved from disuse."; break;
        case ITEM_PORTAL:     message = "$p fades out of existence."; break;
        case ITEM_CONTAINER: 
            if (CAN_WEAR(obj,ITEM_WEAR_FLOAT) || obj->pIndexData->vnum == OBJ_VNUM_DISC)
            {
                if (obj->contains) 
                    message = "$p flickers and vanishes, spilling its contents on the floor.";
                else
                    message = "$p flickers and vanishes.";
            }
            else if (obj->pIndexData->vnum == OBJ_VNUM_WATERWHEEL)
            {
                message = "With a final creaking, clanking sound, $p breaks apart, falling into ruin.";
                handle_waterwheel_destruction(*obj);
            }
            else
                message = "$p crumbles into dust.";
            break;
        
    }

	if ( obj->carried_by != NULL )
	{
	    act( message, obj->carried_by, obj, NULL, TO_CHAR );
	    if (IS_SET(obj->worn_on, WORN_FLOAT))
		act(message,obj->carried_by,obj,NULL,TO_ROOM);
	}

	if ((IS_SET(obj->worn_on, WORN_FLOAT) || obj->pIndexData->vnum == OBJ_VNUM_DISC) &&  obj->contains)
	{   /* save the contents */
     	    OBJ_DATA *t_obj, *next_obj;

	    for (t_obj = obj->contains; t_obj != NULL; t_obj = next_obj)
	    {
		next_obj = t_obj->next_content;
		obj_from_obj(t_obj);

		if (obj->in_obj) /* in another object */
		    obj_to_obj(t_obj,obj->in_obj);

		else if (obj->carried_by)  /* carried */
		{
		    if (IS_SET(obj->worn_on, WORN_FLOAT))
		    {
			if (obj->carried_by->in_room == NULL)
			    extract_obj(t_obj);
			else
			    obj_to_room(t_obj,obj->carried_by->in_room);
		    }
		    else
		    	obj_to_char(t_obj,obj->carried_by);
		}
		else if (obj->in_room == NULL)  /* destroy it */
		    extract_obj(t_obj);

		else /* to a room */
		    obj_to_room(t_obj,obj->in_room);
	    }
	}

	/* Now, if its a corpse, drop its contents into the right align pit */
	if (obj->item_type == ITEM_CORPSE_PC && obj->contains)
	{
	    ROOM_INDEX_DATA *pRoom = NULL;
	    OBJ_DATA *pit = NULL;
	    OBJ_DATA *pObj = NULL;
	    OBJ_DATA *pobj_next;

	    pRoom = get_room_index(obj->value[4]);

	    if (pRoom)
	        for (pit = pRoom->contents; pit; pit = pit->next_content)
		    if ((pit->item_type == ITEM_CONTAINER)
		     && is_name("corpsealtar", pit->name))
		        break;

   	    if (pit)
		for (pObj = obj->contains; pObj; pObj = pobj_next)
		{
		    pobj_next = pObj->next_content;
		    obj_from_obj(pObj);
		    obj_to_obj(pObj, pit);
		}
	    else if (pRoom)
		for (pObj = obj->contains; pObj; pObj = pobj_next)
		{
		    pobj_next = pObj->next_content;
		    obj_from_obj(pObj);
		    obj_to_room(pObj, pRoom);
		}
	    else if (obj->in_room)
		for (pObj = obj->contains; pObj; pObj = pobj_next)
		{
		    pobj_next = pObj->next_content;
		    obj_from_obj(pObj);
		    obj_to_room(pObj, obj->in_room);
		}
	}

	extract_obj( obj );
    }
}

/* Add a new event.  Events are processed in a LIFO manner. */

void add_event(void *vo, int timer, EVENT_FUN event)
{
    EVENT_DATA *ev = new_event();
    CHAIN_DATA *chain = NULL, *cl = event_list;

    // Create a new event_list if needed.
    if (!cl)
    {
	event_list = new_chain();
	cl = event_list;
    }
   
    while (timer > 0)
    {
	if (!cl->next_chain)
	{
	    chain = new_chain();

	    cl->nci = timer;
	    cl->next_chain = chain;
	    cl = chain;
	    break;
	}
	else if (cl->nci > timer)
	{
	    chain = new_chain();

	    chain->next_chain = cl->next_chain;
	    chain->nci = cl->nci - timer;
	    cl->nci = timer;
	    cl->next_chain = chain;
	    cl = chain;
	    break;
	}
	else
	{
	    timer -= cl->nci;
	    cl = cl->next_chain;
	}
    }

    // Adding to an existing chain
    if (!chain)
	ev->next = cl->event;

    cl->event = ev;

    // Okay, we've now added our new event to the proper place on the event stack.
    // Now to build the event.
	
    ev->efunc = event;
    ev->point = vo;
}

void process_events( void )
{
    EVENT_DATA *ev, *ev_next;
    CHAIN_DATA *nc;

    if (!event_list)
	return;

    for (ev = event_list->event; ev; ev = ev_next)
    {
	ev_next = ev->next;

	(*ev->efunc)(ev->point);

	free_event(ev);
    }

    if (event_list->nci > 1)
    {
	CHAIN_DATA *chain = new_chain();

	chain->next_chain = event_list->next_chain;
	chain->nci = event_list->nci - 1;
	event_list->next_chain = chain;
    }

    nc = event_list->next_chain;
    free_chain(event_list);
    event_list = nc;
}

void event_warpath(void * data)
{
    CHAR_DATA * ch = (CHAR_DATA*)data;
    if (is_affected(ch, gsn_warpath))
	add_event((void *) ch, 1, &event_warpath);
    else
	return;

    if (IS_AWAKE(ch) && ch->in_room && ch->hunting && ch->hunting->in_room
     && (ch->in_room == ch->hunting->in_room) && !ch->fighting
     && can_see(ch,ch->hunting) && !IS_OAFFECTED(ch->hunting,AFF_GHOST))
    {
        ch->fighting = ch->hunting;
        multi_hit(ch, ch->hunting, TYPE_UNDEFINED);
    }
}

void event_ashurmadness(void * data)
{
	CHAR_DATA * ch = (CHAR_DATA*)data;
    CHAR_DATA *vch;
    OBJ_DATA *obj;
    bool foods = FALSE;

    if (IS_NAFFECTED(ch, AFF_ASHURMADNESS))
	add_event((void *) ch, 1, &event_ashurmadness);
    else
	return;

    if (ch->fighting || !ch->in_room || !IS_AWAKE(ch))
	return;

    WAIT_STATE(ch, UMAX(ch->wait, 4));
    for (obj = ch->in_room->contents; obj != NULL; obj = obj->next_content)
	if (obj->pIndexData->vnum >= OBJ_VNUM_TORN_HEART && obj->pIndexData->vnum < OBJ_VNUM_SEVERED_HEAD)
	    break;

    if (obj != NULL)
    {
	foods = TRUE;
	obj_from_room(obj);
	obj_to_char(obj,ch);
	do_eat(ch, "1.");
	return;
    }

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
    {
	if (!IS_SET(vch->in_room->room_flags, ROOM_SAFE)
	 && !IS_AFFECTED(vch, AFF_CHARM)
	 && (ch != vch)
	 && !IS_OAFFECTED(vch, AFF_GHOST)
	 && (IS_NPC(vch) || IS_PK(vch, ch))
	 && can_see(ch, vch)
	 && (number_bits(1) != 0))
	{
	    send_to_char("Your eyes glaze as your fit of hunger takes you!\n\r", ch);

	    if ((obj = get_eq_char(ch, WEAR_WIELD)) && obj->item_type != ITEM_WEAPON)
	    {
		act("You throw down $p as you leap towards $N!", ch, obj, vch, TO_CHAR);
		act("$n throws down $p as $e leaps towards you!", ch, obj, vch, TO_VICT);
		act("$n throws down $p as $e leaps towards $N!", ch, obj, vch, TO_NOTVICT);

		unequip_char(ch, obj);
		if (!obj->worn_on && can_drop_obj(ch, obj))
		{
		    obj_from_char(obj);
		    obj_to_room(obj, ch->in_room);
		}
	    }

	    if ((obj = get_eq_char(ch, WEAR_DUAL_WIELD)) && obj->item_type != ITEM_WEAPON)
	    {
		act("You throw down $p as you leap towards $N!", ch, obj, vch, TO_CHAR);
		act("$n throws down $p as $e leaps towards you!", ch, obj, vch, TO_VICT);
		act("$n throws down $p as $e leaps towards $N!", ch, obj, vch, TO_NOTVICT);

		unequip_char(ch, obj);
		if (!obj->worn_on && can_drop_obj(ch, obj))
		{
		    obj_from_char(obj);
		    obj_to_room(obj, ch->in_room);
		}
	    }

	    if (IS_NPC(ch) && !IS_SET(ch->act, PLR_AUTOATTACK))
		SET_BIT(ch->act,PLR_AUTOATTACK);
	    multi_hit(ch, vch, TYPE_UNDEFINED);
	    foods = TRUE;
	    break;
	}
	
    }
    if (foods == FALSE)
    {
	int door,x;
	EXIT_DATA *pexit = NULL;
	while (x++ < 30)
	{
	    door = number_range(0,5);
	    if ((pexit = ch->in_room->exit[door]) != NULL)
	        if (!IS_SET(pexit->exit_info, EX_CLOSED))
		{
		    move_char(ch,door,TRUE);
		    break;
		}
		else if (!IS_SET(pexit->exit_info, EX_LOCKED))
		{
		    do_open(ch,dir_name[door]);
		    move_char(ch,door,TRUE);
		    break;
		}
	}
    }
}

void event_berserk(void * data)
{
    CHAR_DATA * ch = (CHAR_DATA*)data;
    CHAR_DATA *vch;
    OBJ_DATA *obj;

    if (is_affected(ch, gsn_berserk))
	add_event((void *) ch, 1, &event_berserk);
    else
	return;

    if (ch->fighting || !ch->in_room || !IS_AWAKE(ch))
	return;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
    {
	if (!IS_SET(vch->in_room->room_flags, ROOM_SAFE)
	 && !IS_AFFECTED(vch, AFF_CHARM)
	 && (ch != vch)
	 && !IS_OAFFECTED(vch, AFF_GHOST)
	 && (IS_NPC(vch) || IS_PK(vch, ch))
	 && can_see(ch, vch)
	 && (number_bits(1) != 0))
	{
	    send_to_char("Your eyes glaze as your fit of rage takes you!!\n\r", ch);

	    if ((obj = get_eq_char(ch, WEAR_WIELD)) && obj->item_type != ITEM_WEAPON)
	    {
		act("You throw down $p as you leap towards $N!", ch, obj, vch, TO_CHAR);
		act("$n throws down $p as $e leaps towards you!", ch, obj, vch, TO_VICT);
		act("$n throws down $p as $e leaps towards $N!", ch, obj, vch, TO_NOTVICT);

		unequip_char(ch, obj);
		if (!obj->worn_on && can_drop_obj(ch, obj))
		{
		    obj_from_char(obj);
		    obj_to_room(obj, ch->in_room);
		}
	    }

	    if ((obj = get_eq_char(ch, WEAR_DUAL_WIELD)) && obj->item_type != ITEM_WEAPON)
	    {
		act("You throw down $p as you leap towards $N!", ch, obj, vch, TO_CHAR);
		act("$n throws down $p as $e leaps towards you!", ch, obj, vch, TO_VICT);
		act("$n throws down $p as $e leaps towards $N!", ch, obj, vch, TO_NOTVICT);

		unequip_char(ch, obj);
		if (!obj->worn_on && can_drop_obj(ch, obj))
		{
		    obj_from_char(obj);
		    obj_to_room(obj, ch->in_room);
		}
	    }

	    multi_hit(ch, vch, TYPE_UNDEFINED);
	    break;
	}
    }
}

void event_stampede(void * data)
{
	CHAR_DATA * ch = (CHAR_DATA*) data;
    CHAR_DATA *vch;

    if (is_affected(ch, gsn_stampede))
	add_event((void *) ch, 1, &event_stampede);
    else
	return;

    if (ch->fighting || !ch->in_room || !IS_AWAKE(ch))
	return;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
    {
	if (!IS_SET(vch->in_room->room_flags, ROOM_SAFE)
	 && !IS_AFFECTED(vch, AFF_CHARM)
	 && (ch != vch)
	 && !IS_NPC(vch)
	 && !IS_OAFFECTED(vch, AFF_GHOST)
	 && can_see(ch, vch)
	 && !is_safe_druid(vch)
	 && (number_bits(1) != 0)
	 && (!ch->master || IS_PK(ch->master, vch))
	 && (ch->master || (vch->level < (ch->level - 8))))
	{
	    act("$N stampedes $n!", vch, NULL, ch, TO_NOTVICT); 
	    act("$N stampedes you!", vch, NULL, ch, TO_CHAR); 
	    multi_hit(ch, vch, TYPE_UNDEFINED);
	    break;
	}
    }

    return;
}

void event_setambush(void * data)
{
	CHAR_DATA * ch = (CHAR_DATA*)data;
    CHAR_DATA *vch;

    if (is_affected(ch, gsn_setambush))
	add_event((void *) ch, 1, &event_setambush);
    else
	return;

    if (ch->fighting || !ch->in_room || !IS_AWAKE(ch))
	return;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
    {
	if (!IS_SET(vch->in_room->room_flags, ROOM_SAFE)
	 && !IS_AFFECTED(vch, AFF_CHARM)
	 && (ch != vch)
	 && !IS_AFFECTED(vch, AFF_GHOST)
	 && (IS_NPC(vch) || IS_PK(vch, ch))
	 && can_see(ch, vch)
	 && (number_bits(1) != 0)
	 && (get_modifier(ch->affected, gsn_setambush) == vch->id))
	{
	    do_ambush(ch, vch->unique_name);
	    affect_strip(ch, gsn_setambush);
	    break;
	}
    }
}

void event_plague_madness(void *data)
{
	CHAR_DATA * ch = (CHAR_DATA*)data;
    CHAR_DATA *vch;

    if (is_affected(ch, gsn_plague_madness))
	add_event((void *) ch, 1, &event_plague_madness);
    else
	return;

    if (ch->fighting || !ch->in_room || !IS_AWAKE(ch) || (number_bits(7) != 0))
	return;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
    {
	if (!IS_SET(vch->in_room->room_flags, ROOM_SAFE)
	 && !IS_AFFECTED(vch, AFF_CHARM)
	 && (ch != vch)
	 && !IS_OAFFECTED(vch, AFF_GHOST)
	 && (IS_NPC(vch) || IS_PK(vch, ch))
	 && can_see(ch, vch)
	 && (number_bits(1) != 0))
	{
	    send_to_char("Visions of madness overtake you, and you strike out in a mindless rage!\n\r", ch);
	    if (IS_NPC(ch) && !IS_SET(ch->act, PLR_AUTOATTACK))
		SET_BIT(ch->act,PLR_AUTOATTACK);
	    multi_hit(ch, vch, TYPE_UNDEFINED);
	    break;
	}
    }
}

void event_moonray(void * data)
{
	CHAR_DATA * ch = (CHAR_DATA*)data;
    CHAR_DATA *vch;

    if (is_affected(ch, gsn_moonray))
	add_event((void *) ch, 1, &event_moonray);
    else
	return;

    if (ch->fighting || !ch->in_room || !IS_AWAKE(ch) || (number_bits(8) != 0))
	return;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
    {
	if (!IS_SET(vch->in_room->room_flags, ROOM_SAFE)
	 && !IS_AFFECTED(vch, AFF_CHARM)
	 && (ch != vch)
	 && !IS_OAFFECTED(vch, AFF_GHOST)
	 && (IS_NPC(vch) || IS_PK(vch, ch))
	 && can_see(ch, vch)
	 && (number_bits(1) != 0))
	{
	    send_to_char("You are overtaken by madness as the glow of the moon clouds your mind!\r\n", ch);
	    act("$n is overtaken by madness as the glow of the moon shines in $s eyes!", ch, NULL, NULL, TO_ROOM);
	    multi_hit(ch, vch, TYPE_UNDEFINED);
	    break;
	}
    }

    return;
}

void event_demonpos(void * data)
{
	CHAR_DATA * ch = (CHAR_DATA*) data;
    CHAR_DATA *vch;

    if (IS_OAFFECTED(ch, AFF_DEMONPOS))
	add_event((void *) ch, 1, &event_demonpos);
    else
	return;

    if (ch->fighting || !ch->in_room || !IS_AWAKE(ch))
	return;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
    {
	if (!IS_SET(vch->in_room->room_flags, ROOM_SAFE)
	 && !IS_AFFECTED(vch, AFF_CHARM)
	 && (ch != vch)
	 && !IS_OAFFECTED(vch, AFF_GHOST)
	 && (IS_NPC(vch) || IS_PK(vch, ch))
	 && can_see(ch, vch)
	 && ((vch->class_num == global_int_class_spiritscholar)
	  || (vch->class_num == global_int_class_spirittemplar)))
	{
	    act("The demon snatches control of your body at the sight of $N!",ch,NULL,vch,TO_CHAR);
            act("$n snarls at the sight of $N, attacking $M!",ch,NULL,vch,TO_NOTVICT);    
            act("$n snarls, leaping to attack you!", ch, NULL, vch,TO_VICT);                        
            do_murder(ch, vch->unique_name);
	    break;
	}
    }

    return;
}

static AggressionCause should_aggro_npc_specific(CHAR_DATA * npc, CHAR_DATA * victim)
{
    // Basic disqualifiers
    if (IS_SET(npc->act, ACT_WIMPY) || IS_AFFECTED(npc, AFF_CHARM))
        return Aggression_None;

    // Check for tracking
    if (npc->tracking != NULL && !IS_SET(npc->act, ACT_NOTRACK))
    {
        // Will hit the target or a pyrokinetic mirror posing as the target
        if (npc->tracking == victim 
        || (IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_PYROKINETICMIRROR && !strcmp(npc->tracking->name, victim->name)))
            return Aggression_Tracking;
    }

    // Check for battle seraphim
    if (npc->pIndexData->vnum == MOB_VNUM_BATTLESERAPH && npc->memfocus[0] != NULL && npc->memfocus[0]->in_room == npc->in_room && npc->memfocus[0]->fighting == victim)
        return Aggression_Normal;

    // The remaining checks only apply to PC victims and only non-charmed attackers
    if ((IS_NPC(victim) && find_bilocated_body(victim) == NULL) || IS_AFFECTED(npc, AFF_CHARM))
        return Aggression_None;

    // Check for faction enemy
    const Faction * faction(FactionTable::LookupFor(*npc));
    if (faction != NULL && faction->HasFlag(Faction::AggroEnemy) && FactionTable::CurrentStanding(*victim, *npc) == Rating_Enemy
    && (IS_SET(npc->nact, ACT_PSYCHO) || npc->level >= victim->level - 5))
        return Aggression_FactionEnemy;

    // Check for gravebeating to override aggro
    AFFECT_DATA * gravebeat(get_affect(npc, gsn_gravebeat));
    if (gravebeat != NULL)
    {
        // Returning gravebeaters are not aggro unless in their home area
        if (gravebeat->modifier == 0 && (npc->in_room == NULL || npc->in_room->area != npc->pIndexData->area))
            return Aggression_None;

        // Controlled gravebeaters are not aggro to those outside the controller's PK range
        if (gravebeat->modifier != 0 && npc->in_room != NULL)
        {
            CHAR_DATA * controller(get_char_by_id_any_room(gravebeat->modifier, *npc->in_room));
            if (controller != NULL && !IS_PK(controller, victim))
                return Aggression_None;
        }
    }

    // Check for psycho
    if (IS_SET(npc->nact, ACT_PSYCHO))
       return Aggression_Normal;

    // Check for aggro
    if (IS_SET(npc->act, ACT_AGGRESSIVE))
    {
        // Check for undead/revenant
        int levelGap(-5);
        if (IS_SET(npc->act, ACT_UNDEAD))
            levelGap += (get_skill(victim, gsn_revenant) / 8);

        // Check the NPC level against the victim's level, accounting for level gap
        if (npc->level >= victim->level + levelGap)
            return Aggression_Normal;
    }
    return Aggression_None;
}

static AggressionCause should_aggro_pc_specific(CHAR_DATA * pc, CHAR_DATA * victim)
{
    // Basic disqualifiers
    if (!IS_NPC(victim) && !IS_PK(pc, victim))
        return Aggression_None;

    // Check for unclean spirit
    if (IS_OAFFECTED(pc, AFF_UNCLEANSPIRIT) && !IS_AFFECTED(victim, AFF_CHARM)
    && (victim->class_num == global_int_class_spiritscholar || victim->class_num == global_int_class_spirittemplar)
    && number_bits(1) == 0)
        return Aggression_UncleanSpirit;

   return Aggression_None;
}

AggressionCause should_aggro(CHAR_DATA * ch, CHAR_DATA * victim)
{
    // Basic disqualifiers which apply in all cases
    if (ch == NULL || victim == NULL || !ch->valid || !victim->valid
    || IS_OAFFECTED(ch, AFF_GHOST) || IS_OAFFECTED(victim, AFF_GHOST)
    || ch->in_room == NULL || ch->in_room != victim->in_room 
    || ch == victim || ch->fighting != NULL
    || IS_IMMORTAL(victim) || IS_SET(ch->in_room->room_flags, ROOM_SAFE)
    || IS_AFFECTED(ch, AFF_CALM) || !IS_AWAKE(ch) || IS_NAFFECTED(ch, AFF_SUBDUE)
    || ch->position == POS_DEAD || victim->position == POS_DEAD)
        return Aggression_None;

    // Partition by pc or npc
    AggressionCause result;
    if (IS_NPC(ch)) result = should_aggro_npc_specific(ch, victim);
    else result = should_aggro_pc_specific(ch, victim);

    // Now check for slower things; do this last to avoid it in most cases
    // Generally we want to avoid such microoptimizations, but this one actually matters
    if (result != Aggression_None)
    {
        if (Luck::Check(*victim) == Luck::Lucky || !can_see(ch, victim))
           return Aggression_None;
    }

    return result;
}

void aggr_update()
{
    // Iterate all characters in the world
    CHAR_DATA * ch_next;
    for (CHAR_DATA * ch(char_list); ch != NULL; ch = ch_next)
    {
        ch_next = ch->next;
        if (ch->in_room == NULL || IS_SET(ch->act, PLR_FREEZE))
            continue;

        // Iterate all characters in the same room, building a list of aggression targets
        std::vector<std::pair<CHAR_DATA *, AggressionCause> > targets;
        for (CHAR_DATA * room_char(ch->in_room->people); room_char != NULL; room_char = room_char->next_in_room)
        {
            // Add in the target and the cause of aggression
            AggressionCause cause(should_aggro(ch, room_char));
            if (cause != Aggression_None)
                targets.push_back(std::make_pair(room_char, cause));
        }

        // If there were no targets, move on
        if (targets.empty())
            continue;

        // Choose a random target to hit
        unsigned int index(number_range(0, targets.size() - 1));
        CHAR_DATA * target(targets[index].first);
        switch (targets[index].second)
        {
            // Echo according to the cause
            case Aggression_UncleanSpirit:
                act("The unclean spirit inside snatches control of your body at the sight of $N!", ch, NULL, target, TO_CHAR);
                act("$n snarls at the sight of $N, attacking $M!", ch, NULL, target, TO_NOTVICT);
                act("$n snarls, leaping to attack you!", ch, NULL, target, TO_VICT);
                break;

            case Aggression_FactionEnemy:
                act("$n sees you and attacks!", ch, NULL, target, TO_VICT);
                act("$n sees $N and attacks!", ch, NULL, target, TO_NOTVICT);
                if (race_table[ch->race].pc_race)
                {
                    const Faction * faction(FactionTable::LookupFor(*ch));
                    if (faction != NULL)
                    {
                        std::ostringstream mess;
                        mess << "The enemies of " << faction->Name() << " will suffer!";
                        do_yell(ch, const_cast<char*>(mess.str().c_str()));
                    }
                }
                break;

            case Aggression_Tracking:
                act("$n sees you and comes at you in fury!", ch, NULL, target, TO_VICT);
                act("$n sees $N and charges in fury!", ch, NULL, target, TO_NOTVICT);
                break;

            default:
                act("$n sees you and attacks!", ch, NULL, target, TO_VICT);
                act("$n sees $N and attacks!", ch, NULL, target, TO_NOTVICT);
                break;
        }

        // Now actually start the fight
        if (IS_NPC(ch) && IS_SET(ch->nact, ACT_CLASSED))
            mob_class_aggr(ch, target);
        else
            multi_hit(ch, target, TYPE_UNDEFINED);
    }
}

/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */

void update_handler( void )
{
    static  int     pulse_area(0);
    static  int     pulse_mobile(3);
    static  int     pulse_violence(6);
    static  int     pulse_point(9);
    static  int     pulse_oaths(10);
    static  int     pulse_disc(0);
    static  int     pulse_lodestone(0);
    static time_t   time_weave(current_time);

    if (--pulse_oaths <= 0)
    {
        pulse_oaths = PULSE_OATHS;
        Oaths::UpdateOaths();
    }

    if (--pulse_area <= 0)
    {
        pulse_area	= PULSE_AREA;
        area_update();
    }

    if (--pulse_disc <= 0)
    {
        pulse_disc = PULSE_DISC;
        disc_update();
    }

    if (--pulse_lodestone <= 0)
    {
        pulse_lodestone = PULSE_LODESTONE;
        lodestone_update();
    }

    if ( --pulse_mobile   <= 0 )
    {
        pulse_mobile	= PULSE_MOBILE;

        if (global_song_message)
            global_song_message = FALSE;
        else
            global_song_message = TRUE;

        mobile_update	( );
    }

    if ( --pulse_violence <= 0 )
    {
	pulse_violence	= PULSE_VIOLENCE;
	violence_update	( );
    }

    if ( --pulse_point    <= 0 )
    {
	wiznet("TICK!",NULL,NULL,WIZ_TICKS,0,0);
	pulse_point     = PULSE_TICK;
	time_update	( );

	if (!time_info.half)
	    weather_update  ( );

	char_update	( );
	obj_update	( );
    stone_update	( );
	track_update	( );
	area_affect_update ( );
	room_update	( );
	rumor_update	( );
    }

    if (pulse_point % 10 == 0)
	pillage_update ( );

    // Weave generation
    if (current_time - time_weave >= NUM_SECONDS_HOUR)
    {
        Weave::Regenerate();
        ShadeControl::Update();
        time_weave = current_time;
    }

        /*  
         * Equipment crumbling.
         * Ten-second time frame to ensure it activates.
         */
        if ((current_time - 39600) % NUM_SECONDS_DAY >= 0
         && (current_time - 39600) % NUM_SECONDS_DAY < 10)
        {   
	    DESCRIPTOR_DATA *td;

            if (!crumble_done)
            {   
	        for (td = descriptor_list; td; td = td->next)
		    if (td->connected == CON_PLAYING)
		        write_to_descriptor(td->descriptor, "Performing system maintenance.  Please wait.\n\r", 0); 

		crumble_done = TRUE;
		log_string("Performing nightly maintenance...");
		pfile_maint(FALSE);
        rumor_maint();
		log_string("Nightly maintenance complete.");
            }

        }   
        else
	{
            crumble_done = FALSE;
	    rip_done = FALSE;
	}

    if (crumble_test)
    {    
        crumble_test = FALSE;
	    pfile_maint(FALSE);
    }   

    aggr_update( );
    process_events();
}

void rip_proc(void)
{
    rip_process = TRUE;
    log_string("Doing limit/level update.");
    get_player_levels();
    log_string ("Got player levels");
    calc_limit_values();
    log_string ("Calced values");
    rip_obj_pfiles();
    log_string ("ripped pfiles");
    rip_process = FALSE;
}

void send_to_area(const char * message, AREA_DATA & area)
{
    for (DESCRIPTOR_DATA * d(descriptor_list); d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING && d->character->in_room != NULL && d->character->in_room->area == &area)
            send_to_char(message, d->character);
    }
}

void area_affect_update ( void )
{
    AFFECT_DATA *paf,*paf_next;
    AREA_DATA *pArea;
    
    for (pArea = area_first; pArea != NULL; pArea = pArea->next)
    {
        for (paf = pArea->affected; paf != NULL; paf = paf_next)
        {
	        paf_next = paf->next;

            if (paf->duration == 0)
            {
                // Gralcian funnel
                if (paf->type == gsn_gralcianfunnel)
                {
                    CHAR_DATA * recipient(get_char_by_id(paf->modifier));
                    if (recipient != NULL)
                        send_to_char("The winds cease their whisperings to you.\n", recipient);
                }

                // Echo-only effects
                else if (paf->type == gsn_mirage) 
                    send_to_area("The shimmering mirage fades away.\n", *pArea);
                else if (paf->type == gsn_electricalstorm) 
                    send_to_area("With a final crackle of power, the raging electrical storm dies down.\n", *pArea);
                else if (paf->type == gsn_rainoffire)
                    send_to_area("The sky loses its blood-red hue as the rain of fire ceases.\n", *pArea);
                else if (paf->type == gsn_holyground)
                    send_to_area("The faint golden light filling this place dies away.\n", *pArea);
                else if (paf->type == gsn_brume)
                    send_to_area("The heavy fog obscuring this place lifts.\n", *pArea);
                else if (paf->type == gsn_barrowmist)
                    send_to_area("The reeking mists slowly dissipate.\n", *pArea);
                else if (paf->type == gsn_icestorm)
                    send_to_area("The fury of the ice storm subsides.\n", *pArea);
                else if (paf->type == gsn_duskfall)
                    send_to_area("The preternatural darkness filling this place slowly abates.\n", *pArea);
                else if (paf->type == gsn_masquerade)
                    send_to_area("The terrible illusion blanketing this place unravels at all.\n", *pArea);

                affect_remove_area(pArea,paf);
                continue;
            }

    	    if (paf->duration > 0)
   	        {
    	    	paf->duration--;
        		if(number_range(0,4) == 0 && paf->level > 0)
		            paf->level--;

                continue;
    	    }

	        if (paf->duration < 0)
		    {
                continue;
            }
	    }
    }
}

void rumor_update( void )
{
    RUMORS *nr;
    for (nr = rumor_list;nr;nr=nr->next)
	if (IS_VALID(nr))
	    if (nr->sticky)
		continue;
	    else if (current_time > nr->stamp + NUM_SECONDS_DAY*NUM_DAYS_YEAR)
		INVALIDATE(nr);
}

