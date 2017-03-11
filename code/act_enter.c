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
#include <time.h>
#else
#include <sys/types.h>
#endif
#if defined(unix)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"

/* command procedures needed */
DECLARE_DO_FUN(do_look		);
DECLARE_DO_FUN(do_stand		);

/* random room generation procedure */
ROOM_INDEX_DATA  *get_random_room(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *room;

    for ( ; ; )
    {
        room = get_room_index( number_range( 0, 65535 ) );
        if ( room != NULL )
        if ( can_see_room(ch,room)
	&&   !room_is_private(room)
        &&   !IS_SET(room->room_flags, ROOM_PRIVATE)
        &&   !IS_SET(room->room_flags, ROOM_SOLITARY) 
	&&   !IS_SET(room->room_flags, ROOM_SAFE)
	&&   !IS_SET(room->room_flags, ROOM_NO_RECALL)
	&&   !IS_SET(room->room_flags, ROOM_NOGATE)
        &&   !IS_SET(room->room_flags, ROOM_NEWBIES_ONLY)
	&&   !IS_SET(room->room_flags, ROOM_NOMAGIC))
            break;
    }

    return room;
}

ROOM_INDEX_DATA  *get_random_room_area(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *room;
    int max_rooms = 0,tar_vnum = 0;
    VNUM_RANGE *deeznums = NULL;
    for (deeznums=ch->in_room->area->vnums;deeznums;deeznums=deeznums->next)
    {
	max_rooms += deeznums->max_vnum - deeznums->min_vnum + 1;
    }
    
    for (int i=0;i<100;i++ )
    {
	tar_vnum = number_range(0,max_rooms-1)+ch->in_room->area->vnums->min_vnum;
	if (tar_vnum > ch->in_room->area->vnums->max_vnum)
	    for(deeznums=ch->in_room->area->vnums;deeznums;deeznums=deeznums->next)
	    {
		tar_vnum -= deeznums->max_vnum;
		if (deeznums->next)
		{
		    tar_vnum += deeznums->next->min_vnum;
		    if (tar_vnum < deeznums->next->max_vnum)
			break;
		}
		else
		{
		    tar_vnum = -1;
		    break;
		}
	    }
	if (tar_vnum == -1)
	    continue;   
	room = get_room_index( tar_vnum );
        if ( room != NULL )
            if ( can_see_room(ch,room)
	    &&   !room_is_private(room)
            &&   !IS_SET(room->room_flags, ROOM_PRIVATE)
            &&   !IS_SET(room->room_flags, ROOM_SOLITARY) 
	    &&   !IS_SET(room->room_flags, ROOM_SAFE)
	    &&   !IS_SET(room->room_flags, ROOM_NO_RECALL)
  	    &&   (IS_SET(room->room_flags, ROOM_NOGATE) ? 
		   (ch->in_room->clan == ch->clan) : TRUE )
            &&   !IS_SET(room->room_flags, ROOM_NEWBIES_ONLY)
	    &&   !IS_SET(room->room_flags, ROOM_NOMAGIC))
                break;
	room = NULL;
    }

    return room;
}

ROOM_INDEX_DATA *get_random_room_no_char(void)
{
    ROOM_INDEX_DATA *room;

    for ( ; ; )
    {
        room = get_room_index( number_range( 0, 65535 ) );
        if ( room != NULL )
	if (   !room_is_private(room)
        &&   !IS_SET(room->room_flags, ROOM_PRIVATE)
        &&   !IS_SET(room->room_flags, ROOM_SOLITARY) 
	&&   !IS_SET(room->room_flags, ROOM_SAFE)
	&&   !IS_SET(room->room_flags, ROOM_NO_RECALL)
	&&   !IS_SET(room->room_flags, ROOM_NOGATE)
	&&   !IS_SET(room->room_flags, ROOM_GODS_ONLY)
	&&   !IS_SET(room->room_flags, ROOM_NEWBIES_ONLY)
	&&   !IS_SET(room->room_flags, ROOM_NOMAGIC))
            break;
    }

    return room;
}

/* RT Enter portals */
void do_enter( CHAR_DATA *ch, char *argument)
{    
    ROOM_INDEX_DATA *location; 

    if ( ch->fighting != NULL ) 
	return;

    /* nifty portal stuff */
    if (argument[0] != '\0')
    {
        ROOM_INDEX_DATA *old_room;
	OBJ_DATA *portal;
	CHAR_DATA *fch, *fch_next;

        old_room = ch->in_room;

	portal = get_obj_list( ch, argument,  ch->in_room->contents );
	
	if (portal == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}

	if (portal->item_type != ITEM_PORTAL 
        ||  (IS_SET(portal->value[1],EX_CLOSED) && !IS_TRUSTED(ch,ANGEL)))
	{
	    send_to_char("You can't seem to find a way in.\n\r",ch);
	    return;
	}

	if (IS_SET(portal->value[2],GATE_RANDOM) || portal->value[3] == -1)
	{
	    location = get_random_room(ch);
	    portal->value[3] = location->vnum; /* for record keeping :) */
	}
	else if (IS_SET(portal->value[2],GATE_BUGGY) && (number_percent() < 5))
	    location = get_random_room(ch);
	else
	    location = get_room_index(portal->value[3]);

	if (location == NULL
	||  location == old_room
	||  !can_see_room(ch,location) 
	||  (room_is_private(location) && !IS_TRUSTED(ch,IMPLEMENTOR)))
	{
	   act("$p doesn't seem to go anywhere.",ch,portal,NULL,TO_CHAR);
	   return;
	}

        if (IS_NPC(ch) && IS_SET(location->room_flags,ROOM_LAW)
	 && (IS_SET(ch->act,ACT_AGGRESSIVE) || IS_SET(ch->nact, ACT_PSYCHO)))
        {
            send_to_char("Something prevents you from leaving...\n\r",ch);
            return;
        }

	if (is_affected(ch, gsn_matrix))
	{
	send_to_char("The power of the matrix prevents you from entering the gate.\n\r", ch);
	return;
	}

	act("$n steps into $p.",ch,portal,NULL,TO_ROOM);
	
	act("You enter $p.",ch,portal,NULL,TO_CHAR);

	char_from_room(ch);
	char_to_room(ch, location);

	if (IS_SET(portal->value[2],GATE_GOWITH)) /* take the gate along */
	{
	    obj_from_room(portal);
	    obj_to_room(portal,location);
	}

	if (IS_SET(portal->value[2],GATE_NORMAL_EXIT) 
	  && !IS_IMMORTAL(ch) && (!IS_NPC(ch) || !IS_AFFECTED(ch, AFF_WIZI)))
	    act("$n has arrived.",ch,portal,NULL,TO_ROOM);
	else
	    act("$n has arrived through $p.",ch,portal,NULL,TO_ROOM);

	do_look(ch,"auto");
	if (portal->pIndexData->vnum == OBJ_VNUM_PORTAL_NEXUS && !IS_IMMORTAL(ch) && (!IS_NPC(ch) || !IS_AFFECTED(ch, AFF_WIZI)))
		{
		damage_old(ch, ch, number_range(ch->level/4, ch->level), gsn_gate, DAM_NEGATIVE, TRUE);
		WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
		}

	/* charges */
	if (portal->value[0] > 0)
	{
	    portal->value[0]--;
	    if (portal->value[0] == 0)
		portal->value[0] = -1;
	}

	/* protect against circular follows */
	if (old_room == location)
	    return;

    	for ( fch = old_room->people; fch != NULL; fch = fch_next )
    	{
            fch_next = fch->next_in_room;

            if (portal == NULL || portal->value[0] == -1) 
	    /* no following through dead portals */
                continue;
 
            if ( fch->master == ch && IS_AFFECTED(fch,AFF_CHARM)
            &&   fch->position < POS_STANDING)
            	do_stand(fch,"");

            if ( fch->master == ch && fch->position == POS_STANDING)
            {
 
                if (IS_SET(ch->in_room->room_flags,ROOM_LAW)
                &&  (IS_NPC(fch) && (IS_SET(fch->act,ACT_AGGRESSIVE) || IS_SET(fch->nact, ACT_PSYCHO))))
                {
                    act("You can't bring $N into the city.",
                    	ch,NULL,fch,TO_CHAR);
                    act("You aren't allowed in the city.",
                    	fch,NULL,NULL,TO_CHAR);
                    continue;
            	}
 
            	act( "You follow $N.", fch, NULL, ch, TO_CHAR );
		do_enter(fch,argument);
            }
    	}

 	if (portal != NULL && portal->value[0] == -1)
	{
	    act("$p fades out of existence.",ch,portal,NULL,TO_CHAR);
	    if (ch->in_room == old_room)
		act("$p fades out of existence.",ch,portal,NULL,TO_ROOM);
	    else if (old_room->people != NULL)
	    {
		act("$p fades out of existence.", 
		    old_room->people,portal,NULL,TO_CHAR);
		act("$p fades out of existence.",
		    old_room->people,portal,NULL,TO_ROOM);
	    }
	    extract_obj(portal);
	}
	return;
    }

    send_to_char("Nope, can't do it.\n\r",ch);
    return;
}
