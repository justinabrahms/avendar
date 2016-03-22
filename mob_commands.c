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
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy........    N'Atas-Ha *
 ***************************************************************************/
/***************************************************************************
 * More prog stuff added by Ashur of Avendar for LOFT. See README.LOFT
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <sstream>
#include "merc.h"
#include "magic.h"
#include "interp.h"
#include "tables.h"
#include "recycle.h"
#include "olc.h"
#include "demons.h"
#include "RoomPath.h"
#include "fight.h"
#include "spells_fire.h"
#include "lookup.h"
#include "Faction.h"
#include "NameMaps.h"
#include "Encumbrance.h"

DECLARE_DO_FUN( do_mpcast    );
DECLARE_DO_FUN( do_mpset    );
DECLARE_DO_FUN( do_mpsetskills );
DECLARE_DO_FUN( do_mpvaultdrop );
DECLARE_ODO_FUN( do_opmset   );
DECLARE_ODO_FUN( do_oposet   );
DECLARE_ODO_FUN( do_oprset   );
DECLARE_ODO_FUN( do_opsset   );
DECLARE_DO_FUN( do_transfer );
DECLARE_DO_FUN( do_set      );

/* from fight.c */
extern	void	dam_message	args( ( CHAR_DATA *ch, CHAR_DATA *victim,
					int dam,int dt, char *attack, bool immune ) );
extern	void	set_fighting	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );

/* from act_info.c */
extern	char *	format_obj_to_char args( ( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort ) );

/* from act_obj.c */
extern	int	hands_free	args( ( CHAR_DATA *ch ) );
extern	bool 	remove_obj_slot args( ( CHAR_DATA *ch, int slot, bool fReplace ) );
/* from flags.c */
int flag_lookup args( ( const char *name, const struct flag_type *flag_table) );

/* from act_move.c */
int find_door   args( ( CHAR_DATA *ch, char *arg ) );

/* from act_wiz.c */
void	rget	args( ( ROOM_INDEX_DATA * obj, char * arg2, char *argument, int *memval) );
void	oget	args( ( OBJ_DATA * obj, char *argument, int *memval) );
void	mget	args( ( CHAR_DATA *victim, char *argument, int *memval) );
void	sget	args( ( CHAR_DATA *victim, char *argument, int *memval ) );

extern	bool	verb_stop_issued;
extern	int	prog_dam_val;

int	hitprog_damage;

extern	void	change_faction	args((CHAR_DATA *victim, unsigned int fnum, short fval, bool display));

extern	void	fwrite_rumors();

extern bool mprog_driver(char* com_list, CHAR_DATA* mob, CHAR_DATA* actor, OBJ_DATA* obj, ROOM_INDEX_DATA *room, void * vo, char * txt);

/*
 * Local functions.
 */

char *		mprog_type_to_name	args( ( int type ) );

/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. It allows the words to show up in mpstat to
 *  make it just a hair bit easier to see what a mob should be doing.
 */

char *mprog_type_to_name( int type )
{
    switch ( type )
    {
    case IN_FILE_PROG:          return "in_file_prog";
    case ACT_PROG:              return "act_prog";
    case SPEECH_PROG:           return "speech_prog";
    case RAND_PROG:             return "rand_prog";
	case TICK_PROG:				return "tick_prog";
	case HOUR_PROG:				return "hour_prog";
    case FIGHT_PROG:            return "fight_prog";
    case HITPRCNT_PROG:         return "hitprcnt_prog";
    case DEATH_PROG:            return "death_prog";
    case ENTRY_PROG:            return "entry_prog";
    case GREET_PROG:            return "greet_prog";
    case ALL_GREET_PROG:        return "all_greet_prog";
    case GIVE_PROG:             return "give_prog";
    case BRIBE_PROG:            return "bribe_prog";
    case EXIT_PROG:             return "exit_prog";
    case WEAR_PROG:		return "wear_prog";
    case REMOVE_PROG:		return "remove_prog";
    case DEMON_PROG:		return "demon_prog";
    case TIME_PROG:		return "time_prog";
    case LOAD_PROG:		return "load_prog";
    case TAKE_PROG:		return "take_prog";
    case ALL_DEATH_PROG:	return "all_death_prog";
    case VERB_PROG:		return "verb_prog";
    case TRIGGER_PROG:		return "trigger_prog";
    case SAC_PROG:		return "sac_prog";
    case DATA_PROG:		return "data_prog";
    case HIT_PROG:		return "hit_prog";
    case HAIL_PROG:		return "hail_prog";
    case ATTACK_PROG:		return "attack_prog";
    case EAT_PROG:		return "eat_prog";
    case DRINK_PROG:		return "drink_prog";
    case SUB_PROG:		return "sub_prog";
    default:                    return "ERROR_PROG";
    }
}

static void logBug(const char * text, const char * func, int ptype, CHAR_DATA * ch, OBJ_DATA * obj, ROOM_INDEX_DATA * room)
{
    std::ostringstream mess;
    mess << '[';
    switch (ptype)
    {
        case MOB_PROG: mess << "Mob " << ch->pIndexData->vnum; break;
        case OBJ_PROG: mess << "Obj " << obj->pIndexData->vnum; break;
        case ROOM_PROG: mess << "Room " << room->vnum; break;
    }
    mess << "] [" << func << "] " << text;
    bug(mess.str().c_str(), 0);
}

#define LOGBUG(TEXT) logBug(TEXT, __FUNCTION__, ptype, ch, obj, room)

static int * lookupProgValues(int ptype, CHAR_DATA * ch, OBJ_DATA * obj, ROOM_INDEX_DATA * room)
{
    switch (ptype)
    {
        case MOB_PROG:  return ch->mobvalue;
        case OBJ_PROG:  return obj->objvalue;
        case ROOM_PROG: return room->roomvalue;
    }
    return NULL;
}

static PROG_VARDATA * lookupProgVar(int ptype, CHAR_DATA * ch, OBJ_DATA * obj, ROOM_INDEX_DATA * room, char * arg)
{
    switch (ptype)
	{
	    case MOB_PROG: return mprog_find_pvar(arg, ch->prog_data, TRUE);
	    case OBJ_PROG: return mprog_find_pvar(arg, obj->prog_data, TRUE);
	    case ROOM_PROG: return mprog_find_pvar(arg, room->prog_data, TRUE);
	}
    return NULL;
}

static CHAR_DATA * lookupProgChar(int ptype, CHAR_DATA * ch, OBJ_DATA * obj, ROOM_INDEX_DATA * room, char * arg)
{
    switch (ptype)
    {
        case MOB_PROG: return get_char_world(ch, arg);
        case OBJ_PROG: return obj_get_char_world(obj, arg);
        case ROOM_PROG: return room_get_char_world(room, arg);
    }
    return NULL;
}

static OBJ_DATA * lookupProgObj(int ptype, CHAR_DATA * ch, OBJ_DATA * obj, ROOM_INDEX_DATA * room, char * arg)
{
    switch (ptype)
    {
        case MOB_PROG: return get_obj_world(ch, arg);
        case OBJ_PROG: return obj_get_obj_world(obj, arg);
        case ROOM_PROG: return room_get_obj_world(room, arg);
    }
    return NULL;
}

static ROOM_INDEX_DATA * lookupProgRoom(int ptype, CHAR_DATA * ch, OBJ_DATA * obj, ROOM_INDEX_DATA * room, char * arg)
{
    if (!str_cmp(arg, "here"))
    {
        switch (ptype)
        {
            case MOB_PROG: return ch->in_room;
            case OBJ_PROG: return get_room_for_obj(*obj);
            case ROOM_PROG: return room;
        }
    }
    
    if (is_number(arg))
        return get_room_index(atoi(arg));

    return NULL;
}


/* A trivial rehack of do_mstat.  This doesnt show all the data, but just
 * enough to identify the mob and give its basic condition.  It does however,
 * show the MOBprograms which are set.
 */

void do_mpstat( CHAR_DATA *ch, char *argument )
{
    char        buf[ MAX_STRING_LENGTH ];
    BUFFER      *buffer;
    char        arg[ MAX_INPUT_LENGTH  ];
    MPROG_DATA *mprg;
    CHAR_DATA  *victim;

    buffer = new_buf();
    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "MobProg stat whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( !IS_NPC( victim ) )
    {
	send_to_char( "Only Mobiles can have Programs!\n\r", ch);
	return;
    }

    if ( !( victim->pIndexData->progtypes ) || IS_SET(victim->act, ACT_ILLUSION))
    {
        send_to_char( "That Mobile has no Programs set.\n\r", ch);
        return;
    }

    sprintf( buf, "Name: %s.  Vnum: %d.\n\r",
	victim->name, victim->pIndexData->vnum );
    add_buf(buffer, buf);
    
    sprintf( buf, "Short description: %s.\n\rLong  description: %s",
	    victim->short_descr,
	    victim->long_descr[0] != '\0' ?
	    victim->long_descr : "(none).\n\r" );
    add_buf(buffer, buf);
    
    if (IS_NPC(victim))
    {
        sprintf(buf, "Memory Values: %d %d %d %d %d %d %d %d %d %d\n\r", victim->mobvalue[0],
	victim->mobvalue[1], victim->mobvalue[2], victim->mobvalue[3],
	victim->mobvalue[4],victim->mobvalue[5], victim->mobvalue[6],
	victim->mobvalue[7],victim->mobvalue[8], victim->mobvalue[9]);
        add_buf(buffer, buf);
    }

    sprintf( buf, "Hp: %d/%d.  Mana: %d/%d.  Move: %d/%d. \n\r",
	victim->hit,         victim->max_hit,
	victim->mana,        victim->max_mana,
	victim->move,        victim->max_move );
    add_buf(buffer, buf);
    
    sprintf( buf,
	"Lv: %d.  Class: %d.  Align: %d.  AC: %d.  Exp: %d.\n\r",
	victim->level,       victim->class_num,        victim->alignment,
	GET_AC( victim,AC_BASH ),         victim->exp );
    add_buf(buffer, buf);

    for ( mprg = victim->pIndexData->mobprogs; mprg != NULL;
	 mprg = mprg->next )
    {
      sprintf( buf, ">%s %s\n\r%s\n\r",
	      mprog_type_to_name( mprg->type ),
	      mprg->arglist,
	      mprg->comlist );
      add_buf(buffer, buf);
    }
    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);
    return;

}

void do_rpstat( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *pRoom;
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    MPROG_DATA *mprg;
    int i;

    buffer = new_buf();
    
    if (argument[0] == '\0')
	pRoom = ch->in_room;
    else
	pRoom = find_location(ch, NULL, argument);

    if (!pRoom)
	return;

    if (!pRoom->mobprogs)
    {
	if (pRoom == ch->in_room)
	    send_to_char("This room has no programs set.\n\r", ch);
	else
	    send_to_char("That room has no programs set.\n\r", ch);
	return;
    }

    sprintf(buf, "Name: %s.  Vnum: %d.\n\r", pRoom->name, pRoom->vnum);
    add_buf(buffer, buf);

    add_buf(buffer, "Memory values:");
    for (i = 0; i < 10; i++)
    {
	sprintf(buf, " %d", pRoom->roomvalue[i]);
	add_buf(buffer, buf);
    }
    add_buf(buffer, "\n\r");

    for ( mprg = pRoom->mobprogs; mprg != NULL;
	 mprg = mprg->next )
    {
      sprintf( buf, ">%s %s\n\r%s\n\r",
	      mprog_type_to_name( mprg->type ),
	      mprg->arglist,
	      mprg->comlist );
      add_buf(buffer, buf);
    }

    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);
    
    return;
}
    

void do_opstat( CHAR_DATA *ch, char *argument )
{
    char        buf[ MAX_STRING_LENGTH ];
    char        arg[ MAX_INPUT_LENGTH  ];
    BUFFER *buffer;
    MPROG_DATA *mprg=NULL;
    OBJ_DATA  *victim;

    buffer = new_buf();
    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "ObjProg stat whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_obj_list( ch, arg, ch->in_room->contents ) ) == NULL
       && (victim = get_obj_carry( ch, arg, ch)) == NULL )
    {
        act( "I see no $T here.", ch, NULL, arg, TO_CHAR );
	return;
    }

    if ( !( victim->pIndexData->progtypes ) )
    {
        send_to_char( "That Object has no Programs set.\n\r", ch);
        return;
    }

    sprintf( buf, "Name: %s.  Vnum: %d.\n\r",
	victim->name, victim->pIndexData->vnum );
    add_buf( buffer, buf );

    sprintf( buf, "Short description: %s.\n\rDescription      : %s\n\r",
	    victim->short_descr,
	    victim->description[0] != '\0' ?
	    victim->description : "(none).\n\r" );
    add_buf( buffer, buf );

    for ( mprg = victim->pIndexData->objprogs; mprg != NULL;
	 mprg = mprg->next )
    {
      sprintf( buf, ">%s %s\n\r%s\n\r",
	      mprog_type_to_name( mprg->type ),
	      mprg->arglist,
	      mprg->comlist );
      add_buf( buffer, buf );
    }

    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);	
}

PROG_FUN( prog_knockout )
{
    CHAR_DATA *victim = NULL;

    switch (ptype)
    {
	case MOB_PROG:	victim = get_char_room(ch, argument);		break;
	case OBJ_PROG:	victim = obj_get_char_room(obj, argument);	break;
	case ROOM_PROG:	victim = room_get_char_room(room, argument);	break;
    }

    if (!victim)
	return;

    stop_fighting_all(victim);

    if (victim)
	switch_position(victim, POS_SLEEPING);

    return;
}

static ProgState * lookupProgState(CHAR_DATA * ch, OBJ_DATA * obj, ROOM_INDEX_DATA * room, int ptype)
{
    switch (ptype)
    {
        case MOB_PROG:  return &ch->prog_state;
        case OBJ_PROG:  return &obj->prog_state;
        case ROOM_PROG: return &room->prog_state;
    }

    return NULL;
}

static MPROG_DATA * lookupProgData(CHAR_DATA * ch, OBJ_DATA * obj, ROOM_INDEX_DATA * room, const char * argument, int ptype, int progType)
{
    // Lookup the progData and progTypes
    int progTypes;
    MPROG_DATA * progData;
    switch (ptype)
    {
    	case MOB_PROG:  progTypes = ch->pIndexData->progtypes;      progData = ch->pIndexData->mobprogs;    break;
    	case OBJ_PROG:  progTypes = obj->pIndexData->progtypes;     progData = obj->pIndexData->objprogs;   break;
    	case ROOM_PROG: progTypes = room->progtypes;                progData = room->mobprogs;              break;
        default: return NULL;
    }
	
    // Check whether the bits are set
    if (!IS_SET(progTypes, progType))
		return NULL;

    // Lookup the appropriate progData
    while (progData != NULL)
    {
        if (progData->type == progType && !str_cmp(progData->arglist, argument))
            return progData;

        progData = progData->next;
    }

    return NULL;   
}

PROG_FUN(prog_setdata)
{
    ProgState * progState(lookupProgState(ch, obj, room, ptype));
    if (progState == NULL)
        return;

    // Get an argument
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);

    // Lookup the prog, and if found and not already set or there is no 'noreset' supplied, reset the progState to it
    MPROG_DATA * progData(lookupProgData(ch, obj, room, arg, ptype, DATA_PROG));
    if (progData != NULL && (str_cmp(argument, "noreset") || progState->data_prog != progData))
    {
        progState->data_marker = progData->comlist;
        progState->data_prog = progData;
    }
}

PROG_FUN(prog_callsub)
{
    // Before anything else, check the recursion count
    static const int MaxRecursionCount = 100;
    static int recursionCount = 0;
    if (recursionCount >= MaxRecursionCount)
    {
        LOGBUG("Exceeded max recursion limit");
        return;
    }

    // Determine the prog list to search
    MPROG_DATA * prog;
    CHAR_DATA * next_ch(NULL);
    OBJ_DATA * next_obj(NULL);
    ROOM_INDEX_DATA * next_room(NULL);

    char arg[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);
    if (!strcmp(arg, "obj"))
    {
        // Get the obj
        argument = one_argument(argument, arg);
        next_obj = lookupProgObj(ptype, ch, obj, room, arg);
        if (next_obj == NULL)
        {
            LOGBUG("Unable to resolve obj target");
            return;
        }
    
        prog = next_obj->pIndexData->objprogs;
        argument = one_argument(argument, arg);
    }
    else if (!strcmp(arg, "mob"))
    {
        // Get the mob
        argument = one_argument(argument, arg);
        next_ch = lookupProgChar(ptype, ch, obj, room, arg);
        if (next_ch == NULL)
        {
            LOGBUG("Unable to resolve mob target");
            return;
        }
        if (!IS_NPC(next_ch))
        {
            LOGBUG("Mob target resolved to PC");
            return;
        }
        
        prog = next_ch->pIndexData->mobprogs;
        argument = one_argument(argument, arg);
    }
    else if (!strcmp(arg, "room"))
    {
        // Get the room
        argument = one_argument(argument, arg);
        next_room = lookupProgRoom(ptype, ch, obj, room, arg);
        if (next_room == NULL)
        {
            LOGBUG("Unable to resolve room target");
            return;
        }

        prog = next_room->mobprogs;
        argument = one_argument(argument, arg);
    }
    else
    {
        switch (ptype)
        {
            case MOB_PROG: prog = ch->pIndexData->mobprogs; break;
            case OBJ_PROG: prog = obj->pIndexData->objprogs; break;
            case ROOM_PROG: prog = room->mobprogs; break;
            default: LOGBUG("Invalid prog type"); return;
        }

        next_ch = ch;
        next_obj = obj;
        next_room = room;
    }

    // Get the sub_prog name
    if (arg[0] == '\0')
    {
        LOGBUG("Insufficient arguments");
        return;
    }
    
    // Determine the actor
    CHAR_DATA * actor(NULL);
    if (argument[0] != '\0')
    {
        actor = lookupProgChar(ptype, ch, obj, room, argument);
        if (actor == NULL)
        {
            LOGBUG("Could not find specified actor");
            return;
        }
    }

    // Lookup the sub_prog with the arg
    while (prog != NULL)
    {
        // Check for a match
        if (((prog->type & SUB_PROG) != 0) && !str_cmp(prog->arglist, arg))
        {
            // Found a match, so call the driver
            ++recursionCount;
            mprog_driver(prog->comlist, next_ch, actor, next_obj, next_room, NULL, NULL);
            --recursionCount;
            return;
        }

        // Advance to the next
        prog = prog->next;
    }

    LOGBUG("Unknown sub_prog specified");
}

PROG_FUN(prog_adddata)
{
    ProgState * progState(lookupProgState(ch, obj, room, ptype));
    if (progState == NULL || progState->data_prog == NULL || progState->data_marker == NULL)
        return;

    size_t index(progState->data_marker - progState->data_prog->comlist);
    std::string prefix(progState->data_prog->comlist, index);
    prefix += argument;
    prefix += '\n';
    prefix += progState->data_marker;
    
    free_string(progState->data_prog->comlist);
    progState->data_prog->comlist = str_dup(prefix.c_str());
    progState->data_marker = progState->data_prog->comlist + index;
}

PROG_FUN(prog_deletedata)
{
    ProgState * progState(lookupProgState(ch, obj, room, ptype));
    if (progState == NULL || progState->data_prog == NULL || progState->data_marker == NULL)
        return;

    // Determine how many lines to delete
    if (!str_cmp(argument, "all"))
    {
        // Wipe the whole string
        free_string(progState->data_prog->comlist);
        progState->data_prog->comlist = str_dup("");
        progState->data_marker = progState->data_prog->comlist;
        return;
    }

    size_t numLines(1);
    if (argument[0] != '\0')
        numLines = atoi(argument);

    // Grab the first portion of the string
    size_t index(progState->data_marker - progState->data_prog->comlist);
    std::string prefix(progState->data_prog->comlist, index);

    // Walk until the end of the string or lines, whichever comes first
    while (numLines > 0 && *progState->data_marker != '\0')
    {
        if (*progState->data_marker == '\n')
        {
            // Consume any extra carriage return
            ++progState->data_marker;
            if (*progState->data_marker == '\r')
                ++progState->data_marker;

            --numLines;
            continue;
        }

        // Advance
        ++progState->data_marker;
    }

    prefix += progState->data_marker;
    free_string(progState->data_prog->comlist);
    progState->data_prog->comlist = str_dup(prefix.c_str());
    progState->data_marker = progState->data_prog->comlist + index;
}

PROG_FUN(prog_nextdata)
{
    ProgState * progState(lookupProgState(ch, obj, room, ptype));
    if (progState == NULL || progState->data_prog == NULL || progState->data_marker == NULL)
        return;

    int offset;
    char * datastr(progState->data_marker);
    switch (argument[0])
    {
        case '\0':  offset = 1; break;
        default:    datastr = progState->data_prog->comlist; // Fall-through
        case '-':   // Fall-through
        case '+':   offset = atoi(argument); break;

        case '*':
            // Just scroll to end
            while (*datastr != '\0') ++datastr;
            progState->data_marker = datastr;
            return;
    }

    // Handle stepping forward
    while (offset > 0)
    {
        while (true)
        {
            // Check for end of string
            if (*datastr == '\0')
            {
                offset = 0;
                break;
            }

            // Check for end of line
            if (*datastr == '\n')
            {
                --offset;
                ++datastr;
                if (*datastr == '\r')
                    ++datastr;

                break;
            }

            // Advance the pointer
            ++datastr;
        }
    }

    // Handle stepping backward
    while (offset < 0)
    {
        bool skippedOne(false);
        while (true)
        {
            // Check for beginning of string
            if (datastr == progState->data_prog->comlist)
            {
                offset = 0;
                break;
            }

            // Check for end of line
            if (skippedOne)
            {
                if (*datastr == '\n' || *datastr == '\r')
                {
                    // Already skipped one but found another, so unconsume it
                    ++offset;
                    ++datastr;
                    break;
                }
            }
            else
            {
                // Look for the first one to skip
                if (*datastr == '\n')
                    skippedOne = true;
            }

            // Advance (in reverse) the pointer
            --datastr;
        }
    }

    progState->data_marker = datastr;
}

PROG_FUN( prog_grant )
{
    CHAR_DATA *victim = NULL;
    char arg[4][MAX_INPUT_LENGTH];
    int i, add, overmax;

    argument = one_argument(argument, arg[0]);

    switch (ptype)
    {
	case MOB_PROG:	victim = get_char_room(ch, arg[0]);		break;
	case OBJ_PROG:	victim = obj_get_char_room(obj, arg[0]);	break;
	case ROOM_PROG:	victim = room_get_char_room(room, arg[0]);	break;
    }

    if (!victim)
	return;

    for (i = 0; i < 4; i++)
    {
	argument = one_argument(argument, arg[i]);

	if ((arg[i][0] == '\0') && (i < 3))
	    return;
    }

    if (arg[3][0] != '\0')
        overmax = atoi(arg[3]);
    else
	overmax = FALSE;

    add = atoi(arg[0]);

    if (add)
    {
	if (overmax)
	    victim->hit += add;
	else
	    victim->hit = UMIN(victim->max_hit, victim->hit + add);
    }

    add = atoi(arg[1]);

    if (add)
    {
	if (overmax)
	    victim->mana += add;
	else
	    victim->mana = UMIN(victim->max_mana, victim->mana + add);
    }

    add = atoi(arg[2]);

    if (add)
    {
	if (overmax)
	    victim->move += add;
	else
	    victim->move = UMIN(victim->max_move, victim->move + add);
    }

    return;
}

PROG_FUN( prog_roomaffect )
{
    ROOM_INDEX_DATA *pRoom = NULL;
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int i;

    switch (ptype)
    {
	case MOB_PROG:	pRoom = ch->in_room;	break;
	case OBJ_PROG:	pRoom = obj->carried_by ? obj->carried_by->in_room : obj->in_room; break;
	case ROOM_PROG: pRoom = room;		break;
    }

    if (!pRoom)
	return;

    af.where = TO_ROOM;

    for (i = 0; i < 6; i++)
    {
	argument = one_argument(argument, arg);

	if (arg[0] == '\0')
	    return;

	switch (i)
	{
	    case 0:	af.type = atoi(arg);		break;
	    case 1:	af.level = atoi(arg);		break;
	    case 2:	af.duration = atoi(arg);	break;
	    case 3:	af.location = atoi(arg);	break;
	    case 4:	af.modifier = atoi(arg);	break;
	    case 5:	af.bitvector = atoi(arg);	break;
	}
    }

    affect_to_room( pRoom, &af );
    return;
}

PROG_FUN( prog_addaffect )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim = NULL;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int i;

    argument = one_argument(argument, arg);
   
    if (!is_number(arg))
    {
	switch (ptype)
	{
	    case MOB_PROG:  victim = get_char_world(ch, arg);		break;
	    case OBJ_PROG:  victim = obj_get_char_world(obj, arg);	break;
	    case ROOM_PROG: victim = room_get_char_world(room, arg);	break;
	}

	if (!victim)
	    return;

	argument = one_argument(argument, arg);
    }
    else 
    {
	switch (ptype)
	{
	    case MOB_PROG:  victim = ch;		break;
	    case OBJ_PROG:  victim = obj->carried_by;	break;
	}

	if (!victim)
	    return;
    }

    af.where	 = TO_AFFECTS;

    for (i = 0; i < 6; i++)
    {
	if ((arg[0] == '\0') || !is_number(arg))
	    return;

        switch (i)
	{
	    case 0:	af.type = atoi(arg);		break;
	    case 1:	af.level = atoi(arg);		break;
	    case 2:	af.duration = atoi(arg);	break;
	    case 3:	af.location = atoi(arg);	break;
	    case 4:	af.modifier = atoi(arg);	break;
	    case 5:	af.bitvector = atoi(arg);	break;
	}

	argument = one_argument(argument, arg);
    }

    if (arg[0] != '\0')
    {
	if (UPPER(arg[0]) == 'N')
	    af.where = TO_NAFFECTS;
	else if (UPPER(arg[0]) == 'O')
	    af.where = TO_OAFFECTS;
	else if (UPPER(arg[0]) == 'P')
	    af.where = TO_PAFFECTS;

	argument = one_argument(argument, arg);

	if (arg[0] != '\0')
	    if (af.type == gsn_generic || af.type == gsn_sacrifice)
		af.point = str_dup(arg);
    }

    affect_to_char(victim, &af);

    return;
}

PROG_FUN( prog_addroomaffect )
{
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *pRoom = NULL;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int i;

    switch (ptype)
    {
        case MOB_PROG:  pRoom = ch->in_room;		break;
        case OBJ_PROG:  pRoom = obj->carried_by ? obj->carried_by->in_room : obj->in_room; break;
	case ROOM_PROG: pRoom = room;			break;
    }

    if (!pRoom)
        return;

    argument = one_argument(argument, arg);
   
    af.where	 = TO_ROOM;

    for (i = 0; i < 6; i++)
    {
	if ((arg[0] == '\0') || !is_number(arg))
	    return;

        switch (i)
	{
	    case 0:	af.type = atoi(arg);		break;
	    case 1:	af.level = atoi(arg);		break;
	    case 2:	af.duration = atoi(arg);	break;
	    case 3:	af.location = atoi(arg);	break;
	    case 4:	af.modifier = atoi(arg);	break;
	    case 5:	af.bitvector = atoi(arg);	break;
	}

	argument = one_argument(argument, arg);
    }

    affect_to_room(pRoom, &af);

    return;
}

PROG_FUN( prog_addevent )
{
    CHAR_DATA *victim = NULL;
    char arg[MAX_STRING_LENGTH];

    argument = one_argument(argument, arg);

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg);	    break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg);	    break;
	case ROOM_PROG: victim = room_get_char_world(room, arg);    break;
    }

    if (!victim)
        return;

    if (!str_cmp(argument, "berserk"))
        add_event((void *) victim, 1, &event_berserk);

    return;
}
    



PROG_FUN( prog_asound )
{
    ROOM_INDEX_DATA *pRoom = NULL;
    int door;

    switch (ptype)
    {
	case MOB_PROG:	pRoom = ch->in_room;	break;
	case OBJ_PROG:	pRoom = obj->carried_by ? obj->carried_by->in_room : obj->in_room; break;
	case ROOM_PROG: pRoom = room;		break;
    }

    if (!pRoom)
	return;

    if (argument[0] == '\0')
	return;

    for (door = 0; door < 6; door++)
    {
	if (pRoom->exit[door])
	{
	    act(argument, pRoom->exit[door]->u1.to_room->people, NULL, NULL, TO_CHAR);
	    act(argument, pRoom->exit[door]->u1.to_room->people, NULL, NULL, TO_ROOM);
	}
    }

    return;
}

PROG_FUN( prog_wander )
{
    EXIT_DATA *pexit=NULL;
    int x=0, door=0;

    if (ptype != MOB_PROG)
	return;

    if (!IS_AWAKE(ch))
	return;

    door = number_range(0, 5);
    while (x < 30 && ((pexit = ch->in_room->exit[door]) == NULL))
    {
	x++;
	door = number_range(0, 5);
    }

    if (pexit == NULL)
	return;

    if ( (!IS_SET(ch->affected_by, AFF_CHARM) || IS_AFFECTED(ch, AFF_CHARM_FREEAGENT))
    && !ch->desc
    && ch->tracking == NULL
    && ch->position == POS_STANDING
    &&   pexit->u1.to_room != NULL
    &&   !IS_SET(pexit->exit_info, EX_CLOSED)
    &&   !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)
    && ( !IS_SET(ch->act, ACT_STAY_AREA)
    ||   pexit->u1.to_room->area == ch->in_room->area )
    && ( !IS_SET(ch->act, ACT_OUTDOORS)
    ||   !IS_SET(pexit->u1.to_room->room_flags,ROOM_INDOORS))
    && ( !IS_SET(ch->act, ACT_INDOORS)
    ||   IS_SET(pexit->u1.to_room->room_flags,ROOM_INDOORS)))
    {
        move_char( ch, door, TRUE );
    }
}

PROG_FUN( prog_hitdamage )
{
    if (argument[0] == '\0')
	return;

    hitprog_damage = atoi(argument);

    return;
}

PROG_FUN( prog_dammess )
{
    CHAR_DATA *acting = NULL;
    CHAR_DATA *victim = NULL;
    char arg[MAX_INPUT_LENGTH];
    int dam, dtype = -1;

    argument = one_argument(argument, arg);

    if ((arg[0] == '\0') || (argument[0] == '\0'))
	return;

    if (ptype == MOB_PROG)
	acting = ch;
    else if (ptype == OBJ_PROG)
	acting = obj->carried_by;

    if (!acting)
	return;

    if ((victim = get_char_room(acting, arg)) == NULL)
	return;
    
    argument = one_argument(argument, arg);

    if (!is_number(arg))
	return;

    dam = atoi(arg);

    argument = one_argument(argument, arg);

    if ((argument[0] != '\0') && is_number(argument))
	dtype = atoi(argument);

    global_damage_noun = arg;
    dam_message(acting, victim, dam, dtype, arg, FALSE);
    global_damage_noun = NULL;

    return;
}

PROG_FUN(prog_dealdamage)
{
    // Syntax: mpdealdamage <target> <source, either var or string> <noun or specify 'default' to use source's natural damage noun> <damtype_0> <dam_0> [damtype_1] [dam_1] ... [damtype_n] [dam_n]
    // Grab the first argument so as to look up the target
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);

    CHAR_DATA * victim(NULL);
    switch (ptype)
    {
	    case MOB_PROG:	victim = get_char_room(ch, arg);		break;
    	case OBJ_PROG:	victim = obj_get_char_room(obj, arg);	break;
	    case ROOM_PROG:	victim = room_get_char_room(room, arg);	break;
    }

    if (victim == NULL)
    	return;
    
    // Grab the source and noun arguments
    CHAR_DATA * source(NULL);
    char noun[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);
    argument = one_argument(argument, noun);
    switch (ptype)
    {
	    case MOB_PROG:	source = get_char_room(ch, arg);		break;
    	case OBJ_PROG:	source = obj_get_char_room(obj, arg);	break;
	    case ROOM_PROG:	source = room_get_char_room(room, arg);	break;
    }

    // Determine damage
    std::vector<DamageInfo> damage;
    char temp[MAX_INPUT_LENGTH];
    while (argument[0] != '\0')
    {
        // Read in type
        argument = one_argument(argument, temp);
        int type;
        if (is_number(temp)) type = atoi(temp);
        else type = damtype_lookup(temp);
        if (type <= 0 || type > DAM_MAX || damtype_table[type].name == NULL)
        {
            LOGBUG("Invalid damtype argument to mpdealdamage");
            return;
        }

        // Read in amount
        argument = one_argument(argument, temp);
        if (!is_number(temp))
        {
            LOGBUG("Invalid damage amount argument to mpdealdamage");
            return;
        }

        damage.push_back(DamageInfo(atoi(temp), type));
    }

    // Verify at least one damage provided
    if (damage.empty())
    {
        LOGBUG("Missing damage arguments to mpdealdamage");
        return;
    }

    // Do the actual damage
    if (source == NULL) 
    {
        // No source implies the noun should be used
        global_damage_noun = noun;
        sourcelessDamage(victim, arg, TYPE_UNDEFINED, damage);
    }
    else
    {
        // Source provided; use the noun if "default" is specified
        if (!strcmp(noun, "default") && source->dam_verb != NULL)  global_damage_noun = source->dam_verb;
        else global_damage_noun = noun;
        damage_new(source, victim, damage, TYPE_UNDEFINED, true);
    }

    // Reset the noun
    global_damage_noun = NULL;
}

PROG_FUN( prog_damfrom )
{
    CHAR_DATA *victim = NULL;
    char arg[MAX_INPUT_LENGTH];
    int dval;

    argument = one_argument(argument, arg);

    if ((arg[0] == '\0') || (argument[0] == '\0'))
	return;

    switch (ptype)
    {
	case MOB_PROG:	victim = get_char_room(ch, arg);		break;
	case OBJ_PROG:	victim = obj_get_char_room(obj, arg);		break;
	case ROOM_PROG:	victim = room_get_char_room(room, arg);		break;
    }

    if (!victim)
	return;
    
    argument = one_argument(argument, arg);

    if (!is_number(arg))
	return;

    dval = atoi(arg);

    argument = one_argument(argument, arg);

    global_damage_noun = arg;
    global_damage_from = argument;
    dam_message(ch, victim, dval, -1, arg, FALSE);
    global_damage_noun = NULL;
    global_damage_from = NULL;

    return;
}

PROG_FUN( prog_damfromtype )
{
    CHAR_DATA *victim = NULL;
    char arg[MAX_INPUT_LENGTH];
    int dval;
    int dtype;

    argument = one_argument(argument, arg);

    if ((arg[0] == '\0') || (argument[0] == '\0'))
	return;

    switch (ptype)
    {
	case MOB_PROG:	victim = get_char_room(ch, arg);		break;
	case OBJ_PROG:	victim = obj_get_char_room(obj, arg);		break;
	case ROOM_PROG:	victim = room_get_char_room(room, arg);		break;
    }

    if (!victim)
	return;
    
    argument = one_argument(argument, arg);

    if (!is_number(arg))
	return;

    dval = atoi(arg);
    
    argument = one_argument(argument, arg);

    if (!is_number(arg))
	return;

    dtype = atoi(arg);

    argument = one_argument(argument, arg);

    global_damage_noun = arg;
    global_damage_from = argument;
    dam_message(ch, victim, dval, 1000+dtype, arg, FALSE);
    global_damage_noun = NULL;
    global_damage_from = NULL;

    return;
}

PROG_FUN( prog_brand )
{
    CHAR_DATA *victim = NULL;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if ((arg[0] == '\0') || (argument[0] == '\0'))
	return;

    switch (ptype)
    {
	case MOB_PROG:	victim = get_char_room(ch, arg);		break;
	case OBJ_PROG:	victim = obj_get_char_room(obj, arg);		break;
	case ROOM_PROG:	victim = room_get_char_room(room, arg);		break;
    }

    if (!victim)
	return;

    if (is_number(argument))
	brand_char((ptype == MOB_PROG) ? ch : NULL, victim, atoi(argument), FALSE);

    return;
}

PROG_FUN( prog_slay )
{
    CHAR_DATA *victim = NULL;
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0')
	return;

    switch (ptype)
    {
	case MOB_PROG:	victim = get_char_room(ch, argument);		break;
	case OBJ_PROG:	victim = obj_get_char_room(obj, argument);	break;
	case ROOM_PROG:	victim = room_get_char_room(room, argument);	break;
    }

    if (!victim)
	return;

    if (((ptype == MOB_PROG) && (victim->level >= LEVEL_IMMORTAL) && (ch->level < victim->level))
     || ((ptype == OBJ_PROG) && (victim->level >= LEVEL_IMMORTAL) && (obj->level < victim->level))
     || (victim->level == 60))
    {
	switch (ptype)
	{
	    case MOB_PROG: 
		act("$n just tried to progslay you.", ch, NULL, victim, TO_VICT);
		break;
	    case OBJ_PROG: 
		act("$p just tried to progslay you.", victim, obj, NULL, TO_CHAR);
		break;
	    case ROOM_PROG:
		send_to_char("A room_prog just tried to slay you.\n\r", victim);
		break;
	}
	return;
    }
    
    if (!IS_NPC(victim))
    {
	switch (ptype)
	{
	    case MOB_PROG: 
                sprintf(buf, "%s progslayed %s in %s [%i].\n\r", ch->name, victim->name, victim->in_room->name, victim->in_room->vnum); break;
	    case OBJ_PROG:
                sprintf(buf, "%s progslayed %s in %s [%i].\n\r", obj->name, victim->name, victim->in_room->name, victim->in_room->vnum); break;
	    case ROOM_PROG:
                sprintf(buf, "%s was progslayed by %s [%i].\n\r", victim->name, victim->in_room->name, victim->in_room->vnum); break;
    	}

	log_string(buf);
    }    
        
    raw_kill(victim);

    return;
}

PROG_FUN( prog_statup )
{
    CHAR_DATA *victim = NULL;
    char arg[MAX_INPUT_LENGTH];
    int stat;

    if (argument[0] == '\0')
	return;

    argument = one_argument(argument, arg);

    switch (ptype)
    {
	case MOB_PROG:	victim = get_char_room(ch, arg);		break;
	case OBJ_PROG:	victim = obj_get_char_room(obj, arg);		break;
	case ROOM_PROG:	victim = room_get_char_room(room, arg);		break;
    }

    if (!victim)
	return;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if      (!str_cmp(arg, "str")) stat = STAT_STR;
    else if (!str_cmp(arg, "int")) stat = STAT_INT;
    else if (!str_cmp(arg, "wis")) stat = STAT_WIS;
    else if (!str_cmp(arg, "dex")) stat = STAT_DEX;
    else if (!str_cmp(arg, "con")) stat = STAT_CON;
    else if (!str_cmp(arg, "chr")) stat = STAT_CHR;
    else return;

    argument = one_argument(argument, arg);
 
    if (arg[0] == '\0')
	return;

    if (!is_number(arg))
	return;

    set_perm_stat(victim, stat, UMIN(victim->perm_stat[stat] + atoi(arg), get_max_stat(victim, stat)));
    
    return;
}

PROG_FUN( prog_statdown )
{
    CHAR_DATA *victim = NULL;
    char arg[MAX_INPUT_LENGTH];
    int stat;

    if (argument[0] == '\0')
	return;

    argument = one_argument(argument, arg);

    switch (ptype)
    {
	case MOB_PROG:	victim = get_char_room(ch, arg);		break;
	case OBJ_PROG:	victim = obj_get_char_room(obj, arg);		break;
	case ROOM_PROG:	victim = room_get_char_room(room, arg);		break;
    }

    if (!victim)
	return;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if      (!str_cmp(arg, "str")) stat = STAT_STR;
    else if (!str_cmp(arg, "int")) stat = STAT_INT;
    else if (!str_cmp(arg, "wis")) stat = STAT_WIS;
    else if (!str_cmp(arg, "dex")) stat = STAT_DEX;
    else if (!str_cmp(arg, "con")) stat = STAT_CON;
    else if (!str_cmp(arg, "chr")) stat = STAT_CHR;
    else return;

    argument = one_argument(argument, arg);
 
    if (arg[0] == '\0')
	return;

    if (!is_number(arg))
	return;


    set_perm_stat(victim, stat, victim->perm_stat[stat] - atoi(arg));
    
    return;
}

PROG_FUN( prog_silverset )
{
    silver_state = 16;
    return;
}

	
PROG_FUN ( prog_kill )
{
    CHAR_DATA *victim = NULL;

    if (ptype != MOB_PROG)
	return;

    if (argument[0] == '\0')
	return;

    if ((victim = get_char_room(ch, argument)) == NULL)
	return;

    if (victim == ch)
	return;

    if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == victim) 
     && (ch->demontrack != ch->master))
	return;

    if ((ch->position == POS_FIGHTING) && (ch->clan == 0))
	return;

    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}

PROG_FUN( prog_rangekill )
{
    CHAR_DATA *victim = NULL;

    if (ptype != MOB_PROG)
	return;

    if (argument[0] == '\0')
	return;

    if ((victim = get_char_room(ch, argument)) == NULL)
	return;

    if ( victim == ch )
	return;

    if ( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim && ch->demontrack != ch->master)
	return;

    if ( ch->position == POS_FIGHTING && ch->clan == 0)
	return;

    if (ch->level - victim->level > 8)
	return;

    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}

PROG_FUN( prog_junk )
{
    OBJ_DATA *pobj;
    OBJ_DATA *pobj_next;

    if (argument[0] == '\0')
	return;

    if (ptype == MOB_PROG)
    {
	if ( str_cmp( argument, "all" ) && str_prefix( "all.", argument ) )
	{
	    if ( ( pobj = get_obj_wear( ch, argument ) ) != NULL )
            {
	        unequip_char( ch, pobj );
		oprog_remove_trigger(pobj);
	        extract_obj( pobj );
	        return;
            }
            if ( ( pobj = get_obj_carry( ch, argument, ch) ) == NULL )
	        return; 
            extract_obj( pobj );
        }
	else
            for ( pobj = ch->carrying; pobj != NULL; pobj = pobj_next )
            {
        	pobj_next = pobj->next_content;
        	if ( argument[3] == '\0' || is_name( &argument[4], pobj->name ) )
        	{
		    if (pobj->worn_on)
		    {
	    		unequip_char( ch, pobj );
			oprog_remove_trigger(pobj);
		    }

          	    extract_obj( pobj );
        	} 
      	    }
    }
    else
    {
	ROOM_INDEX_DATA *pRoom = NULL;

	if (ptype == OBJ_PROG)
	{
	    if (obj->carried_by)
		pRoom = obj->carried_by->in_room;
	    else
		pRoom = obj->in_room;
	}
	else
	    pRoom = room;

	if (!room)
	    return;

        for (pobj = pRoom->contents; pobj != NULL; pobj = pobj_next )
	{
            pobj_next = pobj->next_content;
            if (is_name(argument,pobj->name))
                break;
        }

        if ( pobj != NULL )
        {
	    extract_obj( pobj );
            return;
        }
    }

    return;
}

PROG_FUN( prog_zecho )
{
    AREA_DATA *pArea = NULL;
    DESCRIPTOR_DATA *d;
    int t;

    if (argument[0] == '\0')
	return;

    switch(ptype)
    {
	case MOB_PROG:  pArea = ch->in_room ? ch->in_room->area : NULL;	break;
	case OBJ_PROG:
	    if (obj->carried_by && obj->carried_by->in_room)
		pArea = obj->carried_by->in_room->area;
	    else if (obj->in_room)
		pArea = obj->in_room->area;
	    break;
	case ROOM_PROG: pArea = room->area; break;
    }

    if (!pArea)
	return;

    for (d = descriptor_list; d; d = d->next)
    {   
        if (d->connected == CON_PLAYING
        &&  d->character->in_room != NULL
        &&  d->character->in_room->area == pArea)
        {   
	    t = get_trust(d->character);
            if (((ptype != MOB_PROG) || (t >= get_trust(ch))) && (t >= LEVEL_IMMORTAL))
                send_to_char("zone> ",d->character);
            send_to_char(argument,d->character);
            send_to_char("\n\r",d->character);
        }
    }

    return;
}

PROG_FUN( prog_gecho )
{
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    int t;

    if ( argument[0] == '\0' )
        return;

    for ( d = descriptor_list; d; d = d->next )
    {
        if ( d->connected == CON_PLAYING )
        {
	    t = get_trust(d->character);
            if (((ptype != MOB_PROG) || (t >= get_trust(ch))) && t > 51)
            {
                sprintf(buf, "%s: global> ", ch->name);
                send_to_char( buf ,d->character);
            }
            send_to_char( argument, d->character );
            send_to_char( "\n\r",   d->character );
        }
    }

    return;
}

PROG_FUN( prog_echoaround )
{
    CHAR_DATA *victim = NULL;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if ((argument[0] == '\0') || (arg[0] == '\0'))
	return;

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg);	break;
    }

    if (!victim)
	return;

    if (ptype == MOB_PROG)
	emote_act( argument, ch, NULL, victim, TO_VICTROOM );
    else
	emote_act( argument, victim, NULL, NULL, TO_ROOM);
}

PROG_FUN( prog_echoat )
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim = NULL;

    argument = one_argument( argument, arg );

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg);	break;
    }

    if (!victim)
	return;

    if (ptype == MOB_PROG)
	emote_act_new(argument, ch, NULL, victim, TO_VICT, POS_DEAD);
    else
	emote_act_new(argument, victim, NULL, NULL, TO_CHAR, POS_DEAD);
}

PROG_FUN( prog_echo )
{
    ROOM_INDEX_DATA *pRoom = NULL;

    if (argument[0] == '\0')
	return;

    switch (ptype)
    {
	case MOB_PROG:  pRoom = ch->in_room;		break;
	case OBJ_PROG:  pRoom = obj->carried_by ? obj->carried_by->in_room : obj->in_room; break;
	case ROOM_PROG: pRoom = room;
    }

    if (!pRoom)
	return;

    emote_act(argument, pRoom->people, NULL, NULL, TO_CHAR);
    emote_act(argument, pRoom->people, NULL, NULL, TO_ROOM);
}

PROG_FUN( prog_echo_fight )
{
    ROOM_INDEX_DATA *pRoom;

    if (ptype != OBJ_PROG)
	return;

    if ( argument[0] == '\0' )
        return;

    pRoom = obj->carried_by ? obj->carried_by->in_room : obj->in_room;

//    if (obj->carried_by && (obj->wear_loc > -1))

    if (obj->carried_by && obj->worn_on)
    {
	emote_act( argument, pRoom->people, NULL, NULL, TO_ROOM );
	emote_act( argument, pRoom->people, NULL, NULL, TO_CHAR );
    }

    return;
}

PROG_FUN( prog_mload )
{
    ROOM_INDEX_DATA *pRoom = NULL;
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *pMob;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if ((arg[0] == '\0') || !is_number(arg))
	return;

    switch (ptype)
    {
	case MOB_PROG:  pRoom = ch->in_room;		break;
	case OBJ_PROG:  pRoom = obj->carried_by ? obj->carried_by->in_room : obj->in_room; break;
	case ROOM_PROG: pRoom = room;
    }

    if (!pRoom)
	return;

    if ((pMobIndex = get_mob_index(atoi(arg))) == NULL)
	return;

    if (ptype == MOB_PROG)
        sprintf(buf, "Mob %d loaded mob %d.", ch->pIndexData->vnum, pMobIndex->vnum);
    else if (ptype == OBJ_PROG)
	sprintf(buf, "Obj %d loaded mob %d.", obj->pIndexData->vnum, pMobIndex->vnum);
    else if (ptype == ROOM_PROG)
	sprintf(buf, "Room %d loaded mob %d.", room->vnum, pMobIndex->vnum);
    else
	sprintf(buf, "Mob %d was loaded by an unknown prog type.", pMobIndex->vnum);

    wiznet(buf,NULL,NULL,WIZ_DEBUG,WIZ_SECURE,0);

    pMob = create_mobile(pMobIndex);
    char_to_room(pMob, pRoom);

    if ((argument[0] != '\0') && !str_cmp(argument, "reset"))
        mprog_load_trigger(pMob);

    return;
}

PROG_FUN( prog_tellimm )
{
    int oldtrust;

    if (ptype != MOB_PROG)
	return;

    oldtrust = ch->trust;
    ch->trust = 60;
    do_tell(ch, argument);
    ch->trust = oldtrust;

    return;
}

PROG_FUN( prog_oload )
{
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *pRoom = NULL;
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *pObj = NULL;
    int level;

    argument = one_argument(argument, arg);

    if ((arg[0] == '\0') || !is_number(arg))
	return;

    if ((pObjIndex = get_obj_index(atoi(arg))) == NULL)
	return;

    switch (ptype)
    {
	case MOB_PROG:  level = ch->level;		break;
	case OBJ_PROG:	level = obj->level;		break;
	default:	level = pObjIndex->level;	break;
    }

    argument = one_argument(argument, arg);

    switch(ptype)
    {
        case MOB_PROG:  pRoom = ch->in_room;	break;
        case OBJ_PROG:  pRoom = obj->carried_by ? obj->carried_by->in_room : obj->in_room; break;
        case ROOM_PROG: pRoom = room;		break;
    }

    if (!pRoom)
	return;

    if (arg[0] == '\0')
    {
	pObj = create_object(pObjIndex, level);

	if ((ptype == MOB_PROG) && CAN_WEAR(pObj, ITEM_TAKE))
	{
	    obj_to_char(pObj, ch);
	    return;
	}

	obj_to_room(pObj, pRoom);
    }
    else if (!str_cmp(arg, "in"))
    {
	OBJ_DATA *container = NULL, *vObj;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0')
	    return;

	pObj = create_object(pObjIndex, level);
	
	if (!str_cmp(arg, "room"))
	{
	    obj_to_room(pObj, pRoom);
	    return;
	}

	if ((ptype == OBJ_PROG) && !str_cmp(arg, "self"))
	   container = obj;
	
	if (ptype == MOB_PROG)
	{
	    for (vObj = ch->carrying; vObj; vObj = vObj->next_content)
		if (is_name(arg, vObj->name))
		{
		    container = vObj;
		    break;
		}
	}

	for (vObj = pRoom->contents; vObj; vObj = vObj->next_content)
	{
	    if (is_name(arg, vObj->name))
	    {
		container = vObj;
		break;
	    }
	}

	if (container)
	    obj_to_obj(pObj, container);
	else
	{
	    extract_obj(pObj);
	    return;
	}
    }
    else if (!str_cmp(arg, "on"))
    {
	CHAR_DATA *victim = NULL;
	int slot = 0;

	argument = one_argument(argument, arg);
	
	if (arg[0] == '\0')
	    return;

	switch(ptype)
	{
	    case MOB_PROG:  victim = get_char_world(ch, arg);		break;
	    case OBJ_PROG:  victim = obj_get_char_world(obj, arg);	break;
	    case ROOM_PROG: victim = room_get_char_room(room, arg);	break;
	}

	if (!victim)
	    return;

	argument = one_argument(argument, arg);

	if ((arg[0] != '\0') && is_number(arg))
	    slot = atoi(arg);

	pObj = create_object(pObjIndex, level);

	obj_to_char(pObj, victim);

	if (slot)
	{
	    equip_char(victim, pObj, (1 << slot));
	    if ((get_eq_char(victim, slot)) != pObj)
	    {
	        extract_obj(pObj);
	        return;
	    }
	    else
		oprog_wear_trigger(pObj);
	}

    }

    if (pObj)
        oprog_load_trigger(pObj);

    return;
}

PROG_FUN( prog_purge )
{
    ROOM_INDEX_DATA *pRoom = NULL;
    CHAR_DATA *vch = NULL, *vch_next;
    OBJ_DATA *vobj, *vobj_next;

    switch(ptype)
    {
        case MOB_PROG:  pRoom = ch->in_room;	break;
        case OBJ_PROG:  pRoom = obj->carried_by ? obj->carried_by->in_room : obj->in_room; break;
        case ROOM_PROG: pRoom = room;		break;
    }

    if (!pRoom)
        return;

    if (argument[0] == '\0')
    {
        for (vch = pRoom->people; vch; vch = vch_next)
        {
            vch_next = vch->next_in_room;
            if (IS_NPC(vch) && !IS_SET(vch->act, ACT_NOPURGE) && (vch != ch))
                extract_char(vch, TRUE);
        }

        for (vobj = pRoom->contents; vobj; vobj = vobj_next)
        {
            vobj_next = vobj->next_content;
            if (!IS_OBJ_STAT(vobj,ITEM_NOPURGE))
            extract_obj(vobj);
        }

        return;
    }

    if (!str_cmp(argument, "self") && (ptype == OBJ_PROG))
    {
        extract_obj(obj);
        return;
    }

    bool override(false);
    bool retain(false);
    char arg[MAX_STRING_LENGTH];
    char * remainder = one_argument(argument, arg);

    if (arg[0] == '+')
    {
        argument = remainder;

        for (unsigned int i(1); arg[i] != '\0'; ++i)
        {
            switch (arg[i])
            {
                case 'o': override = true; break;
                case 'r': retain = true; break;
            }
        }
    }

    switch(ptype)
    {
        case MOB_PROG:  vch = get_char_room(ch, argument);		break;
        case OBJ_PROG:	vch = obj_get_char_room(obj, argument);		break;
        case ROOM_PROG:	vch = room_get_char_room(room, argument);	break;
    }

    if (!vch)
    {
        for (vobj = pRoom->contents; vobj; vobj = vobj_next)
        {
            vobj_next = vobj->next_content;
            if (is_name(argument, vobj->name) && (!IS_OBJ_STAT(vobj, ITEM_NOPURGE) || override))
                break;
        }

        if (vobj == NULL && ptype == MOB_PROG)
        {
            for (vobj = ch->carrying; vobj; vobj = vobj_next)
            {
                vobj_next = vobj->next_content;
                if (is_name(argument, vobj->name) && (!IS_OBJ_STAT(vobj, ITEM_NOPURGE) || override))
                    break;
            }
        }

        if (vobj != NULL)
        {
            if (retain)
            {
                OBJ_DATA * obj_next;
                for (OBJ_DATA * objcont = vobj->contains; objcont; objcont = obj_next)
                {
                    obj_next = objcont->next_content;
                    obj_from_obj(objcont);
                    obj_to_room(objcont, pRoom);
                }
            }

            extract_obj(vobj);
            return;
        }
    }
    else if (IS_NPC(vch))
        extract_char(vch, TRUE);
}

PROG_FUN(prog_follow)
{
    char arg[MAX_STRING_LENGTH];

    // Check the argument count
    if (argument[0] == '\0')
    {
        LOGBUG("Insufficient arguments provided to mpfollow");
        return;
    }

    // Get the follower
    argument = one_argument(argument, arg);
    CHAR_DATA * follower(lookupProgChar(ptype, ch, obj, room, arg));
    if (follower == NULL)
    {
        LOGBUG("Unable to resolve follower in mpfollow");
        return;
    }

    // Get the leader
    CHAR_DATA * leader(lookupProgChar(ptype, ch, obj, room, argument));
    if (leader == NULL)
    {
        LOGBUG("Unable to resolve leader in mpfollow");
        return;
    }

    // Make the following happen
    add_follower(follower, leader);
    set_leader(follower, leader);
}


static int resolveNameScore(const char * name_str, const std::vector<std::string> & args)
{
    int score(0);
    int lastName(-2);

    // Determine the stripped name
    std::string name(stripPunctuation(name_str));

    // Iterate the args
    for (unsigned int i(0); i < args.size(); ++i)
    {
        // Check whether the arg is a name of the obj
        if (is_name(args[i].c_str(), name.c_str()))
        {
            // One point is awarded if the names match, and another if consecutive names match
            ++score;
            if ((lastName + 1) == static_cast<int>(i))
                ++score;

            lastName = i;
        }
    }

    return score;
}

static OBJ_DATA * resolveObjects(OBJ_DATA * startObj, const std::vector<std::string> & args)
{
    int resolveScore(0);
    OBJ_DATA * result(NULL);

    // Iterate the objects, computing scores for each
    for (OBJ_DATA * obj(startObj); obj != NULL; obj = obj->next_content)
    {
        int score(resolveNameScore(obj->name, args));
        if (score > resolveScore)
        {
            // This score is better than the current forerunner, so replace it
            resolveScore = score;
            result = obj;
        }
    }
    return result;
}

static std::vector<std::string> PrepareResolveArgs(const char * argument)
{
    // Strip the punctuation
    std::vector<std::string> args = splitArguments(argument);
    for (unsigned int i(0); i < args.size(); ++i)
        args[i] = stripPunctuation(args[i]);

    return args;
}

static bool ResolvePCWorld(PROG_VARDATA & c_var, const char * argument)
{
    // Prepare the args
    std::vector<std::string> args = PrepareResolveArgs(argument);
    if (args.empty())
        return false;

    int resolveScore(0);
    CHAR_DATA * ch(NULL);
    for (DESCRIPTOR_DATA * d = descriptor_list; d != NULL; d = d->next)
    {
        // Filter out certain characters
        if (d->connected != CON_PLAYING)
            continue;

        // Check the names
        int score(resolveNameScore(d->character->name, args));
        if (score > resolveScore)
        {
            resolveScore = score;
            ch = d->character;
        }
    }

    if (ch != NULL)
        copy_string(c_var.value, ch->name);

    return true;
}

PROG_FUN(prog_resolve)
{
    char arg[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];

    // Lookup the target string
    argument = one_argument(argument, arg);
    PROG_VARDATA * c_var(lookupProgVar(ptype, ch, obj, room, arg));
	if (c_var == NULL)
    {
        LOGBUG("Invalid local string target supplied to mpresolve");
        return;
    }
    copy_string(c_var->value, "");

    // Check for obj; this is to support future extensions to other resolve types
    argument = one_argument(argument, arg);
    if (!str_cmp(arg, "pc"))
    {
        if (!ResolvePCWorld(*c_var, argument))
            LOGBUG("Insufficient arguments");
        return;
    }

    if (str_cmp(arg, "obj") != 0)
    {
        std::ostringstream mess;
        mess << "Expected resolve type 'pc' or 'obj' not supplied to mpresolve, found '" << arg << "'";
        LOGBUG(mess.str().c_str());
        return;
    }

    // Prepare the next args
    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);
    std::vector<std::string> args = PrepareResolveArgs(argument);
    if (args.empty())
    {
        LOGBUG("Insufficient arguments");
        return;
    }

    // Check for the target container
    OBJ_DATA * resultObj(NULL);
    if (!str_cmp(arg, "char"))
    {
        // Find the char
        CHAR_DATA * vch(lookupProgChar(ptype, ch, obj, room, arg2));
        if (vch == NULL)
        {
            LOGBUG("Unable to resolve char target of mpresolve");
            return;
        }

        // Iterate the char's objects to find a match
        resultObj = resolveObjects(vch->carrying, args);
    }
    else if (!str_cmp(arg, "room"))
    {
        // Find the room
        ROOM_INDEX_DATA * troom(lookupProgRoom(ptype, ch, obj, room, arg2));
        if (troom == NULL)
        {
            LOGBUG("Unable to resolve room target of mpresolve");
            return;
        }

        // Iterate the room's objects to find a match
        resultObj = resolveObjects(troom->contents, args);
    }
    else
    {
        LOGBUG("Unknown resolve container supplied to mpresolve; expected either 'char' or 'room'");
        return;
    }

    // Put the text into the c_var
    if (resultObj != NULL)
        copy_string(c_var->value, resultObj->name);
}

PROG_FUN(prog_loadstring)
{
    char arg[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);

    char ** svalues(NULL);
    if (!str_cmp(arg, "char"))
    {
        // Look up the char
        CHAR_DATA * vch(lookupProgChar(ptype, ch, obj, room, arg2));
        if (vch != NULL)
            svalues = vch->stringvalue;
    }
    else if (!str_cmp(arg, "obj"))
    {
        // Look up the obj
        OBJ_DATA * vobj(lookupProgObj(ptype, ch, obj, room, arg2));
        if (vobj != NULL)
            svalues = vobj->stringvalue;
    }
    else if (!str_cmp(arg, "room"))
    {
        // Look up the room
        ROOM_INDEX_DATA * troom(lookupProgRoom(ptype, ch, obj, room, arg2));
        if (troom != NULL)
            svalues = troom->stringvalue;
    }

    // Verify a target
    if (svalues == NULL)
    {
        LOGBUG("Invalid target specified to mploadstring");
        return;
    }

    // Determine which string was specified
    argument = one_argument(argument, arg);
    if (!is_number(arg))
    {
        LOGBUG("Invalid string number specified to mploadstring");
        return;
    }

    int index(atoi(arg));
    if (index < 0 || index >= MAX_MOBVALUE)
    {
        LOGBUG("String number out of range in mploadstring");
        return;
    }

    // Check for a var to load into
    argument = one_argument(argument, arg);
    PROG_VARDATA * c_var(lookupProgVar(ptype, ch, obj, room, arg));
	if (c_var == NULL)
    {
        LOGBUG("Invalid local string target supplied to mploadstring");
        return;
    }

    // Put the text into the c_var
    copy_string(c_var->value, svalues[index] == 0 ? "" : svalues[index]);
}

PROG_FUN(prog_loaddesc)
{
    // Get the target argument
    std::string textBase;
    char arg[MAX_STRING_LENGTH];
    argument = one_argument(argument, arg);
    if (argument[0] == '\0' || arg[0] == '\0')
    	return;

    // Load up the text in question
    const char * text(NULL);
    if (!str_cmp(arg, "here"))
    {
        // Load from this room
    	switch(ptype)
	    {
	        case MOB_PROG:  text = ch->in_room->description; break;
    	    case OBJ_PROG:  text = obj->in_room->description; break;
	        case ROOM_PROG: text = room->description; break;
    	}
    }
    else if (!str_cmp(arg, "self"))
    {
        // Load from prog entity
    	switch(ptype)
	    {
	        case MOB_PROG:  text = ch->description; break;
    	    case OBJ_PROG:  text = obj->description; break;
	        case ROOM_PROG: text = room->description; break;
    	}
    }
    else if (is_number(arg))
    {
        // Load from room indicated by vnum
	    ROOM_INDEX_DATA * textRoom(find_location(ch, obj, arg));
        if (textRoom != NULL)
            text = textRoom->description;
    }
    else if (!str_prefix("helpfile ", arg))
    {
        // Pull off the rest of the arg as the helpfile keywords
        char dummy[MAX_STRING_LENGTH];
        const char * keywords(one_argument(arg, dummy));
        
        // Load up the helpfile
        HELP_DATA * helpData(load_helpfile(keywords));
        if (helpData != NULL)
        {
            textBase = helpData->text;
            text = textBase.c_str();
            free_help(helpData);
        }
    }
    else
    {
        // Lookup local chars
        CHAR_DATA * vch(NULL);
    	switch(ptype)
        {
	        case MOB_PROG:  vch = get_char_room(ch, arg);		    break;
    	    case OBJ_PROG:  vch = obj_get_char_room(obj, arg);		break;
	        case ROOM_PROG: vch = room_get_char_room(room, arg);	break;
        }

        if (vch != NULL)
            text = vch->description;
        else
	    {
            // No char found, look for an object
            OBJ_DATA * vobj(NULL);
	        switch(ptype)
    	    {
	    	    case MOB_PROG:  vobj = get_obj_here(ch, arg);		break;
    		    case OBJ_PROG:	vobj = obj_get_obj_here(obj, arg);	break;
        		case ROOM_PROG:	vobj = get_obj_list(NULL, arg, room->contents); break;
	        }

            if (vobj != NULL)
                text = vobj->description;
	    }
    }

    // If no text loaded at this time, bail out
    if (text == NULL)
    {
        LOGBUG("No target text found for mploaddesc");
        return;
    }

    // Check for data progs to load into
    MPROG_DATA * progData(lookupProgData(ch, obj, room, argument, ptype, DATA_PROG));
    if (progData != NULL)
    {
        // Found a data prog, give it the new text
        free_string(progData->comlist);
        progData->comlist = str_dup(text);

        // If the progState is set, reset it
        ProgState * progState(lookupProgState(ch, obj, room, ptype));
        if (progState != NULL && progState->data_prog == progData)
            progState->data_marker = progData->comlist;
        
        return;
    }

    // Check for a var to load into
    PROG_VARDATA * c_var(NULL);
    switch (ptype)
	{
	    case MOB_PROG: c_var = mprog_find_pvar(arg, ch->prog_data, FALSE); break;
	    case OBJ_PROG: c_var = mprog_find_pvar(arg, obj->prog_data, FALSE); break;
	    case ROOM_PROG: c_var = mprog_find_pvar(arg, room->prog_data, FALSE); break;
	}

	if (c_var == NULL)
    {
        LOGBUG("Invalid data prog or var target supplied to mploaddesc");
        return;
    }

    // Put the text into the c_var
    free_string(c_var->value);
    c_var->value = str_dup(text);
}

PROG_FUN( prog_desc )
{
    char arg[MAX_STRING_LENGTH];
    char keyword[MAX_STRING_LENGTH];
    bool found;
    EXTRA_DESCR_DATA *ed = NULL;
    ROOM_INDEX_DATA *pRoom = NULL;
    CHAR_DATA *vch = NULL;
    OBJ_DATA *vobj = NULL;
    HELP_DATA helpData = {0};
    MPROG_DATA *mprog(0);
    MPROG_DATA *oprog(0);

    argument = one_argument(argument, arg);

    if (argument[0] == '\0' || arg[0] == '\0')
	return;

    if (!str_cmp(arg, "here"))
    {
    	switch(ptype)
	    {
	        case MOB_PROG:  pRoom = ch->in_room;	break;
    	    case OBJ_PROG:  pRoom = obj->in_room;	break;
	        case ROOM_PROG: pRoom = room;		break;
    	}
    }
    else if (!str_cmp(arg, "self"))
    {
    	switch(ptype)
	    {
	        case MOB_PROG:  vch	  = ch;
    	    case OBJ_PROG:  vobj  = obj;
	        case ROOM_PROG: pRoom = room;
    	}
    }
    else if (is_number(arg))
	    pRoom = find_location(ch, obj, arg);
    else if (!str_prefix("helpfile ", arg))
    {
        // Copy the rest of the arg after the "helpfile " into keyword
        char * remnant = one_argument(arg, keyword);
        strcpy(keyword, remnant);
        helpData.keyword = keyword;
    }
    else
    {
    	switch(ptype)
        {
	        case MOB_PROG:  vch = get_char_room(ch, arg);		break;
    	    case OBJ_PROG:  vch = obj_get_char_room(obj, arg);		break;
	        case ROOM_PROG: vch = room_get_char_room(room, arg);	break;
        }

    	if (!vch)
	    {
	        switch(ptype)
    	    {
	    	    case MOB_PROG:  vobj = get_obj_here(ch, arg);		break;
    		    case OBJ_PROG:	vobj = obj_get_obj_here(obj, arg);	break;
        		case ROOM_PROG:	vobj = get_obj_list(NULL, arg, room->contents); break;
	        }
	    }
    }

    if (!pRoom && !vch && !vobj && helpData.keyword == NULL)
        return;

    argument = one_argument(argument, arg, true);
    found = FALSE;

    switch (ptype)
    {
        case MOB_PROG:
            for (mprog = ch->pIndexData->mobprogs; mprog; mprog = mprog->next)
            if ((mprog->type == DATA_PROG) && !str_cmp(mprog->arglist, arg))
            {
                strcpy(arg, mprog->comlist);
                found = TRUE;
                break;
            }
            break;

        case OBJ_PROG:
            for (oprog = obj->pIndexData->objprogs; oprog; oprog = oprog->next)
            if ((oprog->type == DATA_PROG) && !str_cmp(oprog->arglist, arg))
            {
                strcpy(arg, oprog->comlist);
                found = TRUE;
                break;
            }
            break;

        case ROOM_PROG:
            for (mprog = room->mobprogs; mprog; mprog = mprog->next)
            if ((mprog->type == DATA_PROG) && !str_cmp(mprog->arglist, arg))
            {
                strcpy(arg, mprog->comlist);
                found = TRUE;
                break;
            }
            break;
    }

    if (!found)
    {
        PROG_VARDATA *c_var = NULL;

        switch(ptype)
        {
            case MOB_PROG: c_var = mprog_find_pvar(arg, ch->prog_data, FALSE); break;
            case OBJ_PROG: c_var = mprog_find_pvar(arg, obj->prog_data, FALSE); break;
            case ROOM_PROG: c_var = mprog_find_pvar(arg, room->prog_data, FALSE); break;
        }

        if (c_var)
        {
            strcpy(arg, c_var->value);
            strcat(arg, "\n\r");
        }
    }
    
    if (argument[0] != '\0')
    {
	if (pRoom)
	    ed = pRoom->extra_descr;
	else if (vobj)
	    ed = vobj->extra_descr;
	else
	    return;

	for ( ; ed; ed = ed->next )
	{
	    if ( is_name( argument, ed->keyword ) )
		break;
	}

	if (ed)
	{
	    free_string(ed->description);
	    ed->description = str_dup(arg);
	}
	else
	{
	    ed = new_extra_descr();

	    if (pRoom)
	    {
		ed->next = pRoom->extra_descr;
		pRoom->extra_descr = ed;
	    }
	    else
	    {
		ed->next = vobj->extra_descr;
		vobj->extra_descr = ed;
	    }

	    ed->keyword = str_dup(argument);
	    ed->description = str_dup(arg);
	}

	return;
    }

    if (pRoom)
    {
	free_string(pRoom->description);
	pRoom->description = str_dup(arg);
    }
    else if (vch)
    {
	free_string(vch->description);
	vch->description = str_dup(arg);
    }
    else if (helpData.keyword != NULL)
    {
        helpData.text = arg;
        save_helptext(&helpData);
    }
}

PROG_FUN( prog_listbounty )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d=NULL;
    CHAR_DATA *victim=NULL;
    bool found=FALSE;

    if (argument[0] == '\0')
	return;
 
    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, argument); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, argument);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, argument);	break;
    }

    if (!victim)
	return;

    send_to_char("Outstanding Bounties:\n\r\n\r",victim);

    for (d = descriptor_list; d; d = d->next)
	if (d->character && can_see(victim,d->character) && d->character->level < 52)
	    if (d->character->pcdata->bounty)
	    {
		sprintf(buf,"%s: %s\n\r",d->character->name,
		  value_to_str(d->character->pcdata->bounty*1000));
		send_to_char(buf,victim);
		found=TRUE;
	    }
    if (!found)
	send_to_char("None are present with a price on their head.\n\r",victim);
    return;
}

PROG_FUN( prog_concat )
{
    PROG_VARDATA *n_var, *c_var;
    PROG_RUNDATA *p_data = NULL;
    char arg[MAX_INPUT_LENGTH];
    char newstr[MAX_STRING_LENGTH];
    bool phrase;

    newstr[0] = '\0';

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
        return;

    switch(ptype)
    {
	case MOB_PROG:	p_data = ch->prog_data;	    break;
	case OBJ_PROG:  p_data = obj->prog_data;    break;
	case ROOM_PROG:	p_data = room->prog_data;   break;
    }

    if (!p_data)
	return;

    n_var = mprog_find_pvar(arg, p_data, TRUE);

    while (argument[0] != '\0')
    {
	if (argument[0] == '\'' || argument[0] == '"')
	    phrase = TRUE;
	else
	    phrase = FALSE;

	argument = one_argument(argument, arg);

	if (phrase)
	    strcat(newstr, arg);
	else
	{
	    c_var = mprog_find_pvar(arg, p_data, FALSE);

	    if (c_var)
		strcat(newstr, c_var->value);
	}
    }

    free_string(n_var->value);
    n_var->value = str_dup(newstr);

    return;
}   


PROG_FUN( prog_exit )
{
    CHAR_DATA *victim = NULL, *tVict = NULL;
    ROOM_INDEX_DATA *tRoom, *origRoom;
    EXIT_DATA *oldExit = NULL;
//    bool tempExit = FALSE;
    int direction;
    int vnum = -1;
    char arg[MAX_STRING_LENGTH];

    smash_tilde( argument );

    /* Get victim argument */

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg);	break;
    }

    if (!victim || !victim->in_room)
	return;

    /* Determine direction */

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if (is_number(arg))
	direction = atoi(arg);
    else
    {
	if      (!str_prefix(arg, "north")) direction = 0;
	else if (!str_prefix(arg, "east" )) direction = 1;
	else if (!str_prefix(arg, "south")) direction = 2;
	else if (!str_prefix(arg, "west" )) direction = 3;
	else if (!str_prefix(arg, "up"   )) direction = 4;
	else if (!str_prefix(arg, "down" )) direction = 5;
	else return;
    }

    /* Determine target room */

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if (is_number(arg))
    {
	vnum = atoi(arg);

	if ((tRoom = get_room_index(vnum)) == NULL)
	    return;
    }
    else
    {
	switch (ptype)
        {
	    case MOB_PROG:  tVict = get_char_world(ch, arg); 		break;
	    case OBJ_PROG:  tVict = obj_get_char_world(obj, arg);  	break;
	    case ROOM_PROG: tVict = room_get_char_world(room, arg);	break;
	}

	if (!tVict || !tVict->in_room)
	    return;

	tRoom = tVict->in_room;
    }

    if (!victim || !victim->in_room)
	return;
	

    origRoom = victim->in_room;

    oldExit = victim->in_room->exit[direction];

    victim->in_room->exit[direction] = new_exit();
    victim->in_room->exit[direction]->u1.to_room = tRoom;

    if ((argument[0] != '\0') && !str_cmp("follow", argument))
	move_char(victim, direction, TRUE);
    else
        move_char(victim, direction, FALSE);

    free_exit(origRoom->exit[direction]);
    origRoom->exit[direction] = oldExit;

    return;
}
 
PROG_FUN( prog_slotremove )
{
    CHAR_DATA *victim = NULL;
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    OBJ_DATA *pObj;
    int slot;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ((arg1[0] == '\0')
     || (arg2[0] == '\0'))
	return;

    if (!is_number(arg2))
	return;
    else
	slot = atoi(arg2);

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg1); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg1);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg1);	break;
    }

    if (!victim)
	return;

    for (pObj = victim->carrying; pObj; pObj = pObj->next_content)
    {
	if (IS_SET(pObj->worn_on, (1 << slot)))
	{
	    unequip_char(victim, pObj);
	    oprog_remove_trigger(pObj);
	    break;
	}
    }

    return;
}

PROG_FUN( prog_slotpurge )
{
    CHAR_DATA *victim = NULL;
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    int slot;
    OBJ_DATA *pObj, *obj_next;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ((arg1[0] == '\0')
     || (arg2[0] == '\0'))
	return;

    if (!is_number(arg2))
	return;
    else
	slot = atoi(arg2);

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg1); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg1);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg1);	break;
    }

    if (!victim)
	return;

    for (pObj = victim->carrying; pObj; pObj = obj_next)
    {
	obj_next = pObj->next_content;

//	if (pObj->wear_loc == slot)

	if (IS_SET(pObj->worn_on, (1 << slot)))
	{
	    extract_obj(pObj);
	    break;
	}
    }

    return;
}

PROG_FUN( prog_setskilled )
{
    if (ptype != MOB_PROG)
	return;

    ch->skilled = TRUE;
}

PROG_FUN( prog_unsetskilled )
{
    if (ptype != MOB_PROG)
	return;

    ch->skilled = FALSE;
}

PROG_FUN( prog_goto )
{
    ROOM_INDEX_DATA *location;

    if (argument[0] == '\0')
	return;

    if (ptype == ROOM_PROG)
	return;

    if ((location = find_location(ch, obj, argument)) == NULL)
	return;

    switch(ptype)
    {
	case MOB_PROG:
	    if (ch->fighting)
		stop_fighting_all(ch);
	    char_from_room(ch);
	    char_to_room(ch, location);
	    break;

	case OBJ_PROG:
	    if (obj->carried_by)
		obj_from_char(obj);
	    else
		obj_from_room(obj);
	    obj_to_room(obj, location);
	    break;
    }

    return;
}

PROG_FUN( prog_at )
{
    ROOM_INDEX_DATA *original = NULL, *location;
    CHAR_DATA *heldby = NULL;
    CHAR_DATA *wch, *fch = NULL;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if ((argument[0] == '\0') || (arg[0] == '\0'))
	return;

    if ((location = find_location(ch, obj, arg)) == NULL)
	return;

    switch(ptype)
    {
	case MOB_PROG:

	    /* a minor hack which should allow fights to proceed normally */
	    /* when mpats are used during  - Aeolis			  */
	    if ((fch = ch->fighting) != NULL)
	        stop_fighting(ch);

	    original = ch->in_room;
	    global_bool_ranged_attack = TRUE;
	    char_from_room(ch);
	    char_to_room(ch, location);
	    interpret(ch, argument);
	    for (wch = char_list; wch; wch = wch->next)
	    {
		if ((wch == ch) && IS_VALID(wch))
		{
		    char_from_room(wch);
		    char_to_room(wch, original);
		    if (fch)
			set_fighting(ch, fch);
		    break;
		}
	    }
	    global_bool_ranged_attack = FALSE;
	    break;

	case OBJ_PROG:
	   if (obj->carried_by)
	   {
		heldby = obj->carried_by;
		obj_from_char(obj);
	   }
	   else
	   {
		original = obj->in_room;
		obj_from_room(obj);
	   }
	   obj_to_room(obj, location);
	   ointerpret(obj, argument);
	   if (IS_VALID(obj))
	   {
		obj_from_room(obj);
		if (heldby)
		{
		    if (IS_VALID(heldby))
			obj_to_char(obj, heldby);
		    else
			extract_obj(obj);
		}
		else if (original)
		    obj_to_room(obj, original);
		else
		    extract_obj(obj);
	   }
	   break;

	case ROOM_PROG:
	    rinterpret(location, argument);
	    break;
    }

    return;
}

PROG_FUN( prog_lag )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim = NULL;
    int pulses;

    argument = one_argument(argument, arg);

    if ((argument[0] == '\0') || (arg[0] == '\0') || !is_number(argument))
	return;

    if ((pulses = atoi(argument)) < 1)
	return;

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg);	break;
    }

    if (!victim)
	return;

    WAIT_STATE(victim, UMAX(victim->wait, pulses * PULSE_VIOLENCE));

    return;
}

PROG_FUN( prog_transfer )
{
    char             arg1[ MAX_INPUT_LENGTH ];
    char             arg2[ MAX_INPUT_LENGTH ];
    char	     buf[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location = NULL;
    DESCRIPTOR_DATA *d;
    CHAR_DATA       *victim = NULL;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
	return;

    if ( !str_cmp( arg1, "all" ) )
    {
	for ( d = descriptor_list; d != NULL; d = d->next )
	{
	    if ( d->connected == CON_PLAYING
	    &&   d->character->in_room != NULL
	    && ((ptype != MOB_PROG) || ((d->character != ch) && can_see( ch, d->character ))))
	    {
		sprintf( buf, "%s %s", d->character->name, arg2 );
		do_transfer( d->character, buf );
	    }
	}
	return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if ( arg2[0] == '\0' )
    {
	switch(ptype)
        {
            case MOB_PROG:  location = ch->in_room;	break;
            case OBJ_PROG:  location = obj->carried_by ? obj->carried_by->in_room : obj->in_room; break;
            case ROOM_PROG: location = room;		break;
        }

        if (!location)
	    return;
    }
    else
    {
	if ( ( location = find_location( ch, obj, arg2 ) ) == NULL )
	    return;

	if ( room_is_private( location ) )
	    return;
    }

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg1); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg1);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg1);	break;
    }

    if (!victim)
	return;

    if ( victim->in_room == NULL )
	return;

    if ( victim->fighting != NULL )
	stop_fighting_all(victim);

    char_from_room( victim );
    char_to_room( victim, location );

    return;
}

PROG_FUN( prog_grouptransfer )
{
    char             arg1[ MAX_INPUT_LENGTH ];
    char             arg2[ MAX_INPUT_LENGTH ];
    ROOM_INDEX_DATA *location = NULL;
    CHAR_DATA       *victim = NULL, *vch, *vch_next;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
	return;

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if ( arg2[0] == '\0' )
    {
	switch(ptype)
        {
            case MOB_PROG:  location = ch->in_room;	break;
            case OBJ_PROG:  location = obj->carried_by ? obj->carried_by->in_room : obj->in_room; break;
            case ROOM_PROG: location = room;		break;
        }

        if (!location)
	    return;
    }
    else
    {
	if ( ( location = find_location( ch, obj, arg2 ) ) == NULL )
	    return;

	if ( room_is_private( location ) )
	    return;
    }

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg1); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg1);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg1);	break;
    }

    if (!victim)
	return;

    if ( victim->in_room == NULL )
	return;

    for (vch = victim->in_room->people; vch; vch = vch_next)
    {
	vch_next = vch->next_in_room;

	if (is_same_group(vch, victim))
	{
	    if (vch->fighting)
	        stop_fighting_all(vch);

	    global_linked_move = TRUE;
	    char_from_room(vch);
	    char_to_room(vch, location);
	}
    }

    return;
}


PROG_FUN( prog_trackstep )
{
TRACK_DATA *pTrack, *next;
EXIT_DATA *pexit;
int door = -1;

    if (ptype != MOB_PROG)
	return;

        if (ch->tracking == NULL || ch->in_room == NULL)
		return;


        for (pTrack = ch->in_room->tracks; pTrack->ch != NULL; pTrack = next)
               {
                next = pTrack->next;
                if (pTrack->ch == ch->tracking)
                  door = pTrack->direction;
               }

	if (door < 0)
	  return;
        
/*      if ( !IS_SET(ch->act, ACT_SENTINEL)   */
        if ( ch->tracking != NULL
        && ( door <= 5 )
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

PROG_FUN( prog_track )
{
    CHAR_DATA *victim;

    if (ptype != MOB_PROG)
	return;

    if (!strcmp(argument, "none"))
    {
	ch->tracking = NULL;
	return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL)
	return;

    ch->tracking = victim;
}

PROG_FUN( prog_wizi )
{
    switch (ptype)
    {
	case MOB_PROG:	SET_BIT(ch->affected_by, AFF_WIZI); break;
	case OBJ_PROG:	REMOVE_BIT(obj->extra_flags[0], ITEM_WIZI); break;
    }

    return;
}
		
PROG_FUN( prog_unwizi )
{
    switch (ptype)
    {
	case MOB_PROG:	REMOVE_BIT(ch->affected_by, AFF_WIZI);	break;
	case OBJ_PROG:	REMOVE_BIT(obj->extra_flags[0], ITEM_WIZI); break;
    }

    return;
}
		
PROG_FUN( prog_seewizi )
{
    SET_BIT(ch->affected_by, AFF_DETECT_WIZI);
    return;
}
		
PROG_FUN( prog_unseewizi )
{
    REMOVE_BIT(ch->affected_by, AFF_DETECT_WIZI);
    return;
}

PROG_FUN( prog_xp )
{		
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *victim = NULL;
    int xp;

    argument = one_argument(argument, arg);

    if ((argument[0] == '\0') || (arg[0] == '\0'))
	return;

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg);	break;
    }

    if (!victim)
	return;

    xp = atoi(argument);
	
    if (abs(xp) < 101)
	xp *= victim->level;

    gain_exp(victim, xp);

    return;
}


/* lets the mobile force someone to do something.  must be mortal level
   and the all argument only affects those in the room with the mobile */

PROG_FUN( prog_force )
{
    char arg[ MAX_INPUT_LENGTH ];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;
    	ROOM_INDEX_DATA *pRoom = NULL;

	    switch(ptype)
        {
            case MOB_PROG:  pRoom = ch->in_room;	break;
            case OBJ_PROG:  pRoom = obj->carried_by ? obj->carried_by->in_room : obj->in_room; break;
            case ROOM_PROG: pRoom = room;		break;
        }

        if (!pRoom)
	    return;

        for ( vch = pRoom->people; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next_in_room;

            if (((ptype == MOB_PROG) && can_see( ch, vch )) || (ptype != MOB_PROG))
                interpret( vch, argument );
        }
    }
    else
    {
        CHAR_DATA *victim = NULL;

        switch (ptype)
        {
            case MOB_PROG:  victim = get_char_world(ch, arg); 		break;
            case OBJ_PROG:  victim = obj_get_char_world(obj, arg);  	break;
            case ROOM_PROG: victim = room_get_char_world(room, arg);	break;
        }

        if (!victim)
            return;

        if ( victim == ch )
            return;

        interpret( victim, argument );
    }

    return;
}


PROG_FUN( prog_interpret )
{
    char arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim = NULL;

    argument = one_argument( argument, arg );

    if ((arg[0] == '\0') || (argument[0] == '\0'))
	return;

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg);	break;
    }

    if (!victim)
	return;

    if ( victim == ch )
        return;

    proginterpret( victim, argument );

    return;
}

PROG_FUN( prog_valueup )
{
    int amount = 1;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if (argument[0] != '\0' && is_number(argument))
        amount = atoi(argument);

    if (is_number(arg))
    {
        int slot = atoi(arg);

        if ((slot < 0) || (slot > 9))
	    return;

	switch(ptype)
	{
	    case MOB_PROG:  ch->mobvalue[slot] += amount;	break;
	    case OBJ_PROG:  obj->objvalue[slot] += amount;	break;
	    case ROOM_PROG: room->roomvalue[slot] += amount;	break;
	}
    }
    else
    {
        PROG_VARDATA *c_var = NULL;

        switch(ptype)
	{
	    case MOB_PROG: 
	        c_var = mprog_find_pvar(arg, ch->prog_data, FALSE);
		break;

	    case OBJ_PROG:
	        c_var = mprog_find_pvar(arg, obj->prog_data, FALSE);
		break;

	    case ROOM_PROG:
	        c_var = mprog_find_pvar(arg, room->prog_data, FALSE);
		break;
	}

	if (c_var && c_var->value[0] != '\0' && is_number(c_var->value))
	{
	    int newval = atoi(c_var->value) + amount;
	    char numstr[33];

	    free_string(c_var->value);
	    sprintf(numstr, "%d", newval);
	    c_var->value = str_dup(numstr);	}
    }
    

    return;
}

PROG_FUN( prog_valuedown )
{
    int amount = 1;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if (argument[0] != '\0' && is_number(argument))
        amount = atoi(argument);

    if (is_number(arg))
    {
        int slot = atoi(arg);

        if ((slot < 0) || (slot > 9))
	    return;

	switch(ptype)
	{
	    case MOB_PROG:  ch->mobvalue[slot] -= amount;	break;
	    case OBJ_PROG:  obj->objvalue[slot] -= amount;	break;
	    case ROOM_PROG: room->roomvalue[slot] -= amount;	break;
	}
    }
    else
    {
        PROG_VARDATA *c_var = NULL;

        switch(ptype)
	{
	    case MOB_PROG: 
	        c_var = mprog_find_pvar(arg, ch->prog_data, FALSE);
		break;

	    case OBJ_PROG:
	        c_var = mprog_find_pvar(arg, obj->prog_data, FALSE);
		break;

	    case ROOM_PROG:
	        c_var = mprog_find_pvar(arg, room->prog_data, FALSE);
		break;
	}

	if (c_var && c_var->value[0] != '\0' && is_number(c_var->value))
	{
	    int newval = atoi(c_var->value) - amount;
	    char numstr[33];

	    free_string(c_var->value);
	    sprintf(numstr, "%d", newval);
	    c_var->value = str_dup(numstr);
	}
    }
    

    return;
}

static bool determineValueRand(int & result, const char * argument)
{
    char arg[MAX_INPUT_LENGTH];
    
    // Get the lower bound
    argument = one_argument(argument, arg);
    int lower(atoi(arg));
    
    // Verify argument count
    if (argument[0] == '\0')
        return false;
    
    // Get the upper bound
    argument = one_argument(argument, arg);
    int upper(atoi(arg));
    
    // Check for a seed
    if (argument[0] == '\0')
    {
        // No seed, just calulcate the result
        result = number_range(lower, upper);
        return true;
    }

    // A seed provided, so use it
    int seed;
    if (is_number(argument))
        seed = atoi(argument);
    else
    {
        for (unsigned int i(0); argument[i] != '\0'; ++i)
            seed += argument[i];
    }

    // Set the seed, generate the number, and restore the seed
    srand(seed);
    result = number_range(lower, upper);
    srand(time(0));
    return true;
}

PROG_FUN(prog_valuerand)
{
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);

    if (is_number(arg))
    {
        int slot = atoi(arg);
        if (slot < 0 || slot >= MAX_MOBVALUE)
            return;

        int result;
        if (!determineValueRand(result, argument))
        {
            LOGBUG("Insufficient arguments");
            return;
        }

        int * values(lookupProgValues(ptype, ch, obj, room));
        if (values != NULL)
            values[slot] = result;
    }
    else
    {
        PROG_VARDATA *c_var(lookupProgVar(ptype, ch, obj, room, arg));
        if (c_var == NULL)
            return;
    
        int result;
        if (!determineValueRand(result, argument))
        {
            LOGBUG("Insufficient arguments");
            return;
        }
        
        char numstr[64];
        sprintf(numstr, "%d", result);
        copy_string(c_var->value, numstr);
    }
}

PROG_FUN(prog_setexit)
{
	// Obtain the room
	ROOM_INDEX_DATA * target_room(0);
	switch (ptype)
    {
		case MOB_PROG:  target_room = ch->in_room; break;
		case OBJ_PROG:  target_room = obj->in_room; break;
		case ROOM_PROG: target_room = room; break;
	}
	
	// Verify the room
	if (target_room == 0)
	{
		LOGBUG("NULL room in prog setexit");
		return;
	}

	// Obtain the door
	char arg[MAX_INPUT_LENGTH];
	argument = one_argument(argument, arg);
	size_t door = static_cast<size_t>(DetermineDirection(arg));
	
	// Verify the door
	if (door >= MAX_DIR)
	{
		std::ostringstream mess;
		mess << "Invalid direction in prog setexit, room " << target_room->vnum;
		LOGBUG(mess.str().c_str());
		return;
	}

	// Obtain the new exit room
	ROOM_INDEX_DATA * new_room(0);
	argument = one_argument(argument, arg);

    // Handle the case of removing an exit 
    if (strcmp(arg, "none") == 0)
    {
        if (target_room->exit[door] != NULL)
        {
            free_exit(target_room->exit[door]);
            target_room->exit[door] = NULL;
        }
        return;
    }

	// A vnum should be specified
	int vnum(atoi(arg));
	if (vnum <= 0 || (new_room = get_room_index(vnum)) == NULL)
	{
		std::ostringstream mess;
	    mess << "Invalid vnum in prog setexit, room " << target_room->vnum;
		LOGBUG(mess.str().c_str());
		return;
	}

	// Create the exit data if it does not exist
	if (target_room->exit[door] == NULL)
		target_room->exit[door] = new_exit();

	// Assign the new room index
	target_room->exit[door]->u1.to_room = new_room;
}

PROG_FUN( prog_valueset )
{
    int slot = -1;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if ((argument[0] == '\0') || (arg[0] == '\0'))
	return;

    if (is_number(arg))
    {
        if (!is_number(argument))
	    return;

        slot = atoi(arg);

        if ((slot < 0) || (slot > 9))
	    return;

	switch(ptype)
	{
	    case MOB_PROG:  ch->mobvalue[slot] = atoi(argument);	break;
	    case OBJ_PROG:  obj->objvalue[slot] = atoi(argument);	break;
	    case ROOM_PROG: room->roomvalue[slot] = atoi(argument);	break;
	}
    }
    else
    {
        PROG_VARDATA *c_var = NULL;

        switch(ptype)
	{
	    case MOB_PROG: 
	        c_var = mprog_find_pvar(arg, ch->prog_data, TRUE);
		break;

	    case OBJ_PROG:
	        c_var = mprog_find_pvar(arg, obj->prog_data, TRUE);
		break;

	    case ROOM_PROG:
	        c_var = mprog_find_pvar(arg, room->prog_data, TRUE);
		break;
	}

	if (c_var)
	{
	    free_string(c_var->value);
	    c_var->value = str_dup(argument);
	}
    }

    return;
}

PROG_FUN( prog_strlen )
{
    int slot = -1;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        LOGBUG("Insufficient arguments");
	    return;
    }

    if (is_number(arg))
    {
            slot = atoi(arg);

            if ((slot < 0) || (slot > 9))
            return;

        switch(ptype)
        {
            case MOB_PROG:  ch->mobvalue[slot] = strlen(argument);	break;
            case OBJ_PROG:  obj->objvalue[slot] = strlen(argument);	break;
            case ROOM_PROG: room->roomvalue[slot] = strlen(argument);	break;
        }
    }
}

PROG_FUN( prog_verbstop )
{
    verb_stop_issued = TRUE;
    return;
}

PROG_FUN( prog_focus )
{
    char arg[MAX_STRING_LENGTH];
    int fslot = 0;

    argument = one_argument(argument, arg);

    if ((argument[0] != '\0') && is_number(argument))
	fslot = atoi(argument);

    if ((fslot < 0) || ((ptype == MOB_PROG) && (fslot >= MAX_MOBFOCUS))
     || ((ptype == ROOM_PROG) && (fslot >= MAX_ROOMFOCUS)))
        return;

    switch (ptype)
    {
        case MOB_PROG:  ch->memfocus[fslot] = ((arg[0] == '\0') ? NULL : get_char_world(ch, arg)); break;
        case OBJ_PROG:	obj->objfocus[fslot] = ((arg[0] == '\0') ? NULL : obj_get_char_world(obj, arg));	break;
        case ROOM_PROG: room->roomfocus[fslot] = ((arg[0] == '\0') ? NULL : room_get_char_world(room, arg)); break;
    }
}  

PROG_FUN( prog_unfocus )
{
    int fslot = 0;

    if ((argument[0] != '\0') && is_number(argument))
	fslot = atoi(argument);

    if ((fslot < 0) || ((ptype == MOB_PROG) && (fslot >= MAX_MOBFOCUS))
     || ((ptype == ROOM_PROG) && (fslot >= MAX_ROOMFOCUS)))
	return;

    switch (ptype)
    {
	case MOB_PROG:  ch->memfocus[fslot] = NULL;	break;
	case OBJ_PROG:  obj->objfocus[fslot] = NULL;		break;
	case ROOM_PROG: room->roomfocus[fslot] = NULL;	break;
    }

    return;
}

PROG_FUN( prog_bitset )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim = NULL;
    int opt;
    int val = 0;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg);	break;
    }

    if (!victim || IS_NPC(victim))
	return;

    argument = one_argument(argument, arg);

    if ((argument[0] == '\0') || (arg[0] == '\0') || !is_number(arg))
	return;

    opt = atoi(arg);

    if ((opt < 1) || (opt > 65535))
	return;

    val = atoi(argument);

    if (val)
  	BIT_SET(victim->pcdata->bitptr, opt);
    else
  	BIT_CLEAR(victim->pcdata->bitptr, opt);

    return;
}

PROG_FUN( prog_zap )
{
    CHAR_DATA *tmp;

    if (ptype != OBJ_PROG)
	return;

    if ((tmp = obj->carried_by) == NULL)
	return;

    if (!tmp->in_room)
	return;

    obj_from_char(obj);
    obj_to_room(obj, tmp->in_room);

    return;
}

PROG_FUN( prog_tentacle )
{
    CHAR_DATA *vch;

    if (ptype != MOB_PROG)
	return;

    if (argument[0]=='\0')
    {
	act("The huge tentacles flail and grab at things mindlessly!", ch, NULL, NULL, TO_ROOM);
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
	    if (!vch || vch->level > 51)
		continue;
	    if (number_percent() < (50 - get_curr_stat(vch, STAT_DEX)))
	    {
		act("$n's tentacles grab at you, and you fall!", ch, NULL, vch, TO_VICT);
		WAIT_STATE(vch, UMAX(vch->wait, 2*PULSE_VIOLENCE));
	    }
	}
    }
    else
    {
	if ((vch = get_char_room(ch, argument)) == NULL)
	    return;
	if (vch->level > 51)
	    return;
	if (number_percent() < (50 - get_curr_stat(vch, STAT_DEX)))
	{
	    act("$n's tentacles grab at you, and you fall!", ch, NULL, vch, TO_VICT);
	    WAIT_STATE(vch, UMAX(vch->wait, 2*PULSE_VIOLENCE));
	}
    }
 
    return;
}
	

PROG_FUN( prog_vaultdrop )
{
    OBJ_DATA *pobj;
    char arg1 [MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *oldroom;

    if (ptype != MOB_PROG)
        return;

    argument = one_argument( argument, arg1 );

    pobj = get_obj_carry(ch, arg1, ch);

    if (IS_SET(pobj->extra_flags[0], ITEM_NODROP))
    {
	do_say(ch, "This thing is cursed, I cannot not drop it in the vault.");
	extract_obj(pobj);
	return;
    }

    if (ch->fighting != NULL)
    {
	do_say(ch, "I cannot place this in the vault while I am fighting.");
	do_drop(ch, pobj->name);
	return;
    }

    do_say(ch, "I will place this in the vault.");
    act("$n closes $s eyes, and is pulled through the void into the vault.", ch, NULL, NULL, TO_ROOM);
    oldroom = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, get_room_index(clan_table[oldroom->clan].hall));
    do_drop(ch, pobj->name);
    char_from_room(ch);
    char_to_room(ch, oldroom);

    do_emote(ch, "is thrust back through the void a moment later.");
    do_say(ch, "I have placed it in the vault.");
}

PROG_FUN( prog_revolt )
{
CHAR_DATA *demon = ch;

if (ptype != MOB_PROG)
    return;

if (!IS_NPC(demon))
	return;

if (demon->demontrack == NULL)
	{
	LOGBUG("mprevolt with no demontrack");
	return;
	}

if (!IS_AWAKE(demon))
	{
	demon->position = POS_STANDING;
	do_emote(demon, "wakes up and stands up.");
	}

if (demon->demonstate == 0 && demon->master == demon->demontrack && demon->master->in_room == demon->in_room)
	{
	do_emote(demon, "struggles against its mystical bonds.");
	do_emote(demon, "screams with fury as it tears lose its mystical shackles!");
	switch(number_range(1,3))
	{
	case 1:
	do_say(demon, "Foolish mortal! Think you to keep demonkind as pets? Your will has faltered!");
	break;
	case 2:
	do_say(demon, "It is a sad day for a mortal when they overestimate their ability to tame demonkind.");
	do_emote(demon, "chuckes softly, but has a glare of unquenchable malice.");
	break;
	case 3:
	do_say(demon, "Your magic and will have been sundered, fool!");
	do_say(demon, "You have overtapped yourself, and now, you will pay with your existence!");
	break;
	}
	affect_strip(demon, gsn_charm_person);
	REMOVE_BIT(demon->affected_by, AFF_CHARM);
	multi_hit(demon, demon->master, TYPE_UNDEFINED);
	}
}

PROG_FUN( prog_escape )
{
CHAR_DATA *demon = ch;

if (ptype != MOB_PROG)
    return;

if (!IS_NPC(demon))
	return;

if (demon->demontrack == NULL)
	{
	LOGBUG("mprevolt with no demontrack");
	return;
	}

if (!IS_AWAKE(demon))
	{
	demon->position = POS_STANDING;
	do_emote(demon, "wakes up and stands up.");
	}


if (demon->demonstate == 0 && demon->master == demon->demontrack)
	{
	do_emote(demon, "struggles against its mystical bonds.");
	do_emote(demon, "glares around with anger as it sunders its mystical chains!");
	do_say(demon, "Were I not weakened from my escape, I would destroy a mortal who dared imprison me.");
	do_say(demon, "But I have broken free, and the next time I will not be so easily enslaved.");
	do_emote(demon, "disppears quickly through a portal in the fabric of reality, escaping into the Void.");
	extract_char(demon, TRUE);
	}
}

void do_mpbarkjayes (CHAR_DATA *demon, char *argument)
{
char buf[MAX_STRING_LENGTH];
char arg1[MAX_STRING_LENGTH];
CHAR_DATA *mage;
CHAR_DATA *speaker;

int riddle;
int demonstate;

if (!IS_NPC(demon))
	return;

if (demon->demontrack == NULL)
	{
	bug("mpbarkjayes with no demontrack", 0);
	return;
	}

argument = one_argument(argument, arg1);
if ((speaker = get_char_room(demon, arg1)) == NULL)
	{
	bug ("Barkja caught a speech prog with no identified speaker.", 0);
	return;
	}

riddle = atoi(argument);
if (riddle < 1)
	{
	bug ("Barkja caught a speech prog with a bad riddle number.", 0);
	return;
	}
demonstate = demon->demonstate;
mage	= demon->demontrack;

if (mage != speaker)
	return;

if (riddle == demon->demonstate)
	demonstate = 100;

switch(demonstate)
{
case 0:
break;

case 100:
do_say(demon, "Astounding. A mortal with a semblance of wit.");
do_emote(demon, "bows his head a moment in respect.");
do_say(demon, "Well, I will serve, as long as you dare to have me.");
do_emote(demon, "grins widely.");
demon->master = demon->demontrack;
demon->leader = demon->demontrack;
demon->demonstate = 0;
affect_strip(demon, gsn_greaterdemonsummon);
break;

default:
do_say(demon, "Alas, so few succeed.");
if (demon->demontrack)
   sprintf(buf, "%s I will extract the price of your ignorance in blood!", demon->demontrack->name);
do_tell(demon, buf);
demon->demonstate = 0;
demon->tracking = demon->demontrack;
affect_strip(demon, gsn_charm_person);
REMOVE_BIT(demon->affected_by, AFF_CHARM);
if (demon->in_room && demon->demontrack && demon->demontrack->in_room)
  if (demon->demontrack->in_room == demon->in_room)
   multi_hit(demon, demon->demontrack, TYPE_UNDEFINED);
}
}

void do_mpbarkjaset (CHAR_DATA *demon, char *argument)
{
char buf[MAX_STRING_LENGTH];
CHAR_DATA *mage;
int demonstate;

if (!IS_NPC(demon))
	return;

if (demon->demontrack == NULL)
	{
	bug("mpchagrob with no demontrack", 0);
	return;
	}

demonstate = demon->demonstate;
mage	= demon->demontrack;

switch(demonstate)
{
case 101:
do_say(demon, "Well, mortal, you have called me forth.");
do_say(demon, "By the ancient agreement, I can have what I would, and within the Limits.");
do_say(demon, "As always, I will ask a riddle. Answer swiftly, if you value your life.");
do_emote(demon, "smiles dangerously.");
demon->demonstate = number_range(1, 19);
break;

case 0:
break;

default:
do_say(demon, "Alas, so few succeed.");
if (demon->demontrack)
   sprintf(buf, "%s I will extract the price of your ignorance in blood!", demon->demontrack->name);
do_tell(demon, buf);
demon->demonstate = 0;
demon->tracking = demon->demontrack;
affect_strip(demon, gsn_charm_person);
REMOVE_BIT(demon->affected_by, AFF_CHARM);
if (demon->in_room && demon->demontrack && demon->demontrack->in_room)
  if (demon->demontrack->in_room == demon->in_room)
    multi_hit(demon, demon->demontrack, TYPE_UNDEFINED);
}
}

void do_mpchagrob (CHAR_DATA *demon, char *argument)
{
char buf[MAX_STRING_LENGTH];
CHAR_DATA *mage;
OBJ_DATA *obj;
int demonstate;

if (!IS_NPC(demon))
	return;

if (demon->demontrack == NULL)
	{
	bug("mpchagrob with no demontrack", 0);
	return;
	}

demonstate = demon->demonstate;
mage	= demon->demontrack;

switch (demonstate)
{
case 101:
act("$n gazes calmly at $N.", demon, NULL, mage, TO_NOTVICT);
act("$n gazes calmly at you.", demon, NULL, mage, TO_VICT);
do_say(demon, "Well, then, you have summoned the Grand Vizier. My price via the ancient Agreement is a stone of power. Hand it over, or face the consequences.");
do_emote(demon, "waits expectantly.");
demon->demonstate = 50;
break;

case 0:
if (demon->master == demon->demontrack)
	{
	if (demon->carrying && demon->carrying->pIndexData->vnum == OBJ_VNUM_STONE_POWER)
		do_say(demon, "I serve. Do not taunt me with these stones.");
	else
		do_say(demon, "No. It is too late for that, I'm afraid.");
	}
break;

default:
	if (((obj = demon->carrying) == NULL) || (obj->pIndexData->vnum != OBJ_VNUM_STONE_POWER))
	{
	do_say(demon, "If I cannot have a stone of power, I will have your heart, mortal fool.");
	do_say(demon, "The Grand Vizier always extracts his price.");
        if (demon->demontrack)
        sprintf(buf, "%s I will extract the price from your blood, mortal fool!", demon->demontrack->name);
	do_tell(demon, buf);
	demon->demonstate = 0;
	demon->tracking = demon->demontrack;
	affect_strip(demon, gsn_charm_person);
	REMOVE_BIT(demon->affected_by, AFF_CHARM);
	if (demon->in_room && demon->demontrack && demon->demontrack->in_room)
	if (demon->demontrack->in_room == demon->in_room)
	multi_hit(demon, demon->demontrack, TYPE_UNDEFINED);
	return;
	}
	else
	{
	do_emote(demon, "smiles ever so slightly.");
	do_say(demon, "Well, perhaps mortals are useful after all. I will send this on its way to the netherworld.");
	do_say(demon, "And yes, I will serve, as long as your magic and will can keep me. Not long, I think.");
	obj_from_char(obj);
	extract_obj(obj);
        demon->master = demon->demontrack;
        demon->leader = demon->demontrack;
        demon->demonstate = 0;
        affect_strip(demon, gsn_greaterdemonsummon);
	}
break;
}
/* end of possibilities*/
}

void do_mppricina (CHAR_DATA *demon, char *argument)
{
char buf[MAX_STRING_LENGTH];
CHAR_DATA *mage;
OBJ_DATA *obj, *pobj, *next_obj;
int demonstate, ocount;

if (!IS_NPC(demon))
	return;

if (demon->demontrack == NULL)
	{
	bug("mppricina with no demontrack", 0);
	return;
	}

demonstate = demon->demonstate;
mage	= demon->demontrack;
obj = NULL;
ocount = 0;

switch (demonstate)
{
case 101:

while (!obj && ocount < 50)
	{
	ocount++;
	for (pobj = mage->carrying; pobj!=NULL; pobj = next_obj)
		{
		next_obj = pobj->next_content;
		if ( number_bits(5) == 0 && pobj->item_type <= 10)
			{
			obj = pobj;
			break;
			}
		}
	}

if (obj)
	{
	demon->demonstate = obj->pIndexData->vnum;
	do_emote(demon, "looks around eagerly, and looks around for things to take.");
	do_say(demon, "Well, mortal, I'm pleased to come once again to your puny world, to claim more treasures.");
	do_say(demon, "By the terms of the ancient Agreement, I will ply my greed upon you.");
	sprintf(buf, "I will have from you %s. If you cannot or will not provide it, you have defaulted, and you will pay.", obj->short_descr);
	do_say(demon, buf);
	do_emote(demon, "taps her foot impatiently.");
	return;
	}
else
	{
	demon->demonstate = 0;
	do_say(demon, "You mortal fool! How dare you call me out of my dimensions for your pathetic tasks, and have nothing worth payment for such services?!");
	do_say(demon, "It will be better that you had never been born!");
	multi_hit(demon, demon->demontrack, TYPE_UNDEFINED);
        affect_strip(demon, gsn_greaterdemonsummon);
	return;
	}
break;

case 0:
return;

default:
	if (!demon->carrying || demon->carrying->pIndexData->vnum != demon->demonstate)
	{
	do_say(demon, "You pathetic fool. You failed to deliver, and now, you are mine!");
        if (demon->demontrack)
        sprintf(buf, "%s I wonder what price your hide will fetch!", demon->demontrack->name);
	do_tell(demon, buf);
	demon->demonstate = 0;
	demon->tracking = demon->demontrack;
	affect_strip(demon, gsn_charm_person);
	REMOVE_BIT(demon->affected_by, AFF_CHARM);
	if (demon->in_room && demon->demontrack && demon->demontrack->in_room)
	if (demon->demontrack->in_room == demon->in_room)
        affect_strip(demon, gsn_greaterdemonsummon);
	multi_hit(demon, demon->demontrack, TYPE_UNDEFINED);
	return;
	}
	else
	{
	do_say(demon, "Ah. Always enjoyable to see a humans price. I will serve, for now.");
	obj = demon->carrying;
	obj_from_char(obj);
	extract_obj(obj);
	do_say(demon, "Well, mortal, you have given true. I will serve you, while your feeble bond holds.");
        demon->master = demon->demontrack;
        demon->leader = demon->demontrack;
        demon->demonstate = 0;
        affect_strip(demon, gsn_greaterdemonsummon);
	}
break;
}
/* end of possibilities*/
}

void do_mpagduk (CHAR_DATA *demon, char *argument)
{
char buf[MAX_STRING_LENGTH];
CHAR_DATA *mage;
OBJ_DATA *obj, *pobj, *next_obj;
int demonstate, ocount = 0;

if (!IS_NPC(demon))
	return;

if (demon->demontrack == NULL)
	{
	bug("mpagduk with no demontrack", 0);
	return;
	}

demonstate = demon->demonstate;
mage	= demon->demontrack;
obj = NULL;
next_obj = NULL;
pobj = NULL;

switch (demonstate)
{
case 101:
do_say(demon, "You fool. I am the son of Xthjich and Pricina, and you dare to summon me?!");
do_emote(demon, "growls angrily.");
do_say(demon, "Your will may be strong, but we shall see if you can provide the price.");

while (!obj && ocount < 50)
	{
	ocount++;
	for (pobj = mage->carrying; pobj!=NULL; pobj = next_obj)
		{
		next_obj = pobj->next_content;
		if ( number_bits(3) == 0 && pobj->item_type == ITEM_WEAPON && pobj->pIndexData->limit > 0)
			{
			obj = pobj;
			break;
			}
		}
	}

if (obj)
{
sprintf(buf, "%s will satisfy the Requirement. Provide it to me now, or face the consequences of defaulting.", obj->short_descr);
do_say(demon, buf);
demon->demonstate = 50;
break;
}
else
{
do_say(demon, "You fool! You would summon the Scion without preparation? You cannot pay!");
do_say(demon, "I am free, and you are dead!");
do_emote(demon, "roars with wild abandon!");
affect_strip(demon, gsn_greaterdemonsummon);
if (demon->demontrack)
multi_hit(demon, demon->demontrack, TYPE_UNDEFINED);
break;
}

case 0:
return;

default:
	if (demon->carrying && demon->carrying->item_type == ITEM_WEAPON &&
	demon->carrying->pIndexData->limit > 0)
	{
	do_say(demon, "You have saved yourself only momentarily, mortal. I will break your will.");
	do_say(demon, "I will overpower your magics. Then, I will devour you whole.");
	obj = demon->carrying;
	obj_from_char(obj);
	extract_obj(obj);
        demon->master = demon->demontrack;
        demon->leader = demon->demontrack;
        demon->demonstate = 0;
        affect_strip(demon, gsn_greaterdemonsummon);
	}
	else
	{
        if (demon->demontrack)
        sprintf(buf, "%s You inane incompetent fool! Death will be your reward!", demon->demontrack->name);
	do_tell(demon, buf);
	demon->demonstate = 0;
	demon->tracking = demon->demontrack;
	affect_strip(demon, gsn_charm_person);
        affect_strip(demon, gsn_greaterdemonsummon);
	REMOVE_BIT(demon->affected_by, AFF_CHARM);
	if (demon->in_room && demon->demontrack && demon->demontrack->in_room)
	if (demon->demontrack->in_room == demon->in_room)
	multi_hit(demon, demon->demontrack, TYPE_UNDEFINED);
	return;
	}
break;
}
/* end of possibilities*/
}

void do_mpisetaton (CHAR_DATA *demon, char *argument)
{
char buf[MAX_STRING_LENGTH];
CHAR_DATA *mage;
OBJ_DATA *obj, *pobj, *next_obj;
int demonstate, ocount = 0;

if (!IS_NPC(demon))
	return;

if (demon->demontrack == NULL)
	{
	bug("mpisetaton with no demontrack", 0);
	return;
	}

demonstate = demon->demonstate;
mage	= demon->demontrack;
obj = NULL;
next_obj = NULL;
pobj = NULL;

switch (demonstate)
{
case 101:
do_say(demon, "I am the Sultan of the Underworld, mortal. You had best have an iron will.");
do_emote(demon, "concentrates a moment, then frowns.");
do_say(demon, "Well, I will at least extract the Price, in accord with the Agreement.");

while (!obj && ocount < 50)
	{
	ocount++;
	for (pobj = mage->carrying; pobj!=NULL; pobj = next_obj)
		{
		next_obj = pobj->next_content;
		if ( number_bits(3) == 0 && pobj->item_type == ITEM_ARMOR && pobj->pIndexData->limit > 0)
			{
			obj = pobj;
			break;
			}
		}
	}

if (obj)
{
sprintf(buf, "I will have from you %s. Or your life.", obj->short_descr);
do_say(demon, buf);
demon->demonstate = 50;
break;
}
else
{
do_say(demon, "Of course, you have nothing which will satisfy my demands.");
do_say(demon, "You should not overtax your resources thus, mortal fool.");
affect_strip(demon, gsn_greaterdemonsummon);
if (demon->demontrack)
multi_hit(demon, demon->demontrack, TYPE_UNDEFINED);
break;
}

case 0:
return;

default:
	if (demon->carrying && demon->carrying->item_type == ITEM_ARMOR &&
	demon->carrying->pIndexData->limit > 0)
	{
	do_say(demon, "Ah, well, it seems you have a servant. For a time.");
	do_emote(demon, "shrugs, seemingly resigned.");
	obj = demon->carrying;
	obj_from_char(obj);
	extract_obj(obj);
        demon->master = demon->demontrack;
        demon->leader = demon->demontrack;
        demon->demonstate = 0;
        affect_strip(demon, gsn_greaterdemonsummon);
	}
	else
	{
        if (demon->demontrack)
        sprintf(buf, "%s You pathetic fool. I will devour you myself!", demon->demontrack->name);
	do_tell(demon, buf);
	demon->demonstate = 0;
	demon->tracking = demon->demontrack;
	affect_strip(demon, gsn_charm_person);
        affect_strip(demon, gsn_greaterdemonsummon);
	REMOVE_BIT(demon->affected_by, AFF_CHARM);
	if (demon->in_room && demon->demontrack && demon->demontrack->in_room)
	if (demon->demontrack->in_room == demon->in_room)
	multi_hit(demon, demon->demontrack, TYPE_UNDEFINED);
	return;
	}
break;
}
/* end of possibilities*/
}

void do_mpgharku (CHAR_DATA *demon, char *argument)
{
char buf[MAX_STRING_LENGTH];
CHAR_DATA *mage;
OBJ_DATA *obj;
int demonstate;

if (!IS_NPC(demon))
	return;

if (demon->demontrack == NULL)
	{
	bug("mpgharku with no demontrack", 0);
	return;
	}

demonstate = demon->demonstate;
mage	= demon->demontrack;

switch (demonstate)
{
case 101:

do_emote(demon, "looks about, confused, and then angry.");
do_say(demon, "You would pull me from the wells of ether! Mortal fiend!");
do_say(demon, "Then you had best understand the ancient agreement, and what you have called on.");
do_say(demon, "I demand an item of magic: a wand, a scroll, or a potion. Give it o'er now, or perish.");
do_emote(demon, "waits in anticipation of failure to comply.");
demon->demonstate = 50;
break;

case 0:
if (demon->master == demon->demontrack)
	{
	if (((obj = demon->carrying) == NULL) || ((obj->item_type != ITEM_POTION)
	&& (obj->item_type != ITEM_SCROLL) && (obj->item_type != ITEM_WAND) && (obj->item_type != ITEM_OIL)))
		do_say(demon, "I will not be appeased now! I demand blood!");
	else
		do_say(demon, "Ah! Magic. How I love its ether feel...");
	}
break;

default:
	if (((obj = demon->carrying) == NULL) || ((obj->item_type != ITEM_POTION)
	&& (obj->item_type != ITEM_SCROLL) && (obj->item_type != ITEM_WAND) && (obj->item_type != ITEM_OIL)))
	{
	do_say(demon, "Ye gods, I ask for magic, and what do I get?");
        if (demon->demontrack)
        sprintf(buf, "%s You have defaulted on a bargain with the netherworld, fool! You are doomed!", demon->demontrack->name);
	do_tell(demon, buf);
	demon->demonstate = 0;
	demon->tracking = demon->demontrack;
	affect_strip(demon, gsn_charm_person);
	REMOVE_BIT(demon->affected_by, AFF_CHARM);
	if (demon->in_room && demon->demontrack && demon->demontrack->in_room)
	if (demon->demontrack->in_room == demon->in_room)
	multi_hit(demon, demon->demontrack, TYPE_UNDEFINED);
	return;
	}
	else
	{
	do_say(demon, "So, mortal, you can deliver? Hmm.");
	do_emote(demon, "screams in ecstacy as it absorbs the magic from the price.");
	obj = demon->carrying;
	obj_from_char(obj);
	extract_obj(obj);
	do_say(demon, "Well, mortal, you have given true. I will serve you, while your feeble bond holds.");
        demon->master = demon->demontrack;
        demon->leader = demon->demontrack;
        demon->demonstate = 0;
        affect_strip(demon, gsn_lesserdemonsummon);
	}
break;
}
/* end of possibilities*/
}

void do_mporzub (CHAR_DATA *demon, char *argument)
{
char buf[MAX_STRING_LENGTH];
CHAR_DATA *mage;
OBJ_DATA *obj;
int demonstate;

if (!IS_NPC(demon))
	return;
	
if (demon->demontrack == NULL)
	{
	bug("mporzub with no demontrack", 0);
	return;
	}

demonstate = demon->demonstate;
mage	= demon->demontrack;

switch (demonstate)
{
case 101:

do_say(demon, "Mortal, you have called me from the depths, and I am hungry.");
do_emote(demon, "snarls hungrily.");
do_say(demon, "You would have me serve, by the ancient Agreement, then I demand flesh.");
do_say(demon, "Provide me with flesh to eat now, or face my wrath!");
demon->demonstate = 50;
break;

case 0:
if (demon->master == demon->demontrack)
	{
	do_say(demon,"I am not hungry, mortal.");
	do_drop(demon, "bodypart");
	}
	else
	{
	do_say(demon, "Bah. I feel feed upon mage flesh now!");
	do_emote(demon, "snarls viciously.");
	}
break;

default:
	if (( obj = get_obj_list(demon, "bodypart", demon->carrying)) == NULL)
	{
        do_say(demon, "I hunger for flesh!");
	do_emote(demon, "screeches insatiably.");
        if (demon->demontrack)
        sprintf(buf, "%s Mortal slime! I will tear you flesh from your bones!", demon->demontrack->name);
	do_tell(demon, buf);
	demon->demonstate = 0;
	demon->tracking = demon->demontrack;
	affect_strip(demon, gsn_charm_person);
	REMOVE_BIT(demon->affected_by, AFF_CHARM);
	if (demon->in_room && demon->demontrack && demon->demontrack->in_room)
	if (demon->demontrack->in_room == demon->in_room)
	multi_hit(demon, demon->demontrack, TYPE_UNDEFINED);
	return;
	}
	else
	{
	do_say(demon, "Ah! Sweet flesh!");
	do_eat(demon, "bodypart");
	affect_strip(demon, gsn_poison);
	REMOVE_BIT(demon->affected_by, AFF_POISON);
	do_emote(demon, "grins with wicked satisfaction.");
	do_say(demon, "You have fed me the flesh I desire. I will serve you, for now.");
        demon->master = demon->demontrack;
        demon->leader = demon->demontrack;
        demon->demonstate = 0;
        affect_strip(demon, gsn_lesserdemonsummon);
	}
break;
}
/* end of possibilities*/
}

PROG_FUN( prog_vaultgive )
{
char buf [MAX_STRING_LENGTH];
char arg1 [MAX_INPUT_LENGTH];
char arg2 [MAX_INPUT_LENGTH];
char arg3 [MAX_INPUT_LENGTH];
char arg4 [MAX_INPUT_LENGTH];
ROOM_INDEX_DATA *oldroom;
CHAR_DATA *target;
OBJ_DATA *pobj;
bool found;

    if (ptype != MOB_PROG)
        return;

	if (!IS_NPC(ch))
	{
	send_to_char("Huh?\n\r", ch);
	return;
	}

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );
	argument = one_argument( argument, arg3 );
	argument = one_argument( argument, arg4 );

	if (strcmp(arg2, "bring"))
		{
		return;
		}

	if (strcmp(arg3, "me:"))
		{
		return;
		}

	if ((target = get_char_room(ch, arg1)) == NULL)
	{
	return;
	}

	if (ch->in_room->clan != target->clan)
	{
	do_say(ch, "I'll not be getting anything from the vault for you!");
	return;
	}

	if (ch->fighting != NULL)
	{
	do_say(ch, "I'm not going to be able to get anything while I'm fighting!");
	return;
	}

	do_say(ch, "All right, I will try to retrieve it for you.");
	do_emote(ch, "closes his eyes, and is pulled through the void into the vault.");
	oldroom = ch->in_room;
	char_from_room(ch);
	char_to_room(ch, get_room_index(clan_table[target->clan].hall));
	
	if ((pobj = get_obj_list(ch, arg4, ch->in_room->contents)) == NULL)
		found = FALSE;
	else
		found = TRUE;

	if (!found)
	{
	char_from_room(ch);
	char_to_room(ch, oldroom);
	do_emote(ch, "is thrust back through the void a moment later.");
	do_say(ch, "I'm sorry, but I couldn't find what you asked for in the vault.");
	return;
	}

	if (get_carry_weight(*target) + get_obj_weight(pobj) > static_cast<int>(Encumbrance::CarryWeightCapacity(*target)))
	{
        char_from_room(ch);
        char_to_room(ch, oldroom);
        do_emote(ch, "is thrust back through the void a moment later.");
        do_say(ch, "I'm afraid you aren't strong enough to bear that weight of that. Perhaps you could drop something and I'll try again.");
        return;
	}


	obj_from_room(pobj);
	obj_to_char(pobj, ch);
	char_from_room(ch);
	char_to_room(ch, oldroom);
	do_emote(ch, "is thrust back through the void a moment later.");
	sprintf(buf, "%s %s", arg4, target->name);
	do_give(ch, buf);
}

PROG_FUN( prog_vaultshow )
{
char arg1 [MAX_INPUT_LENGTH];
CHAR_DATA *target;
ROOM_INDEX_DATA *oldroom;

if (ptype != MOB_PROG)
    return;

	if (!IS_NPC(ch))
	{
	send_to_char("Huh?\n\r", ch);
	return;
	}

	argument = one_argument( argument, arg1 );

	if ((target = get_char_room(ch, arg1)) == NULL)
	{
	return;
	}

	if (ch->in_room->clan != target->clan)
	{
	do_say(ch, "I'll not be showing you anything!");
	return;
	}

	if (ch->fighting != NULL)
	{
	do_say(ch, "I can't show you while I'm fighting!");
	return;
	}


	do_say(ch, "Look, then, upon the vault!");
	act("$N glares at you intensely, and you see a vision...", target, NULL, ch, TO_CHAR);
	act("$N glares at $n intensely, as if communicating something.", target, NULL, ch, TO_ROOM);

	oldroom = target->in_room;
	global_bool_ranged_attack = TRUE;
	char_from_room(target);
	char_to_room(target, get_room_index(clan_table[target->clan].hall));
	do_look(target, "auto");
	char_from_room(target);
	char_to_room(target, oldroom);
	global_bool_ranged_attack = FALSE;
	return;
}

PROG_FUN( prog_vaultprep )
{
char arg1 [MAX_INPUT_LENGTH];
CHAR_DATA *target;

if (ptype != MOB_PROG)
    return;

	if (!IS_NPC(ch))
	{
	send_to_char("Huh?\n\r", ch);
	return;
	}

	argument = one_argument( argument, arg1 );

	if ((target = get_char_room(ch, arg1)) == NULL)
	{
	return;
	}

	if (ch->in_room->clan != target->clan)
	{
	do_say(ch, "I'll not be getting anything from the vault for you!");
	return;
	}

	if (ch->fighting != NULL)
	{
	do_say(ch, "I'm not going to be able to get anything while I'm fighting!");
	return;
	}


	do_say(ch, "I will prepare to enter the vault. Think hard on what you desire.");
}

PROG_FUN( prog_remareaaffect )
{
    ROOM_INDEX_DATA *pRoom = NULL;
    int sn;

    if (argument[0] == '\0' || !is_number(argument))
	return;

    switch (ptype)
    {
        case MOB_PROG:  pRoom = ch->in_room;		break;
        case OBJ_PROG:  pRoom = obj->carried_by ? obj->carried_by->in_room : obj->in_room; break;
	case ROOM_PROG: pRoom = room;			break;
    }

    if (!pRoom)
        return;

    sn = atoi(argument);

    if ((sn < 0) || (sn > MAX_SKILL))
	return;

    area_affect_strip(pRoom->area, sn);

    return;
}


PROG_FUN( prog_remaffect )
{
    CHAR_DATA *victim = NULL;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if (!is_number(arg))
    {
	switch(ptype)
	{
	    case MOB_PROG:  victim = get_char_world(ch, arg);		break;
	    case OBJ_PROG:  victim = obj_get_char_world(obj, arg);	break;
	    case ROOM_PROG: victim = room_get_char_world(room, arg);	break;
	}

	if (!victim)
	    return;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0')
	    return;
    }
    else
    {
	switch(ptype)
	{
	    case MOB_PROG:  victim = ch;		break;
	    case OBJ_PROG:  victim = obj->carried_by;	break;
	}

	if (!victim)
	    return;
    }

    affect_strip(victim, atoi(arg));

    return;
}

PROG_FUN( prog_cast )
{
    CHAR_DATA *caster, *victim = NULL;
    bool use_fake = FALSE;
    int level, sn;
    char arg[MAX_INPUT_LENGTH];

    if (ptype == MOB_PROG)
	caster = ch;
    else if (ptype == OBJ_PROG)
    {
	if (obj->carried_by)
	    caster = obj->carried_by;
	else if (obj->in_room)
	{
	    caster = create_mobile(get_mob_index(MOB_VNUM_CASTER));
	    caster->short_descr = str_dup(obj->short_descr);
	    char_to_room(caster, obj->in_room);
	    use_fake = TRUE;
	}
	else
	    return;
    }
    else if (ptype == ROOM_PROG)
    {
	caster = create_mobile(get_mob_index(MOB_VNUM_CASTER));
	caster->short_descr = str_dup(room->name);
	char_to_room(caster, room);
	use_fake = TRUE;
    }
    else
	return;

    argument = one_argument(argument, arg);

    if ((arg[0] == '\0') || !is_number(arg))
	return;

    sn = atoi(arg);

    if ((sn < 1) || (sn > MAX_SKILL))
	return;

    argument = one_argument(argument, arg);

    if ((arg[0] == '\0') || !is_number(arg))
	return;

    level = atoi(arg);

    if (level < 1)
	return;

    if (level == 99)
    {
	switch(ptype)
	{
	    case MOB_PROG:  level = ch->level;		break;
	    case OBJ_PROG:  level = obj->level;		break;
	}
    }

    if (argument[0] != '\0')
    {
	target_name = argument;

        switch(ptype)
        {
            case MOB_PROG:  victim = get_char_world(ch, argument);		break;
	    case OBJ_PROG:  victim = obj_get_char_world(obj, argument);		break;
	    case ROOM_PROG: victim = room_get_char_world(room, argument);	break;
        }

/*
        if (!victim)
            return;
*/
    }

    if (ptype == MOB_PROG)
	do_mprog_cast(caster, sn, level, victim);
    else
    	obj_cast_spell(sn, level, caster, victim, NULL);

    if (use_fake)
        extract_char(caster, TRUE);
     
    return;
}

/* Well sure, this is a little less efficient than before.  But given the */
/* option between less efficient and double-coding flag changes, it was   */
/* not a difficult decision.  - Aeolis.					  */

PROG_FUN( prog_set )
{
    CHAR_DATA *caster;
    bool use_fake = FALSE;

    if (ptype == MOB_PROG)
        caster = ch;
    else if (ptype == OBJ_PROG)
    {
        global_obj_target = obj;

        if (obj->carried_by)
            caster = obj->carried_by;
        else if (obj->in_room)
        {
            caster = create_mobile(get_mob_index(MOB_VNUM_CASTER));
            caster->short_descr = str_dup(obj->short_descr);
            char_to_room(caster, obj->in_room);
            use_fake = TRUE;
        }
        else
            return;
    }
    else if (ptype == ROOM_PROG)
    {
        caster = create_mobile(get_mob_index(MOB_VNUM_CASTER));
        caster->short_descr = str_dup(room->name);
        char_to_room(caster, room);
        use_fake = TRUE;
    }
    else
        return;

    do_set(caster, argument);

    if (use_fake)
        extract_char(caster, TRUE);

    if (ptype == OBJ_PROG)
        global_obj_target = NULL;

    return;
}

/* Designed to do a single trap hit, based on mob level -- the mob is generated
   based on the trap skill for thieves, but since damage is not normally level-based,
   here we are */

PROG_FUN( prog_trap )
{
    int dam;
    char      arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    if (ptype != MOB_PROG)
	return;

    if ( !IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n\r", ch );
	return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	return;
    }

    if (IS_NPC(victim))
	return;

    if ( victim == ch )
    {
	return;
    }

    if ( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
    {
	return;
    }

    if ( ch->position == POS_FIGHTING )
    {	
	return;
    }

    dam = 4*ch->level;
    if (ch->level > victim->level)
	dam -= ((20*ch->level) / victim->level);
    
    if (ch->pIndexData->vnum == MOB_VNUM_TRAP)
    	damage(ch,victim,number_range( 1, UMAX(1,dam) ),gsn_trap,DAM_PIERCE,TRUE);
    else if (ch->pIndexData->vnum == MOB_VNUM_PUNJIPIT)
    {
	if (!ch->in_room || !ch->in_room->exit[5] || !ch->in_room->exit[5]->u1.to_room
	|| ch->in_room->exit[5]->u1.to_room->vnum < ROOM_VNUM_PUNJI_LOW
	|| ch->in_room->exit[5]->u1.to_room->vnum > ROOM_VNUM_PUNJI_HIGH
	|| ch->in_room->exit[5]->u1.to_room->exit[4]->u1.to_room != ch->in_room)
 	  {
	   extract_char(ch, TRUE);
	   return;
	  }
/*
	if (victim->level > ch->level ? victim->level - ch->level < 9 : ch->level - victim->level < 9 && 
	   (!is_affected(victim, gsn_punjipit)
	   || get_modifier(victim->affected, gsn_findcover) != 
  	   get_modifier(ch->in_room->exit[5]->u1.to_room->affected, gsn_findcover))
	   && !is_flying(victim))
	{
	  act("$n suddenly falls through camouflage, into a punji pit!", victim, NULL, NULL, TO_ROOM);
	  act("You suddenly find yourself falling through camouflage, into a punji pit!", victim, NULL, NULL, TO_CHAR);
	  REMOVE_BIT(ch->in_room->exit[5]->exit_info, EX_SECRET);
	  char_from_room(victim);
	  char_to_room(victim, ch->in_room->exit[5]->u1.to_room);
	  char_from_room(ch);
	  char_to_room(ch, victim->in_room);
	  do_look(victim, "auto");
	  WAIT_STATE(victim, UMAX(victim->wait, 3*PULSE_VIOLENCE));
	  room_affect_strip(ch->in_room, gsn_punjipit);
    	  damage(ch,victim,number_range(1,UMAX(1,dam)),gsn_punjipit,DAM_PIERCE,TRUE);
	  extract_char(ch, TRUE);
	  return;
	}
*/
      }
    else
    	damage(ch,victim,number_range( 1, UMAX(1,dam/4) ),gsn_pillage,DAM_PIERCE,TRUE);

	stop_fighting_all(ch);

    return;
}

PROG_FUN( prog_lock )
{
    char arg[MAX_INPUT_LENGTH];
    int door;

    if (ptype != MOB_PROG)
	return;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
        return;

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET(obj->value[1],EX_ISDOOR)
            ||  IS_SET(obj->value[1],EX_NOCLOSE))
                return;

            SET_BIT(obj->value[1],EX_LOCKED);
            return;
        }

        /* 'lock object' */
        if ( obj->item_type != ITEM_CONTAINER )
            return;

        SET_BIT(obj->value[1], CONT_LOCKED);
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'lock door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit   = ch->in_room->exit[door];

        if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
	    return;

        SET_BIT(pexit->exit_info, EX_LOCKED);

        /* lock the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
        &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
        &&   pexit_rev->u1.to_room == ch->in_room )
            SET_BIT( pexit_rev->exit_info, EX_LOCKED );
    }

    return;
}

PROG_FUN( prog_unlock )
{
    char arg[MAX_INPUT_LENGTH];
    int door;

    if (ptype != MOB_PROG)
	return;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
        return;

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET(obj->value[1],EX_ISDOOR))
		return;

            if (!IS_SET(obj->value[1],EX_CLOSED))
		return;

            if (!IS_SET(obj->value[1],EX_LOCKED))
		return;

            REMOVE_BIT(obj->value[1],EX_LOCKED);
            return;
        }

        /* 'unlock object' */
        if ( obj->item_type != ITEM_CONTAINER )
            return;
        if ( !IS_SET(obj->value[1], CONT_CLOSED) )
            return;
        if ( !IS_SET(obj->value[1], CONT_LOCKED) )
            return;

        REMOVE_BIT(obj->value[1], CONT_LOCKED);
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'unlock door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
            return;

        REMOVE_BIT(pexit->exit_info, EX_LOCKED);

        /* unlock the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
        &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
        &&   pexit_rev->u1.to_room == ch->in_room )
            REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
    }

    return;
}

/* Well sure, this is a little less efficient than before.  But given the */
/* option between less efficient and double-coding flag changes, it was   */
/* not a difficult decision.  - Aeolis.					  */

PROG_FUN( prog_flag )
{
    CHAR_DATA *caster;
    bool use_fake = FALSE;

    if (ptype == MOB_PROG)
	caster = ch;
    else if (ptype == OBJ_PROG)
    {
	if (obj->carried_by)
	    caster = obj->carried_by;
	else if (obj->in_room)
	{
	    caster = create_mobile(get_mob_index(MOB_VNUM_CASTER));
	    caster->short_descr = str_dup(obj->short_descr);
	    char_to_room(caster, obj->in_room);
	    use_fake = TRUE;
	}
	else
	    return;
    }
    else if (ptype == ROOM_PROG)
    {
	caster = create_mobile(get_mob_index(MOB_VNUM_CASTER));
	caster->short_descr = str_dup(room->name);
	char_to_room(caster, room);
	use_fake = TRUE;
    }
    else
	return;

    do_flag(caster, argument);

    if (use_fake)
	extract_char(caster, TRUE);
}

PROG_FUN( prog_string )
{
    CHAR_DATA *caster;
    bool use_fake = FALSE;

    if (ptype == MOB_PROG)
        caster = ch;
    else if (ptype == OBJ_PROG)
    {
        global_obj_target = obj;

        if (obj->carried_by)
            caster = obj->carried_by;
        else if (obj->in_room)
        {
            caster = create_mobile(get_mob_index(MOB_VNUM_CASTER));
            caster->short_descr = str_dup(obj->short_descr);
            char_to_room(caster, obj->in_room);
            use_fake = TRUE;
        }
        else
            return;
    }
    else if (ptype == ROOM_PROG)
    {
        caster = create_mobile(get_mob_index(MOB_VNUM_CASTER));
        caster->short_descr = str_dup(room->name);
        char_to_room(caster, room);
        use_fake = TRUE;
    }
    else
        return;

    do_string(caster, argument);

    if (use_fake)
        extract_char(caster, TRUE);

    if (ptype == OBJ_PROG)
        global_obj_target = NULL;
}

PROG_FUN( prog_stopfollow )
{
    CHAR_DATA *vch;
    
    if ((ptype != MOB_PROG) && (ptype != ROOM_PROG))
	return;

    for (vch = char_list; vch; vch = vch->next)
	if (vch->master == ch)
	    stop_follower(vch);

    return;
}

PROG_FUN( prog_loot )
{
    char arg1[MAX_INPUT_LENGTH];
    CHAR_DATA *vch;
    OBJ_DATA *obj_next, *vObj = NULL;

    if (ptype != MOB_PROG)
        return;

    if ( !IS_NPC(ch))
    {
	send_to_char( "Huh?\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg1);

    if ((vch = get_char_room(ch, arg1)) == NULL)
	if (((vObj = get_obj_here(ch, arg1)) == NULL)
	 || !vObj->contains)
	    return;

    if (!strcmp(argument, "all"))
    {
	if (vch)
	{
	    /* Take all the victims stuff */
	    for (obj = vch->carrying; obj; obj = obj_next)
	    {
		obj_next = obj->next_content;
		if (IS_SET(obj->wear_flags, ITEM_WEAR_SIGIL)
		|| IS_SET(obj->wear_flags, ITEM_PROG))
		    continue;

		if (obj->worn_on)
		{
		    unequip_char( vch, obj );
		    oprog_remove_trigger(obj);
		}

		obj_from_char(obj);
		obj_to_char(obj, ch);
	    }
	}
	else
	{
	    for (obj = vObj->contains; obj; obj = obj_next)
	    {
		obj_next = obj->next_content;
		if (IS_SET(obj->wear_flags, ITEM_WEAR_SIGIL)
		 || IS_SET(obj->wear_flags, ITEM_PROG))
		    continue;

		obj_from_obj(obj);
		obj_to_char(obj, ch);
	    }
	}
    }
    else if (vch && !strcmp(argument, "inv"))
    {
	/* Take everything a character has in their inventory */
	for (obj = vch->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;

	    if (IS_SET(obj->wear_flags,ITEM_WEAR_SIGIL)
	     || IS_SET(obj->wear_flags,ITEM_PROG))
		continue;

	    if (obj->worn_on)
	        continue;

	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	}   
    }
    else
    {
	if ((obj = get_obj_list(ch, argument, vch ? vch->carrying : vObj->contains)) == NULL)
	    return;
		
	if (IS_SET(obj->wear_flags, ITEM_WEAR_SIGIL)
	 || IS_SET(obj->wear_flags, ITEM_PROG))
	    return;

	if (obj->worn_on)
	{
	    unequip_char(vch, obj);
	    oprog_remove_trigger(obj);
	}

	vch ? obj_from_char(obj) : obj_from_obj(obj);
	obj_to_char(obj, ch);
    }

    return;
}

PROG_FUN( prog_castoff )
{
    if (ptype != OBJ_PROG)
	return;

//    if (obj->carried_by && (obj->wear_loc > -1))

    if (obj->carried_by && obj->worn_on)
	prog_cast( ch, obj, room, argument, ptype );

    return;
}

PROG_FUN( prog_trigger )
{
    CHAR_DATA *vch = NULL;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    switch(ptype)
    {
	case MOB_PROG:	vch = get_char_room(ch, arg);		break;
	case OBJ_PROG:  vch = obj_get_char_room(obj, arg);	break;
	case ROOM_PROG: vch = room_get_char_room(room, arg);	break;
    }

    if (!vch)
	return;

    switch(ptype)
    {
	case MOB_PROG:	mprog_percent_check(ch, vch, NULL, NULL, TRIGGER_PROG);	break;
	case OBJ_PROG:  oprog_percent_check(obj,vch, NULL, NULL, TRIGGER_PROG); break;
	case ROOM_PROG: rprog_percent_check(room,vch,NULL, NULL, TRIGGER_PROG); break;
    }
    return;
}


PROG_FUN( prog_roomcmd )
{
    char cmd[MAX_INPUT_LENGTH];
    char *optr;
    CHAR_DATA *vch, *vch_next;
    char arg[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *location = NULL;

    switch(ptype)
    {
        case MOB_PROG:  location = ch->in_room;	break;
        case OBJ_PROG:  location = obj->carried_by ? obj->carried_by->in_room : obj->in_room; break;
        case ROOM_PROG: location = room;		break;
    }

    if (!location)
	return;

    if (argument[0] == '\0')
	return;

    for (vch = location->people; vch; vch = vch_next)
    {
	vch_next = vch->next_in_room;

	memset(&cmd[0], 0, MAX_INPUT_LENGTH);

	switch(ptype)
	{
	    case MOB_PROG:	if (!ch || !ch->valid)		return;	break;
	    case OBJ_PROG:	if (!obj || !obj->valid)	return;	break;
	}

	if (!vch || !vch->valid || ((ptype == MOB_PROG) && (vch == ch)))
	    continue;

	for (optr = one_argument(argument, arg); arg[0] != '\0'; optr = one_argument(optr, arg) )
	{
	    strcat(cmd, " ");
	    if (!strcmp(arg, "all"))
	    {
/*
		strcat(cmd, "\"");
	    	strcat(cmd, vch->name);
	    	strcat(cmd, "\"");
*/

		if (IS_NPC(vch))
		    strcat(cmd, vch->unique_name);
		else
		    strcat(cmd, vch->name);
	    }
            else
	        strcat(cmd, arg);
	}

	switch(ptype)
	{
	    case MOB_PROG:	interpret(ch, cmd);	break;
	    case OBJ_PROG:	ointerpret(obj, cmd);	break;
	    case ROOM_PROG:	rinterpret(room, cmd);	break;
	}
    }

    return;
}

PROG_FUN( prog_remember )
{
    CHAR_DATA *victim;
    MEMORY_DATA *md;
    MEMORY_DATA *mdnew;

    if (ptype != MOB_PROG)
	return;

    if (!IS_NPC(ch))
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
	return;

    if ((victim = get_char_room(ch, argument)) == NULL)
	if ((victim = get_char_world(ch, argument)) == NULL)
	{
	    bug("bad mpremember argument: %d", ch->pIndexData->vnum);
	    return;
	}

    for (md = ch->memgroup; md != NULL; md = mdnew)
    {
	mdnew = md->next;
/*	if (!md->valid)
	    continue;
*/	if (md->ch == ch)
	    continue;
	if (md->ch == victim)
	    return;
    }

    if (ch->memgroup == NULL)
    {
	md = (MEMORY_DATA*)malloc(sizeof(*md));
	g_num_mobmemory++;
	md->ch = victim;
	md->next = NULL;
	md->value = 0;
//	md->valid = TRUE;
	ch->memgroup = md;
	return;
    }

/*
    for (md = ch->memgroup; md != NULL; md = mdnew)
    {
	mdnew = md->next;
	if (!md->valid)
	{
	    md->ch = victim;
	    md->value = 0;
	    md->valid = TRUE;
	    return;
	}
    }
*/

    md = (MEMORY_DATA*)malloc(sizeof(*md));
    g_num_mobmemory++;
    md->ch = victim;
    md->next = ch->memgroup;
    md->value = 0;
//    md->valid = TRUE;
    ch->memgroup = md;
    return;
}

static char * ResolveProgValues(int *& nums, char **& strs, char * argument, int ptype, CHAR_DATA * ch, OBJ_DATA * obj, ROOM_INDEX_DATA * room)
{
    char arg[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];

    // Initialize
    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);
    nums = 0;
    strs = 0;

    // Handle chars
    if (!str_cmp(arg, "char"))
    {
        // Look up the char
        CHAR_DATA * vch(NULL);
        switch (ptype)
        {
            case MOB_PROG: vch = get_char_world(ch, arg2); break;
            case OBJ_PROG: vch = obj_get_char_world(obj, arg2); break;
            case ROOM_PROG: vch = room_get_char_world(room, arg2); break;
        }

        // Load the values
        if (vch != NULL)
        {
            nums = vch->mobvalue;
            strs = vch->stringvalue;
        }
        return argument;
    }

    // Handle objs
    if (!str_cmp(arg, "obj"))
    {
        // Look up the obj
        OBJ_DATA * vobj(NULL);
        switch (ptype)
        {
            case MOB_PROG: vobj = get_obj_world(ch, arg2); break;
            case OBJ_PROG: vobj = obj_get_obj_world(obj, arg2); break;
            case ROOM_PROG: vobj = room_get_obj_world(room, arg2); break;
        }

        // Load the values
        if (vobj != NULL)
        {
            nums = vobj->objvalue;
            strs = vobj->stringvalue;
        }
        return argument;
    }

    // Handle rooms
    if (!str_cmp(arg, "room"))
    {
        // Look up the room
        ROOM_INDEX_DATA * troom(NULL);
        if (!str_cmp(arg2, "here"))
        {
            switch (ptype)
            {
                case MOB_PROG: troom = ch->in_room; break;
                case OBJ_PROG: troom = get_room_for_obj(*obj); break;
                case ROOM_PROG: troom = room; break;
            }
        }
        else if (is_number(arg2))
            troom = get_room_index(atoi(arg2));

        // Load the values
        if (troom != NULL)
        {
            nums = troom->roomvalue;
            strs = troom->stringvalue;
        }
        return argument;
    }
    
    return argument;
}

PROG_FUN(prog_copyvalues)
{
    // Syntax: mpcopyvalues all|num|str char|obj|room <source> char|obj|room <dest>
    char arg[MAX_STRING_LENGTH];
    argument = one_argument(argument, arg);

    // Determine whether to copy nums, strs, or both
    bool copyNums;
    bool copyStrs;
    if (!str_cmp(arg, "all")) {copyNums = true; copyStrs = true;}
    else if (!str_cmp(arg, "num")) {copyNums = true; copyStrs = false;}
    else if (!str_cmp(arg, "str")) {copyNums = false, copyStrs = true;}
    else
    {
        // Invalid, ignore
        LOGBUG("Invalid type supplied to mpcopyvalues, expected 'all', 'num', or 'str'");
        return;
    }

    // Determine the source
    int * sourceNums;
    char ** sourceStrs;
    argument = ResolveProgValues(sourceNums, sourceStrs, argument, ptype, ch, obj, room);
    if (sourceNums == 0 || sourceStrs == 0)
    {
        LOGBUG("Unable to resolve source in mpcopyvalues");
        return;
    }

    // Determine the destination
    int * destNums;
    char ** destStrs;
    ResolveProgValues(destNums, destStrs, argument, ptype, ch, obj, room);
    if (destNums == 0 || destStrs == 0)
    {
        LOGBUG("Unable to resolve destination in mpcopyvalues");
        return;
    }

    // Copy the values
    for (unsigned int i(0); i < MAX_MOBVALUE; ++i)
    {
        if (copyNums) destNums[i] = sourceNums[i];
        if (copyStrs) copy_string(destStrs[i], sourceStrs[i]);
    }
}

PROG_FUN( prog_forget )
{
    MEMORY_DATA *md, *mdnext, *mdlast = NULL;
    CHAR_DATA *victim;

    if (ptype != MOB_PROG)
	return;

    if (!IS_NPC(ch))
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (!strcmp(argument, "all"))
    {
	for (md = ch->memgroup; md != NULL; md = mdnext)
      	{
            mdnext = md->next;
	    free(md);
	    g_num_mobmemory--;
        }
	ch->memgroup = NULL;
        return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
	if ((victim = get_char_world(ch, argument)) == NULL)
	{
	    bug("bad mpforget argument: %d", ch->pIndexData->vnum);
	    return;
	}


    for (md = ch->memgroup; md != NULL; md = mdnext)
    {
	mdnext = md->next;
	if (md->ch == victim)
	{
	    if (mdlast)
		mdlast->next = md->next;
	    else
		ch->memgroup = md->next;
	    free(md);
	    g_num_mobmemory--;
	    break;
	}
	mdlast = md;
    }
}

PROG_FUN( prog_memvup )
{
    CHAR_DATA *victim;
    MEMORY_DATA *md, *md_next;

    if (ptype != MOB_PROG)
	return;
      
    if (argument[0] == '\0')
	return;

    if ((victim = get_char_room(ch, argument)) == NULL)
	if ((victim = get_char_world(ch, argument)) == NULL)
		return;

    for (md = ch->memgroup; md != NULL; md = md_next)
    {
	md_next = md->next;
	if (md->ch == victim)
	    md->value++;
    }

    return;
}

PROG_FUN( prog_memvdown )
{
    CHAR_DATA *victim;
    MEMORY_DATA *md, *md_next;

    if (ptype != MOB_PROG)
	return;

    if (argument[0] == '\0')
	return;

    if ((victim = get_char_room(ch, argument)) == NULL)
	if ((victim = get_char_world(ch, argument)) == NULL)
		return;

    for (md = ch->memgroup; md != NULL; md = md_next)
    {
	md_next = md->next;
	if (md->ch == victim)
	    md->value--;
    }

    return;
}
 
PROG_FUN( prog_memvrand )
{
    int lower, upper;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    MEMORY_DATA *md, *md_next;

    if (ptype != MOB_PROG)
	return;

    if (argument[0] == '\0')
	return;

    argument = one_argument(argument, arg1);

    if (argument[0] == '\0')
	return;

    argument = one_argument(argument, arg2);

    if (argument[0] == '\0')
	return;

    lower = atoi(arg1);
    upper = atoi(arg2);

    if ((victim = get_char_room(ch, argument)) == NULL)
	if ((victim = get_char_world(ch, argument)) == NULL)
	    return;

    for (md = ch->memgroup; md != NULL; md = md_next)
    {
	md_next = md->next;
	if (md->ch == victim)
 	    md->value = number_range(lower, upper);
    }

    return;
}

PROG_FUN( prog_memvset )
{
char arg1[MAX_INPUT_LENGTH];
CHAR_DATA *victim;
MEMORY_DATA *md, *md_next;
int value;

if (ptype != MOB_PROG)
    return;

  if (!IS_NPC(ch))
	{
	send_to_char("Huh?\n\r", ch);
	return;
	}

if (argument[0] == '\0')
	return;

argument = one_argument(argument, arg1);

if (argument[0] == '\0')
	return;

value = atoi(arg1);

if ((victim = get_char_room(ch, argument)) == NULL)
	if ((victim = get_char_world(ch, argument)) == NULL)
		return;

for (md = ch->memgroup; md != NULL; md = md_next)
	{
	md_next = md->next;
	if (md->ch == victim)
		md->value = value;
	}
}

PROG_FUN( prog_masstrigger )
{
    DESCRIPTOR_DATA *d, *d_next;

    for (d = descriptor_list;d != NULL;d = d_next)
    {
        d_next = d->next;
        if (d && d->connected == CON_PLAYING
	&& !IS_NPC(d->character))
	{
	    switch (ptype)
	    {
		case MOB_PROG:  mprog_percent_check(ch, d->character, NULL, NULL, TRIGGER_PROG); break;
		case OBJ_PROG:  oprog_percent_check(obj, d->character, NULL, NULL, TRIGGER_PROG); break;
		case ROOM_PROG: rprog_percent_check(room, d->character, NULL, NULL, TRIGGER_PROG); break;
	    }
	}
    }
}

PROG_FUN( prog_bittrigger )
{
    DESCRIPTOR_DATA *d, *d_next;
    int bit;


    if (argument[0] == '\0')
	return;

    if ((bit = atoi(argument)) < 1)
	return;

    for (d = descriptor_list;d != NULL;d = d_next)
    {
        d_next = d->next;
        if (d && d->connected == CON_PLAYING
	&& !IS_NPC(d->character)
	&& BIT_GET(d->character->pcdata->bitptr, bit))
	{
	    switch (ptype)
	    {
		case MOB_PROG:  mprog_percent_check(ch, d->character, NULL, NULL, TRIGGER_PROG); break;
		case OBJ_PROG:  oprog_percent_check(obj, d->character, NULL, NULL, TRIGGER_PROG); break;
		case ROOM_PROG: rprog_percent_check(room, d->character, NULL, NULL, TRIGGER_PROG); break;
	    }
	}
    }
}

PROG_FUN( prog_unbittrigger )
{
    DESCRIPTOR_DATA *d, *d_next;
    int bit;


    if (argument[0] == '\0')
	return;

    if ((bit = atoi(argument)) < 1)
	return;

    for (d = descriptor_list;d != NULL;d = d_next)
    {
        d_next = d->next;
        if (d && d->connected == CON_PLAYING
	&& !IS_NPC(d->character)
	&& !BIT_GET(d->character->pcdata->bitptr, bit))
	{
	    switch (ptype)
	    {
		case MOB_PROG:  mprog_percent_check(ch, d->character, NULL, NULL, TRIGGER_PROG); break;
		case OBJ_PROG:  oprog_percent_check(obj, d->character, NULL, NULL, TRIGGER_PROG); break;
		case ROOM_PROG: rprog_percent_check(room, d->character, NULL, NULL, TRIGGER_PROG); break;
	    }
	}
    }
}

PROG_FUN( prog_get )
{
    char arg[MAX_STRING_LENGTH]; 
    char arg2[MAX_STRING_LENGTH];

    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);

    if (arg[0] == '\0' || arg2[0] == '\0')
    	return;

    // Handle room targets
    if (!str_prefix(arg, "room"))
    {
        ROOM_INDEX_DATA * room_target(NULL);
        int * value(NULL);
        switch (ptype)
        {
            case MOB_PROG: room_target = ch->in_room; value = ch->mobvalue; break;
            case OBJ_PROG: room_target = get_room_for_obj(*obj); value = obj->objvalue; break;
            case ROOM_PROG: room_target = room; value = room->roomvalue; break;
        }

        if (room_target != NULL)
            rget(room_target, arg2, argument, value);

        return;
    }

    // Handle obj targets
    if (!str_prefix(arg, "object"))
    {
        OBJ_DATA * obj_target(NULL);
        int * value(NULL);
        switch (ptype)
        {
            case MOB_PROG: obj_target = get_obj_world(ch, arg2); value = ch->mobvalue; break;
            case OBJ_PROG: obj_target = obj_get_obj_world(obj, arg2); value = obj->objvalue; break;
            case ROOM_PROG: obj_target = room_get_obj_world(room, arg2); value = room->roomvalue; break;
        }

        if (obj_target != NULL)
            oget(obj_target, argument, value);

        return;
    }

    // Handle mob and skill targets    
    if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character") || !str_prefix(arg, "skill"))
    {
        CHAR_DATA * vch(NULL);
        switch(ptype)
        {
            case MOB_PROG:	vch = get_char_world(ch, arg2);		break;
            case OBJ_PROG:  vch = obj_get_char_world(obj, arg2);	break;
            case ROOM_PROG: vch = room_get_char_world(room, arg2);	break;
        }

        if (vch == NULL)
            return;

        if (!str_prefix(arg, "skill"))
        {
            switch(ptype)
            {
                case MOB_PROG:  sget(vch, argument, ch->mobvalue);	  break;
                case OBJ_PROG:  sget(vch, argument, obj->objvalue);   break;
                case ROOM_PROG: sget(vch, argument, room->roomvalue); break;
            }
        }
        else
        {
            switch(ptype)
            {
                case MOB_PROG:  mget(vch, argument, ch->mobvalue);	  break;
                case OBJ_PROG:  mget(vch, argument, obj->objvalue);   break;
                case ROOM_PROG: mget(vch, argument, room->roomvalue); break;
            }
        }
    }
}
 
PROG_FUN( prog_damage )
{
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char arg3[MAX_STRING_LENGTH];
    CHAR_DATA *victim = NULL;
    int numdice, dsize, plus;
    char buf[MAX_STRING_LENGTH];

	if (argument[0] == '\0')
		return;

	argument = one_argument(argument, arg1);

	if (argument[0] == '\0')
		return;

	argument = one_argument(argument, arg2);

	if (argument[0] == '\0')
		return;

	argument = one_argument(argument, arg3);

	if (argument[0] == '\0')
		return;

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg1); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg1);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg1);	break;
    }

    if (!victim)
	return;

	if ((numdice = atoi(arg2)) < 1)
		return;
	
	if ((dsize = atoi(arg3)) < 1)
		return;
	
	if ((plus = atoi(argument)) < 0)
		return;

	victim->hit -= (dice(numdice, dsize) + plus);

	if (victim->level > 51)
		victim->hit = UMAX(victim->hit, 1);
	update_pos(victim);

    if (victim->position == POS_DEAD)
    {
	if ((IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)) || IS_SET(victim->form, FORM_UNDEAD))
	    act("$n is DESTROYED!", victim, 0, 0, TO_ROOM);
	else
	    act("$n is DEAD!!", victim, NULL, NULL, TO_ROOM);
	if(!IS_NPC(victim))
	{
	    sprintf(buf,"%s died to prog damage in %s [%i].\n\r",victim->name,victim->in_room->name,victim->in_room->vnum);
	    log_string(buf);
	}
	raw_kill(victim);
    }

}

PROG_FUN( prog_damtype )
{
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *victim = NULL;
    int numdice, dsize, plus, dam_type, dam;
    char buf[MAX_STRING_LENGTH];
    if (argument[0] == '\0')
	return;

    argument = one_argument(argument, arg);

    if (argument[0] == '\0')
	return;

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg);	break;
    }

    if (!victim)
	return;

    argument = one_argument(argument, arg);

    if (argument[0] == '\0' || !is_number(arg))
	return;

    numdice = atoi(arg);

    argument = one_argument(argument, arg);

    if (argument[0] == '\0' || !is_number(arg))
	return;

    dsize = atoi(arg);

    argument = one_argument(argument, arg);

    if (argument[0] == '\0' || !is_number(arg) || !is_number(argument))
	return;

    plus = atoi(arg);
    dam_type = atoi(argument);

    dam = dice(numdice, dsize) + plus;
    bool immune(false);
    dam = modify_damage(NULL, victim, NULL, dam, TYPE_HIT, dam_type, immune);

    victim->hit -= dam;

    if (victim->level > 51)
	    victim->hit = UMAX(victim->hit, 1);

    update_pos(victim);

    if (victim->position == POS_DEAD)
    {
	if ((IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)) || IS_SET(victim->form, FORM_UNDEAD))
	    act("$n is DESTROYED!", victim, NULL, NULL, TO_ROOM);
	else
	    act("$n is DEAD!!", victim, NULL, NULL, TO_ROOM);
  	if(!IS_NPC(victim))
	{
	    sprintf(buf,"%s was killed by prog damage in %s [%i].\n\r",victim->name,victim->in_room->name,victim->in_room->vnum);
	    log_string(buf);
	}
        raw_kill(victim);
    }
}

/* Why?  Because I couldn't get the "pow" function to work
 */
int math_power(int base, int power)
{
    int total = base;
    int i;

    if (power == 0)
	return 1;

    for (i = 1; i < power; i++)
	total *= base;

    return total;
}

bool is_negative(char *mathchr, char *mathstr)
{
    if (*mathchr != '-')
	return FALSE;

    mathchr -= 1;

    while (mathchr >= mathstr)
    {
	if (isdigit(*mathchr))
	    return FALSE;
	else if (isspace(*mathchr))
	    mathchr -= 1;
	else
	    return TRUE;
   }

    return TRUE;
}

int math_get_left(char *mathchr, int max)
{
    int digits = 0;    
    int total = 0;

    mathchr -= 1;

    while (!isdigit(*mathchr))
    {
	mathchr -= 1;
	if (--max == 0)
	    return 0;
    }

    while (isdigit(*mathchr))
    {
        total += ((*mathchr - 48) * math_power(10,digits++)); 

	if (--max == 0)
	    return total;

	mathchr -= 1;
    }

    if (is_negative(mathchr, mathchr - max))
	return total * -1;
    else
        return total;
}

int math_get_right(char *mathchr)
{
    int total = 0;
    bool negative = FALSE;

    mathchr = mathchr + 1;

    while (!isdigit(*mathchr) && *mathchr != '-')
	mathchr = mathchr + 1;

    if (*mathchr == '-')
    {
	negative = TRUE;
	mathchr = mathchr + 1;
    }

    while (isdigit(*mathchr))
    {
	total *= 10;
	total += (*mathchr - 48);
	mathchr = mathchr + 1;
    }

    if (negative)
	return total * -1;
    else
        return total;
}

void math_calc_replace(char *mathstr, char *mathchr, int result)
{
    char *a = mathchr - 1, *b = mathchr + 1;
    char newmath[255];

    while (!isdigit(*a))
	a -= 1;

    if (a != mathstr)
    {
        while (isdigit(*a))
        {
	    a -= 1;
	    if ((a == mathstr) || (!isdigit(a[-1]) && !is_negative(a-1, mathstr)))
	        break;
        }
    }

    while (!isdigit(*b))
	b += 1;

    if (isdigit(b[1]))
    {
        while (isdigit(*b))
        {
	    b += 1;
	    if (!isdigit(b[1]))
	        break;
        }
    }

    memset((void *) newmath, 0, sizeof(newmath));

    if (a != mathstr)
    {
        strncpy(newmath, mathstr, a - mathstr);
        sprintf(newmath, "%s%d%s", newmath, result, b+1);
    }
    else
	sprintf(newmath, "%d%s", result, b+1);

    strcpy(mathstr, newmath);

    return;
}

int process_math(char *mathstr)
{
    char *a = NULL, *b = NULL;
    char mathproc[255];
    char newmath[255];
    int in_result; 
    bool bFound;
    unsigned short level;

    while ((a = strchr(mathstr, '(')))
    {
	b = a + 1;
	level = 0;
	bFound = FALSE;

	while (*b != '\0')
	{
	    if (*b == '(')
		level++;

	    if (*b == ')')
	    {
		if (level == 0)
		{
		    bFound = TRUE;
		    memset((void *) mathproc, 0, sizeof(mathproc));
		    in_result = process_math(strncpy(mathproc, a+1, b-a-1));
		    memset((void *) newmath, 0, sizeof(newmath));
		    if (a != mathstr)
		    { 
			strncpy(newmath, mathstr, a - mathstr);
			sprintf(newmath, "%s%d%s", newmath, in_result, b+1);
		    }
		    else
		        sprintf(newmath, "%d%s", in_result, b+1);

		    strcpy(mathstr, newmath);

		    break;
		}
		else
		    level--;
	    }

	    b++;
	}
/*
	if ((b = strrchr(mathstr, ')')))
	{
	    memset((void *) mathproc, 0, sizeof(mathproc));
	    in_result = process_math(strncpy(mathproc, a+1, b-a-1));
	    memset((void *) newmath, 0, sizeof(newmath));
	    if (a != mathstr)
	    { 
	        strncpy(newmath, mathstr, a - mathstr);
	        sprintf(newmath, "%s%d%s", newmath, in_result, b+1);
	    }
	    else
		sprintf(newmath, "%d%s", in_result, b+1);

	    strcpy(mathstr, newmath);
	}
	else
	    return 0;
*/
	
	if (!bFound)
	    return 0;
    }

    /* Excessive nulls due to strange errors during initials testruns */

    a = NULL;
    b = NULL;

    while ((a = strchr(mathstr, '^')))
    {
	math_calc_replace(mathstr, a, math_power(math_get_left(a, a-mathstr), math_get_right(a)));
	a = NULL;
    }

    while ((a = strchr(mathstr, '*')) || (b = strchr(mathstr, '/')))
    {
	if (b)
	    math_calc_replace(mathstr, b, math_get_left(b, b-mathstr) / math_get_right(b));
	else
	    math_calc_replace(mathstr, a, math_get_left(a, a-mathstr) * math_get_right(a));

	a = NULL;
	b = NULL;
    }

    while ((a = strchr(mathstr, '+')) || (b = strchr(mathstr + 1, '-')))
    {
        if (b)
            math_calc_replace(mathstr, b, math_get_left(b, b-mathstr) - math_get_right(b));
        else
            math_calc_replace(mathstr, a, math_get_left(a, a-mathstr) + math_get_right(a));

	a = NULL;
	b = NULL;
    }

    return atoi(mathstr);
} 

PROG_FUN( prog_math )
{
    char arg[MAX_INPUT_LENGTH];

    if (argument[0] == '\0')
	return;

    argument = one_argument(argument, arg);

    if (argument[0] == '\0')
	return;
    
    if (is_number(arg))
    {
        int result, slot = atoi(arg);

	if ((slot < 0) || (slot > 9))
	    return;

	result = process_math(argument);

	switch (ptype)
	{
	    case MOB_PROG:	ch->mobvalue[slot] = result;	break;
	    case OBJ_PROG:	obj->objvalue[slot] = result;	break;
	    case ROOM_PROG:	room->roomvalue[slot] = result;	break;
	}
    }
    else
    {
        PROG_VARDATA *c_var = NULL;

        switch(ptype)
	{
	    case MOB_PROG: 
	        c_var = mprog_find_pvar(arg, ch->prog_data, TRUE);
		break;

	    case OBJ_PROG:
	        c_var = mprog_find_pvar(arg, obj->prog_data, TRUE);
		break;

	    case ROOM_PROG:
	        c_var = mprog_find_pvar(arg, room->prog_data, TRUE);
		break;
	}

	if (c_var)
	{
	    char numstr[33];

	    free_string(c_var->value);
	    sprintf(numstr, "%d", process_math(argument));
	    c_var->value = str_dup(numstr);
	}
    }


    return;
}

PROG_FUN(prog_dice)
{
    static const unsigned int ArgCount = 4;
    char args[ArgCount][MAX_INPUT_LENGTH];

    // Get the args from the argument
    for (unsigned int i(0); i < ArgCount; ++i)
    {
        // Check for some args
        if (argument[0] == '\0')
        {
            LOGBUG("Insufficient arguments in mpdice");
            return;
        }

        // Grab the arg
        argument = one_argument(argument, args[i]);
    }

    // Check the dice values, then calculate the result
    if (!is_number(args[1]) || !is_number(args[2]) || !is_number(args[3]))
    {
        LOGBUG("Invalid argument in mpdice");
        return;
    }
    int result(dice(atoi(args[1]), atoi(args[2])) + atoi(args[3]));

    // Get the target integer
    if (is_number(args[0]))
    {
        // The first arg is a number, so it references a memory slot
        int slot(atoi(args[0]));
    	if (slot < 0 || slot > 9)
        {
            LOGBUG("Invalid target memory slot for mpdice; must be between 0 and 9");
    	    return;
        }

        // Determine the target mob/obj/room
        switch (ptype)
        {
            case MOB_PROG:	ch->mobvalue[slot] = result;	break;
            case OBJ_PROG:	obj->objvalue[slot] = result;   break;
            case ROOM_PROG:	room->roomvalue[slot] = result;	break;
            default: LOGBUG("Invalid prog type in mpdice"); break;
        }

        return;
    }
    
    // The first argument is not a number, so look up the var it should represent
    PROG_VARDATA *c_var(NULL);
    switch (ptype)
    {
        case MOB_PROG:  c_var = mprog_find_pvar(args[0], ch->prog_data, true);      break;
        case OBJ_PROG:  c_var = mprog_find_pvar(args[0], obj->prog_data, true);     break;
        case ROOM_PROG: c_var = mprog_find_pvar(args[0], room->prog_data, true);    break;
    }

    if (c_var == NULL)
    {
        LOGBUG("Unknown target for mpdice");
        return;
    }

    // Write a string version of the value into the var
    std::ostringstream val;
    val << result;
    free_string(c_var->value);
    c_var->value = str_dup(val.str().c_str());
}

PROG_FUN( prog_permexdesc )
{
    CHAR_DATA *victim;
    EXTRA_DESCR_DATA *ed;
    char arg1[MAX_STRING_LENGTH];

    argument = one_argument(argument, arg1);

    if ((argument[0] == '\0') || (arg1[0] == '\0'))
	return;

    if ((victim = get_char_world(ch, arg1)) == NULL)
	return;

    if (IS_NPC(victim))
	return;

    argument = one_argument(argument, arg1);

    for (ed = victim->pcdata->extra_descr; ed; ed = ed->next)
    {
	if (!str_cmp(arg1, ed->keyword))
	{
	    if (ed->can_edit)
		ed->can_edit = FALSE;
	    else
		ed->can_edit = TRUE;

	    return;
	}
    }

    return;
}

PROG_FUN( prog_addskill )
{
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *victim = NULL;
    int sn, perc;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int *i;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    switch(ptype)
    {
	case MOB_PROG:	victim = get_char_world(ch, arg);	break;
	case OBJ_PROG:	victim = obj_get_char_world(obj, arg);	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg); break;
    }

    if (!victim)
	return;

    argument = one_argument(argument, arg);
 
    if (!is_number(arg))
	return;

    sn = atoi(arg);

    if ((sn < 0) || (sn >= MAX_SKILL))
	return;

    argument = one_argument(argument, arg);

    if (!is_number(arg))
	return;

    perc = atoi(arg);

    argument = one_argument(argument, arg);

    if ((arg[0] != '\0') && is_number(arg))
	af.duration = atoi(arg);
    else
	af.duration = -1;

    af.where     = TO_AFFECTS;
    af.type	 = gsn_skill;
    af.level     = victim->level;
    af.location  = APPLY_HIDE;
    af.modifier	 = sn;
    af.bitvector = 0;
    af.point     = malloc(sizeof(int));

    i = (int *) af.point;
    *i = perc;

    affect_to_char(victim, &af);

    return;
}

PROG_FUN( prog_removeskill )
{
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *victim = NULL;
    AFFECT_DATA *paf;
    int sn;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    switch(ptype)
    {
	case MOB_PROG:	victim = get_char_world(ch, arg);	break;
	case OBJ_PROG:	victim = obj_get_char_world(obj, arg);	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg); break;
    }

    if (!victim)
	return;

    argument = one_argument(argument, arg);

    if (!is_number(arg))
	return;

    sn = atoi(arg);

    for (paf = victim->affected; paf; paf = paf->next)
	if ((paf->type == gsn_skill) && (paf->modifier == sn))
	{
	    affect_remove(victim, paf);
	    break;
	}

    return;
}

PROG_FUN( prog_pathstep )
{
    // Sanity check
    if (ptype != MOB_PROG)
    {
        std::ostringstream mess;
        mess << "Pathstep prog can only be on an NPC (" << (ch == NULL ? "" : ch->name) << ")";
        LOGBUG(mess.str().c_str());
	    return;
    }

    StepAlongPath(ch);
}

PROG_FUN( prog_findpath )
{
    ROOM_INDEX_DATA *to_room;
    CHAR_DATA *victim = NULL;
    char arg[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];

    // Sanity checks
    if (ptype != MOB_PROG || ch->in_room == NULL)
    {
        std::ostringstream mess;
        mess << "Findpath prog can only be on an NPC, and the NPC must be in a room (" << (ch == NULL ? "" : ch->name) << ")";
        LOGBUG(mess.str().c_str());
	    return;
    }

    // Get the args
    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);
    if ((arg[0] == '\0') || (arg2[0] == '\0') || !is_number(arg2))
    {
        std::ostringstream mess;
        mess << "Missing or invalid arguments in findpath prog (" << ch->name << ")";
        LOGBUG(mess.str().c_str());
	    return;
    }

    bool includeClosed(false);
    if (argument[0] != '\0')
    {
        if (!str_cmp(argument, "true")) includeClosed = true;
        else if (!str_cmp(argument, "false")) includeClosed = false;
        else
        {
            std::ostringstream mess;
            mess << "Unexpected flag for whether to include closed doors in findpath prog (" << ch->name << "); should be 'true', 'false', or blank";
            LOGBUG(mess.str().c_str());
            return;
        }
    }

    // Find the target room
    ClearPath(ch);
    if (is_number(arg))
    {
	    if ((to_room = get_room_index(atoi(arg))) == NULL)
    	{
            std::ostringstream mess;
            mess << "Cannot resolve room from arg '" << arg << "' in findpath prog (" << ch->name << ")";
            LOGBUG(mess.str().c_str());
	        return;
    	}
    }
    else
    {
        switch (ptype)
        {
            case MOB_PROG:  victim = get_char_world(ch, arg);           break;
            case OBJ_PROG:  victim = obj_get_char_world(obj, arg);      break;
            case ROOM_PROG: victim = room_get_char_world(room, arg);    break;
        }

        if (!victim || !victim->in_room)
        {
            std::ostringstream mess;
            mess << "Cannot target (or target room) for arg '" << arg << "' in findpath prog (" << ch->name << ")";
            LOGBUG(mess.str().c_str());
            return;
        }

        to_room = victim->in_room;
    }

    // With the target room resolved, try to build the path
    int maxSteps(atoi(arg2));
	RoomPath roomPath(*ch->in_room, *to_room, ch, (maxSteps < 0 ? RoomPath::Infinite : static_cast<unsigned int>(maxSteps)), (includeClosed ? 0 : EX_CLOSED));
    if (roomPath.Exists())
        AssignNewPath(ch, roomPath);
}

PROG_FUN( prog_stealrandom )
{
    char buf[20], arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *pObj;
    int number = 0, item = 0;

    if (ptype != MOB_PROG)
	return;

    if (!ch->in_room)
	return;

    if (argument[0] == '\0')
	return;

    if ((victim = get_char_room(ch, argument)) == NULL)
	return;

    for (pObj = victim->carrying; pObj; pObj = pObj->next_content)
	if (can_see_obj(ch, pObj)
	 && can_drop_obj(victim, pObj)
	 && !IS_SET(pObj->extra_flags[0], ITEM_INVENTORY))
	    number++;

    if (number == 0)
	return;

    for (pObj = victim->carrying; pObj; pObj = pObj->next_content)
    {
	item++;
	if (can_see_obj(ch, pObj)
	 && can_drop_obj(victim, pObj)
	 && !IS_SET(pObj->extra_flags[0], ITEM_INVENTORY))
	{
	    number--;

	    if (number == 0)
	    {
		one_argument(victim->name, arg);
		sprintf(buf, "%d. %s", item, arg);
		do_steal(ch, buf);
		return;
	    }
	}
    }

    return;
}

PROG_FUN( prog_getroomvnum )
{
    ROOM_INDEX_DATA *pRoom = NULL;

    switch (ptype)
    {
	case MOB_PROG:	pRoom = ch->in_room;	break;
	case OBJ_PROG:	pRoom = obj->carried_by ? obj->carried_by->in_room : obj->in_room; break;
	case ROOM_PROG: pRoom = room;		break;
    }

    if (!pRoom)
	return;

    if (argument[0] == '\0')
	return;

    if (is_number(argument))
    {
        int slot = atoi(argument);

	if ((slot < 0) || (slot > 9))
	    return;

	switch(ptype)
	{
	    case MOB_PROG:	ch->mobvalue[slot] = pRoom->vnum;	break;
	    case OBJ_PROG:	obj->objvalue[slot] = pRoom->vnum;	break;
	    case ROOM_PROG: room->roomvalue[slot] = pRoom->vnum;	break;
	}
    }
    else
    {
        PROG_VARDATA *c_var = NULL;

        switch(ptype)
	{
	    case MOB_PROG: 
	        c_var = mprog_find_pvar(argument, ch->prog_data, TRUE);
		break;

	    case OBJ_PROG:
	        c_var = mprog_find_pvar(argument, obj->prog_data, TRUE);
		break;

	    case ROOM_PROG:
	        c_var = mprog_find_pvar(argument, room->prog_data, TRUE);
		break;
	}

	if (c_var)
	{
	    char numstr[33];

	    free_string(c_var->value);
	    sprintf(numstr, "%d", pRoom->vnum);
	    c_var->value = str_dup(numstr);
	}
    }

    return;
}

PROG_FUN ( prog_disarm )
{
    OBJ_DATA *rearm;
    OBJ_DATA *pobj;
    CHAR_DATA *victim;
    int chance;

    if (ptype != MOB_PROG)
	return;

    if ((victim = ch->fighting) == NULL)
	return;

    if ((chance = get_skill(ch,gsn_disarm)) == 0)
	return;

    if (((pobj = get_eq_char(victim, WEAR_WIELD)) == NULL) || !can_see_obj(ch, obj))
        return;

    if ( IS_OBJ_STAT(pobj,ITEM_NOREMOVE))
        return;

    /* skill */
    if ( get_eq_char(ch,WEAR_WIELD) == NULL)
        chance = chance * get_skill(ch, gsn_hand_to_hand) / 150;
    else
        chance = chance * get_weapon_skill(ch, get_weapon_sn(ch), TRUE) / 100;

    chance += (get_weapon_skill(victim, get_weapon_sn(victim), TRUE) /2
             - get_weapon_skill(ch->fighting,get_weapon_sn(ch->fighting),TRUE)) / 2;

    /* dex vs. strength */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= 2 * get_curr_stat(victim,STAT_STR);

    if (is_affected(victim,gsn_clumsiness))
        chance += 10;

    /* level */
    chance += (ch->level - victim->level) * 2;

    if (is_affected(victim, gsn_grip))
        chance /= 5;

    if (obj_is_affected(pobj, gsn_bindweapon))
        chance /= 5;

    /* and now the attack */
    if (number_percent() < chance)
    {
        oprog_remove_trigger(pobj);
        obj_from_char(pobj);
        if ( IS_OBJ_STAT(pobj,ITEM_NODROP) || IS_OBJ_STAT(pobj,ITEM_INVENTORY) )
            obj_to_char(pobj, victim );
        else
        {
            obj_to_room(pobj, victim->in_room );
            if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim,pobj))
                get_obj(victim,pobj,NULL);
        }


        if (victim->class_num == global_int_class_thief)
        {
            if (((rearm = get_eq_char(victim, WEAR_CONCEAL1)) == NULL)
             && ((rearm = get_eq_char(victim, WEAR_CONCEAL2)) == NULL))
                  return;

            act("$n slips $p from $s sleeve, rearming $mself!", victim, rearm, NULL, TO_ROOM);
            act("You slip $p from your sleeve, rearming yourself!", victim, rearm, NULL, TO_CHAR);
            unequip_char(victim, rearm);
            equip_char(victim, rearm, WORN_WIELD);
        }
   }

   return;
}

PROG_FUN( prog_faction )
{
    CHAR_DATA *victim = NULL;
    char arg[MAX_INPUT_LENGTH];
    unsigned int fnum;
    short fval;
    int i;

    if (argument[0] == '\0')
	return;

    argument = one_argument(argument, arg);

    switch (ptype)
    {
	case MOB_PROG:	victim = get_char_world(ch, arg);		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg);	break;
    }

    if (!victim || IS_NPC(victim))
	return;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || !is_number(arg))
	return;

    fnum = atoi(arg);

    if (fnum >= FactionTable::Instance().Count())
    	return;

    argument = one_argument(argument, arg);
    if (arg[0] == '\0' || !is_number(arg))
	    return;

    fval = atoi(arg);

    argument = one_argument(argument, arg);
    if (arg[0] == '\0' || !is_number(arg) || ((i = atoi(arg)) == 0))
        victim->pcdata->faction_standing->Change(*victim, fnum, fval, 0, 0, false, false);
    else
        victim->pcdata->faction_standing->Change(*victim, fnum, fval, 0, 0, true, false);
}

PROG_FUN( prog_doppel )
{
    CHAR_DATA *victim = NULL;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (ptype != MOB_PROG)
	return;

    if (argument[0] == '\0')
	return;

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, argument); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, argument);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, argument);	break;
    }

    if (!victim)
	return;

    setName(*ch, victim->name);

    free_string(ch->description);
    ch->description = str_dup(victim->description);

    free_string(ch->short_descr);
    if (victim->short_descr[0] != '\0')
	ch->short_descr = str_dup(victim->short_descr);
    else
	ch->short_descr = str_dup(victim->name);

    if (!IS_NPC(victim))
    {
	SET_BIT(ch->nact, ACT_CLASSED);
	ch->class_num = victim->class_num;
    }

    af.where	 = TO_OAFFECTS;
    af.type	 = gsn_doppelganger;
    af.level	 = ch->level;
    af.duration  = -1;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_DOPPEL;
    affect_to_char(ch, &af);

    ch->memfocus[DOPPEL_SLOT] = victim;
}

PROG_FUN( prog_takecoins )
{
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *victim = NULL;
    float tcoins, vcoins;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0' || !is_number(argument))
	return;

    switch (ptype)
    {
	case MOB_PROG:  victim = get_char_world(ch, arg); 		break;
	case OBJ_PROG:  victim = obj_get_char_world(obj, arg);  	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg);	break;
    }

    if (!victim)
	return;

    tcoins = (float) atof(argument);

    if (tcoins <= 0)
	return;

    vcoins = coins_to_value(victim->coins);

    if (tcoins > vcoins)
	tcoins = vcoins;

    dec_player_coins(victim, value_to_coins(tcoins, FALSE));
}
  

/** Special demon-related progs **/

PROG_FUN( prog_treefruit )
{
    AFFECT_DATA *paf;
    OBJ_DATA *fruit;
    int chance;
 
    if (ptype != MOB_PROG)
	return;

    if (!ch->in_room || !ch->in_room->area)
	return;

    if ((paf = affect_find(ch->in_room->area->affected, gsn_deathwalk)) == NULL)
	return;

    chance = paf->modifier / 2;

    if (number_percent() <= chance)
    {
	act("A sickly, twisted fruit grows from a branch of $n, and falls to the ground.", ch, NULL, NULL, TO_ROOM);
	fruit = create_object(get_obj_index(OBJ_VNUM_VOID_FRUIT), ch->level);
	fruit->objfocus[0] = ch->memfocus[0];
	obj_to_room(fruit, ch->in_room);
    }

    return;
}

PROG_FUN( prog_addtree )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (ptype != MOB_PROG)
	return;

    if (!ch->in_room || !ch->in_room->area)
	return;

    if (area_is_affected(ch->in_room->area, gsn_deathwalk))
	return;

    af.where	 = TO_AFFECTS;
    af.type	 = gsn_deathwalk;
    af.level	 = ch->level;
    af.duration  = 23;
    af.modifier	 = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.duration	 = 24;
    af.point	 = (void *) ch;
    affect_to_area(ch->in_room->area, &af);

    return;
}

PROG_FUN( prog_poisonroom )
{
    OBJ_DATA *fObj;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (ptype != MOB_PROG)
	return;

    if (!ch->in_room)
	return;

    af.where	 = TO_OBJECT;
    af.type	 = gsn_poison;
    af.level     = ch->level;
    af.duration  = 100;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;

    for (fObj = ch->in_room->contents; fObj; fObj = fObj->next_content)
    {
	if ((fObj->item_type == ITEM_FOUNTAIN) && (fObj->value[3] == 0)
         && !obj_is_affected(fObj, gsn_poison))
        {
	    act("The liquid in $p darkens momentarily, but fades back to normal.", ch, fObj, NULL, TO_ROOM);
	    affect_to_obj(fObj, &af);
	}

	if ((fObj->item_type == ITEM_FOOD) && (fObj->value[3] == 0)
	 && !obj_is_affected(fObj, gsn_poison))
	{
	    act("$p emits a brief noxious fume, but quickly returns to normal.", ch, fObj, NULL, TO_ROOM);
	    affect_to_obj(fObj, &af);
	}
    }

    return;
}

PROG_FUN( prog_demonbind )
{
    CHAR_DATA *master, *demon = NULL, *dch;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    bool gfound = FALSE, sfound = FALSE;
    int numfound = 0;

    if (ptype != MOB_PROG)
	return;

    if (ch->master)
	stop_follower(ch);

    if (argument[0] == '\0')
	return;

    if ((master = get_char_room(ch, argument)) == NULL)
	return;

    add_follower(ch, master);
    ch->leader = master;

    af.where	 = TO_AFFECTS;
    af.type      = gsn_demoniccontrol;
    af.level	 = master->level;
    af.duration  = (((ch->pIndexData->vnum >= MOB_VNUM_LESSER_BOUND_FIRST)
                  && (ch->pIndexData->vnum <= MOB_VNUM_LESSER_BOUND_LAST)) ? 48 :
		    ((ch->pIndexData->vnum >= MOB_VNUM_GREATER_BOUND_FIRST)
		  && (ch->pIndexData->vnum <= MOB_VNUM_GREATER_BOUND_LAST)) ? 12 : 24);
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(ch, &af);

    /* check for demon control */

    for (dch = char_list; dch; dch = dch->next)
    {
	if (IS_NPC(dch)
         && (dch->leader == master)
	 && (dch != ch)
         && is_affected(dch, gsn_demoniccontrol))
        {
	    numfound++;
	    if (!demon
             || (demon->level < dch->level))
		demon = dch;

	    if ((dch->pIndexData->vnum >= MOB_VNUM_GREATER_BOUND_FIRST)
	      && (dch->pIndexData->vnum <= MOB_VNUM_GREATER_BOUND_LAST))
	    {
		gfound = TRUE;
		demon = dch;
	    }
 	    if (dch->pIndexData->vnum == MOB_VNUM_GREATER_TASKMASTER)
	    {
	        numfound-=2;
		demon = dch;
	    }
	    
	    if (dch->pIndexData->vnum == ch->pIndexData->vnum)
	    {
		sfound = TRUE;
		demon = dch;
		break;
	    }
	}
    }

    if ((((ch->pIndexData->vnum >= MOB_VNUM_GREATER_BOUND_FIRST)
      && (ch->pIndexData->vnum <= MOB_VNUM_GREATER_BOUND_LAST))
      && gfound)
     || (numfound >= 3)
     || sfound)
    {
	affect_strip(demon, gsn_demoniccontrol);
	act("Your control spreads too thin, and $N rebels against your will!", master, NULL, demon, TO_CHAR);
	demon->tracking = master;
	multi_hit(demon, master, TYPE_UNDEFINED);
    }

    return;
}

PROG_FUN( prog_demonpcid )
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *pobj;
    int iWear;
    bool found;
    int idlevel;
    static char *he_she        [] = { "It",  "He",  "She" };

    if (ptype != MOB_PROG)
	return;

    if (!ch->pnote)
	return;

    argument = one_argument(argument, arg);

    if ((argument[0] == '\0') || (arg[0] == '\0') || !is_number(argument))
	return;

    if ((victim = get_char_world(ch, arg)) == NULL)
	return;

    if (IS_NPC(victim))
	return;

    idlevel = atoi(argument);

    buffer = new_buf();
    add_buf(buffer, ch->pnote->text);

    sprintf(buf, "%s is a %s, a %s of the %d%s rank.\n\r\n\r", PERS(victim, ch), race_table[victim->race].name,
		class_table[victim->class_num].name, victim->level,
		(((victim->level % 10) == 1) ? "st" :
		 ((victim->level % 10) == 2) ? "nd" :
		 ((victim->level % 10) == 3) ? "rd" : "th"));

    add_buf(buffer, buf);

    if (idlevel == 2)
    {
	sprintf(buf, "%s has died %d times, killed %d others, and been wanted %d times.\n\r",
		he_she[victim->sex], victim->pcdata->death_count, victim->pcdata->pkills,
		victim->pcdata->times_wanted);

	add_buf(buffer, buf);

	sprintf(buf, "%s is currently %s.\n\r\n\r", he_she[victim->sex],
		((victim->in_room->sector_type == SECT_INSIDE) ? "inside" :
		 (victim->in_room->sector_type == SECT_CITY) ? "in a city" :
		 (victim->in_room->sector_type == SECT_FIELD) ? "in a field" :
		 (victim->in_room->sector_type == SECT_FOREST) ? "in a forest" :
		 (victim->in_room->sector_type == SECT_HILLS) ? "in the hills" :
		 (victim->in_room->sector_type == SECT_MOUNTAIN) ? "in the mountains" :
		 ((victim->in_room->sector_type == SECT_WATER_SWIM) ||
		  (victim->in_room->sector_type == SECT_WATER_NOSWIM)) ? "on the water" :
		 (victim->in_room->sector_type == SECT_AIR) ? "in the air" :
		 (victim->in_room->sector_type == SECT_DESERT) ? "in the desert" :
		 (victim->in_room->sector_type == SECT_UNDERWATER) ? "underwater" :
		 (victim->in_room->sector_type == SECT_UNDERGROUND) ? "underground" :
		 (victim->in_room->sector_type == SECT_ROAD) ? "on a road" :
		 (victim->in_room->sector_type == SECT_SWAMP) ? "in a swamp" : "in a place unknown"));

	add_buf(buffer, buf);
    }
    else if (idlevel >= 3)
    {
	sprintf(buf, "%s has died %d times, and been wanted %d times.\n\r",
		he_she[victim->sex], victim->pcdata->death_count,
		victim->pcdata->times_wanted);

	add_buf(buffer, buf);

	sprintf(buf, "%s has killed %d adventurers -- %d good, %d neutral, and %d evil.\n\r",
		he_she[victim->sex], victim->pcdata->pkills, victim->pcdata->align_kills[ALIGN_GOOD],
		victim->pcdata->align_kills[ALIGN_NEUTRAL], victim->pcdata->align_kills[ALIGN_EVIL]);

	add_buf(buffer, buf);

	if (idlevel == 3)
	{
	    sprintf(buf, "%s is currently %s.\n\r\n\r", he_she[victim->sex],
		((victim->in_room->sector_type == SECT_INSIDE) ? "inside" :
		 (victim->in_room->sector_type == SECT_CITY) ? "in a city" :
		 (victim->in_room->sector_type == SECT_FIELD) ? "in a field" :
		 (victim->in_room->sector_type == SECT_FOREST) ? "in a forest" :
		 (victim->in_room->sector_type == SECT_HILLS) ? "in the hills" :
		 (victim->in_room->sector_type == SECT_MOUNTAIN) ? "in the mountains" :
		 ((victim->in_room->sector_type == SECT_WATER_SWIM) ||
		  (victim->in_room->sector_type == SECT_WATER_NOSWIM)) ? "on the water" :
		 (victim->in_room->sector_type == SECT_AIR) ? "in the air" :
		 (victim->in_room->sector_type == SECT_DESERT) ? "in the desert" :
		 (victim->in_room->sector_type == SECT_UNDERWATER) ? "underwater" :
		 (victim->in_room->sector_type == SECT_UNDERGROUND) ? "underground" :
		 (victim->in_room->sector_type == SECT_ROAD) ? "on a road" :
		 (victim->in_room->sector_type == SECT_SWAMP) ? "in a swamp" : "in a place unknown"));

	    add_buf(buffer, buf);
	}
	else if (idlevel >= 4)
	{
	    sprintf(buf, "%s is currently %s, in %s.\n\r\n\r", he_she[victim->sex],
		((victim->in_room->sector_type == SECT_INSIDE) ? "inside" :
		 (victim->in_room->sector_type == SECT_CITY) ? "in a city" :
		 (victim->in_room->sector_type == SECT_FIELD) ? "in a field" :
		 (victim->in_room->sector_type == SECT_FOREST) ? "in a forest" :
		 (victim->in_room->sector_type == SECT_HILLS) ? "in the hills" :
		 (victim->in_room->sector_type == SECT_MOUNTAIN) ? "in the mountains" :
		 ((victim->in_room->sector_type == SECT_WATER_SWIM) ||
		  (victim->in_room->sector_type == SECT_WATER_NOSWIM)) ? "on the water" :
		 (victim->in_room->sector_type == SECT_AIR) ? "in the air" :
		 (victim->in_room->sector_type == SECT_DESERT) ? "in the desert" :
		 (victim->in_room->sector_type == SECT_UNDERWATER) ? "underwater" :
		 (victim->in_room->sector_type == SECT_UNDERGROUND) ? "underground" :
		 (victim->in_room->sector_type == SECT_ROAD) ? "on a road" :
		 (victim->in_room->sector_type == SECT_SWAMP) ? "in a swamp" : "in a place unknown"),
		victim->in_room->area ? victim->in_room->area->name : "an unknown area");

	    add_buf(buffer, buf);
	}
    }


    found = FALSE;
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
    {
	if ( ( pobj = get_eq_char( victim, iWear ) ) == NULL )
	    continue;

	if (IS_SET(pobj->extra_flags[0], ITEM_WIZI))
	    continue;

	add_buf(buffer, where_name[iWear]);
	add_buf(buffer, format_obj_to_char( pobj, ch, TRUE ));
	add_buf(buffer, "\n\r");
	found = TRUE;
    }

    if ( !found )
	add_buf(buffer, "Nothing.\n\r");

    free_string( ch->pnote->text );
    ch->pnote->text = str_dup( buf_string(buffer) );
    free_buf(buffer);

    return;
}
		

PROG_FUN( prog_demonid )
{
    OBJ_DATA *pobj = ch->carrying;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;
    int i, idlevel = 1;
    BUFFER *buffer;

    if (ptype != MOB_PROG)
	return;

    if (!pobj)
	return;

    if (!ch->pnote)
	return;

    if ((argument[0] != '\0') && is_number(argument))
	idlevel = atoi(argument);

    buffer = new_buf();
    add_buf(buffer, ch->pnote->text);

    sprintf( buf,
        "Object: '%s' is type %s.\n\rExtra flags %s",
        pobj->short_descr,
        item_name(pobj->item_type),
        extra_bit_name( pobj->extra_flags[0], 0 )
        );

    add_buf(buffer, buf);

    if (str_cmp(extra_bit_name(pobj->extra_flags[1], 1), "none"))
    {
	sprintf(buf, " %s", extra_bit_name(pobj->extra_flags[1], 1));
	add_buf(buffer, buf);
    }

	if (str_cmp(extra_bit_name(pobj->extra_flags[2], 2), "none"))
	{
		sprintf(buf, " %s", extra_bit_name(pobj->extra_flags[2], 2));
		add_buf(buffer, buf);
	}

    sprintf(buf, ".\n\rWeight is %d.%d, level is %d.\n\r",
        pobj->weight / 10,
	pobj->weight % 10,
        pobj->level );
	
    add_buf(buffer, buf);

    sprintf( buf,"Material is %s.\n\r", material_table[pobj->material].name);
    add_buf(buffer, buf);

    switch ( pobj->item_type )
    {
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_OIL:
        sprintf( buf, "Level %d spells of:", pobj->value[0] );
	add_buf(buffer, buf);

        if ( pobj->value[1] >= 0 && pobj->value[1] < MAX_SKILL )
        {
            add_buf( buffer, " '" );
            add_buf( buffer, skill_table[pobj->value[1]].name );
            add_buf( buffer, "'" );
        }

        if ( pobj->value[2] >= 0 && pobj->value[2] < MAX_SKILL )
        {
            add_buf( buffer, " '");
            add_buf( buffer, skill_table[pobj->value[2]].name);
            add_buf( buffer, "'");
        }

        if ( pobj->value[3] >= 0 && pobj->value[3] < MAX_SKILL )
        {
            add_buf( buffer, " '");
            add_buf( buffer, skill_table[pobj->value[3]].name);
            add_buf( buffer, "'");
        }

        if (pobj->value[4] >= 0 && pobj->value[4] < MAX_SKILL)
        {
            add_buf( buffer, " '");
            add_buf( buffer, skill_table[pobj->value[4]].name);
            add_buf( buffer, "'");
        }

        add_buf( buffer, ".\n\r");
        break;

    case ITEM_WAND:
    case ITEM_STAFF:
        sprintf( buf, "Has %d charges of level %d",
            pobj->value[2], pobj->value[0] );
        add_buf( buffer, buf);

        if ( pobj->value[3] >= 0 && pobj->value[3] < MAX_SKILL )
        {
            add_buf( buffer, " '");
            add_buf( buffer, skill_table[pobj->value[3]].name);
            add_buf( buffer, "'");
        }

        add_buf( buffer, ".\n\r" );
        break;

    case ITEM_ARROW:
        buf[0] = '\0';
        for (i = 2; i < 5; i++)
        {
            if (pobj->value[i] > 0)
            {
                if (buf[0] != '\0')
                    sprintf(buf, "%s, %s", buf, skill_table[pobj->value[i]].name);
                else
                    sprintf(buf, "Imbued with %s", skill_table[pobj->value[i]].name);
            }
        }

        if (buf[0] != '\0')
        {
            sprintf(buf, "%s.\n\r", buf);
            add_buf( buffer, buf );
        }
        break;

    case ITEM_DRINK_CON:
        sprintf(buf,"It holds %s-colored %s.\n\r",
            liq_table[pobj->value[2]].liq_color,
            liq_table[pobj->value[2]].liq_name);
        add_buf( buffer, buf);
        break;

    case ITEM_CONTAINER:
        sprintf(buf,"Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
            pobj->value[0], pobj->value[3], cont_bit_name(pobj->value[1]));
        add_buf( buffer, buf);
        if (pobj->value[4] != 100)
        {
            sprintf(buf,"Weight multiplier: %d%%\n\r",
                pobj->value[4]);
            add_buf( buffer, buf);
        }
        break;

    case ITEM_WEAPON:
        add_buf(buffer, "Weapon type is ");
        switch (pobj->value[0])
        {
            case(WEAPON_KNIFE)  : add_buf( buffer, "knife.\n\r");        break;
            case(WEAPON_STAFF)  : add_buf( buffer, "staff.\n\r");        break;
            case(WEAPON_EXOTIC) : add_buf( buffer, "exotic.\n\r");       break;
            case(WEAPON_SWORD)  : add_buf( buffer, "sword.\n\r");        break;
            case(WEAPON_DAGGER) : add_buf( buffer, "dagger.\n\r");       break;
            case(WEAPON_SPEAR)  : add_buf( buffer, "spear.\n\r");	break;
            case(WEAPON_MACE)   : add_buf( buffer, "mace/club.\n\r");    break;
            case(WEAPON_AXE)    : add_buf( buffer, "axe.\n\r");          break;
            case(WEAPON_FLAIL)  : add_buf( buffer, "flail.\n\r");        break;
            case(WEAPON_WHIP)   : add_buf( buffer, "whip.\n\r");         break;
            case(WEAPON_POLEARM): add_buf( buffer, "polearm.\n\r");      break;
            default             : add_buf( buffer, "unknown.\n\r");      break;
        }
        if (pobj->pIndexData->new_format)
            sprintf(buf,"Damage is %dd%d (average %d).\n\r",
                pobj->value[1],pobj->value[2],
                (1 + pobj->value[2]) * pobj->value[1] / 2);
        else
            sprintf( buf, "Damage is %d to %d (average %d).\n\r",
                pobj->value[1], pobj->value[2],
                ( pobj->value[1] + pobj->value[2] ) / 2 );
        add_buf( buffer, buf);
        if (pobj->value[4])  /* weapon flags */
        {
            sprintf(buf,"Weapons flags: %s\n\r",weapon_bit_name(pobj->value[4]));            send_to_char(buf,ch);
        }
        break;

    case ITEM_BOW:
        if (pobj->pIndexData->new_format)
            sprintf(buf,"Damage is %dd%d (average %d).\n\r",
                pobj->value[1],pobj->value[2],
                (1 + pobj->value[2]) * pobj->value[1] / 2);
        else
            sprintf( buf, "Damage is %d to %d (average %d).\n\r",
                pobj->value[1], pobj->value[2],
                ( pobj->value[1] + pobj->value[2] ) / 2 );
        add_buf( buffer, buf);
        break;

    case ITEM_ARMOR:
        sprintf( buf,
        "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\n\r",
            pobj->value[0], pobj->value[1], pobj->value[2], pobj->value[3] );
        add_buf( buffer, buf );
        break;
    }

    if (!pobj->enchanted)
    for ( paf = pobj->pIndexData->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->location != APPLY_NONE && paf->modifier != 0 )
        {
            sprintf( buf, "Affects %s by %d.\n\r",
                affect_loc_name( paf->location ), paf->modifier );
            add_buf( buffer, buf);
            if (paf->bitvector)
            {
                switch(paf->where)
                {
                    case TO_AFFECTS:
                        sprintf(buf,"Adds %s affect.\n",
                            affect_bit_name(paf));
                        break;
                    case TO_OBJECT:
                        sprintf(buf,"Adds %s pobject flag.\n",
                            extra_bit_name(paf->bitvector, 0));
                        break;
		    case TO_OBJECT1: sprintf(buf,"Adds %s pobject flag.\n", extra_bit_name(paf->bitvector, 1)); break;
		    case TO_OBJECT2: sprintf(buf,"Adds %s pobject flag.\n", extra_bit_name(paf->bitvector, 2)); break;
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
                add_buf( buffer, buf );
            }
        }
    }

    for ( paf = pobj->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->location != APPLY_NONE && paf->modifier != 0 )
        {
            sprintf( buf, "Affects %s by %d",
                affect_loc_name( paf->location ), paf->modifier );
            add_buf( buffer, buf );
            if ( paf->duration > -1)
                sprintf(buf,", %d%s hours.\n\r",
		    paf->duration,
		    ((paf->duration % 2) == 0) ? "" : ".5");
            else
                sprintf(buf,".\n\r");
            add_buf( buffer, buf);
            if (paf->bitvector)
            {
                switch(paf->where)
                {
                    case TO_AFFECTS:
                        sprintf(buf,"Adds %s affect.\n",
                            affect_bit_name(paf));
                        break;
                    case TO_OBJECT:
                        sprintf(buf,"Adds %s pobject flag.\n",
                            extra_bit_name(paf->bitvector, 0));
                        break;
		    case TO_OBJECT1:  sprintf(buf,"Adds %s pobject flag.\n", extra_bit_name(paf->bitvector, 1)); break;
		    case TO_OBJECT2:  sprintf(buf,"Adds %s pobject flag.\n", extra_bit_name(paf->bitvector, 2)); break;
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
                add_buf( buffer, buf);
            }
        }
    }

    if (idlevel >= 2)
    {
	if (pobj->lastowner[0][0] != '\0')
        {

	    sprintf(buf, "\n\rThe last owners of this item were: ");
	    for (i = 0; i < MAX_LASTOWNER; i++)
	        if (pobj->lastowner[i][0] != '\0')
	            sprintf(buf, "%s, %s", buf, pobj->lastowner[i]);

	    sprintf(buf, "%s.\n\r", buf);
	    add_buf(buffer, buf);
	}
	
	if (pobj->pIndexData->lore)
	{
	    add_buf(buffer, "\n\r");
	    add_buf(buffer, pobj->pIndexData->lore);
	}

	if (idlevel >= 3)
	{
	    sprintf(buf, "\n\rCurrent %d of these items are held by residents of Avendar.\n\r",
		pobj->pIndexData->current);
	    add_buf(buffer, buf);
	}
    }

    free_string( ch->pnote->text );
    ch->pnote->text = str_dup( buf_string(buffer) );
    free_buf(buffer);

    return;
}

PROG_FUN( prog_removehand )
{
    CHAR_DATA *victim;

    if (ptype != MOB_PROG)
	return;

    if ((victim = get_char_room(ch, argument)) == NULL)
	return;

    SET_BIT(victim->oaffected_by, AFF_ONEHANDED);

    if (hands_free(victim) < 0)
	remove_obj_slot(victim, WEAR_LIGHT, TRUE);
    if (hands_free(victim) < 0)
	remove_obj_slot(victim, WEAR_HOLD, TRUE);
    if (hands_free(victim) < 0)
	remove_obj_slot(victim, WEAR_SHIELD, TRUE);
    if (hands_free(victim) < 0)
	remove_obj_slot(victim, WEAR_DUAL_WIELD, TRUE);
    if (hands_free(victim) < 0)
	remove_obj_slot(victim, WEAR_WIELD, TRUE);

    return;
}

PROG_FUN( prog_addsymbol )
{
    CHAR_DATA *victim = NULL;
    char arg[MAX_STRING_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    switch(ptype)
    {
	case MOB_PROG:	victim = get_char_world(ch, arg);	break;
	case OBJ_PROG:	victim = obj_get_char_world(obj, arg);	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg); break;
    }

    if (!victim)
	return;

    one_argument(argument, arg);

    if (!is_number(arg))
	return;

    SET_BIT(victim->symbols_known, inscribe_table[atoi(arg)].bit);

    return;
}

PROG_FUN( prog_removesymbol )
{
    CHAR_DATA *victim = NULL;
    char arg[MAX_STRING_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    switch(ptype)
    {
	case MOB_PROG:	victim = get_char_world(ch, arg);	break;
	case OBJ_PROG:	victim = obj_get_char_world(obj, arg);	break;
	case ROOM_PROG: victim = room_get_char_world(room, arg); break;
    }

    if (!victim)
	return;

    one_argument(argument, arg);

    if (!is_number(arg))
	return;

    REMOVE_BIT(victim->symbols_known, inscribe_table[atoi(arg)].bit);

    return;
}


PROG_FUN( prog_cursebarkja )
{
    char arg[MAX_STRING_LENGTH];
    OBJ_DATA *object;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    
    one_argument(argument, arg);

    object = get_obj_carry(ch, arg, ch); 

    if (IS_OBJ_STAT(object,ITEM_BLESS))
    {
        AFFECT_DATA *paf;

	paf = affect_find(object->affected,skill_lookup("bless"));
	if (paf != NULL)
	    affect_remove_obj(object,paf);
	REMOVE_BIT(object->extra_flags[0],ITEM_BLESS);
    }

    af.where	= TO_OBJECT;
    af.type	= 0;
    af.level	= ch->level;
    af.duration = -1;
    af.location = APPLY_SAVES;
    af.modifier = 1;
    af.bitvector= ITEM_EVIL;
    affect_to_obj(object, &af);
   
    af.where    = TO_OBJECT;
    af.type     = 0;
    af.level    = ch->level;
    af.duration = -1;
    af.location = 0;
    af.modifier = 0;
    af.bitvector= ITEM_NOREMOVE;
    affect_to_obj(object, &af);

    af.where	= TO_OBJECT;
    af.type	= gsn_cursebarkja;
    af.level	= ch->level;
    af.duration = 3;
    af.location	= 0;
    af.modifier = ch->memfocus[0]->id;
    af.bitvector= 0;
    affect_to_obj(object, &af);

    return;
}

PROG_FUN( prog_strreplace)
{
    PROG_VARDATA *d_var;
    PROG_RUNDATA *p_data = NULL;
    char orig[MAX_INPUT_LENGTH],search[MAX_INPUT_LENGTH],
	replace[MAX_INPUT_LENGTH], dest[MAX_INPUT_LENGTH];
    char newstr[MAX_STRING_LENGTH];

    switch(ptype)
    {
	case MOB_PROG:	p_data = ch->prog_data;	    break;
	case OBJ_PROG:  p_data = obj->prog_data;    break;
	case ROOM_PROG:	p_data = room->prog_data;   break;
    }

    if (!p_data)
	return;
// First, set up the destination variable.
    argument = one_argument(argument, dest);
    d_var = mprog_find_pvar(dest, p_data, TRUE);

    argument = one_argument(argument, orig, true);
    mprog_do_trans(orig, p_data, orig);
    
    argument = one_argument(argument, search, true);
    mprog_do_trans(orig, p_data, orig);
    
    argument = one_argument(argument, replace, true);
    mprog_do_trans(orig, p_data, orig);

    if (dest[0] == '\0' || search[0] == '\0' || replace[0] == '\0')
        return;
    if (!d_var)
	return;

    sprintf(newstr,replace_str(orig, search, replace));

    free_string(d_var->value);
    d_var->value = str_dup(newstr);
}   

PROG_FUN (prog_rumor)
{
    RUMORS *nr = rumor_list;
    int i=0,roll=0;
    int attempts=0;
    CHAR_DATA *victim=NULL;

    switch (ptype)
    {
	case MOB_PROG: victim=ch; break;
	case OBJ_PROG: victim=obj->carried_by; break;
	case ROOM_PROG: return;
    }

    if (victim == NULL || victim->position <= POS_RESTING)
	return;
    while (attempts < 20)
    {
	roll = dice(1,g_num_rumors)-1;
	while(i < roll && nr->next)
	{
	    nr = nr->next;
	    i++;
	}
	if (nr && IS_VALID(nr) && GOODRUMOR(nr))
	{
	    do_say(victim,nr->text);
	    return;
	}
	else
	    attempts++;
    }	
};

PROG_FUN (prog_addrumor)
{
    char arg[MAX_STRING_LENGTH];
    RUMORS *nr = NULL;

    if (argument[0] == '\0')
	return;
 
    nr = new_rumor();
    argument = one_argument(argument,arg);
    if (argument[0]=='\0')
	return;

    nr->name = str_dup(arg);
    nr->text = str_dup(argument);
    nr->stamp = current_time;
    fwrite_rumors();
};

