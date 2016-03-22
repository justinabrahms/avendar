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
 *  such installation can be found in INSTALL.  Enjoy...         N'Atas-Ha *
 ***************************************************************************/

#include <sys/types.h>

#if defined(WIN32)
#include <cregex.h>
#else
#include <regex.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <cassert>
#include <sstream>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "languages.h"
#include "RoomPath.h"
#include "Luck.h"
#include "Faction.h"
#include "mob_prog.h"
#include "ProgConditionals.h"

bool	verb_stop_issued;

extern	int	hitprog_damage;
extern unsigned short      port;

CHAR_DATA *	mcur_running;

/*
 * Local function prototypes
 */

char *	mprog_next_command	args( ( char* clist, LOOP_DATA *curloop ) );
bool	mprog_seval		args( ( char* lhs, char* opr, char* rhs ) );
bool	mprog_veval		args( ( int lhs, char* opr, int rhs ) );
int 	mprog_do_ifchck		args( ( PROG_RUNDATA *prog ) );
char *	mprog_process_if	args( ( char *com_list, PROG_RUNDATA *prog, bool *interp ) );
int	mprog_translate		args( ( char *str, PROG_RUNDATA *prog, char* t ) );
void	mprog_do_trans		args( ( const char *ch, PROG_RUNDATA *prog, char* t ) );
void	mprog_process_cmnd	args( ( PROG_RUNDATA *prog ) );

LOOP_DATA *mprog_process_loop	args( ( PROG_RUNDATA *prog, char *next_com ) );
LOOP_DATA *mprog_process_endloop args(( char *com_list, PROG_RUNDATA *prog ));

bool	mprog_driver		args( ( char* com_list, CHAR_DATA* mob,
				       CHAR_DATA* actor, OBJ_DATA* obj,
					ROOM_INDEX_DATA *room,
				       void* vo, char* txt ) );

/***************************************************************************
 * Local function code and brief comments.
 */

/* if you dont have these functions, you damn well should... */

#ifdef DUNNO_STRSTR
char * strstr     args( (const char *s1, const char *s2 ) );
#endif


#ifdef DUNNO_STRSTR
char * strstr(s1,s2) const char *s1; const char *s2;
{
  char *cp;
  int i,j=strlen(s1)-strlen(s2),k=strlen(s2);
  if(j<0)
    return NULL;
  for(i=0; i<=j && strncmp(s1++,s2, k)!=0; i++);
  return (i>j) ? NULL : (s1-1);
}
#endif

/* this is being added to communicate data back about whether anything
 * actually triggered a prog. Needed for smooth verb prog functionality.
 */
void   mprog_wordlist_check   args ( ( char * arg, CHAR_DATA *mob,
				CHAR_DATA* actor, OBJ_DATA* object,
				void* vo, int type ) );

void oprog_wordlist_check args(( char *arg, OBJ_DATA *mob, CHAR_DATA *actor,
			  OBJ_DATA *obj, void *vo, int type ));

void mprog_percent_check      args ( ( CHAR_DATA *mob, CHAR_DATA* actor, OBJ_DATA* object, void* vo, int type ) );
void oprog_percent_check      args ( ( OBJ_DATA *mob, CHAR_DATA* actor,
				OBJ_DATA* object, void* vo,
				int type ) );
void    mprog_demon_trigger	args( ( CHAR_DATA* demon ) );
void    mprog_act_trigger       args ( ( char* buf, CHAR_DATA* mob,
                                CHAR_DATA* ch, OBJ_DATA* obj,void* vo ));
void    oprog_act_trigger       args ( ( char* buf, OBJ_DATA* mob,
                                CHAR_DATA* ch, OBJ_DATA* obj,void* vo ));
void    mprog_bribe_trigger     args ( ( CHAR_DATA* mob, CHAR_DATA* ch,
                                int amount ) );
void	mprog_load_trigger	args ( ( CHAR_DATA *mob));
void	oprog_load_trigger	args ( ( OBJ_DATA *obj));
void	mprog_time_trigger	args ( ( CHAR_DATA *mob));
void	oprog_time_trigger	args ( ( OBJ_DATA *obj));
void    mprog_entry_trigger     args ( ( CHAR_DATA* mob ));
void    oprog_entry_trigger     args ( ( OBJ_DATA* mob ));
bool    mprog_give_trigger      args ( ( CHAR_DATA* mob, CHAR_DATA* ch,
                                OBJ_DATA* obj ) );
void    oprog_give_trigger      args ( ( OBJ_DATA* mob, CHAR_DATA* ch,
                                OBJ_DATA* obj ) );
void    mprog_greet_trigger     args ( ( CHAR_DATA* mob) );
void    oprog_greet_trigger     args ( ( OBJ_DATA* mob, CHAR_DATA * target) );
void    mprog_fight_trigger     args ( ( CHAR_DATA* mob, CHAR_DATA* ch) );
void    oprog_fight_trigger     args ( ( OBJ_DATA* mob, CHAR_DATA* ch) );
void    mprog_hitprcnt_trigger  args ( ( CHAR_DATA* mob, CHAR_DATA* ch) );
void    oprog_hitprcnt_trigger  args ( ( OBJ_DATA* mob, CHAR_DATA* ch) );
void    mprog_death_trigger     args ( ( CHAR_DATA* mob, CHAR_DATA *killer ) );
void    mprog_random_trigger    args ( ( CHAR_DATA* mob ) );
void    oprog_random_trigger    args ( ( OBJ_DATA* mob ) );
void    mprog_speech_trigger    args ( ( LANG_STRING* txt, CHAR_DATA* mob ) );	
void    oprog_speech_trigger    args ( ( char* txt, OBJ_DATA* mob ) );	
void    mprog_exit_trigger      args ( ( CHAR_DATA* mob ));
void    oprog_exit_trigger      args ( ( OBJ_DATA* mob ));
void	oprog_eat_trigger	args ( ( CHAR_DATA* mob, OBJ_DATA* obj ) );
void	oprog_drink_trigger	args ( ( CHAR_DATA* mob, OBJ_DATA* obj ) );

/* Used to get sequential lines of a multi line string (separated by "\n\r")
 * Thus its like one_argument(), but a trifle different. It is destructive
 * to the multi line string argument, and thus clist must not be shared.
 */
char *mprog_next_command( char *clist, LOOP_DATA *curloop )
{
  
  char *pointer = clist;

  if (clist == NULL)
	return NULL;

  while ( *pointer != '\n' && *pointer != '\0' )
    pointer++;
  if ( *pointer == '\n' )
    *pointer++ = '\0';
  if ( *pointer == '\r' )
    *pointer++ = '\0';

  if (curloop && (curloop->value >= curloop->lower))
      while ((*pointer == '\0') && (pointer < curloop->endprog))
	pointer++;

  return ( pointer );

}

/* These two functions do the basic evaluation of ifcheck operators.
 *  It is important to note that the string operations are not what
 *  you probably expect.  Equality is exact and division is substring.
 *  remember that lhs has been stripped of leading space, but can
 *  still have trailing spaces so be careful when editing since:
 *  "guard" and "guard " are not equal.
 */
bool mprog_seval( char *lhs, char *opr, char *rhs )
{
  char buf[MAX_STRING_LENGTH];

  if ( !str_cmp( opr, "==" ) )
    return ( bool )( !str_cmp( lhs, rhs ) );
  if ( !str_cmp( opr, "!=" ) )
    return ( bool )( str_cmp( lhs, rhs ) );
  if ( !str_cmp( opr, "/" ) )
    return ( bool )( !str_infix( rhs, lhs ) );
  if ( !str_cmp( opr, "!/" ) )
    return ( bool )( str_infix( rhs, lhs ) );

  sprintf(buf, "Improper MOBprog operator, operators was %s, lhs and rhs were %s and %s", opr, lhs, rhs);
  bug ( buf, 0 );
  return 0;

}

bool mprog_veval( int lhs, char *opr, int rhs )
{
  char buf[MAX_STRING_LENGTH];

  if ( !str_cmp( opr, "==" ) )
    return ( lhs == rhs );
  if ( !str_cmp( opr, "!=" ) )
    return ( lhs != rhs );
  if ( !str_cmp( opr, ">" ) )
    return ( lhs > rhs );
  if ( !str_cmp( opr, "<" ) )
    return ( lhs < rhs );
  if ( !str_cmp( opr, "<=" ) )
    return ( lhs <= rhs );
  if ( !str_cmp( opr, ">=" ) )
    return ( lhs >= rhs );
  if ( !str_cmp( opr, "&" ) )
    return ( lhs & rhs );
  if ( !str_cmp( opr, "|" ) )
    return ( lhs | rhs );

  sprintf(buf, "Improper MOBprog operator, operator was %s, lhs and rhs were %d and %d", opr, lhs, rhs);
  bug ( buf, 0 );
  return 0;
}

static CHAR_DATA * lookupRandom(PROG_RUNDATA & prog)
{
    if (prog.rndm == NULL)
        return NULL;

    if (prog.rndm->id == prog.rndmID)
        return prog.rndm;

    prog.rndm = NULL;
    prog.rndmID = 0;
    return NULL;
}

DeduceResult deduce_char_arg(CHAR_DATA *& result, PROG_RUNDATA * prog, char * arg, CHAR_DATA * vict, bool checkVals)
{
    if (arg[0] == '$')
    {
        switch(arg[1])
        {
            case 'i': result = prog->mob; return (result == NULL ? Deduce_Null : Deduce_Proceed);
            case 'n': result = prog->actor; return (result == NULL ? Deduce_Null : Deduce_Proceed);
            case 'r': result = lookupRandom(*prog); return (result == NULL ? Deduce_Null : Deduce_Proceed);
	        case 't': result = vict; return (result == NULL ? Deduce_Null : Deduce_Proceed);

            case 'o': 
                if (prog->obj != NULL && prog->obj->carried_by != NULL)
                    result = prog->obj->carried_by;

                return Deduce_Proceed;

      	    case 'f': 
            {
                // Determine the focus slot
                int fslot(0);
                if (isdigit(arg[2]))
                {
                    fslot = arg[2] - '0';
                    if (fslot < 0)
                        return Deduce_Failure;
                }

                // Pull the target from the appropriate focus slot 
                if (prog->mob != NULL) 
                {
                    if (fslot >= MAX_MOBFOCUS) return Deduce_Failure;
                    result = prog->mob->memfocus[fslot];
                }
                else if (prog->obj != NULL)
                {
                    if (fslot >= MAX_OBJFOCUS) return Deduce_Failure;
                    result = prog->obj->objfocus[fslot];
                }
                else if (prog->room != NULL) 
                {
                    if (fslot >= MAX_ROOMFOCUS) return Deduce_Failure;
                    result = prog->room->roomfocus[fslot];
                }
                return Deduce_Proceed;
            }

            case 'd':
            {
                // Get the datastring pointer
                char * datastr(NULL);
                if (prog->mob != NULL) datastr = prog->mob->prog_state.data_marker;
                else if (prog->obj != NULL) datastr = prog->obj->prog_state.data_marker;
                else if (prog->room != NULL) datastr = prog->room->prog_state.data_marker;

                // Sanity-check
                if (datastr == NULL)
                {
                    bug("NULL data reader in $d deduction", 0);
                    return Deduce_Failure;
                }

                // Iterate the data string, copying it into the arg
                while (*datastr != '\0' && *datastr != '\n')
                {
                    switch (*datastr)
                    {
                        // Ignore \r
                        case '\r': ++datastr; break;

                        // Parse vars
                        case '$': case '%': 
                        {
                             int count(mprog_translate(datastr, prog, arg));
                             datastr += count + 1;
                             while (*arg != '\0') ++arg;
                             break;
                        }

                        // Just copy the character
                        default:
                            *arg = *datastr;
                            ++datastr;
                            ++arg;
                            break;
                    }
                }
                *arg = '\0';
                return Deduce_Proceed;
            }

            default:
                if (arg[1] == 'v' || (arg[1] >= '0' && arg[1] <= '9'))
                {
                    mprog_do_trans(arg, prog, arg);
                    return Deduce_Proceed;
                }

                if (!checkVals)
                    return Deduce_Proceed;

                // Note the error here
                std::ostringstream mess;
                if (prog->mob != NULL) mess << "Mob: " << prog->mob->pIndexData->vnum << ", ";
                else if (prog->obj != NULL) mess << "Obj: " << prog->obj->pIndexData->vnum << ", ";
                else if (prog->room != NULL) mess << "Room: " << prog->room->vnum << ", ";
                mess << " bad argument '" << arg << "'.";
                bug(mess.str().c_str(), 0);
                return Deduce_Failure;
        }
    }

    if (arg[0] == '%')
        mprog_do_trans(arg, prog, arg);

    return Deduce_Proceed;
}

/* This function performs the evaluation of the if checks.  It is
 * here that you can add any ifchecks which you so desire. Hopefully
 * it is clear from what follows how one would go about adding your
 * own. The syntax for an if check is: ifchck ( arg ) [opr val]
 * where the parenthesis are required and the opr and val fields are
 * optional but if one is there then both must be. The spaces are all
 * optional. The evaluation of the opr expressions is farmed out
 * to reduce the redundancy of the mammoth if statement list.
 * If there are errors, then return -1 otherwise return boolean 1,0
 */
int mprog_do_ifchck( PROG_RUNDATA *prog )
{
    char buf[ MAX_INPUT_LENGTH ];
    char arg[ MAX_INPUT_LENGTH ];
    char opr[ MAX_INPUT_LENGTH ];
    char val[ MAX_INPUT_LENGTH ];
    char tmp[3];
    CHAR_DATA *rhstarg = NULL, *target = NULL;
    CHAR_DATA *vict = (CHAR_DATA *) prog->vo;
    char     *txt   = (char *) prog->vo;
    char     *bufpt = buf;
    char     *argpt = arg;
    char     *oprpt = opr;
    char     *valpt = val;
    char     *point = prog->cmnd;
    /* added for compstr */
    char     *proctxt;
    char      checktxt[MAX_STRING_LENGTH];

    proctxt = prog->txt;

    point = one_argument(point, tmp);

    if ( *point == '\0' ) 
    {
	if ( prog->mob )
            bug("Mob: %d null ifchck", prog->mob->pIndexData->vnum); 
	else if ( prog->obj )
	    bug("Obj: %d null ifchck", prog->obj->pIndexData->vnum);
        return -1;
    }

    /* skip leading spaces */
    while ( *point == ' ' )
        point++;

    /* get whatever comes before the left paren.. ignore spaces */
    while ( *point != '(' ) 
        if ( *point == '\0' ) 
	
        {
	    if ( prog->mob )
		bug("Mob: %d ifchck syntax error", prog->mob->pIndexData->vnum); 
	    else if ( prog->obj )
		bug("Obj: %d ifchck syntax error", prog->obj->pIndexData->vnum);
	    return -1;
        }   
        else
      	    if ( *point == ' ' )
		point++;
            else 
		*bufpt++ = *point++; 

    *bufpt = '\0';
    point++;

    /* get whatever is in between the parens.. ignore spaces */
    while ( *point != ')' ) 
    	if ( *point == '\0' ) 
        {
	    if ( prog->mob )
		bug("Mob: %d ifchck syntax error", prog->mob->pIndexData->vnum); 
	    else if ( prog->obj )
		bug("Obj: %d ifchck syntax error", prog->obj->pIndexData->vnum);
	    return -1;
        }   
        else
            if ( *point == ' ' )
		point++;
            else 
		*argpt++ = *point++; 

    *argpt = '\0';
    point++;

    /* check to see if there is an operator */
    while ( *point == ' ' )
        point++;

    if ( *point == '\0' ) 
    {
        *opr = '\0';
        *val = '\0';
    }   
    else /* there should be an operator and value, so get them */
    {
        while ( ( *point != ' ' ) && ( !isalnum( *point ) ) ) 
	    if ( *point == '\0' ) 
	    {
		if ( prog->mob )
	            bug("prog->mob: %d ifchck operator without value", prog->mob->pIndexData->vnum ); 
		else if ( prog->obj )
		    bug("Obj: %d ifchck operator without value", prog->obj->pIndexData->vnum );	
	        return -1;
	    }   
	    else
	  	*oprpt++ = *point++; 

        *oprpt = '\0';
 
        /* finished with operator, skip spaces and then get the value */
        while ( *point == ' ' )
	    point++;
      	for( ; ; )
	{
	    if ( ( *point != ' ' ) && ( *point == '\0' ) )
	        break;
	    else
	        *valpt++ = *point++; 
	}

        *valpt = '\0';
    }

    bufpt = buf;
    argpt = arg;
    oprpt = opr;
    valpt = val;

    if (deduce_char_arg(rhstarg, prog, val, vict, false) == Deduce_Failure)
        return -1;

    if (deduce_char_arg(target, prog, arg, vict, true) != Deduce_Proceed)
        return -1;

  /* Ok... now buf contains the ifchck, arg contains the inside of the
   *  parentheses, opr contains an operator if one is present, and val
   *  has the value if an operator was present.
   *  So.. basically use if statements and run over all known ifchecks
   *  Once inside, use the argument and expand the lhs. Then if need be
   *  send the lhs,opr,rhs off to be evaluated.
   */

    // Check for a "not" in front
    bool inverted(false);
    if (bufpt[0] == 'n' && bufpt[1] == 'o' && bufpt[2] == 't')
    {
        // Found a "not"
        inverted = true;
        bufpt += 3;
    }

    // Lookup the appropriate if check
    ProgConditionals::ConditionalFun predicate(ProgConditionals::Lookup(bufpt));
    if (predicate != NULL)
    {
        // Translate the RHS
        mprog_do_trans(val, prog, val);

        // Found the if check, so marshal up the context and then evaluate it
        ProgConditionals::Context context = {0};
        context.prog = prog;
        context.target = target;
        context.vict = vict;
        context.rhstarg = rhstarg;
        context.txt = txt;
        context.val = val;
        context.opr = opr;
        context.buf = buf;
        context.arg = arg;
        context.proctxt = proctxt;
        context.checktxt = checktxt;
        int result((*predicate)(context));

        // Handle inversion
        if (inverted)
        {
            if (result == 0) result = 1;
            else if (result > 0) result = 0;
        }
        return result;
    }
        
  /* Ok... all the ifchcks are done, so if we didnt find ours then something
   * odd happened.  So report the bug and abort the MOBprogram (return error)
   */
    if (prog->mob)
    {
	sprintf(log_buf, "Mob: %d, unknown ifcheck '%s'.", prog->mob->pIndexData->vnum, buf);
	bug(log_buf, 0);
    }
    else if (prog->obj)
    {
	sprintf(log_buf, "Obj: %d, unknown ifcheck '%s'.", prog->obj->pIndexData->vnum, buf);
	bug(log_buf, 0);
    }
    else if (prog->room)
    {
	sprintf(log_buf, "Room: %d, unknown ifcheck '%s'.", prog->room->vnum, buf);
	bug(log_buf, 0);
    }
    
    return -1;
}

/* Quite a long and arduous function, this guy handles the control
 * flow part of MOBprograms.  Basicially once the driver sees an
 * 'if' attention shifts to here.  While many syntax errors are
 * caught, some will still get through due to the handling of break
 * and errors in the same fashion.  The desire to break out of the
 * recursion without catastrophe in the event of a mis-parse was
 * believed to be high. Thus, if an error is found, it is bugged and
 * the parser acts as though a break were issued and just bails out
 * at that point. I havent tested all the possibilites, so I'm speaking
 * in theory, but it is 'guaranteed' to work on syntactically correct
 * MOBprograms, so if the mud crashes here, check the mob carefully!
 */
char *mprog_process_if( char *com_list, PROG_RUNDATA *prog, bool *interp )
{
    char buf[ MAX_INPUT_LENGTH ];
    char *morebuf = '\0';
//    char    *cmnd = '\0';
    bool loopdone = FALSE;
    bool     flag = FALSE;
    int loopnum = 0;
    int  legal;
    int  ifcntr    = 0;
    int  elsecntr  = 0;
    bool found_next;

    /* check for trueness of the ifcheck */
    if ((legal = mprog_do_ifchck( prog )))
       if ( legal == 1 )
           flag = TRUE;
       else
	   return NULL;

    while( loopdone == FALSE )
    {
	prog->cmnd     = com_list;
     	com_list = mprog_next_command( com_list, prog->curloop );
     	while ( *prog->cmnd == ' ' )
       	    prog->cmnd++;
     	if ( *prog->cmnd == '\0' || *prog->cmnd == '\n' || *prog->cmnd == '\r' || *prog->cmnd == '/')
     	{
	    if (*com_list == '\0')
	    {
	 	if (prog->mob)
	     	    bug("Mob: %d no commands after IF/OR", prog->mob->pIndexData->vnum); 
	 	else if (prog->obj)
	     	    bug("Obj: %d no commands after IF/OR", prog->obj->pIndexData->vnum);
	 	return NULL;
	    }
	    else
		continue;
	}
        morebuf = one_argument( prog->cmnd, buf );
        if ( !str_cmp( buf, "or" ) )
        {
	    if ( ( legal = mprog_do_ifchck( prog ) ) )
	        if ( legal == 1 )
	            flag = TRUE;
	        else
	     	    return NULL;
     	}
        else
	    loopdone = TRUE;
    }

    if ( flag )
    {
	for ( ; ; ) /*ifcheck was true, do commands but ignore else to endif*/ 
   	{
	    if ( !str_cmp( buf, "if" ) )
            { 
	    	com_list = mprog_process_if(com_list, prog, interp);
		if ( com_list == NULL || *com_list == '\0' )
	     	    return NULL;
		found_next = FALSE;
		while (!found_next)
		{
	   	    prog->cmnd     = com_list;
	   	    com_list = mprog_next_command( com_list, prog->curloop );

		    while (*prog->cmnd == ' ')
			prog->cmnd++;

     		    if ( *prog->cmnd == '\0' || *prog->cmnd == '\n' || *prog->cmnd == '\r' || *prog->cmnd == '/')
		    {
			if (com_list == NULL || *com_list == '\0')
			    return NULL;
			else
			    continue;
		    }
		    else
			found_next = TRUE;
		}

	   	morebuf  = one_argument( prog->cmnd,buf );
	   	continue;
       	    }
       	    if ( !str_cmp( buf, "break" ) )
	 	return NULL;
       	    if ( !str_cmp( buf, "endif" ) )
	 	return com_list; 
       	    if ( !str_cmp( buf, "else" ) ) 
       	    {
	   	while ( str_cmp( buf, "endif" ) || elsecntr > 0 ) 
	   	{
	 	    if (!str_cmp(buf, "if"))
	 	  	elsecntr++;
         	    if (!str_cmp(buf, "endif"))
	   	  	elsecntr--;

		    found_next = FALSE;

		    while (!found_next)
		    {
	       	    	prog->cmnd     = com_list;
	       	    	com_list = mprog_next_command( com_list, prog->curloop );
	       	    	while ( *prog->cmnd == ' ' )
		 	    prog->cmnd++;

     		    	if ( prog->cmnd == NULL || *prog->cmnd == '\0' || *prog->cmnd == '\n' || *prog->cmnd == '\r' || *prog->cmnd == '/')
	       	    	{
			    if (com_list == NULL || *com_list == '\0')
			    {
		   	    	if (prog->mob)
		                    bug("Mob %d, missing endif after else.", prog->mob->pIndexData->vnum);
		   	    	else if (prog->obj)
		       	            bug("Obj %d, missing endif after else.", prog->obj->pIndexData->vnum);
				else if (prog->room)
				    bug("Room %d, missing endif after else.", prog->room->vnum);
		   	    	return NULL;
			    }
			    else
			        continue;
			}
			else
			    found_next = TRUE;			
	       	    }
	       	    morebuf = one_argument( prog->cmnd,buf );
	   	}
	   	return com_list; 
       	    }
       	    if (!str_cmp(buf, "mpnextinterp"))
                *interp = TRUE;
       	    else if (!str_cmp(buf, "loop"))
                prog->curloop = mprog_process_loop( prog, com_list );
       	    else if (!str_cmp(buf, "endloop"))
            {
                if (prog->curloop)
                    loopnum = prog->curloop->depth;

                    prog->curloop = mprog_process_endloop( com_list, prog );
            
                if (prog->curloop 
                     && (prog->curloop->value <= prog->curloop->upper) 
                     && (loopnum == prog->curloop->depth))
                {
                        com_list = prog->curloop->ptr;
                        com_list = mprog_next_command( com_list, prog->curloop );
                }
            }
      	    else
                mprog_process_cmnd( prog );

	    found_next = FALSE;

	    while (!found_next)
	    {
       	    	prog->cmnd     = com_list;
       	    	com_list = mprog_next_command( com_list, prog->curloop );
       	    	while ( *prog->cmnd == ' ' )
	 	    prog->cmnd++;
       	    	if ( prog->cmnd == NULL || *prog->cmnd == '\0' || *prog->cmnd == '\n' || *prog->cmnd == '\r' || *prog->cmnd == '/')
       	    	{
	   	    if (com_list == NULL || *com_list == '\0')
	   	    {
	       	    	if (prog->mob)  
			    bug("Mob %d, missing else or endif.", prog->mob->pIndexData->vnum);
	       	    	else if (prog->obj)
	                    bug("Obj %d, missing else or endif.", prog->obj->pIndexData->vnum);
			else if (prog->room)
			    bug("Room %d, missing else or endif.", prog->room->vnum);
			
                    	return NULL;
	            }
		    else
		        continue;
       	    	}
		else
		    found_next = TRUE;
	    }

	    morebuf = one_argument( prog->cmnd, buf );
   	}
   }
   else /*false ifcheck, find else and do existing commands or quit at endif*/
   	{
     	    while ( (( str_cmp( buf, "else" ) ) && ( str_cmp( buf, "endif" ) )) || ifcntr > 0 )
       	    {
	 	if (!str_cmp(buf, "if"))
	   	    ifcntr++;
         	if (!str_cmp(buf, "endif"))
	   	    ifcntr--;

		found_next = FALSE;
		while (!found_next)
		{
	 	    prog->cmnd     = com_list;
	 	    com_list = mprog_next_command( com_list, prog->curloop );
	 	    while ( *prog->cmnd == ' ' )
	   	    	prog->cmnd++;
     		    if ( prog->cmnd == NULL || *prog->cmnd == '\0' || *prog->cmnd == '\n' || *prog->cmnd == '\r' || *prog->cmnd == '/')
	   	    {
			if (com_list == NULL || *com_list == '\0')
			{
	     	            if (prog->mob)
	         	        bug("Mob %d, missing an else or endif.", prog->mob->pIndexData->vnum);
	     	            else if (prog->obj)
		 	        bug("Obj %d, missing an else or endif.", prog->obj->pIndexData->vnum);
			    else if (prog->room)
				bug("Room %d, missing an else or endif.", prog->room->vnum);
      	     	
	     	            return NULL;
			}
			else
			    continue;
	   	    }
		    else
			found_next = TRUE;
		}
	 	morebuf = one_argument( prog->cmnd, buf );
       	    }

     	/* found either an else or an endif.. act accordingly */
     	/* MW -- nested if statements are broken, because if you go have an if,
      	* and it registers false, then you go else/endif-searching. You can then
      	* find a nested "else", and pick up in entirely the wrong chain of the
      	* prog. Instead, you need to watch for if statements along the way, and
      	* increment the ifcntr. Then, if you find an else and the ifcntr is _0_
      	* you haven't found a nested else or endif, and you can use it. If
      	* you HAVE hit a nested one, treat it as normal, and proceed apace.
      	*/

     	    if ( !str_cmp( buf, "endif" ) )
       		return com_list;

	    found_next = FALSE;
	    while (!found_next)
	    {
     	        prog->cmnd     = com_list;
     	        com_list = mprog_next_command( com_list, prog->curloop );
     	        while ( *prog->cmnd == ' ' )
       		    prog->cmnd++;
     		if ( prog->cmnd == NULL || *prog->cmnd == '\0' || *prog->cmnd == '\n' || *prog->cmnd == '\r' || *prog->cmnd == '/')
       	        { 
		    if (com_list == NULL || *com_list == '\0')
		    {
	 	        if (prog->mob)
	     	            bug ( "Mob %d, missing endif", prog->mob->pIndexData->vnum ); 
	 	        else if (prog->obj)
	     	            bug ( "Obj %d, missing endif", prog->obj->pIndexData->vnum );
			else if (prog->room)
			    bug ( "Room %d, missing endif.", prog->room->vnum);
	 	        return NULL;
		    }
		    else
			continue;
       	        }
		else
		    found_next = TRUE;
	    }

     	    morebuf = one_argument( prog->cmnd, buf );
     
     	    for ( ; ; ) /*process the post-else commands until an endif is found.*/
       	    {
	 	if ( !str_cmp( buf, "if" ) )
	   	{ 
	     	    com_list = mprog_process_if(com_list, prog, interp );

	     	    if ( com_list == NULL || *com_list == '\0' )
	       		return NULL;

		    found_next = FALSE;
		    while (!found_next)
		    {
	   	        prog->cmnd     = com_list;
	   	        com_list = mprog_next_command( com_list, prog->curloop );

		        while (*prog->cmnd == ' ')
			    prog->cmnd++;

     		        if ( *prog->cmnd == '\0' || *prog->cmnd == '\n' || *prog->cmnd == '\r' || *prog->cmnd == '/')
		        {
			    if (com_list == NULL || *com_list == '\0')
			        return NULL;
			    else
			        continue;
		        }
		        else
			    found_next = TRUE;
		    }

	     	    morebuf  = one_argument( prog->cmnd,buf );
	     	    continue;
	   	}

	 	if ( !str_cmp( buf, "else" ) ) 
	   	{
	     	    if (prog->mob)
	         	bug ( "Mob: %d found else in an else section", prog->mob->pIndexData->vnum ); 
	     	    else if (prog->obj)
		 	bug ( "Obj: %d found else in an else section", prog->obj->pIndexData->vnum );
	     	    return NULL;
	   	}
	 	if ( !str_cmp( buf, "break" ) )
	   	    return NULL;
	 	if ( !str_cmp( buf, "endif" ) )
	   	    return com_list; 
        if (!str_cmp(buf, "mpnextinterp")) 
	    	    *interp = TRUE;
	 	else if (!str_cmp(buf, "loop"))
	    	    prog->curloop = mprog_process_loop( prog, com_list );
	 	else if (!str_cmp(buf, "endloop"))
	 	{ 
            if (prog->curloop)
                loopnum = prog->curloop->depth;

            prog->curloop = mprog_process_endloop( com_list, prog );

            if (prog->curloop && (prog->curloop->value <= prog->curloop->upper) 
            && (loopnum == prog->curloop->depth))
            {
                com_list = prog->curloop->ptr;
                com_list = mprog_next_command( com_list, prog->curloop );
            }
	 	}
	 	else
            mprog_process_cmnd( prog );

		found_next = FALSE;
		while (!found_next)
		{
	 	    prog->cmnd     = com_list;
	 	    com_list = mprog_next_command( com_list, prog->curloop );
	 	    while ( *prog->cmnd == ' ' )
	   	        prog->cmnd++;

     		    if ( prog->cmnd == NULL || *prog->cmnd == '\0' || *prog->cmnd == '\n' || *prog->cmnd == '\r' || *prog->cmnd == '/')
	   	    {
			if (com_list == NULL || *com_list == '\0')
			{
	      	            if (prog->mob)
	          	        bug ("Mob %d, missing endif in else section.", prog->mob->pIndexData->vnum ); 
	      	            else if (prog->obj)
		  	        bug ("Obj %d, missing endif in else section.", prog->obj->pIndexData->vnum );
			    else if (prog->room)
				bug ("Room %d, missing endif in else section.", prog->room->vnum);
	     	            return NULL;
	   	        }
			else
			    continue;
		    }
		    else
			found_next = TRUE;
		}

	 	morebuf = one_argument( prog->cmnd, buf );
       	    }
   	}
}

PROG_VARDATA *mprog_find_pvar(const char *txt, PROG_RUNDATA *prog, bool create)
{
	// The prog should never be null during normal prog execution,
	// but it can be if an imm triggered a command directly.
	// In this case, the request is really meaningless, but
	// even meaningless requests can crash us (and did, before this fix)
	if (prog == NULL)
		return NULL;

	// Prog is not null, proceed with var lookup
    PROG_VARDATA * r_var = prog->vars;

    while (r_var)
    {
        if (!str_cmp(r_var->id, txt))
		    return r_var;
	
		r_var = r_var->next;
    }

    if (create)
    {
		r_var = (PROG_VARDATA*)malloc(sizeof(struct prog_vardata));
		g_num_progvars++;
	
		r_var->next = prog->vars;
		r_var->id = str_dup(txt);
		r_var->value = str_dup("");
		prog->vars = r_var;

		return r_var;
    }

    return NULL;
}

void mprog_do_trans(const char *ch, PROG_RUNDATA *prog, char *t)
{
    char tmp[MAX_STRING_LENGTH];
    char newstr[MAX_STRING_LENGTH];
    char *lp, *ld, *l, *p;
//    unsigned short value, x;

    if (t != ch)
		strcpy(t, ch);

    while (((lp = strrchr(t, '%')) != NULL) | ((ld = strrchr(t, '$')) != NULL))
    {
        if (lp && (!ld || (lp > ld)))
	    l = lp;
	else
	    l = ld;

	p = l + 1 + mprog_translate(l, prog, tmp);
/*
	p = l;
	for (x = 0; x <= value; x++)
	    p++;
*/
	strcpy(newstr, tmp);

	if (*p != '\0')
	    strcat(newstr, p);

	*l = '\0';
	strcat(t, newstr);
    }
};

/* This routine handles the variables for command expansion.
 * If you want to add any go right ahead, it should be fairly
 * clear how it is done and they are quite easy to do, so you
 * can be as creative as you want. The only catch is to check
 * that your variables exist before you use them. At the moment,
 * using $t when the secondary target refers to an object 
 * i.e. >prog_act drops~<nl>if ispc($t)<nl>sigh<nl>endif<nl>~<nl>
 * probably makes the mud crash (vice versa as well) The cure
 * would be to change act() so that prog->vo becomes vict & v_obj.
 * but this would require a lot of small changes all over the code.
 */
int mprog_translate( char *ch, PROG_RUNDATA *prog, char *t )
{
    char buf[MAX_STRING_LENGTH];
    char tmp[MAX_STRING_LENGTH];
    char vname[MAX_INPUT_LENGTH];
    static char *he_she        [] = { "it",  "he",  "she" };
    static char *him_her       [] = { "it",  "him", "her" };
    static char *his_her       [] = { "its", "his", "her" };
    CHAR_DATA *vict = (CHAR_DATA *) prog->vo;
    int value = 1, fslot = 0;

    char *v = vname;

    char *i;
    int count;

    *t = '\0';

    if (*ch == '%')
    {
        PROG_VARDATA *c_var;

        ch++;

        value = 0;
    //	while (!isspace(*ch) && *ch != '\0')
        while (isalnum(*ch))
        {
            value++;
            *v = *ch;
            v++, ch++;
        }
        *v = '\0';

        if ((c_var = mprog_find_pvar(vname, prog, FALSE)) != NULL)
            strcpy(t, c_var->value);

        return value;
    }

    ch++;

    // This isn't technically required anymore because of mprog_do_trans, but
    // has been included for $d variable, since that hasn't been upgraded yet.
    if (*ch == '$')
    {
        value += mprog_translate(ch, prog, tmp);
        ch = tmp;
    }

    switch ( *ch ) 
    {
        case 'x':
            strcpy(t, prog->txt);
            break;

        case 'X':
        {
            char *tmp;

            tmp = one_argument(prog->txt, buf);
            strcpy(t, tmp);
            break;
        }

        case 'd':
        {
            char *datastr = NULL;

            if (prog->mob) datastr = prog->mob->prog_state.data_marker;
            else if (prog->obj) datastr = prog->obj->prog_state.data_marker;
            else if (prog->room) datastr = prog->room->prog_state.data_marker;

            while ( datastr && ( *datastr != '\0') && (*datastr != '\r') && (*datastr != '\n') )
            {
                if (*datastr != '$' && *datastr != '%')
                {
                    *t++ = *datastr++;
                    continue;
                }

                count = mprog_translate( datastr, prog, tmp );
                datastr += count + 1;
       
                i = tmp;
                while ( ( *t = *i ) != '\0' )
                    ++t, ++i;
            }

            *t = '\0';
            break;
        }
	    
        case 'i':
	    if (prog->mob)
		    strcpy(t, prog->mob->unique_name);
	    else if (prog->obj)
	        one_argument( prog->obj->name, t );
            break;
	
        case 'I':
	    if (prog->mob)
		strcpy( t, prog->mob->short_descr );
	    else if (prog->obj)
		strcpy( t, prog->obj->short_descr );
      	    break;

        case 'n':
            if (prog->actor)
            {
                if (!prog->mob || can_see(prog->mob,prog->actor))
                    strcpy(t, prog->actor->unique_name);
            }
      	    break;

        case 'N':
            if (prog->actor)
            {
                if (!prog->mob || can_see(prog->mob, prog->actor))
                {
                    if (IS_NPC(prog->actor) || (prog->actor->short_descr != NULL && prog->actor->short_descr[0] != '\0'))
                        strcpy(t, prog->actor->short_descr);
                    else 
                        strcpy(t, prog->actor->name);
                }
                else
                    strcpy(t, "someone");
            }
    	    break;

        case 't':
            if (vict)
            {
                if (!prog->mob || can_see(prog->mob,vict))
                strcpy(t, vict->unique_name);
            }
      	    break;

        case 'T':
            if (vict)
            {
                    if (!prog->mob || can_see(prog->mob, vict))
                    if (IS_NPC(vict))
                strcpy(t, vict->short_descr);
                    else
                strcpy(t, vict->name);
                else
                    strcpy(t, "someone");
            }
            break;


        case 'Z':
            if (prog->actor)
            {
                if (IS_NPC(prog->actor))
                    strcpy(t, prog->actor->long_descr);
                else
                {
                        if (prog->actor->pcdata->extitle
                     && prog->actor->pcdata->extitle[0] != '\0')
                        sprintf(buf, "%s%s, %s is here.", prog->actor->name,
                                prog->actor->pcdata->title, prog->actor->pcdata->extitle);
                        else
                        sprintf(buf, "%s%s is here.", prog->actor->name,
                        prog->actor->pcdata->title);
                    strcpy(t, buf);
                }
            }
            break;

        case 'b':
            if (prog->obj && prog->obj->carried_by)
                strcpy(t, IS_NPC(prog->obj->carried_by) ? prog->obj->carried_by->short_descr : prog->obj->carried_by->name);
            break;

        case 'B':
            if (prog->obj && prog->obj->carried_by)
                strcpy(t, his_her[prog->obj->carried_by->sex]);
            break;

        case 'F':
	    if (isdigit(ch[1]))
	    {	
            fslot = ch[1] - '0';
            value++;
	    }
	
	    if (prog->mob && prog->mob->memfocus[fslot])
	        if (IS_NPC(prog->mob->memfocus[fslot]))
		    strcpy( t, prog->mob->memfocus[fslot]->short_descr );
	        else
	            strcpy( t, prog->mob->memfocus[fslot]->name );
	    else if (!prog->mob && prog->obj && prog->obj->objfocus[fslot])
		if (IS_NPC(prog->obj->objfocus[fslot]))
		    strcpy( t, prog->obj->objfocus[fslot]->short_descr );
		else
		    strcpy( t, prog->obj->objfocus[fslot]->name );
	    else if (!prog->mob && !prog->obj && prog->room && prog->room->roomfocus[fslot])
		if (IS_NPC(prog->room->roomfocus[fslot]))
		    strcpy( t, prog->room->roomfocus[fslot]->short_descr );
		else
		    strcpy( t, prog->room->roomfocus[fslot]->name );
	    else
	        strcpy( t, "someone" );
	    break;

        case 'f':
	    if (isdigit(ch[1]))
	    {	
            fslot = ch[1] - '0';
            value++;
	    }

        if (prog->mob && prog->mob->memfocus[fslot])
		    strcpy(t, prog->mob->memfocus[fslot]->unique_name);
	    else if (!prog->mob && prog->obj && prog->obj->objfocus[fslot])
		    strcpy(t, prog->obj->objfocus[fslot]->unique_name);
	    else if (!prog->mob && !prog->obj && prog->room && prog->room->roomfocus[fslot])
		    strcpy(t, prog->room->roomfocus[fslot]->unique_name);
	    else
	        strcpy (t, "someone" );
	    break;

        case 'r':
        {
            CHAR_DATA * rndm(lookupRandom(*prog));
            if (rndm != NULL)
                strcpy(t, prog->rndm->unique_name);

            break;
        }

        case 'R':
        {
            CHAR_DATA * rndm(lookupRandom(*prog));
            if (rndm != NULL)
            {
                if (!prog->mob || can_see(prog->mob, rndm))
                {
                    if (IS_NPC(rndm))
                        strcpy(t, rndm->short_descr);
                    else
                        strcpy(t, rndm->name);
                }
                else
                    strcpy(t, "someone");
            }
    	    break;
        }

	case 'e':
            if ( prog->actor )
	   	(!prog->mob || can_see(prog->mob, prog->actor)) ? strcpy(t, he_she[prog->actor->sex])
	        	                      : strcpy(t, "someone");
	    break;

	case 'E':
	    if (isdigit(ch[1]))
	    {	
		fslot = ch[1] - '0';
		value++;
	    }
            if ( prog->mob && prog->mob->memfocus[fslot] )
	   	can_see(prog->mob, prog->mob->memfocus[fslot]) ? strcpy(t, he_she[prog->mob->memfocus[fslot]->sex])
	        	                      : strcpy(t, "someone");
	    else if (!prog->mob && prog->obj && prog->obj->objfocus[fslot])
		strcpy(t, he_she[prog->obj->objfocus[fslot]->sex]);
	    else if (!prog->mob && !prog->obj && prog->room && prog->room->roomfocus[fslot])
		strcpy(t, he_she[prog->room->roomfocus[fslot]->sex]);
	    break;

  
        case 'm':
            if ( prog->actor )
	   	(!prog->mob || can_see(prog->mob, prog->actor)) ? strcpy(t, him_her[prog->actor->sex])
                                              : strcpy( t, "someone" );
	    break;
  
        case 'M':
	    if (isdigit(ch[1]))
	    {	
		fslot = ch[1] - '0';
		value++;
	    }
            if ( prog->mob && prog->mob->memfocus[fslot] )
	   	can_see(prog->mob, prog->mob->memfocus[fslot]) ? strcpy(t, him_her[prog->mob->memfocus[fslot]->sex])
                                              : strcpy( t, "someone" );
	    else if (!prog->mob && prog->obj && prog->obj->objfocus[fslot])
		strcpy(t, him_her[prog->obj->objfocus[fslot]->sex]);
	    else if (!prog->mob && !prog->obj && prog->room && prog->room->roomfocus[fslot])
		strcpy(t, him_her[prog->room->roomfocus[fslot]->sex]);
	    break;

        case 's':
            if (prog->actor)
	   	(!prog->mob || can_see(prog->mob, prog->actor)) ? strcpy(t, his_her[prog->actor->sex])
	                                      : strcpy( t, "someone's" );
	    break;

        case 'S':
	    if (isdigit(ch[1]))
	    {	
		fslot = ch[1] - '0';
		value++;
	    }
            if ( prog->mob && prog->mob->memfocus[fslot] )
	   	can_see(prog->mob, prog->mob->memfocus[fslot]) ? strcpy(t, his_her[prog->mob->memfocus[fslot]->sex])
	                                      : strcpy( t, "someone's" );
	    else if (!prog->mob && prog->obj && prog->obj->objfocus[fslot])
		strcpy(t, his_her[prog->obj->objfocus[fslot]->sex]);
	    else if (!prog->mob && !prog->obj && prog->room && prog->room->roomfocus[fslot])
		strcpy(t, his_her[prog->room->roomfocus[fslot]->sex]);
	    break;
    
        case 'j':
	    if (prog->mob)
	        strcpy( t, he_she[ prog->mob->sex ] );
	    else if (prog->obj)
		strcpy( t, "it" );
	    break;
  
        case 'k':
	    if (prog->mob)
	    	strcpy( t, him_her[ prog->mob->sex ] );
	    else if (prog->obj)
		strcpy(t, "it");
	    break;
  
        case 'l':
	    if (prog->mob)
	    	strcpy( t, his_her[ prog->mob->sex ] );
	    else if (prog->obj)
		strcpy(t, "its");
	    break;

        case 'J':
        {
            CHAR_DATA * rndm(lookupRandom(*prog));
            if (rndm != NULL)
                strcpy(t, he_she[rndm->sex]);
            break;
        }
  
        case 'K':
        {
            CHAR_DATA * rndm(lookupRandom(*prog));
            if (rndm != NULL)
                strcpy(t, him_her[rndm->sex]);
            break;
        }
  
        case 'L':
        {
            CHAR_DATA * rndm(lookupRandom(*prog));
            if (rndm != NULL)
                strcpy(t, his_her[rndm->sex]);
            break;
        }

        case 'o':
            if (prog->obj)
	    {
		if (prog->mob)
	   	    can_see_obj(prog->mob, prog->obj) ? one_argument( prog->obj->name, t )
                                          : strcpy( t, "something" );
		else if (prog->obj->carried_by)
		{
		    one_argument(prog->obj->carried_by->unique_name, t);
       		    *t = UPPER( *t );
		}
	    }
	    break;

        case 'O':
            if ( prog->obj )
	    {
		if (prog->mob)
		    can_see_obj(prog->mob, prog->obj) ? strcpy( t, prog->obj->short_descr )
                                                : strcpy( t, "something" );
		else if (prog->obj->carried_by)
		{
		    if (IS_NPC(prog->obj->carried_by))
		        strcpy(t, prog->obj->carried_by->short_descr);
		    else
			strcpy(t, prog->obj->carried_by->name);
		}
	    }
	    break;

        case '0':
	    if (prog->mob)
		sprintf(buf, "%d", prog->mob->mobvalue[0]);
	    else if (prog->obj)
		sprintf(buf, "%d", prog->obj->objvalue[0]);
	    else if (prog->room)
		sprintf(buf, "%d", prog->room->roomvalue[0]);
	    strcpy(t, buf);
	    break;

        case '1':
	    if (prog->mob)
		sprintf(buf, "%d", prog->mob->mobvalue[1]);
	    else if (prog->obj)
		sprintf(buf, "%d", prog->obj->objvalue[1]);
	    else if (prog->room)
		sprintf(buf, "%d", prog->room->roomvalue[1]);
	    strcpy(t, buf);
	    break;

        case '2':
	    if (prog->mob)
		sprintf(buf, "%d", prog->mob->mobvalue[2]);
	    else if (prog->obj)
		sprintf(buf, "%d", prog->obj->objvalue[2]);
	    else if (prog->room)
		sprintf(buf, "%d", prog->room->roomvalue[2]);
	    strcpy(t, buf);
	    break;

        case '3':
	    if (prog->mob)
		sprintf(buf, "%d", prog->mob->mobvalue[3]);
	    else if (prog->obj)
		sprintf(buf, "%d", prog->obj->objvalue[3]);
	    else if (prog->room)
		sprintf(buf, "%d", prog->room->roomvalue[3]);
	    strcpy(t, buf);
	    break;

        case '4':
	    if (prog->mob)
		sprintf(buf, "%d", prog->mob->mobvalue[4]);
	    else if (prog->obj)
		sprintf(buf, "%d", prog->obj->objvalue[4]);
	    else if (prog->room)
		sprintf(buf, "%d", prog->room->roomvalue[4]);
	    strcpy(t, buf);
	    break;

        case '5':
	    if (prog->mob)
		sprintf(buf, "%d", prog->mob->mobvalue[5]);
	    else if (prog->obj)
		sprintf(buf, "%d", prog->obj->objvalue[5]);
	    else if (prog->room)
		sprintf(buf, "%d", prog->room->roomvalue[5]);
	    strcpy(t, buf);
	    break;

        case '6':
	    if (prog->mob)
		sprintf(buf, "%d", prog->mob->mobvalue[6]);
	    else if (prog->obj)
		sprintf(buf, "%d", prog->obj->objvalue[6]);
	    else if (prog->room)
		sprintf(buf, "%d", prog->room->roomvalue[6]);
	    strcpy(t, buf);
	    break;

        case '7':
	    if (prog->mob)
		sprintf(buf, "%d", prog->mob->mobvalue[7]);
	    else if (prog->obj)
		sprintf(buf, "%d", prog->obj->objvalue[7]);
	    else if (prog->room)
		sprintf(buf, "%d", prog->room->roomvalue[7]);
	    strcpy(t, buf);
	    break;

        case '8':
	    if (prog->mob)
		sprintf(buf, "%d", prog->mob->mobvalue[8]);
	    else if (prog->obj)
		sprintf(buf, "%d", prog->obj->objvalue[8]);
	    else if (prog->room)
		sprintf(buf, "%d", prog->room->roomvalue[8]);
	    strcpy(t, buf);
	    break;

        case '9':
	    if (prog->mob)
		sprintf(buf, "%d", prog->mob->mobvalue[9]);
	    else if (prog->obj)
		sprintf(buf, "%d", prog->obj->objvalue[9]);
	    else if (prog->room)
		sprintf(buf, "%d", prog->room->roomvalue[9]);
	    strcpy(t, buf);
	    break;

        case 'z': //brazen: $z is the current value assosiated with mpremember
	    if (prog->mob)
		sprintf(buf, "%d", get_mval(prog->mob, prog->actor->name));
	    strcpy(t, buf);
	    break;
        
        case '$':
            strcpy( t, "$" );
	    break;

        case ':':
        {
            struct tm *loctime;

            loctime = localtime(&current_time);
            strftime(buf, 128, "%X", loctime);
            strcpy(t, buf);
            break;
        }

        case 'v':
        {
            LOOP_DATA *checkloop = prog->curloop;
            int x = 0;

                ++ch;
                while (isdigit(ch[0]))
                {
                x = (x * 10) + (ch[0] - '0');
                ++ch;
                    value++;
                }
           
            if (x > 0)
            {
                while (checkloop && (checkloop->depth != x))
                if (checkloop->prev)
                        checkloop = checkloop->prev;
                else checkloop = NULL;
                if (checkloop)
                {
                sprintf(buf, "%d", checkloop->value);
                strcpy(t, buf);
                }
            }
            else
            if (prog->mob)
                    bug("Mob: %d invalid loop value check", prog->mob->pIndexData->vnum);
            else if (prog->obj)
                bug("Obj: %d invalid loop value check", prog->obj->pIndexData->vnum);

            break;
        }
	case 'h':
	{
	    sprintf(buf, "%d", hitprog_damage);
	    strcpy(t, buf);
	    break;
	}	
    	case 'y':
     	case 'Y':
	{
	    int x = 0, y;
	    char *proctxt, *curtmp;	
	    bool keepall = TRUE;
        
	    if (ch[0] == 'y')
	        keepall = FALSE;

	    proctxt = prog->txt;	

	    ++ch;
	    while (isdigit(ch[0]))
	    {
	        x = (x * 10) + (ch[0] - '0');
                ++ch;
	        value++;
	    }
	
            if (x > 0)
            {
  	    	for (y = 0; y < (x - 1); y++)
	    	{	
		    while ((!isspace(proctxt[0])) && (proctxt[0] != '\0'))
		        ++proctxt;
		    if (isspace(proctxt[0]))
                        while (isspace(proctxt[0]))
			    ++proctxt;
	        }
	        if (proctxt[0] != '\0')
	        {
	            curtmp = buf;
	            while((proctxt[0] != '\0') && !isspace(proctxt[0]))
	            {  
	    	        if (!keepall)
		        {
		            if (isalpha(proctxt[0]))
		    	    	*curtmp++ = LOWER(proctxt[0]);
			    else if (isdigit(proctxt[0]))
				*curtmp++ = proctxt[0];
		        }
		        else
		            *curtmp++ = proctxt[0];
		        ++proctxt;
		    }
		    *curtmp = '\0';
	        }
	        else
	            buf[0] = '\0';
	    }
	    else
	    {
		if (prog->mob)
	            bug("BUG: Bad value for $y.  Mob: %d.", prog->mob->pIndexData->vnum);
		else if (prog->obj)
		    bug("BUG: Bad value for $y.  Obj: %d.", prog->obj->pIndexData->vnum);
	    }

	    strcpy(t, buf);
	    break;
	}
        default:
	    if (prog->mob)
                bug( "Mob: %d bad $var", prog->mob->pIndexData->vnum );
	    else if (prog->obj)
		bug( "Obj: %d bad $var", prog->obj->pIndexData->vnum );
	    break;
    }

    return value;
}

/* This procedure simply copies the cmnd to a buffer while expanding
 * any variables by calling the translate procedure.  The observant
 * code scrutinizer will notice that this is taken from act()
 */
void mprog_process_cmnd( PROG_RUNDATA *prog )
{
  char buf[ MAX_INPUT_LENGTH ];
  PROG_RUNDATA *curprog;

  mprog_do_trans(prog->cmnd, prog, buf);

   curprog = prog;

  if (prog->mob)
  {
    interpret( prog->mob, buf );
    prog->mob->prog_data = prog;
  }
  else if (prog->obj)
  {
    ointerpret( prog->obj, buf );
    prog->obj->prog_data = prog;
  }
  else if (prog->room)
  {
    rinterpret( prog->room, buf );
    prog->room->prog_data = prog;
  }
}

LOOP_DATA *mprog_process_loop( PROG_RUNDATA *prog, char *next_com )
{
    int loopmin, loopmax;
    char buf[MAX_INPUT_LENGTH];
    char arg[MAX_STRING_LENGTH];
    char *cptr; //, *str, *i;
    LOOP_DATA *loopnew;

    mprog_do_trans(prog->cmnd, prog, buf);
    cptr = buf;

    cptr = one_argument(cptr, arg);
    cptr = one_argument(cptr, arg);
    
    if (is_number(arg))
	loopmin = atoi(arg);
    else
    {
	if (prog->mob)
	    bug("Mob: %d invalid loop call (bad minimum number).", prog->mob->pIndexData->vnum);
	else if (prog->obj)
	    bug("Obj: %d invalid loop call (bad minimum number).", prog->obj->pIndexData->vnum);
        return NULL;
    }
    
    cptr = one_argument(cptr, arg);
    cptr = one_argument(cptr, arg);

    if (is_number(arg))
	loopmax = atoi(arg);
    else
    {
	if (prog->mob)
	    bug("Mob: %d invalid loop call (bad maximum number).", prog->mob->pIndexData->vnum);
	else if (prog->obj)
	    bug("Obj: %d invalid loop call (bad maximum number).", prog->obj->pIndexData->vnum);
   	return NULL;
    }

    loopnew = (LOOP_DATA*)malloc(sizeof(struct loop_data));
    g_num_loopdata++;
    loopnew->value = loopmin;
    loopnew->lower = loopmin;
    loopnew->upper = loopmax;
    loopnew->ptr   = prog->cmnd;

    if (prog->curloop)
    {
	loopnew->prev = prog->curloop;
	loopnew->depth = prog->curloop->depth + 1;
        loopnew->endprog = prog->curloop->endprog;
    }
    else
    {
        loopnew->prev = NULL;
	loopnew->depth = 1;
	loopnew->endprog = next_com + strlen(next_com);
    }

    prog->curloop	   = loopnew;

    return loopnew;
}

LOOP_DATA *mprog_process_endloop(char *com_list, PROG_RUNDATA *prog)
{
    LOOP_DATA *oldloop;

    if (prog->curloop)
    {
	prog->curloop->value++;
	if (prog->curloop->value > prog->curloop->upper)
	{
	    if (prog->curloop->prev)
	    {
		oldloop = prog->curloop;
		prog->curloop = prog->curloop->prev;
		free(oldloop);
		g_num_loopdata--;
		return prog->curloop;
	    }
	    else
	    {
	        free(prog->curloop);  
		g_num_loopdata--;
		return NULL;
 	    }
	}
	else
	    return prog->curloop;
    } 
    else
    {
	if (prog->mob)
	    bug("Mob: %d endloop without loop.", prog->mob->pIndexData->vnum);
	else if (prog->obj)
	    bug("Obj: %d endloop without loop.", prog->obj->pIndexData->vnum);
	return NULL;
    }
}

template <typename TYPE> void CleanUpProgData(TYPE *& pointer)
{
	if (pointer != NULL && !pointer->valid)
		pointer = NULL;
}

/* The main focus of the MOBprograms.  This routine is called 
 *  whenever a trigger is successful.  It is responsible for parsing
 *  the command list and figuring out what to do. However, like all
 *  complex procedures, everything is farmed out to the other guys.
 */
bool mprog_driver ( char *com_list, CHAR_DATA *mob, CHAR_DATA *actor,
		   OBJ_DATA *obj, ROOM_INDEX_DATA *room, void *vo, char *txt)
{
    char tmpcmndlst[ MAX_STRING_LENGTH ];
    char buf       [ MAX_INPUT_LENGTH ];
    char *morebuf;
    char *command_list;
    CHAR_DATA *vch   = NULL;
    CHAR_DATA *ich   = NULL;
    CHAR_DATA *dummy = NULL;
    int loopnum = 0;
    bool interp = FALSE;
    bool found_next;
    PROG_RUNDATA prog;

    if (mob && IS_SET(mob->act, ACT_ILLUSION))
        return false;

    prog.mob = mob;
    prog.actor = actor;
    prog.obj = obj;
    prog.room = room;
    prog.vo = vo;
    prog.txt = txt;
    prog.rndm = NULL;
    prog.rndmID = 0;
    prog.curloop = NULL;
    prog.vars = NULL;
    
    /* get a random visable mortal player who is */
    /* in the room with the mob or obj           */

    if ( mob )
        dummy = mob;
    else if ( obj && ( obj->carried_by ))
        dummy = obj->carried_by;

    if (dummy && dummy->in_room)
        ich = dummy->in_room->people;
    else if (obj && obj->in_room)
        ich = obj->in_room->people;
    else if (room)
        ich = room->people;

    if (ich)
    {
        std::vector<CHAR_DATA*> chars;
    	for (vch = ich; vch; vch = vch->next_in_room)
        {
            if ((dummy && dummy != vch && can_see(dummy, vch)) || !dummy)
                chars.push_back(vch);
        }

        if (!chars.empty())
        {
            prog.rndm = chars[number_range(0, chars.size() - 1)];
            prog.rndmID = prog.rndm->id;
        }
    }

    mcur_running = mob;
	
    strcpy( tmpcmndlst, com_list );
    command_list = tmpcmndlst;

    found_next = FALSE;
    while (!found_next)
    {
    	prog.cmnd     = command_list;
       	command_list = mprog_next_command(command_list, FALSE );
       	while ( *prog.cmnd == ' ' )
		    prog.cmnd++;

       	if ( prog.cmnd == NULL || *prog.cmnd == '\0' || *prog.cmnd == '\n' || *prog.cmnd == '\r' || *prog.cmnd == '/')
       	{
	    	if (command_list == NULL || *command_list == '\0')
               	return interp;
		    else
		        continue;
       	}
		else
		    found_next = TRUE;
    }

    // This is being put here to ensure that it will get removed before the only
    // remaining exit point in this function.
    if (mob)
	mob->prog_data = &prog;
    else if (obj)
	obj->prog_data = &prog;
    else if (room)
	room->prog_data = &prog;

    while ( *prog.cmnd != '\0' && prog.cmnd != NULL )
    {
        morebuf = one_argument( prog.cmnd, buf );

        /* A bit hack-ish, for the sake of making it compatible with the */
        /* old usage of opinterpret.  - Erinos				 */
        if (!str_cmp(prog.cmnd, "mpnextinterp"))
	    	interp = TRUE;
        else if ( !str_cmp( buf, "loop") )
		    prog.curloop = mprog_process_loop( &prog, command_list );
        else if ( !str_cmp( buf, "if" ) )
        {
            command_list = mprog_process_if( command_list, &prog, &interp );
        }
        else if (!str_cmp(buf, "endloop") )
        {
		    if (prog.curloop)
                loopnum = prog.curloop->depth;
		    prog.curloop = mprog_process_endloop( command_list, &prog );

	    	if (prog.curloop && (prog.curloop->value <= prog.curloop->upper) && (prog.curloop->depth == loopnum))
	    	{
	        	command_list = prog.curloop->ptr;
                command_list = mprog_next_command (command_list, prog.curloop);
            }
        }	           
        else
            mprog_process_cmnd( &prog );

		if (!command_list || (!mob && !obj && !room))
	    	break;

		// We do the best we can here, but for now, we're not fully
		// protected against bad or unlucky progs that destroy these
		// variables in the middle
		CleanUpProgData(prog.mob);
		CleanUpProgData(prog.actor);
		CleanUpProgData(prog.obj);
		CleanUpProgData(prog.rndm);
        if (prog.rndm == NULL)
            prog.rndmID = 0;

	found_next = FALSE;
    	while (!found_next)
    	{
    	    prog.cmnd     = command_list;
//       	    command_list = mprog_next_command(command_list, FALSE );
	    command_list = mprog_next_command(command_list, prog.curloop);
       	    while ( *prog.cmnd == ' ' )
	        prog.cmnd++;
       	    if ( prog.cmnd == NULL || *prog.cmnd == '\0' || *prog.cmnd == '\n' || *prog.cmnd == '\r' || *prog.cmnd == '/')
       	    {
	        if (command_list == NULL || *command_list == '\0')
		{
		    prog.cmnd = command_list;
		    break;
		}
	        else
	            continue;
       	    }
	    else
	        found_next = TRUE;
        }
    } 

    if (prog.curloop)
    {
        LOOP_DATA *freeloop(0);
        while (prog.curloop)
        {
            freeloop = prog.curloop;
            if (prog.curloop->prev)
	        prog.curloop = prog.curloop->prev;
	    else
	        prog.curloop = NULL;
		if (freeloop != 0)
		    free(freeloop);
	    g_num_loopdata--;
        }
     }

    if (prog.vars)
    {
	PROG_VARDATA *nextvar;
        while (prog.vars)
	{
	    nextvar = prog.vars->next;
	    free(prog.vars);
	    g_num_progvars--;
	    prog.vars = nextvar;
	}
    }

    if (mob)
	mob->prog_data = NULL;
    else if (obj)
	obj->prog_data = NULL;
    else if (room)
	room->prog_data = NULL;

    mcur_running = NULL;
    return interp;
}

/***************************************************************************
 * Global function code and brief comments.
 */

/* The next two routines are the basic trigger types. Either trigger
 *  on a certain percent, or trigger on a keyword or word phrase.
 *  To see how this works, look at the various trigger routines..
 */
void mprog_wordlist_check( char *arg, CHAR_DATA *mob, CHAR_DATA *actor,
			  OBJ_DATA *obj, void *vo, int type )
{

    char        temp1[ MAX_STRING_LENGTH ];
    char        temp2[ MAX_INPUT_LENGTH ];
    char        word[ MAX_INPUT_LENGTH ];
    char	dupword[ MAX_STRING_LENGTH ];
    MPROG_DATA	*mprg;
    char	*list;
    char	*start;
    char	*dupl;
    char	*dpoint;
    unsigned int i;


    for ( mprg = mob->pIndexData->mobprogs; mprg != NULL; mprg = mprg->next )
	if ( mprg->type & type )
	{
	    strcpy( temp1, mprg->arglist );
	    list = temp1;

	    for ( i = 0; i < strlen( list ); i++ )
		list[i] = LOWER( list[i] );

	    strcpy( temp2, arg );
	    dupl = temp2;

	    for ( i = 0; i < strlen( dupl ); i++ )
		dupl[i] = LOWER( dupl[i] );

	    if ((list[0] == 'x') && (list[1] == ' '))
	    {
		regex_t expr;
		int success;

		if (regcomp(&expr, list, REG_EXTENDED))
		{
		    bug("Bad speech_prog regex compilation, mob %d", mob->pIndexData->vnum);
		    return;
		}
		else
		{
		    success = regexec(&expr, dupl, 0, NULL, 0);
		    regfree(&expr);
	
		    if (!success)
		    {
			mprog_driver( mprg->comlist, mob, actor, obj, NULL, vo, arg );
			return;
		    }
		    else if (success != REG_NOMATCH)
		    {
			bug("Error during speech_prog regex execution, mob %d", mob->pIndexData->vnum);
			return;
		    }
		}
	    }	
	    else if ( ( list[0] == 'p' ) && ( list[1] == ' ' ) )
	    {
		list += 2;

		while ( ( start = strstr( dupl, list ) ) )
		    if ( (start == dupl || *(start-1) == ' ' )
		     && !isalnum(*(start + strlen(list))))
/*
		     && ( *(end = start + strlen( list ) ) == ' '
		      || *end == '\n'
		      || *end == '\r'
		      || *end == '\0' ) )
*/
		    {
			mprog_driver( mprg->comlist, mob, actor, obj, NULL, vo, arg );
			return;
		    }
		    else
			dupl = start+1;
	    }
	    else if ( ( list[0] == 'p' ) && ( list[1] == '\0' || list[1] == '\n' || list[1] == '\r'))
	    {
		bug("blank p driver", 0);
		mprog_driver( mprg->comlist, mob, actor, obj, NULL, vo, arg );
		return;
	    }
	    else
	    {
	        for (dupl = one_argument(dupl, dupword); dupword[0] != '\0'; dupl = one_argument(dupl, dupword))
	        {
		    strip_punc(dupword, dupword);

		    for (dpoint = one_argument(list, word); word[0] != '\0'; dpoint = one_argument(dpoint, word))
		        if (!str_cmp(dupword, word))
		        {
			    mprog_driver(mprg->comlist, mob, actor, obj, NULL, vo, arg);
		 	    return;
		        }
		
	        }
	    }
        }

  return;
}

void oprog_wordlist_check( char *arg, OBJ_DATA *mob, CHAR_DATA *actor,
			  OBJ_DATA *obj, void *vo, int type )
{

  char        temp1[ MAX_STRING_LENGTH ];
  char        temp2[ MAX_INPUT_LENGTH ];
  char        word[ MAX_INPUT_LENGTH ];
  MPROG_DATA *mprg(0);
  char       *list;
  char       *start;
  char       *dupl;
  unsigned int i;

  for ( mprg = mob->pIndexData->objprogs; mprg != NULL; mprg = mprg->next )
    if ( mprg->type & type )
      {
	strcpy( temp1, mprg->arglist );
	list = temp1;
	for ( i = 0; i < strlen( list ); i++ )
	  list[i] = LOWER( list[i] );
	strcpy( temp2, arg );
	dupl = temp2;
	for ( i = 0; i < strlen( dupl ); i++ )
	  dupl[i] = LOWER( dupl[i] );

	    if ((list[0] == 'x') && (list[1] == ' '))
	    {
		regex_t expr;
		int success;

		if (regcomp(&expr, list, REG_EXTENDED))
		{
		    bug("Bad speech_prog regex compilation, obj %d", mob->pIndexData->vnum);
		    return;
		}
		else
		{
		    success = regexec(&expr, dupl, 0, NULL, 0);
		    regfree(&expr);
	
		    if (!success)
		    {
			mprog_driver( mprg->comlist, NULL, actor, mob, NULL, vo, arg);
			return;
		    }
		    else if (success != REG_NOMATCH)
		    {
			bug("Error during speech_prog regex execution, obj %d", mob->pIndexData->vnum);
			return;
		    }
		}
	    }	

	  else if ( ( list[0] == 'p' ) && ( list[1] == ' ' ) )
	  {
	    list += 2;
	    while ( ( start = strstr( dupl, list ) ) )
	      if ( (start == dupl || *(start-1) == ' ' )
		     && !isalnum(*(start + strlen(list))))
/*
		     && ( *(end = start + strlen( list ) ) == ' '
		      || *end == '\n'
		      || *end == '\r'
		      || *end == '\0' ) )
*/
		{
		  mprog_driver( mprg->comlist, NULL, actor, mob, NULL, vo, arg);
		  break;
		}
	      else
		dupl = start+1;
	  }
	else
	  {
	    list = one_argument( list, word );
	    for( ; word[0] != '\0'; list = one_argument( list, word ) )
	      while ( ( start = strstr( dupl, word ) ) )
		if ( ( start == dupl || *(start-1) == ' ' )
		     && !isalnum(*(start + strlen(word))))
/*
		     && ( *(end = start + strlen( word ) ) == ' '
		      || *end == '\n'
		      || *end == '\r'
		      || *end == '\0' ) )
*/
		  {
		    mprog_driver( mprg->comlist, NULL, actor, mob, NULL, vo, arg );
		    break;
		  }
		else
		  dupl = start+1;
	  }
      }

  return;

}

void rprog_wordlist_check( char *arg, ROOM_INDEX_DATA *room, CHAR_DATA *actor,
			   void *vo, int type )
{

  char        temp1[ MAX_STRING_LENGTH ];
  char        temp2[ MAX_INPUT_LENGTH ];
  char        word[ MAX_INPUT_LENGTH ];
  MPROG_DATA *mprg;
  char       *list;
  char       *start;
  char       *dupl;
  unsigned int i;

  for ( mprg = room->mobprogs; mprg != NULL; mprg = mprg->next )
    if ( mprg->type & type )
      {
	strcpy( temp1, mprg->arglist );
	list = temp1;
	for ( i = 0; i < strlen( list ); i++ )
	  list[i] = LOWER( list[i] );
	strcpy( temp2, arg );
	dupl = temp2;
	for ( i = 0; i < strlen( dupl ); i++ )
	  dupl[i] = LOWER( dupl[i] );

	    if ((list[0] == 'x') && (list[1] == ' '))
	    {
		regex_t expr;
		int success;

		if (regcomp(&expr, list, REG_EXTENDED))
		{
		    bug("Bad speech_prog regex compilation, room %d", room->vnum);
		    return;
		}
		else
		{
		    success = regexec(&expr, dupl, 0, NULL, 0);
		    regfree(&expr);
	
		    if (!success)
		    {
			mprog_driver( mprg->comlist, NULL, actor, NULL, room, vo, arg);
			return;
		    }
		    else if (success != REG_NOMATCH)
		    {
			bug("Error during speech_prog regex execution, room %d", room->vnum);
			return;
		    }
		}
	    }	

	  else if ( ( list[0] == 'p' ) && ( list[1] == ' ' ) )
	  {
	    list += 2;
	    while ( ( start = strstr( dupl, list ) ) )
	      if ( (start == dupl || *(start-1) == ' ' )
		     && !isalnum(*(start + strlen(list))))
/*
		     && ( *(end = start + strlen( list ) ) == ' '
		      || *end == '\n'
		      || *end == '\r'
		      || *end == '\0' ) )
*/
		{
		  mprog_driver( mprg->comlist, NULL, actor, NULL, room, vo, arg);
		  break;
		}
	      else
		dupl = start+1;
	  }
	else
	  {
	    list = one_argument( list, word );
	    for( ; word[0] != '\0'; list = one_argument( list, word ) )
	      while ( ( start = strstr( dupl, word ) ) )
		if ( ( start == dupl || *(start-1) == ' ' )
		     && !isalnum(*(start + strlen(word))))
/*
		     && ( *(end = start + strlen( word ) ) == ' '
		      || *end == '\n'
		      || *end == '\r'
		      || *end == '\0' ) )
*/
		  {
		    mprog_driver( mprg->comlist, NULL, actor, NULL, room, vo, arg );
		    break;
		  }
		else
		  dupl = start+1;
	  }
      }

  return;

}

void rprog_percent_check( ROOM_INDEX_DATA *room, CHAR_DATA *actor, OBJ_DATA *obj,
			  void *vo, int type )
{
    MPROG_DATA *mprg;

    for (mprg = room->mobprogs; mprg; mprg = mprg->next)
    {
	if ((mprg->type & type) && (number_percent() <= atoi(mprg->arglist)))
	{
	    mprog_driver(mprg->comlist, NULL, actor, obj, room, vo, NULL);
	    if ((type != GREET_PROG) && (type != ALL_GREET_PROG))
		break;
	}
    }

    return;
}

void mprog_percent_check( CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj,
			 void *vo, int type)
{
 MPROG_DATA * mprg;

 for ( mprg = mob->pIndexData->mobprogs; mprg != NULL; mprg = mprg->next )
   if ( ( mprg->type & type )
       && ( number_percent( ) <= atoi( mprg->arglist ) ) )
     {
       mprog_driver( mprg->comlist, mob, actor, obj, NULL, vo, NULL );
       if ( type != GREET_PROG && type != ALL_GREET_PROG )
	 break;
     }
}

void oprog_percent_check( OBJ_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj,
			 void *vo, int type)
{
 MPROG_DATA * mprg(0);

 for ( mprg = mob->pIndexData->objprogs; mprg != NULL; mprg = mprg->next )
   if ( ( mprg->type & type )
       && ( number_percent( ) <= atoi( mprg->arglist ) ) )
     {
       mprog_driver( mprg->comlist, NULL, actor, mob, NULL, vo, NULL );
       if ( type != GREET_PROG && type != ALL_GREET_PROG )
	 break;
     }

 return;

}

/* The triggers.. These are really basic, and since most appear only
 * once in the code (hmm. i think they all do) it would be more efficient
 * to substitute the code in and make the mprog_xxx_check routines global.
 * However, they are all here in one nice place at the moment to make it
 * easier to see what they look like. If you do substitute them back in,
 * make sure you remember to modify the variable names to the ones in the
 * trigger calls.
 */
void mprog_act_trigger( char *buf, CHAR_DATA *mob, CHAR_DATA *ch,
		       OBJ_DATA *obj, void *vo)
{
  if (IS_NPC(mob) && (mob->pIndexData->progtypes & ACT_PROG))
    mprog_wordlist_check(buf, mob, ch, NULL, NULL, ACT_PROG);

  return;

}

void oprog_act_trigger( char *buf, OBJ_DATA *mob, CHAR_DATA *ch,
		       OBJ_DATA *obj, void *vo)
{
  CHAR_DATA *vmob;

  if (mob->pIndexData->progtypes & ACT_PROG)
  {
    if (mob->carried_by == NULL)
    for ( vmob = mob->in_room->people; vmob != NULL; vmob = vmob->next_in_room )
        {
          oprog_wordlist_check( buf, mob, vmob, NULL, NULL, ACT_PROG );
          break;
        }
    else
    for ( vmob = mob->carried_by; vmob != NULL; vmob = vmob->next_in_room)
        {
          oprog_wordlist_check( buf, mob, vmob, NULL, NULL, ACT_PROG );
          break;
        }
  }

  return;
}

void mprog_bribe_trigger( CHAR_DATA *mob, CHAR_DATA *ch, int amount )
{

  MPROG_DATA *mprg;

  if ( IS_NPC( mob )
      && ( mob->pIndexData->progtypes & BRIBE_PROG ) )
    {

      for ( mprg = mob->pIndexData->mobprogs; mprg != NULL; mprg = mprg->next )
	if ( ( mprg->type & BRIBE_PROG )
	    && ( amount >= atoi( mprg->arglist ) ) )
	  {
	    mprog_driver( mprg->comlist, mob, ch, NULL, NULL, NULL, NULL );
	    break;
	  }
    }
  
  return;

}

void mprog_attack_trigger( CHAR_DATA *mob, CHAR_DATA *killer, CHAR_DATA *victim)
{
    if (IS_SET(mob->pIndexData->progtypes, ATTACK_PROG))
        mprog_percent_check(mob, killer, NULL, (void *) victim, ATTACK_PROG);

    return;
}	    

void rprog_attack_trigger( ROOM_INDEX_DATA *room, CHAR_DATA *killer, CHAR_DATA *victim)
{
    if (IS_SET(room->progtypes, ATTACK_PROG))
        rprog_percent_check(room, killer, NULL, (void *) victim, ATTACK_PROG);

    return;
}	    

void oprog_all_death_trigger( OBJ_DATA *obj, CHAR_DATA *killer, CHAR_DATA *victim)
{
    if (IS_SET(obj->pIndexData->progtypes, ALL_DEATH_PROG))
	oprog_percent_check(obj, killer, NULL, victim, ALL_DEATH_PROG);

    return;
}

void rprog_all_death_trigger( ROOM_INDEX_DATA *room, CHAR_DATA *killer, CHAR_DATA *victim )
{
    if (room->progtypes & ALL_DEATH_PROG)
	rprog_percent_check( room, killer, NULL, victim, ALL_DEATH_PROG );

    return;
} 

void mprog_all_death_trigger( CHAR_DATA *mob, CHAR_DATA *killer, CHAR_DATA *victim )
{
    CHAR_DATA *vmob, *vmob_next;

	if (mob == NULL || mob->in_room == NULL)
		return;

    for (vmob = mob->in_room->people; vmob; vmob = vmob_next)
    {
	vmob_next = vmob->next_in_room;

	if (IS_NPC(vmob) && (vmob->pIndexData->progtypes & ALL_DEATH_PROG))
	    mprog_percent_check(vmob, killer, NULL, victim, ALL_DEATH_PROG);
    }

    return;
}

void mprog_death_trigger( CHAR_DATA *mob, CHAR_DATA *killer )
{
 if ( IS_NPC( mob )
     && ( mob->pIndexData->progtypes & DEATH_PROG ) )
   {
     mprog_percent_check( mob, killer, NULL, NULL, DEATH_PROG );
   }
 
 return;

}

void mprog_demon_trigger( CHAR_DATA *mob )
{
  if (IS_NPC(mob)
	&& (mob->pIndexData->progtypes & DEMON_PROG ) )
     mprog_percent_check(mob, NULL, NULL, NULL, DEMON_PROG );

return;
} 

void mprog_entry_trigger( CHAR_DATA *mob )
{

 if ( IS_NPC( mob )
     && ( mob->pIndexData->progtypes & ENTRY_PROG ) )
   mprog_percent_check( mob, NULL, NULL, NULL, ENTRY_PROG );

 return;
}

void rprog_time_trigger( ROOM_INDEX_DATA *room )
{
    MPROG_DATA *mprg;

    if (room->progtypes & TIME_PROG)
    {
	for (mprg = room->mobprogs; mprg; mprg = mprg->next)
	{
	    if ((mprg->type & TIME_PROG) && !time_info.half
	     && (atoi(mprg->arglist) == time_info.hour))
		mprog_driver(mprg->comlist, NULL, NULL, NULL, room, NULL, NULL);
	}
    }

    return;
}

void oprog_time_trigger( OBJ_DATA *obj )
{

 char        buf[MAX_INPUT_LENGTH];
 MPROG_DATA *oprg(0);
 int time;

  if ( obj->pIndexData->progtypes & TIME_PROG )
   for ( oprg = obj->pIndexData->objprogs; oprg != NULL; oprg = oprg->next )
     {
       one_argument( oprg->arglist, buf );
       if ( ( oprg->type & TIME_PROG )
	  && ((time = atoi(oprg->arglist)) == time_info.hour)
	  && !time_info.half)
	 {
	   mprog_driver( oprg->comlist, NULL, NULL, obj, NULL, NULL, NULL );
	   break;
	 }
     }

 return;
}


void mprog_time_trigger( CHAR_DATA *mob )
{

 char        buf[MAX_INPUT_LENGTH];
 MPROG_DATA *mprg;
 int time;

 if ( IS_NPC( mob )
     && ( mob->pIndexData->progtypes & TIME_PROG ) )
   for ( mprg = mob->pIndexData->mobprogs; mprg != NULL; mprg = mprg->next )
     {
       one_argument( mprg->arglist, buf );
       if ( ( mprg->type & TIME_PROG )
	  && ((time = atoi(mprg->arglist)) == time_info.hour)
	  && !time_info.half )
	 {
	   mprog_driver( mprg->comlist, mob, NULL, NULL, NULL, NULL, NULL );
	   break;
	 }
     }

 return;
}

void mprog_load_trigger( CHAR_DATA *mob )
{
 if (mob->pIndexData->progtypes & LOAD_PROG )
   mprog_percent_check( mob, NULL, NULL, NULL, LOAD_PROG );
}

void oprog_load_trigger( OBJ_DATA *obj )
{
 if (obj->pIndexData->progtypes & LOAD_PROG )
   oprog_percent_check( obj, NULL, NULL, NULL, LOAD_PROG );
}

void oprog_entry_trigger( OBJ_DATA *mob )
{
 if (mob->pIndexData->progtypes & ENTRY_PROG )
   oprog_percent_check( mob, NULL, NULL, NULL, ENTRY_PROG );
}

void oprog_death_trigger( OBJ_DATA *obj, CHAR_DATA *victim )
{
    if (obj->pIndexData->progtypes & DEATH_PROG)
	oprog_percent_check(obj, victim, NULL, NULL, DEATH_PROG);
}

void mprog_fight_trigger( CHAR_DATA *mob, CHAR_DATA *ch )
{

 if ( IS_NPC( mob )
     && ( mob->pIndexData->progtypes & FIGHT_PROG ) )
   mprog_percent_check( mob, ch, NULL, NULL, FIGHT_PROG );
}

void oprog_fight_trigger( OBJ_DATA *mob, CHAR_DATA *ch )
{
 if ( mob->pIndexData->progtypes & FIGHT_PROG )
   oprog_percent_check( mob, ch, NULL, NULL, FIGHT_PROG );

 return;

}

int mprog_hit_trigger( CHAR_DATA *mob, CHAR_DATA *ch, int dam )
{
    hitprog_damage = dam;

    if (IS_NPC(mob) && IS_SET(mob->pIndexData->progtypes, HIT_PROG))
	mprog_percent_check(mob, ch, NULL, NULL, HIT_PROG);
   
    return hitprog_damage;
}

int oprog_hit_trigger( OBJ_DATA *obj, CHAR_DATA *ch, int dam )
{
    hitprog_damage = dam;

    if (IS_SET(obj->pIndexData->progtypes, HIT_PROG))
	oprog_percent_check(obj, ch, NULL, NULL, HIT_PROG);

    return hitprog_damage;
}

void mprog_take_trigger( CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj )
{

 char        buf[MAX_INPUT_LENGTH];
 MPROG_DATA *mprg;

 if ( IS_NPC( mob )
     && ( mob->pIndexData->progtypes & TAKE_PROG ) )
   for ( mprg = mob->pIndexData->mobprogs; mprg != NULL; mprg = mprg->next )
     {
       one_argument( mprg->arglist, buf );
       if ( ( mprg->type & TAKE_PROG )
	   && ( ( !str_cmp( obj->name, mprg->arglist ) )
	       || ( !str_cmp( "all", buf ) ) ) )
	 {
	   mprog_driver( mprg->comlist, mob, ch, obj, NULL, NULL, NULL );
	   break;
	 }
     }

 return;
}


bool mprog_give_trigger(CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj)
{
    char        buf[MAX_INPUT_LENGTH];
    MPROG_DATA *mprg;

    if (IS_NPC(mob) && (mob->pIndexData->progtypes & GIVE_PROG))
	for (mprg = mob->pIndexData->mobprogs; mprg; mprg = mprg->next)
	{
	    if (mprg->type & GIVE_PROG)
	    {
		one_argument( mprg->arglist, buf );
		if (!str_cmp(obj->name, mprg->arglist)
	         || !str_cmp("all", buf))
		{
		    mprog_driver( mprg->comlist, mob, ch, obj, NULL, NULL, NULL );
		    return TRUE;
		}
	    }
	}

    return FALSE;
}


void oprog_take_trigger( OBJ_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj )
{

 char        buf[MAX_INPUT_LENGTH];
 MPROG_DATA *mprg(0);

 if ( mob->pIndexData->progtypes & TAKE_PROG )
   for ( mprg = mob->pIndexData->objprogs; mprg != NULL; mprg = mprg->next)
     {
       one_argument( mprg->arglist, buf );
       if ( ( mprg->type & TAKE_PROG )
	   && ( ( !str_cmp( mob->name, mprg->arglist ) )
	       || ( !str_cmp( "all", buf ) ) ) )
	 {
	   mprog_driver( mprg->comlist, NULL, ch, mob, NULL, NULL, NULL );
	   break;
	 }
     }

 return;
}


void oprog_give_trigger( OBJ_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj )
{

 char        buf[MAX_INPUT_LENGTH];
 MPROG_DATA *mprg(0);

 if ( mob->pIndexData->progtypes & GIVE_PROG )
   for ( mprg = mob->pIndexData->objprogs; mprg != NULL; mprg = mprg->next)
     {
       one_argument( mprg->arglist, buf );
       if ( ( mprg->type & GIVE_PROG )
	   && ( ( !str_cmp( mob->name, mprg->arglist ) )
	       || ( !str_cmp( "all", buf ) ) ) )
	 {
	   mprog_driver( mprg->comlist, NULL, ch, mob, NULL, NULL, NULL );
	   break;
	 }
     }

 return;

}

void mprog_hail_trigger( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if (victim->pIndexData->progtypes & HAIL_PROG)
	mprog_percent_check(victim, ch, NULL, NULL, HAIL_PROG);

    return;
}

void rprog_greet_trigger( ROOM_INDEX_DATA *room, CHAR_DATA *ch )
{
    if (room->progtypes & GREET_PROG)
	rprog_percent_check(room, ch, NULL, NULL, GREET_PROG);

    if (room->progtypes & ALL_GREET_PROG)
	rprog_percent_check(room, ch, NULL, NULL, ALL_GREET_PROG);

    return;
}

void mprog_greet_trigger( CHAR_DATA *ch )
{
    CHAR_DATA *vmob, *vmob_next;

    if (!ch->in_room)
	return;
   
    for ( vmob = ch->in_room->people; vmob != NULL; vmob = vmob_next )
    {
	vmob_next = vmob->next_in_room;
 	if (!vmob->valid)
	    return;

	if ( IS_NPC( vmob )
         && ch != vmob
         && ch->in_room == vmob->in_room
         && can_see( vmob, ch )
         && ( vmob->fighting == NULL || vmob->clan != 0)
         && IS_AWAKE( vmob )
         && ( vmob->pIndexData->progtypes & GREET_PROG) )
	    mprog_percent_check( vmob, ch, NULL, NULL, GREET_PROG );
	else if ( IS_NPC( vmob )
	 && ( vmob->fighting == NULL || vmob->clan != 0)
         && ch->in_room == vmob->in_room
	 && IS_AWAKE( vmob )
	 && ( vmob->pIndexData->progtypes & ALL_GREET_PROG ) )
	    mprog_percent_check(vmob,ch,NULL,NULL,ALL_GREET_PROG);
    }

    return;
}

void oprog_greet_trigger( OBJ_DATA* mob, CHAR_DATA * target )
{
 if (mob->pIndexData->progtypes & GREET_PROG)
     oprog_percent_check( mob, target, NULL, NULL, GREET_PROG );
 else if (mob->pIndexData->progtypes & ALL_GREET_PROG)
     oprog_percent_check(mob, target,NULL,NULL,ALL_GREET_PROG);
}

void mprog_hitprcnt_trigger( CHAR_DATA *mob, CHAR_DATA *ch)
{

 MPROG_DATA *mprg;

 if ( IS_NPC( mob )
     && ( mob->pIndexData->progtypes & HITPRCNT_PROG ) )
   for ( mprg = mob->pIndexData->mobprogs; mprg != NULL; mprg = mprg->next )
     if ( ( mprg->type & HITPRCNT_PROG )
	 && ( ( 100*mob->hit / mob->max_hit ) < atoi( mprg->arglist ) ) )
       {
	 mprog_driver( mprg->comlist, mob, ch, NULL, NULL, NULL, NULL );
	 break;
       }
 
 return;

}

void oprog_hitprcnt_trigger( OBJ_DATA *mob, CHAR_DATA *ch)
{

 MPROG_DATA *mprg(0);

 if (mob->carried_by == NULL) return;

 if ( mob->pIndexData->progtypes & HITPRCNT_PROG )
   for ( mprg = mob->pIndexData->objprogs; mprg != NULL; mprg = mprg->next)
     if ( ( mprg->type & HITPRCNT_PROG )
	 && ( ( 100*mob->carried_by->hit / mob->carried_by->max_hit ) < atoi( mprg->arglist ) ) )
       {
	 mprog_driver( mprg->comlist, NULL, ch, mob, NULL, NULL, NULL );
	 break;
       }
 
 return;

}

void rprog_tick_trigger(ROOM_INDEX_DATA * room)
{
	// Check tick progs
	if (room->progtypes & TICK_PROG)
		rprog_percent_check(room, NULL, NULL, NULL, TICK_PROG);

	// Check hour progs
	if (!time_info.half && (room->progtypes & HOUR_PROG))
		rprog_percent_check(room, NULL, NULL, NULL, HOUR_PROG);
}

void mprog_tick_trigger(CHAR_DATA * mob)
{
	assert(mob->pIndexData);

	// Check tick progs
	if (mob->pIndexData->progtypes & TICK_PROG)
		mprog_percent_check(mob, NULL, NULL, NULL, TICK_PROG);

	// Check hour progs
	if (!time_info.half && (mob->pIndexData->progtypes & HOUR_PROG))
		mprog_percent_check(mob, NULL, NULL, NULL, HOUR_PROG);
}

void oprog_tick_trigger(OBJ_DATA * obj)
{
	assert(obj->pIndexData);

	// Check tick progs
	if (obj->pIndexData->progtypes & TICK_PROG)
		oprog_percent_check(obj, NULL, NULL, NULL, TICK_PROG);

	// Check hour progs
	if (!time_info.half && (obj->pIndexData->progtypes & HOUR_PROG))
		oprog_percent_check(obj, NULL, NULL, NULL, HOUR_PROG);
}

void rprog_random_trigger( ROOM_INDEX_DATA *room )
{
    if (room->progtypes & RAND_PROG)
	    rprog_percent_check(room, NULL, NULL, NULL, RAND_PROG);
}

void mprog_random_trigger( CHAR_DATA *mob )
{
    if ( mob->pIndexData->progtypes & RAND_PROG)
        mprog_percent_check(mob,NULL,NULL,NULL,RAND_PROG);
}

void oprog_random_trigger( OBJ_DATA *mob )
{
    if ( mob->pIndexData->progtypes & RAND_PROG)
        oprog_percent_check(mob,NULL,NULL,NULL,RAND_PROG);
}

bool is_verb_triggered(const MPROG_DATA & mprog, const char * command, const char * subbed_command, const char * text, const char * subbed_text)
{
	// Verify this is a verb prog
	if (mprog.type & VERB_PROG)
	{
		// Base determination off whether an exact match is required
		if (mprog.exact_match)
		{
			// An exact match implies a p or P; seek past it
			bool foundP(false);
			for (const char * start(mprog.arglist); *start != '\0'; ++start)
			{
				// If the p is found, mark it as found and continue
				if (!foundP && LOWER(*start) == 'p')
				{
					foundP = true;
					continue;
				}

				// If the p has already been found and this is not a space, it is the beginning of the verb
				if (foundP && *start != ' ')
				{
					// Return whether the remainder of the arglist is a prefix of the text
					return ((subbed_text != 0 && !str_prefix(start, subbed_text)) || !str_prefix(start, text));
				}
			}

			// The arglist is blank and so matches everything; p must be found by this time, or we've got a code bug
			assert(foundP);
			return true;
		}

		// Exact match not required; seek for the substring
		if (strstr(command, mprog.arglist) != 0)
			return true;

		// Finally, if a substituted command was found, check it for the substring
		return (subbed_command != 0 && strstr(subbed_command, mprog.arglist) != 0);
	}

	// Not a verb_prog, return false
	return false;
}

bool check_room_verbs(CHAR_DATA * ch, ROOM_INDEX_DATA * room, const char * command, const char * subbed_command, char * text, const char * subbed_text)
{
	// Iterate all of the progs for the room
	bool anyFound(false);
	for (MPROG_DATA * iter(room->mobprogs); iter != NULL && !verb_stop_issued; iter = iter->next)
	{
		// Check the verb, and if matched, call the driver
		if (is_verb_triggered(*iter, command, subbed_command, text, subbed_text))
		{
			if (!mprog_driver(iter->comlist, NULL, ch, NULL, room, text, text))
				anyFound = true;
		}
	}

	// Return whether any progs were found
	return anyFound;
}

bool check_object_verbs(CHAR_DATA * ch, OBJ_DATA * obj, const char * command, const char * subbed_command, char * text, const char * subbed_text)
{
	// Make sure the obj has an index data
	if (obj->pIndexData == NULL)
		return false;

	// Iterate all of the progs for the object
	bool anyFound(false);
	for (MPROG_DATA * iter(obj->pIndexData->objprogs); iter != NULL && !verb_stop_issued; iter = iter->next)
	{
		// Check the verb, and if matched, call the driver
		if (is_verb_triggered(*iter, command, subbed_command, text, subbed_text))
		{
			if (!mprog_driver(iter->comlist, NULL, ch, obj, NULL, text, text))
				anyFound = true;
		}
	}

	// Return whether any progs were found
	return anyFound;
}

bool check_object_list_verbs(CHAR_DATA * ch, OBJ_DATA * firstObj, const char * command, const char * subbed_command, char * text, const char * subbed_text)
{
    // Iterate the local object list
	bool anyFound(false);
	for (OBJ_DATA * iter(firstObj); iter != NULL && !verb_stop_issued; iter = iter->next_content)
	{
		// Check each object in turn
		if (check_object_verbs(ch, iter, command, subbed_command, text, subbed_text))
			anyFound = true;
	}
	
	// Return whether any progs were found
	return anyFound;
}

bool check_character_verbs(CHAR_DATA * ch, CHAR_DATA * mob, const char * command, const char * subbed_command, char * text, const char * subbed_text)
{
	bool anyFound(false);
	
	// Start by checking progs on what the character is carrying (or wearing)
	if (check_object_list_verbs(ch, mob->carrying, command, subbed_command, text, subbed_text))
		anyFound = true;

	// Make sure this 'mob' (which at this point could still be a PC) is a valid character for progs
	if (!verb_stop_issued && IS_NPC(mob) && mob->pIndexData != NULL && !IS_SET(mob->act, ACT_ILLUSION))
	{
		// Iterate all of the progs for the mob
		for (MPROG_DATA * iter(mob->pIndexData->mobprogs); iter != NULL && !verb_stop_issued; iter = iter->next)
		{
			// Check the verb, and if matched, call the driver
			if (is_verb_triggered(*iter, command, subbed_command, text, subbed_text))
			{
		 		if (!mprog_driver(iter->comlist, mob, ch, NULL, NULL, text, text))
				 	anyFound = true;
			}
		}
	}

	// Return whether any progs were found
	return anyFound;
}

bool check_character_list_verbs(CHAR_DATA * ch, CHAR_DATA * firstChar, const char * command, const char * subbed_command, char * text, const char * subbed_text)
{
	// Iterate the local character list
	bool anyFound(false);
	for (CHAR_DATA * iter(firstChar); iter != NULL && !verb_stop_issued; iter = iter->next_in_room)
	{
		// Check each character's progs; this check the character's object progs, too
		if (check_character_verbs(ch, iter, command, subbed_command, text, subbed_text))
			anyFound = true;
	}

	// Return whether any progs were found
	return anyFound;
}

class RecursionCounter
{
	static const size_t MaxCalls = 50; 
	static size_t NumCalls;

	public:

		RecursionCounter() {++NumCalls;}
		~RecursionCounter() {assert(NumCalls > 0); --NumCalls;}

		static bool ReachedMax() {return (NumCalls >= MaxCalls);}

	private:
		RecursionCounter(const RecursionCounter &);
		void operator=(const RecursionCounter &);
};

size_t RecursionCounter::NumCalls(0);

bool check_verbs(CHAR_DATA * ch, const char * command, const char * subbed_command, char * text, const char * subbed_text)
{
	// Sanity checks; the subbed vars can be null
	assert(ch != 0);
	assert(command != 0);
	assert(text != 0);

	// Prevent verb progs from triggering each other in an infinite loop
	RecursionCounter counter;
	if (RecursionCounter::ReachedMax())
	{
		std::ostringstream mess;
		mess << "Exceeded max verb recursion count (" << ch->name;
		mess << ") (Verb: '" << text << "')";
		bug(mess.str().c_str(), 0);
		return false;
	}

	// No verb progs if not in a room
	if (ch->in_room == NULL)
		return false;

	// Initialize values
	bool anyFound(false);
	verb_stop_issued = false;

	// Check the room verbs
	if (check_room_verbs(ch, ch->in_room, command, subbed_command, text, subbed_text))
		anyFound = true;

	// Check the verbs for all objects in the room
	if (!verb_stop_issued && check_object_list_verbs(ch, ch->in_room->contents, command, subbed_command, text, subbed_text))
		anyFound = true;

	// Check the verbs for all characters in the room; this in turn checks for all objects on each character
	if (!verb_stop_issued && check_character_list_verbs(ch, ch->in_room->people, command, subbed_command, text, subbed_text))
		anyFound = true;

	// Return whether any verbs were found
	return anyFound;
}

void rprog_speech_trigger( char *txt, ROOM_INDEX_DATA *room, CHAR_DATA *ch )
{
    if (room->progtypes & SPEECH_PROG)
	rprog_wordlist_check(txt, room, ch, txt, SPEECH_PROG);
  
    return;
}

void mprog_speech_trigger( LANG_STRING *txt, CHAR_DATA *mob )
{
    CHAR_DATA *vmob, *vmob_next;

    for ( vmob = mob->in_room->people; vmob != NULL; vmob = vmob_next )
    {
        vmob_next = vmob->next_in_room;
        if ( IS_NPC( vmob ) && (vmob->pIndexData->progtypes & SPEECH_PROG) && (vmob != mob))
            mprog_wordlist_check( translate_out_new(vmob, txt), vmob, mob, NULL, txt->orig_string, SPEECH_PROG );
    }
  
    return;
}


void oprog_speech_trigger( char *txt, OBJ_DATA *mob )
{
  CHAR_DATA *vmob;
  if (mob->pIndexData->progtypes & SPEECH_PROG)
  {
    if (mob->carried_by == NULL)
    for ( vmob = mob->in_room->people; vmob != NULL; vmob = vmob->next_in_room )
        {
          oprog_wordlist_check( txt, mob, vmob, NULL, NULL, SPEECH_PROG );
          break;
        }
    else
    for ( vmob = mob->carried_by; vmob != NULL; vmob = vmob->next_in_room)
        {
          oprog_wordlist_check( txt, mob, vmob, NULL, NULL, SPEECH_PROG );
          break;
        }
  }
  return;
}

void mprog_exit_trigger( CHAR_DATA *ch )
{
    CHAR_DATA *vmob, *vmob_next;

    if (!ch || !ch->in_room)
        return;

    for ( vmob = ch->in_room->people; vmob != NULL; vmob = vmob_next)
    {
        vmob_next = vmob->next_in_room;
        if ( IS_NPC( vmob )
        && ch != vmob
        && can_see( vmob, ch )
        && ( vmob->fighting == NULL )
        && IS_AWAKE( vmob )
        && ( vmob->pIndexData->progtypes & EXIT_PROG) )
            mprog_percent_check( vmob, ch, NULL, NULL, EXIT_PROG );
    }
}

void oprog_exit_trigger( OBJ_DATA* mob )
{

 CHAR_DATA *vmob;

 if (mob->carried_by != NULL) vmob = mob->carried_by;
 else
 if (mob->in_room != NULL) vmob = mob->in_room->people;
 else return;

 if (vmob == NULL) return;

 if (mob->pIndexData->progtypes & EXIT_PROG)
     oprog_percent_check( mob, vmob, NULL, NULL, EXIT_PROG );

 return;
}

void oprog_sac_trigger( OBJ_DATA *obj, CHAR_DATA *ch)
{
    if (obj->pIndexData->progtypes & SAC_PROG)
	oprog_percent_check(obj, ch, NULL, NULL, SAC_PROG);
    return;
}

void oprog_eat_trigger( CHAR_DATA* mob, OBJ_DATA* obj )
{
    if (obj->pIndexData->progtypes & EAT_PROG)
	oprog_percent_check(obj, mob, NULL, NULL, EAT_PROG);
    return;
}
void oprog_drink_trigger( CHAR_DATA* mob, OBJ_DATA* obj )
{
    if (obj->pIndexData->progtypes & DRINK_PROG)
	oprog_percent_check(obj, mob, NULL, NULL, DRINK_PROG);
    return;
}

void oprog_wear_trigger( OBJ_DATA *obj )
{

 CHAR_DATA *ch;

 if (obj->carried_by == NULL)
	{
	bug("wear trigger with NULL owner", 0);
	return;
	}

 ch = obj->carried_by;
 
 if (obj->pIndexData->progtypes & WEAR_PROG)
	oprog_percent_check( obj, ch, NULL, NULL, WEAR_PROG );
 
 return;
}

void oprog_remove_trigger( OBJ_DATA *obj )
{

 CHAR_DATA *ch;

 if (obj->carried_by == NULL)
	{
	bug("wear trigger with NULL owner", 0);
	return;
	}

 ch = obj->carried_by;
 
 if (obj->pIndexData->progtypes & REMOVE_PROG)
	oprog_percent_check( obj, ch, NULL, NULL, REMOVE_PROG );
 
 return;
}

