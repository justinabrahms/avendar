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
#include <limits.h>
#include <math.h>
#include "merc.h"
#include "recycle.h"
#include "lookup.h"
#include "olc.h"
#include "tables.h"
#include "languages.h"
#include "alchemy.h"
#include "PyreInfo.h"
#include "spells_spirit.h"
#include "spells_spirit_earth.h"
#include "spells_air_earth.h"
#include "spells_water_earth.h"
#include "spells_void.h"
#include "Luck.h"
#include "NameMaps.h"
#include "Encumbrance.h"

/* command procedures needed */
DECLARE_DO_FUN(do_split		);
DECLARE_DO_FUN(do_yell		);
DECLARE_DO_FUN(do_autoyell	);
DECLARE_DO_FUN(do_say		);
DECLARE_DO_FUN(do_wake		);
DECLARE_DO_FUN(do_hide		);
DECLARE_DO_FUN(do_tell		);
DECLARE_DO_FUN(do_withdraw	);
DECLARE_DO_FUN(do_look		);

/*
 * Local functions.
 */
#define CD CHAR_DATA
#define OD OBJ_DATA
bool	remove_obj_slot	args( (CHAR_DATA *ch, int slot, bool fReplace) );
CD *	find_keeper	args( (CHAR_DATA *ch ) );
long	base_cost	args( (OBJ_DATA *obj ) );
long	get_cost	args( (CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy ) );
void 	obj_to_keeper	args( (OBJ_DATA *obj, CHAR_DATA *ch ) );
OD *	get_obj_keeper	args( (CHAR_DATA *ch,CHAR_DATA *keeper,char *argument));
void    do_drop		args( ( CHAR_DATA *ch, char *argument ));
int     hands_free      args( (CHAR_DATA *ch) );
void	desiccate_corpse args( (OBJ_DATA *corpse) );
int 	letsmakeadeal(CHAR_DATA *ch, CHAR_DATA *keeper, int cost, bool buy);

extern bool spell_protection_good args((int sn,int level,CHAR_DATA *ch,void *vo,int target));
extern bool check_social args( ( CHAR_DATA *ch, char *command,
			    char *argument ) );
extern int get_moon_state args( ( int moon_num, bool get_size ) );
extern void levelize_mobile args((CHAR_DATA *mob, int level));
extern void check_heirloom_level(CHAR_DATA *ch, OBJ_DATA *obj);
extern void do_pray(CHAR_DATA *ch, char *argument);
extern void do_bug( CHAR_DATA *ch, char *argument );
extern bool unassume_form(CHAR_DATA *ch);
extern  bool    damage_from_obj args( ( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj,			                                        		int dam, int dt, int dam_type, bool show) );
extern int elemental_lookup args((char * argument));
extern int make_bin args((int prebin));
extern const struct wear_type wear_table[];

int	item_svalue[6] = { 1, 3, 6, 12, 36, 72 };

#undef OD
#undef	CD

/* RT part of the corpse looting code */
/* MW hahaha. */

bool can_loot(CHAR_DATA *ch, OBJ_DATA *obj)
{
    if (ch == NULL || obj == NULL) return FALSE;
    if (!obj->owner) return TRUE;
    if (!IS_NPC(ch) && (ch->level > 10 || !strcmp(ch->name, obj->owner))) return TRUE;
	return FALSE;
/* Loot like mad bitches, unless you're a newbie */
}

bool check_aura(CHAR_DATA *ch, OBJ_DATA *obj, bool damage)
{
    if (obj_is_affected(obj, gsn_aura) && IS_EVIL(ch))
    {   
        act( "You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR );
        act( "$n is zapped by $p and drops it.",  ch, obj, NULL, TO_ROOM );
        obj_from_char( obj );
        obj_to_room( obj, ch->in_room );

	if (damage)
            damage_old(ch,ch,number_range(ch->level,2*ch->level),gsn_aura,DAM_HOLY,TRUE);

        return TRUE;
    }
    else
	return FALSE;
}

int get_cur_capacity(CHAR_DATA *ch)
{
    int c = 0;
    OBJ_DATA *obj;

    if (obj = ch->carrying)
        for (; obj; obj = obj->next_content)
	    if (!obj->worn_on)
	        c += item_svalue[obj->size];

    return c;
}

int get_max_capacity(CHAR_DATA *ch)
{
      return INT_MAX;
//    return 45 + (ch->size * 5);
}

void get_obj( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container )
{
    /* variables for AUTOSPLIT */
    bool cvalid = TRUE;
    CHAR_DATA *pMob;
    CHAR_DATA *gch;
    int members;
    char buffer[100];
    int i;
    bool ghostly;

    if (ch == NULL || obj == NULL)
	return;

    if ( !CAN_WEAR(obj, ITEM_TAKE) )
    {
	send_to_char( "You can't take that.\n\r", ch );
	return;
    }

    if ( IS_PAFFECTED(ch, AFF_VOIDWALK) || is_affected(ch, gsn_astralprojection))
    {
	act("Your immaterial hands pass through $d.", ch, NULL, obj->short_descr, TO_CHAR);
	return;
    }

    if ( is_affected(ch, gsn_frostbite) && number_bits(1) == 0)
    {
	act("Your hands shiver and you can't quite pick up $p.", ch, obj, NULL, TO_CHAR);
	return;
    }

    if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
     && (get_cur_capacity(ch) + item_svalue[obj->size]) > get_max_capacity(ch))
    {
	send_to_char("You lack the room to carry that.\n\r", ch);
	return;
    }

    if (ch->carry_number + 1 > static_cast<int>(Encumbrance::CarryCountCapacity(*ch)))
    {
	    act( "$p: you can't carry that many items.", ch, obj, NULL, TO_CHAR );
	    return;
    }

    if ((!obj->in_obj || obj->in_obj->carried_by != ch)
    &&  (get_carry_weight(*ch) + get_obj_weight(obj) > static_cast<int>(Encumbrance::CarryWeightCapacity(*ch))))
    {
	act( "$p: you can't carry that much weight.", ch, obj, NULL, TO_CHAR );
	return;
    }

    if (!can_loot(ch,obj))
    {
	act("Corpse looting is not permitted.",ch,NULL,NULL,TO_CHAR );
	return;
    }

    if (obj_is_affected(obj, gsn_dispatchlodestone))
    {
        act("$p bobs in the air, avoiding your grasp.", ch, obj, NULL, TO_CHAR);
        return;
    }

    if (obj->in_room != NULL)
    {
	for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
	    if (gch->on == obj)
	    {
		act("$N appears to be using $p.",
		    ch,obj,gch,TO_CHAR);
		return;
	    }
    }

    if (IS_NPC(ch) && IS_SET(ch->act, ACT_ILLUSION))
    {
        if (ch->master != NULL)
            act("$N tries to get $p, but cannot since $E isn't real.", ch->master, obj, ch, TO_CHAR);

        return;
    }

    if (!IS_NPC(ch) && IS_OAFFECTED(ch, AFF_GHOST))
    {
	ghostly = TRUE;
	
	if (ch->perm_stat[STAT_CON] >= 4)
	{
		for (i = 0; i < MAX_LASTOWNER; i++)
		    if ((obj->lastowner[i][0] != '\0') && !str_cmp(ch->name, obj->lastowner[i]))
		    {
			ghostly = FALSE;
			break;
		    }
	}

	if (ghostly)
	{
	    act("Your ghostly hands pass through $p.", ch, obj, NULL, TO_CHAR);
	    return;
        }
    }

    if ( container != NULL )
    {
        if (!IS_AFFECTED(ch, AFF_SNEAK) && !is_affected(ch, gsn_lessertentaclegrowth))
            act( "$n gets $p from $P.", ch, obj, container, TO_ROOM );
        act( "You get $p from $P.", ch, obj, container, TO_CHAR );
        REMOVE_BIT(obj->extra_flags[0],ITEM_HAD_TIMER);
        if (ch && obj && obj->in_obj && obj->in_obj->owner && !strcmp(ch->name, obj->in_obj->owner))
          cvalid = FALSE;
        obj_from_obj( obj );
    }
    else
    {
        if (!IS_AFFECTED(ch, AFF_SNEAK))
            act( "$n gets $p.", ch, obj, container, TO_ROOM );
        
        act( "You get $p.", ch, obj, container, TO_CHAR );
        obj_from_room( obj );
        if (obj_is_affected(obj, gsn_runeoffire))
        {
            act("With a blast of flame, a rune of fire on $p explodes at $n!", ch, obj, NULL, TO_ROOM);
            act("With a blast of flame, a rune of fire on $p explodes at you!", ch, obj, NULL, TO_CHAR);
            damage(ch,ch,number_range(ch->level, ch->level*4),gsn_runeoffire,DAM_FIRE,TRUE);
            object_affect_strip(obj, gsn_runeoffire);
        }

        REMOVE_BIT(obj->extra_flags[0],ITEM_HIDDEN);
        REMOVE_BIT(obj->extra_flags[0],ITEM_STASHED);
    }

    if ( obj->item_type == ITEM_MONEY)
    {
	coins_to_char(ch, obj->value[0], obj->value[1]);
        if (IS_SET(ch->act,PLR_AUTOSPLIT))
        { /* AUTOSPLIT code */
    	  members = 0;
    	  for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    	  {
            if (!IS_AFFECTED(gch,AFF_CHARM) && !IS_NPC(gch) && is_same_group( gch, ch ) )
              members++;
    	  }

	  if ( members > 1 && (obj->value[0] > 1 || obj->value[1]))
	  {
	    sprintf(buffer,"%d %s",obj->value[0],coin_table[obj->value[1]].name);
	    do_split(ch,buffer);	
	  }
        }
 
    	oprog_take_trigger( obj, ch, obj );
	for (pMob = ch->in_room->people ; pMob != NULL; pMob = pMob->next_in_room)
	{
    	mprog_take_trigger( pMob, ch, obj );
	}
	extract_obj( obj );
    }
    else
    {
	obj_to_char( obj, ch );
/*	if (!IS_NPC(ch) && cvalid)
	{
	  ch->objlevels += obj->level;
	  if (ch->objlevels > ch->level * 10 && ch->objlevels > 100
	  && obj->level > 1)
	  {
	    sprintf(buf,"%s took %s [level %d] in room %d. Total levels: %ld.",
		  ch->name, obj->short_descr, obj->level, ch->in_room ? ch->in_room->vnum : -1, ch->objlevels);
	    log_string(buf);
	    sprintf(buf, "Overlimit: %s took %s [level %d] in room %d. Total levels: %ld.", ch->name, obj->short_descr, obj->level, ch->in_room ? ch->in_room->vnum : -1, ch->objlevels);
	    ch->objlevels -= 100;
    	    wiznet(buf,NULL,NULL,WIZ_CHEATING,0,0);
	  }
	}*/

    	oprog_take_trigger( obj, ch, obj );
	for (pMob = ch->in_room->people ; pMob != NULL; pMob = pMob->next_in_room)
	{
    	mprog_take_trigger( pMob, ch, obj );
	}
    }

    check_aura(ch, obj, TRUE);

    return;
}



void do_get( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    OBJ_DATA *container;
    bool found;
    int num_canloot; //For working with 'get all corpse'.
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (!str_cmp(arg2,"from"))
	return;

    /* Get type. */
    if ( arg1[0] == '\0' )
    {
	send_to_char( "Get what?\n\r", ch );
	return;
    }

    if ( arg2[0] == '\0' )
    {
	if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
	{
	    /* 'get obj' */
	    obj = get_obj_list( ch, arg1, ch->in_room->contents );
	    if ( obj == NULL )
	    {
		act( "I see no $T here.", ch, NULL, arg1, TO_CHAR );
		return;
	    }

            if (ch->fighting && (IS_NPC(ch) || !BIT_GET(ch->pcdata->traits, TRAIT_FLEET))
	     && number_bits(2) != 0)
            {
                act( "You're too busy fighting to get $p.",ch,obj,NULL, TO_CHAR);
                return;
            }
            else
            {
                get_obj( ch, obj, NULL );
            } 
        }
        else
	{
            if (ch->fighting)
            {
                send_to_char("You're too busy fighting to get everything on the ground.\n\r",ch);
                return;
            }
	    /* 'get all' or 'get all.obj' */
	    found = FALSE;

	    for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
	    {
		obj_next = obj->next_content;

		if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
		&&   can_see_obj( ch, obj )
		&& CAN_WEAR(obj, ITEM_TAKE)
		&& !IS_OBJ_STAT(obj, ITEM_NOLONG) )
		{
		    found = TRUE;
		    get_obj( ch, obj, NULL );
		}
	    }

	    if ( !found ) 
	    {
		if ( arg1[3] == '\0' )
		    send_to_char( "I see nothing here.\n\r", ch );
		else
		    act( "I see no $T here.", ch, NULL, &arg1[4], TO_CHAR );
	    }
	}
    }
    else
    {
	/* 'get ... container' */
	if ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
	{
	    send_to_char( "You can't do that.\n\r", ch );
	    return;
	}

	if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
	{
	    act( "I see no $T here.", ch, NULL, arg2, TO_CHAR );
	    return;
	}

	switch ( container->item_type )
	{
	default:
	    send_to_char( "That's not a container.\n\r", ch );
	    return;

	case ITEM_CONTAINER:
        {
             if (ch->fighting && container->carried_by != ch 
             && number_bits(1) == 0)
             {
                 send_to_char("You're too busy fighting!\n\r",ch);
                 return;
             }
        }
        break;
	case ITEM_CORPSE_NPC:
            {
                if (ch->fighting && ch->class_num != 15)
                {
                    send_to_char("You're too busy fighting to loot a corpse.\n\r",ch);
                    return;
                }  
             }
        break; 
	case ITEM_CORPSE_PC:
	{

            if (ch->fighting && ch->class_num != 15)
            {
                send_to_char("You're too busy fighting to loot a corpse.\n\r",ch);
                return;
            }


            if (!can_loot(ch,container))
            { 	
    	        send_to_char( "You can't do that.\n\r", ch );
	        return;
            }
         }
         break; 
	}   

	if ((container->item_type == ITEM_CONTAINER) && IS_SET(container->value[1], CONT_CLOSED))
	{
	    act( "$d is closed.", ch, NULL, container->short_descr, TO_CHAR );
	    return;
	}

	if (container->in_room && ((container->item_type == ITEM_CORPSE_PC) || (container->item_type == ITEM_CORPSE_NPC)))
	{
	    CHAR_DATA *wch;

	    for (wch = container->in_room->people; wch; wch = wch->next_in_room)
		if (IS_NPC(wch)
                 && (wch->pIndexData->vnum == MOB_VNUM_DEMON_GUARDIAN)
		 && (wch->memfocus[0] != ch)
                 && (container->objfocus[0] == wch->memfocus[0]))
		{
		    act("$n moves protectively in front of the corpse.", wch, NULL, NULL, TO_ROOM);
		    return;
		}
	}

	if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
	{
	    /* 'get obj container' */
	    obj = get_obj_list( ch, arg1, container->contains );
	    if ( obj == NULL )
	    {
		act( "I see nothing like that in the $T.", ch, NULL, arg2, TO_CHAR );
		return;
	    }
	    get_obj( ch, obj, container );

	    if (container->item_type == ITEM_CORPSE_PC)
	        WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	}
	else
	{
	    
	    num_canloot = -1;
	    if ((container->item_type == ITEM_CORPSE_PC) && !IS_IMMORTAL(ch))
        {
    		num_canloot = (str_cmp(container->owner, ch->name) ? 1 : 3 + (number_percent() < 50));
            if (Luck::Check(*ch) == Luck::Lucky)
                ++num_canloot;
        }

	    /* 'get all container' or 'get all.obj container' */
	    found = FALSE;
	    for ( obj = container->contains; obj != NULL && num_canloot != 0; obj = obj_next )
	    {
		obj_next = obj->next_content;
		if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
		&&   can_see_obj( ch, obj ) )
		{
		    found = TRUE;
		    get_obj( ch, obj, container );
		    num_canloot--;
		}
	    }
	    if (container->item_type == ITEM_CORPSE_PC)
	    {
		save_char_obj(ch);
		WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
            }
	    if ( !found )
	    {
		if ( arg1[3] == '\0' )
		    act( "I see nothing in the $T.",
			ch, NULL, arg2, TO_CHAR );
		else
		    act( "I see nothing like that in the $T.",
			ch, NULL, arg2, TO_CHAR );
	    }
	}
    }
}

void do_put( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *container;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (!str_cmp(arg2,"in") || !str_cmp(arg2,"on"))
	argument = one_argument(argument,arg2);

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Put what in what?\n\r", ch );
	return;
    }

    if ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
    {
	send_to_char( "You can't do that.\n\r", ch );
	return;
    }

    if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
    {
	act( "I see no $T here.", ch, NULL, arg2, TO_CHAR );
	return;
    }

    if ( container->item_type != ITEM_CONTAINER )
    {
	send_to_char( "That's not a container.\n\r", ch );
	return;
    }

    if ( IS_SET(container->value[1], CONT_CLOSED) )
    {
	act( "$d is closed.", ch, NULL, container->short_descr, TO_CHAR );
	return;
    }

    if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
    {
	/* 'put obj container' */
	if ( ( obj = get_obj_carry( ch, arg1, ch ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}

	if (IS_PAFFECTED(ch, AFF_VOIDWALK)) 
	{
            act("Your immaterial hands pass through $d.", ch, NULL, obj->short_descr, TO_CHAR);
            return;
	}
	
	if ( obj == container )
	{
	    send_to_char( "You can't fold it into itself.\n\r", ch );
	    return;
	}

	if ( !can_drop_obj( ch, obj ) )
	{
	    send_to_char( "You can't let go of it.\n\r", ch );
	    return;
	}

	if (obj->item_type == ITEM_CONTAINER)
	{
	    send_to_char("You cannot place containers inside of containers.\n\r", ch);
	    return;
	}

    if (WEIGHT_MULT(obj) != 100)
    {
        send_to_char("You have a feeling that would be a bad idea.\n\r",ch);
        return;
    }

	if ((get_obj_weight(obj) + get_weight_contents(*container)) > container->value[0] * 10 || (get_obj_weight(obj) > (container->value[3] * 10)))
	{
	    send_to_char("It won't fit.\n", ch);
	    return;
	}

    if ((container->pIndexData->vnum == OBJ_VNUM_DISC || container->pIndexData->vnum == OBJ_VNUM_WATERWHEEL)
    && container->contains != NULL)
    {
        act("$p isn't big enough for more than one item.", ch, container, NULL, TO_CHAR);
        return;
    }

	if (obj->pIndexData->limit > 0 && container->pIndexData->vnum != OBJ_VNUM_RAIDER_BELT 
    && container->pIndexData->vnum != OBJ_VNUM_DISC && (container->wear_flags & ITEM_TAKE))
	{
	    send_to_char("That item is too powerful to place in such a container.\n\r", ch);
	    return;
	}
	if (obj == ch->nocked)
	{
	    if (is_affected(ch,gsn_ready))
	    {
		act("You stop readying a shot.",ch,NULL,NULL,TO_CHAR);
		act("$n stops readying a shot.",ch,NULL,NULL,TO_ROOM);
		affect_strip(ch,gsn_ready);
	    }
	    ch->nocked = NULL;
	}

	obj_from_char( obj );
	obj_to_obj( obj, container );

	if (IS_SET(container->value[1],CONT_PUT_ON))
	{
        if (!IS_AFFECTED(ch, AFF_SNEAK))
    	    act("$n puts $p on $P.",ch,obj,container, TO_ROOM);
	    act("You put $p on $P.",ch,obj,container, TO_CHAR);
	}
	else
	{
        if (!IS_AFFECTED(ch, AFF_SNEAK))
            act( "$n puts $p in $P.", ch, obj, container, TO_ROOM );
	    act( "You put $p in $P.", ch, obj, container, TO_CHAR );
	}
    }
    else
    {
        /* 'put all container' or 'put all.obj container' */
        for ( obj = ch->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;

            if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
            &&   can_see_obj( ch, obj )
            &&   WEIGHT_MULT(obj) == 100
            &&   !obj->worn_on
            &&   obj != container
            &&   obj->item_type != ITEM_CONTAINER
            &&   can_drop_obj( ch, obj )
            &&   get_obj_weight( obj ) + get_weight_contents(*container)
             <= (container->value[0] * 10) 
            &&   get_obj_weight(obj) < (container->value[3] * 10))
            {
                if (obj->pIndexData->limit > 0 && container->pIndexData->vnum != OBJ_VNUM_RAIDER_BELT 
                && container->pIndexData->vnum != OBJ_VNUM_DISC && (container->wear_flags & ITEM_TAKE))
                {
                    send_to_char("That item is too powerful to place in such a container.\n\r", ch);
                    continue;
                }

                if ((container->pIndexData->vnum == OBJ_VNUM_DISC || container->pIndexData->vnum == OBJ_VNUM_WATERWHEEL)
                && container->contains != NULL)
                {
                    act("$p isn't big enough for more than one item.", ch, container, NULL, TO_CHAR);
                    continue;
                }

                obj_from_char( obj );
                obj_to_obj( obj, container );

                if (IS_SET(container->value[1],CONT_PUT_ON))
                {
                    if (!IS_AFFECTED(ch, AFF_SNEAK))
                        act("$n puts $p on $P.",ch,obj,container, TO_ROOM);
                    act("You put $p on $P.",ch,obj,container, TO_CHAR);
                }
                else
                {
                    if (!IS_AFFECTED(ch, AFF_SNEAK))
                        act( "$n puts $p in $P.", ch, obj, container, TO_ROOM );
                    act( "You put $p in $P.", ch, obj, container, TO_CHAR );
                }
            }
        }
    }
}

void do_search( CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *pObj;
    char buf[MAX_STRING_LENGTH];

    send_to_char("You go over the surrounding area inch by inch.\n\r", ch);
    act("$n goes over the surrounding area inch by inch.", ch, NULL, NULL, TO_ROOM);

    for (pObj = ch->in_room->contents ; pObj != NULL ; pObj = pObj->next_content)
    {
	if (IS_SET(pObj->extra_flags[0],ITEM_STASHED))
	{
	    if (ch->class_num == global_int_class_bandit || ch->class_num == global_int_class_watcher
	     || number_percent() < 20 + get_curr_stat(ch,STAT_INT) + get_curr_stat(ch,STAT_WIS))
	    {
		sprintf(buf, "You uncover %s while searching about.\n\r", pObj->short_descr);
		send_to_char(buf, ch);
		REMOVE_BIT(pObj->extra_flags[0], ITEM_STASHED);
	    }
	}
    }

    WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
}

void do_craftdart(CHAR_DATA *ch, char *argument)
{
    int skill, mod;
    int type = 0;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *vial = NULL, *darts;

    if ((skill = get_skill(ch,gsn_craftdart)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if ((vial = get_obj_carry(ch, argument, ch)) == NULL)
    {
	send_to_char("You aren't carrying that.\n\r", ch);
	return;
    }

    if (vial->pIndexData->vnum != OBJ_VNUM_POISON_VIAL)
    {
	send_to_char("You can only make darts from a poison vial.\n\r", ch);
	return;
    }

    for (type = 0;poison_table[type].spell_fun;type++)
	if (obj_is_affected(vial,*(poison_table[type].sn)))
	    break;
    
    if (poison_table[type].spell_fun == NULL)
    {
	send_to_char("You can't create poison from that.\n\r",ch);
	return;
    }

    mod = get_modifier(vial->affected, *(poison_table[type].sn));

    extract_obj(vial);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_craftdart].beats));

    if (number_percent() > skill)
    {
	check_improve(ch,NULL,gsn_craftdart,FALSE,2);
	act("You try to coat a dart with poison, but fail and ruin the components.", ch, NULL, NULL, TO_CHAR);
	act("$n tries to coat a dart with poison, but fails and ruins the components.", ch, NULL, NULL, TO_ROOM);
	return;
    }
   
    darts = create_object(get_obj_index(OBJ_VNUM_POISON_DART), 0);

    act("$n carefully crafts and poisons a dart.", ch, NULL, NULL, TO_ROOM);
    act("You deftly craft and poison a dart.", ch, NULL, NULL, TO_CHAR);
    check_improve(ch,NULL,gsn_craftdart,TRUE,1);

    af.where      = TO_OBJECT;
    af.type       = *(poison_table[type].sn);
    af.level      = ch->level;
    af.duration   = -1;
    af.location   = 0;
    af.modifier   = 0;
    af.bitvector  = 0;
    affect_to_obj(darts, &af);
    darts->timer = mod+3;

    obj_to_char(darts, ch);
}

void do_identifyowner(CHAR_DATA *ch, char *argument)
{
    int skill;
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];
    int i;

    if ((skill = get_skill(ch,gsn_identifyowner)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if ((obj = get_obj_carry(ch, argument, ch)) == NULL)
    {
	send_to_char("You aren't carrying that.\n\r", ch);
	return;
    }

    if (skill < number_percent())
    {
	send_to_char("You failed to identify the owner.\n\r", ch);

	for (i = 0; i < MAX_LASTOWNER; i++)
	{
	    free_string(obj->lastowner[i]);
	    obj->lastowner[i] = str_dup("");
	}

        check_improve(ch,NULL,gsn_identifyowner,FALSE,2);
	return;
   }

   if (obj->lastowner[0][0] == '\0')
   {
	send_to_char("You can't seem to tell who owned this last.\n\r", ch);
	return;
   }

   sprintf(buf, "This seems to have last been in the possession of %s.\n\r", obj->lastowner[0]);
   send_to_char(buf, ch);

   if (obj->lastowner[1][0] != '\0')
   {
	sprintf(buf, "Before that, it seems to have been owned by %s.\n\r", obj->lastowner[1]);
	send_to_char(buf, ch);
   }

} 

void do_swordrepair(CHAR_DATA * ch, char * argument)
{
	int skill, impmod;
	OBJ_DATA * obj;
	OBJ_DATA * obj2;
	OBJ_DATA * obj3;
	OBJ_DATA * obj4;
	char arg[MAX_INPUT_LENGTH];

	impmod = 0;
	
	if ((skill = get_skill(ch, gsn_swordrepair)) == 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg);
	if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
	{
		send_to_char("You are not carrying that.\n\r", ch);
		return;
	}

	if (obj->pIndexData->vnum == 22721 || obj->pIndexData->vnum == 22722 || obj->pIndexData->vnum == 22723)
	{		
		argument = one_argument(argument, arg);
		if (arg[0] == '\0'
		|| ((obj2 = get_obj_carry(ch, arg, ch)) == NULL)
		|| (obj2 == obj)
		|| (obj2->pIndexData->vnum != 22721 && obj2->pIndexData->vnum != 22722 && obj2->pIndexData->vnum != 22723))
		{
			send_to_char("You can only fix broken swords if you use all the pieces.\n\r", ch);
			return;
		}
		argument = one_argument(argument, arg);
		if (arg[0] == '\0'
		|| ((obj3 = get_obj_carry(ch, arg, ch)) == NULL)
		|| (obj3 == obj2 || obj3 == obj)
		|| (obj3->pIndexData->vnum != 22721 && obj3->pIndexData->vnum != 22722 && obj3->pIndexData->vnum != 22723))
		{
			send_to_char("You can only fix broken swords if you use all the pieces.\n\r", ch);
			return;			
		}
		if ((obj4 = get_obj_carry(ch, "focusing", ch)) == NULL || obj4->pIndexData->vnum != 22614)
		{
			send_to_char("You must be carrying a gem of the proper type to bind into this sword.\n\r", ch);
			return;
		}
		if (skill < 100)
		{
			send_to_char("With a sword this intricate, only a master should even attempt to repair it.\n\r", ch);
			return;
		}
		act("$N masterfully repairs a sword, then touches its hilt with a perfectly-formed diamond.", 
			ch, NULL, NULL, TO_ROOM);
		act("When $e pulls the diamond away, a tiny but perfect copy of it has been left in the hilt of the blade.",
			ch, NULL, NULL, TO_ROOM);
		act("You masterfully repair a sword, then touch its hilt with a perfectly-formed diamond.",
			ch, NULL, NULL, TO_CHAR);
		act("When you pull the diamond away, a tiny but perfect copy of it has been left in the hilt of the blade.",
			ch, NULL, NULL, TO_CHAR);
		extract_obj(obj);
		extract_obj(obj2);
		extract_obj(obj3);
		obj = create_object(get_obj_index(22724), 51);
		obj_to_char(obj, ch);
		return;
	}

	if (obj->item_type != ITEM_WEAPON || obj->value[0] != WEAPON_SWORD)
	{
		send_to_char("You only know how to repair swords.\n\r", ch);
		return;
	}

	if (ch->mana < skill_table[gsn_swordrepair].min_mana)
	{
		send_to_char("You don't have the mental focus right now to repair a sword.\n\r", ch);
		return;
	}
		
	if (obj->condition >= 100)
	{
		send_to_char("That sword is already in excellent shape, but you attempt to hone its edge.\n\r", ch);
		impmod = 3;
	}

	WAIT_STATE(ch, skill_table[gsn_swordrepair].beats);
	expend_mana(ch, skill_table[gsn_swordrepair].min_mana);
	if (number_percent() < skill)
	{
		act("Your efforts to repair $p improve it slightly!", ch, obj, NULL, TO_CHAR);
		act("$n's efforts to repair $p improve it slightly!", ch, obj, NULL, TO_ROOM);
		obj->condition += (impmod == 0 ? 2 : 0);
		check_improve(ch, NULL, gsn_swordrepair, TRUE, UMAX(6 + impmod - (obj->level / 10), 1));
	}
	else
	{
		if (number_percent() < 50)
		{
			act("Your efforts to repair $p degrade it slightly!", ch, obj, NULL, TO_CHAR);
			act("$n's efforts to repair $p degrade it slightly!", ch, obj, NULL, TO_ROOM);
			obj->condition -= 1;
		}
		else
		{
			act("Your efforts to repair $p have no effect upon it.", ch, obj, NULL, TO_CHAR);
			act("$n's efforts to repair $p have no effect upon it.", ch, obj, NULL, TO_ROOM);
		}
		check_improve(ch, NULL, gsn_swordrepair, FALSE, UMAX(6 + impmod - (obj->level / 10), 1));
	}
}

void do_prepare(CHAR_DATA *ch, char *argument )
{
    int skill;
    int ccount, numcat, extracted;
    int type = 0;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *vial, *obj, *next_obj;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    if ((skill = get_skill(ch,gsn_prepare)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (argument[0] == '\0')
    {
	send_to_char("Prepare how many vials of catalyst into what poison?\n\r", ch);
	return;
    }

    if ((numcat = atoi(arg)) < 1)
    {
	send_to_char("You must use at least one vial of catalyst.\n\r", ch);
	return;
    }
 
    ccount = extracted = 0;

    for (type=0;poison_table[type].spell_fun;type++)
        if (!str_prefix(argument, poison_table[type].name))
	    break;

    if (poison_table[type].spell_fun == NULL)
    {
	sprintf(buf,"You can prepare the following types of poisons:\n\r");
	for (type=0;poison_table[type].spell_fun != NULL;type++)
	    sprintf(buf,"%s%s%s",buf,poison_table[type].name,
	      poison_table[type+1].spell_fun == NULL ? ".\n\r" : ", ");
	send_to_char(buf,ch);
	return;
    }

    for (obj = ch->carrying;obj != NULL;obj = next_obj)
    {
	next_obj = obj->next_content;
	if (obj->pIndexData->vnum == OBJ_VNUM_CATALYST_VIAL
        && !obj->worn_on && obj->in_obj == NULL)
	  ccount++;
    }

    if (ccount < numcat)
    {
	send_to_char("You don't have that many vials of catalyst.\n\r", ch);
	return;
    }

    for (obj = ch->carrying;obj != NULL;obj = next_obj)
    {
	next_obj = obj->next_content;
	if (obj->pIndexData->vnum == OBJ_VNUM_CATALYST_VIAL
	  && !obj->worn_on && obj->in_obj == NULL)
    	{
	    extract_obj(obj);
	    extracted++;
	    if (extracted >= numcat)
	        break;
	}
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_prepare].beats));

    if (number_percent() > skill)
    {
      	check_improve(ch,NULL,gsn_prepare,FALSE,2);
      	act("$n mixes a few vials together, but looks frustrated as no reaction occurs.", ch, NULL, NULL, TO_ROOM);
     	act("You mix the appropriate things together, but no reaction seems to occur.", ch, NULL, NULL, TO_CHAR);
     	return;
    }

    act("$n mixes a few vials together, and looks satisfied as one turns crystal clear.", ch, NULL, NULL, TO_ROOM);
    act("You mix a few vials together, and are satisfied as your ingredients mix to form a crystal clear poison.", ch, NULL, NULL, TO_CHAR);

    vial = create_object(get_obj_index(OBJ_VNUM_POISON_VIAL), 0);
    check_improve(ch,NULL,gsn_prepare,TRUE,2);

    af.where      = TO_OBJECT;
    af.type       = *(poison_table[type].sn);
    af.level      = ch->level;
    af.duration   = -1;
    af.location   = 0;
    af.modifier   = numcat;
    af.bitvector  = 0;
    affect_to_obj(vial, &af);
 
    sprintf(buf, obj->short_descr, poison_table[type].name);
    free_string(obj->short_descr);
    obj->short_descr = str_dup(buf);
    sprintf(buf, "vial poison %s", poison_table[type].name);
    setName(*obj, buf);
    obj_to_char(obj, ch);
}

void do_findherbs(CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *pObj;
    int j, skill;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];
    bool hFound = FALSE;
 
    if ((skill = get_skill(ch, gsn_findherbs)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_findherbs))
    {
	send_to_char("You are not ready to search for herbs again.\n\r", ch);
	return;
    }

    if (!ch->in_room)
	return;

    if ((ch->in_room->sector_type != SECT_FIELD)
     && (ch->in_room->sector_type != SECT_FOREST)
     && (ch->in_room->sector_type != SECT_HILLS)
     && (ch->in_room->sector_type != SECT_MOUNTAIN)
     && (ch->in_room->sector_type != SECT_SWAMP))
    {
	send_to_char("You cannot search for herbs here.\n\r", ch);
	return;
    }

    send_to_char("You search the area for herbs.\n\r", ch);
    act("$n searches the area for herbs.", ch, NULL, NULL, TO_ROOM);

    af.where	 = TO_AFFECTS;
    af.type	 = gsn_findherbs;
    af.level	 = ch->level;
    af.duration  = 48;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;

    if ((ch->in_room->herb_type >= 0) && (herb_table[ch->in_room->herb_type].min_level <= ch->level) && !room_is_affected(ch->in_room, gsn_findherbs) && (number_percent() < skill))
    {
	if (!str_cmp(herb_table[ch->in_room->herb_type].name, "flower of Laeren"))
	{
	    if ((time_info.hour >= season_table[time_info.season].sun_up)
	     && (time_info.hour < ((NUM_HOURS_DAY / 2) - 1)))
		hFound = TRUE;
	}
	else if (!str_cmp(herb_table[ch->in_room->herb_type].name, "vin blossom"))
	{
	    if (!str_cmp(season_table[time_info.season].name, "autumn"))
		hFound = TRUE;
	}
	else if (!str_cmp(herb_table[ch->in_room->herb_type].name, "xel petals"))
	{
	    if (!str_cmp(season_table[time_info.season].name, "summer"))
		hFound = TRUE;
	}
	else if (!str_cmp(herb_table[ch->in_room->herb_type].name, "black belidiss blossom"))
	{
	    if ((time_info.hour >= season_table[time_info.season].sun_down)
	     || (time_info.hour < season_table[time_info.season].sun_up))
		hFound = TRUE;
	}
	else if (!str_cmp(herb_table[ch->in_room->herb_type].name, "soala leaves"))
	{
	    if ((time_info.hour >= season_table[time_info.season].sun_up)
	     && (time_info.hour < season_table[time_info.season].sun_down))
		hFound = TRUE;
	}
	else if (!str_cmp(herb_table[ch->in_room->herb_type].name, "yala blossom"))
	{
	    if (!str_cmp(season_table[time_info.season].name, "spring"))
		hFound = TRUE;
	}
	else if (!str_cmp(herb_table[ch->in_room->herb_type].name, "flower of Caelyra"))
	{
	    if (get_moon_state(1, FALSE) == MOON_NEW)
		hFound = TRUE;
	}
	else if (!str_cmp(herb_table[ch->in_room->herb_type].name, "violet belidiss blossom"))
	{
	    if ((ch->in_room->area->w_cur.storm_str > 0)
	     && (ch->in_room->area->w_cur.precip_type == 2))
		hFound = TRUE;
	}
	else if (!str_cmp(herb_table[ch->in_room->herb_type].name, "scarlet belidiss blossom"))
	{
	    if ((time_info.hour >= season_table[time_info.season].sun_down)
	     || (time_info.hour < season_table[time_info.season].sun_up))
		hFound = TRUE;
	}
	else if (!str_cmp(herb_table[ch->in_room->herb_type].name, "nru tuber"))
	{
	    if (!str_cmp(season_table[time_info.season].name, "winter"))
		hFound = TRUE;
	}
	else if (!str_cmp(herb_table[ch->in_room->herb_type].name, "rkala blossom"))
	{
	    if (!str_cmp(season_table[time_info.season].name, "summer"))
		hFound = TRUE;
	}
	else
	    hFound = TRUE;

    }

    if (hFound)
    {
        pObj = create_object(get_obj_index(OBJ_VNUM_HERB), ch->level);

	sprintf(buf, pObj->short_descr, herb_table[ch->in_room->herb_type].name);
	free_string(pObj->short_descr);
	pObj->short_descr = str_dup( buf );

	sprintf(buf, pObj->name, herb_table[ch->in_room->herb_type].name);
    setName(*pObj, buf);

	sprintf(buf, pObj->description, herb_table[ch->in_room->herb_type].name);
	free_string(pObj->description);
	pObj->description = str_dup( buf );

	pObj->value[0]  = ch->level;
	pObj->timer	= herb_table[ch->in_room->herb_type].duration;

	for (j = 0; j < 4; j++)
	    if (herb_table[ch->in_room->herb_type].spell[j])
	        pObj->value[(j+1)] = *herb_table[ch->in_room->herb_type].spell[j];
	    else
	        pObj->value[(j+1)] = -1;
 
	obj_to_char(pObj, ch);

	affect_to_room(ch->in_room, &af);

	sprintf(buf, "Your search for herbs yields %s.\n\r", pObj->short_descr);
	send_to_char(buf, ch);
	check_improve(ch,NULL, gsn_findherbs, TRUE, 1);
	affect_to_room(ch->in_room, &af);
	af.duration = 6;
	affect_to_char(ch, &af);
    } 
    else
    {
	send_to_char("You are unsuccessful in your search.\n\r", ch);
	check_improve(ch,NULL, gsn_findherbs, FALSE, 2);
    }

    return;
}

void do_wildcraft(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int skill;

    if ((skill = get_skill(ch, gsn_wildcraft)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (is_affected(ch, gsn_wildcraft))
    {
        send_to_char("You are not ready to craft anything yet.\n\r", ch);
        return;
    }

    if (ch->mana < skill_table[gsn_wildcraft].min_mana)
    {
        send_to_char("You are too tired to craft anything at the moment.\n\r", ch);
        return;
    }

    if (!(ch->in_room))
    {
        send_to_char("You cannot find sufficient materials to craft anything here.\n\r", ch);
        return;
    } 

    if (argument[0] == '\0')
    {
        send_to_char("What do you wish to craft?\n\r", ch);
        if (ch->class_num == CLASS_RANGER)
            send_to_char("Valid objects to craft are: bow spear mace arrow net firekit\n\r",ch);
        else
            send_to_char("Valid objects to craft are: spear mace firekit\n\r",ch);
        return;
    }
    if (!str_cmp(argument, "firekit"))
    {
        if (ch->in_room->sector_type != SECT_FOREST)
        {
            send_to_char("You can't find sufficient wood and moss here.\n\r",ch);
            return;
        }
            send_to_char("You start gathering small sticks and moss.\n\r",ch);
        act("$n starts gathering small sticks and moss.",ch,NULL,NULL,TO_ROOM);
        if (number_percent() > skill)
        {
            send_to_char("You find only wet and green wood, and discard it.\n\r",ch);
            act("$n discards a handful of sticks and moss.",ch,NULL,NULL,TO_ROOM);
            expend_mana(ch, skill_table[gsn_wildcraft].min_mana);
            WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_wildcraft].beats));
            check_improve(ch, NULL,gsn_wildcraft, FALSE, 1);
            return;
        }
        OBJ_DATA * obj = create_object(get_obj_index(OBJ_VNUM_FIREKIT),ch->level);
        send_to_char("You find some dry sticks and moss, and tie them together with a piece of rope.\n\r",ch);
        act("$n ties a small bundle of dry sticks and moss together.",ch,NULL,NULL,TO_ROOM);
        obj_to_char(obj,ch);
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_wildcraft].beats));

        af.where     = TO_AFFECTS;
        af.type	     = gsn_wildcraft;
        af.level     = ch->level;
            af.duration  = 1;   
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = 0;
            affect_to_char(ch, &af);
        
        expend_mana(ch, skill_table[gsn_wildcraft].min_mana);
        check_improve(ch,NULL, gsn_wildcraft, TRUE, 1);
            return;

    }
    if (!str_cmp(argument, "bow") || !str_cmp(argument, "spear") || !str_cmp(argument, "mace"))
    {
        if (!str_cmp(argument, "bow") && ch->class_num != CLASS_RANGER)
        {
            send_to_char("You don't know how to carve that.\n\r",ch);
            return;
        }
        if (ch->in_room->sector_type != SECT_FOREST)
        {
            send_to_char("You can't find sufficient wood to carve weapons here.\n\r",ch);
            return;
        }
        send_to_char("You locate a straight piece of wood and begin trimming and carving it.\n\r",ch);
        act("$n locates a straight piece of wood and begins trimming and carving it.",ch,NULL,NULL,TO_ROOM);

        if (number_percent() > skill)
        {
            send_to_char("You snap the wood in half, ruining the weapon.\n\r", ch);
            act("$n snaps the wood in half, ruining the weapon.",ch,NULL,NULL,TO_ROOM);
            expend_mana(ch, skill_table[gsn_wildcraft].min_mana);
            WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_wildcraft].beats));
            check_improve(ch, NULL,gsn_wildcraft, FALSE, 1);
            return;
        }

        OBJ_DATA * obj;
        if (!str_cmp(argument, "bow"))
        {	
            obj = create_object(get_obj_index(OBJ_VNUM_RANGER_BOW), ch->level);
            send_to_char("You finish carving a bow.\n\r", ch);
            act("$n finishes carving a bow.",ch,NULL,NULL,TO_ROOM);
        }
        else if (!str_cmp(argument, "mace"))
        {
            obj = create_object(get_obj_index(OBJ_VNUM_RANGER_MACE), ch->level);
            send_to_char("You finish carving a mace.\n\r", ch);
            act("$n finishes carving a mace.",ch,NULL,NULL,TO_ROOM);
        }
        else if (!str_cmp(argument, "spear"))
        {
            obj = create_object(get_obj_index(OBJ_VNUM_RANGER_SPEAR), ch->level);
            act("You finish carving a spear.", ch, NULL, NULL, TO_CHAR);
            act("$n finishes carving a spear.",ch, NULL, NULL, TO_ROOM);
        }
        else return;
         
        obj->level = ch->level;
        obj->pIndexData->level = ch->level;
        if ((((15 + (ch->level-15) / 5)) % 2) == 1)
        {
            obj->value[1] = 3;
            obj->value[2] = 10+(ch->level-15)/10;
        }
        else
        {
            obj->value[1] = 4;
            obj->value[2] = 6+(ch->level-10)/10;
        }
        obj_to_char(obj, ch);
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_wildcraft].beats));

        af.where     = TO_AFFECTS;
        af.type	     = gsn_wildcraft;
        af.level     = ch->level;
            af.duration  = 10;   
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = 0;
            affect_to_char(ch, &af);
        
        expend_mana(ch, skill_table[gsn_wildcraft].min_mana);
        check_improve(ch,NULL, gsn_wildcraft, TRUE, 1);
        return;
    }

    if (!strcmp(argument,"net"))
    {
	if (ch->class_num != CLASS_RANGER)
	{
	    send_to_char("You don't know how to make that.\n\r",ch);
	    return;
	}
	if (!(ch->in_room->sector_type == SECT_FIELD
	  || ch->in_room->sector_type == SECT_ROAD
	  || ch->in_room->sector_type == SECT_FOREST
	  || ch->in_room->sector_type == SECT_SWAMP))
	{
	    send_to_char("You need long grasses or vines to weave a net.\n\r",ch);
	    return;
	}
	if (ch->in_room->sector_type == SECT_FIELD
	  || ch->in_room->sector_type == SECT_ROAD)
	{
	    send_to_char("You gather long grasses and begin plaiting them.\n\r",ch);
	    act("$n gathers long grasses and begins plaiting them.",ch,NULL,NULL,TO_ROOM);
	}
	else
	{
	    send_to_char("You gather several vines, inspecting them for weak spots.\n\r",ch);
	    act("$n gathers several vines, inspecting them for weak spots.",ch,NULL,NULL,TO_ROOM);
	}

	if (number_percent() > skill)
	{
	    if (ch->in_room->sector_type == SECT_FIELD || ch->in_room->sector_type == SECT_ROAD)
	    {
		send_to_char("You tear the grasses during your plaiting, and discard them.\n\r", ch);
		act("$n tears the grasses during $s plaiting, and discards them.",ch,NULL,NULL,TO_ROOM);
	    }
	    else
	    {
		send_to_char("The vines fray too badly as you braid them and you discard them.\n\r",ch);
	 	act("The vines fray as $n braids them and $e discards them.",ch,NULL,NULL,TO_ROOM);
	    }
	    expend_mana(ch, skill_table[gsn_wildcraft].min_mana);
	    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_wildcraft].beats));
	    check_improve(ch, NULL,gsn_wildcraft, FALSE, 1);
	    return;
	}
	OBJ_DATA * obj = create_object(get_obj_index(OBJ_VNUM_RANGER_NET), ch->level);
	if (ch->in_room->sector_type == SECT_FIELD || ch->in_room->sector_type == SECT_ROAD)
	{
	    send_to_char("You weave the grasses into a crude net.\n\r", ch);
	    act("$n weaves the grasses into a crude net.",ch,NULL,NULL,TO_ROOM);
	}
	else
	{
	    send_to_char("You weave the vines into a crude net.\n\r",ch);
	    act("$n weaves the vines into a crude net.",ch,NULL,NULL,TO_ROOM);
	}

	obj_to_char(obj, ch);
	if (ch->in_room->sector_type == SECT_FIELD || ch->in_room->sector_type == SECT_ROAD)
	    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_wildcraft].beats * 2));
	else
	    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_wildcraft].beats));

	af.where     = TO_AFFECTS;
	af.type	     = gsn_wildcraft;
	af.level     = ch->level;
        af.duration  = 10;   
        af.location  = 0;
        af.modifier  = 0;
        af.bitvector = 0;
        affect_to_char(ch, &af);
	
	expend_mana(ch, skill_table[gsn_wildcraft].min_mana);
	check_improve(ch,NULL, gsn_wildcraft, TRUE, 1);
        return;
    }
    
    if (str_cmp(argument,"arrow"))
    {
	send_to_char("What did you want to craft?\n\r",ch);
	if (ch->class_num == CLASS_RANGER)
	    send_to_char("Valid objects to craft are: bow spear mace arrow net firekit\n\r",ch);
	else
	    send_to_char("Valid objects to craft are: spear mace firekit\n\r",ch);
	return;
    }

    if (ch->class_num != CLASS_RANGER)
    {
	send_to_char("You don't know how to carve that.\n\r",ch);
	return;
    }

    if (ch->in_room->sector_type != SECT_FOREST)
    {
        send_to_char("You can't find sufficient wood to carve arrows here.\n\r",ch);
        return;
    }
    send_to_char("You locate a thin piece of wood and begin carving it into an arrow.\n\r",ch);
    act("$n locates a thin piece of wood and begins carving it into an arrow.",ch,NULL,NULL,TO_ROOM);
    if (number_percent() > skill)
    {
    	send_to_char("You snap the wood in half, ruining the arrow.\n\r", ch);
	act("$n snaps the wood in half, ruining the arrow.",ch,NULL,NULL,TO_ROOM);
    	expend_mana(ch, skill_table[gsn_wildcraft].min_mana);
    	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_wildcraft].beats));
    	check_improve(ch,NULL, gsn_wildcraft, FALSE, 1);
    	return;
    }

    send_to_char("You finish carving an arrow.\n\r",ch);
    act("$n finishes carving an arrow.",ch,NULL,NULL,TO_ROOM);

    OBJ_DATA * obj = create_object(get_obj_index(OBJ_VNUM_RANGER_ARROW), ch->level);
    if (obj->lore)
	free_string(obj->lore);
    char buf[MAX_STRING_LENGTH];
    sprintf(buf,"The handiwork of this arrow appears to have been done by %s.", IS_NPC(ch) ? ch->short_descr : ch->name);
    obj->lore = str_dup(buf);
    obj->level = ch->level;
    obj_to_char(obj,ch);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_wildcraft].beats));
    af.where     = TO_AFFECTS;
    af.type      = gsn_wildcraft;
    af.level     = ch->level;
    af.duration  = 1;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.where	 = TO_OBJECT;
    af.type 	 = gsn_arrowcraft;
    af.duration  = -1;
    af.location  = APPLY_RANGE;
    af.modifier  = ch->level/13;
    affect_to_obj(obj,&af);

    expend_mana(ch, skill_table[gsn_wildcraft].min_mana);
    check_improve(ch,NULL, gsn_wildcraft, TRUE, 1);
    return;
}

void do_callanimal(CHAR_DATA *ch, char *argument )
{
    int skill;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *animal;

    if ((skill = get_skill(ch,gsn_callanimal)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_callanimal))
    {
	send_to_char("You have already called for an animal recently.\n\r", ch);
	return;
    }

    if (!can_charm_another(ch))
    {
	send_to_char("You cannot control any more followers!\n\r", ch);
	return;
    }

    if (ch->mana < skill_table[gsn_callanimal].min_mana)
    {
	send_to_char("You're too tired to call for an animal companion.\n\r", ch);
	return;
    }

    if (ch->pet != NULL)
    {
	send_to_char("You already have a pet following you!\n\r", ch);
	return;
    }

    if (number_percent() > skill)
    {
	act("$n tries to calls to nature for aid, but no aid comes.", ch, NULL, NULL, TO_ROOM);
	act("You try to call to nature for aid, but no aid comes.", ch, NULL, NULL, TO_CHAR);
	check_improve(ch,NULL,gsn_callanimal,FALSE,2);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_callanimal].beats));
	expend_mana(ch, skill_table[gsn_callanimal].min_mana/2);
	return;
    }

    if (ch->in_room->sector_type == SECT_FOREST)
	animal = create_mobile(get_mob_index(MOB_VNUM_BLACKBEAR));
    else if (ch->in_room->sector_type == SECT_HILLS)
	animal = create_mobile(get_mob_index(MOB_VNUM_GREYWOLF));
    else if (ch->in_room->sector_type == SECT_MOUNTAIN)
	animal = create_mobile(get_mob_index(MOB_VNUM_HAWK));
    else if (ch->in_room->sector_type == SECT_SWAMP)
	animal = create_mobile(get_mob_index(MOB_VNUM_ALLIGATOR));
    else
    {
// brazen: Ticket #46: Call animal that succeeds in an area with no animals doesn't give an echo
	act("$n tries to calls to nature for aid, but no aid comes.", ch, NULL, NULL, TO_ROOM);
	act("You try to call to nature for aid, but no aid comes.", ch, NULL, NULL, TO_CHAR);
	bug("Call animal: bad sector type for calling.", 0);
	return;
    }


    animal->level = ch->level;
    animal->hit = ch->max_hit;
    animal->max_hit = ch->max_hit;
    animal->damroll = 5 + ch->level/2;
    animal->hitroll = 5 + ch->level/2;
    animal->damage[0] = ch->level/3;
    animal->damage[1] = 5;
    animal->armor[0] = 0 - ch->level;
    animal->armor[1] = 0 - ch->level;
    animal->armor[2] = 0 - ch->level;
    animal->armor[3] = 0 - ch->level;

    af.where     = TO_AFFECTS;
    af.type      = gsn_callanimal;
    af.level     = ch->level;
    af.duration  = 7;
    af.location  = APPLY_HIDE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch,&af);

    char_to_room(animal, ch->in_room);
    add_follower(animal, ch);
    animal->leader = ch;
    animal->master = ch;
    SET_BIT(animal->act, ACT_PET);
    SET_BIT(animal->affected_by, AFF_CHARM);
    animal->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;
    ch->pet = animal;
    
    check_improve(ch,NULL,gsn_callanimal,TRUE,2);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_callanimal].beats));
    expend_mana(ch, skill_table[gsn_callanimal].min_mana);
    act("$n calls to nature for help, and $N comes to $s aid!", ch, NULL, animal, TO_ROOM);
    act("You call to nature for help, and $N comes to your aid!", ch, NULL, animal, TO_CHAR);

    return;
}

void do_findwater(CHAR_DATA *ch, char *argument )
{
int skill;
OBJ_DATA *spring;

    if ((skill = get_skill(ch,gsn_findwater)) == 0)
	{
	send_to_char("Huh?\n\r", ch);
	return;
	}

    if (ch->mana < skill_table[gsn_findwater].min_mana)
	{
	send_to_char("You're too tired to seek water.\n\r", ch);
	return;
	}

	if (ch->in_room->sector_type == SECT_INSIDE 
	  || ch->in_room->sector_type == SECT_CITY
	  || ch->in_room->sector_type == SECT_ROAD)
	{
	    send_to_char("This area is too civilized to find water from the earth.\n\r", ch);
	    return;
	}

	if (ch->in_room->sector_type == SECT_UNDERWATER 
	  || ch->in_room->sector_type == SECT_WATER_SWIM
	  || ch->in_room->sector_type == SECT_WATER_NOSWIM)
	{
	    send_to_char("Are you having trouble finding water here?\n\r", ch);
	    return;
	}
	if (ch->in_room->sector_type == SECT_AIR)
	{
	    send_to_char("In what earth are you planning to find water?\n\r",ch);
	    return;
	}

    if (number_percent() > skill)
	{
	act("$n seeks water in the earth, but it yields $m none.", ch, NULL, NULL, TO_ROOM);
	act("You seek water in the earth, but it yields you none.", ch, NULL, NULL, TO_CHAR);
	check_improve(ch,NULL,gsn_findwater,FALSE,2);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_findwater].beats));
	expend_mana(ch, skill_table[gsn_findwater].min_mana/2);
	return;
	}
	
    spring = create_object(get_obj_index(OBJ_VNUM_RANGER_SPRING), 0);
    spring->timer = number_fuzzy(ch->level+4);

    obj_to_room(spring, ch->in_room);
    check_improve(ch,NULL,gsn_findwater,TRUE,2);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_findwater].beats));
    expend_mana(ch, skill_table[gsn_findwater].min_mana);
    act("$n seeks water in the earth, and it yields up a gushing spring.", ch, NULL, NULL, TO_ROOM);
    act("You seek water in the earth, and it yields up a gushing spring.", ch, NULL, NULL, TO_CHAR);

}
	

void do_herbalmedicine( CHAR_DATA *ch, char *argument )
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
int skill;
CHAR_DATA *victim;

    if ((skill = get_skill(ch,gsn_herbalmedicine)) == 0)
	{
	send_to_char("Huh?\n\r", ch);
	return;
	}

    if (argument[0] == '\0')
	{
	victim = ch;
	}
	else
	{
	if ((victim = get_char_room(ch, argument)) == NULL)
		{
		send_to_char("You don't see them here.\n\r", ch);
		return;
		}
	}

    if (ch->in_room->sector_type != SECT_FOREST && ch->in_room->sector_type != SECT_HILLS && ch->in_room->sector_type != SECT_SWAMP)
	{
	send_to_char("You can't find the right herbs for making herbal medicine here.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	return;
	}


    if (is_affected(ch, gsn_herbalmedicine))
	{
	send_to_char("You're not ready to make more herbal medicine.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	return;
	}

    if (ch->mana < skill_table[gsn_herbalmedicine].min_mana)
	{
	send_to_char("You can't concentrate enough to make herbal medicine.\n\r", ch);
	return;
	}

   if (number_percent() < skill)
	{
	check_improve(ch,victim,gsn_herbalmedicine,TRUE,1);
	if (ch == victim)
		{
		act("You prepare a herbal medicine for healing yourself.", ch, NULL, NULL, TO_CHAR);
		act("$n heals $mself with herbal medicine.", ch, NULL, NULL, TO_ROOM);
		}
		else
		{
		act("You prepare herbal medicine to heal $N.", ch, NULL, victim, TO_CHAR);
		act("$n prepares herbal medicine to heal you.", ch, NULL, victim, TO_VICT);
		act("$n prepares herbal medicine to heal $N.", ch, NULL, victim, TO_NOTVICT);
		}
	victim->hit = UMIN(victim->max_hit, victim->hit + (number_range(ch->level, 2*ch->level)));
	obj_cast_spell(gsn_cure_poison, ch->level, ch, victim, NULL);
	obj_cast_spell(gsn_cure_disease, ch->level, ch, victim, NULL);
        af.where     = TO_AFFECTS;
        af.type      = gsn_herbalmedicine;
        af.level     = ch->level;
        af.duration  = 7;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = 0;
        affect_to_char(ch,&af);
	}
	else
	{
	act("You try to prepare herbal medicine, but fail.", ch, NULL, NULL, TO_CHAR);
	act("$n tries to prepare herbal medicine, but fails.", ch, NULL, NULL, TO_ROOM);
	check_improve(ch,victim,gsn_herbalmedicine,FALSE,1);
	return;
	}
    expend_mana(ch, skill_table[gsn_herbalmedicine].min_mana);

}


void do_poultice( CHAR_DATA *ch, char *argument )
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
int skill;
CHAR_DATA *victim;

    if ((skill = get_skill(ch,gsn_poultice)) == 0)
	{
	send_to_char("Huh?\n\r", ch);
	return;
	}

    if (argument[0] == '\0')
	{
	victim = ch;
	}
	else
	{
	if ((victim = get_char_room(ch, argument)) == NULL)
		{
		send_to_char("You don't see them here.\n\r", ch);
		return;
		}
	}

    if (ch->in_room->sector_type != SECT_FOREST 
      && ch->in_room->sector_type != SECT_HILLS 
      && ch->in_room->sector_type != SECT_FIELD
      && ch->in_room->sector_type != SECT_MOUNTAIN
      && ch->in_room->sector_type != SECT_SWAMP)
	{
	send_to_char("You can't find the right herbs for a poultice here.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	return;
	}


    if (is_affected(ch, gsn_poultice))
	{
	send_to_char("You're not ready to use another poultice.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	return;
	}

    if (ch->mana < skill_table[gsn_poultice].min_mana)
	{
	send_to_char("You can't concentrate enough to make a poultice.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	return;
	}

// brazen: Avatars can no longer be healed 
    if (is_an_avatar(victim))
    {
        act("You cannot apply a poultice to $N.",ch, NULL, victim, TO_CHAR);
	    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
        return;
    }
 
   if (number_percent() < skill)
	{
	check_improve(ch,victim,gsn_poultice,TRUE,1);
	if (ch == victim)
		{
		act("You prepare a poultice for healing yourself.", ch, NULL, NULL, TO_CHAR);
		act("$n heals $mself with a poultice.", ch, NULL, NULL, TO_ROOM);
		}
		else
		{
		act("You prepare a poultice to heal $N.", ch, NULL, victim, TO_CHAR);
		act("$n prepares a poultice to heal you.", ch, NULL, victim, TO_VICT);
		act("$n prepares a poultice to heal $N.", ch, NULL, victim, TO_NOTVICT);
		}
	if (victim->level < 20)
	    victim->hit =UMIN(victim->max_hit,victim->hit + (number_range(20,40)));
	else
	    victim->hit = UMIN(victim->max_hit, victim->hit + (number_range(ch->level, 2*ch->level)));
	obj_cast_spell(skill_lookup("cure poison"), ch->level, ch, victim, NULL);
	obj_cast_spell(skill_lookup("cure disease"), ch->level, ch, victim, NULL);
        af.where     = TO_AFFECTS;
        af.type      = gsn_poultice;
        af.level     = ch->level;
        af.duration  = 7;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = 0;
        affect_to_char(ch,&af);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	
	}
	else
	{
	act("You try to prepare a poultice, but fail.", ch, NULL, NULL, TO_CHAR);
	act("$n tries to prepare a poultice, but fails.", ch, NULL, NULL, TO_ROOM);
	check_improve(ch,victim,gsn_poultice,FALSE,1);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	return;
	}
	
    expend_mana(ch, skill_table[gsn_poultice].min_mana);
}

void do_stash( CHAR_DATA *ch, char *argument )
{
    int skill, ctype;
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    argument = one_argument( argument, arg );

    if ((skill = get_skill(ch,gsn_stash)) == 0)
	{
	send_to_char("Huh?\n\r", ch);
	return;
	}



    if ( arg[0] == '\0' )
    {
	send_to_char( "Stash what?\n\r", ch );
	return;
    }

    if (ch->in_room->room_flags & ROOM_VAULT)
	{
	send_to_char( "You can't find anywhere here to stash things.\n\r", ch);
	return;
	}

    if (number_percent() > skill) 
	{
	check_improve(ch,NULL,gsn_stash,FALSE,1);
	do_drop(ch, argument);
	return;
	}

    if ( is_number( arg ) )
    {
	/* 'drop NNNN coins' */
	int amount;

	amount   = atoi(arg);
	argument = one_argument( argument, arg );
	if ( amount <= 0 || ((ctype = coin_lookup(arg)) == -1))
	{
	    send_to_char( "Sorry, you can't do that.\n\r", ch );
	    return;
	}

	if (ch->coins[ctype] < amount)
	{
	    sprintf(buf, "You don't have that much %s.\n\r", coin_table[ctype].name);
	    send_to_char(buf, ch);
	    return;
	}

	coins_from_char(ch, amount, ctype);

	for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    
	    if (IS_SET(obj->extra_flags[0], ITEM_STASHED))
	    {
	        switch ( obj->pIndexData->vnum )
	        {
		    case OBJ_VNUM_COINS:
			if (obj->value[1] == ctype)
			{
		            amount += obj->value[0];
		            extract_obj(obj);
		            break;
			}
		}
	    }
	}

	obj_to_room( create_money_stashed( amount, ctype ), ch->in_room );
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	send_to_char( "OK.\n\r", ch );
	return;
    }

    if ( str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
    {
	/* 'drop obj' */
	if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}

	if ( !can_drop_obj( ch, obj ) )
	{
	    send_to_char( "You can't let go of it.\n\r", ch );
	    return;
	}

	if ( obj->pIndexData->vnum == OBJ_VNUM_STONE_POWER )
	{
	send_to_char("You can't possibly stash a Stone of Power.\n\r", ch);
	return;
	}

	SET_BIT(obj->extra_flags[0], ITEM_STASHED);
	obj_from_char( obj );
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	act( "You carefully stash $p.", ch, obj, NULL, TO_CHAR );
	obj_to_room( obj, ch->in_room );
/*	if (IS_OBJ_STAT(obj,ITEM_MELT_DROP))
	{
	    act("$p dissolves into smoke.",ch,obj,NULL,TO_ROOM);
	    act("$p dissolves into smoke.",ch,obj,NULL,TO_CHAR);
	    extract_obj(obj);
	} */
    }
    else
    {
	send_to_char("You'll have to stash one item at a time.\n\r", ch);
	return;
    }

    return;
}


void do_conceal( CHAR_DATA *ch, char *argument )
{
    int skill;
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    argument = one_argument( argument, arg );

    if ((skill = get_skill(ch,gsn_conceal)) == 0)
	{
	send_to_char("Huh?\n\r", ch);
	return;
	}


    if ( arg[0] == '\0' )
    {
	send_to_char( "Conceal what?\n\r", ch );
	return;
    }

    if (ch->in_room->room_flags & ROOM_VAULT)
	{
	send_to_char( "You can't find anywhere here to conceal things.\n\r", ch);
	return;
	}

    if (ch->in_room->sector_type != SECT_CITY && ch->in_room->sector_type != SECT_INSIDE)
	{
	send_to_char("You don't know how to conceal something so far from civilization.\n\r", ch);
	return;
	}

    if (number_percent() > skill) 
	{
	check_improve(ch,NULL,gsn_conceal,FALSE,1);
	do_drop(ch, argument);
	return;
	}

    if ( is_number( arg ) )
    {
	/* 'drop NNNN coins' */
	int amount, ctype;

	amount   = atoi(arg);
	argument = one_argument( argument, arg );
	if ( amount <= 0 || ((ctype = coin_lookup(arg)) == -1))
	{
	    send_to_char( "Sorry, you can't do that.\n\r", ch );
	    return;
	}

	if (ch->coins[ctype] < amount)
	{
	    sprintf(buf, "You don't have that much %s.\n\r", coin_table[ctype].name);
	    send_to_char(buf, ch);
	    return;
	}

	coins_from_char(ch, amount, ctype);

	for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    
	    if (IS_SET(obj->extra_flags[0], ITEM_STASHED))
	    {
	        switch ( obj->pIndexData->vnum )
	        {
		    case OBJ_VNUM_COINS:
			if (obj->value[1] == ctype)
			{
		            amount += obj->value[0];
		            extract_obj(obj);
		            break;
			}
		}
	    }
	}

	obj_to_room( create_money_concealed( amount, ctype ), ch->in_room );
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	send_to_char( "OK.\n\r", ch );
	return;
    }

    if ( str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
    {
	/* 'drop obj' */
	if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}

	if ( !can_drop_obj( ch, obj ) )
	{
	    send_to_char( "You can't let go of it.\n\r", ch );
	    return;
	}

	if ( obj->pIndexData->vnum == OBJ_VNUM_STONE_POWER )
	{
	send_to_char("You can't possibly conceal a Stone of Power.\n\r", ch);
	return;
	}

	SET_BIT(obj->extra_flags[0], ITEM_HIDDEN);
	obj_from_char( obj );
	act( "You carefully conceal $p.", ch, obj, NULL, TO_CHAR );
	obj_to_room( obj, ch->in_room );
/*	if (IS_OBJ_STAT(obj,ITEM_MELT_DROP))
	{
	    act("$p dissolves into smoke.",ch,obj,NULL,TO_ROOM);
	    act("$p dissolves into smoke.",ch,obj,NULL,TO_CHAR);
	    extract_obj(obj);
	} */
    }
    else
    {
	send_to_char("You'll have to carefully conceal one item at a time.\n\r", ch);
	return;
    }

    return;
}


void do_drop( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    bool found;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Drop what?\n\r", ch );
	return;
    }

    if ( is_number( arg ) )
    {
	/* 'drop NNNN coins/arrows' */
	long amount;
        int ctype = -1;
	char arg2[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg2);

	ctype = coin_lookup(arg2);

	amount   = atoi(arg);

	if ( amount <= 0 || (ctype == -1))
	{
	    send_to_char( "Sorry, you can't do that.\n\r", ch );
	    return;
	}

    	if (ch->coins[ctype] < amount)
	{
	    sprintf(buf, "You don't have that much %s.\n\r", coin_table[ctype].name);
	    send_to_char(buf,ch);
	    return;
	}

	if (!IS_AFFECTED(ch, AFF_SNEAK))
	{
	    sprintf(buf, "$n drops some %s.", coin_table[ctype].name);
	    act(buf, ch, NULL, NULL, TO_ROOM );
	}

	sprintf(buf, "You drop %ld %s coins.\n\r", amount, coin_table[ctype].name);
	send_to_char(buf, ch);

	coins_from_char(ch, amount, ctype);

	for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;
            if (!IS_SET(obj->extra_flags[0], ITEM_HIDDEN)
	     && !IS_SET(obj->extra_flags[0], ITEM_STASHED)
	     && (obj->pIndexData->vnum == OBJ_VNUM_COINS)
	     && obj->value[1] == ctype)
	    {
	        amount += obj->value[0];
	        extract_obj(obj);
	        break;
	    }
	}

	obj_to_room( create_money( amount, ctype ), ch->in_room );
	
	return;
    }

    if ( str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
    {
	/* 'drop obj' */
	if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}

	if ( !can_drop_obj( ch, obj ) )
	{
	    send_to_char( "You can't let go of it.\n\r", ch );
	    return;
	}
	if (obj == ch->nocked)
	{
	    if (is_affected(ch,gsn_ready))
	    {
		act("You relax your draw on $p.",ch,obj,NULL,TO_CHAR);
		act("$n relaxes $s draw on $p.",ch,obj,NULL,TO_ROOM);
		affect_strip(ch,gsn_ready);
	    }
	    ch->nocked = NULL;
	}

	obj_from_char( obj );
	if (!IS_AFFECTED(ch, AFF_SNEAK))
	  act( "$n drops $p.", ch, obj, NULL, TO_ROOM );
	act( "You drop $p.", ch, obj, NULL, TO_CHAR );
	obj_to_room( obj, ch->in_room );
/*	if (IS_OBJ_STAT(obj,ITEM_MELT_DROP))
	{
	    act("$p dissolves into smoke.",ch,obj,NULL,TO_ROOM);
	    act("$p dissolves into smoke.",ch,obj,NULL,TO_CHAR);
	    extract_obj(obj);
	} */
    }
    else
    {
	/* 'drop all' or 'drop all.obj' */
	found = FALSE;
	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;

	    if ( ( arg[3] == '\0' || is_name( &arg[4], obj->name ) )
	    &&   can_see_obj( ch, obj )
//	    &&   obj->wear_loc == WEAR_NONE
	    &&   !obj->worn_on
	    &&   can_drop_obj( ch, obj ) )
	    {
		found = TRUE;
		obj_from_char( obj );
		if (!IS_AFFECTED(ch, AFF_SNEAK))
		  act( "$n drops $p.", ch, obj, NULL, TO_ROOM );
		act( "You drop $p.", ch, obj, NULL, TO_CHAR );
		obj_to_room( obj, ch->in_room );
/*        	if (IS_OBJ_STAT(obj,ITEM_MELT_DROP))
        	{
             	    act("$p dissolves into smoke.",ch,obj,NULL,TO_ROOM);
            	    act("$p dissolves into smoke.",ch,obj,NULL,TO_CHAR);
            	    extract_obj(obj);
        	}  */
	    }
	}

	if ( !found )
	{
	    if ( arg[3] == '\0' )
		act( "You are not carrying anything.",
		    ch, NULL, arg, TO_CHAR );
	    else
		act( "You are not carrying any $T.",
		    ch, NULL, &arg[4], TO_CHAR );
	}
    }

    return;
}


void do_plant( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim,*vch;
    OBJ_DATA  *obj, *cont=NULL;
    int chance;

    if ((chance = get_skill(ch,gsn_plant)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Plant what on whom?\n\r", ch );
	return;
    }
    
    if ((IS_PAFFECTED(ch, AFF_VOIDWALK)) || is_affected(ch, gsn_astralprojection))
    {
	send_to_char("You can't reach into the material plane.\n\r", ch);
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

//    if ( obj->wear_loc != WEAR_NONE )

    if (obj->worn_on)
    {
	send_to_char( "You must remove it first.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }
    if (!IS_NPC(victim) && !IS_PK(ch,victim))
    {
	act("The gods protect $N.",ch,NULL,victim,TO_CHAR);
	act("The gods protect $N from $n.",ch,NULL,victim,TO_ROOM);
	return;
    }

//brazen: Ticket# 205: Sanity for plant and voidwalk 
    if (IS_PAFFECTED(victim, AFF_VOIDWALK)) 
    {
        send_to_char("You cannot reach into the void from the material plane.\n\r", ch);
	return;
    }
    
    if (is_affected(victim, gsn_astralprojection))
    {
	send_to_char("As a spirit, they would be unable to hold it.\n\r", ch);
	return;
    }

    if ((victim == ch))
    {
        send_to_char( "You can't plant something on yourself.\n\r",ch);
        return;
    }

	if ( victim->demontrack != NULL && victim->demonstate != 0 && ch != victim->demontrack )
	{
		send_to_char( "The mystic protections trapping the demon in the protective circle prevent you from approaching close enough.\n\r", ch);
		return;
	}

    if (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
    {
	act("$N tells you 'Sorry, you'll have to sell that.'",
	    ch,NULL,victim,TO_CHAR);
	ch->reply = victim;
	return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
	send_to_char( "You can't let go of it.\n\r", ch );
	return;
    }

    if (obj_is_affected(obj, gsn_dancingsword) || obj_is_affected(obj, gsn_hoveringshield))
	{
        act("As you go to release $p, it suddenly feels very heavy again.", ch, obj, NULL, TO_CHAR);
        act("As $n goes to release $p, $e slumps as it regains its weight.", ch, obj, NULL, TO_ROOM);
        object_affect_strip(obj, gsn_dancingsword);
        object_affect_strip(obj, gsn_hoveringshield);
	}

    if (arg3[0]!='\0')
        cont = get_obj_carry(victim,arg3,ch);
    
    if (cont)
    {
        if (cont->item_type != ITEM_CONTAINER)
        {
            send_to_char("That's not a container.\n\r",ch);
            return;
        }

        if (IS_SET(cont->value[1], CONT_CLOSED))
        {
            act("$d is closed.", ch, NULL, cont->short_descr, TO_CHAR);
            return;
        }

        if (obj->item_type == ITEM_CONTAINER)
        {
            send_to_char("You can't place containers inside of containers.\n\r",ch);
            return;
        }
    
        if ((get_obj_weight(obj) + get_weight_contents(*cont)) > (cont->value[0] * 10) || (get_obj_weight(obj) > (cont->value[3] * 10)))
        {
            send_to_char("It won't fit.\n\r",ch);
            return;
        }
    
        if (obj->pIndexData->limit > 0 && cont->pIndexData->vnum != OBJ_VNUM_RAIDER_BELT 
        && cont->pIndexData->vnum != OBJ_VNUM_DISC && (cont->wear_flags & ITEM_TAKE))
        {
            send_to_char("That item is too powerful to place in such a container.\n\r",ch);
            return;
        }

        if (cont->pIndexData->vnum == OBJ_VNUM_RAIDER_BELT && cont->contains != NULL)
        {
            act("$p isn't big enough for more than one item.", ch, cont, NULL, TO_CHAR);
            return;
        }
    }
    else
    {
        if (!IS_NPC(victim) && !IS_IMMORTAL(victim)
          && (get_cur_capacity(victim) + item_svalue[obj->size]) > get_max_capacity(victim))
    	{
	    act("$N lacks the room to carry that.", ch, NULL, victim, TO_CHAR);
	    return;
    	}

    	if ( victim->carry_number + 1  > static_cast<int>(Encumbrance::CarryCountCapacity(*victim)))
    	{
	    act( "$N has $S hands full.", ch, NULL, victim, TO_CHAR );
	    return;
    	}

    	if (get_carry_weight(*victim) + get_obj_weight(obj) > static_cast<int>(Encumbrance::CarryWeightCapacity(*victim)))
    	{
	    act( "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR );
	    return;
    	}
    }

    if ( !can_see_obj( victim, obj ) )
	chance += 10;

    obj_from_char( obj );
    if (cont)
	obj_to_obj(obj,cont);
    else
	obj_to_char( obj, victim );

    if (!IS_NPC(victim) && BIT_GET(victim->pcdata->traits, TRAIT_STREETWISE))
	chance -= 10;

    if (cont && cont->worn_on)
	chance /= 3;

    if (chance < number_percent())
    {
      act( "$n tries to plant $p on $N.", ch, obj, victim, TO_NOTVICT );
      act( "$n plants $p on you.",   ch, obj, victim, TO_VICT    );
      act( "You plant $p on $N.", ch, obj, victim, TO_CHAR    );
      check_improve(ch,victim,gsn_plant,FALSE,3);
      mprog_give_trigger( victim, ch, obj );
      oprog_give_trigger( obj, ch, obj );
      return;
    }

    act( "You plant $p on $N.", ch, obj, victim, TO_CHAR);
    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	if (check_detectstealth(vch,ch))
	    if (victim == vch)
		act("$n planted $p on you.", ch, obj, vch, TO_VICT);
	    else
	    {
		char buf[MAX_STRING_LENGTH];
		sprintf(buf,"%s planted %s on %s.\n\r",PERS(ch,vch),obj->short_descr,PERS(victim,vch));
		send_to_char(buf,vch);
	    }
    check_improve(ch,victim,gsn_plant,FALSE,3);
    mprog_give_trigger( victim, ch, obj );
    oprog_give_trigger( obj, ch, obj );
    return;
}

void do_balance( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *teller;
    bool tFound = FALSE;
    char buf[MAX_STRING_LENGTH];

    if (!ch->in_room)
	return;

    for (teller = ch->in_room->people; teller; teller = teller->next_in_room)
	if (IS_NPC(teller) && IS_SET(teller->nact, ACT_BANK))
	{
	    tFound = TRUE;
	    break;
	}

    if (!tFound)
    {
	send_to_char("You see nowhere nearby to check your bank balance.\n\r", ch);
	return;
    }

    if (can_see(ch, teller) && !IS_NPC(ch))
    {
	sprintf(buf, "%s You have %s in your account.", ch->name, coins_to_str(ch->bank));
	do_tell(teller, buf);
    }
    else
    {
	sprintf(buf, "You have %s in your account.\n\r", coins_to_str(ch->bank));
	send_to_char(buf, ch);
    }

    return;
}

void do_withdrawal( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *teller;
    bool tFound = FALSE, wAll = FALSE;
    long amount = 0;
    float wvalue, tvalue;
    int ctype = -1, i;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];

    if (ch->position == POS_FIGHTING)
    {
	do_withdraw(ch, argument);
	return;
    }

    if (!ch->in_room)
	return;

    for (teller = ch->in_room->people; teller; teller = teller->next_in_room)
	if (IS_NPC(teller) && IS_SET(teller->nact, ACT_BANK))
	{
	    tFound = TRUE;
	    break;
	}
  
    if (!tFound)
    {
	send_to_char("You see nowhere nearby to make a withdrawal.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "all"))
    {
	wAll = TRUE;
	wvalue = coins_to_value(ch->bank);
    }
    else if (arg[0] == '\0' || argument[0] == '\0' || !is_number(arg))
    {
	send_to_char("Syntax: withdrawal <amount> <coin type>\n\r", ch);
	send_to_char("    or: withdrawal all\n\r", ch);
	return;
    }
    else
    {
        amount = atoi(arg);

	if (amount <= 0)
	{
	    send_to_char("How would you do that?\n\r", ch);
	    return;
	}

    	for (i = 0; i < MAX_COIN; i++)
	    if (!str_cmp(argument, coin_table[i].name))
	    {
		ctype = i;
		break;
	    }

	if (ctype == -1)
	{
	    send_to_char("Invalid coin type.\n\r", ch);
	    send_to_char("Valid coin types are:", ch);

	    for (i = 0; i < MAX_COIN; i++)
	    {
		send_to_char(" ", ch);
		send_to_char(coin_table[i].name, ch);
	    }

	    send_to_char("\n\r", ch);
	    return;
	}

	wvalue = amount * coin_table[ctype].value;
    }

    tvalue = coins_to_value(ch->bank);

    if (wvalue > tvalue)
    {
	if (can_see(ch, teller) && !IS_NPC(ch))
	{
	    sprintf(buf, "%s I'm sorry%s you do not seem to have that much in your account.", ch->name, ((ch->sex == SEX_MALE) ? " sir," : (ch->sex == SEX_FEMALE) ? " miss," : ","));
	    do_tell(teller, buf);
	}
	else
	    send_to_char("You do not have that much money saved in the bank.\n\r", ch);

	return;
    }

    if (can_see(ch, teller))
    {
	if (wAll)
	    sprintf(buf, "%s hands you %s.\n\r", PERS(teller, ch), coins_to_str(ch->bank));
	else
	    sprintf(buf, "%s hands you %ld %s.\n\r", PERS(teller, ch), amount, coin_table[ctype].name);

	buf[0] = UPPER(buf[0]);
	send_to_char(buf, ch);
	act("$N hands $n some coins.", ch, NULL, teller, TO_ROOM);
    }
    else
    {
	if (wAll)
	    sprintf(buf, "You withdraw %s.\n\r", coins_to_str(ch->bank));
	else
	    sprintf(buf, "You withdraw %ld %s.\n\r", amount, coin_table[ctype].name);

	send_to_char(buf, ch);
	act("$n withdraws some coins.", ch, NULL, NULL, TO_ROOM);
    }

    if (wAll)
	inc_player_coins(ch, ch->bank);
    else
	coins_to_char(ch, amount, ctype);

    memcpy(ch->bank, value_to_coins(tvalue - wvalue, FALSE), sizeof(long) * MAX_COIN);

    return;
}

extern char* ctos (long *coins, bool full_str);

DO_FUNC( do_convert )
{
    CHAR_DATA *teller = NULL;
    bool tFound = FALSE, dAll = FALSE;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    long amount = 0;
    float tvalue;
    int i, ctype = -1;

    if (!ch->in_room)
	return;

    for (teller = ch->in_room->people; teller; teller = teller->next_in_room)
	if (IS_NPC(teller) && IS_SET(teller->nact, ACT_BANK))
	{
	    tFound = TRUE;
	    break;
	}

    if (!tFound)
    {
	send_to_char("You see nowhere nearby to deposit your coins.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "all"))
	dAll = TRUE;
    else if (!is_number(arg))
    {
	if (can_see(ch, teller))
	    act("$N gives you a curious look.", ch, NULL, teller, TO_CHAR);
	else
	    send_to_char("You can't convert that.\n\r", ch);
	return;
    }

    if (!dAll)
    {
        amount = atoi(arg);

        if (amount <= 0)
        {
	    send_to_char("How would you do that?\n\r", ch);
	    return;
        }
    }

    argument = one_argument(argument, arg);
    int toType = -1;
    if (arg[0] != '\0')
    {
        ctype = coin_lookup(arg);
	    if (ctype == -1)
	    {
	        send_to_char("Invalid coin type.\n\r", ch);
    	    dAll = FALSE;  // a little hack never hurt anyone
	    }

        if (argument[0] != '\0')
        {
            toType = coin_lookup(argument);
            if (toType == -1)
            {
                send_to_char("Invalid coin type.\n", ch);
                ctype = -1;
                dAll = FALSE;
            }
        }
    }
    else if (!dAll)
	    send_to_char("Syntax: convert <amount> <from coin type> [to coin type]\n\r", ch);

    if (ctype == -1 && !dAll)
    {
        send_to_char("Valid coin types are:", ch);

        for (i = 0; i < MAX_COIN; i++)
        {
	        send_to_char(" ", ch);
    	    send_to_char(coin_table[i].name, ch);
	    }

    	send_to_char("\n\r", ch);
	    return;
    }

    if (dAll)
    {
	    tvalue = coins_to_value(ch->coins);
    	for (i = 0; i < MAX_COIN; i++)
	    {
	        amount = ch->coins[i];
    	    coins_from_char(ch, amount, i);
	    }
    }
    else
    {
    	if (amount > ch->coins[ctype])
	    {
	        sprintf(buf, "You don't have that much %s.\n\r", coin_table[ctype].name);
    	    send_to_char(buf, ch);
	        return;
	    }

	    tvalue = coin_table[ctype].value * amount;
    	coins_from_char(ch, amount, ctype);
    }

    send_to_char("You convert some coins.\n\r", ch);
    sprintf(buf, "$n converts some %s.", dAll ? "coins" : coin_table[ctype].name);
    act(buf, ch, NULL, NULL, TO_ROOM);
    
    if (dAll || toType == -1)
    {
        sprintf(buf, "You receive %s.\n\r", value_to_str(tvalue));
        send_to_char(buf, ch);
        inc_player_coins(ch, value_to_coins(tvalue, FALSE));    
        return;
    }

    // Converting to a specific coin
    int toCoins = tvalue / coin_table[toType].value;
    int leftoverValue = tvalue - (toCoins * coin_table[toType].value);
    int fromCoins = leftoverValue / coin_table[ctype].value;
    leftoverValue = leftoverValue - (fromCoins * coin_table[ctype].value);
    coins_to_char(ch, toCoins, toType);
    coins_to_char(ch, fromCoins, ctype);
    long * coinsInfo = value_to_coins(leftoverValue, FALSE);
    inc_player_coins(ch, coinsInfo);

    coinsInfo[toType] += toCoins;
    coinsInfo[ctype] += fromCoins;
    sprintf(buf, "You receive %s.\n", ctos(coinsInfo, TRUE));
    send_to_char(buf, ch);
}

void do_deposit( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *teller;
    bool tFound = FALSE, dAll = FALSE;
    long amount = 0;
    float tvalue = 0, interest;
    int i, ctype = -1;
    char buf[MAX_STRING_LENGTH], arg[MAX_INPUT_LENGTH];

    if (!ch->in_room)
	return;

    for (teller = ch->in_room->people; teller; teller = teller->next_in_room)
	if (IS_NPC(teller) && IS_SET(teller->nact, ACT_BANK))
	{
	    tFound = TRUE;
	    break;
	}

    if (!tFound)
    {
	send_to_char("You see nowhere nearby to deposit your coins.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "all"))
	dAll = TRUE;
    else if (!is_number(arg))
    {
	if (can_see(ch, teller))
	    act("$N gives you a curious look.", ch, NULL, teller, TO_CHAR);
	else
	    send_to_char("You can't deposit that.\n\r", ch);
	return;
    }

    if (!dAll)
    {
        amount = atoi(arg);

        if (amount <= 0)
        {
	    send_to_char("How would you do that?\n\r", ch);
	    return;
        }
    }

    if (argument[0] != '\0')
    {
	for (i = 0; i < MAX_COIN; i++)
	    if (!str_cmp(argument, coin_table[i].name))
	    {
		ctype = i;
		break;
	    }

	if (ctype == -1)
	{
	    send_to_char("Invalid coin type.\n\r", ch);
	    dAll = FALSE;  // a little hack never hurt anyone
	}
    }
    else if (!dAll)
	send_to_char("Syntax: deposit <amount> <coin type>\n\r", ch);

    if (ctype == -1 && !dAll)
    {
        send_to_char("Valid coin types are:", ch);

        for (i = 0; i < MAX_COIN; i++)
        {
	    send_to_char(" ", ch);
	    send_to_char(coin_table[i].name, ch);
	}

	send_to_char("\n\r", ch);
	return;
    }

    if (dAll)
    {
	tvalue = coins_to_value(ch->coins);
	interest = UMAX(1, (tvalue / 20));
	for (i = 0; i < MAX_COIN; i++)
	{
	    amount = ch->coins[i];
	    coins_from_char(ch, amount, i);
	    ch->bank[i] += amount;
	}
    }
    else
    {
	if (amount > ch->coins[ctype])
	{
	    sprintf(buf, "You don't have that much %s.\n\r", coin_table[ctype].name);
	    send_to_char(buf, ch);
	    return;
	}

	interest = UMAX(1, coin_table[ctype].value * amount * 1 / 20);
	coins_from_char(ch, amount, ctype);
	ch->bank[ctype] += amount;
    }

    memcpy(ch->bank, convert_coins(ch->bank), sizeof(long) * MAX_COIN);

    if (can_see(ch, teller))
    {
	if (dAll)
	    sprintf(buf, "You hand %s %s.\n\r", teller->short_descr, value_to_str(tvalue));
	else
	    sprintf(buf, "You hand %s %ld %s.\n\r", teller->short_descr, amount, coin_table[ctype].name);
	send_to_char(buf, ch);
	sprintf(buf, "$n hands $N some %s.", dAll ? "coins" : coin_table[ctype].name);
	act(buf, ch, NULL, teller, TO_ROOM);
    }
    else
    {
	if (dAll)
	    sprintf(buf, "You deposit %s.\n\r", coins_to_str(value_to_coins(tvalue, FALSE)));
	else
	    sprintf(buf, "You deposit %ld %s.\n\r", amount, coin_table[ctype].name);
	send_to_char(buf, ch);
	sprintf(buf, "$n deposits some %s.", dAll ? "coins" : coin_table[ctype].name);
	act(buf, ch, NULL, NULL, TO_ROOM);
    }

    sprintf(buf, "The bank removes %s from your account for their service.\n\r", coins_to_str(value_to_coins(interest, FALSE)));
    send_to_char(buf, ch);

    tvalue = coins_to_value(ch->bank);
    tvalue -= interest;
    memcpy(ch->bank, value_to_coins(tvalue, FALSE), sizeof(long) * MAX_COIN);

    return;
}

void do_give( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA  *obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Give what to whom?\n\r", ch );
	return;
    }

    if (IS_PAFFECTED(ch, AFF_VOIDWALK))
    {
	send_to_char("You can't reach into the material plane from the void.\n\r", ch);
	return;
    }

    if ( is_number( arg1 ) )
    {
	/* 'give NNNN coins victim' */
	int amount;
        int ctype = -1;

	ctype = coin_lookup(arg2);

	amount   = atoi(arg1);

	if ( amount <= 0 || ctype == -1)
	{
	    send_to_char( "Sorry, you can't do that.\n\r", ch );
	    return;
	}

	argument = one_argument( argument, arg2 );
	if ( arg2[0] == '\0' )
	{
	    send_to_char( "Give what to whom?\n\r", ch );
	    return;
	}

	if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "They aren't here.\n\r", ch );
	    return;
	}
        if (IS_PAFFECTED(victim, AFF_VOIDWALK))
        {
    	    send_to_char("You cannot reach into the void from the material plane.\n\r", ch);
	    return;
        }
	if (is_affected(victim, gsn_astralprojection))
	{
		send_to_char("As a spirit, they would be unable to hold it.\n\r", ch);
		return;
	}

	    if ( victim->demontrack != NULL && victim->demonstate != 0 && ch != victim->demontrack )
	    {
		send_to_char( "The mystic protections trapping the demon in the protective circle prevent you from approaching close enough.\n\r", ch);
		return;
	    }

	    if (ch->coins[ctype] < amount)
	    {
	        send_to_char( "You haven't got that much.\n\r", ch );
	        return;
	    }

	    coins_from_char(ch, amount, ctype);
	    coins_to_char(victim, amount, ctype);

	    sprintf(buf,"$n gives you %d %s.",amount, coin_table[ctype].name);
	    act( buf, ch, NULL, victim, TO_VICT    );
	    act( "$n gives $N some coins.",  ch, NULL, victim, TO_NOTVICT );
	    sprintf(buf,"You give $N %d %s.",amount, coin_table[ctype].name);
	    act( buf, ch, NULL, victim, TO_CHAR    );

            mprog_bribe_trigger( victim, ch, (int) (amount * coin_table[ctype].value));
	    return;
    }

    if ( ( obj = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if (obj->worn_on || obj == ch->nocked)
    {
	send_to_char( "You must remove it first.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if (IS_PAFFECTED(victim, AFF_VOIDWALK))
    {
        send_to_char("You cannot reach into the void from the material plane.\n\r", ch);
	return;
    }
    if (is_affected(victim, gsn_astralprojection))
    {
	send_to_char("As a spirit, they would be unable to hold it.\n\r", ch);
	return;
    }
		

    if ( victim->demontrack != NULL && victim->demonstate != 0 && ch != victim->demontrack )
    {
	send_to_char( "The mystic protections trapping the demon in the protective circle prevent you from approaching close enough.\n\r", ch);
	return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
	send_to_char( "You can't let go of it.\n\r", ch );
	return;
    }

    if (obj_is_affected(obj, gsn_dancingsword) || obj_is_affected(obj, gsn_hoveringshield))
    {
        act("As you go to release $p, it suddenly feels very heavy again.", ch, obj, NULL, TO_CHAR);
        act("As $n goes to release $p, $e slumps as it regains its weight.", ch, obj, NULL, TO_ROOM);
        object_affect_strip(obj, gsn_dancingsword);
        object_affect_strip(obj, gsn_hoveringshield);
    }

    if (!IS_NPC(victim) && !IS_IMMORTAL(victim)
     && (get_cur_capacity(victim) + item_svalue[obj->size]) > get_max_capacity(victim))
    {
	act("$N lacks the room to carry that.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if ( victim->carry_number + 1 > static_cast<int>(Encumbrance::CarryCountCapacity(*victim)))
    {
	act( "You offer $N $p, but $E has $S hands full.", ch, obj, victim, TO_CHAR );
	act( "$n offers you $p, but your hands are full.", ch, obj, victim, TO_VICT);
	return;
    }

    if (get_carry_weight(*victim) + get_obj_weight(obj) > static_cast<int>(Encumbrance::CarryWeightCapacity(*victim)))
    {
	act( "You offer $N $p, but $E cannot carry that much weight.", ch, obj, victim, TO_CHAR );
	act( "$n offers you $p, but you cannot carry that much weight.", ch, obj, victim, TO_VICT);
	return;
    }

    if ( !can_see_obj( victim, obj ) )
    {
	act( "$N can't see it.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if (!IS_NPC(victim) && IS_SET(victim->nact, PLR_NOACCEPT))
    {
	act("You offer $p to $N, but $E does not accept it.", ch, obj, victim, TO_CHAR);
	act("$n offers you $p, but you do not accept it.", ch, obj, victim, TO_VICT);
	act("$n offers $p to $N, but $E does not accept it.", ch, obj, victim, TO_NOTVICT);
	return;
    }

    obj_from_char( obj );
    obj_to_char( obj, victim );
    act( "$n gives $p to $N.", ch, obj, victim, TO_NOTVICT );
    act( "$n gives you $p.",   ch, obj, victim, TO_VICT    );
    act( "You give $p to $N.", ch, obj, victim, TO_CHAR    );

/*    if (!IS_NPC(victim))
    {
	victim->objlevels += obj->level;
	if (victim->objlevels > victim->level * 10
	  && victim->objlevels > 100
	  && obj->level > 1)
	{
	    sprintf(buf, "%s received %s [%d levels]. Total levels: %ld.",
		  victim->name, obj->short_descr, obj->level, victim->objlevels);
	    bug(buf, 0);

	    if (ch)
		sprintf(buf, "Overlimit: %s received %s [level %d] from %s in room %d. Total levels: %ld.", victim->name, obj->short_descr, obj->level, IS_NPC(ch) ? ch->short_descr : ch->name, victim->in_room ? victim->in_room->vnum : -1, victim->objlevels);
	    else
		sprintf(buf, "Overlimit: %s received %s [level %d] in room %d. Total levels: %ld.", victim->name, obj->short_descr, obj->level, victim->in_room ? victim->in_room->vnum : -1, victim->objlevels);

	    log_string(buf);
    	    wiznet(buf,NULL,NULL,WIZ_CHEATING,0,0);
	}
    }*/
    if (IS_NPC(victim) && !mprog_give_trigger(victim, ch, obj)
     && victim->pIndexData->pShop != NULL)
    {
	act("$N tells you 'Sorry, you'll have to sell that.'", ch, NULL, victim, TO_CHAR);
	ch->reply = victim;
	obj_from_char( obj );
	obj_to_char( obj, ch );
	act( "$n returns $p to $N.", victim, obj, ch, TO_NOTVICT );
	act( "$n returns you $p.",   victim, obj, ch, TO_VICT    );
	act( "You return $p to $N.", victim, obj, ch, TO_CHAR    );	
	return;
    }
    
    oprog_give_trigger( obj, ch, obj );
    return;
}



void do_fill( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *fountain;
    bool found;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Fill what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    found = FALSE;
    for ( fountain = ch->in_room->contents; fountain != NULL;
	fountain = fountain->next_content )
    {
	if ( fountain->item_type == ITEM_FOUNTAIN )
	{
	    found = TRUE;
	    break;
	}
    }

    if ( !found )
    {
	send_to_char( "There is no fountain here!\n\r", ch );
	return;
    }
    if (fountain->value[3])
    {
	act("$p is empty.",ch,fountain,NULL,TO_CHAR);
	return;
    }
    if ( obj->item_type != ITEM_DRINK_CON )
    {
	send_to_char( "You can't fill that.\n\r", ch );
	return;
    }

    if ( obj->value[1] != 0 && obj->value[2] != fountain->value[2] )
    {
	send_to_char( "There is already another liquid in it.\n\r", ch );
	return;
    }

    if ( obj->value[1] >= obj->value[0] )
    {
	send_to_char( "Your container is full.\n\r", ch );
	return;
    }

    object_affect_strip(obj, gsn_boil);
    object_affect_strip(obj, gsn_dilute);
    object_affect_strip(obj, gsn_distill);
    object_affect_strip(obj, gsn_draughtoftheseas);
    
    sprintf(buf,"You fill $p with %s from $P.",
	liq_table[fountain->value[2]].liq_name);
    act( buf, ch, obj,fountain, TO_CHAR );
    sprintf(buf,"$n fills $p with %s from $P.",
	liq_table[fountain->value[2]].liq_name);
    act(buf,ch,obj,fountain,TO_ROOM);
    obj->value[2] = fountain->value[2];
    obj->value[1] = obj->value[0];
}

void do_pour (CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH],buf[MAX_STRING_LENGTH];
    OBJ_DATA *out, *in;
    CHAR_DATA *vch = NULL;
    int amount;

    argument = one_argument(argument,arg);
    
    if (arg[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Pour what into what?\n\r",ch);
	return;
    }
    

    if ((out = get_obj_carry(ch,arg, ch)) == NULL)
    {
	send_to_char("You don't have that item.\n\r",ch);
	return;
    }

    if (out->item_type != ITEM_DRINK_CON)
    {
	send_to_char("That's not a drink container.\n\r",ch);
	return;
    }

    if (!str_cmp(argument,"out"))
    {
	if (out->value[1] == 0)
	{
	    send_to_char("It's already empty.\n\r",ch);
	    return;
	}

	object_affect_strip(out, gsn_boil);
	object_affect_strip(out, gsn_dilute);
	object_affect_strip(out, gsn_distill);
    object_affect_strip(out, gsn_draughtoftheseas);
	
    out->value[1] = 0;
	out->value[3] = 0;
	sprintf(buf,"You invert $p, spilling %s all over the ground.",
		liq_table[out->value[2]].liq_name);
	act(buf,ch,out,NULL,TO_CHAR);
	
	sprintf(buf,"$n inverts $p, spilling %s all over the ground.",
		liq_table[out->value[2]].liq_name);
	act(buf,ch,out,NULL,TO_ROOM);
	return;
    }

    if ((in = get_obj_here(ch,argument)) == NULL)
    {
	vch = get_char_room(ch,argument);

	if (vch == NULL)
	{
	    send_to_char("Pour into what?\n\r",ch);
	    return;
	}

	in = get_eq_char(vch,WEAR_HOLD);

	if (in == NULL)
	{
	    send_to_char("They aren't holding anything.",ch);
 	    return;
	}
    }

    if (in->item_type != ITEM_DRINK_CON)
    {
	send_to_char("You can only pour into other drink containers.\n\r",ch);
	return;
    }
    
    if (in == out)
    {
	send_to_char("You cannot change the laws of physics!\n\r",ch);
	return;
    }

    if (in->value[1] != 0 && in->value[2] != out->value[2])
    {
	send_to_char("They don't hold the same liquid.\n\r",ch);
	return;
    }

    if (out->value[1] == 0)
    {
	act("There's nothing in $p to pour.",ch,out,NULL,TO_CHAR);
	return;
    }

    if (in->value[1] >= in->value[0])
    {
	act("$p is already filled to the top.",ch,in,NULL,TO_CHAR);
	return;
    }

    amount = UMIN(out->value[1],in->value[0] - in->value[1]);

    in->value[1] += amount;
    out->value[1] -= amount;
    in->value[2] = out->value[2];
    
    object_affect_strip(in, gsn_boil);
    object_affect_strip(in, gsn_dilute);
    object_affect_strip(in, gsn_distill);
    object_affect_strip(in, gsn_draughtoftheseas);
    object_affect_strip(out, gsn_boil);
    object_affect_strip(out, gsn_dilute);
    object_affect_strip(out, gsn_distill);
    object_affect_strip(out, gsn_draughtoftheseas);
    
    if (vch == NULL)
    {
    	sprintf(buf,"You pour %s from $p into $P.",
	    liq_table[out->value[2]].liq_name);
    	act(buf,ch,out,in,TO_CHAR);
    	sprintf(buf,"$n pours %s from $p into $P.",
	    liq_table[out->value[2]].liq_name);
    	act(buf,ch,out,in,TO_ROOM);
    }
    else
    {
        sprintf(buf,"You pour some %s for $N.",
            liq_table[out->value[2]].liq_name);
        act(buf,ch,NULL,vch,TO_CHAR);
	sprintf(buf,"$n pours you some %s.",
	    liq_table[out->value[2]].liq_name);
	act(buf,ch,NULL,vch,TO_VICT);
        sprintf(buf,"$n pours some %s for $N.",
            liq_table[out->value[2]].liq_name);
        act(buf,ch,NULL,vch,TO_NOTVICT);
	
    }

    if ((out->value[1] == 0) && (out->value[4] != 0))
    {
	send_to_char("You discard the empty container.\n\r", ch);
	extract_obj(out);
    }

    return;
}

void do_drink( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int amount;
    int liquid;

    one_argument( argument, arg );

    if (is_affected(ch, gsn_astralprojection))
    {
	send_to_char("You can't drink anything when you're in the Astral Plane.\n\r", ch);
	return;
    }

    if ( arg[0] == '\0' )
    {
        for ( obj = ch->in_room->contents; obj; obj = obj->next_content )
        {
            if ( obj->item_type == ITEM_FOUNTAIN )
            break;
        }

        if ( obj == NULL )
        {
            send_to_char( "Drink what?\n\r", ch );
            return;
        }
    }
    else
    {
        if ( ( obj = get_obj_here( ch, arg ) ) == NULL )
        {
            send_to_char( "You can't find it.\n\r", ch );
            return;
        }
    }

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10 )
    {
	send_to_char( "You fail to reach your mouth.  *Hic*\n\r", ch );
	return;
    }

    if (check_bloodofthevizier_drink(*ch, *obj))
        return;

    switch ( obj->item_type )
    {
    default:
	send_to_char( "You can't drink from that.\n\r", ch );
	return;

    case ITEM_FOUNTAIN:
        if ( ( liquid = obj->value[2] )  < 0 )
        {
            bug( "Do_drink: bad liquid number %d.", liquid );
            liquid = obj->value[2] = 0;
        }
	if ( obj->value[3] )
	{
	    act("$p is empty.",ch,obj,NULL,TO_CHAR);
	    return;
	}
	amount = liq_table[liquid].liq_affect[4] * 3;
	break;

    case ITEM_DRINK_CON:
	if ( obj->value[1] <= 0 )
	{
	    send_to_char( "It is already empty.\n\r", ch );
	    return;
	}

	if ( ( liquid = obj->value[2] )  < 0 )
	{
	    bug( "Do_drink: bad liquid number %d.", liquid );
	    liquid = obj->value[2] = 0;
	}

        amount = liq_table[liquid].liq_affect[4];
        amount = UMIN(amount, obj->value[1]);
	break;
     }

    object_affect_strip(obj, gsn_boil);
    object_affect_strip(obj, gsn_dilute);
    object_affect_strip(obj, gsn_distill);
    act( "You drink $T from $p.", ch, obj, liq_table[liquid].liq_name, TO_CHAR );
    act( "$n drinks $T from $p.", ch, obj, liq_table[liquid].liq_name, TO_ROOM );

    int drunkBase(liq_table[liquid].liq_affect[COND_DRUNK]);
    if (number_percent() <= get_skill(ch, gsn_revenant))
    {
        if (!str_cmp(liq_table[liquid].liq_name, "blood"))
            drunkBase = 120;
        else
            drunkBase = 0;
    }
    gain_condition( ch, COND_DRUNK,  amount * drunkBase / 36 );
    gain_condition( ch, COND_THIRST, amount * liq_table[liquid].liq_affect[COND_THIRST] / 10 );

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]  > 10 )
    	send_to_char( "You feel drunk.\n\r", ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_FULL]   > 40 )
	    send_to_char( "You are full.\n\r", ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] > 40 )
    	send_to_char( "Your thirst is quenched.\n\r", ch );

    if (((obj->value[3] != 0) || obj_is_affected(obj, gsn_poison)) && ch->demontrack == NULL)
    {
        /* The drink was poisoned ! */
        AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

        if (is_affected(ch, gsn_protectionfrompoison))
            send_to_char("You choke a moment, but your protection quickly flushes the poison from your system.\n\r", ch);
        else if (number_percent() <= get_skill(ch, gsn_detoxify))
        {
            check_improve(ch, NULL, gsn_detoxify, true, 4);
            send_to_char("You choke as you ingest poison, but your body quickly flushes the toxins from your system.\n", ch);
            return;
        }
        else
        {
            check_improve(ch, NULL, gsn_detoxify, false, 4);

            act( "$n chokes and gags.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "You choke and gag.\n\r", ch );
            af.where     = TO_AFFECTS;
            af.type      = gsn_poison;
            af.level     = number_fuzzy(amount); 
            af.duration  = 8;
            af.location  = APPLY_NONE;
            af.modifier  = 0;
            af.bitvector = AFF_POISON;
            affect_join( ch, &af );
        }
    }

    // Check for draught of the seas
    AFFECT_DATA * draught(get_obj_affect(obj, gsn_draughtoftheseas));
    if (draught != NULL && ch->hit < ch->max_hit)
    {
        send_to_char("You feel the power of the healing draught fill you.\n", ch);
        int healAmount(number_range(draught->level, draught->level * 2));
        if (draught->modifier == draught->duration) healAmount /= 3;
        ch->hit = UMIN(ch->max_hit, ch->hit + healAmount);
        draught->modifier = draught->duration;
    }

    // Check for wellspring
    if (obj->pIndexData->vnum == OBJ_VNUM_WELLSPRING && !is_affected(ch, gsn_wellspring))
    {
        AFFECT_DATA waf = {0};
        waf.where   = TO_AFFECTS;
        waf.type    = gsn_wellspring;
        waf.level   = obj->level;
        waf.duration = 2;
        affect_to_char(ch, &waf);

        send_to_char("You feel the pure waters spread through your body, washing away your weariness.\n", ch);
    }
 
    if (obj->value[0] > 0)
        obj->value[1] -= amount;

    if (obj->value[1] < 1 && obj->value[3] != 0)
	obj->value[3] = 0;

    if (ch && obj)
	oprog_drink_trigger(ch,obj);
    
    // Handle the case of empty containers
    if (obj->value[1] == 0)
    {
        if (obj->value[4] != 0)
        {
        	send_to_char("You discard the empty container.\n\r", ch);
        	extract_obj(obj);
        }
        else
            object_affect_strip(obj, gsn_draughtoftheseas);
    }
}

void do_eat( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int type;
    AFFECT_DATA *paf;
    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Eat what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if ( !IS_IMMORTAL(ch) && obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL )
    {
        send_to_char( "That's not edible.\n\r", ch );
        return;
    }

    switch ( obj->item_type )
    {

    case ITEM_FOOD:

        if ( !IS_NPC(ch) && !IS_IMMORTAL(ch) && ch->pcdata->condition[COND_HUNGER] > get_max_hunger(ch))
        {     
            if (IS_NAFFECTED(ch, AFF_ASHURMADNESS))
	    	send_to_char("Your stomach growls for MORE!\n\r", ch);
	    else
	    {
		send_to_char( "You are too full to eat more.\n\r", ch );
            	return;
	    }
        }
	if (IS_NAFFECTED(ch,AFF_ASHURMADNESS))
	{
	    act("You savor $p, but must find more delicous morsels!",ch,obj,NULL,TO_CHAR);
	    act("$n devours $p!.",ch, obj, NULL, TO_ROOM);
	}
	else
	{
	    act( "$n eats $p.", ch, obj, NULL, TO_ROOM );
	    act( "You eat $p.", ch, obj, NULL, TO_CHAR );
	}

	if ( !IS_NPC(ch) )
	{
	    int condition;
	    bool full = FALSE;
	    int amount;

	    condition = ch->pcdata->condition[COND_HUNGER];

	    if (ch->pcdata->condition[COND_HUNGER] >= (get_max_hunger(ch) * 19 / 20))
		full = TRUE;

	    if (obj->value[1] == 0)
		amount = 0;
	    else
		amount = obj->value[0] * 1000 / obj->value[1];

	    if (full)
		amount = UMIN(amount, (get_max_hunger(ch) * 11 / 10) - condition);
	    else
		amount = UMIN(amount, get_max_hunger(ch) - condition);

	    if (amount)
	        ch->pcdata->food_quality = ((ch->pcdata->food_quality * (condition + 1500)) + (obj->value[1] * amount)) / ((condition + 1500) + amount);

	    gain_condition( ch, COND_HUNGER, amount);

	    if ( condition == 0 && ch->pcdata->condition[COND_HUNGER] > 0 )
		send_to_char( "You are no longer hungry.\n\r", ch );
	    else if ( ch->pcdata->condition[COND_HUNGER] > get_max_hunger(ch))
	    {
		/* Raestral(06/05): prevent newbs from gorging, as their hunger does
		   not decrease at a normal rate, causing much stress. */
		if (ch->level < 6)
		{
		    ch->pcdata->condition[COND_HUNGER] = get_max_hunger(ch) - 1;
		    send_to_char( "You nearly gorge yourself.\n\r", ch);
		}
		else
		    send_to_char( "Ugh!  You have gorged yourself.\n\r", ch);
	    }
	    else if ( ch->pcdata->condition[COND_HUNGER] >= (get_max_hunger(ch) * 19 / 20))
		send_to_char( "You are full.\n\r", ch);
	    else if ( ch->pcdata->condition[COND_HUNGER] >= (get_max_hunger(ch) * 17 / 20))
		send_to_char( "You are nearly full.\n\r", ch );
	}

	if (((obj->value[3] != 0) || obj_is_affected(obj, gsn_poison)) && ch->demontrack == NULL && !is_affected(ch, gsn_poison))
	{
	    /* The food was poisoned! */
	    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

	    if (is_affected(ch, gsn_protectionfrompoison))
	        send_to_char("You choke a moment, but your protection quickly flushes the poison from your system.\n\r", ch);
    	else if (!IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_IRONSTOMACH))
		    send_to_char("You feel somewhat ill, but your iron stomach protects you from harm.\n\r", ch);
        else if (number_percent() <= get_skill(ch, gsn_detoxify))
        {
            check_improve(ch, NULL, gsn_detoxify, true, 4);
            send_to_char("You choke as you ingest poison, but your body quickly flushes the toxins from your system.\n", ch);
        }

	    else
        {
            check_improve(ch, NULL, gsn_detoxify, false, 4);
            act( "$n chokes and gags.", ch, 0, 0, TO_ROOM );
            send_to_char( "You choke and gag.\n\r", ch );

            af.where	 = TO_AFFECTS;
            af.type      = gsn_poison;
            /* * Unless I'm missing something, this is kinda nutty:
            af.level 	 = number_fuzzy(obj->value[0]);
            af.duration  = 2 * obj->value[0];
            */
            af.level = ch->level;
            af.duration = ch->level/10 + number_bits(2);
            af.location  = APPLY_NONE;
            af.modifier  = 0;
            af.bitvector = AFF_POISON;
            affect_join( ch, &af );
        }
	}
	for (type=0;poison_table[type].spell_fun;type++)
	    if ((paf=affect_find(obj->affected,*(poison_table[type].sn))) != NULL)
		break;
	if (poison_table[type].spell_fun)
	    if (paf)
		poison_table[type].spell_fun(ch,ch,paf->level,gsn_taint);
	    else
	    	poison_table[type].spell_fun(ch,ch,ch->level,gsn_taint);
	break;

    case ITEM_PILL:

	act( "$n eats $p.", ch, obj, NULL, TO_ROOM );
	act( "You eat $p.", ch, obj, NULL, TO_CHAR );

    if (!obj_is_affected(obj, gsn_illusion))
    {
	    if (obj_is_affected(obj, gsn_subtlepoison)) send_to_char("You feel as if something is subtly wrong.\n\r", ch);
    	obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
	    obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
    	obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );
    }

	if (ch->fighting)
	    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	break;
	
    default:	act( "$n eats $p.", ch, obj, NULL, TO_ROOM );
		act( "You eat $p.", ch, obj, NULL, TO_CHAR );
		break;
    }
    if (ch && obj)
	oprog_eat_trigger(ch,obj);
    extract_obj( obj );
    return;
}


bool remove_obj_slot(CHAR_DATA *ch, int slot, bool fReplace )
{
    return (remove_obj(ch, get_eq_char(ch, slot), fReplace));
}


/*
 * Remove an object.
 */
bool remove_obj( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace )
{
    bool wielded;
    OBJ_DATA *wield;
    OBJ_DATA *dualobj;
    int vers = get_skill(ch,gsn_versatility);

    if (!obj)
	return TRUE;

    if ( !fReplace )
	return FALSE;

    if ((ch->fighting != NULL)
     && !((CAN_WEAR(obj, ITEM_WEAR_SHIELD))
      || (CAN_WEAR(obj,ITEM_HOLD))
      || (CAN_WEAR(obj,ITEM_WIELD))))
    {
	send_to_char("You cannot undress while fighting.\n\r", ch);
	return FALSE;
    }

    // Handle floating disc
    if (obj->pIndexData->vnum == OBJ_VNUM_DISC)
    {
        if (obj->contains != NULL)
        {
            act("But the disc has $p on it! You should probably take it off first.", ch, obj->contains, NULL, TO_CHAR);
            return false;
        }
            
        act("You snap your fingers, and $p vanishes!", ch, obj, NULL, TO_CHAR);
        act("$n snaps $s fingers, and $p vanishes!", ch, obj, NULL, TO_ROOM);
        extract_obj(obj);
        return true;
    }

    if ( IS_SET(obj->extra_flags[0], ITEM_NOREMOVE) )
    {
    	if (obj->pIndexData->vnum == OBJ_VNUM_BLOOD_SIGIL)
	        act("Your blood sigil will heal in time.",ch,NULL,NULL,TO_CHAR);
    	else
	        act("You can't remove $p.", ch, obj, NULL, TO_CHAR );
    	return FALSE;
    }

    if (obj_is_affected(obj, gsn_dancingsword) || obj_is_affected(obj, gsn_hoveringshield))
    {
        act("$p won't come back down to you.", ch, obj, NULL, TO_CHAR);
        return FALSE;
    }

    if (obj->item_type == ITEM_WEAR_FEET && (race_table[ch->race].aff & AFF_SNEAK))
        send_to_char("You begin sneaking quietly again.\n\r",ch);


    if (IS_SET(obj->worn_on, WORN_WIELD))
	    wielded = TRUE;
    else
    	wielded = FALSE;

    if (IS_SET(obj->worn_on, WORN_SHIELD) && is_affected(ch, gsn_shieldcover))
    {
	    send_to_char("You stop covering your group with your shield.\n\r", ch);
    	act("$n stops covering $s group with $s shield.", ch, NULL, NULL, TO_ROOM); 
    	affect_strip(ch, gsn_shieldcover);
    }

    unequip_char( ch, obj );
    act( "$n stops using $p.", ch, obj, NULL, TO_ROOM );
    act( "You stop using $p.", ch, obj, NULL, TO_CHAR );
    oprog_remove_trigger(obj);

    if (wielded && (dualobj = get_eq_char(ch,WEAR_DUAL_WIELD)))	
    {
 	    unequip_char(ch,dualobj);
    	equip_char(ch,dualobj,WORN_WIELD);
    }

    wield = get_eq_char(ch,WEAR_WIELD);

    if (!wield || (wield->value[0] != WEAPON_SWORD))
    {
        if (vers > 0)
        {    
            if (number_percent() > vers)
            {
            if (unassume_form(ch))
                    send_to_char("You can't maintain your form without your sword.\n\r", ch);
            }
        }    
        else if (unassume_form(ch))
            send_to_char("You can't maintain your form without your sword.\n\r", ch);
        
    }

    if (get_eq_char(ch,WEAR_WIELD) == NULL && get_eq_char(ch, WEAR_DUAL_WIELD) == NULL)
    {
        if (unassume_form(ch))
	       send_to_char("You can't maintain your form without your sword.\n\r", ch);
    }

    return TRUE;
}


int hands_free(CHAR_DATA *ch)
{
    int handsfree = 2;
    OBJ_DATA *obj;

    if (is_affected(ch,gsn_tendrilgrowth))
	handsfree = 0;

    if ((obj = get_eq_char(ch, WEAR_WIELD)) != NULL)
    {
	if (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS))
	    if ((ch->size >= SIZE_LARGE) && (obj->value[0] != WEAPON_SPEAR) && (obj->value[0] != WEAPON_POLEARM) && (obj->value[0] != WEAPON_STAFF) 
	      && !IS_PAFFECTED(ch,AFF_TWOHAND))
		handsfree -= 1;
	    else
		handsfree -= 2;
	else
	    handsfree -= 1;
    }

    if (IS_OAFFECTED(ch, AFF_ONEHANDED))
	handsfree -= 1;
 
    //if (get_eq_char(ch, WEAR_LIGHT))
	//handsfree -= 1;

    if (get_eq_char(ch, WEAR_DUAL_WIELD))
	handsfree -= 1;

    if (get_eq_char(ch, WEAR_HOLD))
	handsfree -= 1;

    if (get_eq_char(ch, WEAR_SHIELD))
	handsfree -= 1;

    return handsfree;
}

bool can_use_dual(CHAR_DATA *ch, OBJ_DATA *weapon, OBJ_DATA *dual)
{
    if (IS_PAFFECTED(ch,AFF_TWOHAND))
	return FALSE;
    if (is_affected(ch,gsn_tendrilgrowth))
	return FALSE;
// usually if ch is fighting and gets mainhand disarmed
    if (weapon == NULL && dual != NULL)
    {
	if (ch->class_num == global_int_class_swordmaster 
	  && dual->value[0] == WEAPON_SWORD)
	    return TRUE;
    }
// Same as before, really.
    else if (weapon != NULL && dual == NULL) 
	return TRUE;
// We already have two weapons, let's see how they compare
    else if (weapon != NULL && dual != NULL)
        if ((weapon->value[0] == WEAPON_DAGGER &&
           dual->value[0] == WEAPON_DAGGER)
     	|| (ch->class_num == global_int_class_swordmaster &&
           weapon->value[0] == WEAPON_SWORD &&
           dual->value[0] == WEAPON_SWORD)
     	|| (get_obj_weight(dual) <= (get_obj_weight(weapon) * ((IS_NPC(ch) || !BIT_GET(ch->pcdata->traits, TRAIT_AMBIDEXTROUS)) ? 8 : 10) / 12) &&
	  (get_obj_weight(dual) <= (str_app[get_curr_stat(ch, STAT_STR)].wield * ((IS_NPC(ch) || !BIT_GET(ch->pcdata->traits, TRAIT_AMBIDEXTROUS)) ? 5 : 7)))))
        {
            return TRUE;
        }
    return FALSE;
}

/* Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 *
 * New strategy.  Build a field of wear locations, call equip_char at end of
 * procedure.  Change made for multi-slot items.  -- Aeolis
 */
void wear_obj( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace, bool mobReset, bool twohand )
{
    OBJ_DATA *weapon;
    int wear_slots = 0, sn = 0, skill;
   
    if ( CAN_WEAR(obj,ITEM_WEAR_FLOAT) )
    {
        if (obj_is_affected(obj, gsn_dancingsword) && ch->class_num != global_int_class_airscholar)
        {
            act("$n releases $p, but it drops to the ground.", ch, obj, NULL, TO_ROOM);
            act("You release $p, but it drops to the ground.", ch, obj, NULL, TO_CHAR);
            object_affect_strip(obj, gsn_dancingsword);
            obj_from_char(obj);
            obj_to_room(obj, ch->in_room);
            return;
        }

        if (!remove_obj_slot(ch, WEAR_FLOAT, fReplace))
            return;

        SET_BIT(wear_slots, WORN_FLOAT);
    }

    if  (obj->item_type == ITEM_LIGHT )
    {
        if (!remove_obj_slot(ch, WEAR_LIGHT, fReplace))
            return;

        SET_BIT(wear_slots, WORN_LIGHT);
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_FINGER ) )
    {
 	if (ch->fighting != NULL)
	{
	    send_to_char("You cannot dress while fighting.\n\r", ch);
	    return;
	}


	if ( get_eq_char( ch, WEAR_FINGER_L ) != NULL
	&&   get_eq_char( ch, WEAR_FINGER_R ) != NULL
	&&   !remove_obj_slot( ch, WEAR_FINGER_L, fReplace )
	&&   !remove_obj_slot( ch, WEAR_FINGER_R, fReplace ) )
	    return;

	if ( get_eq_char( ch, WEAR_FINGER_L ) == NULL )
	    SET_BIT(wear_slots, WORN_FINGER_L);
        else if ( get_eq_char( ch, WEAR_FINGER_R ) == NULL )
	    SET_BIT(wear_slots, WORN_FINGER_R);
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_NECK ) )
    {
 	if (ch->fighting != NULL)
	{
	    send_to_char("You cannot dress while fighting.\n\r", ch);
	    return;
	}

	if ( get_eq_char( ch, WEAR_NECK_1 ) != NULL
	&&   get_eq_char( ch, WEAR_NECK_2 ) != NULL
	&&   !remove_obj_slot( ch, WEAR_NECK_1, fReplace )
	&&   !remove_obj_slot( ch, WEAR_NECK_2, fReplace ) )
	    return;

	if ( get_eq_char( ch, WEAR_NECK_1 ) == NULL )
	    SET_BIT(wear_slots, WORN_NECK_1);
        else if ( get_eq_char( ch, WEAR_NECK_2 ) == NULL )
	    SET_BIT(wear_slots, WORN_NECK_2);
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_BODY ) )
    {
 	if (ch->fighting != NULL)
	{
	    send_to_char("You cannot dress while fighting.\n\r", ch);
	    return;
	}

	if ( !remove_obj_slot( ch, WEAR_BODY, fReplace ) )
	    return;

	SET_BIT(wear_slots, WORN_BODY);
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_HEAD ) )
    {
 	if (ch->fighting != NULL)
	{
	    send_to_char("You cannot dress while fighting.\n\r", ch);
	    return;
	}

	if ( !remove_obj_slot( ch, WEAR_HEAD, fReplace ) )
	    return;

	SET_BIT(wear_slots, WORN_HEAD);
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_LEGS ) )
    {
 	if (ch->fighting != NULL)
	{
	    send_to_char("You cannot dress while fighting.\n\r", ch);
	    return;
	}

	if ( !remove_obj_slot( ch, WEAR_LEGS, fReplace ) )
	    return;

	SET_BIT(wear_slots, WORN_LEGS);
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_FEET ) )
    {
 	if (ch->fighting != NULL)
	{
	    send_to_char("You cannot dress while fighting.\n\r", ch);
	    return;
	}

	if ( !remove_obj_slot( ch, WEAR_FEET, fReplace ) )
	    return;

	SET_BIT(wear_slots, WORN_FEET);
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_HANDS ) )
    {
 	if (ch->fighting != NULL)
	{
	    send_to_char("You cannot dress while fighting.\n\r", ch);
	    return;
	}

	if ( !remove_obj_slot( ch, WEAR_HANDS, fReplace ) )
	    return;

	SET_BIT(wear_slots, WORN_HANDS);
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_ARMS ) )
    {
 	if (ch->fighting != NULL)
	{
	    send_to_char("You cannot dress while fighting.\n\r", ch);
	    return;
	}

	if ( !remove_obj_slot( ch, WEAR_ARMS, fReplace ) )
	    return;

	SET_BIT(wear_slots, WORN_ARMS);
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_ABOUT ) )
    {
 	if (ch->fighting != NULL)
	{
	    send_to_char("You cannot dress while fighting.\n\r", ch);
	    return;
	}

	if ( !remove_obj_slot( ch, WEAR_ABOUT, fReplace ) )
	    return;

	SET_BIT(wear_slots, WORN_ABOUT);
    }
    
    if ( CAN_WEAR( obj, ITEM_WEAR_SIGIL ) )
    {
 	if (ch->fighting != NULL)
	{
	    send_to_char("You cannot dress while fighting.\n\r", ch);
	    return;
	}

	if ( !remove_obj_slot( ch, ITEM_WEAR_SIGIL, fReplace ) )
	    return;

	SET_BIT(wear_slots, WORN_SIGIL);
    }

    if ( CAN_WEAR( obj, ITEM_PROG ) )
    {
        if (ch->fighting != NULL)
        {
            send_to_char("You cannot dress while fighting.\n\r", ch);
            return;
        }

        if ( !remove_obj_slot( ch, ITEM_PROG, fReplace ) )
            return;

        SET_BIT(wear_slots, WORN_PROGSLOT);
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_FAMILIAR ) )
    {
        if ( !remove_obj_slot( ch, ITEM_WEAR_FAMILIAR, fReplace ) )
            return;

        SET_BIT(wear_slots, WORN_FAMILIAR);
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_WAIST ) )
    {
 	if (ch->fighting != NULL)
	{
	    send_to_char("You cannot dress while fighting.\n\r", ch);
	    return;
	}

	if ( !remove_obj_slot( ch, WEAR_WAIST, fReplace ) )
	    return;

	SET_BIT(wear_slots, WORN_WAIST);
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_WRIST ) )
    {
 	if (ch->fighting != NULL)
	{
	    send_to_char("You cannot dress while fighting.\n\r", ch);
	    return;
	}
	
	if ( get_eq_char( ch, WEAR_WRIST_L ) != NULL
	&&   get_eq_char( ch, WEAR_WRIST_R ) != NULL
	&&   !remove_obj_slot( ch, WEAR_WRIST_L, fReplace )
	&&   !remove_obj_slot( ch, WEAR_WRIST_R, fReplace ) )
	    return;

	if ( get_eq_char( ch, WEAR_WRIST_L ) == NULL )
	    SET_BIT(wear_slots, WORN_WRIST_L);
	else if ( get_eq_char( ch, WEAR_WRIST_R ) == NULL )
	    SET_BIT(wear_slots, WORN_WRIST_R);

    }

    if ( CAN_WEAR( obj, ITEM_WEAR_SHIELD ) )
    {
	if (remove_obj_slot(ch, WEAR_SHIELD, fReplace))
	{
	    if ((hands_free(ch) <= 0) && fReplace)
	        remove_obj_slot(ch, WEAR_HOLD, fReplace);
	    if ((hands_free(ch) <= 0) && fReplace)
	        remove_obj_slot(ch, WEAR_LIGHT, fReplace);
	    if ((hands_free(ch) <= 0) && fReplace)
	        remove_obj_slot(ch, WEAR_DUAL_WIELD, fReplace);
	    if ((hands_free(ch) <= 0) && fReplace)
	        remove_obj_slot(ch, WEAR_WIELD, fReplace);
	}
	else
	    return;

	if (hands_free(ch) <= 0)
	{
	    send_to_char("Your hands are full.\n\r", ch);
	    return;
	}

	if (is_affected(ch, gsn_form_of_the_bear))
	{
	    send_to_char("You can't wield a shield while in the form of the bear.\n\r", ch);
	    return;
	}

	if (is_affected(ch, gsn_lesserhandwarp) || is_affected(ch, gsn_handwarp) || is_affected(ch, gsn_greaterhandwarp)
	|| is_affected(ch, gsn_lessercarapace) || is_affected(ch, gsn_greatercarapace) 
	|| is_affected(ch, gsn_lycanthropy) || is_affected(ch, gsn_aviancurse) || is_affected(ch, gsn_shadowfiend))
	{
		send_to_char("You can't use a shield with your body transformed like that.\n\r", ch);
		return;
	}

	SET_BIT(wear_slots, WORN_SHIELD);
    }

    if ( CAN_WEAR( obj, ITEM_WIELD ) )
    {
	int hands;

	if (!IS_NPC(ch)
	 && ((get_skill(ch,gsn_augmented_strength) > 0) ?
	   get_obj_weight(obj) > str_app[URANGE(0,get_curr_stat(ch,STAT_STR)+3,25)].wield * 10 :
	   get_obj_weight(obj) > str_app[get_curr_stat(ch,STAT_STR)].wield * 10))
	{
	    send_to_char( "It is too heavy for you to wield.\n\r", ch );
	    return;
	}

        if (twohand || (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS) 
	  && ((ch->size < SIZE_LARGE) 
	    || ((ch->size >= SIZE_LARGE) 
	      && ((obj->value[0] == WEAPON_SPEAR) 
		|| (obj->value[0] == WEAPON_POLEARM) 
		|| (obj->value[0] == WEAPON_STAFF))))))
	    hands = 2;
	else
	    hands = 1;

        weapon = get_eq_char(ch, WEAR_WIELD);
        if (CAN_DUAL(ch)
	  && (weapon == NULL)
	  && (hands == 1)
	  && ((weapon = get_eq_char(ch, WEAR_DUAL_WIELD)) != NULL)
	  && can_use_dual(ch, obj, weapon))
	{
	    SET_BIT(wear_slots, WORN_WIELD);
	    sn = get_weapon_sn_from_obj(obj);	    
	}
	else
	{
	    if (remove_obj_slot(ch, WEAR_DUAL_WIELD, fReplace)
	      && weapon
	      && CAN_DUAL(ch) 
	      && (hands == 1)
	      && !can_use_dual(ch, weapon, obj))
	    {
		remove_obj_slot(ch, WEAR_WIELD, fReplace);
		if (get_eq_char(ch, WEAR_WIELD))
		    return;
	    }

            if ((!CAN_DUAL(ch) || (hands_free(ch) < hands)) && fReplace)
		remove_obj_slot(ch, WEAR_WIELD, fReplace);

	    weapon = get_eq_char(ch, WEAR_WIELD);

	    if ((hands == (hands_free(ch) + 1)) || (hands == 1) || ((hands == 2) && !weapon))
	    {	
		if ((hands_free(ch) < hands) && fReplace)
	            remove_obj_slot(ch, WEAR_HOLD, fReplace);
		if ((hands_free(ch) < hands) && fReplace)
	            remove_obj_slot(ch, WEAR_SHIELD, fReplace);
		if ((hands_free(ch) < hands) && fReplace)
		    remove_obj_slot(ch, WEAR_LIGHT, fReplace);
	    }
	
	    if (hands_free(ch) < hands)
	    {
		if (hands == 2)
	            send_to_char("You require both hands free to wield that.\n\r", ch);
		else
		    send_to_char("Your hands are full.\n\r", ch);
		return;
 	    }

	    if (is_affected(ch, gsn_lesserhandwarp) 
	      || is_affected(ch, gsn_handwarp) 
	      || is_affected(ch, gsn_greaterhandwarp)
	      || is_affected(ch, gsn_lycanthropy) 
	      || is_affected(ch, gsn_shadowfiend) 
	      || is_affected(ch, gsn_aviancurse))
            {
		send_to_char("You can't use a weapon with your body transformed like that.\n\r", ch);
		return;
	    }

	    weapon = get_eq_char(ch, WEAR_WIELD);
	
	    if (weapon && !twohand && CAN_DUAL(ch))
		SET_BIT(wear_slots, WORN_DUAL_WIELD);
	    else if (!weapon)
		SET_BIT(wear_slots, WORN_WIELD);
	    else
		return;
	    if (twohand || hands == 2)
		SET_BIT(ch->paffected_by, AFF_TWOHAND);

	    sn = get_weapon_sn_from_obj(obj);
	}
    }

    if ( CAN_WEAR( obj, ITEM_HOLD ) )
    {
	if (remove_obj_slot(ch, WEAR_HOLD, fReplace))
	{
	    if ((hands_free(ch) <= 0) && fReplace)
		remove_obj_slot(ch, WEAR_SHIELD, fReplace);
	    if ((hands_free(ch) <= 0) && fReplace)
		remove_obj_slot(ch, WEAR_LIGHT, fReplace);
	    if ((hands_free(ch) <= 0) && fReplace)
		remove_obj_slot(ch, WEAR_DUAL_WIELD, fReplace);
	    if ((hands_free(ch) <= 0) && fReplace)
		remove_obj_slot(ch, WEAR_WIELD, fReplace);
	}
	else
	    return;

	if (hands_free(ch) <= 0)
        {
	    send_to_char("Your hands are full.\n\r", ch);
	    return;
        }

	if (is_affected(ch, gsn_lesserhandwarp) || is_affected(ch, gsn_handwarp) || is_affected(ch, gsn_greaterhandwarp)
	|| is_affected(ch, gsn_lycanthropy) || is_affected(ch, gsn_lycanthropy) || is_affected(ch, gsn_shadowfiend))
	{
		send_to_char("You can't hold objects properly with your body transformed like that.\n\r", ch);
		return;
	}
		

	SET_BIT(wear_slots, WORN_HOLD);
    }

    if (wear_slots)
    {

	/* This all needs to be indented. */
	if (!mobReset)
	{
	if (IS_SET(wear_slots, WORN_FLOAT))
	{
	    act("$n releases $p to float next to $m.",ch,obj,NULL,TO_ROOM);
	    act("You release $p and it floats next to you.",ch,obj,NULL,TO_CHAR);
	}

	if (IS_SET(wear_slots, WORN_LIGHT))
	{
	    act( "$n lights $p, which begins to shine brightly.", ch, obj, NULL, TO_ROOM );
	    act( "You light $p, which begins to shine brightly.",  ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_FINGER_L))
	{
	    act( "$n wears $p on $s left finger.",    ch, obj, NULL, TO_ROOM );
	    act( "You wear $p on your left finger.",  ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_FINGER_R))
	{
	    act( "$n wears $p on $s right finger.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p on your right finger.", ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_NECK_1) || IS_SET(wear_slots, WORN_NECK_2))
	{
	    act( "$n wears $p around $s neck.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_BODY))
	{
	    act( "$n wears $p on $s torso.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p on your torso.", ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_HEAD))
	{
	    act( "$n wears $p on $s head.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p on your head.", ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_LEGS))
	{
	    act( "$n wears $p on $s legs.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p on your legs.", ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_FEET))
	{
	    act( "$n wears $p on $s feet.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p on your feet.", ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_HANDS))
	{
	    act( "$n wears $p on $s hands.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p on your hands.", ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_ARMS))
	{
	    act( "$n wears $p on $s arms.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p on your arms.", ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_ABOUT))
	{
	    act( "$n wears $p about $s torso.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p about your torso.", ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_SIGIL))
	{
	    act( "$n places $p upon $s arm, and it grafts firmly to $m.",   ch, obj, NULL, TO_ROOM );
  	    act( "You place $p upon your arm, and it grafts firmly to you.", ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_WAIST))
	{
	    act( "$n wears $p about $s waist.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p about your waist.", ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_WRIST_L))
	{
	    act( "$n wears $p around $s left wrist.", ch, obj, NULL, TO_ROOM );
	    act( "You wear $p around your left wrist.",	ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_WRIST_R))
	{
	    act( "$n wears $p around $s right wrist.", ch, obj, NULL, TO_ROOM );
	    act( "You wear $p around your right wrist.", ch, obj, NULL, TO_CHAR );
	}

	if (IS_SET(wear_slots, WORN_SHIELD))
	{
	    act( "$n wears $p as a shield.", ch, obj, NULL, TO_ROOM );
	    act( "You wear $p as a shield.", ch, obj, NULL, TO_CHAR );
	}
	 
	if (IS_SET(wear_slots, WORN_DUAL_WIELD))
	{
	    act("$n dual wields $p.", ch, obj, NULL, TO_ROOM);
	    act("You dual wield $p.", ch, obj, NULL, TO_CHAR);
	}

	if (IS_SET(wear_slots, WORN_WIELD))
	{
	    if (IS_SET(ch->paffected_by,AFF_TWOHAND))
	    {
		act("$n wields $p with both hands.", ch, obj, NULL, TO_ROOM);
		act("You wield $p with both hands.", ch, obj, NULL, TO_CHAR);
	    }
	    else
	    {
		act("$n wields $p.", ch, obj, NULL, TO_ROOM);
		act("You wield $p.", ch, obj, NULL, TO_CHAR);
	    }
	}

	if ((IS_SET(wear_slots, WORN_DUAL_WIELD) || IS_SET(wear_slots, WORN_WIELD))
	 && (sn != gsn_hand_to_hand))
	{
            skill = get_weapon_skill(ch,sn,TRUE);
 
            if (skill >= 100)
                act("$p feels like a part of you!",ch,obj,NULL,TO_CHAR);
            else if (skill > 85)
                act("You feel quite confident with $p.",ch,obj,NULL,TO_CHAR);
            else if (skill > 70)
                act("You are skilled with $p.",ch,obj,NULL,TO_CHAR);
            else if (skill > 50)
                act("Your skill with $p is adequate.",ch,obj,NULL,TO_CHAR);
            else if (skill > 25)
                act("$p feels a little clumsy in your hands.",ch,obj,NULL,TO_CHAR);
            else if (skill > 1)
                act("You fumble and almost drop $p.",ch,obj,NULL,TO_CHAR);
            else
                act("You don't even know which end is up on $p.",
                    ch,obj,NULL,TO_CHAR);
	}

	if (IS_SET(wear_slots, WORN_HOLD))
	{
	    act( "$n holds $p in $s hand.",   ch, obj, NULL, TO_ROOM );
	    act( "You hold $p in your hand.", ch, obj, NULL, TO_CHAR );
	}
	}

	equip_char(ch, obj, wear_slots);
	oprog_wear_trigger(obj);

	if (obj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
	    check_heirloom_level(ch, obj);
    }
    else if ( fReplace )
	send_to_char( "You can't wear, wield, or hold that.\n\r", ch );
}

void do_sleeve( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int chance;

    one_argument( argument, arg );

    if ((chance = get_skill(ch,gsn_sleeve)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if ( arg[0] == '\0' )
    {
	send_to_char( "Slip what up your sleeve?\n\r", ch );
	return;
    }

	if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}

        if ((obj->item_type != ITEM_WEAPON) || (obj->value[0] != WEAPON_DAGGER))
	{
	   send_to_char("You can't slip that up your sleeve.\n", ch);
	   return;
	}

	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_sleeve].beats));

	if (chance < number_percent())	
	{
	  act("$n tries to slip $p up $s sleeve, but fumbles and drops it.", ch, obj, NULL, TO_ROOM);
	  act("You try to slip $p up your sleeve, but fumble and drop it.", ch, obj, NULL, TO_CHAR);
	  obj_from_char(obj);
	  obj_to_room(obj, ch->in_room);
	  check_improve(ch,NULL,gsn_sleeve,FALSE,4);
	  return;
	}
	else
	  check_improve(ch,NULL,gsn_sleeve,TRUE,4);

        if ((get_eq_char(ch, WEAR_CONCEAL1) != NULL) &&
		(get_eq_char(ch, WEAR_CONCEAL2) != NULL))
	{
	  send_to_char("You don't have room up your sleeves for that.\n", ch);
	  return;
	}

	if (get_eq_char(ch, WEAR_CONCEAL1) == NULL)
	    equip_char(ch, obj, WORN_CONCEAL1);
        else
	    equip_char(ch, obj, WORN_CONCEAL2);

	act("$n slips $p into a concealed position up $s sleeve.", ch, obj, NULL, TO_ROOM);
	act("You slip $p into a concealed position up your sleeve.", ch, obj, NULL, TO_CHAR);
}

void do_wear( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Wear, wield, or hold what?\n\r", ch );
	return;
    }

    if (is_affected(ch, gsn_commandword))
	return;

    if (IS_NPC(ch) && (is_affected(ch, gsn_possession) || IS_AFFECTED(ch, AFF_CHARM)))
	return;

    if ( !str_cmp( arg, "all" ) )
    {
	OBJ_DATA *obj_next;

	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;

	    if (!obj->worn_on && can_see_obj(ch, obj))
		wear_obj(ch, obj, FALSE, FALSE, FALSE);
	}
	return;
    }
    else
    {
	if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}
	if (argument[0] != '\0')
	{
	    if (!str_prefix(argument,"twohand"))
		wear_obj(ch,obj,TRUE,FALSE,TRUE);
	    else
		wear_obj(ch, obj, TRUE, FALSE, FALSE);
	}
	else   
	    wear_obj( ch, obj, TRUE, FALSE, FALSE );
    }

    return;
}



void do_remove( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Remove what?\n\r", ch );
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
        OBJ_DATA *obj_next;

        for ( obj = ch->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;

	    if (obj->worn_on && can_see_obj(ch, obj) && !IS_SET(obj->worn_on, WORN_SIGIL) && !IS_SET(obj->worn_on, WORN_PROGSLOT))
		remove_obj(ch, obj, TRUE);
        }
        return;
    }
    else
    {
        if ( ( obj = get_obj_wear( ch, arg ) ) == NULL )
        {
            if ((obj = get_obj_carry(ch,arg,ch)) != NULL && obj == ch->nocked)
	    {
		if (is_affected(ch,gsn_ready))
		{
		    act("You relax your draw on $p, and stop readying your shot.",ch,obj,NULL,TO_CHAR);
		    act("$n relaxes $s draw on $p, and stops readying a shot.",ch,obj,NULL,TO_ROOM);
		    affect_strip(ch,gsn_ready);
		}
		else
		{
		    act("You relax your draw on $p.",ch,obj,NULL,TO_CHAR);
		    act("$n relaxes $s draw on $p.",ch,obj,NULL,TO_ROOM);
		}
		ch->nocked = NULL;
	    }
	    else
		send_to_char( "You do not have that item.\n\r", ch );
            return;
        }

        remove_obj( ch, obj, TRUE );
    }
}

void do_destroy( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj=NULL, *objcont, *obj_next;
    AFFECT_DATA paf;
    OBJ_DATA *reaver;
paf.valid = TRUE;
paf.point = NULL;
    CHAR_DATA *gch;
    char buf[MAX_STRING_LENGTH];
    
    if (ch->fighting != NULL)
    {
	send_to_char("You couldn't possibly manage that in the middle of a fight!\n\r", ch);
	return;
    }
    
    one_argument( argument, arg );
	
    if (arg[0] == '\0')
    {
	send_to_char("What do you wish to destroy?\n\r", ch);
	return;
    }

    if (ch && ch->in_room)
    	obj = get_obj_list( ch, arg, ch->in_room->contents );
    else
	obj = NULL;
    
    if ( obj == NULL )
    {
	send_to_char( "You can't find it.\n\r", ch );
	return;
    }
    
// brazen: Ticket #205: Astral projections should not be able to destroy
    if ( IS_PAFFECTED(ch, AFF_VOIDWALK) || is_affected(ch, gsn_astralprojection))
    {
        act("Your immaterial hands pass through $d.", ch, NULL, obj->short_descr, TO_CHAR);
	return;
    }

    if (!IS_NPC(ch) && IS_OAFFECTED(ch, AFF_GHOST))
    {
        act("Your ghostly hands pass through $p.", ch, obj, NULL, TO_CHAR);
	return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_VAULT))
    {
	send_to_char("The sanctity of this place prevents you from destroying anything here.\n\r", ch);
	return;
    }

	if (obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_CORPSE_NPC)
    {
        if ((silver_state == SILVER_FINAL) 
        || area_is_affected(ch->in_room->area, gsn_deathwalk) 
        || area_is_affected(ch->in_room->area, gsn_barrowmist))
	    {
	        send_to_char("The corpse resists your attempts to destroy it, its dead flesh quickly reforming.\n\r", ch);
	        return;
	    }
    }

    if (!(obj->item_type == ITEM_CORPSE_PC 
      || obj->item_type == ITEM_CORPSE_NPC)
      && (!CAN_WEAR(obj, ITEM_TAKE) || CAN_WEAR(obj,ITEM_NO_SAC)))
    {
        send_to_char("You cannot destroy that.\n\r", ch);
        return;
    }

    if (obj_is_affected(obj, gsn_dispatchlodestone))
    {
        act("$p bobs in the air, avoiding your grasp.", ch, obj, NULL, TO_CHAR);
        return;
    }

    if (IS_SET(obj->extra_flags[0],ITEM_NODESTROY))
    {
	send_to_char("You cannot destroy that.\n\r",ch);
	return;
    }
    if (obj->in_room != NULL)
    {
	for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
	    if (gch->on == obj)
	    {
		act("$N appears to be using $p.", ch,obj,gch,TO_CHAR);
		return;
	    }
    }
		
    if ( obj->item_type == ITEM_CORPSE_PC ) 
    {
	reaver = get_eq_char(ch, WEAR_WIELD);

	if (!reaver
	 || !obj_is_affected(reaver, gsn_soulreaver))
	{
	    reaver = get_eq_char(ch, WEAR_DUAL_WIELD);
	    
	    if (!reaver
	     || !obj_is_affected(reaver, gsn_soulreaver))
	   {
	       send_to_char( "You cannot effectively destroy that.\n\r",ch);
	       return;
	   }
	}

	if (IS_SET(obj->value[1], CORPSE_DESTROYED))
	{
	    send_to_char("That corpse has already been desiccated.\n\r", ch);
	    return;
	}

        if (obj->killed_by != NULL && (!strcmp(obj->name, ch->name) || strcmp(obj->killed_by, ch->name)))
        {
            send_to_char("You must be a corpse's killer to sacrifice it.\n\r",ch); 
            return;
        }
       
	sprintf(buf, "You ritualistically mutilate %s, drawing its lingering life energies into %s.", obj->short_descr, reaver->short_descr);
	act(buf, ch, NULL, NULL, TO_CHAR);

	sprintf(buf, "$n ritualistically mutilates %s, drawing its lingering life energies into %s.", obj->short_descr, reaver->short_descr);
	act(buf, ch, NULL, NULL, TO_ROOM);

	paf.where	= TO_OBJECT;
	paf.type	= gsn_soulreaver;
	paf.level	= obj->level;
	paf.duration	= -1;
	paf.modifier	= 1;
	paf.location    = APPLY_HITROLL;
	paf.bitvector   = 0;
	obj_affect_join(reaver, &paf);

	paf.modifier    = 1;
	paf.location	= APPLY_DAMROLL;
	obj_affect_join(reaver, &paf);

	desiccate_corpse(obj);
	return;
    }
    
    /* moved up here so we can return after money items */
    act("You quickly destroy $p.", ch, obj, NULL, TO_CHAR);
    act("$n quickly destroys $p.", ch, obj, NULL, TO_ROOM);

    if (obj->item_type == ITEM_MONEY)
    {
	extract_obj(obj);
	return;
    }

    oprog_sac_trigger(obj, ch);

    if (( obj->item_type == ITEM_CORPSE_NPC ) ||
       ( obj->item_type == ITEM_CONTAINER ))
    {
      for ( objcont = obj->contains; objcont; objcont = obj_next )
      {
          obj_next = objcont->next_content;
          obj_from_obj( objcont );
          obj_to_room( objcont, ch->in_room );
      }
    }
    
    if (obj->pIndexData->vnum == OBJ_VNUM_STONE_POWER)
    {
    	send_to_char("As you shatter the stone of power, you feel some of its power flow into you.\n\r", ch);
	    act("As $n shatters the stone of power, you see an aura coalesce about $m for a moment.", ch, NULL, NULL, TO_ROOM);
	
        int modMultiplier(check_stoneloupe(ch) ? 2 : 1);
        paf.where      = TO_AFFECTS;
        paf.type       = gsn_stoneofpower;
        paf.level      = ch->level;
        paf.duration   = 50;
        paf.location   = APPLY_HIT;
        paf.modifier   = 100 * modMultiplier;
        paf.bitvector  = 0;
    	affect_to_char(ch, &paf);

	    paf.location   = APPLY_MANA;
    	affect_to_char(ch, &paf);

	    paf.location   = APPLY_SAVES;
    	paf.modifier   = -10 * modMultiplier;
	    affect_to_char(ch, &paf);
    }

    if (obj->pIndexData->vnum == OBJ_VNUM_SPIRIT_STONE)
    {
	act("$n breaks the stone, and a warm glow surrounds $m for a moment.", ch, NULL, NULL, TO_ROOM);
	act("You break the stone, and a warm glow surrounds you for a moment.", ch, NULL, NULL, TO_CHAR);
        ch->mana = UMIN(ch->max_mana, ch->mana+obj->value[0]);
    }

    handle_waterwheel_crystal_destroyed(*ch, *obj);
    handle_quintessence_destruction(ch, obj);
    handle_chargestone_destruction(ch, obj);
    handle_phylactery_destruction(*ch, *obj);
    activate_demon_bind(ch, obj);
    extract_obj( obj );
}

void do_sacrifice( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj, *objcont, *obj_next;
    AFFECT_DATA paf;
paf.valid = TRUE;
paf.point = NULL;
    int i = -1, sacvalue = 0,chance=0, r=0;
    OBJ_DATA *reaver;
//    bool gFound = FALSE;
    CHAR_DATA *gch;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || !str_cmp( arg, ch->name ) )
    {
	act( "$n offers $mself to the gods, who graciously declines.", ch, NULL, NULL, TO_ROOM );
	send_to_char("The gods appreciate your offer and may accept it later.\n\r", ch );
	return;
    }

    if (ch->fighting != NULL)
    {
	send_to_char("You couldn't possibly manage that in the middle of a fight!\n\r", ch);
	return;
    }

    if (ch && ch->in_room)
    	obj = get_obj_list( ch, arg, ch->in_room->contents );
    else
	obj = NULL;
    if ( obj == NULL )
    {
	send_to_char( "You can't find it.\n\r", ch );
	return;
    }

    if (!IS_NPC(ch) && IS_OAFFECTED(ch, AFF_GHOST))
    {
        act("Your ghostly hands pass through $p.", ch, obj, NULL, TO_CHAR);
	return;
    }

    if (is_affected(ch,gsn_sacrifice))
    {
	if (get_modifier(ch->affected,gsn_sacrifice)>0)
	    send_to_char("You're still empowered by your last sacrifice.\n\r",ch);
	else
	    send_to_char("You've attempted to sacrifice something too recently.\n\r",ch);
	return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_VAULT))
    {
	send_to_char("The sanctity of this place prevents you from sacrificing here.\n\r", ch);
	return;
    }

    if (obj_is_affected(obj, gsn_dispatchlodestone))
    {
        act("$p bobs in the air, avoiding your grasp.", ch, obj, NULL, TO_CHAR);
        return;
    }

    if ( IS_PAFFECTED(ch, AFF_VOIDWALK) || is_affected(ch, gsn_astralprojection))
    {
        act("Your immaterial hands pass through $d.", ch, NULL, obj->short_descr, TO_CHAR);
        return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Sacrifice to which god?\n\r", ch);
	return;
    }
	
    if ((i = god_lookup(argument)) == -1)
    {
	send_to_char("That is not an acceptable power to sacrifice to.\n\r", ch);
	return;
    }

    if ((silver_state == SILVER_FINAL)
     || area_is_affected(ch->in_room->area, gsn_deathwalk))
	if ((obj->item_type == ITEM_CORPSE_PC)
	 || (obj->item_type == ITEM_CORPSE_NPC))
	{
	    send_to_char("A strange, morbid still comes over the room as you try to sacrifice the corpse. The gods are not listening.\n\r", ch);
	    return;
	}

    if ( obj->item_type == ITEM_CORPSE_PC ) 
    {
	reaver = get_eq_char(ch, WEAR_WIELD);

	if (!reaver
	 || (reaver->pIndexData->vnum != OBJ_VNUM_SOUL_REAVER)
	 || !obj_is_affected(reaver, gsn_soulreaver))
	{
	    reaver = get_eq_char(ch, WEAR_DUAL_WIELD);
	    
	    if (!reaver
	     || (reaver->pIndexData->vnum != OBJ_VNUM_SOUL_REAVER)
	     || !obj_is_affected(reaver, gsn_soulreaver))
	   {
	       send_to_char( "The gods wouldn't like that.\n\r",ch);
	       return;
	   }
	}

	if (IS_SET(obj->value[1], CORPSE_DESTROYED))
	{
	    send_to_char("That corpse has already been desiccated.\n\r", ch);
	    return;
	}

        if (obj->killed_by != NULL && (!strcmp(obj->name, ch->name) || strcmp(obj->killed_by, ch->name)))
        {
            send_to_char("You must be a corpse's killer to sacrifice it.\n\r",ch); 
            return;
        }
       
	sprintf(buf, "You ritualistically sacrifice %s to the gods, drawing its lingering life energies into %s.", obj->short_descr, reaver->short_descr);
	act(buf, ch, NULL, NULL, TO_CHAR);

	sprintf(buf, "$n ritualistically sacrifices %s to the gods, drawing its lingering life energies into %s.", obj->short_descr, reaver->short_descr);
	act(buf, ch, NULL, NULL, TO_ROOM);

	paf.where	= TO_OBJECT;
	paf.type	= gsn_soulreaver;
	paf.level	= obj->level;
	paf.duration	= -1;
	paf.modifier	= 1;
	paf.location    = APPLY_HITROLL;
	paf.bitvector   = 0;
	affect_to_obj(reaver, &paf);

	paf.location	= APPLY_DAMROLL;
	affect_to_obj(reaver, &paf);

	desiccate_corpse(obj);
	return;
    }

    if (!CAN_WEAR(obj, ITEM_TAKE) || CAN_WEAR(obj, ITEM_NO_SAC))
    {
	act( "$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR );
	return;
    }

    if (obj->in_room != NULL)
    {
	for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
	    if (gch->on == obj)
	    {
		act("$N appears to be using $p.", ch,obj,gch,TO_CHAR);
		return;
	    }
    }
    
    if (god_table[i].can_follow == FALSE)
    {
	send_to_char("That is not yet an acceptable power to sacrifice to.\n\r", ch);
	return;
    }

    act("You offer $p to $T.", ch, obj, god_table[i].name, TO_CHAR);
    act("$n sacrifices $p to $T.", ch, obj, god_table[i].name, TO_ROOM);
    sprintf(buf, "$N sends up $p as a burnt offering to %s.", god_table[i].name);
    wiznet(buf, ch, obj, WIZ_SACCING, 0, 0);
    sprintf(buf,"RP: %s sacrificed %s to %s at %s [%d].",ch->name,obj->short_descr,god_table[i].name,ch->in_room->name,ch->in_room->vnum);
    log_string(buf);

    if (!IS_NPC(ch))
    {
        // Check for heart of the inferno
        AFFECT_DATA * heartAff = get_obj_affect(obj, gsn_heartoftheinferno);
        while (heartAff != NULL && heartAff->location != APPLY_NONE)
            heartAff = get_obj_affect(obj, gsn_heartoftheinferno, heartAff);

        // Found affect; check to set the bit
        if (heartAff != NULL)
            PyreInfo::checkHeartSacrifice(ch, obj, heartAff);

	sacvalue = obj->level;
	if ((obj->pIndexData->vnum >= OBJ_VNUM_TROPHY_FINGER
	  && obj->pIndexData->vnum <= OBJ_VNUM_TROPHY_TONGUE)
	  || obj->pIndexData->vnum == OBJ_VNUM_TROPHY_HEART
	  || (obj->pIndexData->vnum >= OBJ_VNUM_TROPHY_BRAINS
	    && obj->pIndexData->vnum <= OBJ_VNUM_TROPHY_EYE)
	  || (obj->pIndexData->vnum >= OBJ_VNUM_HIDE_CAP
	    && obj->pIndexData->vnum <= OBJ_VNUM_HIDE_COAT)
	  || obj->pIndexData->vnum == OBJ_VNUM_MMM_MEAT)
	    sacvalue /= 2;
	if (IS_SET(ch->in_room->gods_altar, 1 << i))
	    sacvalue *= 4/3;
	if (BIT_GET(ch->pcdata->traits, TRAIT_PIOUS))
	    sacvalue *= 4/3;

	paf.where = TO_AFFECTS;
	paf.type = gsn_sacrifice;
	paf.level = obj->level;
	paf.duration = 47;
	paf.bitvector = 0;

	if (god_table[i].apply == APPLY_NONE)
        {
	    paf.modifier = 0;
	    paf.location = APPLY_NONE;
        }
        else
        {
            chance = ch->pcdata->sacrifices[i].level/((ch->pcdata->sacrifices[i].number+1)*2)+sacvalue/2+ch->pcdata->sacrifices[i].number/10;

	    if (number_percent() < chance)
	    {
	    	sprintf(buf,"$N was rewarded! Chance of success: %d%% (item: %d%%, history: %d%%, frequency: %d%%",
		  chance,sacvalue/2,
		  ch->pcdata->sacrifices[i].level/((ch->pcdata->sacrifices[i].number+1)*2),
		  ch->pcdata->sacrifices[i].number/10);
            	wiznet(buf, ch, obj, WIZ_SACCING, 0, 0);
	    	if (IS_IMMORTAL(ch))
	    	{
	            sprintf(buf,"You were rewarded! Chance of success: %d%% (item: %d%%, history: %d%%, frequency: %d%%\n\r",
		      chance,sacvalue/2,
		      ch->pcdata->sacrifices[i].level/((ch->pcdata->sacrifices[i].number+1)*2),
		      ch->pcdata->sacrifices[i].number/10);
		    send_to_char(buf,ch);
	    	}
		
	    	act("$T is pleased by your sacrifice!",ch,NULL,god_table[i].name, TO_CHAR);
	    	act("$T is pleased by $n's sacrifice!",ch,NULL,god_table[i].name, TO_ROOM);
	    	
	    	paf.point = str_dup((char *) god_table[i].godaffname);

		if (!str_cmp(god_table[i].name,"Tzajai"))
		    do
		    {
			r = number_range(0,MAX_GODS); 
			  //this bullshit can change when all the gods are done
		    } while (god_table[r].can_follow == FALSE || !str_cmp(god_table[r].name,"Tzajai"));
		else
		    r = i;

	    	paf.location = god_table[r].apply;
		if (!str_cmp(god_table[r].name,"Ayaunj"))
		{
		    if (IS_SET(ch->in_room->gods_altar, 1 << i) 
		      || BIT_GET(ch->pcdata->traits, TRAIT_PIOUS))
			i = obj->level;
		    else
			i = obj->level * 3;
		    ch->coins[C_COPPER] += i;
		    sprintf(buf,"You gained %d copper coins!\n\r",i);
		    send_to_char(buf,ch);
		    paf.modifier = 0;
		    paf.location = APPLY_NONE;
		}
		else
		{
		    switch(god_table[r].apply)
		    {
			case APPLY_NONE: paf.point = NULL; break;
			case APPLY_SAVES: paf.modifier = -2; break;
			case APPLY_DAMROLL: paf.modifier = 2; break;
			case APPLY_HITROLL: paf.modifier = 7; break;
			case APPLY_HIT:
			case APPLY_MANA: paf.modifier = 15; break;
			case APPLY_MOVE: paf.modifier = 25; break;
			default: paf.modifier = 3; break;
		    }
		    if (IS_SET(ch->in_room->gods_altar, 1 << i) 
		      || BIT_GET(ch->pcdata->traits, TRAIT_PIOUS))
		    	paf.modifier *= 2;
		}
	    }
	    else
	    {
	    	sprintf(buf,"$N was not rewarded. Chance of success: %d%% (item: %d%%, history: %d%%, frequency: %d%%",
		  chance,sacvalue/2,
		  ch->pcdata->sacrifices[i].level/((ch->pcdata->sacrifices[i].number+1)*2),
		  ch->pcdata->sacrifices[i].number/10);
            	wiznet(buf, ch, obj, WIZ_SACCING, 0, 0);
	    	if (IS_IMMORTAL(ch))
	    	{
	            sprintf(buf,"You were not rewarded. Chance of success: %d%% (item: %d%%, history: %d%%, frequency: %d%%\n\r",
		      chance,sacvalue/2,
		      ch->pcdata->sacrifices[i].level/((ch->pcdata->sacrifices[i].number+1)*2),
		      ch->pcdata->sacrifices[i].number/10);
		    send_to_char(buf,ch);
	    	}
	
	    	paf.modifier = 0;
	    	paf.location = APPLY_NONE;
		paf.duration = 23;
	    }
	}
	affect_to_char(ch,&paf);
	  
	ch->pcdata->sacrifices[i].level += sacvalue;	
	ch->pcdata->sacrifices[i].number += 1;
    }

    if (obj->item_type == ITEM_MONEY)
    {
	extract_obj(obj);
	return;
    }

    oprog_sac_trigger(obj, ch);

    if (( obj->item_type == ITEM_CORPSE_NPC ) ||
       ( obj->item_type == ITEM_CONTAINER ))
    {
	for ( objcont = obj->contains; objcont; objcont = obj_next )
	{
          obj_next = objcont->next_content;
          obj_from_obj( objcont );
          obj_to_room( objcont, ch->in_room );
	}
    }

    if (obj->pIndexData->vnum == OBJ_VNUM_STONE_POWER)
    {
	    send_to_char("As you shatter the stone of power, you feel some of its power flow into you.\n\r", ch);
    	act("As $n shatters the stone of power, you see an aura coalesce about $m for a moment.", ch, NULL, NULL, TO_ROOM);
	
        int modMultiplier(check_stoneloupe(ch) ? 2 : 1);
        paf.where      = TO_AFFECTS;
        paf.type       = gsn_stoneofpower;
        paf.level      = ch->level;
        paf.duration   = 50;
        paf.location   = APPLY_HIT;
        paf.modifier   = 100 * modMultiplier;
        paf.bitvector  = 0;
    	affect_to_char(ch, &paf);

	    paf.location   = APPLY_MANA;
    	affect_to_char(ch, &paf);

	    paf.location   = APPLY_SAVES;
    	paf.modifier   = -10 * modMultiplier;
	    affect_to_char(ch, &paf);
    }

    if (obj->pIndexData->vnum == OBJ_VNUM_SPIRIT_STONE)
    {
	act("$n breaks the stone, and a warm glow surrounds $m for a moment.", ch, NULL, NULL, TO_ROOM);
	act("You break the stone, and a warm glow surrounds you for a moment.", ch, NULL, NULL, TO_CHAR);
        ch->mana = UMIN(ch->max_mana, ch->mana+250);
    }

    handle_waterwheel_crystal_destroyed(*ch, *obj);
    handle_quintessence_destruction(ch, obj);
    handle_chargestone_destruction(ch, obj);
    handle_phylactery_destruction(*ch, *obj);
    activate_demon_bind(ch, obj);
    extract_obj( obj );
}

void activate_demon_bind(CHAR_DATA *ch, OBJ_DATA *obj)
{
    if (obj_is_affected(obj, gsn_demon_bind))
    {
	char buf[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *d;
	CHAR_DATA *vch, *demon;
	int dmod = get_modifier(obj->affected, gsn_demon_bind);

	act("An exultant scream echoes from $p!", ch, obj, NULL, TO_CHAR);
	act("An exultant scream echoes from $p!", ch, obj, NULL, TO_ROOM);
	
	for (d = descriptor_list; d; d = d->next)
	    if (d->connected == CON_PLAYING)
	    {
		vch = d->original ? d->original : d->character;
		
		if (!IS_NPC(vch) && (vch->id == dmod) && vch->in_room)
		{
		    if (obj_is_affected(obj, gsn_jawsofidcizon))
			demon = create_mobile(get_mob_index(MOB_VNUM_DEMON_IDCIZON));
		    else if (obj_is_affected(obj, gsn_blade_of_vershak))
			demon = create_mobile(get_mob_index(MOB_VNUM_DEMON_VERSHAK));
		    else if (obj_is_affected(obj, gsn_caressofpricina))
			demon = create_mobile(get_mob_index(MOB_VNUM_DEMON_SUCCUBI));
		    else if (obj_is_affected(obj, gsn_hungerofgrmlarloth))
			demon = create_mobile(get_mob_index(MOB_VNUM_DEMON_GRMLARLOTH));
		    else if (obj_is_affected(obj, gsn_defilement))
			demon = create_mobile(get_mob_index(MOB_VNUM_DEMON_LOGOR));
		    else if (obj_is_affected(obj, gsn_mireofoame))
			demon = create_mobile(get_mob_index(MOB_VNUM_DEMON_SCION));
		    else
			break;

		    demon->demontrack = vch;
		    char_to_room(demon, ch->in_room);
		    act("$p begins to emit a noxious black smoke as it dissolves.", demon, obj, NULL, TO_ROOM);
		    act("A hulking demonic form rises out of the smoke!", demon, obj, NULL, TO_ROOM);
		    do_yell(demon, "At last I am free!");
		    if (demon->in_room != vch->in_room)
		    {
		        act("$n steps through a dark portal and disappears.", demon, NULL, NULL, TO_ROOM);
		        char_from_room(demon);
		        char_to_room(demon, vch->in_room);
		        act("A dark portal appears in mid-air, and $n steps through.", demon, NULL, NULL, TO_ROOM);
		    }
		    sprintf(buf, "You will pay for your transgressions, %s!", APERS(vch, demon));
		    do_yell(demon, buf);
		    multi_hit(demon, vch, TYPE_UNDEFINED);
		    if (!IS_NPC(ch)) ch->pcdata->adrenaline += 20;
		}
	    }
    }			
}


void do_quaff( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Quaff what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
	send_to_char( "You do not have that potion.\n\r", ch );
	return;
    }

    if ( obj->item_type != ITEM_POTION )
    {
	send_to_char( "You can quaff only potions.\n\r", ch );
	return;
    }

    act( "$n quaffs $p.", ch, obj, NULL, TO_ROOM );
    act( "You quaff $p.", ch, obj, NULL ,TO_CHAR );

    if (ch->fighting)
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));

    if (obj_is_affected(obj, gsn_illusion) || (ch->in_room && IS_SET(ch->in_room->room_flags, ROOM_NOMAGIC)))
        send_to_char("Nothing happened.\n\r", ch);
    else
    {
	if (obj_is_affected(obj, gsn_subtlepoison)) send_to_char("You feel as if something is subtly wrong.\n\r", ch);
	obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
        obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
        obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );
    }

    extract_obj( obj );
}

void do_recite( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *scroll;
    OBJ_DATA *obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( get_skill(ch,gsn_scrolls) < 5 ) {
	send_to_char( "It's unintelligible to you.\n\r", ch);
        return;
    }

    if (is_an_avatar(ch))
    {
	    send_to_char("You can read the scroll, but cannot speak.\n\r",ch);
    	return;
    }

    if ( ( scroll = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
	send_to_char( "You do not have that scroll.\n\r", ch );
	return;
    }

    if ( scroll->item_type != ITEM_SCROLL )
    {
	send_to_char( "You can recite only scrolls.\n\r", ch );
	return;
    }

    if ( ch->level < scroll->level)
    {
	send_to_char(
		"This scroll is too complex for you to comprehend.\n\r",ch);
	return;
    }

    obj = NULL;
    if ( arg2[0] == '\0' )
    {
	victim = ch;
    }
    else
    {
	if ( ( victim = get_char_room ( ch, arg2 ) ) == NULL
	&&   ( obj    = get_obj_here  ( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "You can't find it.\n\r", ch );
	    return;
	}
    }

    act( "$n recites $p.", ch, scroll, NULL, TO_ROOM );
    act( "You recite $p.", ch, scroll, NULL, TO_CHAR );

    if (obj_is_affected(scroll, gsn_illusion))
        act("$p crumbles uselessly in your hands!", ch, scroll, NULL, TO_CHAR);
    else if (number_percent() >= 20 + get_skill(ch,gsn_scrolls) * 4/5)
    {
    	send_to_char("You mispronounce a syllable.\n\r",ch);
	    check_improve(ch,NULL,gsn_scrolls,FALSE,2);
    }
    else
    {
    	obj_cast_spell( scroll->value[1], scroll->value[0], ch, victim, obj );
    	obj_cast_spell( scroll->value[2], scroll->value[0], ch, victim, obj );
    	obj_cast_spell( scroll->value[3], scroll->value[0], ch, victim, obj );
    	check_improve(ch,NULL,gsn_scrolls,TRUE,2);
    }

    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE*2));

    if (number_percent() <= (get_skill(ch, gsn_scriptmastery) / 2))
    {
        check_improve(ch, NULL, gsn_scriptmastery, true, 4);
        send_to_char("Drawing on your mastery of script, you manage to preserve the power of the scroll.\n", ch);
    }
    else
        extract_obj( scroll );
}

void do_brandish( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    OBJ_DATA *staff;
    int sn;

    if ( get_skill(ch,gsn_staves) < 5 ) {
	send_to_char( "You don't know how to brandish!\n\r", ch );
        return;
    }

    if ( ( staff = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
    {
	send_to_char( "You hold nothing in your hand.\n\r", ch );
	return;
    }

    if ( staff->item_type != ITEM_STAFF )
    {
	send_to_char( "You can brandish only with a staff.\n\r", ch );
	return;
    }

    if ( ( sn = staff->value[3] ) < 0
    ||   sn >= MAX_SKILL
    ||   skill_table[sn].spell_fun == 0 )
    {
        bug( "Do_brandish: bad sn %d.", sn );
        return;
    }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    if ( staff->value[2] > 0 )
    {
        act( "$n brandishes $p.", ch, staff, NULL, TO_ROOM );
        act( "You brandish $p.",  ch, staff, NULL, TO_CHAR );

        if (obj_is_affected(staff, gsn_illusion))
            act("...and nothing happens.",ch,NULL,NULL,TO_ALL);
        else if ( ch->level < staff->level 
        ||   number_percent() >= 20 + get_skill(ch,gsn_staves) * 4/5)
        {
            act ("You fail to invoke $p.",ch,staff,NULL,TO_CHAR);
            act ("...and nothing happens.",ch,NULL,NULL,TO_ROOM);
            check_improve(ch,NULL,gsn_staves,FALSE,2);
        }
        else
        {
            for ( vch = ch->in_room->people; vch; vch = vch_next )
            {
                vch_next	= vch->next_in_room;
                if (!is_same_group(vch, ch) || (IS_NPC(vch) && IS_AFFECTED(vch, AFF_WIZI)))
                    continue;

                switch ( skill_table[sn].target )
                {
                    default:
                        bug( "Do_brandish: bad target for sn %d.", sn );
                        return;

                    case TAR_IGNORE:
                    case TAR_CHAR_OFFENSIVE:
                    case TAR_OBJ_CHAR_OFF:
                    case TAR_CHAR_SELF:
                        if ( vch != ch )
                            continue;
                        break;

                    case TAR_CHAR_DEFENSIVE:
                    case TAR_OBJ_CHAR_DEF:
                        break;
                }

                // HACK -- cloak of the void goes target to target, not caster to target
                if (staff->value[3] == gsn_cloakofthevoid)
                    obj_cast_spell( staff->value[3], staff->value[0], vch, vch, NULL );
                else	
                    obj_cast_spell( staff->value[3], staff->value[0], ch, vch, NULL );
                check_improve(ch,NULL,gsn_staves,TRUE,2);
            }
        }
    }

    if ( --staff->value[2] <= 0 )
    {
        act( "$p blazes brightly and is gone.", ch, staff, NULL, TO_ROOM );
        act( "$p blazes brightly and is gone.", ch, staff, NULL, TO_CHAR );
        extract_obj( staff );
    }
}

void do_runicmessage(CHAR_DATA *ch, char *argument)
{
char buf[MAX_STRING_LENGTH];
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
OBJ_DATA *obj, *obj_next;
EXTRA_DESCR_DATA *ed;
int chance;


    if ((chance = get_skill(ch,gsn_runicmessage)) == 0)
        {
        send_to_char("Huh?\n\r", ch);
        return;
        }

    if (argument[0] == '\0')
    {
	send_to_char("Scribe what on the runic message?\n\r", ch);
	return;
    }

    for (obj = ch->in_room->contents; obj != NULL; obj = obj_next)
    {
	obj_next = obj->next_content;
	if (obj->pIndexData->vnum == OBJ_VNUM_RUNIC_MESSAGE)
	{
	  act("You begin by destroying the existing rune.", ch, NULL, NULL, TO_ROOM);
	  extract_obj(obj);
	}
    }


    if (chance < number_percent())
    {
      send_to_char("You failed and destroyed the rune in the making.\n\r", ch);
      check_improve(ch,NULL,gsn_runicmessage,FALSE,2);
      return;
    }

    check_improve(ch,NULL,gsn_runicmessage,TRUE,2);

    if (is_affected(ch, gsn_runicmessage))
    {
	send_to_char("You aren't ready to craft another message yet.\n\r", ch);
	return;
    }

    if ((obj = create_object(get_obj_index(OBJ_VNUM_RUNIC_MESSAGE),0)) == NULL)
	return;

    sprintf(buf, "The rune reads: %s", argument);

    ed = new_extra_descr();
    ed->keyword = str_dup("rune");
    ed->description = str_dup(buf);
    ed->next = obj->extra_descr;
    obj->extra_descr = ed;
    ed = new_extra_descr();
    ed->keyword = str_dup("runic");
    ed->description = str_dup(buf);
    ed->next = obj->extra_descr;
    obj->extra_descr = ed;

    act("$n creates and places a hidden rune.", ch, NULL, NULL, TO_ROOM);
    act("You create and place a hidden rune.", ch, NULL, NULL, TO_CHAR);
    obj_to_room(obj, ch->in_room);
    SET_BIT(obj->extra_flags[0], ITEM_HIDDEN);

        af.where      = TO_AFFECTS;
        af.type       = gsn_runicmessage;
        af.level      = ch->level;
        af.duration   = 6;
        af.location   = 0;
        af.modifier   = 0;
        af.bitvector  = 0;
	affect_to_char(ch, &af);
}

void do_applyoil( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_STRING_LENGTH];
	OBJ_DATA * target_obj;
	OBJ_DATA * oil;
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
	int i, space = 0, ease = 0;
	bool fail = FALSE;
	
	argument = one_argument(argument, arg);
	if (arg[0] == '\0')
	{
		send_to_char("Apply what?\n\r", ch);
		return;
	}

	if ((oil = get_obj_carry(ch, arg, ch)) == NULL)
	{
		sprintf(buf, "You aren't carrying any %s.\n\r", arg);
		send_to_char(buf, ch);
		return;
	}
	if (ch->mana < skill_table[gsn_applyoil].min_mana)
	{
		send_to_char("You don't have the energy right now to concentrate on properly applying an oil.\n\r", ch);
		return;
	}

	if (oil->item_type != ITEM_OIL)
	{
		send_to_char("You can only apply oils.\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg);
	if (arg[0] == '\0')
	{
		send_to_char("To what do you wish to apply the oil?\n\r", ch);
		return;
	}

	if ((target_obj = get_obj_carry(ch, arg, ch)) == NULL)
	{
		sprintf(buf, "You aren't carrying any %s.\n\r", arg);
		send_to_char(buf, ch);
		return;
	}
	// Object application
	if (oil == target_obj)
	{
		send_to_char("Apply the oil to itself? How does that work?\n\r", ch);
		return;
	}
		
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_applyoil].beats));
	expend_mana(ch, skill_table[gsn_applyoil].min_mana);
	
	sprintf(buf, "%s applies %s to %s.", ch->name, oil->short_descr, target_obj->short_descr);
	act( buf, ch, NULL, NULL, TO_ROOM );
	sprintf(buf, "You apply %s to %s.", oil->short_descr, target_obj->short_descr);
	act( buf, ch, oil, target_obj, TO_CHAR );

	if (number_percent() >= get_skill(ch, gsn_applyoil))
	{
		send_to_char("Your improper technique fails to release the magic from the oil.\n\r", ch);
		check_improve(ch, NULL, gsn_applyoil, FALSE, 1);
		extract_obj(oil);
		return;
	}
		
        for (i = 1; i < 5; i++)
	{
		if (oil->value[i] > 0)
		{
			if (target_obj->item_type == ITEM_WAND || target_obj->item_type == ITEM_STAFF)
			{
				if (target_obj->value[2] <= 0 || target_obj->value[3] == oil->value[i])
				{
					if (target_obj->value[1] < target_obj->value[2] + 1)
					{
			send_to_char("You waste your oil attempting to charge the item more than is possible.\n\r", ch);
						break;
					}
					space = elemental_lookup(skill_table[oil->value[i]].name);
					if (space & make_bin(BIAS_VOID_INDEX))
						ease += material_table[target_obj->material].elem_bias.bias_void;
					if (space & make_bin(BIAS_SPIRIT_INDEX))
						ease += material_table[target_obj->material].elem_bias.bias_spirit;
					if (space & make_bin(BIAS_EARTH_INDEX))
						ease += material_table[target_obj->material].elem_bias.bias_earth;
					if (space & make_bin(BIAS_FIRE_INDEX))
						ease += material_table[target_obj->material].elem_bias.bias_fire;
					if (space & make_bin(BIAS_WATER_INDEX))
						ease += material_table[target_obj->material].elem_bias.bias_water;
					if (space & make_bin(BIAS_AIR_INDEX))
						ease += material_table[target_obj->material].elem_bias.bias_air;
					if (space & make_bin(BIAS_NATURE_INDEX))
						ease += material_table[target_obj->material].elem_bias.bias_nature;
					if (space & make_bin(BIAS_ANCIENT_INDEX))
						ease += material_table[target_obj->material].elem_bias.bias_ancient;
					if (space & make_bin(BIAS_TRANSMOGRIFY_INDEX))
						ease += material_table[target_obj->material].elem_bias.bias_transmogrify;
					
					if (ease < 1) fail = TRUE;
					
					if (obj_is_affected(target_obj, gsn_applyoil)
					&& get_obj_affect(target_obj, gsn_applyoil)->level == oil->value[i])
						ease += get_obj_affect(target_obj, gsn_applyoil)->modifier;
					else
						object_affect_strip(target_obj, gsn_applyoil);
					
					if (number_percent() >= ease * (13 - (target_obj->item_type == ITEM_STAFF ? 2 : 0))
					|| fail)
					{
						sprintf(buf, "%s resists taking a charge of %s.\n\r",
							target_obj->name, skill_table[oil->value[i]].name);
						buf[0] = UPPER(buf[0]);
						send_to_char(buf, ch);
						af.where        = TO_OBJECT;
						af.type         = gsn_applyoil;
						af.level        = oil->value[i];
						af.duration     = ch->level;
						af.location     = 0;
						af.modifier     = (target_obj->item_type == ITEM_WAND ? 3 : 2);
						af.bitvector    = 0;
						obj_affect_join(target_obj, &af);
						break;
					}
					
					object_affect_strip(target_obj, gsn_applyoil);
					act("The oil's magic takes hold, and $p is charged up!", 
						ch, target_obj, NULL, TO_CHAR);
					//Levels are averaged, more or less
					if (target_obj->value[0] > oil->value[0])
					{
						target_obj->value[0] = ((target_obj->value[2]+1) * target_obj->value[0] +
									oil->value[0]) / (target_obj->value[2] + 2);
					}
					target_obj->value[2]++;
					target_obj->value[3] = oil->value[i];
				}
				else
				{
					sprintf(buf, "%s cannot accept any charges of %s.\n\r", 
						target_obj->short_descr, skill_table[oil->value[i]].name);
					buf[0] = UPPER(buf[0]);
					send_to_char(buf, ch);
				}
			}
			else
			{
				switch (skill_table[oil->value[i]].target)
				{
					case TAR_OBJ_INV:
					case TAR_OBJ_CHAR_DEF:
                    case TAR_OBJ_ROOM:
						(*skill_table[oil->value[i]].spell_fun) 
						( oil->value[i], oil->value[0], ch, (void *) target_obj, TARGET_OBJ);
						break;
					case TAR_OBJ_CHAR_OFF:
					case TAR_CHAR_OFFENSIVE:
						if (!obj_is_affected(target_obj, gsn_applyoil) 
						&& target_obj->item_type == ITEM_WEAPON)
						{
							af.where        = TO_OBJECT;
							af.type         = gsn_applyoil;
							af.level        = oil->value[0];
							af.duration     = ch->level / 5;
							af.location     = 0;
							af.modifier     = oil->value[i];
							af.bitvector    = 0;
							affect_to_obj(target_obj, &af);
							act("$p shimmers faintly as $n magically charges it.", 
								ch, target_obj, NULL, TO_ROOM);
							act("$p shimmers faintly as you magically charge it.",
								ch, target_obj, NULL, TO_CHAR);
						}									
					default: break;
				}
			}
		}
	}
	check_improve(ch, NULL, gsn_applyoil, TRUE, 1);
	extract_obj( oil );	
	return;
}

void do_pitch( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *item;
    int i;
    
    argument = one_argument( argument, arg );
    if ( arg[0] == '\0')
    {
	send_to_char( "What do you want to pitch?\n\r", ch );
	return;
    }

    if ((item = get_obj_carry(ch, arg, ch)) == NULL)
    {
	    sprintf(buf, "You aren't carrying any %s.\n\r", arg);
	    send_to_char(buf, ch);
	    return;
    }
    if (IS_SET(item->extra_flags[0], ITEM_NODROP))
    {
	    send_to_char("You can't let go of it.\n\r", ch);
	    return;
    }
    
    victim = NULL;
    argument = one_argument( argument, arg);
    if (arg[0] == '\0' && ch->fighting == NULL )
    {
	    send_to_char("At whom do you wish to pitch it?\n\r", ch);
	    return;
    }
    else if (arg[0] == '\0') victim = ch->fighting;
    else if ((victim = get_char_room ( ch, arg )) == NULL)
    {
	    send_to_char("They aren't here.\n\r", ch);
	    return;
    }

    //Logically shouldn't happen, but just in case
    if (victim == NULL)
    {
	    send_to_char("They aren't here.\n\r", ch);
	    return;
    }
    
    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    if (number_percent() > get_curr_stat(victim, STAT_DEX) - 15)
    {
	
        if ( item->item_type == ITEM_POTION )
	{
		act( "$n pitches $p at $N's chest, where it bursts open!", ch, item, victim, TO_NOTVICT );
		act( "You pitch $p at $N's chest, where it bursts open!", ch, item, victim, TO_CHAR );
		act( "$n pitches $p at your chest, where it bursts open!",ch, item, victim, TO_VICT );

		for (i = 1; i < 5; i++)
		{
			if (item->value[i] > 0)
			{
				switch (skill_table[item->value[i]].target)
				{
					case TAR_CHAR_SELF: break;
					case TAR_CHAR_OFFENSIVE: 
					    check_killer(ch,victim); //fall through
					default:
						obj_cast_spell( item->value[i], item->value[0], ch, victim, item );
						break;
				}
			}
		}
    		extract_obj( item );
	}
	else
	{
		if (!IS_IMMORTAL(ch) && get_obj_weight(item) > (str_app[get_curr_stat(ch,STAT_STR)].wield) * 11)
		{
			send_to_char("It is too heavy for you to throw.\n\r", ch);
			return;
		}
		
		act( "$n pitches $p at $N!", ch, item, victim, TO_NOTVICT );
		act( "You pitch $p at $N.", ch, item, victim, TO_CHAR );
		act( "$n pitches $p at you!",ch, item, victim, TO_VICT );

		damage_old(ch, victim, number_fuzzy((int)(get_obj_weight(item) / 10 + ch->level / 5)), 
			gsn_throw, DAM_BASH, TRUE);
		obj_from_char(item);
		obj_to_room(item, ch->in_room);
	}
    }
    else
    {
	if (item->item_type == ITEM_POTION)
	{
    		act( "$n pitches $p at $N, who leaps nimbly out of the way!", ch, item, victim, TO_NOTVICT );
		act( "You pitch $p at $N, who leaps nimbly out of the way!", ch, item, victim, TO_CHAR );
		act( "$n pitches $p at you, but you leap nimbly out of the way!",ch, item, victim, TO_VICT );
        	extract_obj( item );
	}
	else
	{
		if (!IS_IMMORTAL(ch) && get_obj_weight(item) > (str_app[get_curr_stat(ch,STAT_STR)].wield) * 11)
		{
			send_to_char("It is too heavy for you to throw.\n\r", ch);
			return;
		}

		act( "$n pitches $p at $N, who jumps neatly aside.", ch, item, victim, TO_NOTVICT );
		act( "You pitch $p at $N, who dodges easily.", ch, item, victim, TO_CHAR );
		act( "$n pitches $p at you, but you jump aside.",ch, item, victim, TO_VICT );
		obj_from_char(item);
		obj_to_room(item, ch->in_room);
	}
    }
   
      	if (!IS_NPC(victim))
	    if (can_see(victim, ch))
	    {
		sprintf( buf, "Help!  %s is attacking me!", PERS(ch, victim) );
		do_autoyell( victim, buf );
	    }
	    else
	    {
		sprintf( buf, "Help!  I'm being attacked!" );
		do_autoyell( victim, buf );
	    }
	check_killer(ch, victim);

    return;
}    


void do_zap( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wand;
    OBJ_DATA *obj;

    if ( get_skill(ch,gsn_wands) < 5 ) {
	send_to_char ("You wave the wand about in the air looking foolish.\n\r",ch);
    return;
    }

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' && ch->fighting == NULL )
    {
	send_to_char( "Zap whom or what?\n\r", ch );
	return;
    }

    if ( ( wand = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
    {
	send_to_char( "You hold nothing in your hand.\n\r", ch );
	return;
    }

    if ( wand->item_type != ITEM_WAND )
    {
	send_to_char( "You can zap only with a wand.\n\r", ch );
	return;
    }

    obj = NULL;
    victim = NULL;
    if ( arg[0] == '\0' )
    {
	if ( ch->fighting != NULL )
	{
	    victim = ch->fighting;
	}
	else
	{
	    send_to_char( "Zap whom or what?\n\r", ch );
	    return;
	}
    }
    else
    {
	if ( ( victim = get_char_room ( ch, arg ) ) == NULL
	&&   ( obj    = get_obj_here  ( ch, arg ) ) == NULL )
	{
	    send_to_char( "You can't find it.\n\r", ch );
	    return;
	}
    }

    if (!obj && !victim)
    {
	send_to_char("Zap whom or what?\n\r", ch);
	return;
    }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    switch(skill_table[wand->value[3]].target)
    {
	case TAR_IGNORE: 
	    victim = NULL; 
	    obj = NULL; 
	break;
	case TAR_CHAR_OFFENSIVE: 
	    if (!victim)
	    {
		send_to_char("Zap whom or what?\n\r", ch);
		return;
	    }
	break;
	case TAR_CHAR_DEFENSIVE:
	    if (!victim)
	    {
		send_to_char("Zap whom or what?\n\r", ch);
		return;
	    }
	break;
	case TAR_CHAR_SELF:
	    victim = NULL;
	    act("$n zaps $mself with $p.", ch, wand, NULL, TO_ROOM);
	    act("You zap yourself with $p.", ch, wand, NULL, TO_CHAR);
	break;
	case TAR_OBJ_INV:
	    if (!obj || obj->carried_by != ch)
	    {
		send_to_char("You must be carrying the object for that.\n\r", ch);
		return;
	    }
	break;
	default:
	break;
    }	  

    if ( wand->value[2] > 0 )
    {
        if ( victim != NULL )
        {
            act( "$n zaps $N with $p.", ch, wand, victim, TO_NOTVICT );
            act( "You zap $N with $p.", ch, wand, victim, TO_CHAR );
            act( "$n zaps you with $p.",ch, wand, victim, TO_VICT );
        }
        else if (obj != NULL)
        {
            act( "$n zaps $P with $p.", ch, wand, obj, TO_ROOM );
            act( "You zap $P with $p.", ch, wand, obj, TO_CHAR );
        }

        if ( skill_table[wand->value[3]].target == TAR_CHAR_OFFENSIVE )
        {
            if (!IS_NPC(victim))
            if (can_see(victim, ch))
                    {
                    sprintf( buf, "Help!  %s is attacking me!", PERS(ch, victim) );
                    do_autoyell( victim, buf );
                    }
                    else
                    {
                    sprintf( buf, "Help!  I'm being attacked!" );
                    do_autoyell( victim, buf );
                }
        }

        if (obj_is_affected(wand, gsn_illusion))
            act("...and nothing happens.", ch, NULL, NULL, TO_ALL);
        else if (  number_percent() >= ((20 + (get_skill(ch,gsn_wands) * 4)/5) - (wand->level - ch->level))
        ||  ( (wand->pIndexData->vnum == OBJ_VNUM_MAGIC_CRYSTAL) &&
              ((ch->class_num == global_int_class_earthtemplar && number_percent() < 20) ||
              (ch->class_num < 6 && ch->class_num != global_int_class_earthscholar && number_percent() < 61) ||
              (ch->class_num > 5  && ch->class_num < 12  && ch->class_num != global_int_class_earthtemplar && number_percent() < 81) ||
              (ch->class_num > 11 && number_percent() < 95) ) )
            )
        {
            act( "Your efforts with $p produce only smoke and sparks.",
             ch,wand,NULL,TO_CHAR);
            act( "$n's efforts with $p produce only smoke and sparks.",
             ch,wand,NULL,TO_ROOM);
            check_improve(ch,victim,gsn_wands,FALSE,2);
        }
        else
        {
            obj_cast_spell( wand->value[3], wand->value[0], ch, victim, obj );
            check_improve(ch,victim,gsn_wands,TRUE,2);
        }
    }

    if ( --wand->value[2] <= 0 )
    {
	act( "$p explodes into fragments in $n's hand.", ch, wand, NULL, TO_ROOM );
	act( "$p explodes into fragments in your hand.", ch, wand, NULL, TO_CHAR );
	extract_obj( wand );
    }

    return;
}


void do_loot( CHAR_DATA *ch, char *argument )
{
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int percent, ctype;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ((arg1[0] == '\0') || (arg2[0] == '\0'))
    {
	send_to_char( "Loot what from whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "That's pointless.\n\r", ch );
	return;
    }

    if (is_safe(ch,victim))
	return;

    if ( IS_NPC(victim) 
	  && victim->position == POS_FIGHTING)
    {
	send_to_char("You'd better not -- you might get hit.\n\r",ch);
	return;
    }

    WAIT_STATE( ch, skill_table[gsn_loot].beats );
    percent  = number_percent();

    if (IS_AWAKE(victim))
    {
	send_to_char("You don't think you can snatch gear from someone who's fully conscious.\n\r", ch);
	return;
    }

    if (ch->class_num == global_int_class_bandit)
	percent *= 2;

    if (ch->class_num == global_int_class_thief)
	percent *= 4;

    if (( !IS_NPC(ch) && percent > get_skill(ch,gsn_loot)) &&
        (ch->level < 53))
    {
	/*
	 * Failure.
	 */
	send_to_char( "Oops.\n\r", ch );
	affect_strip(ch,gsn_sneak);
	REMOVE_BIT(ch->affected_by,AFF_SNEAK);

	act( "$n tried to loot you.\n\r", ch, NULL, victim, TO_VICT    );
	act( "$n tried to loot $N.\n\r",  ch, NULL, victim, TO_NOTVICT );
	sprintf(buf,"Help! %s is trying to loot me!",PERS(ch, victim));

	damage(ch,victim,number_range(1,4),gsn_loot,DAM_PIERCE,TRUE);

        if (!IS_AWAKE(victim))
            do_wake(victim,"");
	if (IS_AWAKE(victim) && !IS_NPC(victim))
	    do_autoyell( victim, buf );
	if ( !IS_NPC(ch) )
	{
	    if ( IS_NPC(victim) )
	    {
	        check_improve(ch,victim,gsn_loot,FALSE,1);
		multi_hit( victim, ch, TYPE_UNDEFINED );
	    }
	    else
	    {
		sprintf(buf,"$N tried to loot from %s.",victim->name);
		wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
	    }
	}

	return;
    }

    if ((ctype = coin_lookup(arg1)) != -1)
    {
	int amount = victim->coins[ctype] * number_range(1, ch->level) / 60;

	if ( amount <= 0 )
	{
	    sprintf(buf, "You couldn't get any %s.\n\r", coin_table[ctype].name);
	    send_to_char( buf, ch );
	    return;
	}

	coins_to_char(ch, amount, ctype);
	coins_from_char(victim, amount, ctype);

	sprintf(buf, "Bingo!  You got %d %s coin%s.\n\r", amount, coin_table[ctype].name, amount > 1 ? "s" : "");
	send_to_char( buf, ch );
	check_improve(ch,victim,gsn_loot,TRUE,1);
	return;
    }


    if ((obj = get_obj_wear_loot(victim, arg1)) == NULL)
    {
	send_to_char("You can't find that.", ch);
	return;
    }

    if (!can_see_obj(ch, obj))
    {
	send_to_char("You can't find that.", ch);
	return;
    }
	
    if ( !can_drop_obj( ch, obj )
    ||   IS_SET(obj->extra_flags[0], ITEM_INVENTORY)
    ||   IS_SET(obj->extra_flags[0], ITEM_NOREMOVE))
    {
	send_to_char( "You can't pry it away.\n\r", ch );
	return;
    }

    if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
     && (get_cur_capacity(ch) + item_svalue[obj->size]) > get_max_capacity(ch))
    {
	send_to_char("You lack the room to carry that.\n\r", ch);
	return;
    }

    if ( ch->carry_number + 1 > static_cast<int>(Encumbrance::CarryCountCapacity(*ch)))
    {
	send_to_char( "You have your hands full.\n\r", ch );
	return;
    }

    if ( ch->carry_weight + get_obj_weight( obj ) > static_cast<int>(Encumbrance::CarryWeightCapacity(*ch)))
    {
	send_to_char( "You can't carry that much weight.\n\r", ch );
	return;
    }

    sprintf(buf,"Help! %s just looted me!",PERS(ch, victim));
    damage(ch,victim,number_range(1,4),gsn_loot,DAM_PIERCE,TRUE);

    if (IS_AWAKE(victim) && !IS_NPC(victim))
	do_autoyell(victim, buf);

    unequip_char(victim, obj);
    oprog_remove_trigger(obj);

    if (!obj || !obj->valid || !obj->carried_by || obj->carried_by != victim || obj->worn_on )
    {
	act("You fail to get $p.", ch, obj, NULL, TO_CHAR);
	return;
    }
    obj_from_char( obj );
    obj_to_char( obj, ch );
    act("You pocket $p.",ch,obj,NULL,TO_CHAR);
    check_improve(ch,victim,gsn_loot,TRUE,1);
    send_to_char( "Got it!\n\r", ch );

    check_aura(ch, obj, TRUE);
}

OBJ_DATA *make_trophy(int vnum, OBJ_DATA *corpse)
{
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];

    if ((obj = create_object(get_obj_index(vnum), corpse->level)) == NULL)
	return NULL;

    obj->level = corpse->level;
    obj->value[0] = corpse->value[0];
    obj->value[2] = corpse->value[2];

    sprintf(buf, obj->short_descr, &corpse->short_descr[IS_SET(corpse->value[1], CORPSE_DESTROYED) ? 25 : 14]);
    free_string(obj->short_descr);
    obj->short_descr = str_dup(buf);
    setName(*obj, buf);
    sprintf(buf, obj->description, &corpse->short_descr[IS_SET(corpse->value[1], CORPSE_DESTROYED) ? 25 : 14]);
    free_string(obj->description);
    obj->description = str_dup(buf);

    return obj;
}

void do_trophy( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *corpse;
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];
    int chance;

    argument = one_argument(argument, arg);

    if ((argument[0] == '\0') || (arg[0] == '\0'))
    {
	send_to_char("Syntax: trophy <corpse> <finger/ear/teeth/head/hair/hand/tongue/entrails/heart/brain/eye>\n\r", ch);
	return;
    }
  
    if ((corpse = get_obj_list(ch, arg, ch->in_room->contents)) == NULL)
    {
	send_to_char("You don't see any such corpse here.\n\r", ch);
	return;
    }

    if (obj_is_affected(corpse, gsn_decorporealize) || (corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_PC && corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_NPC))
    {
	send_to_char("You can't take a trophy off of that!\n\r", ch);
	return;
    }

    if (IS_SET(corpse->value[1], CORPSE_DESTROYED))
    {
	send_to_char("That corpse has already been desiccated.\n\r", ch);
	return;
    }

    if (!IS_NPC(ch) && IS_OAFFECTED(ch, AFF_GHOST))
    {
        act("Your ghostly hands pass through $p.", ch, corpse, NULL, TO_CHAR);
	return;
    }

    if (area_is_affected(ch->in_room->area, gsn_barrowmist))
    {
        send_to_char("The corpse resists your attempts to mangle it, the dead flesh quickly reforming.\n", ch);
        return;
    }

    if (!str_cmp(argument, "finger"))
    {
	if (IS_SET(corpse->value[1], CORPSE_MISSING_FINGER1))
	{
	    if (IS_SET(corpse->value[1], CORPSE_MISSING_FINGER2))
	    {
		send_to_char("No more fingers may be taken from this corpse.\n\r", ch);
		return;
	    }
	    else
	    {
		if (IS_SET(corpse->value[1], CORPSE_MISSING_HAND1) && IS_SET(corpse->value[1], CORPSE_MISSING_HAND2))
		{
		    send_to_char("You can sever no fingers from a handless corpse.", ch);
		    return;
		}
		SET_BIT(corpse->value[1], CORPSE_MISSING_FINGER2);
	        obj = make_trophy(OBJ_VNUM_TROPHY_FINGER, corpse);
	        act("You bend down, and quickly cut a finger off of $p.", ch, corpse, NULL, TO_CHAR);
	        act("$n bends down, and cuts a finger off of $p.", ch, corpse, NULL, TO_ROOM);
	    }
	}
	else
	{
  	    if (IS_SET(corpse->value[1], CORPSE_MISSING_HAND1) && IS_SET(corpse->value[1], CORPSE_MISSING_HAND2))
	    {
	        send_to_char("You can sever no fingers from a handless corpse.", ch);
	        return;
	    }
	    SET_BIT(corpse->value[1], CORPSE_MISSING_FINGER1);
  	    obj = make_trophy(OBJ_VNUM_TROPHY_FINGER, corpse);
 	    act("You bend down, and quickly cut a finger off of $p.", ch, corpse, NULL, TO_CHAR);
	    act("$n bends down, and cuts a finger off of $p.", ch, corpse, NULL, TO_ROOM);
	}
    }
    else if (!str_cmp(argument, "hand"))
    {
	if (IS_SET(corpse->value[1], CORPSE_MISSING_HAND1))
	{
	    if (IS_SET(corpse->value[1], CORPSE_MISSING_HAND2))
	    {
		send_to_char("The hands are already missing from this corpse.\n\r", ch);
		return;
	    }
	    else
	    {
		SET_BIT(corpse->value[1], CORPSE_MISSING_HAND2);
		if (!IS_SET(corpse->value[1], CORPSE_MISSING_FINGER1))
		    SET_BIT(corpse->value[1], CORPSE_MISSING_FINGER1);
		else
		    SET_BIT(corpse->value[1], CORPSE_MISSING_FINGER2);

	        obj = make_trophy(OBJ_VNUM_TROPHY_HAND, corpse);
 		act("You bend down, and cut a hand off of $p.", ch, corpse, NULL, TO_CHAR);
		act("$n bends down, and cuts a hand off of $p.", ch, corpse, NULL, TO_ROOM);
	    }
	}
	else
	{
	    SET_BIT(corpse->value[1], CORPSE_MISSING_HAND1);
	    if (!IS_SET(corpse->value[1], CORPSE_MISSING_FINGER1))
		SET_BIT(corpse->value[1], CORPSE_MISSING_FINGER1);
	    else
		SET_BIT(corpse->value[1], CORPSE_MISSING_FINGER2);
	
	    obj = make_trophy(OBJ_VNUM_TROPHY_HAND, corpse);
	    act("You bend down, and cut a hand off of $p.", ch, corpse, NULL, TO_CHAR);
	    act("$n bends down, and cuts a hand off of $p.", ch, corpse, NULL, TO_ROOM);
	}
    }
    else if (!str_cmp(argument, "ear"))
    {
	if (IS_SET(corpse->value[1], CORPSE_MISSING_HEAD))
	{
	    send_to_char("The head is already missing from that corpse.\n\r", ch);
	    return;
	}

	if (IS_SET(corpse->value[1], CORPSE_MISSING_EAR1))
	{
	    if (IS_SET(corpse->value[1], CORPSE_MISSING_EAR2))
	    {
		send_to_char("The ears are already missing from that corpse.\n\r", ch);
		return;
	    }
	    else
	    {
		SET_BIT(corpse->value[1], CORPSE_MISSING_EAR2);
		obj = make_trophy(OBJ_VNUM_TROPHY_EAR, corpse);
		act("You bend down, and quickly cut an ear off of $p.", ch, corpse, NULL, TO_CHAR);
		act("$n bends down, and cuts an ear off of $p.", ch, corpse, NULL, TO_ROOM);
	    }
	}
	else
	{
	    SET_BIT(corpse->value[1], CORPSE_MISSING_EAR1);
	    obj = make_trophy(OBJ_VNUM_TROPHY_EAR, corpse);
	    act("You bend down, and quickly cut an ear off of $p.", ch, corpse, NULL, TO_CHAR);
	    act("$n bends down, and cuts an ear off of $p.", ch, corpse, NULL, TO_ROOM);
	}
    }
    else if (!str_cmp(argument, "head"))
    {
	if (IS_SET(corpse->value[1], CORPSE_MISSING_HEAD))
	{
	    send_to_char("The head is already missing from that corpse.\n\r", ch);
	    return;
	}
	SET_BIT(corpse->value[1], CORPSE_MISSING_HEAD);
	obj = make_trophy(OBJ_VNUM_TROPHY_HEAD, corpse);
	obj->timer = 48;
	act("You bend down, and cut the head off of $p.", ch, corpse, NULL, TO_CHAR);
	act("$n bends down, and cuts the head off of $p.", ch, corpse, NULL, TO_ROOM);
    }
    else if (!str_cmp(argument, "teeth"))
    {
	if (IS_SET(corpse->value[1], CORPSE_MISSING_HEAD))
	{
	    send_to_char("The head is already missing from that corpse.\n\r", ch);
	    return;
	}
	else if (IS_SET(corpse->value[1], CORPSE_MISSING_TEETH))
	{
	    send_to_char("The usable teeth are already missing from that corpse.\n\r", ch);
	    return;
	}
	act("You bend down, and remove some teeth from $p.", ch, corpse, NULL, TO_CHAR);
	act("$n bends down, and removes some teeth from $p.", ch, corpse, NULL, TO_ROOM);
	SET_BIT(corpse->value[1], CORPSE_MISSING_TEETH);
	obj = make_trophy(OBJ_VNUM_TROPHY_TOOTH, corpse);
    }
    else if (!str_cmp(argument, "hair"))
    {
        if (corpse->value[2] == global_int_race_shuddeni)
        {
            send_to_char("That corpse is without hair.\n\r", ch);
            return;
        }
	if (IS_SET(corpse->value[1], CORPSE_MISSING_HEAD))
	{
	    send_to_char("The head is already missing from that corpse.\n\r", ch);
	    return;
	}
	else if (IS_SET(corpse->value[1], CORPSE_MISSING_HAIR))
	{
	    send_to_char("The usable hair is already missing from that corpse.\n\r", ch);
	    return;
	}
	act("You bend down, and cut a bit of hair off of $p.", ch, corpse, NULL, TO_CHAR);
	act("$n bends down, and cuts a bit of hair off of $p.", ch, corpse, NULL, TO_ROOM);
	SET_BIT(corpse->value[1], CORPSE_MISSING_HAIR);
	obj = make_trophy(OBJ_VNUM_TROPHY_HAIR, corpse);
    }
    else if (!str_cmp(argument, "tongue"))
    {
	if (IS_SET(corpse->value[1], CORPSE_MISSING_HEAD))
	{
	    send_to_char("The head is already missing from that corpse.\n\r", ch);
	    return;
	}
	else if (IS_SET(corpse->value[1], CORPSE_MISSING_TONGUE))
	{
	    send_to_char("The tongue is already missing from that corpse.\n\r", ch);
	    return;
	}
	act("You bend down, and cut the tongue out of $p.", ch, corpse, NULL, TO_CHAR);
	act("$n bends down, and cuts the tongue out of $p.", ch, corpse, NULL, TO_ROOM);
	SET_BIT(corpse->value[1], CORPSE_MISSING_TONGUE);
	obj = make_trophy(OBJ_VNUM_TROPHY_TONGUE, corpse);
    }
    else if (!str_cmp(argument, "entrails"))
    {
	if (IS_SET(corpse->value[1], CORPSE_MISSING_ENTRAILS))
	{
	    send_to_char("The entrails are already missing from that corpse.\n\r", ch);
	    return;
	}
	SET_BIT(corpse->value[1], CORPSE_MISSING_ENTRAILS);
	act("You bend down, and remove the bloody entrails from $p.", ch, corpse, NULL, TO_CHAR);
	act("$n bends down, and removes the bloody entrails from $p.", ch, corpse, NULL, TO_ROOM);
	obj = make_trophy(OBJ_VNUM_TROPHY_ENTRAILS, corpse);
    }
    else if (!str_cmp(argument, "heart"))
    {
	if (IS_SET(corpse->value[1], CORPSE_MISSING_HEART))
	{
	    send_to_char("The heart is already missing from that corpse.\n\r", ch);
	    return;
	}
	SET_BIT(corpse->value[1], CORPSE_MISSING_HEART);
	act("You bend down, and carve out the heart from $p.", ch, corpse, NULL, TO_CHAR);
	act("$n bends down, and carves out the heart from $p.", ch, corpse, NULL, TO_ROOM);
	obj = make_trophy(OBJ_VNUM_TROPHY_HEART, corpse);
    }
    else if (!str_cmp(argument, "brains") || !str_cmp(argument, "brain"))
    {
	if (IS_SET(corpse->value[1], CORPSE_MISSING_HEAD))
	{
	    send_to_char("The head is already missing from that corpse.\n\r", ch);
	    return;
	}

	if (IS_SET(corpse->value[1], CORPSE_MISSING_BRAINS))
	{
	    send_to_char("The brains are already missing from that corpse.\n\r", ch);
	    return;
	}

	SET_BIT(corpse->value[1], CORPSE_MISSING_BRAINS);
	act("You bend down, and carefully remove the brain from $p.", ch, corpse, NULL, TO_CHAR);
	act("$n bends down, and carefully removes the brain from $p.", ch, corpse, NULL, TO_ROOM);
	obj = make_trophy(OBJ_VNUM_TROPHY_BRAINS, corpse);
    }
    else if (!str_cmp(argument, "eye"))
    {
        if (corpse->value[2] == global_int_race_shuddeni)
        {
            send_to_char("That corpse is without eyes.\n\r", ch);
            return;
        }

	if (IS_SET(corpse->value[1], CORPSE_MISSING_HEAD))
	{
	    send_to_char("The head is already missing from that corpse.\n\r", ch);
	    return;
	}

	if (IS_SET(corpse->value[1], CORPSE_MISSING_EYE1))
	{
	    if (IS_SET(corpse->value[1], CORPSE_MISSING_EYE2))
	    {
		send_to_char("The eyes are already missing from that corpse.\n\r", ch);
		return;
	    }
	    else
	    {
		SET_BIT(corpse->value[1], CORPSE_MISSING_EYE2);
	        obj = make_trophy(OBJ_VNUM_TROPHY_EYE, corpse);
	        act("You kneel beside $p, and carefully pry out an eye.", ch, corpse, NULL, TO_CHAR);
	        act("$n kneels beside $p, and carefully pries out an eye.", ch, corpse, NULL, TO_ROOM);
	    }
	}
	else
	{
	    SET_BIT(corpse->value[1], CORPSE_MISSING_EYE1);
	    obj = make_trophy(OBJ_VNUM_TROPHY_EYE, corpse);
            act("You kneel beside $p, and carefully pry out an eye.", ch, corpse, NULL, TO_CHAR);
            act("$n kneels beside $p, and carefully pries out an eye.", ch, corpse, NULL, TO_ROOM);
	}
    }

    else
    {
	send_to_char("That is not an appropriate trophy to take.\n\r", ch);
	send_to_char("You may take: finger, ear, head, teeth, hair, hand, tongue, entrails, heart.\n\r", ch);
	return;
    }

    chance = UMAX(25,get_skill(ch,gsn_dissection));
    if (!IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_MORTICIAN))
        chance = UMAX(chance, 90);

    if (number_percent() > chance)
    {
	check_improve(ch,NULL,gsn_dissection,FALSE,1);
	send_to_char("You ruin the rest of the remains.\n\r",ch);
	act("$n ruins the rest of the remains.",ch,NULL,NULL,TO_ROOM);
	desiccate_corpse(corpse);
    }
    else
	check_improve(ch,NULL,gsn_dissection,TRUE,1);
   
    if (get_carry_weight(*ch) + get_obj_weight(obj) > static_cast<int>(Encumbrance::CarryWeightCapacity(*ch)))
    {
        sprintf(buf, "%s: you cannot carry that much weight.\n\r",obj->short_descr);
	send_to_char(buf, ch);
	obj_to_room(obj, ch->in_room);
    }
    else if (ch->carry_number + 1 > static_cast<int>(Encumbrance::CarryCountCapacity(*ch)))
    {
        sprintf(buf, "%s: you cannot carry that many items.\n\r",obj->short_descr);
	send_to_char(buf, ch);
	obj_to_room(obj, ch->in_room);
    }
    else
        obj_to_char(obj, ch);
    
    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE*2));
}

void do_fetish( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *corpse;
    OBJ_DATA *fetish;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];
    int chance, val;

    if ((chance = get_skill(ch,gsn_fetish)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    chance = round(chance * 0.8);

    if (ch->mana < skill_table[gsn_fetish].min_mana)
    {
	send_to_char("You're too tired to make a fetish.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Use what as a fetish?\n\r", ch);
	return;
    }

    if ((corpse = get_obj_list( ch, argument, ch->in_room->contents)) == NULL)
    {
	send_to_char("You don't see any such thing here.\n\r", ch);
	return;
    }

    if (corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_PC)
    {
	send_to_char("You can't make a fetish out of that!\n\r", ch);
	return;
    }

    if (str_cmp(corpse->killed_by, IS_NPC(ch) ? ch->short_descr : ch->name))
    {
	send_to_char("You don't feel their ear would make an appropriate fetish.\n\r", ch);
	return;
    }

    if (IS_SET(corpse->value[1], CORPSE_DESTROYED))
    {
	send_to_char("That corpse has already been desiccated.\n\r", ch);
	return;
    }

    if (IS_SET(corpse->value[1], CORPSE_MISSING_HEAD))
    {
	send_to_char("The head has been removed from that corpse.\n\r", ch);
	return;
    }

    if (IS_SET(corpse->value[1], CORPSE_MISSING_EAR1))
    {
	if (IS_SET(corpse->value[1], CORPSE_MISSING_EAR2))
	{
	    send_to_char("Both ears have been removed from that corpse.\n\r", ch);
	    return;
	}
	else
	    SET_BIT(corpse->value[1], CORPSE_MISSING_EAR2);
    }
    else
	SET_BIT(corpse->value[1], CORPSE_MISSING_EAR1);

    if (IS_SET(corpse->value[1], CORPSE_FETISHED))
    {
	send_to_char("A fetish has already been taken from that corpse.\n\r", ch);
	return;
    }

    SET_BIT(corpse->value[1], CORPSE_FETISHED);

    if (number_percent() > get_skill(ch, gsn_fetish))
    {
	send_to_char("You fail to properly fetish the corpse, ruining the ears.\n\r", ch);
	act("$n tries to make a fetish from $p, but fails.", ch, corpse, NULL, TO_ROOM);
	check_improve(ch, NULL, gsn_fetish, FALSE, 1);

	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_fetish].beats));
	expend_mana(ch, skill_table[gsn_fetish].min_mana/2);
	return;
    }

    af.where      = TO_OBJECT;
    af.type       = gsn_fetish;
    af.level      = ch->level;
    af.duration   = -1;
    af.location   = APPLY_DAMROLL;
    af.modifier   = 1;
    af.bitvector  = 0;


    for ( fetish = ch->carrying; fetish != NULL; fetish = fetish->next_content )
    {
	if (fetish->pIndexData->vnum == OBJ_VNUM_FETISH)
	{
	    unequip_char(ch, fetish);
	    obj_affect_join(fetish, &af);
	    equip_char(ch, fetish, WORN_NECK_1);
	    if ((val = get_modifier(fetish->affected, gsn_fetish)) < 0)
		val = 0;
    	    sprintf( buf, fetish->pIndexData->short_descr, val );
    	    free_string( fetish->short_descr );
    	    fetish->short_descr = str_dup( buf );
    	    fetish->value[0] = ch->level/3;
    	    fetish->value[1] = ch->level/3;
    	    fetish->value[2] = ch->level/3;
    	    fetish->value[3] = ch->level/3;
	    act("You carve up $p, keeping the ear!", ch, corpse, NULL, TO_CHAR);
	    act("$n carves up $p, keeping the ear!", ch, corpse, NULL, TO_ROOM);
    	    check_improve(ch,NULL,gsn_fetish,TRUE,0);
	    expend_mana(ch, skill_table[gsn_fetish].min_mana);
	    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_fetish].beats));
/*
            for ( objcont = corpse->contains; objcont; objcont = obj_next )
            {
              obj_next = objcont->next_content;
              obj_from_obj( objcont );
              obj_to_room( objcont, ch->in_room );
            }
	    extract_obj(corpse);
*/
	    return;
	}
    }

    fetish = create_object(get_obj_index(OBJ_VNUM_FETISH), 0);
    sprintf( buf, "a necklace dangling a single ear" );
    free_string( fetish->short_descr );
    fetish->short_descr = str_dup( buf );
    fetish->value[0] = ch->level/3;
    fetish->value[1] = ch->level/3;
    fetish->value[2] = ch->level/3;
    fetish->value[3] = ch->level/3;
    act("You carves up $p, keeping the ear!", ch, corpse, NULL, TO_CHAR);
    act("$n carves up $p, keeping the ear!", ch, corpse, NULL, TO_ROOM);
    affect_to_obj(fetish, &af);

/*
      for ( objcont = corpse->contains; objcont; objcont = obj_next )
      {
          obj_next = objcont->next_content;
          obj_from_obj( objcont );
          obj_to_room( objcont, ch->in_room );
      }
	extract_obj(corpse);
*/
    obj_to_char(fetish, ch);
    return;
}


void desiccate_corpse(OBJ_DATA *corpse)
{
    char *cpoint;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    cpoint = one_argument(corpse->short_descr, arg);  /* corpse */
    cpoint = one_argument(cpoint, arg);               /* of     */
    cpoint = one_argument(cpoint, arg);

    if (corpse->pIndexData->vnum == OBJ_VNUM_CORPSE_NPC)
	corpse->timer = UMAX(corpse->timer/4,1);

    sprintf(buf, "the desiccated corpse of %s", cpoint);
    free_string(corpse->short_descr);
    corpse->short_descr = str_dup(buf);

    sprintf(buf, "The desiccated husk of %s's corpse lies here.", cpoint);
    free_string(corpse->description);
    corpse->description = str_dup(buf);

    SET_BIT(corpse->value[1], CORPSE_DESTROYED);
}

void do_debone(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *corpse, *bones;
    char buf[MAX_STRING_LENGTH];
    MOB_INDEX_DATA *pMobIndex;

    if (!IS_NPC(ch) && (ch->class_num != global_int_class_voidscholar))
    {
	send_to_char("You are not versed in the ways of deboning.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("What do you wish to debone?\n\r", ch);
	return;
    }

    if ((corpse = get_obj_list(ch, argument, ch->in_room->contents)) == NULL)
    {
	send_to_char("You don't see that here.\n\r", ch);
	return;
    }

    if ((corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_NPC)
     && (corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_PC))
    {
	send_to_char("You cannot debone that.\n\r", ch);
	return;
    }

    if ((corpse->value[0] > 0) && ((pMobIndex = get_mob_index(corpse->value[0])) != NULL)
     && (IS_SET(pMobIndex->form, FORM_CONSTRUCT)
      || IS_SET(pMobIndex->form, FORM_MIST)
      || IS_SET(pMobIndex->form, FORM_INTANGIBLE)))
    {
	send_to_char("That corpse has no bones to remove.\n\r", ch);
	return;
    }

    if (IS_SET(corpse->value[1], CORPSE_DESTROYED))
    {
	send_to_char("That corpse has already been desiccated.\n\r", ch);
	return;
    }

    bones = create_object(get_obj_index(OBJ_VNUM_REAGENT_BONES), corpse->level);
    bones->level = corpse->level;
    bones->value[0] = corpse->value[0];

    sprintf( buf, bones->short_descr, &corpse->short_descr[IS_SET(corpse->value[1], CORPSE_DESTROYED) ? 25 : 14] );
    free_string( bones->short_descr );
    bones->short_descr = str_dup( buf );
    setName(*bones, buf);
    sprintf( buf, bones->description, &corpse->short_descr[IS_SET(corpse->value[1], CORPSE_DESTROYED) ? 25 : 14] );
    free_string( bones->description);
    bones->description = str_dup( buf );

    send_to_char("You carefully remove some bones from the corpse.\n\r", ch);
    act("$n kneels down, and carefully removes some bones from $p.", ch, corpse, NULL, TO_ROOM);

    desiccate_corpse(corpse);
    obj_to_char(bones, ch);
}

void do_exsanguinate(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *corpse, *vial, *blood;
    char buf[MAX_STRING_LENGTH];

    if (!IS_NPC(ch) && (ch->class_num != global_int_class_voidscholar))
    {
	send_to_char("You are not versed in the ways of exsanguination.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("What do you wish to exsanguinate?\n\r", ch);
	return;
    }

    if ((corpse = get_obj_list(ch, argument, ch->in_room->contents)) == NULL)
    {
	send_to_char("You don't see that here.\n\r", ch);
	return;
    }

    if ((corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_NPC)
     && (corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_PC))
    {
	send_to_char("You cannot exsanguinate that.\n\r", ch);
	return;
    }

    if (IS_SET(corpse->value[1], CORPSE_DESTROYED))
    {
	send_to_char("That corpse has already been desiccated.\n\r", ch);
	return;
    }

    for (vial = ch->carrying; vial; vial = vial->next_content)
	if (vial->pIndexData->vnum == OBJ_VNUM_BLOODVIAL)
	    break;

    if (!vial)
    {
	send_to_char("You lack a proper vial for storing the blood.\n\r", ch);
	return;
    }

    extract_obj(vial);

    blood = create_object(get_obj_index(OBJ_VNUM_REAGENT_BLOOD), corpse->level);
    blood->level = corpse->level;
    blood->value[0] = corpse->value[0];
    blood->value[1] = corpse->value[2];

    sprintf( buf, blood->short_descr, &corpse->short_descr[IS_SET(corpse->value[1], CORPSE_DESTROYED) ? 25 : 14] );
    free_string( blood->short_descr );
    blood->short_descr = str_dup( buf );
    setName(*blood, buf);
    sprintf( buf, blood->description, &corpse->short_descr[IS_SET(corpse->value[1], CORPSE_DESTROYED) ? 25 : 14] );
    free_string( blood->description);
    blood->description = str_dup( buf );

    sprintf(buf, "You carefully exsanguinate the corpse, creating %s!\n\r", blood->short_descr);
    send_to_char(buf, ch);

    act("$n kneels down beside $p, and proceeds to exsanguinate it, collecting the blood into a small vial.", ch, corpse, NULL, TO_ROOM);

    desiccate_corpse(corpse);
    obj_to_char(blood, ch);
}

void do_bleed( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *vial, *blood;
    char buf[MAX_STRING_LENGTH];

    if (!(ch->class_num == global_int_class_voidscholar 
     || ch->class_num == global_int_class_voidtemplar)
     || (argument[0] == '\0') || str_cmp(argument, "self"))
    {
	check_social(ch, "bleed", argument);
	return;
    }

    if (ch->hit <= 50)
    {
	send_to_char("You are too weakened to bleed yourself.\n\r", ch);
	return;
    }

    for (vial = ch->carrying; vial; vial = vial->next_content)
	if (vial->pIndexData->vnum == OBJ_VNUM_BLOODVIAL)
	    break;

    if (!vial)
    {
	send_to_char("You lack a proper vial for storing the blood.\n\r", ch);
	return;
    }

    extract_obj(vial);
   
    blood = create_object(get_obj_index(OBJ_VNUM_REAGENT_BLOOD), ch->level);
    blood->value[0] = (ch->id * -1);

    sprintf( buf, blood->short_descr, ch->name );
    free_string( blood->short_descr );
    blood->short_descr = str_dup( buf );
    setName(*blood, buf);
    sprintf( buf, blood->description, ch->name );
    free_string( blood->description);
    blood->description = str_dup( buf );

    act("You carefully cut yourself, draining some of your blood into a small vial.", ch, NULL, NULL, TO_CHAR);
    act("$n carefully cuts $mself, draining some of $s blood into a small vial.", ch, NULL, NULL, TO_ROOM);

    ch->hit -= 50;
    obj_to_char(blood, ch);
}

void do_tan( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *corpse;
    OBJ_DATA *hide;
    MOB_INDEX_DATA *pMobIndex = NULL;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int vnum, chance;

    if ((chance = get_skill(ch,gsn_tan)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->mana < skill_table[gsn_tan].min_mana)
    {
	send_to_char("You're too tired to make clothing from a hide now.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_tan))
    {
	send_to_char("You do not yet feel prepared to begin the process of tanning again.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Tan what into what?\n\r", ch);
	return;
    }

    if ((corpse = get_obj_list( ch, arg, ch->in_room->contents)) == NULL)
    {
	send_to_char("You don't see any such thing here.\n\r", ch);
	return;
    }

    if (corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_NPC && corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_PC)
    {
	send_to_char("You can't make a hide out of that!\n\r", ch);
	return;
    }

    if (IS_SET(corpse->value[1], CORPSE_DESTROYED))
    {
	send_to_char("That corpse has already been desiccated.\n\r", ch);
	return;
    }

    if (!strcmp(argument, "cap"))		vnum = OBJ_VNUM_HIDE_CAP;
    else if (!strcmp(argument, "vest"))		vnum = OBJ_VNUM_HIDE_VEST;
    else if (!strcmp(argument, "gloves"))	vnum = OBJ_VNUM_HIDE_GLOVES;
    else if (!strcmp(argument, "boots"))	vnum = OBJ_VNUM_HIDE_BOOTS;
    else if (!strcmp(argument, "sleeves"))	vnum = OBJ_VNUM_HIDE_SLEEVES;
    else if (!strcmp(argument, "leggings"))	vnum = OBJ_VNUM_HIDE_LEGGINGS;
    else if (!strcmp(argument, "bracer"))	vnum = OBJ_VNUM_HIDE_BRACER;
    else if (!strcmp(argument, "collar"))	vnum = OBJ_VNUM_HIDE_COLLAR;
    else if (!strcmp(argument, "belt"))		vnum = OBJ_VNUM_HIDE_BELT;
    else if (!strcmp(argument, "coat"))		vnum = OBJ_VNUM_HIDE_COAT;
    else
    {
	send_to_char("You don't know how to make that out of a hide.\n\r", ch);
	send_to_char("You can make a cap, vest, gloves, boots, leggings, sleeves, bracer, collar, belt, or coat.\n\r", ch);
	return;
    }


    if (number_percent() > get_skill(ch, gsn_tan))
    {
	send_to_char("You try to craft clothing from the hide, but fail.\n\r", ch);
	act("$n tries to craft clothing from $p, but fails.", ch, corpse, NULL, TO_ROOM);
    	check_improve(ch,NULL,gsn_tan,FALSE,2);
	desiccate_corpse(corpse);

	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_tan].beats));
	expend_mana(ch, skill_table[gsn_tan].min_mana/2);
	return;
    }

    hide            = create_object(get_obj_index(vnum), 0);
    hide->cost 	= 0;
    hide->level = ch->level;

    sprintf( buf, hide->short_descr, &corpse->short_descr[IS_SET(corpse->value[1], CORPSE_DESTROYED) ? 25 : 14] );
    free_string( hide->short_descr );
    hide->short_descr = str_dup( buf );
    sprintf( buf, hide->description, &corpse->short_descr[IS_SET(corpse->value[1], CORPSE_DESTROYED) ? 25 : 14] );
    free_string( hide->description);
    hide->description = str_dup( buf );
    hide->value[0] = UMIN(10,corpse->level/8);
    hide->value[1] = UMIN(10,corpse->level/8);
    hide->value[2] = UMIN(10,corpse->level/8);
    hide->value[3] = UMIN(10,corpse->level/6);

    if (corpse->value[0] > 0)
	pMobIndex = get_mob_index(corpse->value[0]);

    if (pMobIndex)
    {
	long curimmune;
        int i;

        curimmune = pMobIndex->imm_flags;
        for (i = 0; i < MAX_RESIST-2; i++)
	{
	    if ((curimmune % 2) == 1)
	    {
		switch (i)
		{
		    case RESIST_HOLY:
		    case RESIST_NEGATIVE:
		    case RESIST_BASH:
		    case RESIST_PIERCE:
		    case RESIST_SLASH:
		    case RESIST_FIRE:
		    case RESIST_COLD:
		    case RESIST_LIGHTNING:
		    case RESIST_ACID:
            case RESIST_FEAR:
            case RESIST_DEFILEMENT:
		    case RESIST_POISON:
		    case RESIST_ENERGY:
		    case RESIST_DISEASE:
		    case RESIST_LIGHT:
		    case RESIST_SOUND:
		    {
			if (number_percent() < (get_skill(ch, gsn_tan) / 2))
			{
		    	    af.where     = TO_OBJECT;
		    	    af.type	 = gsn_tan;
		    	    af.level	 = ch->level;
		    	    af.duration  = -1;
		    	    af.location  = (FIRST_APPLY_RESIST + i);
		    	    af.modifier  = (1 + ((corpse->level - 40) / 15));
		    	    if (number_percent() < (get_skill(ch, gsn_tan) / 2))
				af.modifier += 1;
		    	    af.bitvector = 0;
		    	    affect_to_obj(hide, &af);
		    	    break;
			}
		    }
		    default: break;
		}
	    }
	    curimmune = (curimmune >> 1); 
	}
    }

    if (pMobIndex && number_percent() < get_skill(ch,gsn_tan)
	&& (IS_SET(pMobIndex->act, ACT_BADASS) 
	  || pMobIndex->race == race_lookup("demon")
	  || pMobIndex->race == race_lookup("celestial")
	  || pMobIndex->race == race_lookup("seraph")
	  || pMobIndex->race == race_lookup("dragon")))
    {
        af.where	 = TO_OBJECT;
        af.type	 = gsn_tan;
        af.level	 = ch->level;
        af.duration  = -1;
        af.location  = APPLY_SAVES;
	af.modifier  = corpse->level > 50 ? -2 : -1;
        af.bitvector = 0;
        affect_to_obj(hide, &af);
    }

    if (number_percent() < get_skill(ch, gsn_tan))
    {
        af.where	 = TO_OBJECT;
        af.type	 = gsn_tan;
	af.level	 = ch->level;
	af.duration  = -1;
	af.location  = APPLY_HIT;
	af.modifier  = corpse->pIndexData->vnum == ITEM_CORPSE_NPC
			? number_range(corpse->level/8,corpse->level/4)
			: number_range(corpse->level/7,corpse->level/3);
	af.bitvector = 0;
	affect_to_obj(hide, &af);
    }

    if (number_percent() < (corpse->level * get_skill(ch, gsn_tan)))
    { 
        af.where      = TO_OBJECT;
        af.type       = gsn_tan;
        af.level      = ch->level;
        af.duration   = -1;
        af.location   = number_bits(1) == 0 ? APPLY_HITROLL : APPLY_DAMROLL;
	af.modifier  = corpse->pIndexData->vnum == ITEM_CORPSE_NPC
			? corpse->level/20
			: corpse->level/17;
        af.bitvector  = 0;
        affect_to_obj(hide, &af);
    }

    if (number_percent() < get_skill(ch, gsn_tan)
      && corpse->level > 25
      && (corpse->value[2] == race_lookup("dragon")
	|| corpse->value[2] == race_lookup("bear")
	|| corpse->value[2] == race_lookup("giant")
	|| corpse->value[2] == global_int_race_alatharya
	|| corpse->value[2] == global_int_race_chaja
	|| corpse->value[2] == race_lookup("wyvern")))
    {
        af.where      = TO_OBJECT;
        af.type       = gsn_tan;
        af.level      = ch->level;
        af.duration   = -1;
        af.location   = APPLY_STR;
        af.modifier   = 1;
        af.bitvector  = 0;
        affect_to_obj(hide, &af);
    }
    
    if (number_percent() < get_skill(ch, gsn_tan)
      && corpse->level > 25
      && (corpse->value[2] == race_lookup("wolf")
	|| corpse->value[2] == global_int_race_shuddeni
	|| corpse->value[2] == global_int_race_srryn
	|| corpse->value[2] == global_int_race_kankoran
	|| corpse->value[2] == global_int_race_aelin
	|| corpse->value[2] == global_int_race_nefortu
	|| corpse->value[2] == race_lookup("bird")
	|| corpse->value[2] == race_lookup("wyvern")))
    {
        af.where      = TO_OBJECT;
        af.type       = gsn_tan;
        af.level      = ch->level;
        af.duration   = -1;
        af.location   = APPLY_DEX;
        af.modifier   = 1;
        af.bitvector  = 0;
        affect_to_obj(hide, &af);
    }

    if (number_percent() < get_skill(ch, gsn_tan)
      && corpse->level > 25
      && (corpse->value[2] == global_int_race_alatharya
	|| corpse->value[2] == global_int_race_nefortu
	|| corpse->value[2] == global_int_race_chaja
	|| corpse->value[2] == global_int_race_ethron
	|| corpse->value[2] == race_lookup("demon")))
    {
        af.where      = TO_OBJECT;
        af.type       = gsn_tan;
        af.level      = ch->level;
        af.duration   = -1;
        af.location   = APPLY_CON;
        af.modifier   = 1;
        af.bitvector  = 0;
        affect_to_obj(hide, &af);
    }
    
    if (number_percent() < get_skill(ch, gsn_tan)
      && corpse->level > 25
      && (corpse->value[2] == global_int_race_chtaren
	|| corpse->value[2] == global_int_race_shuddeni
	|| corpse->value[2] == global_int_race_aelin
	|| corpse->value[2] == global_int_race_caladaran))
    {
        af.where      = TO_OBJECT;
        af.type       = gsn_tan;
        af.level      = ch->level;
        af.duration   = -1;
        af.location   = APPLY_MANA;
        af.modifier   = corpse->pIndexData->vnum == ITEM_CORPSE_NPC
			? number_range(corpse->level/8,corpse->level/4)
			: number_range(corpse->level/7,corpse->level/3);
        af.bitvector  = 0;
        affect_to_obj(hide, &af);
    }
    
    if (number_percent() < get_skill(ch, gsn_tan)
      && corpse->level > 25
      && (corpse->value[2] == global_int_race_kankoran
	|| corpse->value[2] == global_int_race_aelin
	|| corpse->value[2] == global_int_race_nefortu
	|| corpse->value[2] == race_lookup("rabbit")
	|| corpse->value[2] == race_lookup("bird")))
    {
        af.where      = TO_OBJECT;
        af.type       = gsn_tan;
        af.level      = ch->level;
        af.duration   = -1;
        af.location   = APPLY_MOVE;
        af.modifier   = corpse->pIndexData->vnum == ITEM_CORPSE_NPC
			? number_range(corpse->level/8,corpse->level/4)
			: number_range(corpse->level/7,corpse->level/3);
        af.bitvector  = 0;
        affect_to_obj(hide, &af);
    }
    
    if (hide->lore)
	free_string(hide->lore);

    sprintf(buf, "The handiwork of this tanning appears to have been done by %s.", IS_NPC(ch) ? ch->short_descr : ch->name);
    hide->lore = str_dup(buf);

    act("You craft clothing from $p!", ch, corpse, NULL, TO_CHAR);
    act("$n crafts clothing from $p!", ch, corpse, NULL, TO_ROOM);
    check_improve(ch,NULL,gsn_tan,TRUE,2);
    expend_mana(ch, skill_table[gsn_tan].min_mana);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_tan].beats));

    af.where      = TO_AFFECTS;
    af.type       = gsn_tan;
    af.level      = ch->level;
    af.duration   = 5;
    af.location   = APPLY_NONE;
    af.modifier   = 0;
    af.bitvector  = 0;
    affect_to_char(ch, &af);

    desiccate_corpse(corpse);

    obj_to_char(hide, ch);

}

 

void do_taint( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int type=0;
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim=NULL;
    OBJ_DATA *obj=NULL;
    OBJ_DATA *vial=NULL;
    int chance;
    int mod(1);

    obj = NULL;

    if ((chance = get_skill(ch,gsn_taint)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Poison what with which poison?\n\r", ch );
	return;
    }

    if ((obj = get_obj_list( ch, arg1, ch->in_room->contents)) == NULL)
	if ((obj = get_obj_list(ch, arg1, ch->carrying)) == NULL)
	{
	    send_to_char( "You don't see that object here.\n\r", ch );
	    return;
        }

    if ((vial = get_obj_list(ch, arg2, ch->carrying)) == NULL)
    {
	send_to_char("You don't seem to have that poison.\n\r",ch);
	return;
    }

    if (!obj->item_type == ITEM_FOOD)
    {
	send_to_char("You can't taint that with poison.\n\r",ch);
	return;
    }

    for (type = 0;poison_table[type].spell_fun;type++)
	if (obj_is_affected(obj,*(poison_table[type].sn)))
	    break;

    if (poison_table[type].spell_fun != NULL)
    {
	act("That item is already tainted.",ch,obj,NULL,TO_CHAR);
	return;
    }

    if (vial->pIndexData->vnum != OBJ_VNUM_POISON_VIAL)
    {
	send_to_char("You can't poison anything with that.\n\r",ch);
	return;
    }

    if (ch->mana < skill_table[gsn_taint].min_mana)
    {
	send_to_char("You don't have enough energy for skilled poisoning of food.\n\r", ch);
	return;
    }

    for (type = 0;poison_table[type].spell_fun;type++)
	if (obj_is_affected(vial,*(poison_table[type].sn)))
	    break;

    if (poison_table[type].spell_fun == NULL)
    {
	act("That vial contains no poison.",ch,obj,NULL,TO_CHAR);
	return;
    }
    
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_taint].beats));
    extract_obj(vial);

    if (number_percent() > chance)
    {
	send_to_char("You spill the poison on the ground.\n\r", ch);
	expend_mana(ch, skill_table[gsn_taint].min_mana/2);
    	check_improve(ch,victim,gsn_taint,FALSE,2);
	return;
    }

    expend_mana(ch, skill_table[gsn_taint].min_mana);
    check_improve(ch,victim,gsn_taint,TRUE,2);
    sprintf(buf,"You lace $p with %s poison.", poison_table[type].name);
    act(buf,ch,obj,NULL,TO_CHAR);

    af.where     = TO_OBJECT;
    af.type      = *(poison_table[type].sn);
    af.level     = ch->level;
    af.duration  = mod*6;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_obj(obj,&af);
}

/*
 * Shopping commands.
 */
CHAR_DATA *find_keeper( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *keeper;
    SHOP_DATA *pShop;

    pShop = NULL;
    for ( keeper = ch->in_room->people; keeper; keeper = keeper->next_in_room )
    {
	if ( IS_NPC(keeper) && (pShop = keeper->pIndexData->pShop) != NULL )
	    break;
    }

    if ( pShop == NULL )
    {
	send_to_char( "You can't do that here.\n\r", ch );
	return NULL;
    }

    if ( !IS_NPC(ch) && IS_CRIMINAL(ch) && (keeper->clan != ch->clan || keeper->clan != clan_lookup("RAIDER")) && keeper->in_room->room_flags & ROOM_LAW )
    {
	do_say( keeper, "Criminals are not welcome!" );
	sprintf( buf, "%s the criminal is over here!", ch->name );
	do_yell( keeper, buf );
	return NULL;
    }

    /*
     * Shop hours.
     */
    if (!IS_IMMORTAL(ch) && (pShop->open_hour < pShop->close_hour))
    {
      if ( time_info.hour < pShop->open_hour )
      {
	  sprintf(buf, "Sorry, I am closed. Come back at %d:00%s.", 
	    (pShop->open_hour % (NUM_HOURS_DAY / 2) == 0) ? (NUM_HOURS_DAY / 2) : (pShop->open_hour % (NUM_HOURS_DAY / 2)), 
	    (pShop->open_hour >= (NUM_HOURS_DAY / 2)) ? "pm" : "am");
	  do_say( keeper, buf );
	  return NULL;
      }
    
      if ( time_info.hour > pShop->close_hour )
      {
	  sprintf(buf, "Sorry, I am closed. Come back at %d:00%s.", 
	    (pShop->open_hour % (NUM_HOURS_DAY / 2) == 0) ? (NUM_HOURS_DAY / 2) : (pShop->open_hour % (NUM_HOURS_DAY / 2)), 
	    (pShop->open_hour >= (NUM_HOURS_DAY / 2)) ? "pm" : "am");
	  do_say( keeper, buf );
	  return NULL;
      }
    }
    else
    {
      if (!IS_IMMORTAL(ch) && (time_info.hour < pShop->open_hour && time_info.hour >
pShop->close_hour) )
      {
	  sprintf(buf, "Sorry, I am closed. Come back at %d:00%s.", 
	    (pShop->open_hour % (NUM_HOURS_DAY / 2) == 0) ? (NUM_HOURS_DAY / 2) : (pShop->open_hour % (NUM_HOURS_DAY / 2)), 
	    (pShop->open_hour >= (NUM_HOURS_DAY / 2)) ? "pm" : "am");
	  do_say( keeper, buf );
	  return NULL;
      }
    }

    /*
     * Invisible or hidden people.
     */
    if ( !can_see( keeper, ch ) )
    {
	do_say( keeper, "I don't trade with folks I can't see." );
	return NULL;
    }

    return keeper;
}

/* insert an object at the right spot for the keeper */
void obj_to_keeper( OBJ_DATA *obj, CHAR_DATA *ch )
{
    OBJ_DATA *t_obj, *t_obj_next;

    /* see if any duplicates are found */
    for (t_obj = ch->carrying; t_obj != NULL; t_obj = t_obj_next)
    {
	t_obj_next = t_obj->next_content;

	if (obj->pIndexData == t_obj->pIndexData 
	&&  !str_cmp(obj->short_descr,t_obj->short_descr))
	{
	    /* if this is an unlimited item, destroy the new one */
	    if (IS_OBJ_STAT(t_obj,ITEM_INVENTORY))
	    {
		extract_obj(obj);
		return;
	    }
	    obj->cost = t_obj->cost; /* keep it standard */
	    break;
	}
    }

    if (t_obj == NULL)
    {
	obj->next_content = ch->carrying;
	ch->carrying = obj;
    }
    else
    {
	obj->next_content = t_obj->next_content;
	t_obj->next_content = obj;
    }

    obj->carried_by      = ch;
    obj->in_room         = NULL;
    obj->in_obj          = NULL;
    ch->carry_number    += get_obj_number( obj );
    ch->carry_weight    += get_obj_weight( obj );
}

/* get an object from a shopkeeper's list */
OBJ_DATA *get_obj_keeper( CHAR_DATA *ch, CHAR_DATA *keeper, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;
 
    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = keeper->carrying; obj != NULL; obj = obj->next_content )
    {
//        if (obj->wear_loc == WEAR_NONE

	if (!obj->worn_on
        &&  can_see_obj( keeper, obj )
	&&  can_see_obj(ch,obj)
        &&  is_name( arg, obj->name ) )
        {
            if ( ++count == number )
                return obj;
	
	    /* skip other objects of the same name */
	    while (obj->next_content != NULL
	    && obj->pIndexData == obj->next_content->pIndexData
	    && !str_cmp(obj->short_descr,obj->next_content->short_descr))
		obj = obj->next_content;
        }
    }
 
    return NULL;
}

long base_cost(OBJ_DATA *obj)
{
    long cost;

    cost = obj->cost / 20;

    if (cost > 10000)
	cost = 10000 + ((cost - 10000) / 40);

    if (cost > 15000)
	cost = 15000 + ((cost - 15000) / 60);

    return cost;
}

long get_cost( CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy )
{
    SHOP_DATA *pShop;
    long cost;
    bool found = FALSE;
	
    if ( obj == NULL || ( pShop = keeper->pIndexData->pShop ) == NULL )
	return 0;

    if ( fBuy )
    {
	cost = obj->cost * pShop->profit_buy / 100;
    }
    else
    {
//	OBJ_DATA *obj2;
	int itype;

	cost = base_cost(obj);

	for ( itype = 0; itype < MAX_TRADE; itype++ )
	{
	    if ( obj->item_type == pShop->buy_type[itype] )
	    {
		cost = cost * pShop->profit_sell / 100;
		found = TRUE;
		break;
	    }
	}

	if (!found)
	    return 0;
    }

    if ( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND )
    {
	if (obj->value[1] == 0)
	    cost = 0;
	else
	    cost = cost * obj->value[2] / (obj->value[1] * 2);
    }

    return cost;
}


void do_sentry( CHAR_DATA *ch, char *argument)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
int skill;
int cost;

    if ((skill = get_skill(ch,gsn_sentry)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
 
    if (is_affected(ch, gsn_sentry))
    {
	send_to_char("You can't set up another sentry yet.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_sentry].beats));
	return;
    }

    if (ch->in_room->room_flags & ROOM_NO_RECALL)
    {
	send_to_char("The Office of Records will not post a sentry here.\n\r",ch);
	return;
    }
    if (number_percent() > skill) 
    {
        check_improve(ch,NULL,gsn_sentry,FALSE,1);
	send_to_char("The Office of Records denies your request.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_sentry].beats));
        return;
    }

    cost = ch->in_room->sector_type == SECT_CITY ? 1000 : 1500;
    if (!dec_player_bank(ch, value_to_coins(cost,FALSE)))
	if (!dec_player_coins(ch, value_to_coins(cost, FALSE)))
	{
	    send_to_char("It is unwise to so engage the Office of Records without sufficient payment.\n\r", ch);
	    return;
        }

    check_improve(ch,NULL,gsn_sentry,TRUE,1);
    send_to_char("You notify the Office of Records to monitor this location.\n\r", ch);
    
    af.where     = TO_AFFECTS;
    af.type      = gsn_sentry;
    af.level     = ch->level;
    af.duration  = 3;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch,&af);

    af.where = TO_ROOM;
    af.duration = 19;
    af.modifier = ch->id;
    affect_to_room(ch->in_room,&af);
}

void do_findcover( CHAR_DATA *ch, char *argument)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
ROOM_INDEX_DATA *proom;
int skill;
int direction;

    if ((skill = get_skill(ch,gsn_findcover)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (is_affected(ch, gsn_findcover))
    {
	send_to_char("You can't find cover again yet.\n\r", ch);
	return;
    }

    if (ch->in_room->sector_type != SECT_CITY 
      || ch->in_room->room_flags & ROOM_INDOORS) 
    {
	send_to_char("You can't slip between buildings here.\n\r", ch);
	return;
    }
   
    if (argument[0] == '\0')
    {
	send_to_char("In which direction do you want to look for cover?\n\r",ch);
	return;
    }
 
    if ( !str_prefix( argument, "north" ) ) direction = 0;
    else if (!str_prefix (argument, "east") ) direction = 1;
    else if (!str_prefix (argument, "south") ) direction = 2;
    else if (!str_prefix (argument, "west") ) direction = 3;
    else
    {
	send_to_char("You find no cover in that direction.\n\r",ch);
	return;
    }

    if (ch->in_room->exit[direction])
    {
        send_to_char("There is already an opening in that direction.\n\r", ch);
        return;
    }

    if (number_percent() > skill) 
    {
        check_improve(ch,NULL,gsn_findcover,FALSE,1);
	send_to_char("You fail to find cover.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_findcover].beats));
        return;
    }

    proom = new_room_area(ch->in_room->area);
    proom->name = str_dup("A Space Between Buildings");
    proom->description = str_dup("High walls between buildings form this short alley. The space is cramped\n\rand offers little comfort. The buildings block most of the light from\n\rthe street, casting the alleyway in drab shadows.\n\r");
    proom->room_flags = ROOM_NOSUM_TO|ROOM_NOWHERE;
    proom->sector_type = SECT_CITY;

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL))
	SET_BIT(proom->room_flags, ROOM_NO_RECALL);

    proom->exit[OPPOSITE(direction)] = new_exit();
    proom->exit[OPPOSITE(direction)]->orig_door = OPPOSITE(direction);
    proom->exit[OPPOSITE(direction)]->u1.to_room = ch->in_room;

    af.where     = TO_ROOM_AFF;
    af.type      = gsn_findcover;
    af.level     = ch->level;
    af.duration  = 10;
    af.location  = 0;
    af.modifier  = ch->in_room->vnum;
    af.bitvector = 0;
    affect_to_room(proom,&af);

    af.where     = TO_AFFECTS;
    af.duration	 = 30;
    af.location  = 0;
    affect_to_char(ch,&af);

    check_improve(ch,NULL,gsn_findcover,TRUE,1);
    
    send_to_char("You slip between two buildings.\n\r", ch);
    act("$n slips between two buildings, vanishing into the space between.", ch, NULL, NULL, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch,proom);
    do_look(ch,"auto");
//    move_char(ch,direction,FALSE);
}



DO_FUNC( do_retainer )
{
    CHAR_DATA *retainer;
    char buf[MAX_STRING_LENGTH],name[MAX_INPUT_LENGTH],sex[MAX_INPUT_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_NPC(ch) || !BIT_GET(ch->pcdata->traits, TRAIT_RETAINER))
    {
	send_to_char("You do not a have loyal retainer to call upon.\n\r", ch);
	return;
    }
    
    if (ch->level < 10)
    {
	send_to_char("You are not yet powerful enough to call upon your retainer.\n\r", ch);
	return;
    }

    if (is_affected(ch,gsn_retainercall))
    {
	send_to_char("You are not yet able to call your loyal retainer again.\n\r", ch);
	return;
    }

    if (ch->pet)
    {
	send_to_char("You already have a pet.\n\r", ch);
	return;
    }

    if (argument[0] != '\0')
    {
	argument = one_argument(argument,name);

	if (!str_cmp(name,"set"))
    	{
	    if (ch->pcdata->retainername)
	    {
	    	send_to_char("You have already chosen a name and sex for your retainer.\n\r",ch);
	    	return;
	    }
	    argument = one_argument(argument,name);
	    one_argument(argument,sex);
	    if (sex[0] == '\0' 
	      || name[0] == '\0') 
	    {
		send_to_char("What name and sex did you want to give your retainer?\n\r",ch);
		return;
	    }
	    int m;
	    for (m = 0;m < MAX_GODS;m++)
	        if (!str_cmp(god_table[m].name,name))
		{
		    send_to_char("Don't do that.\n\r",ch);
		    sprintf(buf,"DUMBASS: I just tried to set my retainer name to %s. Maybe I shouldn't have them anymore.",name);
		    do_pray(ch,buf);
		    return;
		} 
	    if (!str_cmp(sex,"male"))
		ch->pcdata->retainergender = SEX_MALE;
	    else if (!str_cmp(sex,"female"))
		ch->pcdata->retainergender = SEX_FEMALE;
	    else
	    {
		send_to_char("Please choose male or female for your retainer's gender.\n\r",ch);
		return;
	    }
	    name[0] = UPPER(name[0]);
	    ch->pcdata->retainername = str_dup(name);
	    sprintf(buf,"Your retainer's name has been set to %s and its gender is %s.\n\r",name,ch->pcdata->retainergender == SEX_MALE ? "male" : "female");
	    
	}
	else
	{
	    if (ch->pcdata->retainername == NULL 
	      || ch->pcdata->retainergender == 0)
	    {
		send_to_char("First use syntax: retainer set [name] [gender]\n\rLater uses: retainer\n\r",ch);
		return;
	    }
	}
    }
    else
	if (ch->pcdata->retainername == NULL 
	  || ch->pcdata->retainergender == 0)
	{
	    send_to_char("First use syntax: retainer set [name] [gender]\n\rLater uses: retainer\n\r",ch);
	    return;
	}
    
    retainer = create_mobile(get_mob_index(MOB_VNUM_RETAINER));

// Disabled this so people can't tell character alignment from retainer
//    retainer->alignment = ch->alignment;

    levelize_mobile(retainer, ch->level);

    sprintf(buf, retainer->description, ch->name, ch->name);
    free_string(retainer->description);
    retainer->description = str_dup(buf);
    sprintf(buf,retainer->name,ch->pcdata->retainername);
    setName(*retainer, buf);
    retainer->sex = ch->pcdata->retainergender;
    sprintf(buf,retainer->short_descr,ch->pcdata->retainername);
    free_string(retainer->short_descr);
    retainer->short_descr = str_dup(buf);
    sprintf(buf,retainer->long_descr,ch->pcdata->retainername,pc_race_table[retainer->race].name,ch->name);
    free_string(retainer->long_descr);
    retainer->long_descr = str_dup(buf);
    if (retainer->sex == SEX_FEMALE)
    {
	sprintf(buf,"This woman is a loyal retainer, pledged to the service of %s. While she\n\rdoes not carry herself with the casual grace of an adventurer, she has\n\rclearly been trained as a woman-at-arms. She serves loyally and faithfully,\n\rbound by oath and years spent following %s.",ch->name,ch->name);
	free_string(retainer->description);
	retainer->description = str_dup(buf);
    }
    retainer->lang_flags = (1 << race_table[ch->race].native_tongue) | 1;

    char_to_room( retainer, ch->in_room );

    send_to_char("You call your loyal retainer to service!\n\r", ch);
    act("$n calls $s loyal retainer to service!", ch, NULL, NULL, TO_ROOM);
    act("$n has arrived.", retainer, NULL, NULL, TO_ROOM);

    add_follower( retainer, ch );
    retainer->leader = ch;
    ch->pet = retainer;
    af.type = gsn_retainercall;
    af.modifier = 0;
    af.duration = 99;
    af.bitvector = 0;
    af.level = ch->level;
    af.where = TO_AFFECTS;
    af.location = APPLY_HIDE;
    affect_to_char(ch,&af);
}

size_t calculate_pet_price(size_t level)
{
	return static_cast<size_t>(600 * pow(2, level / 10.0));
}

int letsmakeadeal(CHAR_DATA *ch, CHAR_DATA *keeper, int cost, bool buy)
{
    int roll, costmod=100;
    bool haggle=FALSE, bargain=FALSE;

    roll = number_percent();
// This will probably turn into bargain but maintain the status quo for now */
    if (ch->class_num == global_int_class_alchemist && ch->level >= 20)
	costmod = 90;
    /* haggle */
    if (roll <= get_skill(ch,gsn_haggle))
    {
        costmod = (100 - (get_curr_stat(ch, STAT_CHR) * roll / 100));
	haggle=TRUE;
        check_improve(ch,NULL,gsn_haggle,TRUE,4);
    }

    if (roll <= get_skill(ch,gsn_bargain))
    {
	costmod = (costmod * 90 / 100);
	bargain = TRUE;
	check_improve(ch,NULL,gsn_bargain,TRUE,4);
    }

    // Check luck
    if (Luck::Check(*ch) == Luck::Lucky)
        costmod = (costmod * 9) / 10;

    if (buy)
	cost = cost * costmod/100;
    else
	cost = cost * (200-costmod)/100;
    
    if (haggle && bargain)
        if (keeper)
	    act("You persuade $N to give you a great deal.",ch,NULL,keeper,TO_CHAR);
	else
	    send_to_char("You drive the price down with a bit of bargaining.\n\r",ch);
    else if (haggle)
	act("You haggle over the price.",ch,NULL,NULL,TO_CHAR);
    else if (bargain)
	if (keeper)
	    act("You persuade $N to give you a discount.",ch,NULL,keeper,TO_CHAR);
	else
	    send_to_char("You drive the price down with a bit of bargaining.\n\r",ch);

    return cost;
}

void do_buy( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    long cost;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Buy what?\n\r", ch );
	return;
    }

    if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM))
    {
	send_to_char( "You are too enamored with your master to go buy things right now.\n\r", ch);
	return;
    }

    if ( IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP) )
    {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *pet;
	ROOM_INDEX_DATA *pRoomIndexNext;
	ROOM_INDEX_DATA *in_room;

	smash_tilde(argument);

	if ( IS_NPC(ch) )
	    return;

	argument = one_argument(argument,arg);

	/* hack to make new thalos pets work */
	if (ch->in_room->vnum == 9621)
	    pRoomIndexNext = get_room_index(9706);
	else
	    pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
	if ( pRoomIndexNext == NULL )
	{
	    bug( "Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum );
	    send_to_char( "Sorry, you can't buy that here.\n\r", ch );
	    return;
	}

	in_room     = ch->in_room;
	ch->in_room = pRoomIndexNext;
	pet         = get_char_room( ch, arg );
	ch->in_room = in_room;

	if ( pet == NULL || !IS_SET(pet->act, ACT_PET) )
	{
	    send_to_char( "Sorry, you can't buy that here.\n\r", ch );
	    return;
	}

	if ( ch->pet != NULL )
	{
	    send_to_char("You already own a pet.\n\r",ch);
	    return;
	}

 	cost = calculate_pet_price(pet->level);

	if ( coins_to_value(ch->coins) < cost )
	{
	    send_to_char( "You can't afford it.\n\r", ch );
	    return;
	}

	if ( ch->level < pet->level )
	{
	    send_to_char(
		"You're not powerful enough to master this pet.\n\r", ch );
	    return;
	}
	
	cost = letsmakeadeal(ch,NULL,cost,TRUE);

        sprintf(buf,"You make your purchase for %s.\n\r", coins_to_str(value_to_coins((float) cost, FALSE)));
        send_to_char(buf,ch);
	deduct_cost(ch, (float) cost);
	pet			= create_mobile( pet->pIndexData );
	SET_BIT(pet->act, ACT_PET);
	SET_BIT(pet->affected_by, AFF_CHARM);
	pet->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;

	argument = one_argument( argument, arg );
	if ( arg[0] != '\0' )
	{
	    sprintf( buf, "%s %s", pet->name, arg );
        setName(*pet, buf);
	}

	sprintf( buf, "%sA neck tag says, 'I belong to %s'.\n\r",
	    pet->description, ch->name );
	free_string( pet->description );
	pet->description = str_dup( buf );

	char_to_room( pet, ch->in_room );
	add_follower( pet, ch );
	pet->leader = ch;
	ch->pet = pet;
	send_to_char( "Enjoy your pet.\n\r", ch );
	act( "$n bought $N as a pet.", ch, NULL, pet, TO_ROOM );
	return;
    }
    else
    {
	CHAR_DATA *keeper;
	OBJ_DATA *obj,*t_obj;
	char arg[MAX_INPUT_LENGTH];
	int number, count = 1;

	if ( ( keeper = find_keeper( ch ) ) == NULL )
	    return;

	number = mult_argument(argument,arg);
	obj  = get_obj_keeper( ch,keeper, arg );
	cost = get_cost( keeper, obj, TRUE );

	if (number < 1)
	{
	    act("$n tells you '",keeper,NULL,ch,TO_VICT);
	    return;
	}

	if ( cost <= 0 || !can_see_obj( ch, obj ) )
	{
	    act( "$n tells you 'I don't sell that -- try 'list''.",
		keeper, NULL, ch, TO_VICT );
	    ch->reply = keeper;
	    return;
	}

	if (!IS_OBJ_STAT(obj,ITEM_INVENTORY))
	{
	    for (t_obj = obj->next_content;
	     	 count < number && t_obj != NULL; 
	     	 t_obj = t_obj->next_content) 
	    {
	    	if (t_obj->pIndexData == obj->pIndexData
	    	&&  !str_cmp(t_obj->short_descr,obj->short_descr))
		    count++;
	    	else
		    break;
	    }

	    if (count < number)
	    {
	    	act("$n tells you 'I don't have that many in stock.",
		    keeper,NULL,ch,TO_VICT);
	    	ch->reply = keeper;
	    	return;
	    }
	}

	if ( coins_to_value(ch->coins) < cost * number )
	{
	    if (number > 1)
		act("$n tells you 'You can't afford to buy that many.",
		    keeper,obj,ch,TO_VICT);
	    else
	    	act( "$n tells you 'You can't afford to buy $p'.",
		    keeper, obj, ch, TO_VICT );
	    ch->reply = keeper;
	    return;
	}

	if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
         && (get_cur_capacity(ch) + item_svalue[obj->size]) > get_max_capacity(ch))
	{
	    send_to_char("You lack the room to carry that.\n\r", ch);
	    return;
        }

	if (ch->carry_number +  number   > static_cast<int>(Encumbrance::CarryCountCapacity(*ch)))
	{
	    send_to_char( "You can't carry that many items.\n\r", ch );
	    return;
	}

	if ( ch->carry_weight + number * get_obj_weight(obj) > static_cast<int>(Encumbrance::CarryWeightCapacity(*ch)))
	{
	    send_to_char( "You can't carry that much weight.\n\r", ch );
	    return;
	}

	OBJ_DATA *pc = get_obj_potioncontainer(ch);
	int additive = 0, vials;
	for (additive = OBJ_VNUM_ADD_FIRST;additive <= OBJ_VNUM_ADD_LAST;additive++)
	{
	    if (obj->pIndexData->vnum == additive)
	    {
		if (pc == NULL)
		{
		    send_to_char("You don't have anything to carry it in.\n\r",ch);
		    return;
		}
		else
		{
		    vials = pc->value[0]+pc->value[1]+pc->value[2]+pc->value[3]+pc->value[4];
		    if (vials + number > pc->weight)
		    {
			act("You can't fit any more vials in $p.",ch,pc,NULL,TO_CHAR);
			return;
		    }
		    else
			break;
		}
	    }
	}

	if (!IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT))
	    cost = letsmakeadeal(ch,keeper,cost,TRUE);

	if (IS_SET(obj->pIndexData->extra_flags[0],ITEM_INVENTORY))
   	    SET_BIT(obj->extra_flags[0],ITEM_INVENTORY); 

	if (number > 1)
	{
	    sprintf(buf,"$n buys $p[%d].",number);
	    act(buf,ch,obj,NULL,TO_ROOM);
	    sprintf(buf,"You buy $p[%d] for %s.",number, coins_to_str(value_to_coins((float) (cost * number), FALSE)));
	    act(buf,ch,obj,NULL,TO_CHAR);
	}
	else
	{
	    act( "$n buys $p.", ch, obj, NULL, TO_ROOM );
	    sprintf(buf,"You buy $p for %s.", value_to_str((float) cost));
	    act( buf, ch, obj, NULL, TO_CHAR );
	}
	deduct_cost(ch,(float) cost * number);

	if (IS_SET(keeper->nact, ACT_KEEPGOLD))
	    inc_player_coins(keeper, value_to_coins((float) cost, FALSE));

	if (obj->pIndexData->vnum >= OBJ_VNUM_ADD_FIRST 
	  && obj->pIndexData->vnum <= OBJ_VNUM_ADD_LAST && pc != NULL)
	{
	    pc->value[additive-OBJ_VNUM_ADD_FIRST] += number;
	    return;
	}

	for (count = 0; count < number; count++)
	{
	    if ( IS_SET( obj->extra_flags[0], ITEM_INVENTORY ) )
	    	t_obj = create_object( obj->pIndexData, obj->level );
	    else
	    {
		t_obj = obj;
		obj = obj->next_content;
	    	obj_from_char( t_obj );
	    }

	    if (t_obj->timer > 0 && !IS_OBJ_STAT(t_obj,ITEM_HAD_TIMER))
	    	t_obj->timer = 0;
	    REMOVE_BIT(t_obj->extra_flags[0],ITEM_HAD_TIMER);
	    obj_to_char( t_obj, ch );

		// Set this object as the last item purchased
		if (ch->pcdata != NULL)
		{
			ch->pcdata->purchased_item = t_obj;
			ch->pcdata->purchased_item_cost = cost;
			ch->pcdata->purchased_time = current_time;
		}
	}
    }
}

void do_list( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char fstr[MAX_INPUT_LENGTH];

    if ( IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP) )
    {
		ROOM_INDEX_DATA *pRoomIndexNext;
		CHAR_DATA *pet;
		bool found;

        pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );

		if (pRoomIndexNext == NULL)
		{
	    	bug( "Do_list: bad pet shop at vnum %d.", ch->in_room->vnum );
		    send_to_char( "You can't do that here.\n\r", ch );
		    return;
		}

		found = FALSE;
		size_t maxlen(0);

	// Iteration 1: determine the longest display string and whether
	// there are any pets at all
	for (pet = pRoomIndexNext->people; pet; pet = pet->next_in_room)
	{
		if (IS_SET(pet->act, ACT_PET))
		{
			if (!found)
			{
				found = true;
				send_to_char( "Pets for sale:\n\r", ch );
			}
			size_t price(calculate_pet_price(pet->level));
			const char * price_string(value_to_sstr((float)price));
			maxlen = UMAX(maxlen, strlen(price_string));
		}
	}

	// If no pets were found, just bail out now
	if (found)
	{
		// Pets were found, so perform iteration 2: listing the pets
		for (pet = pRoomIndexNext->people; pet; pet = pet->next_in_room)
		{
		    if (IS_SET(pet->act, ACT_PET))
	    	{
				size_t price(calculate_pet_price(pet->level));
				sprintf(fstr, "[%%2d] [%%%ds] %%s\n\r", maxlen);
				sprintf(buf, fstr, pet->level, value_to_sstr((float) price), pet->short_descr);
				send_to_char(buf, ch);
		    }
		}
	}
	else
	    send_to_char( "Sorry, we're out of pets right now.\n\r", ch );
	return;
    }
    else
    {
	CHAR_DATA *keeper;
	OBJ_DATA *obj;
	int cost,count;
	bool found;
	char arg[MAX_INPUT_LENGTH];
	char *tstr=NULL;
	int type=0,wearslot=0;
	unsigned int maxlen = 0;
	
	if ( ( keeper = find_keeper( ch ) ) == NULL )
	    return;

	argument = one_argument(argument,arg);

	if (!strcmp(arg,"weapon"))
	{
	    tstr = str_dup(arg);
	    one_argument(argument,arg);
	    type = weapon_type(arg);
	}
	else if (!strcmp(arg,"wear"))
	{
	    tstr = str_dup(arg);
	    one_argument(argument,arg);
	    int c;
	    for (c=0;c < ITEM_WEAR_MAX;c++)
		if (!strcmp(arg,wear_table[c].loc_name))
		    break;
	    if (c >= ITEM_WEAR_MAX 
	      || wear_table[c].wear_bit == ITEM_TAKE
	      || wear_table[c].wear_bit == ITEM_WIELD)
	    {
		send_to_char("Valid slots are: light, finger, neck, torso, head, legs, feet, hands, arms,\n\r                 shield, body, waist, wrist, hold.\n\r",ch);
		return;
	    }
		    
	    wearslot = wear_table[c].wear_bit;
	}

	for (obj = keeper->carrying; obj; obj = obj->next_content)
	    if (!obj->worn_on
	    &&   can_see_obj( ch, obj )
	    &&   ( cost = get_cost( keeper, obj, TRUE ) ) > 0 
	    &&   ( arg[0] == '\0'
	      || tstr != NULL
 	      || is_name(arg,obj->name) ))
	    	if (tstr != NULL)
		{
		    if ((obj->item_type == ITEM_WEAPON
		        && obj->value[0] == type)
		      || (wearslot > 0
			&& obj->wear_flags & wearslot))
		    	maxlen = UMAX(maxlen, strlen(value_to_sstr((float) cost)));
		}
		else
		    maxlen = UMAX(maxlen, strlen(value_to_sstr((float) cost)));   
	found = FALSE;
	for ( obj = keeper->carrying; obj; obj = obj->next_content )
	{
	    if (!obj->worn_on
	    &&   can_see_obj( ch, obj )
	    &&   ( cost = get_cost( keeper, obj, TRUE ) ) > 0 
	    &&   ( arg[0] == '\0'
	      || tstr != NULL
 	      ||  is_name(arg,obj->name) ))
	    {
		if (tstr == NULL
		  || (obj->item_type == ITEM_WEAPON
		    && obj->value[0] == type)
		  || (obj->wear_flags & wearslot))
		{   
		    if ( !found )
		    {
		    	found = TRUE;
		    	sprintf(fstr, "[Qty %%-%ds] Item\n\r", maxlen);
		    	sprintf(buf, fstr, "Price");
		    	send_to_char( buf, ch );
		    }

		    if (IS_OBJ_STAT(obj,ITEM_INVENTORY))
		    {
		    	sprintf(fstr, "[--- %%%ds] %%s\n\r", maxlen);
		    	sprintf(buf, fstr, value_to_sstr((float) cost), obj->short_descr);
		    }
		    else
		    {
		    	count = 1;

		    	while (obj->next_content != NULL 
		    	  && obj->pIndexData == obj->next_content->pIndexData
		    	  && !str_cmp(obj->short_descr,
			       obj->next_content->short_descr))
		    	{
			    obj = obj->next_content;
			    count++;
		    	}

		    	sprintf(fstr,"[%%3d %%%ds] %%s\n\r", maxlen);
		    	sprintf(buf,fstr, count, value_to_sstr((float) cost), obj->short_descr);		    
		    }
		    send_to_char( buf, ch );
	    	}
	    }
	}

	if ( !found )
	    send_to_char( "You can't buy anything here.\n\r", ch );
	return;
    }
}

void do_returnitem(CHAR_DATA * ch, char * argument)
{
	// Find a shop keeper
	CHAR_DATA * keeper(find_keeper(ch));
	char buf[MAX_INPUT_LENGTH];
	// Perform sanity checks
	if (keeper == NULL || ch->pcdata == NULL || ch->pcdata->purchased_item == NULL || ch->pcdata->purchased_item_cost <= 0)
	{
		send_to_char("You cannot return anything here.\n\r", ch);
		return;
	}

	OBJ_DATA * obj(ch->pcdata->purchased_item);
	if (!can_drop_obj(ch, obj))
	{
		send_to_char("You can't let go of it.\n\r", ch);
		return;
	}

	if (ch->pcdata->purchased_time + PULSE_TICK/4 > current_time)
	{
            sprintf(buf, "%s I will restock that after you've taken a closer look.", ch->unique_name);
	    
	    do_tell(keeper,buf);
	    return;
	}	
	if (obj->cost == 0)
	{
	    sprintf(buf, "%s I can't give you a refund on that.",ch->unique_name);
	    do_tell(keeper,buf);
	    return;
	}
	// Return the item
	int cost(ch->pcdata->purchased_item_cost);
	sprintf(buf, "You return $p for %s.", value_to_str((float) cost));
    act(buf, ch, obj, NULL, TO_CHAR);
    act("$n returns $p.", ch, obj, NULL, TO_ROOM);
	inc_player_coins(ch, value_to_coins((float) cost, FALSE));
	obj_from_char(obj);
	obj_to_keeper(obj, keeper);
	ch->pcdata->purchased_item = NULL;
	ch->pcdata->purchased_item_cost = 0;
	ch->pcdata->purchased_time = 0;
}

void do_sell( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Sell what?\n\r", ch );
	return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
	return;

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
	act( "{g$n tells you 'You don't have that item'.{x", keeper, NULL, ch, TO_VICT );
	ch->reply = keeper;
	return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
	send_to_char( "You can't let go of it.\n\r", ch );
	return;
    }

    if (!can_see_obj(keeper,obj))
    {
	act("$n doesn't see what you are offering.",keeper,NULL,ch,TO_VICT);
	return;
    }

    if (((cost = get_cost(keeper, obj, FALSE)) <= 0)
     || IS_SET(obj->extra_flags[0], ITEM_ROT_DEATH))
    {
	act( "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
	return;
    }

    if (IS_SET(obj->extra_flags[0], ITEM_AFFINITY))
    {
        sprintf(buf, "%s There is something odd about that item.  I do not wish to purchase it.", ch->unique_name);
        do_tell(keeper, buf);
        return;
    }

    if ((keeper->pIndexData->pShop->max_buy > 0) && (obj->cost > keeper->pIndexData->pShop->max_buy))
    {
	act("{g$n tells you 'I'm afraid I don't have enough to buy $p.{x", keeper,obj,ch,TO_VICT);
	return;
    }
   
    act( "$n sells $p.", ch, obj, NULL, TO_ROOM );

    cost = letsmakeadeal(ch,keeper,cost,FALSE);
    
    sprintf( buf, "You sell $p for %s.", value_to_str((float) cost));
    act( buf, ch, obj, NULL, TO_CHAR );
    inc_player_coins(ch, value_to_coins((float) cost, FALSE));
//    deduct_cost(keeper,cost);

// We don't want items with the inventory flag, when sold to a shopkeeper, to
// be purchaseable multiple times
    if (IS_SET(obj->extra_flags[0], ITEM_INVENTORY))
	REMOVE_BIT(obj->extra_flags[0],ITEM_INVENTORY);

    if ( obj->item_type == ITEM_TRASH || IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT))
    {
	extract_obj( obj );
    }
    else
    {
	obj_from_char( obj );
	if (obj->timer)
	    SET_BIT(obj->extra_flags[0],ITEM_HAD_TIMER);
	else
	    obj->timer = number_range(2160,3600);
	obj_to_keeper( obj, keeper );
    }

    return;
}



void do_value( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Value what?\n\r", ch );
	return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
	return;

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
	act( "$n tells you 'You don't have that item'.",
	    keeper, NULL, ch, TO_VICT );
	ch->reply = keeper;
	return;
    }

    if (!can_see_obj(keeper,obj))
    {
        act("$n doesn't see what you are offering.",keeper,NULL,ch,TO_VICT);
        return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
	send_to_char( "You can't let go of it.\n\r", ch );
	return;
    }

    if ( ( cost = get_cost( keeper, obj, FALSE ) ) <= 0 )
    {
	act( "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
	return;
    }

    sprintf( buf, "$n tells you 'I'll give you %s for $p'.", value_to_str((float) cost));
    act( buf, keeper, obj, ch, TO_VICT );
    ch->reply = keeper;

    return;
}

void do_appraise(CHAR_DATA *ch, char *argument)
{
    int skill;
    OBJ_DATA *obj;
    long cost, cmod;
    char buf[MAX_STRING_LENGTH];

    if ((skill = get_skill(ch, gsn_appraise)) <= 0)
    {
	send_to_char("You do not know how to properly appraise items.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("What do you wish to appraise?\n\r", ch);
	return;
    }

    if ((obj = get_obj_carry(ch, argument, ch)) == NULL)
    {
	send_to_char("You are not carrying that.\n\r", ch);
	return;
    }

    if (ch->mana < 10)
    {
	send_to_char("You are too tired to study anything at the moment.\n\r", ch);
	return;
    }

    expend_mana(ch, 10);
    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));

    cost = base_cost(obj);

    cmod = cost * (100 - skill) / 100;

    if (number_bits(1) == 0)
	cost += cmod;
    else
	cost -= cmod;

    sprintf(buf, "You estimate $p is worth %s to a merchant.", value_to_str((float) cost));
    check_improve(ch, NULL, gsn_appraise, TRUE, 3);

    act(buf, ch, obj, NULL, TO_CHAR);

    return;
}
    

void do_write(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *surface;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char one_name[MAX_INPUT_LENGTH];
    char translated[MAX_STRING_LENGTH];
    int page = 1, curlang = 0, newlang, ti = 0, dlen, i;
    EXTRA_DESCR_DATA *ed;
    LANG_STRING *trans_new;

    if (argument[0] == '\0')
    {
        send_to_char("Write on what?\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg);

    if (((surface = get_obj_carry(ch, arg, ch)) == NULL)
     && ((surface = get_obj_here(ch, arg)) == NULL))
    {
	send_to_char("There is nothing like that around to write on.\n\r", ch);
	return;
    }

    if ((surface->item_type != ITEM_WRITING) || (surface->value[0] < 1)
     || (surface->value[2] == 0))
    {
	send_to_char("That cannot be written upon.\n\r", ch);
	return;
    }
/*
    if ((ch->position != POS_RESTING) && (ch->position != POS_SITTING))
    {
	send_to_char("You must sit down first.\n\r", ch);
	return;
    }
*/
    if (surface->value[0] > 1)
    {
	argument = one_argument(argument, arg);

	if (!is_number(arg) || ((page = atoi(arg)) < 1) || (page > surface->value[0]))
	{
	     send_to_char("Invalid page number.\n\r", ch);
	     return;
	}
    }

    if (strlen(argument) > 68)
    {
	send_to_char("Line too long.\n\r", ch);
	return;
    }

    one_argument(surface->name, one_name);

    sprintf(buf, "%s%d", one_name, page);

    for (ed = surface->extra_descr; ed; ed = ed->next)
	if (!str_cmp(ed->keyword, buf))
	    break;

    if (!ed)
    {
	ed = new_extra_descr();
        ed->next = surface->extra_descr;
        surface->extra_descr = ed;

	ed->keyword = str_dup(buf);
        ed->description = str_dup("");
    }

    dlen = strlen(ed->description);

    for (i = 0; i < dlen; i++)
    {
	if ((ed->description[i] == '{') && (ed->description[i+1] == 'l'))
	{
	    i += 2;
	    ti += 3;
	    newlang = 0;
	    while (ed->description[i] != '}')
	    {
	        ti++;
		if ((ed->description[i] >= '0') && (ed->description[i] <= '9'))
		{
		    newlang *= 10;
		    newlang += ed->description[i] - '0';
		}

		i++;
		
		if (ed->description[i] == '\0')
		    return;
	    }
	    curlang = newlang;
	}
    }

    if (curlang != ch->speaking)
    {
	sprintf(buf, "%s{l%d}", ed->description, ch->speaking);
	free_string(ed->description);
	ed->description = str_dup(buf);
    }

    if ((int)(strlen(ed->description) + strlen(argument) - ti) > surface->value[1])
    {
	send_to_char("Insufficient space left on page.\n\r", ch);
	return;
    }

    trans_new = translate_spoken_new(ch, argument);
    trans_new->orig_string = argument;

    memset((void *) translated, 0, MAX_STRING_LENGTH);
    strncpy(translated, translate_out_new(ch, trans_new), 76);

    sprintf(buf, "%s%s\n\r", ed->description, translated);

    free_string(ed->description);
    ed->description = str_dup(buf);

    if (surface->value[0] == 1)
	act("You carefully write upon $p.", ch, surface, NULL, TO_CHAR);
    else
	act("You carefully write in $p.", ch, surface, NULL, TO_CHAR);

    return;
}

long repair_cost(OBJ_DATA *obj)
{
    return (long) (pow(2, obj->level / 10.0) * 750 * (100 - obj->condition) / 100);
}

DO_FUNC( do_repair )
{
    CHAR_DATA *smith = NULL, *vch;
    OBJ_DATA *weapon;
    long rcost;

    if (!ch->in_room)
	return;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	if (IS_NPC(vch) && IS_SET(vch->nact, ACT_SMITH))
	{
	    smith = vch;
	    break;
	}

    if (!smith)
    {
	send_to_char("There is no smith here.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("What do you wish to have repaired?\n\r", ch);
	return;
    }

    if ((weapon = get_obj_carry(ch, argument, ch)) == NULL)
    {
	send_to_char("You are not carrying that.\n\r", ch);
	return;
    }

    if (weapon->item_type != ITEM_WEAPON)
    {
	send_to_char("Only weapons can be repaired.\n\r", ch);
	return;
    }

    if (weapon->condition >= 100)
    {
	act("You hand $p to $N.", ch, weapon, smith, TO_CHAR);
	act("$n hands $p to $N.", ch, weapon, smith, TO_ROOM);
	act("$n examines the weapon carefully.", smith, weapon, NULL, TO_ROOM);
	do_say(smith, "This weapon is in no need of repair.");
	act("$N returns $p to you.", ch, weapon, smith, TO_CHAR);
	act("$N returns $p to $n.", ch, weapon, smith, TO_ROOM);
	return;
    }

    rcost = repair_cost(weapon);

    if (coins_to_value(ch->coins) < rcost)
    {
	send_to_char("You lack the coins to pay for the repair.\n\r", ch);
	return;
    }

    dec_player_coins(ch, value_to_coins((float) rcost, FALSE));
    weapon->condition = 100;
    act("$N quickly repairs $p, and hands it back to you.", ch, weapon, smith, TO_CHAR);
    act("$N quickly repairs $p, and hands it back to $n.", ch, weapon, smith, TO_ROOM);
}

DO_FUNC( do_estimate )
{
    CHAR_DATA *smith = NULL, *vch;
    OBJ_DATA *weapon;
    char buf[MAX_STRING_LENGTH];

    if (!ch->in_room)
	return;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	if (IS_NPC(vch) && IS_SET(vch->nact, ACT_SMITH))
	{
	    smith = vch;
	    break;
	}

    if (!smith)
    {
	send_to_char("There is no smith here.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("What do you wish to have repaired?\n\r", ch);
	return;
    }

    if ((weapon = get_obj_carry(ch, argument, ch)) == NULL)
    {
	send_to_char("You are not carrying that.\n\r", ch);
	return;
    }

    if (weapon->item_type != ITEM_WEAPON)
    {
	send_to_char("Only weapons can be repaired.\n\r", ch);
	return;
    }

    act("You hand $p to $N.", ch, weapon, smith, TO_CHAR);
    act("$n hands $p to $N.", ch, weapon, smith, TO_ROOM);
    act("$n examines the weapon carefully.", smith, weapon, NULL, TO_ROOM);

    if (weapon->condition >= 100)
    {
	do_say(smith, "This weapon is in no need of repair.");
	return;
    }

    sprintf(buf, "This will cost %s to repair.", value_to_str((float) repair_cost(weapon)));
    do_say(smith, buf);

    act("$N returns $p to you.", ch, weapon, smith, TO_CHAR);
    act("$N returns $p to $n.", ch, weapon, smith, TO_ROOM);
    return;

}

DO_FUNC( do_accept )
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->nact, PLR_NOACCEPT))
    {
	send_to_char("You will now accept items given to you.\n\r", ch);
	REMOVE_BIT(ch->nact, PLR_NOACCEPT);
    }
    else
    {
	send_to_char("You will no longer accept items given to you.\n\r", ch);
	SET_BIT(ch->nact, PLR_NOACCEPT);
    }

    return;
}

DO_FUNC( do_disfigure )
{
    OBJ_DATA *vial;
    OBJ_DATA *corpse;

    if (ch->class_num != global_int_class_assassin)
    {
	send_to_char("You don't know how to disfigure corpses with acid.\n\r",ch);
	return;
    }
    if (((vial = get_eq_char(ch, WEAR_HOLD)) == NULL)
      || vial->pIndexData->vnum != OBJ_VNUM_POISON_VIAL
      || !obj_is_affected(vial,gsn_erosivepoison))
    {
	send_to_char("You must be holding a vial of erosive poison to disfigure a corpse.\n\r",ch);
	return;  
    }

    if ((corpse = get_obj_list(ch,argument,ch->in_room->contents)) == NULL)
    {
	send_to_char("You don't see that here.\n\r",ch);
	return;
    }
    if (!(corpse->pIndexData->vnum == OBJ_VNUM_CORPSE_PC
      || corpse->pIndexData->vnum == OBJ_VNUM_CORPSE_NPC))
    {
	send_to_char("That's not a corpse.\n\r",ch);
	return;
    }

    act("You pour acid over $p, removing its identifying features.",ch,corpse,NULL,TO_CHAR);
    act("$n pours acid over $p, removing its identifying features.",ch,corpse,NULL,TO_ROOM);

    corpse->timer /= 2;
    setName(*corpse, "corpse");
    free_string(corpse->short_descr);
    corpse->short_descr = str_dup("a corpse");
    free_string(corpse->description);
    corpse->description = str_dup("A corpse is here.");
    extract_obj(vial);
}
