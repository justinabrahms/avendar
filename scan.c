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
#include "merc.h"
#include "skills_chirurgeon.h"
#include "lookup.h"
#include "tables.h"
/* char *const distance[4]=
{
"right here.", "nearby to the %s.", "not far %s.", "off in the distance %s."
}; */

char *const distance[4]=
{
"=== 1 %s ===", "=== 2 %s ===", "=== 3 %s ===", "=== 4 %s ==="
};

void scan_list           args((ROOM_INDEX_DATA *scan_room, CHAR_DATA *ch,
                               int depth, int door));
void scan_char           args((CHAR_DATA *victim, CHAR_DATA *ch,
                               int depth, int door));
void do_scan(CHAR_DATA *ch, char *argument)
{
   extern char *const dir_name[];
   char arg1[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
   ROOM_INDEX_DATA *scan_room;
   EXIT_DATA *pExit;
   int door, depth;
   int maxdepth = 3;

   argument = one_argument(argument, arg1);

   if (!str_cmp(arg1, "n") || !str_cmp(arg1, "north")) door = 0;
   else if (!str_cmp(arg1, "e") || !str_cmp(arg1, "east"))  door = 1;
   else if (!str_cmp(arg1, "s") || !str_cmp(arg1, "south")) door = 2;
   else if (!str_cmp(arg1, "w") || !str_cmp(arg1, "west"))  door = 3;
   else if (!str_cmp(arg1, "u") || !str_cmp(arg1, "up" ))   door = 4;
   else if (!str_cmp(arg1, "d") || !str_cmp(arg1, "down"))  door = 5;
   else { send_to_char("Which way do you want to scan?\n\r", ch); return; }

   if (ch->invis_level < 52)
	{
   act("You peer intently $T.", ch, NULL, dir_name[door], TO_CHAR);
   act("$n peers intently $T.", ch, NULL, dir_name[door], TO_ROOM);
	}

	if (!ch->in_room)
		return;
	if (area_is_affected(ch->in_room->area, gsn_icestorm))
		{
		send_to_char("The swirling ice and snow prevents you from seeing any reasonable distance.\n\r", ch);
		return;
		}
	if ((room_is_affected(ch->in_room, gsn_smoke) || room_is_affected(ch->in_room, gsn_smokescreen)) 
    && (!(ch->race == global_int_race_shuddeni || is_affected(ch,gsn_sensory_vision))))
    {
		send_to_char("The smoke prevents you from seeing any reasonable distance.\n\r", ch);
		return;
	}

    if (room_is_affected(ch->in_room, gsn_steam) && ch->race != global_int_race_shuddeni && !is_affected(ch, gsn_sensory_vision))
    {
        send_to_char("The thick steam prevents you from seeing any reasonable distance.\n", ch);
        return;
    }

    if (area_is_affected(ch->in_room->area, gsn_brume) && number_percent() > get_skill(ch, gsn_stormmastery))
    {
        send_to_char("The thick fog prevents you from seeing any reasonable distance.\n", ch);
        return;
    }

   sprintf(buf, "Looking %s you see:\n\r", dir_name[door]);

   scan_room = ch->in_room;

    if ((ch->in_room->area->w_cur.storm_str > 0) && IS_OUTSIDE(ch))
    	if (ch->in_room->area->w_cur.precip_type == 0)
        {
	    maxdepth -= (ch->in_room->area->w_cur.storm_str / 8);
	    if (maxdepth <= 0)
	    {
		send_to_char("The blowing snow prevents you from seeing any reasonable distance.\n\r", ch);
		return;
	    }
	}
	else if (ch->in_room->area->w_cur.precip_type == 2)
	{
	    maxdepth -= (ch->in_room->area->w_cur.storm_str / 25);
	    if (maxdepth <= 0)
	    {
		send_to_char("The pouring rain prevents you from seeing any reasonable distance.", ch);
		return;
	    }
	}
	
    if (is_affected(ch, gsn_lunarinfluence) 
      && ((time_info.phase_rhos == MOON_WAXING_GIBBOUS)
        || (time_info.phase_rhos == MOON_WAXING_HALF)
        || (time_info.phase_rhos == MOON_WAXING_CRESCENT)))
	maxdepth++;

    if (!IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_EAGLE_EYED)) maxdepth += 2;
    if (is_affected(ch, gsn_eagleeyes)) maxdepth += 3;
    if (ch->race == global_int_race_alatharya) maxdepth += 1;
    if (number_percent() <= get_skill(ch,gsn_farsight))
    {
	    maxdepth += 1;
    	check_improve(ch,NULL,gsn_farsight,2,TRUE);
    }

    // Intentionally no chance of improvement here due to high spamming possibilities
    if (is_flying(ch) && number_percent() <= get_skill(ch, gsn_controlledflight))
        ++maxdepth;

    if (is_affected(ch,gsn_eyesoftheforest))
	if (ch->in_room->sector_type == SECT_CITY)
	    maxdepth -= 1;
	else if (ch->in_room->sector_type == SECT_FOREST)
	    maxdepth += 1;	

    if (is_affected(ch,gsn_moonsight))
	if (time_info.hour >= season_table[time_info.season].sun_down
	  || time_info.hour < season_table[time_info.season].sun_up)
	    maxdepth += 1;
	else
	    maxdepth -= 1;

    for (depth = 1; depth < (maxdepth + 1); depth++)
    {
	if ((pExit = scan_room->exit[door]) != NULL)
	{
	    if (pExit->exit_info & EX_CLOSED || room_is_affected(pExit->u1.to_room, gsn_smoke)
	      || pExit->exit_info & EX_WALLED 
          || pExit->exit_info & EX_WALLOFFIRE 
	      || pExit->exit_info & EX_SECRET || depth > ((ch->level+9)/8))
	    {
		return;
	    }
	    else
	    {
         	buf[0] = '\0';
         	sprintf(buf,"=== %d %s ===", depth ,dir_name[door]);
         	strcat(buf,"\n\r");
         	send_to_char(buf,ch);
         	scan_room = pExit->u1.to_room;

		if (!can_see_in_room(ch, scan_room))
		    send_to_char("It's too dark to see clearly.\n\r", ch);
		if (room_is_affected(scan_room, gsn_smokescreen) && (!(ch->race == global_int_race_shuddeni || is_affected(ch,gsn_sensory_vision))))
	 	{
		    send_to_char("All you see is smoke.\n\r", ch);
		    return;
	 	}

        if (room_is_affected(scan_room, gsn_steam) && ch->race != global_int_race_shuddeni && !is_affected(ch, gsn_sensory_vision))
        {
            send_to_char("All you see is thick, swirling steam.\n", ch);
            return;
        }

        if (area_is_affected(scan_room->area, gsn_brume) && number_percent() > get_skill(ch, gsn_stormmastery))
        {
            send_to_char("All you see is a dense, heavy fog.\n", ch);
            return;
        }
	
		scan_list(pExit->u1.to_room, ch, depth, door);
		send_to_char("\n\r",ch);
	    }
	}
    }
    return;
}

void scan_list(ROOM_INDEX_DATA *scan_room, CHAR_DATA *ch, int depth,
               int door)
{
    CHAR_DATA *rch;

    if (scan_room == NULL) return;
    for (rch=scan_room->people; rch != NULL; rch=rch->next_in_room)
    {
	if (rch == ch) continue;
	if (!IS_NPC(rch) && rch->invis_level > get_trust(ch) ) continue;
	if (IS_NPC(rch) && IS_SET(rch->affected_by, AFF_WIZI) && !IS_SET(ch->act, PLR_HOLYLIGHT))
	    continue;
	if (is_affected(rch, gsn_camoublind) && !IS_PAFFECTED(ch, AFF_SHARP_VISION))
	    continue;
	if (is_affected(rch, gsn_shadow_ward) && ch->clan != clan_lookup("SHUNNED"))
	    continue;
	if (can_see(ch, rch)) scan_char(rch, ch, depth, door);
   }
   return;
}

void scan_char(CHAR_DATA *victim, CHAR_DATA *ch, int depth, int door)
{
/*   extern char *const dir_name[];
   extern char *const distance[];
   char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

   buf[0] = '\0';

   strcat(buf, PERS(victim, ch));
   strcat(buf, ", ");
   sprintf(buf2, distance[depth], dir_name[door]);
   strcat(buf, buf2); 
   strcat(buf, "\n\r");
 
   send_to_char(buf, ch); */

show_char_to_char_0(victim, ch);
   return;
}
