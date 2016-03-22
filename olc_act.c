/***************************************************************************
 *  File: olc_act.c                                                        *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/



#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iomanip>
#include <sstream>
#include <time.h>
#include <mysql/mysql.h>
#include <math.h>
#include "merc.h"
#include "olc.h"
#include "tables.h"
#include "recycle.h"
#include "lookup.h"
#include "db.h"
#include "interp.h"
#include "languages.h"
#include "LeyGroup.h"
#include "Shades.h"
#include "Faction.h"
#include "DisplayPanel.h"

char * mprog_type_to_name ( int type );
void   fix_exits	  args ( ( void ) );
void	olc_switch	args((DESCRIPTOR_DATA *d, void *newedit, int olc_type));

long	obj_default_price args((OBJ_INDEX_DATA *pObj));

extern MYSQL mysql;

/* Return TRUE if area changed, FALSE if not. */
#define REDIT( fun )		bool fun( CHAR_DATA *ch, char *argument )
#define OEDIT( fun )		bool fun( CHAR_DATA *ch, char *argument )
#define MEDIT( fun )		bool fun( CHAR_DATA *ch, char *argument )
#define AEDIT( fun )		bool fun( CHAR_DATA *ch, char *argument )
#define HEDIT( fun )		bool fun( CHAR_DATA *ch, char *argument )
#define FEDIT( fun )		bool fun( CHAR_DATA *ch, char *argument )

static void RenderPanel(const DisplayPanel::Component & component, CHAR_DATA * ch)
{
    DisplayPanel::Options options(DisplayPanel::Style_Default, 'W');
    send_to_char(DisplayPanel::Render(component, options, 80).c_str(), ch);
}

static void RenderPanelBuffered(const DisplayPanel::Component & component, CHAR_DATA * ch)
{
    DisplayPanel::Options options(DisplayPanel::Style_Default, 'W');
    page_to_char(DisplayPanel::Render(component, options, 80).c_str(), ch);
}

static void show_prog_to_char(CHAR_DATA * ch, const MPROG_DATA * prog)
{
    std::ostringstream mess;
    mess << '>' << mprog_type_to_name(prog->type) << ' ' << prog->arglist << '\n';
    send_to_char(mess.str().c_str(), ch);
    page_to_char(prog->comlist, ch);
}

static void do_prog_view(CHAR_DATA * ch, const MPROG_DATA * prog, char * progtype, const char * progarg)
{
    if (progtype[0] == '\0')
    {
        send_to_char("Syntax:  prog view <progtype> [progarg]\n", ch);
        return;
    }

    int ptype(mprog_name_to_type(progtype));
    const MPROG_DATA * candidate(NULL);
    bool foundDuplicate(false);
    while (prog != NULL)
    {
        // Check the type
        if (ptype == prog->type)
        {
            // Check the args
            if (!str_cmp(progarg, prog->arglist))
            {
                // Exact match
                show_prog_to_char(ch, prog);
                return;
            }

            // Type matches, so if this is the only one we'll use it
            if (candidate == NULL) candidate = prog;
            else foundDuplicate = true;
        }
        
        // Advance to the next
        prog = prog->next;
    }

    // No exact match, but check whether there is a unique prog of this type
    if (candidate != NULL && !foundDuplicate)
    {
        show_prog_to_char(ch, candidate);
        return;
    }

    send_to_char("Prog not found.\n", ch);
}

struct olc_help_type
{
    char *command;
    const void *structure;
    char *desc;
};

bool show_version( CHAR_DATA *ch, char *argument )
{
    send_to_char( VERSION, ch );
    send_to_char( "\n\r", ch );
    send_to_char( AUTHOR, ch );
    send_to_char( "\n\r", ch );
    send_to_char( DATE, ch );
    send_to_char( "\n\r", ch );
    send_to_char( CREDITS, ch );
    send_to_char( "\n\r", ch );

    return FALSE;
}    

/*
 * This table contains help commands and a brief description of each.
 * ------------------------------------------------------------------
 */
const struct olc_help_type help_table[] =
{
    /* Aedit Tables */

    {	"area",		area_flags,	 "Area attributes."		 },
    {   "geography",    geo_flags,	 "Geography types."		 },
    {   "windmag",	wind_mag_flags,	 "Wind magnitude values."	 },
    {   "temperature",  temp_flags,	 "Temperature values."		 },
    {   "precip",	precip_flags,	 "Precipitation values."	 },
    {   "herbs",	herb_table,	 "Herb type."			 },

    /* Redit Tables */

    {	"room",		room_flags,	 "Room attributes."		 },
    {	"sector",	sector_flags,	 "Sector types, terrain."	 },
    {	"exit",		exit_flags,	 "Exit types."			 },
    {	"type",		type_flags,	 "Types of objects."		 },
    {	"extra",	extra_flags,	 "Object attributes."		 },
    {	"wear",		wear_flags,	 "Where to wear object."	 },

    /* Oedit Tables */

    {	"container",	container_flags, "Container status."		 },
    {   "wclass",       weapon_class,    "Weapon class."                 }, 
    {   "wtype",        weapon_type2,    "Special weapon type."          },
    {	"portal",	portal_flags,	 "Portal types."		 },
    {   "light",    light_flags,    "Light types."          },
    {	"furniture",	furniture_flags, "Furniture types."		 },
    {   "liquid",	liq_table,	 "Liquid types."		 },
    {	"spec",		spec_table,	 "Available special programs." 	 },
    {	"material",	material_table,	 "Material types."		 },

    /* Reset Tables (?) */

    {	"wear-loc",	wear_loc_flags,	 "Where mobile wears object."	 },

    /* Medit Tables */

    {	"sex",		sex_flags,	 "Sexes."			 },
    {	"act",		act_flags,	 "Mobile attributes."		 },
    {	"affect",	affect_flags,	 "Mobile affects."		 },
    {	"spells",	skill_table,	 "Names of current spells." 	 },
    {	"armor",	ac_type,	 "Ac for different attacks."	 },
    {   "apply",	apply_flags,	 "Apply flags"			 },
    {	"form",		form_flags,	 "Mobile body form."	         },
    {	"part",		part_flags,	 "Mobile body parts."		 },
    {	"imm",		imm_flags,	 "Mobile immunity."		 },
    {	"resist",	res_flags,	 "Mobile resistance."	         },
    {	"off",		off_flags,	 "Mobile offensive behaviour."	 },
    {	"size",		size_flags,	 "Mobile size."			 },
    {   "position",     position_flags,  "Mobile positions."             },
    {	"languages",	lang_flags,	 "Languages."			 },

    {	NULL,		NULL,		 NULL				 }
};



/*****************************************************************************
 Name:		show_flag_cmds
 Purpose:	Displays settable flags and stats.
 Called by:	show_help(olc_act.c).
 ****************************************************************************/
void show_flag_cmds( CHAR_DATA *ch, const struct flag_type *flag_table )
{
    char buf  [ MAX_STRING_LENGTH ];
    char buf1 [ MAX_STRING_LENGTH ];
    int  flag;
    int  col;
 
    buf1[0] = '\0';
    col = 0;
    for (flag = 0; flag_table[flag].name != NULL; flag++)
    {
        if ( flag_table[flag].settable )
        {
            sprintf( buf, "%-19.18s", flag_table[flag].name );
            strcat( buf1, buf );
            if ( ++col % 4 == 0 )
                strcat( buf1, "\n\r" );
        }
    }
 
    if ( col % 4 != 0 )
    	strcat( buf1, "\n\r" );

    send_to_char( buf1, ch );
    return;
}


/*****************************************************************************
 Name:		show_skill_cmds
 Purpose:	Displays all skill functions.
 		Does remove those damn immortal commands from the list.
 		Could be improved by:
 		(1) Adding a check for a particular class_num.
 		(2) Adding a check for a level range.
 Called by:	show_help(olc_act.c).
 ****************************************************************************/
void show_skill_cmds( CHAR_DATA *ch, int tar )
{
    char buf  [ MAX_STRING_LENGTH ];
    char buf1 [ MAX_STRING_LENGTH*2 ];
    int  sn;
    int  col;
 
    buf1[0] = '\0';
    col = 0;
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
	if ( !skill_table[sn].name )
	    break;

	if ( !str_cmp( skill_table[sn].name, "reserved" )
	  || skill_table[sn].spell_fun == spell_null
	   || skill_table[sn].spell_fun == spell_form )
	    continue;

	if ( tar == -1 || skill_table[sn].target == tar )
	{
	    sprintf( buf, "%-19.18s", skill_table[sn].name );
	    strcat( buf1, buf );
	    if ( ++col % 4 == 0 )
		strcat( buf1, "\n\r" );
	}
    }
 
    if ( col % 4 != 0 )
	strcat( buf1, "\n\r" );

    send_to_char( buf1, ch );
    return;
}



/*****************************************************************************
 Name:		show_spec_cmds
 Purpose:	Displays settable special functions.
 Called by:	show_help(olc_act.c).
 ****************************************************************************/
void show_spec_cmds( CHAR_DATA *ch )
{
    char buf  [ MAX_STRING_LENGTH ];
    char buf1 [ MAX_STRING_LENGTH ];
    int  spec;
    int  col;
 
    buf1[0] = '\0';
    col = 0;
    send_to_char( "Preceed special functions with 'spec_'\n\r\n\r", ch );
    for (spec = 0; spec_table[spec].function != NULL; spec++)
    {
	sprintf( buf, "%-19.18s", &spec_table[spec].name[5] );
	strcat( buf1, buf );
	if ( ++col % 4 == 0 )
	    strcat( buf1, "\n\r" );
    }
 
    if ( col % 4 != 0 )
	strcat( buf1, "\n\r" );

    send_to_char( buf1, ch );
    return;
}



/*****************************************************************************
 Name:		show_help
 Purpose:	Displays help for many tables used in OLC.
 Called by:	olc interpreters.
 ****************************************************************************/
bool show_help( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char spell[MAX_INPUT_LENGTH];
    int cnt;

    argument = one_argument( argument, arg );
    one_argument( argument, spell );

    /*
     * Display syntax.
     */
    if ( arg[0] == '\0' )
    {
	send_to_char( "Syntax:  ? [command]\n\r\n\r", ch );
	send_to_char( "[command]  [description]\n\r", ch );
	for (cnt = 0; help_table[cnt].command != NULL; cnt++)
	{
	    sprintf( buf, "%-10.10s -%s\n\r",
	        capitalize( help_table[cnt].command ),
		help_table[cnt].desc );
	    send_to_char( buf, ch );
	}
	return FALSE;
    }

    /*
     * Find the command, show changeable data.
     * ---------------------------------------
     */
    for (cnt = 0; help_table[cnt].command != NULL; cnt++)
    {
        if (  arg[0] == help_table[cnt].command[0]
          && !str_prefix( arg, help_table[cnt].command ) )
	{
	    if ( help_table[cnt].structure == spec_table )
	    {
		show_spec_cmds( ch );
		return FALSE;
	    }
	    else if ( help_table[cnt].structure == liq_table )
	    {
	        show_liqlist( ch );
	        return FALSE;
	    }
	    else if ( help_table[cnt].structure == material_table )
	    {
		    show_matlist(ch);
		    return FALSE;
	    }
	    else if ( help_table[cnt].structure == damtype_table )
	    {
	        show_damlist( ch );
	        return FALSE;
	    }
	    else if (help_table[cnt].structure == herb_table )
	    {
		show_herblist( ch );
		return FALSE;
	    }
	    else if ( help_table[cnt].structure == skill_table )
	    {

		if ( spell[0] == '\0' )
		{
		    send_to_char( "Syntax:  ? spells "
		        "[ignore/attack/defend/self/object/all]\n\r", ch );
		    return FALSE;
		}

		if ( !str_prefix( spell, "all" ) )
		    show_skill_cmds( ch, -1 );
		else if ( !str_prefix( spell, "ignore" ) )
		    show_skill_cmds( ch, TAR_IGNORE );
		else if ( !str_prefix( spell, "attack" ) )
		    show_skill_cmds( ch, TAR_CHAR_OFFENSIVE );
		else if ( !str_prefix( spell, "defend" ) )
		    show_skill_cmds( ch, TAR_CHAR_DEFENSIVE );
		else if ( !str_prefix( spell, "self" ) )
		    show_skill_cmds( ch, TAR_CHAR_SELF );
		else if ( !str_prefix( spell, "object" ) )
		    show_skill_cmds( ch, TAR_OBJ_INV );
		else
		    send_to_char( "Syntax:  ? spell "
		        "[ignore/attack/defend/self/object/all]\n\r", ch );
		    
		return FALSE;
	    }
	    else if (help_table[cnt].structure == act_flags)
	    {
		show_flag_cmds(ch, static_cast<const flag_type*> (help_table[cnt].structure));
		show_flag_cmds(ch, nact_flags);
		return FALSE;
	    }
        else if (help_table[cnt].structure == extra_flags)
        {
    		show_flag_cmds(ch, extra_flags[0]);
    		show_flag_cmds(ch, extra_flags[1]);
    		show_flag_cmds(ch, extra_flags[2]);
            return false;
        }
	    else
	    {
    		show_flag_cmds( ch, static_cast<const flag_type*> (help_table[cnt].structure));
	    	return FALSE;
	    }
	}
    }

    show_help( ch, "" );
    return FALSE;
}

DO_FUNC( do_flist )
{
    unsigned int i, fnum;
    int j, nMatch = 0;
    BUFFER *outbuf;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *mob;
    bool found = FALSE;
    CHAR_DATA *victim;

    if (FactionTable::Instance().Count() == 0)
    {
        send_to_char("No factions currently exist.\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] != '\0')
    {
        if (is_number(arg))
        {
            fnum = atoi(arg);

            if (fnum <= 0 || static_cast<unsigned int>(fnum) >= FactionTable::Instance().Count())
            {
                send_to_char("Faction does not exist.\n\r", ch);
                return;
            }

            sprintf(buf, "Mobiles assigned to faction '%s (%d)':\n\r", FactionTable::Instance()[fnum].Name().c_str(), fnum);
            send_to_char(buf, ch);

            for (j = 0; nMatch < top_mob_index; j++)
            {
                if (mob = get_mob_index(j))
                {
                    nMatch++;

                    if (mob->factionNumber == fnum)
                    {
                        sprintf(buf, "[%5d] %s\n\r", mob->vnum, mob->short_descr);
                        send_to_char(buf, ch);
                        found = TRUE;
                    }
                }
            }

            if (!found)
                send_to_char("none.\n\r", ch);

            return;
        }
        
        if (victim = get_char_world(ch, arg))
        {
            if (IS_NPC(victim))
            {
                send_to_char("Only PCs have faction values.\n\r", ch);
                return;
            }

            if (argument[0] != '\0')
            {
                if (is_number(argument))
                {
                    i = atoi(argument);

                    if (i < 0 || static_cast<unsigned int>(i) >= FactionTable::Instance().Count())
                    {
                        send_to_char("Invalid faction number.\n\r", ch);
                        return;
                    }

                    std::ostringstream mess;
                    mess << victim->name << " is ";
                    switch (FactionTable::CurrentStanding(*victim, i))
                    {
                        case Rating_Friend: mess << "a {Gfriend{x of"; break;
                        case Rating_Enemy: mess << "an {Renemy{x of"; break;
                        default: mess << "neutral towards"; break;
                    }
                    mess << " '" << FactionTable::Instance()[i].Name() << " (" << i << ")'. (Rating = ";
                    mess << victim->pcdata->faction_standing->Value(*victim, i) << ")\n";
                    send_to_char(mess.str().c_str(), ch);
                    return;
                }

                send_to_char("Usage: flist <character> <faction number>\n\r", ch);
                return;
            }

            outbuf = new_buf();

            send_to_char("[ FNum] Faction Name                             Rating Status\n\r", ch);
            send_to_char("==============================================================\n\r", ch);
            
            for (size_t index(0); index < FactionTable::Instance().Count(); ++index)
            {
                std::ostringstream mess;
                mess << "[" << std::setw(5) << index << "] " << std::setw(40) << FactionTable::Instance()[index].Name() << " ";
                mess << std::setw(6) << victim->pcdata->faction_standing->Value(*victim, index) << " ";

                switch (FactionTable::CurrentStanding(*victim, index))
                {
                    case Rating_Friend: mess << "{GFriend{x"; break;
                    case Rating_Enemy: mess << "{REnemy{x"; break;
                    default: mess << "Neutral"; break;
                }
                mess << '\n';
                add_buf(outbuf, mess.str().c_str());
            }

            page_to_char(buf_string(outbuf), ch);
            free_buf(outbuf);
            return;
        }
	    send_to_char("Usage: flist [faction number]\n\r", ch);
        return;
    }
	
    send_to_char("[FNum ] Faction Name\n\r", ch);
    send_to_char("============================================================================\n\r", ch);

    outbuf = new_buf();

    for (size_t index(0); index < FactionTable::Instance().Count(); ++index)
    {
        std::ostringstream mess;
        mess << "[" << std::setw(5) << index << "] " << FactionTable::Instance()[index].Name() << '\n';
	    add_buf(outbuf, mess.str().c_str());
    }

    page_to_char(buf_string(outbuf), ch);
    free_buf(outbuf);
}

void AddToList(BUFFER * buf, int numCols, int vnum, const char * name)
{
	std::ostringstream mess;
	mess << std::setfill(' ');
	mess << "[" << std::setw(5) << std::right << vnum;
	mess << "] " << std::left;

	// Write until the null terminator or colWidth
	size_t colWidth(UMAX(1, (80 / numCols) - 8));
	size_t len(0);
	while (len < colWidth && name[len] != '\0')
		++len;
	mess.write(name, len);

	// Make up the difference with spaces
	while (len < colWidth)
	{
		mess << ' ';
		++len;
	}

	// Add to the list
	add_buf(buf, mess.str().c_str());
}

DO_FUNC(do_rlist)
{
    ROOM_INDEX_DATA	*pRoomIndex;
    AREA_DATA		*pArea;
    BUFFER		*buf1;
    char arg[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    bool found,ffree,fAll;
    int vnum;
    int col = 0;
    int numCols = 3;
    VNUM_RANGE		*vRange;

    argument = one_argument(argument, arg);

    if ( arg[0] == '\0' )
    {
	send_to_char( "Syntax: rlist <all/free> [columns]\n\r", ch );
	return;
    }
    fAll = !str_cmp( arg, "all" );
    ffree = !str_cmp(arg, "free");

	// Get the number of columns requested
    one_argument(argument, arg2);
    numCols = atoi(arg2);
    if (arg2[0] == '\0' && ffree)
	numCols = 7;
    if (numCols == 0)
	numCols = 3;
    
    pArea = ch->in_room->area;
    buf1 = new_buf();
    found = FALSE;

    if (ffree)
    {
	// parse text parameter
	
	for (vRange = pArea->vnums; vRange; vRange = vRange->next)
	{
            for ( vnum = vRange->min_vnum; vnum <= vRange->max_vnum; vnum++ )
	    {
	        if (!( pRoomIndex = get_room_index( vnum ) ) )
		{
		    found = TRUE;
		    AddToList(buf1, numCols, vnum, "");
		    if (++col % numCols == 0)
			add_buf(buf1, "\n\r");
		    else
			add_buf(buf1, " ");
	    	}
	    }
	}	
    }
    else
    {
	// Get the number of columns requested

	for (vRange = pArea->vnums; vRange; vRange = vRange->next)
	{
            for ( vnum = vRange->min_vnum; vnum <= vRange->max_vnum; vnum++ )
	    {
	    	if ( ( pRoomIndex = get_room_index( vnum ) ) )
		{
		    found = TRUE;
		    AddToList(buf1, numCols, vnum, pRoomIndex->name);
		    if (++col % numCols == 0)
			add_buf(buf1, "\n\r");
		    else
			add_buf(buf1, " ");
	    	}
	    }
	}
    }

    if ( !found )
    {
	send_to_char( "Room(s) not found in this area.\n\r", ch);
	return;
    }

    if (col % numCols != 0)
		add_buf( buf1, "\n\r" );

    page_to_char( buf_string(buf1), ch );
    free_buf(buf1);
    return;
}

DO_FUNC(do_mlist)
{
    MOB_INDEX_DATA	*pMobIndex;
    AREA_DATA		*pArea;
    BUFFER		*buf1;
    char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
    bool fAll, found, ffree=FALSE;
    int vnum;
    int col = 0;
    VNUM_RANGE		*vRange;

    argument = one_argument(argument, arg);

    if ( arg[0] == '\0' )
    {
	send_to_char( "Syntax: mlist <all/name/free> [columns]\n\r", ch );
	return;
    }
    fAll = !str_cmp( arg, "all" );
    ffree = !str_cmp(arg, "free");

	// Get the number of columns requested
    one_argument(argument, arg2);
    int numCols = atoi(arg2);
    if (arg2[0] == '\0' && ffree)
	numCols = 7;
    if (numCols == 0)
	numCols = 3;

    buf1 = new_buf();
    pArea = ch->in_room->area;
    found = FALSE;

    if (ffree)
    {
	for (vRange = pArea->vnums; vRange; vRange = vRange->next)
            for ( vnum = vRange->min_vnum; vnum <= vRange->max_vnum; vnum++ )
	    	if (((pMobIndex = get_mob_index(vnum)) == NULL))
	    	{
	            found = TRUE;
		    AddToList(buf1, numCols, vnum, "");

		    if (++col % numCols == 0)
		    	add_buf(buf1, "\n\r");
		    else
		    	add_buf(buf1, " ");
	        }
    }
    else
    {
	for (vRange = pArea->vnums; vRange; vRange = vRange->next)
            for ( vnum = vRange->min_vnum; vnum <= vRange->max_vnum; vnum++ )
	    	if (((pMobIndex = get_mob_index(vnum)) != NULL)
	     	  && (fAll || is_name( arg, pMobIndex->player_name)))
	    	{
	            found = TRUE;
		    AddToList(buf1, numCols, pMobIndex->vnum, capitalize(pMobIndex->short_descr));

		    if (++col % numCols == 0)
		    	add_buf(buf1, "\n\r");
		    else
		    	add_buf(buf1, " ");
	        }
    }

    if ( !found )
    	if (ffree)
	{
	    send_to_char("No free mobile vnums found in this area.\n\r",ch);
	    return;
	}
	else
	{
	    send_to_char( "Mobile(s) not found in this area.\n\r", ch);
	    return;
    	}

    if ( col % numCols != 0 )
	add_buf( buf1, "\n\r" );

    page_to_char( buf_string(buf1), ch );
    free_buf(buf1);
    return;
}

DO_FUNC(do_olist)
{
    OBJ_INDEX_DATA	*pObjIndex;
    AREA_DATA		*pArea;
    BUFFER		*buf1;
    char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
    bool fAll, found,ffree;
    int vnum;
    int  col = 0;
    VNUM_RANGE		*vRange;

    argument = one_argument(argument, arg);
    if ( arg[0] == '\0' )
    {
		send_to_char("Syntax:  olist <all/name/item_type/free> [columns]\n\r", ch);
		return;
    }
    fAll = !str_cmp( arg, "all" );
    ffree = !str_cmp(arg, "free");

	// Get the number of columns requested
    one_argument(argument, arg2);
    int numCols = atoi(arg2);
    if (arg2[0] == '\0' && ffree)
	numCols = 7;
    if (numCols == 0)
	numCols = 3;

    pArea = ch->in_room->area;
    buf1 = new_buf();
    found = FALSE;

    if (ffree)
    {
	for (vRange = pArea->vnums; vRange; vRange = vRange->next)
            for ( vnum = vRange->min_vnum; vnum <= vRange->max_vnum; vnum++ )
	    	if (!(pObjIndex = get_obj_index(vnum)))
	    	{
		    found = TRUE;
		    AddToList(buf1, numCols, vnum, "");
		    if ( ++col % numCols == 0 )
		    	add_buf( buf1, "\n\r" );
		    else
			add_buf(buf1, " ");
	    	}
    }
    else
    {
	for (vRange = pArea->vnums; vRange; vRange = vRange->next)
            for ( vnum = vRange->min_vnum; vnum <= vRange->max_vnum; vnum++ )
	    	if ((pObjIndex = get_obj_index(vnum))
	     	  && (( fAll || is_name( arg, pObjIndex->name )
	          || flag_value( type_flags, arg ) == pObjIndex->item_type )))
	    	{
		    found = TRUE;
		    AddToList(buf1, numCols, pObjIndex->vnum, capitalize(pObjIndex->short_descr));
		    if ( ++col % numCols == 0 )
		    	add_buf( buf1, "\n\r" );
		    else
			add_buf(buf1, " ");
	    	}
    }

    if ( !found )
        if (ffree)
	{
	    send_to_char("No free object vnums found in this area.\n\r",ch);
	    return;
	}
	else
	{
	    send_to_char( "Object(s) not found in this area.\n\r", ch);
	    return;
    	}

    if ( col % numCols != 0 )
	add_buf( buf1, "\n\r" );

    page_to_char( buf_string(buf1), ch );
    free_buf(buf1);
    return;
}



REDIT( redit_mshow )
{
    MOB_INDEX_DATA *pMob;
    int value;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  mshow <vnum>\n\r", ch );
	return FALSE;
    }

    if ( !is_number( argument ) )
    {
       send_to_char( "REdit: Need a number.\n\r", ch);
       return FALSE;
    }

    if ( is_number( argument ) )
    {
	value = atoi( argument );
	if ( !( pMob = get_mob_index( value ) ))
	{
	    send_to_char( "REdit:  That mobile does not exist.\n\r", ch );
	    return FALSE;
	}

	ch->desc->pEdit = (void *)pMob;
    }
 
    medit_show( ch, argument );
    ch->desc->pEdit = (void *)ch->in_room;
    return FALSE; 
}



REDIT( redit_oshow )
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  oshow <vnum>\n\r", ch );
	return FALSE;
    }

    if ( !is_number( argument ) )
    {
       send_to_char( "REdit: Need a number.\n\r", ch);
       return FALSE;
    }

    if ( is_number( argument ) )
    {
	value = atoi( argument );
	if ( !( pObj = get_obj_index( value ) ))
	{
	    send_to_char( "REdit:  That object does not exist.\n\r", ch );
	    return FALSE;
	}

	ch->desc->pEdit = (void *)pObj;
    }
 
    oedit_show( ch, argument );
    ch->desc->pEdit = (void *)ch->in_room;
    return FALSE; 
}



/*****************************************************************************
 Name:		check_range( lower vnum, upper vnum )
 Purpose:	Ensures the range spans only one area.
 Called by:	aedit_vnum(olc_act.c).
 ****************************************************************************/
bool check_range( int lower, int upper )
{
    AREA_DATA *pArea;
    VNUM_RANGE *vRange;
    int cnt = 0;

    for ( pArea = area_first; pArea; pArea = pArea->next )
    {
	/*
	 * lower < area < upper
	 */
	for (vRange = pArea->vnums; vRange; vRange = vRange->next)
            if ( ( lower <= vRange->min_vnum && vRange->min_vnum <= upper )
	    ||   ( lower <= vRange->max_vnum && vRange->max_vnum <= upper ) )
	        ++cnt;

	if ( cnt > 1 )
	    return FALSE;
    }
    return TRUE;
}



AREA_DATA *get_vnum_area( int vnum )
{
    AREA_DATA *pArea;
    VNUM_RANGE *vRange;

    for ( pArea = area_first; pArea; pArea = pArea->next )
	for (vRange = pArea->vnums; vRange; vRange = vRange->next)
            if ( vnum >= vRange->min_vnum
             && vnum <= vRange->max_vnum )
                return pArea;

    return 0;
}


FEDIT( fedit_show )
{
    char buf[MAX_STRING_LENGTH];

    Faction * faction(reinterpret_cast<Faction*>(ch->desc->pEdit));
    sprintf(buf, "{w%s\n", faction->Name().c_str());
    send_to_char(buf, ch);

    sprintf(buf, "Min. Friend Rating: {W%d  {wMin. Enemy Rating: {W%d{w\n\r", faction->FriendThreshold(), faction->EnemyThreshold());
    send_to_char(buf, ch);

    std::ostringstream mess;
    mess << "Flags:{W";
    for (unsigned int i(0); i < Faction::MaxFlag; ++i)
    {
        Faction::Flag flag(static_cast<Faction::Flag>(i));
        if (faction->HasFlag(flag))
            mess << ' ' << Faction::NameFor(flag);
    }
    mess << "{w\n";
    send_to_char(mess.str().c_str(), ch);

    for (size_t i(0); i < faction->AllyCount(); ++i)
	{
        const Faction * otherFaction(FactionTable::LookupFor(faction->Ally(i)));
        if (otherFaction == NULL)
            continue;

	    sprintf(buf, "Ally: %s (%d)\n", otherFaction->Name().c_str(), faction->Ally(i));
	    send_to_char(buf, ch);
    }

    for (size_t i(0); i < faction->EnemyCount(); ++i)
	{
        const Faction * otherFaction(FactionTable::LookupFor(faction->Enemy(i)));
        if (otherFaction == NULL)
            continue;

	    sprintf(buf, "Oppo: %s (%d)\n", otherFaction->Name().c_str(), faction->Enemy(i));
	    send_to_char(buf, ch);
    }

    send_to_char("Initial Ratings\n", ch);
    send_to_char("---------------\n", ch);

    for (size_t i(0); i < faction->InitClassCount(); ++i)
    {
        mess.str("");
        std::pair<unsigned int, int> value(faction->InitClass(i));
        mess << "Class: " << class_table[value.first].name << " = {W" << value.second << "{x\n";
        send_to_char(mess.str().c_str(), ch);
    }

    for (size_t i(0); i < faction->InitRaceCount(); ++i)
    {
        mess.str("");
        std::pair<unsigned int, int> value(faction->InitRace(i));
        mess << "Race: " << race_table[value.first].name << " = {W" << value.second << "{x\n";
        send_to_char(mess.str().c_str(), ch);
    }

    for (size_t i(0); i < faction->InitAltarCount(); ++i)
    {
        mess.str("");
        std::pair<unsigned int, int> value(faction->InitAltar(i));
        mess << "Altar: " << value.first << " = {W" << value.second << "{x\n";
        send_to_char(mess.str().c_str(), ch);
    }

    for (size_t i(0); i < faction->InitGenderCount(); ++i)
    {
        mess.str("");
        std::pair<unsigned int, int> value(faction->InitGender(i));
        mess << "Gender: " << sex_table[value.first].name << " = {W" << value.second << "{x\n";
        send_to_char(mess.str().c_str(), ch);
    }

    return false;
}

FEDIT( fedit_rating )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int rating;

    Faction * faction(reinterpret_cast<Faction*>(ch->desc->pEdit));
    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "friend"))
    {
        if (!is_number(argument))
        {
            send_to_char("Usage: fedit rating friend [min. rating]\n\r", ch);
            return FALSE;
        }

        rating = atoi(argument);

        if (rating < FactionStanding::Minimum || rating > FactionStanding::Maximum)
        {
            send_to_char("Rating out of range.\n\r", ch);
            sprintf(buf, "Valid ratings are from %d to %d.\n\r", FactionStanding::Minimum, FactionStanding::Maximum);
            send_to_char(buf, ch);
            return FALSE;
        }

        faction->SetFriendThreshold(rating);
        return TRUE;
    }
    
    if (!str_cmp(arg, "enemy"))
    {
        if (!is_number(argument))
        {
            send_to_char("Usage: fedit rating enemy [min. rating]\n\r", ch);
            return FALSE;
        }

        rating = atoi(argument);

        if (rating < FactionStanding::Minimum || rating > FactionStanding::Maximum)
        {
            send_to_char("Rating out of range.\n\r", ch);
            sprintf(buf, "Valid ratings are from %d to %d.\n\r", FactionStanding::Minimum, FactionStanding::Maximum);
            send_to_char(buf, ch);
            return FALSE;
        }

        faction->SetEnemyThreshold(rating);
        return TRUE;
    }
    
    send_to_char("Usage: fedit rating [friend/enemy] [min. rating]\n\r", ch);
    return FALSE;
}

FEDIT( fedit_opposing )
{
    char buf[MAX_STRING_LENGTH];

    Faction * faction(reinterpret_cast<Faction*>(ch->desc->pEdit));
    if (!is_number(argument))
    {
        send_to_char("Usage: fedit opposing [faction to toggle]\n\r", ch);
        return FALSE;
    }

    unsigned int factionNumber(atoi(argument));
    const Faction * otherFaction(FactionTable::LookupFor(factionNumber));
    if (otherFaction == NULL)
    {
        send_to_char("Faction does not exist.\n\r", ch);
        return FALSE;
    }

    if (faction == otherFaction)
    {
        send_to_char("Faction cannot be opposed to itself.  This isn't America.\n\r", ch);
        return FALSE;
    }

    if (faction->ToggleEnemy(factionNumber))
    {
        sprintf(buf, "Opposing Faction '%s (%d)' added.\n\r", otherFaction->Name().c_str(), factionNumber);
        send_to_char(buf, ch);
        return true;
    }

	sprintf(buf, "Opposing Faction '%s (%d)' removed.\n\r", otherFaction->Name().c_str(), factionNumber);
	send_to_char(buf, ch);
	return true;
}

FEDIT( fedit_ally )
{
    char buf[MAX_STRING_LENGTH];

    Faction * faction(reinterpret_cast<Faction*>(ch->desc->pEdit));
    if (!is_number(argument))
    {
        send_to_char("Usage: fedit ally [faction to toggle]\n\r", ch);
        return FALSE;
    }

    unsigned int factionNumber(atoi(argument));
    const Faction * otherFaction(FactionTable::LookupFor(factionNumber));
    if (otherFaction == NULL)
    {
        send_to_char("Faction does not exist.\n\r", ch);
        return FALSE;
    }

    if (faction == otherFaction)
    {
        send_to_char("Faction cannot be allied with itself.  This isn't Monsanto.\n\r", ch);
        return FALSE;
    }

    if (faction->ToggleAlly(factionNumber))
    {
        sprintf(buf, "Allied Faction '%s (%d)' added.\n\r", otherFaction->Name().c_str(), factionNumber);
        send_to_char(buf, ch);
        return true;
    }

	sprintf(buf, "Allied Faction '%s (%d)' removed.\n\r", otherFaction->Name().c_str(), factionNumber);
	send_to_char(buf, ch);
	return true;
}

FEDIT( fedit_name )
{
    Faction * faction(reinterpret_cast<Faction*>(ch->desc->pEdit));
    if (argument[0] == '\0')
    {
        send_to_char("Usage: fedit name [faction name]\n\r", ch);
        return FALSE;
    }

    faction->SetName(argument);
    return TRUE;
}

static void show_fedit_init_syntax(CHAR_DATA * ch)
{
    send_to_char("Usage: fedit init class <class_name> <value>\n", ch);
    send_to_char("       fedit init race <race_name> <value>\n", ch);
    send_to_char("       fedit init altar <altar_num> <value>\n", ch);
    send_to_char("       fedit init gender <gender_name> <value>\n", ch);
    send_to_char("Use a value of 0 to clear an entry.\n", ch);
}

FEDIT( fedit_init )
{
    Faction * faction(reinterpret_cast<Faction*>(ch->desc->pEdit));
    
    char arg0[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg0);
    argument = one_argument(argument, arg1);

    // Convert last argument
    if (!is_number(argument))
    {
        show_fedit_init_syntax(ch);
        return false;
    }

    int value(atoi(argument));
    if (value < FactionStanding::Minimum || value > FactionStanding::Maximum)
    {
        send_to_char("Invalid initial value; exceeds bounds.\n", ch);
        return false;
    }

    // Handle according to type of init data
    if (!str_prefix(arg0, "class"))
    {
        int class_num(class_lookup(arg1));
        if (class_num < 0)
        {
            send_to_char("Invalid class specified.\n", ch);
            return false;
        }

        faction->SetInitClass(class_num, value);
        return true;
    }

    if (!str_prefix(arg0, "race"))
    {
        int race_num(race_lookup(arg1));
        if (race_num < 0)
        {
            send_to_char("Invalid race specified.\n", ch);
            return false;
        }

        faction->SetInitRace(race_num, value);
        return true;
    }

    if (!str_prefix(arg0, "altar"))
    {
        if (!is_number(arg1))
        {
            send_to_char("Altar must be a room vnum.\n", ch);
            return false;
        }

        int vnum(atoi(arg1));
        if (get_room_index(vnum) == NULL)
        {
            send_to_char("Invalid room vnum; no room at that index.\n", ch);
            return false;
        }

        faction->SetInitAltar(vnum, value);
        return true;
    }

    if (!str_prefix(arg0, "gender"))
    {
        int sex_num(sex_lookup(arg1));
        if (sex_num < 0)
        {
            send_to_char("Invalid gender specified.\n", ch);
            return false;
        }

        faction->SetInitGender(sex_num, value);
        return true;
    }

    show_fedit_init_syntax(ch);
    return false;
}

FEDIT ( fedit_add )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int vnum;
    MOB_INDEX_DATA *mob;
    bool found = FALSE;

    Faction * faction(reinterpret_cast<Faction*>(ch->desc->pEdit));
    if (argument[0] == '\0')
    {
        send_to_char("Usage: add [mob vnum] <additional vnums> <...>\n\r", ch);
        return FALSE;
    }

    size_t factionNumber(FactionTable::Instance().LookupIndex(*faction));
    if (factionNumber == Faction::None)
    {
        send_to_char("Could not retrieve index for faction.\n", ch);
        return false;
    }

    do
    {
        argument = one_argument(argument, arg);

        if (!is_number(arg) || ((vnum = atoi(arg)) <= 0))
        {
            sprintf(buf, "Invalid vnum: %s\n\r", arg);
            send_to_char(buf, ch);
            continue;
        }

        if ((mob = get_mob_index(vnum)) == NULL)
        {
            sprintf(buf, "Mobile not found: %d\n\r", vnum);
            send_to_char(buf, ch);
            continue;
        }

        mob->factionNumber = factionNumber;
        SET_BIT(mob->area->area_flags, AREA_CHANGED);

        sprintf(buf, "Added: %s (%d)\n\r", mob->short_descr, mob->vnum);
        send_to_char(buf, ch);

        found = TRUE;
    }
    while (argument[0] != '\0');

    if (found)
	send_to_char("Faction added to mobs.  Remember to 'asave changed' to save changes.\n\r", ch);

    return FALSE;
}
    
	    
	
HEDIT( hedit_show )
{
    HELP_DATA *pHelp;
    BUFFER *output;
    char buf[MAX_STRING_LENGTH];

    EDIT_HELP(ch, pHelp);

    output = new_buf();

    sprintf(buf, "Keyword(s): %s\n\r", pHelp->keyword);
    add_buf(output, buf);

    sprintf(buf, "Title     : %s\n\r", pHelp->title);
    add_buf(output, buf);

    sprintf(buf, "Min. Level: %d\n\r", pHelp->level);
    add_buf(output, buf);

    if (pHelp->text)
        add_buf(output, pHelp->text);

    page_to_char(buf_string(output),ch);

    return FALSE;
}

HEDIT( hedit_keywords )
{
    HELP_DATA *pHelp;
    MYSQL_RES *result;
    MYSQL_ROW row;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char qbuf[MAX_INPUT_LENGTH * 2 + 60];
    char *a;
    bool found = FALSE;

    EDIT_HELP(ch, pHelp);

    if (argument[0] == '\0')
    {
	send_to_char("What do you wish to set the keyword(s) to?\n\r", ch);
	return FALSE;
    }

    a = argument;

    while(argument[0] != '\0')
    {
	argument = one_argument(argument, arg);

	sprintf(qbuf, "SELECT keywords from help_data WHERE keywords LIKE \"%%%s%%\"", arg);
	if (mysql_query(&mysql, qbuf))
	{
	    log_mysql_error();
	    return FALSE;
	}

	result = mysql_use_result(&mysql);

	while (!found && (row = mysql_fetch_row(result)) != NULL)
	{
	    if (is_name(arg, row[0]) && str_cmp(row[0], pHelp->keyword))
	    {
		sprintf(buf, "A helpfile already exists with the keyword %s.\n\r", arg);
		send_to_char(buf, ch);
		found = TRUE;
		break;
	    }
	}

	mysql_free_result(result);
	
	if (found)
	   return FALSE;
    }

    argument = a;
    while (*a != '\0')
    {
	*a = UPPER(*a);
	a++;
    }

    sprintf(qbuf, "UPDATE help_data SET keywords = \"%s\" WHERE keywords = \"%s\"", argument, pHelp->keyword);

    if (mysql_query(&mysql, qbuf))
    {
	log_mysql_error();
	return FALSE;
    }

    free_string(pHelp->keyword);
    pHelp->keyword = str_dup(argument);

    return TRUE;
}

HEDIT( hedit_text )
{
    HELP_DATA *pHelp;

    EDIT_HELP(ch, pHelp);

    if ( argument[0] == '\0' )
    {
	string_append( ch, &pHelp->text );
	return TRUE;
    }

    send_to_char( "Syntax: desc -or- text\n\r", ch );
    return FALSE;
}

HEDIT( hedit_title )
{
    HELP_DATA *pHelp;
    char buf[MAX_INPUT_LENGTH * 2];
    char qbuf[MAX_INPUT_LENGTH * 4 + 60];
    bool title_none = FALSE;
 
    EDIT_HELP(ch, pHelp);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: title <title>\n\r", ch);
	return FALSE;
    }

    if (!str_cmp(argument, "none"))
    {
	sprintf(qbuf, "UPDATE help_data SET title = NULL WHERE keywords = \"%s\"", pHelp->keyword);
	title_none = TRUE;
    }
    else
    {
        mysql_real_escape_string(&mysql, buf, argument, strlen(argument));
        sprintf(qbuf, "UPDATE help_data SET title = \"%s\" WHERE keywords = \"%s\"", buf, pHelp->keyword);
    }

    if (mysql_query(&mysql, qbuf))
    {
	log_mysql_error();
	return FALSE;
    }

    free_string(pHelp->title);

    if (!title_none)
        pHelp->title = str_dup(argument);

    send_to_char("Title set.\n\r", ch);
    return TRUE;
}

HEDIT( hedit_level )
{
    HELP_DATA *pHelp;
    char qbuf[MAX_INPUT_LENGTH + 60];

    EDIT_HELP(ch, pHelp);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax: level <minimum level for helpfile viewing>\n\r", ch);
	return FALSE;
    }

    sprintf(qbuf, "UPDATE help_data SET level = %d WHERE keywords = \"%s\"", atoi(argument), pHelp->keyword);

    if (mysql_query(&mysql, qbuf))
    {
	log_mysql_error();
	return FALSE;
    }

    pHelp->level = atoi(argument);

    send_to_char("Minimum level set.\n\r", ch);
    return TRUE;
}

/*
 * Area Editor Functions.
 */
AEDIT( aedit_show )
{
    AREA_DATA *pArea;
    char buf  [MAX_STRING_LENGTH];
    int i;
    VNUM_RANGE *vRange;

    EDIT_AREA(ch, pArea);

    sprintf( buf, "Name:     [%5d] %s\n\r", pArea->vnum, pArea->name );
    send_to_char( buf, ch );

#if 0  /* ROM OLC */
    sprintf( buf, "Recall:   [%5d] %s\n\r", pArea->recall,
	get_room_index( pArea->recall )
	? get_room_index( pArea->recall )->name : "none" );
    send_to_char( buf, ch );
#endif /* ROM */

    sprintf( buf, "File:     %s\n\r", pArea->file_name );
    send_to_char( buf, ch );

    send_to_char("Vnums:    ", ch);
    for (vRange = pArea->vnums; vRange; vRange = vRange->next)
    {
        sprintf( buf, "[%d-%d]%s", vRange->min_vnum, vRange->max_vnum, vRange->next ? ", " : "\n\r" );
        send_to_char( buf, ch );
    }

    sprintf( buf, "Age:      [%d]\n\r",	pArea->age );
    send_to_char( buf, ch );

    sprintf( buf, "Players:  [%d]\n\r", pArea->nplayer );
    send_to_char( buf, ch );

    sprintf( buf, "Security: [%d]\n\r", pArea->security );
    send_to_char( buf, ch );

    sprintf(buf, "Danger:     [%d]\n\r", pArea->danger);
    send_to_char(buf, ch);

    sprintf( buf, "Builders: [%s]\n\r", pArea->builders );
    send_to_char( buf, ch );

    sprintf( buf, "Credits : [%s]\n\r", pArea->credits );
    send_to_char( buf, ch );

    sprintf( buf, "Flags:    [%s]\n\r", flag_string( area_flags, pArea->area_flags ) );
    send_to_char( buf, ch );
   
    sprintf( buf, "Flags2:   [%s]\n\r", flag_string( ainfo_flags, pArea->ainfo_flags ) );
    send_to_char( buf, ch );

    sprintf( buf, "\n\rWeather Data:\n\r" );
    send_to_char( buf, ch );
    
    sprintf( buf, "Precipitation:  [%s]\n\r", flag_string( precip_flags, pArea->base_precip) );
    send_to_char( buf, ch );

    sprintf( buf, "Temperature:    [%s]\n\r", flag_string( temp_flags, pArea->base_temp) );
    send_to_char( buf, ch );

    sprintf( buf, "Wind Magnitude: [%s]\n\r", flag_string( wind_mag_flags, pArea->base_wind_mag ) );
    send_to_char( buf, ch );

    sprintf( buf, "Wind Direction: [%s]\n\r",
	((pArea->base_wind_dir == 0) ? "north" :
	 (pArea->base_wind_dir == 1) ? "east"  :
	 (pArea->base_wind_dir == 2) ? "south" :
	 (pArea->base_wind_dir == 3) ? "west"  :
	 (pArea->base_wind_dir == 4) ? "up"    :
	 (pArea->base_wind_dir == 5) ? "down"  : "undefined" ));
    send_to_char( buf, ch );

    sprintf( buf, "Geography:      [%s]\n\r", flag_string( geo_flags, pArea->geography) );

    send_to_char( buf, ch );

    sprintf( buf, "Herbs: [");
    for (i = 0; herb_table[i].name; i++)
	if (IS_SET(pArea->herbs, herb_table[i].bit))
	    sprintf(buf, "%s '%s' ", buf, herb_table[i].name);
    sprintf(buf, "%s]\n\r", buf);
	   
    send_to_char( buf, ch );

    // Weave-related
    sprintf(buf, "Fount: [Frequency: %s] [Order Bias: %d] [Positive Bias: %d]\n", 
        Fount::NameFor(pArea->fount_frequency), pArea->fount_order_bias, pArea->fount_positive_bias);
    send_to_char(buf, ch);

    // Shade-related
    sprintf(buf, "Shades: [Density: %s] [Power: %s]\n", Shades::NameFor(pArea->shade_density), Shades::NameFor(pArea->shade_power));
    send_to_char(buf, ch);

    // Earth-related
    if (pArea->stone_type >= 0 && pArea->stone_type < MAX_MATERIALS && material_table[pArea->stone_type].stone)
        sprintf(buf, "Stone: %s\n", material_table[pArea->stone_type].name);
    else
        sprintf(buf, "Stone: none\n");
    send_to_char(buf, ch);
 
    return FALSE;
}

AEDIT( aedit_reset )
{
    AREA_DATA *pArea;

    EDIT_AREA(ch, pArea);

    reset_area( pArea );
    send_to_char( "Area reset.\n\r", ch );

    return FALSE;
}

AEDIT( aedit_roomkill )
{
    AREA_DATA *pArea;
    ROOM_INDEX_DATA *pRoom, *nRoom;
    RESET_DATA *pReset, *vReset;
    EXIT_DATA *pexit;
    VNUM_RANGE *vRange;
    char arg[MAX_INPUT_LENGTH];
    int vnum, door, iHash;
    bool found = FALSE;

    EDIT_AREA(ch, pArea);

    argument = one_argument( argument, arg );

    if (!is_number(arg))
    {
	send_to_char("Syntax: roomkill <vnum>\n\r", ch);
	return FALSE;
    }

    vnum = atoi(arg);

    if ((nRoom = get_room_index(vnum)) == NULL)
    {
	send_to_char("Room does not exist.\n\r", ch);
	return FALSE;
    }

    for (vRange = pArea->vnums; vRange; vRange = vRange->next)
        if ((vnum >= vRange->min_vnum) && (vnum <= vRange->max_vnum))
        {
	    found = TRUE;
	    break;
        }

    if (!found)
    {
	send_to_char("Room not in current area.\n\r", ch);
	return FALSE;
    }

    if ((pRoom = room_index_hash[vnum % MAX_KEY_HASH])
     && (pRoom->vnum == vnum))
	room_index_hash[vnum % MAX_KEY_HASH] = pRoom->next;
    else
	for ( pRoom = room_index_hash[vnum % MAX_KEY_HASH];
              pRoom != NULL;
              pRoom = pRoom->next )
	    if ((pRoom->next) && (pRoom->next->vnum == vnum))
            {
	        pRoom->next = pRoom->next->next;
	        break;
	    }

    for (pReset = pArea->reset_first; pReset != NULL; pReset = pReset->next)
    {
	for (vReset = pRoom->reset_first; vReset != NULL; vReset = vReset->next )
	    if (pReset->next == vReset)
		pReset->next = pReset->next->next;
	
	if (!pReset->next)
	    break;
    }  

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
        for ( pRoom = room_index_hash[iHash];
              pRoom != NULL;
              pRoom = pRoom->next )
            for ( door = 0; door <= 5; door++ )
                if ( ( pexit = pRoom->exit[door] ) != NULL )
	            if (pexit->u1.to_room == nRoom)
		        pRoom->exit[door] = NULL;

    for (door = 0; door <= 5; door++ )
        nRoom->exit[door] = NULL;    	

    send_to_char("Room killed.\n\r", ch);
    return TRUE;
}


AEDIT( aedit_create )
{
    AREA_DATA *pArea;

    pArea               =   new_area();
    area_last->next     =   pArea;
    area_last		=   pArea;	/* Thanks, Walker. */
    ch->desc->pEdit     =   (void *)pArea;

    SET_BIT( pArea->area_flags, AREA_ADDED );
    send_to_char( "Area Created.\n\r", ch );
    return FALSE;
}

AEDIT( aedit_name )
{
    AREA_DATA *pArea;

    EDIT_AREA(ch, pArea);

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:   name [$name]\n\r", ch );
	return FALSE;
    }

    free_string( pArea->name );
    pArea->name = str_dup( argument );

    send_to_char( "Name set.\n\r", ch );
    return TRUE;
}

AEDIT( aedit_credits )
{
    AREA_DATA *pArea;

    EDIT_AREA(ch, pArea);

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:   credits [$credits]\n\r", ch );
	return FALSE;
    }

    free_string( pArea->credits );
    pArea->credits = str_dup( argument );

    send_to_char( "Credits set.\n\r", ch );
    return TRUE;
}

AEDIT( aedit_file )
{
    AREA_DATA *pArea;
    char file[MAX_STRING_LENGTH];
    int i, length;

    EDIT_AREA(ch, pArea);

    one_argument( argument, file );	/* Forces Lowercase */

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  filename [$file]\n\r", ch );
	return FALSE;
    }

    /*
     * Simple Syntax Check.
     */
    length = strlen( argument );
    if ( length > 8 )
    {
	send_to_char( "No more than eight characters allowed.\n\r", ch );
	return FALSE;
    }
    
    /*
     * Allow only letters and numbers.
     */
    for ( i = 0; i < length; i++ )
    {
	if ( !isalnum( file[i] ) )
	{
	    send_to_char( "Only letters and numbers are valid.\n\r", ch );
	    return FALSE;
	}
    }    

    free_string( pArea->file_name );
    strcat( file, ".are" );
    pArea->file_name = str_dup( file );

    send_to_char( "Filename set.\n\r", ch );
    return TRUE;
}



AEDIT( aedit_age )
{
    AREA_DATA *pArea;
    char age[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    one_argument( argument, age );

    if ( !is_number( age ) || age[0] == '\0' )
    {
	send_to_char( "Syntax:  age [#xage]\n\r", ch );
	return FALSE;
    }

    pArea->age = atoi( age );

    send_to_char( "Age set.\n\r", ch );
    return TRUE;
}


#if 0 /* ROM OLC */
AEDIT( aedit_recall )
{
    AREA_DATA *pArea;
    char room[MAX_STRING_LENGTH];
    int  value;

    EDIT_AREA(ch, pArea);

    one_argument( argument, room );

    if ( !is_number( argument ) || argument[0] == '\0' )
    {
	send_to_char( "Syntax:  recall [#xrvnum]\n\r", ch );
	return FALSE;
    }

    value = atoi( room );

    if ( !get_room_index( value ) )
    {
	send_to_char( "AEdit:  Room vnum does not exist.\n\r", ch );
	return FALSE;
    }

    pArea->recall = value;

    send_to_char( "Recall set.\n\r", ch );
    return TRUE;
}
#endif /* ROM OLC */

AEDIT( aedit_security )
{
    AREA_DATA *pArea;
    char sec[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int  value;

    EDIT_AREA(ch, pArea);

    one_argument( argument, sec );

    if ( !is_number( sec ) || sec[0] == '\0' )
    {
	send_to_char( "Syntax:  security [#xlevel]\n\r", ch );
	return FALSE;
    }

    value = atoi( sec );

    if ( value > ch->pcdata->security || value < 0 )
    {
	if ( ch->pcdata->security != 0 )
	{
	    sprintf( buf, "Security is 0-%d.\n\r", ch->pcdata->security );
	    send_to_char( buf, ch );
	}
	else
	    send_to_char( "Security is 0 only.\n\r", ch );
	return FALSE;
    }

    pArea->security = value;

    send_to_char( "Security set.\n\r", ch );
    return TRUE;
}

template <typename Type> bool do_edit_stone(CHAR_DATA * ch, const char * argument, Type * edited)
{
    // Show help if nothing entered
    if (argument[0] == '\0')
    {
        std::ostringstream mess;
        mess << "Syntax: stone <stone_type>|none\n";
        mess << "Stone options are:\n";
        for (unsigned int i(0); i < MAX_MATERIALS; ++i)
        {
            if (material_table[i].stone)
                mess << " " << material_table[i].name << "\n";
        }
        send_to_char(mess.str().c_str(), ch);
        return false;
    }

    // Check for none
    if (!str_prefix(argument, "none"))
    {
        edited->stone_type = -1;
        send_to_char("Stone set to none.\n", ch);
        return true;
    }

    // Lookup the material
    int material(material_lookup(argument));
    if (material < 0 || material >= MAX_MATERIALS || !material_table[material].stone)
    {
        send_to_char("Invalid stone type.\n", ch);
        return true;
    }

    // Set the stone
    edited->stone_type = material;
    std::ostringstream mess;
    mess << "Stone set to " << material_table[material].name << '\n';
    send_to_char(mess.str().c_str(), ch);
    return true;
}

AEDIT(aedit_stone)
{
    AREA_DATA * pArea;
    EDIT_AREA(ch, pArea);
    return do_edit_stone(ch, argument, pArea);
}

AEDIT(aedit_fount)
{
    AREA_DATA * pArea;
    char arg[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    // Show help if nothing entered
    if (argument[0] == '\0')
    {
        std::ostringstream mess;
        mess << "Syntax: fount <frequency> <order_bias> <positive_bias>\n";
        mess << "Frequency options are:";
        for (unsigned int i(0); i < Fount::Max; ++i)
            mess << " " << Fount::NameFor(static_cast<Fount::Frequency>(i));
        mess << "\nOrder/Positive bias range is " << Fount::PowerMin << " to " << Fount::PowerMax << "\n";
        send_to_char(mess.str().c_str(), ch);
        return false;
    }

    // Get and set the values (note that bad frequency arguments resolve to 'default')
    argument = one_argument(argument, arg); pArea->fount_frequency = Fount::ValueFor(arg);
    argument = one_argument(argument, arg); pArea->fount_order_bias = URANGE(Fount::PowerMin, atoi(arg), Fount::PowerMax);
    argument = one_argument(argument, arg); pArea->fount_positive_bias = URANGE(Fount::PowerMin, atoi(arg), Fount::PowerMax);
    
    // Inform the builder
    std::ostringstream mess;
    mess << "Fount values set to [Frequency: " << Fount::NameFor(pArea->fount_frequency);
    mess << "] [Order Bias: " << pArea->fount_order_bias << "] [Positive Bias: " << pArea->fount_positive_bias << "]\n";
    send_to_char(mess.str().c_str(), ch);
    return true;
}

AEDIT(aedit_shades)
{
    AREA_DATA * pArea;
    char arg[MAX_STRING_LENGTH];
    EDIT_AREA(ch, pArea);

    // Show help if nothing entered
    if (argument[0] == '\0')
    {
        std::ostringstream mess;
        mess << "Syntax: shades <density> <power>\n";

        mess << "Density options are:";
        for (unsigned int i(0); i < Shades::MaxDensity; ++i)
            mess << " " << Shades::NameFor(static_cast<Shades::Density>(i));

        mess << "\nPower options are:";
        for (unsigned int i(0); i < Shades::MaxPower; ++i)
            mess << " " << Shades::NameFor(static_cast<Shades::Power>(i));

        mess << "\n";
        send_to_char(mess.str().c_str(), ch);
        return false;
    }

    // Get and set the values (note that bad arguments resolve to default)
    argument = one_argument(argument, arg); pArea->shade_density = Shades::DensityFor(arg);
    argument = one_argument(argument, arg); pArea->shade_power = Shades::PowerFor(arg);

    // Inform the builder
    std::ostringstream mess;
    mess << "Shade values set to [Density: " << Shades::NameFor(pArea->shade_density) << "] [Power: ";
    mess << Shades::NameFor(pArea->shade_power) << "]\n";
    send_to_char(mess.str().c_str(), ch);
    return true;
}

AEDIT( aedit_danger )
{
    AREA_DATA *pArea;
    char sec[MAX_STRING_LENGTH];
    int  value;

    EDIT_AREA(ch, pArea);

    one_argument( argument, sec );

    if ( !is_number( sec ) || sec[0] == '\0' )
    {
	send_to_char( "Syntax:  danger [#danger]\n\r", ch );
	return FALSE;
    }

    value = atoi( sec );

    if (value < 0 || value > 10)
    {
	send_to_char( "Danger must be in the range of 0-10.\n\r", ch);
	return FALSE;
    }

    pArea->danger = value;

    send_to_char( "Danger set.\n\r", ch );
    return TRUE;
}



AEDIT( aedit_builder )
{
    AREA_DATA *pArea;
    char name[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    one_argument( argument, name );

    if ( name[0] == '\0' )
    {
	send_to_char( "Syntax:  builder [$name]  -toggles builder\n\r", ch );
	send_to_char( "Syntax:  builder All      -allows everyone\n\r", ch );
	return FALSE;
    }

    name[0] = UPPER( name[0] );

    if ( strstr( pArea->builders, name ) != '\0' )
    {
	pArea->builders = string_replace( pArea->builders, name, "\0" );
	pArea->builders = string_unpad( pArea->builders );

	if ( pArea->builders[0] == '\0' )
	{
	    free_string( pArea->builders );
	    pArea->builders = str_dup( "None" );
	}
	send_to_char( "Builder removed.\n\r", ch );
	return TRUE;
    }
    else
    {
	buf[0] = '\0';
	if ( strstr( pArea->builders, "None" ) != '\0' )
	{
	    pArea->builders = string_replace( pArea->builders, "None", "\0" );
	    pArea->builders = string_unpad( pArea->builders );
	}

	if (pArea->builders[0] != '\0' )
	{
	    strcat( buf, pArea->builders );
	    strcat( buf, " " );
	}
	strcat( buf, name );
	free_string( pArea->builders );
	pArea->builders = string_proper( str_dup( buf ) );

	send_to_char( "Builder added.\n\r", ch );
	send_to_char( pArea->builders,ch);
	return TRUE;
    }

    return FALSE;
}



AEDIT( aedit_vnum )
{
    AREA_DATA *pArea, *lArea;
    char lower[MAX_STRING_LENGTH];
    char upper[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;
    VNUM_RANGE *vRange;

    EDIT_AREA(ch, pArea);

    argument = one_argument( argument, lower );
    one_argument( argument, upper );

    if ( !is_number( lower ) || lower[0] == '\0'
    || !is_number( upper ) || upper[0] == '\0' )
    {
	send_to_char( "Syntax:  vnum [#xlower] [#xupper]\n\r", ch );
	return FALSE;
    }

    if ( ( ilower = atoi( lower ) ) > ( iupper = atoi( upper ) ) )
    {
	send_to_char( "AEdit:  Upper must be larger then lower.\n\r", ch );
	return FALSE;
    }

    if (((lArea = get_vnum_area(ilower)) && (lArea != pArea))
     || ((lArea = get_vnum_area(iupper)) && (lArea != pArea)))
    {
	send_to_char( "AEdit:  vnums already assigned to different area.\n\r", ch);
	return FALSE;
    }
    
    if ( !check_range( atoi( lower ), atoi( upper ) ) )
    {
	send_to_char( "AEdit:  range must include only one block within this area.\n\r", ch );
	return FALSE;
    }
/*
    if ( get_vnum_area( ilower )
    && get_vnum_area( ilower ) != pArea )
    {
	send_to_char( "AEdit:  Lower vnum already assigned.\n\r", ch );
	return FALSE;
    }

    pArea->min_vnum = ilower;
    send_to_char( "Lower vnum set.\n\r", ch );

    if ( get_vnum_area( iupper )
    && get_vnum_area( iupper ) != pArea )
    {
	send_to_char( "AEdit:  Upper vnum already assigned.\n\r", ch );
	return TRUE;
    }

    pArea->max_vnum = iupper;
    send_to_char( "Upper vnum set.\n\r", ch );
*/

    // First check if we simply want to expand an existing block...
    for (vRange = pArea->vnums; vRange; vRange = vRange->next)
	if (((ilower <= vRange->min_vnum) && (iupper >= vRange->max_vnum))
	 || ((ilower > vRange->min_vnum) && (ilower < vRange->max_vnum))
	 || ((iupper > vRange->min_vnum) && (iupper < vRange->max_vnum)))
	{
	    vRange->min_vnum = ilower;
	    vRange->max_vnum = iupper;
	    send_to_char("Existing vnum block resized.\n\r", ch);
	    return TRUE;
	}

    vRange = (VNUM_RANGE*)alloc_perm(sizeof(*vRange));
    g_vnum_blocks++;
    vRange->min_vnum = ilower;
    vRange->max_vnum = iupper;
    vRange->next = pArea->vnums;
    pArea->vnums = vRange;

    send_to_char("New vnum range set.\n\r", ch);

    return TRUE;
}

/*

AEDIT( aedit_lvnum )
{
    AREA_DATA *pArea;
    char lower[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;

    EDIT_AREA(ch, pArea);

    one_argument( argument, lower );

    if ( !is_number( lower ) || lower[0] == '\0' )
    {
	send_to_char( "Syntax:  min_vnum [#xlower]\n\r", ch );
	return FALSE;
    }

    if ( ( ilower = atoi( lower ) ) > ( iupper = pArea->max_vnum ) )
    {
	send_to_char( "AEdit:  Value must be less than the max_vnum.\n\r", ch );
	return FALSE;
    }
    
    if ( !check_range( ilower, iupper ) )
    {
	send_to_char( "AEdit:  Range must include only this area.\n\r", ch );
	return FALSE;
    }

    if ( get_vnum_area( ilower )
    && get_vnum_area( ilower ) != pArea )
    {
	send_to_char( "AEdit:  Lower vnum already assigned.\n\r", ch );
	return FALSE;
    }

    pArea->min_vnum = ilower;
    send_to_char( "Lower vnum set.\n\r", ch );
    return TRUE;
}



AEDIT( aedit_uvnum )
{
    AREA_DATA *pArea;
    char upper[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;

    EDIT_AREA(ch, pArea);

    one_argument( argument, upper );

    if ( !is_number( upper ) || upper[0] == '\0' )
    {
	send_to_char( "Syntax:  max_vnum [#xupper]\n\r", ch );
	return FALSE;
    }

    if ( ( ilower = pArea->min_vnum ) > ( iupper = atoi( upper ) ) )
    {
	send_to_char( "AEdit:  Upper must be larger then lower.\n\r", ch );
	return FALSE;
    }
    
    if ( !check_range( ilower, iupper ) )
    {
	send_to_char( "AEdit:  Range must include only this area.\n\r", ch );
	return FALSE;
    }

    if ( get_vnum_area( iupper )
    && get_vnum_area( iupper ) != pArea )
    {
	send_to_char( "AEdit:  Upper vnum already assigned.\n\r", ch );
	return FALSE;
    }

    pArea->max_vnum = iupper;
    send_to_char( "Upper vnum set.\n\r", ch );

    return TRUE;
}

*/

AEDIT( aedit_precip )
{
    AREA_DATA *pArea;
    int value;

    if ( argument[0] != '\0' )
        if ((value = flag_value(precip_flags, argument)) != NO_FLAG)
	{
	    EDIT_AREA(ch, pArea);
	    pArea->base_precip = value;
	    send_to_char("Area precipitation set.\n\r", ch);
	    return TRUE;
	}

    send_to_char( "Syntax:  precip <type>\n\r"
		  "Type '? precip' for a list of acceptable types.\n\r", ch );
    return FALSE;
}

AEDIT( aedit_temp )
{
    AREA_DATA *pArea;
    int value;

    if ( argument[0] != '\0' )
	if ((value = flag_value(temp_flags, argument)) != NO_FLAG)
	{
	    EDIT_AREA(ch, pArea);
	    pArea->base_temp = value;
	    send_to_char("Base temperature set.\n\r", ch);
	    return TRUE;
	}

    send_to_char( "Syntax:  temp <type>\n\r"
		  "Type '? temp' for a list of flags.\n\r", ch );
    return FALSE;
}

AEDIT( aedit_wind_mag )
{
    AREA_DATA *pArea;
    int value;

    if ( argument[0] != '\0' )
	if ((value = flag_value(wind_mag_flags, argument)) != NO_FLAG)
	{
	    EDIT_AREA(ch, pArea);
	    pArea->base_wind_mag = value;
	    send_to_char("Base wind magnitude set.\n\r", ch);
	    return TRUE;
	}

    send_to_char( "Syntax:  windmag <type>\n\r"
		  "Type '? windmag' for a list of flags.\n\r", ch );
    return FALSE;
}

AEDIT( aedit_wind_dir )
{
    AREA_DATA *pArea;
    int value = -1;

    if ( argument[0] != '\0' )
    {
	EDIT_AREA(ch, pArea);
        if (!str_prefix(argument, "north")) 		value = 0;
	else if (!str_prefix(argument, "east"))		value = 1;
	else if (!str_prefix(argument, "south"))	value = 2;
	else if (!str_prefix(argument, "west"))		value = 3;
	else if (!str_prefix(argument, "up"))		value = 4;
	else if (!str_prefix(argument, "down"))		value = 5;

	if (value >= 0)
	{
	    pArea->base_wind_dir = value;
	    send_to_char("Base wind direction set.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char( "Syntax:  winddir <dir>\n\r", ch);
    return FALSE;
}

AEDIT( aedit_geography )
{
    AREA_DATA *pArea;
    int value;

    if ( argument[0] != '\0' )
	if ((value = flag_value(geo_flags, argument)) != NO_FLAG)
	{
	    EDIT_AREA(ch, pArea);
	    pArea->geography ^= value;
	    send_to_char("Geography set.\n\r", ch);
	    return TRUE;
	}

    send_to_char( "Syntax:  geography <type>\n\r"
		  "Type '? geography' for a list of flags.\n\r", ch );
    return FALSE;
}
	
AEDIT( aedit_herbs )
{
    AREA_DATA *pArea;
    char arg[MAX_STRING_LENGTH];
    bool hFound = FALSE;
    int herb;

    EDIT_AREA(ch, pArea);

    while (argument[0] != '\0')    
    {
	argument = one_argument(argument, arg);

	for (herb = 0; herb_table[herb].name; herb++)
	{
	    if (!str_cmp(arg, herb_table[herb].name))
	    {
		if (IS_SET(pArea->herbs, herb_table[herb].bit))
		    REMOVE_BIT(pArea->herbs, herb_table[herb].bit);
		else
		    SET_BIT(pArea->herbs, herb_table[herb].bit);
		hFound = TRUE;
		break;
	    }
	}
    }

    if (!hFound)
    {
	send_to_char( "Syntax:  herbs <type>\n\r"
		      "Type '? herbs' for a list of herbs.\n\r", ch );
	return FALSE;
    }

    return TRUE;
}

/*
 * Room Editor Functions.
 */
REDIT( redit_show )
{
    ROOM_INDEX_DATA	*pRoom;
    char		buf  [MAX_STRING_LENGTH];
    char		buf1 [2*MAX_STRING_LENGTH];
    OBJ_DATA		*obj;
    CHAR_DATA		*rch;
    int			door, temp;
    bool		fcnt;
    
    EDIT_ROOM(ch, pRoom);

    buf1[0] = '\0';
    
    sprintf( buf, "Description:\n\r%s", pRoom->description );
    strcat( buf1, buf );

    sprintf( buf, "Name:       [%s]\n\rArea:       [%5d] %s\n\r",
	    pRoom->name, pRoom->area->vnum, pRoom->area->name );
    strcat( buf1, buf );

    sprintf( buf, "Vnum:       [%5d]\n\rSector:     [%s]\n\r",
	    pRoom->vnum, flag_string( sector_flags, pRoom->sector_type ) );
    strcat( buf1, buf );

    sprintf( buf, "Room flags: [%s]\n\r",
	    flag_string( room_flags, pRoom->room_flags ) );
    strcat( buf1, buf );

	sprintf(buf, "Altars to:");
	for (temp = 0; temp < MAX_GODS; temp++)
	{
		if (IS_SET(pRoom->gods_altar, 1 << temp))
		{
			strcat(buf, " ");
			strcat(buf, god_table[temp].name);
		}
	}
	strcat(buf, "\n\r");
	strcat(buf1, buf);

    sprintf( buf, "Health recovery:[%d]\n\rMana recovery  :[%d]\n\r",
            pRoom->heal_rate , pRoom->mana_rate );
    strcat( buf1, buf );

    sprintf(buf, "Move recovery  :[%d]\n\r", pRoom->move_rate);
    strcat( buf1, buf);

       
    sprintf( buf, "Clan:       [%d] %s\n\r" , pRoom->clan ,
    ((pRoom->clan > 0) ? clan_table[pRoom->clan].name : "none" ));
    strcat( buf1, buf );

    if (pRoom->owner && pRoom->owner[0] != '\0')
    {
	sprintf(buf, "Owner:      [%s]\n\r", pRoom->owner);
	strcat(buf1, buf);
    }

    // Weave-related
    sprintf(buf, "Fount: [Frequency: %s] [Order Bias: %d] [Positive Bias: %d]\n",
        Fount::NameFor(pRoom->fount_frequency), pRoom->fount_order_power, pRoom->fount_positive_power);
    strcat(buf1, buf);

    // Shade-related
    sprintf(buf, "Shades: [Density: %s] [Power: %s]\n", Shades::NameFor(pRoom->shade_density), Shades::NameFor(pRoom->shade_power));
    strcat(buf1, buf);

    // Earth-related
    if (pRoom->stone_type >= 0 && pRoom->stone_type < MAX_MATERIALS && material_table[pRoom->stone_type].stone)
        sprintf(buf, "Stone: %s\n", material_table[pRoom->stone_type].name);
    else
        sprintf(buf, "Stone: none\n");
    strcat(buf1, buf);
    
    if ( pRoom->extra_descr )
    {
	EXTRA_DESCR_DATA *ed;

	strcat( buf1, "Desc Kwds:  [" );
	for ( ed = pRoom->extra_descr; ed; ed = ed->next )
	{
	    strcat( buf1, ed->keyword );
	    if ( ed->next )
		strcat( buf1, " " );
	}
	strcat( buf1, "]\n\r" );
    }

    strcat( buf1, "Characters: [" );
    fcnt = FALSE;
    for ( rch = pRoom->people; rch; rch = rch->next_in_room )
    {
	one_argument( rch->name, buf );
	strcat( buf1, buf );
	strcat( buf1, " " );
	fcnt = TRUE;
    }

    if ( fcnt )
    {
	int end;

	end = strlen(buf1) - 1;
	buf1[end] = ']';
	strcat( buf1, "\n\r" );
    }
    else
	strcat( buf1, "none]\n\r" );

    strcat( buf1, "Objects:    [" );
    fcnt = FALSE;
    for ( obj = pRoom->contents; obj; obj = obj->next_content )
    {
	one_argument( obj->name, buf );
	strcat( buf1, buf );
	strcat( buf1, " " );
	fcnt = TRUE;
    }

    if ( fcnt )
    {
	int end;

	end = strlen(buf1) - 1;
	buf1[end] = ']';
	strcat( buf1, "\n\r" );
    }
    else
	strcat( buf1, "none]\n\r" );

    sprintf(buf, "Danger:     [%3d]", pRoom->danger);
    strcat(buf1, buf);
    strcat(buf1, "\n\r");

    for ( door = 0; door < MAX_DIR; door++ )
    {
	EXIT_DATA *pexit;

	if ( ( pexit = pRoom->exit[door] ) )
	{
	    sprintf( buf, "-%-5s to [%5d] Key: [%5d] Exit flags: [%s]\n\r",
		capitalize(dir_name[door]),
		pexit->u1.to_room ? pexit->u1.to_room->vnum : 0,      /* ROM OLC */
		pexit->key,
		flag_string(exit_flags, pexit->rs_flags) );
	    strcat( buf1, buf );

	    /*
	     * Format up the exit info.
	     * Capitalize all flags that are not part of the reset info.
	     *
	    strcpy( reset_state, flag_string( exit_flags, pexit->rs_flags ) );
	    state = flag_string( exit_flags, pexit->exit_info );
	    strcat( buf1, " Exit flags: [" );
	    for (; ;)
	    {
		state = one_argument( state, word );

		if ( word[0] == '\0' )
		{
		    int end;

		    end = strlen(buf1) - 1;
		    buf1[end] = ']';
		    strcat( buf1, "\n\r" );
		    break;
		}

		if ( str_infix( word, reset_state ) )
		{
		    length = strlen(word);
		    for (i = 0; i < length; i++)
			word[i] = UPPER(word[i]);
		}
		strcat( buf1, word );
		strcat( buf1, " " );
	    }
	    */

	    if ( pexit->keyword && pexit->keyword[0] != '\0' )
	    {
		sprintf( buf, "Kwds: [%s]\n\r", pexit->keyword );
		strcat( buf1, buf );
	    }
	    if ( pexit->description && pexit->description[0] != '\0' )
	    {
		sprintf( buf, "%s", pexit->description );
		strcat( buf1, buf );
	    }
	}
    }

    send_to_char( buf1, ch );
    return FALSE;
}




/* Local function. */
bool change_exit( CHAR_DATA *ch, char *argument, int door )
{
    ROOM_INDEX_DATA *pRoom;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int  value;

    EDIT_ROOM(ch, pRoom);

    /*
     * Set the exit flags, needs full argument.
     * ----------------------------------------
     */
    if ( ( value = flag_value( exit_flags, argument ) ) != NO_FLAG )
    {
	ROOM_INDEX_DATA *pToRoom;
	int rev;                                    /* ROM OLC */

	if ( !pRoom->exit[door] )
	   {
	   	send_to_char("Door does not exist.\n\r",ch);
	   	return FALSE;
	   }
	 /*   pRoom->exit[door] = new_exit(); */

	/*
	 * This room.
	 */
	TOGGLE_BIT(pRoom->exit[door]->rs_flags,  value);
	/* Don't toggle exit_info because it can be changed by players. */
	pRoom->exit[door]->exit_info = pRoom->exit[door]->rs_flags;

	/*
	 * Connected room.
	 */
	pToRoom = pRoom->exit[door]->u1.to_room;     /* ROM OLC */
	rev = rev_dir[door];

	if (pToRoom->exit[rev] != NULL && value != EX_FAKE)
	{
	    if (IS_SET(pRoom->exit[door]->rs_flags, value))
		SET_BIT(pToRoom->exit[rev]->rs_flags, value);
	    else
		REMOVE_BIT(pToRoom->exit[rev]->rs_flags, value);
	   //TOGGLE_BIT(pToRoom->exit[rev]->exit_info, value);
	   //pToRoom->exit[rev]->exit_info = pRoom->exit[door]->rs_flags;
	}

	send_to_char( "Exit flag toggled.\n\r", ch );
	return TRUE;
    }

    /*
     * Now parse the arguments.
     */
    argument = one_argument( argument, command );
    one_argument( argument, arg );

    if ( command[0] == '\0' && argument[0] == '\0' )	/* Move command. */
    {
	move_char( ch, door, TRUE );                    /* ROM OLC */
	return FALSE;
    }

    if ( command[0] == '?' )
    {
	do_help( ch, "EXIT" );
	return FALSE;
    }

    if ( !str_cmp( command, "delete" ) )
    {
	ROOM_INDEX_DATA *pToRoom;
	int rev;                                     /* ROM OLC */
	
	if ( !pRoom->exit[door] )
	{
	    send_to_char( "REdit:  Cannot delete a null exit.\n\r", ch );
	    return FALSE;
	}

	/*
	 * Remove ToRoom Exit.
	 */
	rev = rev_dir[door];
	pToRoom = pRoom->exit[door]->u1.to_room;       /* ROM OLC */
	
	if ( pToRoom->exit[rev] )
	{
	    free_exit( pToRoom->exit[rev] );
	    pToRoom->exit[rev] = NULL;
	}

	/*
	 * Remove this exit.
	 */
	free_exit( pRoom->exit[door] );
	pRoom->exit[door] = NULL;

	send_to_char( "Exit unlinked.\n\r", ch );
	return TRUE;
    }

    if ( !str_cmp( command, "link" ) )
    {
	EXIT_DATA *pExit;

	if ( arg[0] == '\0' || !is_number( arg ) )
	{
	    send_to_char( "Syntax:  [direction] link [vnum]\n\r", ch );
	    return FALSE;
	}

	value = atoi( arg );

	if ( !get_room_index( value ) )
	{
	    send_to_char( "REdit:  Cannot link to non-existant room.\n\r", ch );
	    return FALSE;
	}

	if ( !IS_BUILDER( ch, get_room_index( value )->area ) )
	{
	    send_to_char( "REdit:  Cannot link to that area.\n\r", ch );
	    return FALSE;
	}

	if ( get_room_index( value )->exit[rev_dir[door]] )
	{
	    send_to_char( "REdit:  Remote side's exit already exists.\n\r", ch );
	    return FALSE;
	}

	if ( !pRoom->exit[door] )
	{
	    pRoom->exit[door] = new_exit();
	}

	pRoom->exit[door]->u1.to_room = get_room_index( value );   /* ROM OLC */
	pRoom->exit[door]->orig_door = door;
	
/*	pRoom->exit[door]->vnum = value;                Can't set vnum in ROM */

	pRoom                   = get_room_index( value );
	door                    = rev_dir[door];
	pExit                   = new_exit();
	pExit->u1.to_room       = ch->in_room;
/*	pExit->vnum             = ch->in_room->vnum;    Can't set vnum in ROM */
	pExit->orig_door	= door;
	pRoom->exit[door]       = pExit;

	send_to_char( "Two-way link established.\n\r", ch );
	return TRUE;
    }
        
    if ( !str_cmp( command, "dig" ) )
    {
	char buf[MAX_STRING_LENGTH];
	
	if ( arg[0] == '\0' || !is_number( arg ) )
	{
	    send_to_char( "Syntax: [direction] dig <vnum>\n\r", ch );
	    return FALSE;
	}
	
	redit_create( ch, arg );
	sprintf( buf, "link %s", arg );
	change_exit( ch, buf, door);
	return TRUE;
    }

    if ( !str_cmp( command, "room" ) )
    {
	if ( arg[0] == '\0' || !is_number( arg ) )
	{
	    send_to_char( "Syntax:  [direction] room [vnum]\n\r", ch );
	    return FALSE;
	}

	if ( !pRoom->exit[door] )
	{
	    pRoom->exit[door] = new_exit();
	}

	value = atoi( arg );

	if ( !get_room_index( value ) )
	{
	    send_to_char( "REdit:  Cannot link to non-existant room.\n\r", ch );
	    return FALSE;
	}

	pRoom->exit[door]->u1.to_room = get_room_index( value );    /* ROM OLC */
	pRoom->exit[door]->orig_door = door;
/*	pRoom->exit[door]->vnum = value;                 Can't set vnum in ROM */

	send_to_char( "One-way link established.\n\r", ch );
	return TRUE;
    }

    if ( !str_cmp( command, "key" ) )
    {
	if ( arg[0] == '\0' || !is_number( arg ) )
	{
	    send_to_char( "Syntax:  [direction] key [vnum]\n\r", ch );
	    return FALSE;
	}

	if ( !pRoom->exit[door] )
	   {
	   	send_to_char("Door does not exist.\n\r",ch);
	   	return FALSE;
	   }

/*	if ( !pRoom->exit[door] )
	{
	    pRoom->exit[door] = new_exit();
	} */

	value = atoi( arg );

	if ( !get_obj_index( value ) )
	{
	    send_to_char( "REdit:  Item doesn't exist.\n\r", ch );
	    return FALSE;
	}

	pRoom->exit[door]->key = value;

	send_to_char( "Exit key set.\n\r", ch );
	return TRUE;
    }

    if ( !str_cmp( command, "name" ) )
    {
	if ( arg[0] == '\0' )
	{
	    send_to_char( "Syntax:  [direction] name [string]\n\r", ch );
	    return FALSE;
	}

	if ( !pRoom->exit[door] )
	   {
	   	send_to_char("Door does not exist.\n\r",ch);
	   	return FALSE;
	   }

/*	if ( !pRoom->exit[door] )
	{
	    pRoom->exit[door] = new_exit();
	} */

	free_string( pRoom->exit[door]->keyword );
	pRoom->exit[door]->keyword = str_dup( arg );

	send_to_char( "Exit name set.\n\r", ch );
	return TRUE;
    }

    if ( !str_prefix( command, "description" ) )
    {
	if ( arg[0] == '\0' )
	{
	   if ( !pRoom->exit[door] )
	   {
	   	send_to_char("Door does not exist.\n\r",ch);
	   	return FALSE;
	   }

/*	    if ( !pRoom->exit[door] )
	    {
	        pRoom->exit[door] = new_exit();
	    } */

	    string_append( ch, &pRoom->exit[door]->description );
	    return TRUE;
	}

	send_to_char( "Syntax:  [direction] desc\n\r", ch );
	return FALSE;
    }

    return FALSE;
}



REDIT( redit_north )
{
    if ( change_exit( ch, argument, DIR_NORTH ) )
	return TRUE;

    return FALSE;
}



REDIT( redit_south )
{
    if ( change_exit( ch, argument, DIR_SOUTH ) )
	return TRUE;

    return FALSE;
}



REDIT( redit_east )
{
    if ( change_exit( ch, argument, DIR_EAST ) )
	return TRUE;

    return FALSE;
}



REDIT( redit_west )
{
    if ( change_exit( ch, argument, DIR_WEST ) )
	return TRUE;

    return FALSE;
}



REDIT( redit_up )
{
    if ( change_exit( ch, argument, DIR_UP ) )
	return TRUE;

    return FALSE;
}



REDIT( redit_down )
{
    if ( change_exit( ch, argument, DIR_DOWN ) )
	return TRUE;

    return FALSE;
}


REDIT( redit_ed )
{
    ROOM_INDEX_DATA *pRoom;
    EXTRA_DESCR_DATA *ed;
    char command[MAX_INPUT_LENGTH];
    char keyword[MAX_INPUT_LENGTH];

    EDIT_ROOM(ch, pRoom);

    argument = one_argument( argument, command );
    one_argument( argument, keyword );

    if ( command[0] == '\0' || keyword[0] == '\0' )
    {
	send_to_char( "Syntax:  ed add [keyword]\n\r", ch );
	send_to_char( "         ed edit [keyword]\n\r", ch );
	send_to_char( "         ed delete [keyword]\n\r", ch );
	send_to_char( "         ed format [keyword]\n\r", ch );
	return FALSE;
    }

    if ( !str_cmp( command, "add" ) )
    {
	if ( keyword[0] == '\0' )
	{
	    send_to_char( "Syntax:  ed add [keyword]\n\r", ch );
	    return FALSE;
	}

	ed			=   new_extra_descr();
	ed->keyword		=   str_dup( keyword );
	ed->description		=   str_dup( "" );
	ed->next		=   pRoom->extra_descr;
	pRoom->extra_descr	=   ed;

	string_append( ch, &ed->description );

	return TRUE;
    }


    if ( !str_cmp( command, "edit" ) )
    {
	if ( keyword[0] == '\0' )
	{
	    send_to_char( "Syntax:  ed edit [keyword]\n\r", ch );
	    return FALSE;
	}

	for ( ed = pRoom->extra_descr; ed; ed = ed->next )
	{
	    if ( is_name( keyword, ed->keyword ) )
		break;
	}

	if ( !ed )
	{
	    send_to_char( "REdit:  Extra description keyword not found.\n\r", ch );
	    return FALSE;
	}

	string_append( ch, &ed->description );

	return TRUE;
    }


    if ( !str_cmp( command, "delete" ) )
    {
	EXTRA_DESCR_DATA *ped = NULL;

	if ( keyword[0] == '\0' )
	{
	    send_to_char( "Syntax:  ed delete [keyword]\n\r", ch );
	    return FALSE;
	}

	for ( ed = pRoom->extra_descr; ed; ed = ed->next )
	{
	    if ( is_name( keyword, ed->keyword ) )
		break;
	    ped = ed;
	}

	if ( !ed )
	{
	    send_to_char( "REdit:  Extra description keyword not found.\n\r", ch );
	    return FALSE;
	}

	if ( !ped )
	    pRoom->extra_descr = ed->next;
	else
	    ped->next = ed->next;

	free_extra_descr( ed );

	send_to_char( "Extra description deleted.\n\r", ch );
	return TRUE;
    }


    if ( !str_cmp( command, "format" ) )
    {
	if ( keyword[0] == '\0' )
	{
	    send_to_char( "Syntax:  ed format [keyword]\n\r", ch );
	    return FALSE;
	}

	for ( ed = pRoom->extra_descr; ed; ed = ed->next )
	{
	    if ( is_name( keyword, ed->keyword ) )
		break;
	}

	if ( !ed )
	{
	    send_to_char( "REdit:  Extra description keyword not found.\n\r", ch );
	    return FALSE;
	}

	ed->description = format_string( ed->description );

	send_to_char( "Extra description formatted.\n\r", ch );
	return TRUE;
    }

    redit_ed( ch, "" );
    return FALSE;
}



REDIT( redit_create )
{
    AREA_DATA *pArea;
    ROOM_INDEX_DATA *pRoom;
    int value;
    int iHash;
    
    EDIT_ROOM(ch, pRoom);

    value = atoi( argument );

    if ( argument[0] == '\0' || value <= 0 )
    {
	send_to_char( "Syntax:  create [vnum > 0]\n\r", ch );
	return FALSE;
    }

    pArea = get_vnum_area( value );
    if ( !pArea )
    {
	send_to_char( "REdit:  That vnum is not assigned an area.\n\r", ch );
	return FALSE;
    }

    if ( !IS_BUILDER( ch, pArea ) )
    {
	send_to_char( "REdit:  Vnum in an area you cannot build in.\n\r", ch );
	return FALSE;
    }

    if ( get_room_index( value ) )
    {
	send_to_char( "REdit:  Room vnum already exists.\n\r", ch );
	return FALSE;
    }

    pRoom			= new_room_index();
    pRoom->area			= pArea;
    pRoom->vnum			= value;

    if ( value > top_vnum_room )
        top_vnum_room = value;

    iHash			= value % MAX_KEY_HASH;
    pRoom->next			= room_index_hash[iHash];
    room_index_hash[iHash]	= pRoom;
    ch->desc->pEdit		= (void *)pRoom;

    send_to_char( "Room created.\n\r", ch );
    return TRUE;
}

REDIT( redit_name )
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  name [name]\n\r", ch );
	return FALSE;
    }

    free_string( pRoom->name );
    pRoom->name = str_dup( argument );

    send_to_char( "Name set.\n\r", ch );
    return TRUE;
}


REDIT( redit_owner )
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if (argument[0] == '\0')
    {
	if (!pRoom->owner || pRoom->owner[0] == '\0')
	{
	    send_to_char("Syntax: owner <owner>\n\r", ch);
	    return FALSE;
	}
	else
	{
	    free_string(pRoom->owner);
	    send_to_char("Room owner removed.\n\r", ch);
	    return TRUE;
	}
    }

    free_string(pRoom->owner);
    pRoom->owner = str_dup(argument);

    send_to_char("Owner set.\n\r", ch);
    return TRUE;
}

REDIT( redit_desc )
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if ( argument[0] == '\0' )
    {
	string_append( ch, &pRoom->description );
	return TRUE;
    }

    send_to_char( "Syntax:  desc\n\r", ch );
    return FALSE;
}

REDIT( redit_prog )
{
    ROOM_INDEX_DATA *pRoom;
    MPROG_DATA *ed;
    char command[MAX_INPUT_LENGTH];
    char progtype[MAX_INPUT_LENGTH];
    char progarg[MAX_INPUT_LENGTH];

    EDIT_ROOM(ch, pRoom);

    argument = one_argument( argument, command );
    argument = one_argument( argument, progtype );
    strcpy( progarg, argument );

    if ( command[0] == '\0' )
    {
        send_to_char( "Syntax:  prog add [progtype] [progarg]\n\r", ch );
        send_to_char( "         prog delete [progtype] [progarg]\n\r", ch);
        send_to_char( "         prog edit [progtype] [progarg]\n\r", ch );
        send_to_char( "		prog prioritize [progtype] [progarg]\n\r", ch);
        return FALSE;
    }

    if ( !str_cmp( command, "add" ) )
    {
        if ( progtype[0] == '\0' || progarg[0] == '\0' )
        {
            send_to_char( "Syntax:  prog add [progtype] [progarg]\n\r", ch);
            return FALSE;
        }

        if ( mprog_name_to_type( progtype ) == ERROR_PROG )
        {
          send_to_char( "Invalid progtype.\n\r", ch );
          return FALSE;
        }

        ed                  =   (MPROG_DATA *)alloc_perm( sizeof( MPROG_DATA ));
	g_num_progs++;
        ed->type            =   mprog_name_to_type( progtype );
        ed->next            =   pRoom->mobprogs;
        ed->arglist         =   str_dup( progarg );
		determine_exact_match(ed);

	if ((ed->type == RAND_PROG) && !IS_SET(pRoom->progtypes, RAND_PROG))
	{
	    pRoom->next_rand = room_rand_first;
	    room_rand_first = pRoom;
	}

        pRoom->mobprogs      =   ed;
        pRoom->progtypes     =   pRoom->progtypes | ed->type;

        string_append( ch, &ed->comlist );

        return TRUE;
    }

    if (!str_cmp(command, "view"))
    {
        do_prog_view(ch, pRoom->mobprogs, progtype, progarg);
        return FALSE;
    }

    if ( !str_cmp( command, "edit" ) )
    {
        if ( progtype[0] == '\0' || progarg[0] == '\0' )
        {
            send_to_char( "Syntax:  prog edit [progtype] [progarg]\n\r", ch );
            return FALSE;
        }

        for ( ed = pRoom->mobprogs; ed; ed = ed->next )
        {
            if ( !str_cmp(progarg, ed->arglist)  &&
                 mprog_name_to_type(progtype) == ed->type )
                break;
        }

        if ( !ed )
        {
            send_to_char( "REdit:  Room program not found.\n\r", ch);
            return FALSE;
        }

        string_append( ch, &ed->comlist );

        return TRUE;
    }

    if ( !str_cmp( command, "delete" ) )
    {
        MPROG_DATA *ped = NULL;
	int ptype;

        if ( progtype[0] == '\0' || progarg[0] == '\0' )
        {
            send_to_char( "Syntax:  prog delete [progtype] [progarg]\n\r",ch );
            return FALSE;
        }

	ptype = mprog_name_to_type(progtype);

        for ( ed = pRoom->mobprogs; ed; ed = ed->next )
        {
            if (is_name(progarg, ed->arglist) && (ptype == ed->type))
                break;
            ped = ed;
        }

        if ( !ed )
        {
            send_to_char( "REdit:  Room program not found.\n\r", ch);
            return FALSE;
        }

        if ( !ped )
            pRoom->mobprogs = ed->next;
        else
            ped->next = ed->next;

        free_string( ed->arglist );
        free_string( ed->comlist );
        pRoom->progtypes = 0;
        for ( ed = pRoom->mobprogs; ed; ed = ed->next )
        {
          pRoom->progtypes = pRoom->progtypes | ed->type;
        }

	if ((ptype == RAND_PROG) && !IS_SET(pRoom->progtypes, RAND_PROG))
	{
	    ROOM_INDEX_DATA *tRoom;

	    if (room_rand_first == pRoom)
		room_rand_first = pRoom->next_rand;
	    else
	    {
		for (tRoom = room_rand_first; tRoom; tRoom = tRoom->next_rand)
		    if (tRoom->next_rand == pRoom)
		    {
		        tRoom->next_rand = pRoom->next_rand;
		        break;
		    }
	    }

	    pRoom->next_rand = NULL;
	}

        send_to_char( "Room program deleted.\n\r", ch );
        return TRUE;
    }

    if ( !str_cmp(command, "prioritize") )
    {
        MPROG_DATA *ped = NULL;

        if ( progtype[0] == '\0' || progarg[0] == '\0' )
        {
            send_to_char( "Syntax:  prog prioritize [progtype] [progarg]\n\r",ch );
            return FALSE;
        }

        for ( ed = pRoom->mobprogs; ed; ed = ed->next )
        {
            if ( is_name( progarg, ed->arglist )  &&
                 mprog_name_to_type(progtype) == ed->type )
                break;
            ped = ed;
        }
	
        if ( !ed )
        {
            send_to_char( "REdit:  Room program not found.\n\r", ch);
            return FALSE;
        }

        if ( !ped )
	{
	    send_to_char("REdit: Prog already has priority.\n\r", ch);
	    return FALSE;
	}

        ped->next = ed->next;
	ed->next = pRoom->mobprogs;
	pRoom->mobprogs = ed;
	send_to_char("Mobile program prioritized.\n\r", ch);
	return TRUE;
    }

    redit_prog( ch, "" );
    return FALSE;
}


REDIT( redit_danger )
{
    ROOM_INDEX_DATA *pRoom;
    int value;
    
    EDIT_ROOM(ch, pRoom);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char( "Syntax: danger [#danger]\n\r", ch);
	return FALSE;
    }

    value = atoi( argument );

    if (value < 0 || value > 10)
    {
	send_to_char( "Danger must be in the range of 0-10.\n\r", ch);
	return FALSE;
    }
    
    pRoom->danger = value;
    send_to_char ( "Danger set.\n\r", ch);

    return TRUE;
}       

REDIT(redit_stone)
{
    ROOM_INDEX_DATA * pRoom;
    EDIT_ROOM(ch, pRoom);
    return do_edit_stone(ch, argument, pRoom);
}

REDIT(redit_fount)
{
    ROOM_INDEX_DATA * pRoom;
    EDIT_ROOM(ch, pRoom);
    char arg[MAX_STRING_LENGTH];

    // Show help if nothing entered
    if (argument[0] == '\0')
    {
        std::ostringstream mess;
        mess << "Syntax: fount <frequency> <order_power> <positive_power>\n";
        mess << "Frequency options are:";
        for (unsigned int i(0); i < Fount::Max; ++i)
            mess << " " << Fount::NameFor(static_cast<Fount::Frequency>(i));
        mess << "\nOrder/Positive power range is " << Fount::PowerMin << " to " << Fount::PowerMax << "\n";
        send_to_char(mess.str().c_str(), ch);
        return false;
    }

    // Get and set the values (note that bad frequency arguments resolve to 'default')
    argument = one_argument(argument, arg); pRoom->fount_frequency = Fount::ValueFor(arg);
    argument = one_argument(argument, arg); pRoom->fount_order_power = URANGE(Fount::PowerMin, atoi(arg), Fount::PowerMax);
    argument = one_argument(argument, arg); pRoom->fount_positive_power = URANGE(Fount::PowerMin, atoi(arg), Fount::PowerMax);

    // Inform the builder
    std::ostringstream mess;
    mess << "Fount values set to [Frequency: " << Fount::NameFor(pRoom->fount_frequency);
    mess << "] [Order Power: " << pRoom->fount_order_power << "] [Positive Power: " << pRoom->fount_positive_power << "]\n";
    send_to_char(mess.str().c_str(), ch);
    return true;
}

REDIT(redit_shades)
{
    ROOM_INDEX_DATA * pRoom;
    EDIT_ROOM(ch, pRoom);
    char arg[MAX_STRING_LENGTH];

    // Show help if nothing entered
    if (argument[0] == '\0')
    {
        std::ostringstream mess;
        mess << "Syntax: shades <density> <power>\n";
        mess << "Density options are:";
        for (unsigned int i(0); i < Shades::MaxDensity; ++i)
            mess << " " << Shades::NameFor(static_cast<Shades::Density>(i));

        mess << "\nPower options are:";
        for (unsigned int i(0); i < Shades::MaxPower; ++i)
            mess << " " << Shades::NameFor(static_cast<Shades::Power>(i));

        mess << "\n";
        send_to_char(mess.str().c_str(), ch);
        return false;
    }

    // Get and set the values (note that bad arguments resolve to 'default')
    argument = one_argument(argument, arg); pRoom->shade_density = Shades::DensityFor(arg);
    argument = one_argument(argument, arg); pRoom->shade_power = Shades::PowerFor(arg);

    // Inform the builder
    std::ostringstream mess;
    mess << "Shade values set to [Density: " << Shades::NameFor(pRoom->shade_density) << "] [Power:";
    mess << Shades::NameFor(pRoom->shade_power) << "]\n";
    send_to_char(mess.str().c_str(), ch);
    return true;
}

REDIT( redit_heal )
{
    ROOM_INDEX_DATA *pRoom;
    
    EDIT_ROOM(ch, pRoom);
    
    if (is_number(argument))
       {
          pRoom->heal_rate = atoi ( argument );
          send_to_char ( "Heal rate set.\n\r", ch);
          return TRUE;
       }

    send_to_char ( "Syntax : heal <#xnumber>\n\r", ch);
    return FALSE;
}       

REDIT( redit_move )
{
    ROOM_INDEX_DATA *pRoom;
    
    EDIT_ROOM(ch, pRoom);
    
    if (is_number(argument))
       {
          pRoom->move_rate = atoi ( argument );
          send_to_char ( "Move rate set.\n\r", ch);
          return TRUE;
       }

    send_to_char ( "Syntax : move <#xnumber>\n\r", ch);
    return FALSE;
}       

REDIT( redit_altars )
{
	int i;
	ROOM_INDEX_DATA *pRoom;
	EDIT_ROOM(ch, pRoom);
	
	if (argument[0] == '\0')
	{
		send_to_char("Syntax : altars <god_name>\n\r", ch);
		return FALSE;
	}
	
	for (i = 0; i < MAX_GODS; i++)
	{
		if (!str_prefix(argument, god_table[i].name))
		{
			TOGGLE_BIT(pRoom->gods_altar, 1 << i);
			send_to_char("God altar toggled.\n\r", ch);
			return TRUE;
		}
	}

	send_to_char("Invalid god name.\n\r", ch);
	return FALSE;		
}

REDIT( redit_mana )
{
    ROOM_INDEX_DATA *pRoom;
    
    EDIT_ROOM(ch, pRoom);
    
    if (is_number(argument))
       {
          pRoom->mana_rate = atoi ( argument );
          send_to_char ( "Mana rate set.\n\r", ch);
          return TRUE;
       }

    send_to_char ( "Syntax : mana <#xnumber>\n\r", ch);
    return FALSE;
}       

REDIT( redit_clan )
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);
    
    pRoom->clan = clan_lookup(argument);
    
    send_to_char ( "Clan set.\n\r", ch);
    return TRUE;
}
      
REDIT( redit_format )
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    pRoom->description = format_string( pRoom->description );

    send_to_char( "String formatted.\n\r", ch );
    return TRUE;
}



REDIT( redit_mreset )
{
    ROOM_INDEX_DATA	*pRoom;
    MOB_INDEX_DATA	*pMobIndex;
    CHAR_DATA		*newmob;
    char		arg [ MAX_INPUT_LENGTH ];
    char		arg2 [ MAX_INPUT_LENGTH ];

    RESET_DATA		*pReset;
    char		output [ MAX_STRING_LENGTH ];

    EDIT_ROOM(ch, pRoom);

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( arg[0] == '\0' || !is_number( arg ) )
    {
	send_to_char ( "Syntax:  mreset <vnum> <max #x> <mix #x>\n\r", ch );
	return FALSE;
    }

    if ( !( pMobIndex = get_mob_index( atoi( arg ) ) ) )
    {
	send_to_char( "REdit: No mobile has that vnum.\n\r", ch );
	return FALSE;
    }

    if ( pMobIndex->area != pRoom->area )
    {
	send_to_char( "REdit: No such mobile in this area.\n\r", ch );
	return FALSE;
    }

    /*
     * Create the mobile reset.
     */
    pReset              = new_reset_data();
    pReset->command	= 'M';
    pReset->arg1	= pMobIndex->vnum;
    pReset->arg2	= is_number( arg2 ) ? atoi( arg2 ) : MAX_MOB;
    pReset->arg3	= pRoom->vnum;
    pReset->arg4	= is_number( argument ) ? atoi (argument) : 1;
    add_reset( pRoom, pReset, 0/* Last slot*/ );

    /*
     * Create the mobile.
     */
    newmob = create_mobile( pMobIndex );
    char_to_room( newmob, pRoom );

    sprintf( output, "%s (%d) has been loaded and added to resets.\n\r"
	"There will be a maximum of %d loaded to this room.\n\r",
	capitalize( pMobIndex->short_descr ),
	pMobIndex->vnum,
	pReset->arg2 );
    send_to_char( output, ch );
    act( "$n has created $N!", ch, NULL, newmob, TO_ROOM );
    return TRUE;
}

WEAR_TYPE wear_table[] =
{
    {	WEAR_NONE,	ITEM_TAKE, 		"take"	},
    {	WEAR_LIGHT,	ITEM_LIGHT,		"light" },
    {	WEAR_FINGER_L,	ITEM_WEAR_FINGER,	"finger" },
    {	WEAR_FINGER_R,	ITEM_WEAR_FINGER,	"finger" },
    {	WEAR_NECK_1,	ITEM_WEAR_NECK,		"neck" },
    {	WEAR_NECK_2,	ITEM_WEAR_NECK,		"neck" },
    {	WEAR_BODY,	ITEM_WEAR_BODY,		"torso" },
    {	WEAR_HEAD,	ITEM_WEAR_HEAD,		"head" },
    {	WEAR_LEGS,	ITEM_WEAR_LEGS,		"legs" },
    {	WEAR_FEET,	ITEM_WEAR_FEET,		"feet" },
    {	WEAR_HANDS,	ITEM_WEAR_HANDS,	"hands" },
    {	WEAR_ARMS,	ITEM_WEAR_ARMS,		"arms" },
    {	WEAR_SHIELD,	ITEM_WEAR_SHIELD,	"shield" },
    {	WEAR_ABOUT,	ITEM_WEAR_ABOUT,	"body" },
    {	WEAR_WAIST,	ITEM_WEAR_WAIST,	"waist" },
    {	WEAR_WRIST_L,	ITEM_WEAR_WRIST,	"wrist" },
    {	WEAR_WRIST_R,	ITEM_WEAR_WRIST,	"wrist" },
    {	WEAR_WIELD,	ITEM_WIELD,		"wield" },
    {	WEAR_HOLD,	ITEM_HOLD,		"hold" },
    {	WEAR_FAMILIAR,	ITEM_WEAR_FAMILIAR,		"familiar" },
    {	NO_FLAG,	NO_FLAG,		"no" }
};



/*****************************************************************************
 Name:		wear_loc
 Purpose:	Returns the location of the bit that matches the count.
 		1 = first match, 2 = second match etc.
 Called by:	oedit_reset(olc_act.c).
 ****************************************************************************/
int wear_loc(int bits, int count)
{
    int flag;
 
    for (flag = 0; wear_table[flag].wear_bit != NO_FLAG; flag++)
    {
        if ( IS_SET(bits, wear_table[flag].wear_bit) && --count < 1)
            return wear_table[flag].wear_loc;
    }
 
    return NO_FLAG;
}



/*****************************************************************************
 Name:		wear_bit
 Purpose:	Converts a wear_loc into a bit.
 Called by:	redit_oreset(olc_act.c).
 ****************************************************************************/
int wear_bit(int loc)
{
    int flag;
 
    for (flag = 0; wear_table[flag].wear_loc != NO_FLAG; flag++)
    {
        if ( loc == wear_table[flag].wear_loc )
            return wear_table[flag].wear_bit;
    }
 
    return 0;
}



REDIT( redit_oreset )
{
    ROOM_INDEX_DATA	*pRoom;
    OBJ_INDEX_DATA	*pObjIndex;
    OBJ_DATA		*newobj;
    OBJ_DATA		*to_obj;
    CHAR_DATA		*to_mob;
    char		arg1 [ MAX_INPUT_LENGTH ];
    char		arg2 [ MAX_INPUT_LENGTH ];
    int			olevel = 0;

    RESET_DATA		*pReset;
    char		output [ MAX_STRING_LENGTH ];

    EDIT_ROOM(ch, pRoom);

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || !is_number( arg1 ) )
    {
	send_to_char ( "Syntax:  oreset <vnum> <args>\n\r", ch );
	send_to_char ( "        -no_args               = into room\n\r", ch );
	send_to_char ( "        -<obj_name>            = into obj\n\r", ch );
	send_to_char ( "        -<mob_name> <wear_loc> = into mob\n\r", ch );
	return FALSE;
    }

    if ( !( pObjIndex = get_obj_index( atoi( arg1 ) ) ) )
    {
	send_to_char( "REdit: No object has that vnum.\n\r", ch );
	return FALSE;
    }

    if ( pObjIndex->area != pRoom->area )
    {
	send_to_char( "REdit: No such object in this area.\n\r", ch );
	return FALSE;
    }

    /*
     * Load into room.
     */
    if ( arg2[0] == '\0' )
    {
	pReset		= new_reset_data();
	pReset->command	= 'O';
	pReset->arg1	= pObjIndex->vnum;
	pReset->arg2	= 0;
	pReset->arg3	= pRoom->vnum;
	pReset->arg4	= 0;
	add_reset( pRoom, pReset, 0/* Last slot*/ );

	newobj = create_object( pObjIndex, number_fuzzy( olevel ) );
	obj_to_room( newobj, pRoom );

	sprintf( output, "%s (%d) has been loaded and added to resets.\n\r",
	    capitalize( pObjIndex->short_descr ),
	    pObjIndex->vnum );
	send_to_char( output, ch );
    }
    else
    /*
     * Load into object's inventory.
     */
    if ( argument[0] == '\0'
    && ( ( to_obj = get_obj_list( ch, arg2, pRoom->contents ) ) != NULL ) )
    {
	pReset		= new_reset_data();
	pReset->command	= 'P';
	pReset->arg1	= pObjIndex->vnum;
	pReset->arg2	= 0;
	pReset->arg3	= to_obj->pIndexData->vnum;
	pReset->arg4	= 1;
	add_reset( pRoom, pReset, 0/* Last slot*/ );

	newobj = create_object( pObjIndex, number_fuzzy( olevel ) );
	newobj->cost = 0;
	obj_to_obj( newobj, to_obj );

	sprintf( output, "%s (%d) has been loaded into "
	    "%s (%d) and added to resets.\n\r",
	    capitalize( newobj->short_descr ),
	    newobj->pIndexData->vnum,
	    to_obj->short_descr,
	    to_obj->pIndexData->vnum );
	send_to_char( output, ch );
    }
    else
    /*
     * Load into mobile's inventory.
     */
    if ( ( to_mob = get_char_room( ch, arg2 ) ) != NULL )
    {
	int	wear_loc;

	/*
	 * Make sure the location on mobile is valid.
	 */
	if ( (wear_loc = flag_value( wear_loc_flags, argument )) == NO_FLAG )
	{
	    send_to_char( "REdit: Invalid wear_loc.  '? wear-loc'\n\r", ch );
	    return FALSE;
	}

	/*
	 * Disallow loading a sword(WEAR_WIELD) into WEAR_HEAD.
	 */
	if ( !IS_SET( pObjIndex->wear_flags, wear_bit(wear_loc) ) )
	{
	    sprintf( output,
	        "%s (%d) has wear flags: [%s]\n\r",
	        capitalize( pObjIndex->short_descr ),
	        pObjIndex->vnum,
		flag_string( wear_flags, pObjIndex->wear_flags ) );
	    send_to_char( output, ch );
	    return FALSE;
	}

	/*
	 * Can't load into same position.
	 */
	if ( get_eq_char( to_mob, wear_loc ) )
	{
	    send_to_char( "REdit:  Object already equipped.\n\r", ch );
	    return FALSE;
	}

	pReset		= new_reset_data();
	pReset->arg1	= pObjIndex->vnum;
	pReset->arg2	= wear_loc;
	if ( pReset->arg2 == WEAR_NONE )
	    pReset->command = 'G';
	else
	    pReset->command = 'E';
	pReset->arg3	= wear_loc;

	add_reset( pRoom, pReset, 0/* Last slot*/ );

	olevel  = URANGE( 0, to_mob->level - 2, LEVEL_HERO );
        newobj = create_object( pObjIndex, number_fuzzy( olevel ) );

	if ( to_mob->pIndexData->pShop )	/* Shop-keeper? */
	{
	    switch ( pObjIndex->item_type )
	    {
	    default:		olevel = 0;				break;
	    case ITEM_PILL:	olevel = number_range(  0, 10 );	break;
	    case ITEM_OIL:	olevel = number_range( 	0, 10 );	break;
	    case ITEM_POTION:	olevel = number_range(  0, 10 );	break;
	    case ITEM_SCROLL:	olevel = number_range(  5, 15 );	break;
	    case ITEM_WAND:	olevel = number_range( 10, 20 );	break;
	    case ITEM_STAFF:	olevel = number_range( 15, 25 );	break;
	    case ITEM_ARMOR:	olevel = number_range(  5, 15 );	break;
	    case ITEM_WEAPON:	if ( pReset->command == 'G' )
	    			    olevel = number_range( 5, 15 );
				else
				    olevel = number_fuzzy( olevel );
		break;
	    }

	    newobj = create_object( pObjIndex, olevel );
	    if ( pReset->arg2 == WEAR_NONE )
		SET_BIT( newobj->extra_flags[0], ITEM_INVENTORY );
	}
	else
	    newobj = create_object( pObjIndex, number_fuzzy( olevel ) );

	obj_to_char( newobj, to_mob );

	if ( pReset->command == 'E' )
	{
	    equip_char (to_mob, newobj, (1 << pReset->arg3));
	    oprog_wear_trigger(newobj);
	}

	sprintf( output, "%s (%d) has been loaded "
	    "%s of %s (%d) and added to resets.\n\r",
	    capitalize( pObjIndex->short_descr ),
	    pObjIndex->vnum,
	    flag_string( wear_loc_strings, pReset->arg3 ),
	    to_mob->short_descr,
	    to_mob->pIndexData->vnum );
	send_to_char( output, ch );
    }
    else	/* Display Syntax */
    {
	send_to_char( "REdit:  That mobile isn't here.\n\r", ch );
	return FALSE;
    }

    act( "$n has created $p!", ch, newobj, NULL, TO_ROOM );
    return TRUE;
}

static void AddKey(DisplayPanel::Box & keys, const std::string & line)
{
    if (line.empty())
    {
        keys.AddLine(line);
        return;
    }

    std::ostringstream mess;
    mess << line << ":{x";
    keys.AddLine(mess.str());
}

static void AddValue(DisplayPanel::Box & values, const std::string & line)
{
    std::ostringstream mess;
    mess << "{W" << line << "{x";
    values.AddLine(mess.str());
}

static std::string Bracketize(const std::string & line)
{
    std::ostringstream mess;
    mess << '[';
    mess << line;
    mess << ']';
    return mess.str();
}

static DisplayPanel::HorizontalSplit show_obj_values(const OBJ_INDEX_DATA & obj)
{
    DisplayPanel::Box nums;
    DisplayPanel::Box keys;
    DisplayPanel::Box vals;

    switch (obj.item_type)
    {
        case ITEM_LIGHT:
        {
            nums.AddLine("[v1]");
            AddKey(keys, "Flags");
            AddValue(vals, flag_string(light_flags, obj.value[1]));

            nums.AddLine("[v2]");
            AddKey(keys, "Light");
            if (obj.value[2] == -1 || obj.value[2] == 999) AddValue(vals, "Infinite[-1]");
            else AddValue(vals, makeString(obj.value[2]));

            break;
        }

        case ITEM_WAND:
        case ITEM_STAFF:
            nums.AddLine("[v0]");
            AddKey(keys, "Level");
            AddValue(vals, makeString(obj.value[0]));

            nums.AddLine("[v1]");
            AddKey(keys, "Charges Total");
            AddValue(vals, makeString(obj.value[1]));

            nums.AddLine("[v2]");
            AddKey(keys, "Charges Left");
            AddValue(vals, makeString(obj.value[2]));

            nums.AddLine("[v3]");
            AddKey(keys, "Spell");
            if (obj.value[3] < 0) AddValue(vals, "none");
            else AddValue(vals, skill_table[obj.value[3]].name);
            break;

        case ITEM_PORTAL:
            nums.AddLine("[v0]");
            AddKey(keys, "Charges");
            AddValue(vals, makeString(obj.value[0]));

            nums.AddLine("[v1]");
            AddKey(keys, "Exit Flags");
            AddValue(vals, flag_string(exit_flags, obj.value[1]));
 
            nums.AddLine("[v2]");
            AddKey(keys, "Portal Flags");
            AddValue(vals, flag_string(portal_flags, obj.value[2]));
 
            nums.AddLine("[v3]");
            AddKey(keys, "Goes to Vnum");
            AddValue(vals, makeString(obj.value[3]));
            break;
            
        case ITEM_FURNITURE:          
            nums.AddLine("[v0]");
            AddKey(keys, "Max People");
            AddValue(vals, makeString(obj.value[0]));
            
            nums.AddLine("[v]1");
            AddKey(keys, "Max Weight");
            AddValue(vals, makeString(obj.value[1]));
 
            nums.AddLine("[v2]");
            AddKey(keys, "Furniture Flags");
            AddValue(vals, flag_string(furniture_flags, obj.value[2]));
            
            nums.AddLine("[v3]");
            AddKey(keys, "Heal Bonus");
            AddValue(vals, makeString(obj.value[3]));
            
            nums.AddLine("[v4]");
            AddKey(keys, "Mana Bonus");
            AddValue(vals, makeString(obj.value[4]));
            break;

        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
        case ITEM_OIL:
            nums.AddLine("[v0]");
            AddKey(keys, "Level");
            AddValue(vals, makeString(obj.value[0]));
 
            for (unsigned int i(1); i <= 4; ++i)
            {
                std::ostringstream mess;
                mess << "[v" << i << "]";
                nums.AddLine(mess.str());
                AddKey(keys, "Spell");

                if (obj.value[i] < 0) AddValue(vals, "none");
                else AddValue(vals, skill_table[obj.value[i]].name);
            }
            break;
        
        case ITEM_POTIONCONTAINER:
            nums.AddLine("[v0]");
            AddKey(keys, "Explosive");
            AddValue(vals, makeString(obj.value[0]));
 
            nums.AddLine("[v1]");
            AddKey(keys, "Adhesive");
            AddValue(vals, makeString(obj.value[1]));
            
            nums.AddLine("[v2]");
            AddKey(keys, "Anesthetic");
            AddValue(vals, makeString(obj.value[2]));

            nums.AddLine("[v3]");
            AddKey(keys, "Toxin");
            AddValue(vals, makeString(obj.value[3]));
 
            nums.AddLine("[v4]");
            AddKey(keys, "Suppressive");
            AddValue(vals, makeString(obj.value[4]));
            break;
        

        case ITEM_ARMOR:
            nums.AddLine("[v0]");
            AddKey(keys, "AC Pierce");
            AddValue(vals, makeString(obj.value[0]));

            nums.AddLine("[v1]");
            AddKey(keys, "AC Bash");
            AddValue(vals, makeString(obj.value[1]));
 
            nums.AddLine("[v2]");
            AddKey(keys, "AC Slash");
            AddValue(vals, makeString(obj.value[2]));
 
            nums.AddLine("[v3]");
            AddKey(keys, "AC Exotic");
            AddValue(vals, makeString(obj.value[3]));
            break;

        case ITEM_WEAPON:
            nums.AddLine("[v0]");
            AddKey(keys, "Weapon Type");
            AddValue(vals, flag_string(weapon_class, obj.value[0]));
 
            nums.AddLine("[v1]");
            AddKey(keys, "Dice Count");
            AddValue(vals, makeString(obj.value[1]));
 
            nums.AddLine("[v2]");
            AddKey(keys, "Dice Size");
            AddValue(vals, makeString(obj.value[2]));
 
            nums.AddLine("[v3]");
            AddKey(keys, "Damtype");
            if (obj.value[3] < 0) AddValue(vals, "(null)");
            else AddValue(vals, damtype_table[obj.value[3]].name);
 
            nums.AddLine("[v4]");
            AddKey(keys, "Special");
            AddValue(vals, flag_string(weapon_type2, obj.value[4]));
            break;

        case ITEM_SPECIAL:
            nums.AddLine("[v0]");
            AddKey(keys, "Type");
            AddValue(vals, flag_string(spec_obj_types, obj.value[0]));
            break;

        case ITEM_INSTRUMENT:
            nums.AddLine("[v0]");
            AddKey(keys, "Type");
            AddValue(vals, flag_string(instrument_table, obj.value[0]));
            break;

            case ITEM_BOW:
            nums.AddLine("[v1]");
            AddKey(keys, "Dice Count");
            AddValue(vals, makeString(obj.value[1]));
 
            nums.AddLine("[v2]");
            AddKey(keys, "Dice Size");
            AddValue(vals, makeString(obj.value[2]));
            break;

        case ITEM_ARROW:
            nums.AddLine("[v0]");
            AddKey(keys, "Damtype");
            AddValue(vals, damtype_table[obj.value[0]].name);
 
            nums.AddLine("[v1]");
            AddKey(keys, "Damage %");
            AddValue(vals, makeString(obj.value[1]));
 
            for (unsigned int i(2); i <= 4; ++i)
            {
                std::ostringstream mess;
                mess << "[v" << i << "]";
                nums.AddLine(mess.str());
                AddKey(keys, "Spell");

                if (obj.value[i] < 0) AddValue(vals, "none");
                else AddValue(vals, skill_table[obj.value[i]].name);
            }
            break;

        case ITEM_CONTAINER:
        {
            nums.AddLine("[v0]");
            AddKey(keys, "Weight");
            AddValue(vals, makeString(obj.value[0]));
 
            nums.AddLine("[v1]");
            AddKey(keys, "Flags");
            AddValue(vals, flag_string(container_flags, obj.value[1]));
 
            nums.AddLine("[v2]");
            AddKey(keys, "Key");
            const OBJ_INDEX_DATA * keyIndex(get_obj_index(obj.value[2]));
            if (keyIndex == NULL) 
                AddValue(vals, "none");
            else
            {
                std::ostringstream mess;
                mess << keyIndex->short_descr << " [" << obj.value[2] << ']';
                AddValue(vals, mess.str());
            }
 
            nums.AddLine("[v3]");
            AddKey(keys, "Capacity");
            AddValue(vals, makeString(obj.value[3]));
 
            nums.AddLine("[v4]");
            AddKey(keys, "Weight Multiplier");
            AddValue(vals, makeString(obj.value[4]));
            break;
        }

        case ITEM_DRINK_CON:
            nums.AddLine("[v0]");
            AddKey(keys, "Liquid Total");
            AddValue(vals, makeString(obj.value[0]));
 
            nums.AddLine("[v1]");
            AddKey(keys, "Liquid Left");
            AddValue(vals, makeString(obj.value[1]));
 
            nums.AddLine("[v2]");
            AddKey(keys, "Liquid Type");
            AddValue(vals, liq_table[obj.value[2]].liq_name);
 
            nums.AddLine("[v3]");
            AddKey(keys, "Poisoned");
            AddValue(vals, obj.value[3] == 0 ? "No" : "Yes");

            // Yes/No logic reversed for backward compatibility 
            nums.AddLine("[v4]");
            AddKey(keys, "Refillable");
            AddValue(vals, obj.value[4] == 0 ? "Yes" : "No");
            break;

        case ITEM_FOUNTAIN:
            nums.AddLine("[v0]");
            AddKey(keys, "Liquid Total");
            AddValue(vals, makeString(obj.value[0]));
 
            nums.AddLine("[v1]");
            AddKey(keys, "Liquid Left");
            AddValue(vals, makeString(obj.value[1]));
 
            nums.AddLine("[v2]");
            AddKey(keys, "Liquid Type");
            AddValue(vals, liq_table[obj.value[2]].liq_name);
 
            nums.AddLine("[v3]");
            AddKey(keys, "Empty");
            AddValue(vals, obj.value[3] == 0 ? "False" : "True");
            break;
                
        case ITEM_FOOD:
            nums.AddLine("[v0]");
            AddKey(keys, "Food Ticks");
            AddValue(vals, makeString(obj.value[0]));
 
            nums.AddLine("[v1]");
            AddKey(keys, "Food Quality");
            AddValue(vals, makeString(obj.value[1]));
 
            nums.AddLine("[v3]");
            AddKey(keys, "Poisoned");
            AddValue(vals, obj.value[3] == 0 ? "No" : "Yes");
            break;

        case ITEM_WRITING:
            nums.AddLine("[v0]");
            AddKey(keys, "Page Count");
            AddValue(vals, makeString(obj.value[0]));
 
            nums.AddLine("[v1]");
            AddKey(keys, "Page Characters");
            AddValue(vals, makeString(obj.value[1]));
 
            nums.AddLine("[v2]");
            AddKey(keys, "Writable");
            AddValue(vals, obj.value[2] == 0 ? "No" : "Yes");
            break;

        case ITEM_MONEY:
            nums.AddLine("[v0]");
            AddKey(keys, "Value");
            AddValue(vals, makeString(obj.value[0]));
 
            nums.AddLine("[v1]");
            AddKey(keys, "Type");
            AddValue(vals, coin_table[obj.value[1]].name);
            break;
    }
   
    DisplayPanel::HorizontalSplit result(DisplayPanel::Options(DisplayPanel::Style_None));
    result.Add(nums);
    result.Add(keys);
    result.Add(vals); 
    return result;
}

bool set_obj_values( CHAR_DATA *ch, OBJ_INDEX_DATA *pObj, int value_num, char *argument)
{
    int value;

    switch( pObj->item_type )
    {
        default:
            break;
     
	case ITEM_SPECIAL:
	    switch ( value_num )
	    {
		default:
		    return FALSE;

		case 0:
		    if ((value = flag_value(spec_obj_types, argument)) != NO_FLAG)
		    {
		        send_to_char("Type of special object set.\n\r", ch);
		        pObj->value[0] = value;
		    }
		    else
			send_to_char("Invalid special object type.\n\r", ch);
		    break;
	    }
	    break;

	case ITEM_INSTRUMENT:
	    switch( value_num )
	    {
		default:
		    return FALSE;

		case 0:
		    if ((value = flag_value(instrument_table, argument)) != NO_FLAG)
		    {
			send_to_char("Instrument type set.\n\r", ch);
			pObj->value[0] = value;
		    }
		    else
			send_to_char("Invalid instrument type.\n\r", ch);
		    break;
	    }
	    break;
       
        case ITEM_LIGHT:
	    switch ( value_num )
	    {
	        default:
		    do_help( ch, "ITEM_LIGHT" );
	            return FALSE;
            case 1:
            {
                send_to_char( "LIGHT FLAGS SET.\n\r\n\r", ch);
                int flagVal = flag_value( light_flags, argument );
                pObj->value[1] = (flagVal == NO_FLAG ? 0 : flagVal);
                break;
            }
	        case 2:
	            send_to_char( "HOURS OF LIGHT SET.\n\r\n\r", ch );
	            pObj->value[2] = atoi( argument );
	            break;
	    }
        break;

        case ITEM_WAND:
        case ITEM_STAFF:
	    switch ( value_num )
	    {
	        default:
		    do_help( ch, "ITEM_STAFF_WAND" );
	            return FALSE;
	        case 0:
	            send_to_char( "SPELL LEVEL SET.\n\r\n\r", ch );
	            pObj->value[0] = atoi( argument );
	            break;
	        case 1:
	            send_to_char( "TOTAL NUMBER OF CHARGES SET.\n\r\n\r", ch );
	            pObj->value[1] = atoi( argument );
	            break;
	        case 2:
	            send_to_char( "CURRENT NUMBER OF CHARGES SET.\n\r\n\r", ch );
	            pObj->value[2] = atoi( argument );
	            break;
	        case 3:
	            send_to_char( "SPELL TYPE SET.\n\r", ch );
	            pObj->value[3] = skill_lookup( argument );
	            break;
	    }
            break;

	case ITEM_WRITING:
	    switch ( value_num )
	    {
		case 0:
		    send_to_char("Number of pages set.\n\r\n\r", ch);
		    pObj->value[0] = atoi(argument);
		    break;

		case 1:
		    send_to_char("Characters per page set.\n\r\n\r", ch);
		    pObj->value[1] = atoi(argument);
		    break;

	        case 2:
	            send_to_char( "Writable value toggled.\n\r\n\r", ch );
	            pObj->value[2] = ( pObj->value[2] == 0 ) ? 1 : 0;
	            break;

		default:
		    return FALSE;
	    }
	    break;

        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
	case ITEM_OIL:
	    switch ( value_num )
	    {
	        default:
		    do_help( ch, "ITEM_SCROLL_POTION_PILL" );
	            return FALSE;
	        case 0:
	            send_to_char( "SPELL LEVEL SET.\n\r\n\r", ch );
	            pObj->value[0] = atoi( argument );
	            break;
	        case 1:
	            send_to_char( "SPELL TYPE 1 SET.\n\r\n\r", ch );
	            pObj->value[1] = skill_lookup( argument );
	            break;
	        case 2:
	            send_to_char( "SPELL TYPE 2 SET.\n\r\n\r", ch );
	            pObj->value[2] = skill_lookup( argument );
	            break;
	        case 3:
	            send_to_char( "SPELL TYPE 3 SET.\n\r\n\r", ch );
	            pObj->value[3] = skill_lookup( argument );
	            break;
	        case 4:
	            send_to_char( "SPELL TYPE 4 SET.\n\r\n\r", ch );
	            pObj->value[4] = skill_lookup( argument );
	            break;
 	    }
	    break;

/* ARMOR for ROM: */

        case ITEM_ARMOR:
	    switch ( value_num )
	    {
	        default:
		    do_help( ch, "ITEM_ARMOR" );
		    return FALSE;
	        case 0:
		    send_to_char( "AC PIERCE SET.\n\r\n\r", ch );
		    pObj->value[0] = atoi( argument );
		    break;
	        case 1:
		    send_to_char( "AC BASH SET.\n\r\n\r", ch );
		    pObj->value[1] = atoi( argument );
		    break;
	        case 2:
		    send_to_char( "AC SLASH SET.\n\r\n\r", ch );
		    pObj->value[2] = atoi( argument );
		    break;
	        case 3:
		    send_to_char( "AC EXOTIC SET.\n\r\n\r", ch );
		    pObj->value[3] = atoi( argument );
		    break;
	    }
	    break;

/* WEAPONS changed in ROM */

        case ITEM_WEAPON:
	    switch ( value_num )
	    {
	        default:
		    do_help( ch, "ITEM_WEAPON" );
	            return FALSE;
	        case 0:
		    send_to_char( "WEAPON CLASS SET.\n\r\n\r", ch );
		    pObj->value[0] = flag_value( weapon_class, argument );
		    break;
	        case 1:
	            send_to_char( "NUMBER OF DICE SET.\n\r\n\r", ch );
	            pObj->value[1] = atoi( argument );
	            break;
	        case 2:
	            send_to_char( "TYPE OF DICE SET.\n\r\n\r", ch );
	            pObj->value[2] = atoi( argument );
	            break;
	        case 3:
		    value = damtype_lookup(argument);
		    if (value == -1)
		    {
			send_to_char("Invalid damage type.\n\r", ch);
			return FALSE;
		    }
	            send_to_char( "DAMAGE TYPE SET.\n\r\n\r", ch );
	            pObj->value[3] = value;
	            break;
	        case 4:
                    send_to_char( "SPECIAL WEAPON TYPE TOGGLED.\n\r\n\r", ch );
		    pObj->value[4] ^= (flag_value( weapon_type2, argument ) != NO_FLAG
		    ? flag_value( weapon_type2, argument ) : 0 );
		    break;
	    }
            break;

	case ITEM_BOW:
	    switch ( value_num )
	    {
		default:
		    do_help(ch, "ITEM_BOW" );
		    return FALSE;

		case 1:
		    send_to_char("NUMBER OF DICE SET.\n\r\n\r", ch);
		    pObj->value[1] = atoi( argument );
		    break;

		case 2:
		    send_to_char("TYPE OF DICE SET.\n\r\n\r", ch);
		    pObj->value[2] = atoi( argument );
		    break;
	    }
	    break;

        case ITEM_ARROW:
	    switch ( value_num )
	    {
		default:
		    do_help(ch, "ITEM_ARROW" );
		    return FALSE;

		case 0:
		    send_to_char( "WEAPON TYPE SET.\n\r\n\r", ch );
		    pObj->value[0] = attack_lookup( argument );
		    break;

		case 1:
		    send_to_char( "DAMAGE MULTIPLIER SET.\n\r\n\r", ch);
		    pObj->value[1] = atoi( argument );
		    break;

		case 2:
		case 3:
		case 4:
		    send_to_char( "SPELL EFFECT ADDED.\n\r\n\r", ch);
		    pObj->value[value_num] = atoi( argument );
		    break;
	    }
	    break;

	case ITEM_PORTAL:
	    switch ( value_num )
	    {
	        default:
	            do_help(ch, "ITEM_PORTAL" );
	            return FALSE;
	            
	    	case 0:
	    	    send_to_char( "CHARGES SET.\n\r\n\r", ch);
	    	    pObj->value[0] = atoi ( argument );
	    	    break;
	    	case 1:
	    	    send_to_char( "EXIT FLAGS SET.\n\r\n\r", ch);
	    	    pObj->value[1] = flag_value( exit_flags, argument );
	    	    break;
	    	case 2:
	    	    send_to_char( "PORTAL FLAGS SET.\n\r\n\r", ch);
	    	    pObj->value[2] = flag_value( portal_flags, argument );
	    	    break;
	    	case 3:
	    	    send_to_char( "EXIT VNUM SET.\n\r\n\r", ch);
	    	    pObj->value[3] = atoi ( argument );
	    	    break;
	   }
	   break;

	case ITEM_FURNITURE:
	    switch ( value_num )
	    {
	        default:
	            do_help( ch, "ITEM_FURNITURE" );
	            return FALSE;
	            
	        case 0:
	            send_to_char( "NUMBER OF PEOPLE SET.\n\r\n\r", ch);
	            pObj->value[0] = atoi ( argument );
	            break;
	        case 1:
	            send_to_char( "MAX WEIGHT SET.\n\r\n\r", ch);
	            pObj->value[1] = atoi ( argument );
	            break;
	        case 2:
	            send_to_char( "FURNITURE FLAGS TOGGLED.\n\r\n\r", ch);
	            pObj->value[2] ^= (flag_value( furniture_flags, argument ) != NO_FLAG
	            ? flag_value( furniture_flags, argument ) : 0);
	            break;
	        case 3:
	            send_to_char( "HEAL BONUS SET.\n\r\n\r", ch);
	            pObj->value[3] = atoi ( argument );
	            break;
	        case 4:
	            send_to_char( "MANA BONUS SET.\n\r\n\r", ch);
	            pObj->value[4] = atoi ( argument );
	            break;
	    }
	    break;
	   
        case ITEM_CONTAINER:
	    switch ( value_num )
	    {
		int value;
		
		default:
		    do_help( ch, "ITEM_CONTAINER" );
	            return FALSE;
		case 0:
	            send_to_char( "WEIGHT CAPACITY SET.\n\r\n\r", ch );
	            pObj->value[0] = atoi( argument );
	            break;
		case 1:
	            if ( ( value = flag_value( container_flags, argument ) )
	              != NO_FLAG )
	        	TOGGLE_BIT(pObj->value[1], value);
		    else
		    {
			do_help ( ch, "ITEM_CONTAINER" );
			return FALSE;
		    }
	            send_to_char( "CONTAINER TYPE SET.\n\r\n\r", ch );
	            break;
		case 2:
		    if ( atoi(argument) != 0 )
		    {
			if ( !get_obj_index( atoi( argument ) ) )
			{
			    send_to_char( "THERE IS NO SUCH ITEM.\n\r\n\r", ch );
			    return FALSE;
			}

			if ( get_obj_index( atoi( argument ) )->item_type != ITEM_KEY )
			{
			    send_to_char( "THAT ITEM IS NOT A KEY.\n\r\n\r", ch );
			    return FALSE;
			}
		    }
		    send_to_char( "CONTAINER KEY SET.\n\r\n\r", ch );
		    pObj->value[2] = atoi( argument );
		    break;
		case 3:
		    send_to_char( "CONTAINER MAX WEIGHT SET.\n\r", ch);
		    pObj->value[3] = atoi( argument );
		    break;
		case 4:
		    send_to_char( "WEIGHT MULTIPLIER SET.\n\r\n\r", ch );
		    pObj->value[4] = atoi ( argument );
		    break;
	    }
	    break;

	case ITEM_DRINK_CON:
	    switch ( value_num )
	    {
	        default:
		    do_help( ch, "ITEM_DRINK" );
/* OLC		    do_help( ch, "liquids" );    */
	            return FALSE;
	        case 0:
	            send_to_char( "MAXIMUM AMOUT OF LIQUID HOURS SET.\n\r\n\r", ch );
	            pObj->value[0] = atoi( argument );
	            break;
	        case 1:
	            send_to_char( "CURRENT AMOUNT OF LIQUID HOURS SET.\n\r\n\r", ch );
	            pObj->value[1] = atoi( argument );
	            break;
	        case 2:
	            send_to_char( "LIQUID TYPE SET.\n\r\n\r", ch );
	            pObj->value[2] = ( liq_lookup(argument) != -1 ?
	            		       liq_lookup(argument) : 0 );
	            break;
	        case 3:
	            send_to_char( "POISON VALUE TOGGLED.\n\r\n\r", ch );
	            pObj->value[3] = ( pObj->value[3] == 0 ) ? 1 : 0;
	            break;

		case 4:
		    send_to_char( "REFILL VALUE TOGGLED.\n\r\n\r", ch );
		    pObj->value[4] = ( pObj->value[4] == 0 ) ? 1 : 0;
		    break;
	    }
            break;

	case ITEM_FOUNTAIN:
	    switch (value_num)
	    {
	    	default:
		    do_help( ch, "ITEM_FOUNTAIN" );
/* OLC		    do_help( ch, "liquids" );    */
	            return FALSE;
	        case 0:
	            send_to_char( "MAXIMUM AMOUT OF LIQUID HOURS SET.\n\r\n\r", ch );
	            pObj->value[0] = atoi( argument );
	            break;
	        case 1:
	            send_to_char( "CURRENT AMOUNT OF LIQUID HOURS SET.\n\r\n\r", ch );
	            pObj->value[1] = atoi( argument );
	            break;
	        case 2:
	            send_to_char( "LIQUID TYPE SET.\n\r\n\r", ch );
	            pObj->value[2] = ( liq_lookup( argument ) != -1 ?
	            		       liq_lookup( argument ) : 0 );
	            break;
		case 3:
		    send_to_char( "Fountain Empty Toggled.\n\r\n\r",ch);
		    pObj->value[3] = !pObj->value[3];
		    break;
            }
	break;
		    	
	case ITEM_FOOD:
	    switch ( value_num )
	    {
	        default:
		    do_help( ch, "ITEM_FOOD" );
	            return FALSE;
	        case 0:
	            send_to_char( "HOURS OF FOOD SET.\n\r\n\r", ch );
	            pObj->value[0] = atoi( argument );
	            break;
	        case 1:
	            send_to_char( "FOOD QUALITY SET..\n\r\n\r", ch );
	            pObj->value[1] = atoi( argument );
	            break;
	        case 3:
	            send_to_char( "POISON VALUE TOGGLED.\n\r\n\r", ch );
	            pObj->value[3] = ( pObj->value[3] == 0 ) ? 1 : 0;
	            break;
	    }
            break;

	case ITEM_MONEY:
	    switch ( value_num )
	    {
	        default:
		    do_help( ch, "ITEM_MONEY" );
	            return FALSE;
	        case 0:
	            send_to_char( "VALUE AMOUNT SET.\n\r\n\r", ch );
	            pObj->value[0] = atoi( argument );
	            break;
		case 1:
		    value = coin_lookup(argument);
		    if (value == -1)
		    {
			send_to_char("Invalid coin type.\n\r", ch);
			return FALSE;
		    }
	            send_to_char( "COIN TYPE SET.\n\r\n\r", ch );
	            pObj->value[1] = value;
	            break;
	    }
            break;
    }

    std::ostringstream mess;
    mess << DisplayPanel::Render(show_obj_values(*pObj)) << '\n';
    send_to_char(mess.str().c_str(), ch);
    return true;
}

OEDIT(oedit_show)
{
    OBJ_INDEX_DATA *pObj;
    EDIT_OBJ(ch, pObj);

    DisplayPanel::Box keys;
    DisplayPanel::Box values;

    // Vnum
    AddKey(keys, "Vnum");
    AddValue(values, Bracketize(makeString(pObj->vnum)));
    
    // Area
    AddKey(keys, "Area");
    if (pObj->area == NULL)
        AddValue(values, "(none)");
    else
    {
        std::ostringstream mess;
        mess << Bracketize(makeString(pObj->area->vnum)) << ' ' << pObj->area->name;
        AddValue(values, mess.str());
    }

    // Strings
    AddKey(keys, "Name");
    AddValue(values, pObj->name);

    AddKey(keys, "Short");
    AddValue(values, pObj->short_descr);

    AddKey(keys, "Long");
    AddValue(values, pObj->description);

    // Ex descs
    AddKey(keys, "Ex Descs");
    if (pObj->extra_descr == NULL)
        AddValue(values, "");
    else
    {
        for (EXTRA_DESCR_DATA * ed(pObj->extra_descr); ed != NULL; ed = ed->next)
        {
            if (ed != pObj->extra_descr)
                AddKey(keys, "");

            AddValue(values, Bracketize(ed->keyword));
        }
    }

    // Progs
    AddKey(keys, "Progs");
    if (pObj->objprogs == NULL)
        AddValue(values, "");
    else
    {
        for (const MPROG_DATA * prog(pObj->objprogs); prog != NULL; prog = prog->next)
        {
            std::ostringstream mess;
            mess << '>' << mprog_type_to_name(prog->type) << ' ' << prog->arglist;

            if (prog != pObj->objprogs)
                AddKey(keys, "");

            AddValue(values, mess.str());
        }
    }


    // Types
    DisplayPanel::Box typeKeys;
    DisplayPanel::Box typeValues;
    std::string typeValue(flag_string(type_flags, pObj->item_type));
    AddKey(typeKeys, "Type");
    AddValue(typeValues, typeValue);
 
    // Damverb
    if (pObj->item_type == ITEM_WEAPON || pObj->item_type == ITEM_ARROW)
    {
        AddKey(typeKeys, "Damverb");
        AddValue(typeValues, pObj->obj_str);
    }
    else if (IS_SET(pObj->wear_flags, ITEM_WEAR_FAMILIAR))
    {
        AddKey(typeKeys, "Familiar");
        AddValue(typeValues, pObj->obj_str == NULL ? "" : pObj->obj_str);
    }

    DisplayPanel::Box lhsKeys;
    DisplayPanel::Box lhsValues;

    AddKey(lhsKeys, "Level");
    AddValue(lhsValues, makeString(pObj->level));
   
    AddKey(lhsKeys, "Material");
    AddValue(lhsValues, material_table[pObj->material].name);

    AddKey(lhsKeys, "Weight");
    AddValue(lhsValues, makeString(pObj->weight));

    AddKey(lhsKeys, "Size");
    AddValue(lhsValues, size_table[pObj->size].name);

    AddKey(lhsKeys, "Cost");
    AddValue(lhsValues, makeString(pObj->cost));

    AddKey(lhsKeys, "Limit");
    AddValue(lhsValues, makeString(pObj->limit_factor));

    AddKey(lhsKeys, "Wear");
    AddValue(lhsValues, Bracketize(flag_string(wear_flags, pObj->wear_flags)));

    // Extra
    AddKey(lhsKeys, "Extra");
    for (size_t i(0); i <= 2; ++i)
    {
        if (pObj->extra_flags[i] == 0)
            AddValue(lhsValues, Bracketize(""));
        else
            AddValue(lhsValues, Bracketize(flag_string(extra_flags[i], pObj->extra_flags[i])));
    }

       // Build the effect list
    DisplayPanel::Box numBox("Number");     numBox.AddLine("------");
    DisplayPanel::Box modBox("Modifier");   modBox.AddLine("--------");
    DisplayPanel::Box affBox("Affects");    affBox.AddLine("-------");

    int i(0);
    for (AFFECT_DATA * paf(pObj->affected); paf; paf = paf->next)
    {
        numBox.AddLine(Bracketize(makeString(i)));
        AddValue(modBox, makeString(paf->modifier));
        AddValue(affBox, flag_string(apply_flags, paf->location));
        ++i;
    }

    // Compose the top panel
    DisplayPanel::HorizontalSplit topPanel(DisplayPanel::Options(DisplayPanel::Style_None));
    topPanel.Add(keys);
    topPanel.Add(values);
    RenderPanel(topPanel, ch);

    // Compose the middle panel
    DisplayPanel::HorizontalSplit lhsPanel(DisplayPanel::Options(DisplayPanel::Style_None));
    lhsPanel.Add(lhsKeys);
    lhsPanel.Add(lhsValues);

    DisplayPanel::HorizontalSplit typePanel(DisplayPanel::Options(DisplayPanel::Style_None));
    typePanel.Add(typeKeys);
    typePanel.Add(typeValues);

    DisplayPanel::VerticalSplit rhsPanel(DisplayPanel::Options(DisplayPanel::Style_None));
    rhsPanel.Add(typePanel);
    rhsPanel.Add(show_obj_values(*pObj));
    if (pObj->affected != NULL)
    {
        DisplayPanel::HorizontalSplit affectsPanel(DisplayPanel::Options(DisplayPanel::Style_None));
        affectsPanel.Add(numBox);
        affectsPanel.Add(modBox);
        affectsPanel.Add(affBox);
        rhsPanel.Add(affectsPanel);
    }

    DisplayPanel::HorizontalSplit middlePanel(lhsPanel, rhsPanel);
    RenderPanel(middlePanel, ch);
 
    // Compose the bottom panel
    if (pObj->lore && pObj->lore[0] != '\0')
    {
        DisplayPanel::Box loreBox("Lore:");
        std::ostringstream mess;
        mess << "     " << pObj->lore;
        loreBox.AddText(mess.str());
        RenderPanel(loreBox, ch);
    }
    return false;
}

/*
 * Need to issue warning if flag isn't valid. -- does so now -- Hugin.
 */
OEDIT( oedit_addaffect )
{
    int value;
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];

    EDIT_OBJ(ch, pObj);

    argument = one_argument( argument, loc );
    one_argument( argument, mod );

    if ( loc[0] == '\0' || mod[0] == '\0' || !is_number( mod ) )
    {
	send_to_char( "Syntax:  addaffect [location] [#mod]\n\r", ch );
	return FALSE;
    }

    if ( ( value = flag_value( apply_flags, loc ) ) == NO_FLAG ) /* Hugin */
    {
        send_to_char( "Valid affects are:\n\r", ch );
	show_help( ch, "apply" );
	return FALSE;
    }

    pAf             =   new_affect();
    pAf->location   =   value;
    pAf->modifier   =   atoi( mod );
    pAf->type       =   -1;
    pAf->duration   =   -1;
    pAf->bitvector  =   0;
    pAf->next       =   pObj->affected;
    pObj->affected  =   pAf;

    send_to_char( "Affect added.\n\r", ch);
    return TRUE;
}

/*
 * My thanks to Hans Hvidsten Birkeland and Noam Krendel(Walker)
 * for really teaching me how to manipulate pointers.
 */
OEDIT( oedit_delaffect )
{
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    AFFECT_DATA *pAf_next;
    char affect[MAX_STRING_LENGTH];
    int  value;
    int  cnt = 0;

    EDIT_OBJ(ch, pObj);

    one_argument( argument, affect );

    if ( !is_number( affect ) || affect[0] == '\0' )
    {
	send_to_char( "Syntax:  delaffect [#xaffect]\n\r", ch );
	return FALSE;
    }

    value = atoi( affect );

    if ( value < 0 )
    {
	send_to_char( "Only non-negative affect-numbers allowed.\n\r", ch );
	return FALSE;
    }

    if ( !( pAf = pObj->affected ) )
    {
	send_to_char( "OEdit:  Non-existant affect.\n\r", ch );
	return FALSE;
    }

    if( value == 0 )	/* First case: Remove first affect */
    {
	pAf = pObj->affected;
	pObj->affected = pAf->next;
	free_affect( pAf );
    }
    else		/* Affect to remove is not the first */
    {
	while ( ( pAf_next = pAf->next ) && ( ++cnt < value ) )
	     pAf = pAf_next;

	if( pAf_next )		/* See if it's the next affect */
	{
	    pAf->next = pAf_next->next;
	    free_affect( pAf_next );
	}
	else                                 /* Doesn't exist */
	{
	     send_to_char( "No such affect.\n\r", ch );
	     return FALSE;
	}
    }

    send_to_char( "Affect removed.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_size )
{
    OBJ_INDEX_DATA *pObj;
    int size;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: size [size]\n\r", ch);
	return FALSE;
    }

    size = size_lookup(argument);
  
    if (size == -1)
    {
	send_to_char("Invalid size.\n\r", ch);
	return FALSE;
    }

    pObj->size = size;
    send_to_char("Size set.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_name )
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  name [string]\n\r", ch );
	return FALSE;
    }

    free_string( pObj->name );
    pObj->name = str_dup( argument );

    send_to_char( "Name set.\n\r", ch);
    return TRUE;
}



OEDIT( oedit_short )
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  short [string]\n\r", ch );
	return FALSE;
    }

    free_string( pObj->short_descr );
    pObj->short_descr = str_dup( argument );
    pObj->short_descr[0] = LOWER( pObj->short_descr[0] );

    send_to_char( "Short description set.\n\r", ch);
    return TRUE;
}

OEDIT(oedit_familiarstring)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (pObj->item_type == ITEM_WEAPON || pObj->item_type == ITEM_ARROW)
    {
        send_to_char("Weapon and arrows may not use a familiar string.\n", ch);
        return false;
    }

    if (!IS_SET(pObj->wear_flags, ITEM_WEAR_FAMILIAR))
    {
        send_to_char("Set the familiar wear flag first.\n", ch);
        return false;
    }

    if (argument[0] == '\0')
    {
        send_to_char("Syntax: familiarstring <string>\n\r", ch);
        return false;
    }

    copy_string(pObj->obj_str, argument);

    send_to_char("Familiar string set.\n\r", ch);
    return true;
}

OEDIT( oedit_damverb )
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if ((pObj->item_type != ITEM_WEAPON) && (pObj->item_type != ITEM_ARROW))
    {
	send_to_char("Only weapons and arrows have damage verbs.\n\r", ch);
	return FALSE;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: damverb [string]\n\r", ch);
	return FALSE;
    }

    free_string(pObj->obj_str);
    pObj->obj_str = str_dup(argument);

    send_to_char("Damage verb set.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_long )
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  long [string]\n\r", ch );
	return FALSE;
    }
        
    free_string( pObj->description );
    pObj->description = str_dup( argument );
    pObj->description[0] = UPPER( pObj->description[0] );

    send_to_char( "Long description set.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_lore )
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if ( argument[0] == '\0' )
    {
	string_append( ch, &pObj->lore );
	return TRUE;
    }

    send_to_char( "Syntax:  desc    - line edit\n\r", ch );
    return FALSE;
}

bool set_value( CHAR_DATA *ch, OBJ_INDEX_DATA *pObj, char *argument, int value )
{
    if ( argument[0] == '\0' )
    {
	set_obj_values( ch, pObj, -1, "" );     /* '\0' changed to "" -- Hugin */
	return FALSE;
    }

    if ( set_obj_values( ch, pObj, value, argument ) )
	return TRUE;

    return FALSE;
}



/*****************************************************************************
 Name:		oedit_values
 Purpose:	Finds the object and sets its value.
 Called by:	The four valueX functions below. (now five -- Hugin )
 ****************************************************************************/
bool oedit_values( CHAR_DATA *ch, char *argument, int value )
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if ( set_value( ch, pObj, argument, value ) )
        return TRUE;

    return FALSE;
}


OEDIT( oedit_value0 )
{
    if ( oedit_values( ch, argument, 0 ) )
        return TRUE;

    return FALSE;
}



OEDIT( oedit_value1 )
{
    if ( oedit_values( ch, argument, 1 ) )
        return TRUE;

    return FALSE;
}



OEDIT( oedit_value2 )
{
    if ( oedit_values( ch, argument, 2 ) )
        return TRUE;

    return FALSE;
}



OEDIT( oedit_value3 )
{
    if ( oedit_values( ch, argument, 3 ) )
        return TRUE;

    return FALSE;
}



OEDIT( oedit_value4 )
{
    if ( oedit_values( ch, argument, 4 ) )
        return TRUE;

    return FALSE;
}



OEDIT( oedit_weight )
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
	send_to_char( "Syntax:  weight [number]\n\r", ch );
	return FALSE;
    }

    pObj->weight = atoi( argument );

    send_to_char( "Weight set.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_setprice )
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    pObj->cost = obj_default_price(pObj);

    send_to_char("Default price set.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_cost )
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
	send_to_char( "Syntax:  cost [number]\n\r", ch );
	return FALSE;
    }

    pObj->cost = atoi( argument );

    send_to_char( "Cost set.\n\r", ch);
    return TRUE;
}



OEDIT( oedit_create )
{
    OBJ_INDEX_DATA *pObj;
    AREA_DATA *pArea;
    int  value;
    int  iHash;

    value = atoi( argument );
    if ( argument[0] == '\0' || value == 0 )
    {
	send_to_char( "Syntax:  oedit create [vnum]\n\r", ch );
	return FALSE;
    }

    pArea = get_vnum_area( value );
    if ( !pArea )
    {
	send_to_char( "OEdit:  That vnum is not assigned an area.\n\r", ch );
	return FALSE;
    }

    if ( !IS_BUILDER( ch, pArea ) )
    {
	send_to_char( "OEdit:  Vnum in an area you cannot build in.\n\r", ch );
	return FALSE;
    }

    if ( get_obj_index( value ) )
    {
	send_to_char( "OEdit:  Object vnum already exists.\n\r", ch );
	return FALSE;
    }
        
    pObj			= new_obj_index();
    pObj->vnum			= value;
    pObj->area			= pArea;
        
    if ( value > top_vnum_obj )
	top_vnum_obj = value;

    iHash			= value % MAX_KEY_HASH;
    pObj->next			= obj_index_hash[iHash];
    obj_index_hash[iHash]	= pObj;
    pObj->limit_factor		= 0;
    pObj->limit			= 0;
    pObj->current		= 0;

    olc_switch(ch->desc, (void *) pObj, ED_OBJECT);

    send_to_char( "Object Created.\n\r", ch );
    return TRUE;
}

OEDIT( oedit_copy )
{
    char buf[MAX_STRING_LENGTH];
    int vnum;
    OBJ_INDEX_DATA *pObj, *vObj;
    OBJ_INDEX_DATA origObj;
    EXTRA_DESCR_DATA *ed, *ed_next, *ed_new;
    AFFECT_DATA *paf, *paf_next, *paf_new;

    if ((argument[0] == '\0') || !is_number(argument))
    {
	send_to_char("Syntax: copy <vnum>\n\r", ch);
	return FALSE;
    }

    vnum = atoi(argument);

    if ((vObj = get_obj_index(vnum)) == NULL)
    {
	sprintf(buf, "Error: object vnum %d does not exist.\n\r", vnum);
	send_to_char(buf, ch);
	return FALSE;
    }

    EDIT_OBJ(ch, pObj);

    origObj = *pObj;
    *pObj = *vObj;

    pObj->next		= origObj.next;
    pObj->area		= origObj.area;
    pObj->count		= origObj.count;
    pObj->reset_num	= origObj.reset_num;
    pObj->current	= origObj.current;
    pObj->vnum		= origObj.vnum;
    pObj->objprogs	= origObj.objprogs;
    pObj->progtypes	= origObj.progtypes;

    pObj->name		= str_dup(vObj->name);
    pObj->short_descr	= str_dup(vObj->short_descr);
    pObj->lore		= str_dup(vObj->lore);
    pObj->description	= str_dup(vObj->description);

    pObj->obj_str = (vObj->obj_str == NULL ? NULL : str_dup(vObj->obj_str));

    free_string(origObj.name);
    free_string(origObj.short_descr);
    free_string(origObj.lore);
    free_string(origObj.description);
    free_string(origObj.obj_str);

    for (ed = origObj.extra_descr; ed != NULL; ed = ed_next )
    {
	ed_next = ed->next;
	free_extra_descr(ed);
    }
    pObj->extra_descr = NULL;

    for (ed = vObj->extra_descr; ed != NULL; ed = ed->next)
    {
        ed_new                  = new_extra_descr();
        ed_new->keyword    	= str_dup( ed->keyword );
        ed_new->description     = str_dup( ed->description );
        ed_new->next           	= pObj->extra_descr;
        pObj->extra_descr  	= ed_new;
    }

    for (paf = origObj.affected; paf != NULL; paf = paf_next)
    {
		paf_next = paf->next;
		free_affect(paf);
    }
    pObj->affected = NULL;

    for (paf = vObj->affected; paf != NULL; paf = paf->next) 
    {
		paf_new		= new_affect();
		*paf_new	= *paf;
		paf_new->next	= pObj->affected;
		pObj->affected	= paf_new;
    }

    send_to_char("Object index successfully copied.\n\r", ch);
    return TRUE;
}




OEDIT( oedit_ed )
{
    OBJ_INDEX_DATA *pObj;
    EXTRA_DESCR_DATA *ed;
    char command[MAX_INPUT_LENGTH];
    char keyword[MAX_INPUT_LENGTH];

    EDIT_OBJ(ch, pObj);

    argument = one_argument( argument, command );
    one_argument( argument, keyword );

    if ( command[0] == '\0' )
    {
	send_to_char( "Syntax:  ed add [keyword]\n\r", ch );
	send_to_char( "         ed delete [keyword]\n\r", ch );
	send_to_char( "         ed edit [keyword]\n\r", ch );
	send_to_char( "         ed format [keyword]\n\r", ch );
	return FALSE;
    }

    if ( !str_cmp( command, "add" ) )
    {
	if ( keyword[0] == '\0' )
	{
	    send_to_char( "Syntax:  ed add [keyword]\n\r", ch );
	    return FALSE;
	}

	ed                  =   new_extra_descr();
	ed->keyword         =   str_dup( keyword );
	ed->next            =   pObj->extra_descr;
	pObj->extra_descr   =   ed;

	string_append( ch, &ed->description );

	return TRUE;
    }

    if ( !str_cmp( command, "edit" ) )
    {
	if ( keyword[0] == '\0' )
	{
	    send_to_char( "Syntax:  ed edit [keyword]\n\r", ch );
	    return FALSE;
	}

	for ( ed = pObj->extra_descr; ed; ed = ed->next )
	{
	    if ( is_name( keyword, ed->keyword ) )
		break;
	}

	if ( !ed )
	{
	    send_to_char( "OEdit:  Extra description keyword not found.\n\r", ch );
	    return FALSE;
	}

	string_append( ch, &ed->description );

	return TRUE;
    }

    if ( !str_cmp( command, "delete" ) )
    {
	EXTRA_DESCR_DATA *ped = NULL;

	if ( keyword[0] == '\0' )
	{
	    send_to_char( "Syntax:  ed delete [keyword]\n\r", ch );
	    return FALSE;
	}

	for ( ed = pObj->extra_descr; ed; ed = ed->next )
	{
	    if ( is_name( keyword, ed->keyword ) )
		break;
	    ped = ed;
	}

	if ( !ed )
	{
	    send_to_char( "OEdit:  Extra description keyword not found.\n\r", ch );
	    return FALSE;
	}

	if ( !ped )
	    pObj->extra_descr = ed->next;
	else
	    ped->next = ed->next;

	free_extra_descr( ed );

	send_to_char( "Extra description deleted.\n\r", ch );
	return TRUE;
    }


    if ( !str_cmp( command, "format" ) )
    {
	EXTRA_DESCR_DATA *ped = NULL;

	if ( keyword[0] == '\0' )
	{
	    send_to_char( "Syntax:  ed format [keyword]\n\r", ch );
	    return FALSE;
	}

	for ( ed = pObj->extra_descr; ed; ed = ed->next )
	{
	    if ( is_name( keyword, ed->keyword ) )
		break;
	    ped = ed;
	}

	if ( !ed )
	{
                send_to_char( "OEdit:  Extra description keyword not found.\n\r", ch );
                return FALSE;
	}

	ed->description = format_string( ed->description );

	send_to_char( "Extra description formatted.\n\r", ch );
	return TRUE;
    }

    oedit_ed( ch, "" );
    return FALSE;
}

OEDIT( oedit_prog )
{
    OBJ_INDEX_DATA *pObj;
    MPROG_DATA *ed;
    char command[MAX_INPUT_LENGTH];
    char progtype[MAX_INPUT_LENGTH];
    char progarg[MAX_INPUT_LENGTH];

    EDIT_OBJ(ch, pObj);

    argument = one_argument( argument, command );
    argument = one_argument( argument, progtype );
    strcpy( progarg, argument );

    if ( command[0] == '\0' )
    {
        send_to_char( "Syntax:  prog add [progtype] [progarg]\n\r", ch );
        send_to_char( "         prog delete [progtype] [progarg]\n\r", ch);
        send_to_char( "         prog edit [progtype] [progarg]\n\r", ch );
	send_to_char( "		prog prioritize [progtype] [progarg]\n\r", ch);
        return FALSE;
    }

    if ( !str_cmp( command, "add" ) )
    {
        if ( progtype[0] == '\0' || progarg[0] == '\0' )
        {
            send_to_char( "Syntax:  prog add [progtype] [progarg]\n\r", ch);
            return FALSE;
        }

        if ( mprog_name_to_type( progtype ) == ERROR_PROG )
        {
          send_to_char( "Invalid progtype.\n\r", ch );
          return FALSE;
        }

        ed                  =   (MPROG_DATA *)alloc_perm( sizeof( MPROG_DATA ));
	g_num_progs++;
        ed->type            =   mprog_name_to_type( progtype );
        ed->next            =   pObj->objprogs;
        ed->arglist         =   str_dup( progarg );
		determine_exact_match(ed);
        pObj->objprogs      =   ed;
        pObj->progtypes     =   pObj->progtypes | ed->type;

        string_append( ch, &ed->comlist );

        return TRUE;
    }

    if (!str_cmp(command, "view"))
    {
        do_prog_view(ch, pObj->objprogs, progtype, progarg);
        return FALSE;
    }

    if ( !str_cmp( command, "edit" ) )
    {
        if ( progtype[0] == '\0' || progarg[0] == '\0' )
        {
            send_to_char( "Syntax:  prog edit [progtype] [progarg]\n\r",ch );
            return FALSE;
        }

        for ( ed = pObj->objprogs; ed; ed = ed->next )
        {
            if ( !str_cmp(ed->arglist, progarg)  &&
                 mprog_name_to_type(progtype) == ed->type )
                break;
        }

        if ( !ed )
        {
            send_to_char( "OEdit:  Object program not found.\n\r", ch);
            return FALSE;
        }

        string_append( ch, &ed->comlist );

        return TRUE;
    }

    if ( !str_cmp( command, "delete" ) )
    {
        MPROG_DATA *ped = NULL;

        if ( progtype[0] == '\0' || progarg[0] == '\0' )
        {
            send_to_char( "Syntax:  prog delete [progtype] [progarg]\n\r",ch );
            return FALSE;
        }

        for ( ed = pObj->objprogs; ed; ed = ed->next )
        {
            if ( is_name( progarg, ed->arglist )  &&
                 mprog_name_to_type(progtype) == ed->type )
                break;
            ped = ed;
        }

        if ( !ed )
        {
            send_to_char( "OEdit:  Object program not found.\n\r", ch);
            return FALSE;
        }

        if ( !ped )
            pObj->objprogs = ed->next;
        else
            ped->next = ed->next;

        free_string( ed->arglist );
        free_string( ed->comlist );
        pObj->progtypes = 0;
        for ( ed = pObj->objprogs; ed; ed = ed->next )
        {
          pObj->progtypes = pObj->progtypes | ed->type;
        }

        send_to_char( "Object program deleted.\n\r", ch );
        return TRUE;
    }

    if ( !str_cmp(command, "prioritize") )
    {
        MPROG_DATA *ped = NULL;

        if ( progtype[0] == '\0' || progarg[0] == '\0' )
        {
            send_to_char( "Syntax:  prog prioritize [progtype] [progarg]\n\r",ch );
            return FALSE;
        }

        for ( ed = pObj->objprogs; ed; ed = ed->next )
        {
            if ( is_name( progarg, ed->arglist )  &&
                 mprog_name_to_type(progtype) == ed->type )
                break;
            ped = ed;
        }
	
        if ( !ed )
        {
            send_to_char( "OEdit:  Object program not found.\n\r", ch);
            return FALSE;
        }

        if ( !ped )
	{
	    send_to_char("OEdit: Prog already has priority.\n\r", ch);
	    return FALSE;
	}

        ped->next = ed->next;
	ed->next = pObj->objprogs;
	pObj->objprogs = ed;
	send_to_char("Object program prioritized.\n\r", ch);
	return TRUE;
    }

    oedit_prog( ch, "" );
    return FALSE;
}




/* ROM object functions : */

OEDIT( oedit_extra )      /* Moved out of oedit() due to naming conflicts -- Hugin */
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if ( argument[0] != '\0' )
    {
	EDIT_OBJ(ch, pObj);

	if ( ( value = flag_value( extra_flags[0], argument ) ) != NO_FLAG )
	{
	    TOGGLE_BIT(pObj->extra_flags[0], value);

	    send_to_char( "Extra flag toggled.\n\r", ch);
	    return TRUE;
	}

	if ( ( value = flag_value( extra_flags[1], argument ) ) != NO_FLAG )
	{
	    TOGGLE_BIT(pObj->extra_flags[1], value);

	    send_to_char( "Extra flag toggled.\n\r", ch);
	    return TRUE;
	}
	if ((value = flag_value(extra_flags[2], argument)) != NO_FLAG)
	{
		TOGGLE_BIT(pObj->extra_flags[2], value);
		send_to_char("Extra flag toggled.\n\r", ch);
		return TRUE;
	}
    }

    send_to_char( "Syntax:  extra [flag]\n\rType '? extra' for a list of flags.\n\r", ch );
    return FALSE;
}


OEDIT( oedit_wear )      /* Moved out of oedit() due to naming conflicts -- Hugin */
{
    OBJ_INDEX_DATA *pObj;
    int value;

     if ( argument[0] != '\0' )
    {
	EDIT_OBJ(ch, pObj);

	if ( ( value = flag_value( wear_flags, argument ) ) != NO_FLAG )
	{
	    TOGGLE_BIT(pObj->wear_flags, value);

	    send_to_char( "Wear flag toggled.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char( "Syntax:  wear [flag]\n\r"
		  "Type '? wear' for a list of flags.\n\r", ch );
    return FALSE;
}


OEDIT( oedit_type )      /* Moved out of oedit() due to naming conflicts -- Hugin */
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if ( argument[0] != '\0' )
    {
	EDIT_OBJ(ch, pObj);

	if ( ( value = flag_value( type_flags, argument ) ) != NO_FLAG )
	{
	    pObj->item_type = value;

	    send_to_char( "Type set.\n\r", ch);

	    /*
	     * Clear the values.
	     */
	    pObj->value[0] = 0;
	    pObj->value[1] = 0;
	    pObj->value[2] = 0;
	    pObj->value[3] = 0;
	    pObj->value[4] = 0;     /* ROM */

	    return TRUE;
	}
    }

    send_to_char( "Syntax:  type [flag]\n\r"
		  "Type '? type' for a list of flags.\n\r", ch );
    return FALSE;
}

OEDIT( oedit_material )
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  material [string]\n\r", ch );
	return FALSE;
    }

    pObj->material = material_lookup(argument);

    if (pObj->material == 0) send_to_char( "Material set to default.\n\r", ch);
    else send_to_char("Material set.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_limit )
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
	send_to_char( "Syntax:  limit [number]\n\r", ch );
	return FALSE;
    }
    pObj->limit_factor = atoi( argument );
    pObj->limit = pObj->limit_factor;

    send_to_char( "Limit Factor set.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_level )
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
	send_to_char( "Syntax:  level [number]\n\r", ch );
	return FALSE;
    }

    pObj->level = atoi( argument );

    send_to_char( "Level set.\n\r", ch);
    return TRUE;
}



OEDIT( oedit_condition )
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if ( argument[0] != '\0'
    && ( value = atoi (argument ) ) >= 0
    && ( value <= 100 ) )
    {
	EDIT_OBJ( ch, pObj );

	pObj->condition = value;
	send_to_char( "Condition set.\n\r", ch );

	return TRUE;
    }

    send_to_char( "Syntax:  condition [number]\n\r"
		  "Where number can range from 0 (ruined) to 100 (perfect).\n\r",
		  ch );
    return FALSE;
}

static std::string makeDiceString(int dice[3])
{
    std::ostringstream mess;
    mess << "[";
    mess << std::right << std::setw(3) << dice[DICE_NUMBER] << " d ";
    mess << std::left << std::setw(3) << dice[DICE_TYPE] << " + ";
    mess << std::left << std::setw(4) << dice[DICE_BONUS] << "] (Ave: ";

    int ave((1 + dice[DICE_TYPE]) * dice[DICE_NUMBER]);
    mess << ((ave / 2) + dice[DICE_BONUS]) << '.' << (((ave % 2) == 0) ? '0' : '5') << ')';

    return mess.str();
}

/*
 * Mobile Editor Functions.
 */
MEDIT( medit_show )
{
    MOB_INDEX_DATA *pMob;
    EDIT_MOB(ch, pMob);

    DisplayPanel::Box keys;
    DisplayPanel::Box values;

    // Vnum
    AddKey(keys, "Vnum");
    AddValue(values, Bracketize(makeString(pMob->vnum)));

    // Area
    AddKey(keys, "Area");
    if (pMob->area == NULL)
        AddValue(values, "(none)");
    else
    {
        std::ostringstream mess;
        mess << '[' << pMob->area->vnum << "] " << pMob->area->name;
        AddValue(values, mess.str());
    }

    // Faction
    const Faction * faction(FactionTable::LookupFor(pMob->factionNumber));
    if (faction != NULL)
    {
        std::ostringstream mess;
        mess << '[' << pMob->factionNumber << "] " << faction->Name();
        AddKey(keys, "Faction");
        AddValue(values, mess.str());
    }

    // Strings
    AddKey(keys, "Name");
    AddValue(values, pMob->player_name);

    AddKey(keys, "Short");
    AddValue(values, pMob->short_descr);

    AddKey(keys, "Long");
    AddValue(values, trim(pMob->long_descr));

    // Progs
    AddKey(keys, "Progs");
    if (pMob->mobprogs == NULL)
        AddValue(values, "");
    else
    {
        for (const MPROG_DATA * prog(pMob->mobprogs); prog != NULL; prog = prog->next)
        {
            std::ostringstream mess;
            mess << '>' << mprog_type_to_name(prog->type) << ' ' << prog->arglist;

            if (prog != pMob->mobprogs)
                AddKey(keys, "");

            AddValue(values, mess.str());
        }
    }

    // Split by general concept; basic stats on the left
    DisplayPanel::Box lhsKeys;
    DisplayPanel::Box lhsValues;

    AddKey(lhsKeys, "Level");
    AddValue(lhsValues, makeString(pMob->level));

    AddKey(lhsKeys, "Align");
    AddValue(lhsValues, align_names[pMob->alignment]);

    const char * sexString("neutral");
    switch (pMob->sex)
    {
        case SEX_MALE:      sexString = "male";     break;
        case SEX_FEMALE:    sexString = "female";   break;
        case 3:             sexString = "random";   break; // ROM magic number
    }

    AddKey(lhsKeys, "Sex");
    AddValue(lhsValues, sexString);

    AddKey(lhsKeys, "Race");
    AddValue(lhsValues, race_table[pMob->race].name);
   
    if (IS_SET(pMob->nact, ACT_CLASSED))
    {
        AddKey(lhsKeys, "Class");
        AddValue(lhsValues, class_table[pMob->class_num].name);
    }

    AddKey(lhsKeys, "Size");
    AddValue(lhsValues, flag_string(size_flags, pMob->size));

    AddKey(lhsKeys, "Wealth");
    AddValue(lhsValues, makeString(pMob->wealth));

    if (pMob->spec_fun != NULL)
    {
        AddKey(lhsKeys, "Special");
        AddValue(lhsValues, spec_name(pMob->spec_fun));
    }

    // Prepare guild
    if (IS_SET(pMob->act, ACT_PRACTICE))
    {
        unsigned int total(0);
        for (const GM_DATA * gm(pMob->gm); gm != NULL; gm = gm->next)
            ++total;

        AddKey(lhsKeys, "Skills");
        AddValue(lhsValues, makeString(total));
    }

    // Combat stats on the right
    DisplayPanel::Box rhsKeys;
    DisplayPanel::Box rhsValues;

    AddKey(rhsKeys, "HP Dice");
    AddValue(rhsValues, makeDiceString(pMob->hit));

    AddKey(rhsKeys, "Mana Dice");
    AddValue(rhsValues, makeDiceString(pMob->mana));

    AddKey(rhsKeys, "Dam Dice");
    AddValue(rhsValues, makeDiceString(pMob->damage));

    // Prepare AC display
    std::ostringstream mess;
    mess << "[pierce: " << pMob->ac[AC_PIERCE] << " bash: " << pMob->ac[AC_BASH];
    mess << " slash: " << pMob->ac[AC_SLASH] << " magic: " << pMob->ac[AC_EXOTIC] << ']';
    
    AddKey(rhsKeys, "Armor");
    AddValue(rhsValues, mess.str());
 
    AddKey(rhsKeys, "Hitroll");
    AddValue(rhsValues, makeString(pMob->hitroll));

    AddKey(rhsKeys, "Damtype");
    AddValue(rhsValues, damtype_table[pMob->dam_type].name);

    AddKey(rhsKeys, "Damverb");
    AddValue(rhsValues, pMob->dam_verb);
 
    // Flags and other multifields
    DisplayPanel::Box multiKeys;
    DisplayPanel::Box multiValues;

    mess.str("");
    mess << flag_string(act_flags, pMob->act);
    const char * nactString(flag_string(nact_flags, pMob->nact));
    if (str_cmp(nactString, "none"))
        mess << ' ' << nactString;

    AddKey(multiKeys, "Act");
    AddValue(multiValues, Bracketize(mess.str()));

    AddKey(multiKeys, "Off");
    AddValue(multiValues, Bracketize(flag_string(off_flags, pMob->off_flags)));

    AddKey(multiKeys, "Affected");
    AddValue(multiValues, Bracketize(flag_string(affect_flags, pMob->affected_by)));

    // Prepare position
    mess.str("");
    mess << "[Start: " << flag_string(position_flags, pMob->start_pos);
    mess << "] [Default: " << flag_string(position_flags, pMob->default_pos) << ']';
    
    AddKey(multiKeys, "Position");
    AddValue(multiValues, mess.str());

    AddKey(multiKeys, "Form");
    AddValue(multiValues, Bracketize(flag_string(form_flags, pMob->form)));
    
    AddKey(multiKeys, "Parts");
    AddValue(multiValues, Bracketize(flag_string(part_flags, pMob->parts)));
 
    AddKey(multiKeys, "Imm");
    AddValue(multiValues, Bracketize(flag_string(imm_flags, pMob->imm_flags)));

    // Prepare the resists 
    mess.str("");
    for (unsigned int i(0); i < MAX_RESIST; ++i)
    {
        if (pMob->resist[i] != 0)
        {
            if (mess.tellp() > 0) 
                mess << ' ';

            mess << resist_table[i].name << '(' << pMob->resist[i] << ')';
        }
    }

    AddKey(multiKeys, "Resists");
    AddValue(multiValues, Bracketize(mess.str()));

    // Prepare languages
    mess.str("");
    for (unsigned int i(0); i < MAX_LANGUAGE; ++i)
    {
        if (IS_SET(pMob->lang_flags, (1 << i)))
        {
            if (mess.tellp() > 0)
                mess << ' ';

            mess << lang_data[i].name;
        }
    }
    
    AddKey(multiKeys, "Language");
    AddValue(multiValues, Bracketize(mess.str()));

    // Prepare assists
    mess.str("");
    for (unsigned i(0); i < MAX_ASSIST_VNUM; ++i)
    {
	    if (pMob->assist_vnum[i] != 0)
            mess << '[' << pMob->assist_vnum[i] << ']';
    }

    if (mess.tellp() > 0)
    {
        AddKey(multiKeys, "Assists");
        AddValue(multiValues, mess.str());
    }

    // Compose the top panel
    DisplayPanel::HorizontalSplit topPanel(DisplayPanel::Options(DisplayPanel::Style_None));
    topPanel.Add(keys);
    topPanel.Add(values);
    RenderPanel(topPanel, ch);

    // Compose the middle panel
    DisplayPanel::HorizontalSplit lhsPanel(DisplayPanel::Options(DisplayPanel::Style_None));
    lhsPanel.Add(lhsKeys);
    lhsPanel.Add(lhsValues);

    DisplayPanel::HorizontalSplit rhsPanel(DisplayPanel::Options(DisplayPanel::Style_None));
    rhsPanel.Add(rhsKeys);
    rhsPanel.Add(rhsValues);

    RenderPanel(DisplayPanel::HorizontalSplit(lhsPanel, rhsPanel), ch);

    // Compose the multi panel
    DisplayPanel::HorizontalSplit multiPanel(DisplayPanel::Options(DisplayPanel::Style_None));
    multiPanel.Add(multiKeys);
    multiPanel.Add(multiValues);
    RenderPanel(multiPanel, ch);

    // Description gets its own box
    DisplayPanel::Box descBox("Description:");
    mess.str("");
    mess << "     " << pMob->description;
    descBox.AddText(mess.str());
    RenderPanel(descBox, ch);

    // Prepare shop
    if (pMob->pShop != NULL)
    {
        SHOP_DATA *pShop(pMob->pShop);

        DisplayPanel::Box shopKeys;
        DisplayPanel::Box shopValues;

        // Prepare hours
        mess.str("");
        mess << '[' << pShop->open_hour << "] to [" << pShop->close_hour << ']';

        AddKey(shopKeys, "Hours");
        AddValue(shopValues, mess.str());

        AddKey(shopKeys, "Markup");
        AddValue(shopValues, makeString(pShop->profit_buy) + '%');

        AddKey(shopKeys, "Markdown");
        AddValue(shopValues, makeString(pShop->profit_sell) + '%');

        AddKey(shopKeys, "Max Sale");
        AddValue(shopValues, makeString(pShop->max_buy));
        
        DisplayPanel::HorizontalSplit shopBasicPanel(DisplayPanel::Style_None);
        shopBasicPanel.Add(shopKeys);
        shopBasicPanel.Add(shopValues);

        // Prepare the traded types
        DisplayPanel::Box tradedNumbers("#", "-");
        DisplayPanel::Box tradedTypes("Type", "----");
        bool anyTraded(false);
        for (unsigned int i(0); i < MAX_TRADE; ++i)
        {
            if (pShop->buy_type[i] == 0)
                continue;
            
            anyTraded = true;
            AddKey(tradedNumbers, makeString(i));
            AddValue(tradedTypes, flag_string(type_flags, pShop->buy_type[i]));
            
        }

        // Prepare the shop panel
        DisplayPanel::HorizontalSplit shopPanel;
        shopPanel.Add(shopBasicPanel);

        // Prepare the traded panel
        if (anyTraded)
        {
            DisplayPanel::HorizontalSplit tradedPanel(DisplayPanel::Style_None);
            tradedPanel.Add(tradedNumbers);
            tradedPanel.Add(tradedTypes);
            shopPanel.Add(tradedPanel);
        }

        // Compose the full panel
        DisplayPanel::VerticalSplit fullShopPanel(DisplayPanel::Options(DisplayPanel::Style_Collapse));
        fullShopPanel.Add(DisplayPanel::Box("Shop Settings"));
        fullShopPanel.Add(shopPanel);
        RenderPanel(fullShopPanel, ch);
    }
 
    return FALSE;
}

MEDIT( medit_create )
{
    MOB_INDEX_DATA *pMob;
    AREA_DATA *pArea;
    int  value;
    int  iHash;

    value = atoi( argument );
    if ( argument[0] == '\0' || value == 0 )
    {
	send_to_char( "Syntax:  medit create [vnum]\n\r", ch );
	return FALSE;
    }

    pArea = get_vnum_area( value );

    if ( !pArea )
    {
	send_to_char( "MEdit:  That vnum is not assigned an area.\n\r", ch );
	return FALSE;
    }

    if ( !IS_BUILDER( ch, pArea ) )
    {
	send_to_char( "MEdit:  Vnum in an area you cannot build in.\n\r", ch );
	return FALSE;
    }

    if ( get_mob_index( value ) )
    {
	send_to_char( "MEdit:  Mobile vnum already exists.\n\r", ch );
	return FALSE;
    }

    pMob			= new_mob_index();
    pMob->vnum			= value;
    pMob->area			= pArea;
        
    if ( value > top_vnum_mob )
	top_vnum_mob = value;        

    pMob->act			= ACT_IS_NPC;
    iHash			= value % MAX_KEY_HASH;
    pMob->next			= mob_index_hash[iHash];
    mob_index_hash[iHash]	= pMob;

    olc_switch(ch->desc, (void *) pMob, ED_MOBILE);

    send_to_char( "Mobile Created.\n\r", ch );
    return TRUE;
}

MEDIT( medit_copy )
{
    char buf[MAX_STRING_LENGTH];
    int vnum;
    MOB_INDEX_DATA *pMob, *vMob;
    MOB_INDEX_DATA origMob;

    if ((argument[0] == '\0') || !is_number(argument))
    {
	send_to_char("Syntax: copy <vnum>\n\r", ch);
	return FALSE;
    }

    vnum = atoi(argument);

    if ((vMob = get_mob_index(vnum)) == NULL)
    {
	sprintf(buf, "Error: mobile vnum %d does not exist.\n\r", vnum);
	send_to_char(buf, ch);
	return FALSE;
    }

    EDIT_MOB(ch, pMob);

    origMob = *pMob;
    *pMob = *vMob;

    pMob->next		= origMob.next;
    pMob->area		= origMob.area;
    pMob->count		= origMob.count;

    pMob->total_count	= origMob.total_count;
    pMob->vnum		= origMob.vnum;
    pMob->mobprogs	= origMob.mobprogs;
    pMob->progtypes	= origMob.progtypes;

    pMob->player_name	= str_dup(vMob->player_name);
    pMob->short_descr	= str_dup(vMob->short_descr);
    pMob->long_descr	= str_dup(vMob->long_descr);
    pMob->description	= str_dup(vMob->description);

    free_string(origMob.player_name);
    free_string(origMob.short_descr);
    free_string(origMob.long_descr);
    free_string(origMob.description);

    send_to_char("Mobile index successfully copied.\n\r", ch);
    return TRUE;
}


MEDIT( medit_spec )
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  spec [special function]\n\r", ch );
	return FALSE;
    }


    if ( !str_cmp( argument, "none" ) )
    {
        pMob->spec_fun = NULL;

        send_to_char( "Spec removed.\n\r", ch);
        return TRUE;
    }

    if ( spec_lookup( argument ) )
    {
	pMob->spec_fun = spec_lookup( argument );
	send_to_char( "Spec set.\n\r", ch);
	return TRUE;
    }

    send_to_char( "MEdit: No such special function.\n\r", ch );
    return FALSE;
}

MEDIT( medit_damtype )
{
    MOB_INDEX_DATA *pMob;
    int result;

    EDIT_MOB(ch, pMob);

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  damtype [damage typename]\n\r", ch );
	return FALSE;
    }

    if ((result = damtype_lookup(argument)) >= 0)
	pMob->dam_type = damtype_lookup(argument);
    else
    {
	send_to_char("Invalid damage type.\n\r", ch);
	return FALSE;
    }

    send_to_char( "Damage type set.\n\r", ch);
    return TRUE;
}

MEDIT (medit_damverb)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
	send_to_char("Syntax: damverb [damage verb]\n\r", ch);
	return FALSE;
    }

    pMob->dam_verb = str_dup(argument);
    send_to_char("Damage verb set.\n\r", ch);
    return TRUE;
}

MEDIT( medit_faction )
{
    MOB_INDEX_DATA *pMob;
    char buf[MAX_STRING_LENGTH];
    unsigned int faction;

    EDIT_MOB(ch, pMob);

    if (!str_prefix(argument, "none"))
    {
        send_to_char("Mobile faction cleared.\n", ch);
        pMob->factionNumber = Faction::None;
        return true;
    }

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Usage: faction [faction number|'none']\n\r", ch);
        return FALSE;
    }

    faction = atoi(argument);
    const Faction * factionInfo(FactionTable::LookupFor(faction));
    if (factionInfo == NULL)
    {
        send_to_char("Faction does not exist.\n\r", ch);
        return FALSE;
    }

    pMob->factionNumber = faction;
	sprintf(buf, "Mobile set to faction '%s (%d)'.\n\r", factionInfo->Name().c_str(), faction);
	send_to_char(buf, ch);
    return TRUE;
}


MEDIT( medit_align )
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    switch (UPPER(argument[0]))
    {
	case 'E':
	    pMob->alignment = ALIGN_EVIL;
	    send_to_char("Alignment set to evil.\n\r", ch);
	    break;
	case 'N':
	    pMob->alignment = ALIGN_NEUTRAL;
	    send_to_char("Alignment set to neutral.\n\r", ch);
	    break;
	case 'G':
	    pMob->alignment = ALIGN_GOOD;
	    send_to_char("Alignment set to good.\n\r", ch);
	    break;
	case 'R':
	    pMob->alignment = ALIGN_RANDOM;
	    send_to_char("Alignment set to random.\n\r", ch);
	    break;

	default:
	    send_to_char( "Synrax: alignment [good/neutral/evil/random]\n\r", ch);
	    return FALSE;
 	    break;
    }

    return TRUE;
}

MEDIT( medit_setlevel )
{
    MOB_INDEX_DATA *pMob;
    int x;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0' || !is_number(argument))
    {
	send_to_char("Syntax: setlevel [number]\n\r", ch);
	return FALSE;
    }

    pMob->level = atoi(argument);

    /* This formula is completely insane.  I invented it in my backyard on  */
    /* lazy Saturday morning. (Okay, not really, but it makes it sound like */
    /* a crazy science project.  We really need to smooth out the hit point */
    /* curve on mobs by level.            -- Erinos                         */
    pMob->hit[DICE_NUMBER] = round((float) pMob->level * (5.0 - pow(2, (float) pMob->level / 70.0)) / 5.0);
    pMob->hit[DICE_TYPE]   = round((float) pMob->level * pow(2, 1.0 + ((float) pMob->level / 50.0)) / 2.0);
    pMob->hit[DICE_BONUS]  = round((float) pMob->level * (21.0 + ((float) pMob->level / 4.0)) * (1.0 + (0.2 - fabs((25.0 - (float) pMob->level) / 50.0))));

    pMob->damage[DICE_NUMBER] = round((float) pMob->level / 10.0) + 1;
    pMob->damage[DICE_BONUS]  = round((float) pMob->level / 2.0);
    pMob->damage[DICE_TYPE]   = round(((float) pMob->level - (float) pMob->damage[DICE_BONUS]) / (float) pMob->damage[DICE_NUMBER] * 2.0);

    pMob->mana[DICE_NUMBER] = pMob->level;
    pMob->mana[DICE_TYPE]   = 10;
    pMob->mana[DICE_BONUS]  = 100;

    pMob->hitroll = round((float) pMob->level / 2.0);

    for (x = 0; x < 3; x++)
        pMob->ac[x] = (pMob->level - 15) * -6;

    pMob->ac[AC_EXOTIC] = (pMob->level - 30) * -3;

    send_to_char("Level set, and values calculated.\n\r", ch);
    return TRUE;
}


MEDIT( medit_level )
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
	send_to_char( "Syntax:  level [number]\n\r", ch );
	return FALSE;
    }

    pMob->level = atoi( argument );

    send_to_char( "Level set.\n\r", ch);
    return TRUE;
}



MEDIT( medit_desc )
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if ( argument[0] == '\0' )
    {
	string_append( ch, &pMob->description );
	return TRUE;
    }

    send_to_char( "Syntax:  desc    - line edit\n\r", ch );
    return FALSE;
}

MEDIT( medit_prog )
{
    MOB_INDEX_DATA *pMob;
    MPROG_DATA *ed;
    char command[MAX_INPUT_LENGTH];
    char progtype[MAX_INPUT_LENGTH];
    char progarg[MAX_INPUT_LENGTH];

    EDIT_MOB(ch, pMob);

    argument = one_argument( argument, command );
    argument = one_argument( argument, progtype );
    strcpy( progarg, argument );

    if ( command[0] == '\0' )
    {
        send_to_char( "Syntax:  prog add [progtype] [progarg]\n\r", ch );
        send_to_char( "         prog delete [progtype] [progarg]\n\r", ch);
        send_to_char( "         prog edit [progtype] [progarg]\n\r", ch );
        send_to_char( "         prog view [progtype] [progarg]\n\r", ch );
        send_to_char( "		prog prioritize [progtype] [progarg]\n\r", ch);
        return FALSE;
    }

    if ( !str_cmp( command, "add" ) )
    {
        if ( progtype[0] == '\0' || progarg[0] == '\0' )
        {
            send_to_char( "Syntax:  prog add [progtype] [progarg]\n\r", ch);
            return FALSE;
        }

        if ( mprog_name_to_type( progtype ) == ERROR_PROG )
        {
          send_to_char( "Invalid progtype.\n\r", ch );
          return FALSE;
        }

        ed                  =   (MPROG_DATA *)alloc_perm( sizeof( MPROG_DATA ));
	g_num_progs++;
        ed->type            =   mprog_name_to_type( progtype );
        ed->next            =   pMob->mobprogs;
        ed->arglist         =   str_dup( progarg );
		determine_exact_match(ed);
        pMob->mobprogs      =   ed;
        pMob->progtypes     =   pMob->progtypes | ed->type;

        string_append( ch, &ed->comlist );

        return TRUE;
    }

    if (!str_cmp(command, "view"))
    {
        do_prog_view(ch, pMob->mobprogs, progtype, progarg);
        return FALSE;
    }

    if ( !str_cmp( command, "edit" ) )
    {
        if ( progtype[0] == '\0' || progarg[0] == '\0' )
        {
            send_to_char( "Syntax:  prog edit [progtype] [progarg]\n\r", ch );
            return FALSE;
        }

        for ( ed = pMob->mobprogs; ed; ed = ed->next )
        {
            if ( !str_cmp(progarg, ed->arglist)  &&
                 mprog_name_to_type(progtype) == ed->type )
                break;
        }

        if ( !ed )
        {
            send_to_char( "MEdit:  Mobile program not found.\n\r", ch);
            return FALSE;
        }

        string_append( ch, &ed->comlist );

        return TRUE;
    }

    if ( !str_cmp( command, "delete" ) )
    {
        MPROG_DATA *ped = NULL;

        if ( progtype[0] == '\0' || progarg[0] == '\0' )
        {
            send_to_char( "Syntax:  prog delete [progtype] [progarg]\n\r",ch );
            return FALSE;
        }

        for ( ed = pMob->mobprogs; ed; ed = ed->next )
        {
            if ( is_name( progarg, ed->arglist )  &&
                 mprog_name_to_type(progtype) == ed->type )
                break;
            ped = ed;
        }

        if ( !ed )
        {
            send_to_char( "MEdit:  Mobile program not found.\n\r", ch);
            return FALSE;
        }

        if ( !ped )
            pMob->mobprogs = ed->next;
        else
            ped->next = ed->next;

        free_string( ed->arglist );
        free_string( ed->comlist );
        pMob->progtypes = 0;
        for ( ed = pMob->mobprogs; ed; ed = ed->next )
        {
          pMob->progtypes = pMob->progtypes | ed->type;
        }

        send_to_char( "Mobile program deleted.\n\r", ch );
        return TRUE;
    }

    if ( !str_cmp(command, "prioritize") )
    {
        MPROG_DATA *ped = NULL;

        if ( progtype[0] == '\0' || progarg[0] == '\0' )
        {
            send_to_char( "Syntax:  prog prioritize [progtype] [progarg]\n\r",ch );
            return FALSE;
        }

        for ( ed = pMob->mobprogs; ed; ed = ed->next )
        {
            if ( is_name( progarg, ed->arglist )  &&
                 mprog_name_to_type(progtype) == ed->type )
                break;
            ped = ed;
        }
	
        if ( !ed )
        {
            send_to_char( "MEdit:  Mobile program not found.\n\r", ch);
            return FALSE;
        }

        if ( !ped )
	{
	    send_to_char("MEdit: Prog already has priority.\n\r", ch);
	    return FALSE;
	}

        ped->next = ed->next;
	ed->next = pMob->mobprogs;
	pMob->mobprogs = ed;
	send_to_char("Mobile program prioritized.\n\r", ch);
	return TRUE;
    }

    medit_prog( ch, "" );
    return FALSE;
}



MEDIT( medit_long )
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  long [string]\n\r", ch );
	return FALSE;
    }

    free_string( pMob->long_descr );
    strcat( argument, "\n\r" );
    pMob->long_descr = str_dup( argument );
    pMob->long_descr[0] = UPPER( pMob->long_descr[0]  );

    send_to_char( "Long description set.\n\r", ch);
    return TRUE;
}



MEDIT( medit_short )
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  short [string]\n\r", ch );
	return FALSE;
    }

    free_string( pMob->short_descr );
    pMob->short_descr = str_dup( argument );

    send_to_char( "Short description set.\n\r", ch);
    return TRUE;
}



MEDIT( medit_name )
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  name [string]\n\r", ch );
	return FALSE;
    }

    free_string( pMob->player_name );
    pMob->player_name = str_dup( argument );

    send_to_char( "Name set.\n\r", ch);
    return TRUE;
}

MEDIT( medit_shop )
{
    MOB_INDEX_DATA *pMob;
    char command[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];

    argument = one_argument( argument, command );
    argument = one_argument( argument, arg1 );

    EDIT_MOB(ch, pMob);

    if ( command[0] == '\0' )
    {
	send_to_char( "Syntax:  shop hours [open] [close]\n\r", ch );
	send_to_char( "         shop profit [buying%] [selling%]\n\r", ch );
	send_to_char( "         shop type [0-4] [item type/\"none\"]\n\r", ch );
	send_to_char( "         shop delete\n\r", ch );
	send_to_char( "         shop maxbuy [value]\n\r", ch);
	return FALSE;
    }


    if ( !str_cmp( command, "hours" ) )
    {
	if ( arg1[0] == '\0' || !is_number( arg1 )
	|| argument[0] == '\0' || !is_number( argument ) )
	{
	    send_to_char( "Syntax:  shop hours [#xopening] [#xclosing]\n\r", ch );
	    return FALSE;
	}

	if ( !pMob->pShop )
	{
	    pMob->pShop         = new_shop();
	    pMob->pShop->keeper = pMob->vnum;
	    shop_last->next     = pMob->pShop;
	}

	pMob->pShop->open_hour = atoi( arg1 );
	pMob->pShop->close_hour = atoi( argument );

	send_to_char( "Shop hours set.\n\r", ch);
	return TRUE;
    }

    if ( !str_cmp( command, "maxbuy" ) )
    {
	if (arg1[0] == '\0' || !is_number(arg1))
	{
	    send_to_char("Syntax: shop maxbuy [price]\n\r", ch);
	    return FALSE;
	}

	if ( !pMob->pShop )
	{
	    pMob->pShop         = new_shop();
	    pMob->pShop->keeper = pMob->vnum;
	    shop_last->next     = pMob->pShop;
	}

	pMob->pShop->max_buy = atoi(arg1);

	send_to_char("Max purchase set.\n\r", ch);
	return TRUE;
    }

    if ( !str_cmp( command, "profit" ) )
    {
	if ( arg1[0] == '\0' || !is_number( arg1 )
	|| argument[0] == '\0' || !is_number( argument ) )
	{
	    send_to_char( "Syntax:  shop profit [#xbuying%] [#xselling%]\n\r", ch );
	    return FALSE;
	}

	if ( !pMob->pShop )
	{
	    pMob->pShop         = new_shop();
	    pMob->pShop->keeper = pMob->vnum;
	    shop_last->next     = pMob->pShop;
	}

	pMob->pShop->profit_buy     = atoi( arg1 );
	pMob->pShop->profit_sell    = atoi( argument );

	send_to_char( "Shop profit set.\n\r", ch);
	return TRUE;
    }


    if ( !str_cmp( command, "type" ) )
    {
	char buf[MAX_INPUT_LENGTH];
	int value;

	if (arg1[0] == '\0' || !is_number(arg1) || argument[0] == '\0')
	{
	    send_to_char( "Syntax:  shop type [#x0-4] [item type/\"none\"]\n\r", ch );
	    return FALSE;
	}

	if ( atoi( arg1 ) >= MAX_TRADE )
	{
	    sprintf( buf, "REdit:  May sell %d items max.\n\r", MAX_TRADE );
	    send_to_char( buf, ch );
	    return FALSE;
	}

	if (!str_cmp(argument, "none"))
	    value = 0;
	else if ((value = flag_value(type_flags, argument)) == NO_FLAG)
	{
	    send_to_char( "REdit:  That type of item is not known.\n\r", ch );
	    return FALSE;
	}

	if ( !pMob->pShop )
	{
	    pMob->pShop         = new_shop();
	    pMob->pShop->keeper = pMob->vnum;
	    shop_last->next     = pMob->pShop;
	}

	pMob->pShop->buy_type[atoi( arg1 )] = value;

	send_to_char( "Shop type set.\n\r", ch);
	return TRUE;
    }

    if (!str_cmp(command, "delete"))
    {
	if (!pMob->pShop)
	{
	    send_to_char("REdit: Non-existant shop.\n\r", ch);
	    return FALSE;
	}

	free_shop(pMob->pShop);
	pMob->pShop = NULL;
	send_to_char("Shop deleted.\n\r", ch);
	return TRUE;
    }
   


/*
    if ( !str_cmp( command, "delete" ) )
    {
	SHOP_DATA *pShop;
	SHOP_DATA *pShop_next;
	int value;
	int cnt = 0;
	
	if ( arg1[0] == '\0' || !is_number( arg1 ) )
	{
	    send_to_char( "Syntax:  shop delete [#x0-4]\n\r", ch );
	    return FALSE;
	}

	value = atoi( argument );
	
	if ( !pMob->pShop )
	{
	    send_to_char( "REdit:  Non-existant shop.\n\r", ch );
	    return FALSE;
	}

	if ( value == 0 )
	{
	    pShop = pMob->pShop;
	    pMob->pShop = pMob->pShop->next;
	    free_shop( pShop );
	}
	else
	for ( pShop = pMob->pShop, cnt = 0; pShop; pShop = pShop_next, cnt++ )
	{
	    pShop_next = pShop->next;
	    if ( cnt+1 == value )
	    {
		pShop->next = pShop_next->next;
		free_shop( pShop_next );
		break;
	    }
	}

	send_to_char( "Shop deleted.\n\r", ch);
	return TRUE;
    }
*/

    medit_shop( ch, "" );
    return FALSE;
}


/* ROM medit functions: */


MEDIT( medit_sex )          /* Moved out of medit() due to naming conflicts -- Hugin */
{
    MOB_INDEX_DATA *pMob;
    int value;

    if ( argument[0] != '\0' )
    {
	EDIT_MOB( ch, pMob );

	if ( ( value = flag_value( sex_flags, argument ) ) != NO_FLAG )
	{
	    pMob->sex = value;

	    send_to_char( "Sex set.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char( "Syntax: sex [sex]\n\r"
		  "Type '? sex' for a list of flags.\n\r", ch );
    return FALSE;
}

static void show_guild_syntax(CHAR_DATA * ch)
{
    send_to_char("Syntax:\n", ch);
    send_to_char("guild show: {Wlists all skills this mobile trains{x\n", ch);
    send_to_char("guild add <name>: {Wadds the unquoted skill or group of skills{x\n", ch);
    send_to_char("guild delete <name>: {Wremoves the unquoted skill or group of skills{x\n", ch);
}
	
MEDIT( medit_guild )          /* Moved out of medit() due to naming conflicts -- Hugin */
{
    MOB_INDEX_DATA *pMob;
    char arg1[MAX_STRING_LENGTH];
    int sn;
    int gn, i;
    GM_DATA *pGm;
    GM_DATA *newGm;

    if (argument[0] == '\0')
    {
        show_guild_syntax(ch);
        return false;
    }

	EDIT_MOB( ch, pMob );
	argument = one_argument(argument, arg1);

    if (!strcmp(arg1, "show"))
    {
        if (pMob->gm == NULL)
        {
            send_to_char("This mob teaches no skills.\n", ch);
            return false;
        }

        // Prepare guild
        static const unsigned int GuildBoxCount = 2;
        DisplayPanel::Box guildKeys[GuildBoxCount];
        DisplayPanel::Box guildValues[GuildBoxCount];
        for (unsigned int i(0); i < GuildBoxCount; ++i)
        {
            guildKeys[i].AddLine("#");
            guildKeys[i].AddLine("-");
            guildValues[i].AddLine("Skill Name");
            guildValues[i].AddLine("----------");
        }

        unsigned int col(0);
        unsigned int total(0);
        for (const GM_DATA * gm(pMob->gm); gm != NULL; gm = gm->next)
        {
            AddKey(guildKeys[col], Bracketize(makeString(gm->sn)));
            AddValue(guildValues[col], skill_table[gm->sn].name);
            col = (col + 1) % GuildBoxCount;
            ++total;
        }

        // Compose the panel
        DisplayPanel::HorizontalSplit guildPanel;
        for (unsigned int i(0); i < GuildBoxCount && i < total; ++i)
        {
            DisplayPanel::HorizontalSplit skillPanel(DisplayPanel::Options(DisplayPanel::Style_None));
            skillPanel.Add(guildKeys[i]);
            skillPanel.Add(guildValues[i]);
            guildPanel.Add(skillPanel);
        }

        // Compose the full panel
        DisplayPanel::VerticalSplit fullGuildPanel(DisplayPanel::Options(DisplayPanel::Style_None));
        fullGuildPanel.Add(DisplayPanel::Box("Skills Taught"));
        fullGuildPanel.Add(guildPanel);
        RenderPanelBuffered(fullGuildPanel, ch);
        return false;
    }

	if (!strcmp(arg1, "add"))
    {
	    if ((sn = skill_lookup_full(argument)) < 0)
	    {
	    	if ((gn = group_lookup(argument)) < 0)
	    	{
		    send_to_char("That skill/spell was not found.\n\r",ch);
		    return FALSE;
	        }
            for (i = 0; i < MAX_IN_GROUP ;i++)
	    	{
		    if (group_table[gn].spells[i] == NULL)
            	    	break;
		    sn = skill_lookup(group_table[gn].spells[i]);
		    pGm = pMob->gm;
		    newGm = (GM_DATA*)alloc_perm(sizeof(*pGm));
		    g_gm_count++;
		    newGm->sn = sn;
		    newGm->next = pGm;
		    pMob->gm = newGm;
	    	}
	    	return TRUE;
	    }

	    pGm = pMob->gm;
  	    newGm = (GM_DATA*)alloc_perm(sizeof(*pGm));
	    g_gm_count++;
	    newGm->sn = sn;
	    newGm->next = pGm;
	    pMob->gm = newGm;
	    return TRUE;
    }

    if (!strcmp(arg1, "delete"))
    {
	    if (!str_cmp(argument, "all"))
	    {
	    	pMob->gm = NULL;
	    	return TRUE;
	    }

	    if ((sn = skill_lookup(argument)) == 0)
	    {
	    	send_to_char("That skill/spell was not found.\n\r", ch);
	    	return FALSE;
	    }

	    newGm = pMob->gm;
	    if (newGm->sn == sn)
	    {
	    	pMob->gm = newGm->next;
	    	return TRUE;
	    }
	    for (pGm = pMob->gm ; pGm != NULL ; pGm = pGm->next)
	    {
	    	if (pGm->sn == sn)
	    	{
		    newGm->next = pGm->next;
		    return TRUE;
	    	}
	    	newGm = pGm;
	    }
    }

    show_guild_syntax(ch);	
    return false;
}

MEDIT( medit_act )          /* Moved out of medit() due to naming conflicts -- Hugin */
{
    MOB_INDEX_DATA *pMob;
    int value;

    if ( argument[0] != '\0' )
    {
	EDIT_MOB( ch, pMob );

	if ( ( value = flag_value( act_flags, argument ) ) != NO_FLAG )
	{
	    pMob->act ^= value;
	    SET_BIT( pMob->act, ACT_IS_NPC );

	    send_to_char( "Act flag toggled.\n\r", ch);
	    return TRUE;
	}

	if ( ( value = flag_value( nact_flags, argument ) ) != NO_FLAG )
	{
	    pMob->nact ^= value;
	    send_to_char( "Act flag toggled.\n\r", ch);
	    return TRUE;
        }
    }

    send_to_char( "Syntax: act [flag]\n\r"
		  "Type '? act' for a list of flags.\n\r", ch );
    return FALSE;
}

MEDIT( medit_affect )      /* Moved out of medit() due to naming conflicts -- Hugin */
{
    MOB_INDEX_DATA *pMob;
    int value;

    if ( argument[0] != '\0' )
    {
	EDIT_MOB( ch, pMob );

	if ( ( value = flag_value( affect_flags, argument ) ) != NO_FLAG )
	{
	    pMob->affected_by ^= value;

	    send_to_char( "Affect flag toggled.\n\r", ch);
	    return TRUE;
	}
    }

    send_to_char( "Syntax: affect [flag]\n\r"
		  "Type '? affect' for a list of flags.\n\r", ch );
    return FALSE;
}



MEDIT( medit_ac )
{
    MOB_INDEX_DATA *pMob;
    char arg[MAX_INPUT_LENGTH];
    int pierce, bash, slash, exotic;

    do   /* So that I can use break and send the syntax in one place */
    {
	if ( argument[0] == '\0' )  break;

	EDIT_MOB(ch, pMob);
	argument = one_argument( argument, arg );

	if ( !is_number( arg ) )  break;
	pierce = atoi( arg );
	argument = one_argument( argument, arg );

	if ( arg[0] != '\0' )
	{
	    if ( !is_number( arg ) )  break;
	    bash = atoi( arg );
	    argument = one_argument( argument, arg );
	}
	else
	    bash = pMob->ac[AC_BASH];

	if ( arg[0] != '\0' )
	{
	    if ( !is_number( arg ) )  break;
	    slash = atoi( arg );
	    argument = one_argument( argument, arg );
	}
	else
	    slash = pMob->ac[AC_SLASH];

	if ( arg[0] != '\0' )
	{
	    if ( !is_number( arg ) )  break;
	    exotic = atoi( arg );
	}
	else
	    exotic = pMob->ac[AC_EXOTIC];

	pMob->ac[AC_PIERCE] = pierce;
	pMob->ac[AC_BASH]   = bash;
	pMob->ac[AC_SLASH]  = slash;
	pMob->ac[AC_EXOTIC] = exotic;
	
	send_to_char( "Ac set.\n\r", ch );
	return TRUE;
    } while ( FALSE );    /* Just do it once.. */

    send_to_char( "Syntax:  ac [ac-pierce [ac-bash [ac-slash [ac-exotic]]]]\n\r"
		  "help MOB_AC  gives a list of reasonable ac-values.\n\r", ch );
    return FALSE;
}

MEDIT( medit_form )
{
    MOB_INDEX_DATA *pMob;
    int value;

    if ( argument[0] != '\0' )
    {
	EDIT_MOB( ch, pMob );

	if ( ( value = flag_value( form_flags, argument ) ) != NO_FLAG )
	{
	    pMob->form ^= value;
	    send_to_char( "Form toggled.\n\r", ch );
	    return TRUE;
	}
    }

    send_to_char( "Syntax: form [flags]\n\r"
		  "Type '? form' for a list of flags.\n\r", ch );
    return FALSE;
}

MEDIT( medit_part )
{
    MOB_INDEX_DATA *pMob;
    int value;

    if ( argument[0] != '\0' )
    {
	EDIT_MOB( ch, pMob );

	if ( ( value = flag_value( part_flags, argument ) ) != NO_FLAG )
	{
	    pMob->parts ^= value;
	    send_to_char( "Parts toggled.\n\r", ch );
	    return TRUE;
	}
    }

    send_to_char( "Syntax: part [flags]\n\r"
		  "Type '? part' for a list of flags.\n\r", ch );
    return FALSE;
}

MEDIT( medit_imm )
{
    MOB_INDEX_DATA *pMob;
    int value;

    if ( argument[0] != '\0' )
    {
	EDIT_MOB( ch, pMob );

	if ( ( value = flag_value( imm_flags, argument ) ) != NO_FLAG )
	{
	    pMob->imm_flags ^= value;
	    send_to_char( "Immunity toggled.\n\r", ch );
	    return TRUE;
	}
    }

    send_to_char( "Syntax: imm [flags]\n\r"
		  "Type '? imm' for a list of flags.\n\r", ch );
    return FALSE;
}

MEDIT( medit_assist )
{
    MOB_INDEX_DATA *pMob;
    int i, vnum;
    char varg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    argument = one_argument(argument, varg);

    if ((varg[0] != '\0') && is_number(varg))
    {
	EDIT_MOB( ch, pMob );

	vnum = atoi(varg);
	for (i = 0; i < MAX_ASSIST_VNUM; i++)
	    if (pMob->assist_vnum[i] == vnum)
	    {
		pMob->assist_vnum[i] = 0;
		sprintf(buf, "VNum %d removed from assist list.\n\r", vnum);
		send_to_char(buf, ch);
		return TRUE;
	    }

	for (i = 0; i < MAX_ASSIST_VNUM; i++)
	    if (pMob->assist_vnum[i] == 0)
	    {
		pMob->assist_vnum[i] = vnum;
		sprintf(buf, "VNum %d added to assist list.\n\r", vnum);
		send_to_char(buf, ch);
		return TRUE;
	    }

	send_to_char("Error: assist_vnum list full.\n\r", ch);
	return FALSE;
    }

   send_to_char("Usage: assist <vnum>\n\r", ch);
   return FALSE;
}

MEDIT( medit_res )
{
    MOB_INDEX_DATA *pMob;
    int i;
    char varg[MAX_STRING_LENGTH];

    argument = one_argument(argument, varg);

    if ((varg[0] != '\0') && is_number(argument))
    {
	EDIT_MOB( ch, pMob );

        for (i = 0; i < MAX_RESIST; i++)
	    if (!str_cmp(varg, resist_table[i].name))
	    {
		pMob->resist[i] = atoi(argument);
		if (pMob->resist[i] > 0)
		    send_to_char("Resistance added.\n\r", ch);
		else if (pMob->resist[i] < 0)
		    send_to_char("Vulnerability added.\n\r", ch);
		else
		    send_to_char("Value reset.\n\r", ch);
		return TRUE;
	    } 
    }

    send_to_char( "Syntax: res <resist-type> <value>\n\r"
		  "Type '? resist' for a list of resist-types.\n\r", ch );
    return FALSE;
}

MEDIT( medit_vuln )
{
    MOB_INDEX_DATA *pMob;
    int i;
    char varg[MAX_STRING_LENGTH];

    argument = one_argument(argument, varg);

    if ((varg[0] != '\0') && is_number(argument))
    {
	EDIT_MOB( ch, pMob );

	for (i = 0; i < MAX_RESIST; i++)
	    if (!str_cmp(varg, resist_table[i].name))
	    {
		pMob->resist[i] = -atoi(argument);
		if (pMob->resist[i] > 0)
		    send_to_char("Resistance added.\n\r", ch);
		else if (pMob->resist[i] < 0)
		    send_to_char("Vulnerability added.\n\r", ch);
		else
		    send_to_char("Value reset.\n\r", ch);
		return TRUE;
	    }
    }

    send_to_char( "Syntax: vuln <resist-type> <value>n\r"
		  "Type '? resist' for a list of resist-types.\n\r", ch );
    return FALSE;
}

MEDIT( medit_material )
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax:  material [string]\n\r", ch );
	return FALSE;
    }

    pMob->material = material_lookup(argument);

    send_to_char( "Material set.\n\r", ch);
    return TRUE;
}

MEDIT( medit_off )
{
    MOB_INDEX_DATA *pMob;
    int value;

    if ( argument[0] != '\0' )
    {
	EDIT_MOB( ch, pMob );

	if ( ( value = flag_value( off_flags, argument ) ) != NO_FLAG )
	{
	    pMob->off_flags ^= value;
	    send_to_char( "Offensive behaviour toggled.\n\r", ch );
	    return TRUE;
	}
    }

    send_to_char( "Syntax: off [flags]\n\r"
		  "Type '? off' for a list of flags.\n\r", ch );
    return FALSE;
}

MEDIT( medit_size )
{
    MOB_INDEX_DATA *pMob;
    int value;

    if ( argument[0] != '\0' )
    {
	EDIT_MOB( ch, pMob );

	if ( ( value = flag_value( size_flags, argument ) ) != NO_FLAG )
	{
	    pMob->size = value;
	    send_to_char( "Size set.\n\r", ch );
	    return TRUE;
	}
    }

    send_to_char( "Syntax: size [size]\n\r"
		  "Type '? size' for a list of sizes.\n\r", ch );
    return FALSE;
}

MEDIT( medit_hitdice )
{
    static char syntax[] = "Syntax:  hitdice <number> d <type> + <bonus>\n\r";
    char *num, *type, *bonus, *cp;
    MOB_INDEX_DATA *pMob;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
	send_to_char( syntax, ch );
	return FALSE;
    }

    num = cp = argument;

    while ( isdigit( *cp ) ) ++cp;
    while ( *cp != '\0' && !isdigit( *cp ) )  *(cp++) = '\0';

    type = cp;

    while ( isdigit( *cp ) ) ++cp;
    while ( *cp != '\0' && !isdigit( *cp ) ) *(cp++) = '\0';

    bonus = cp;

    while ( isdigit( *cp ) ) ++cp;
    if ( *cp != '\0' ) *cp = '\0';

    if ( ( !is_number( num   ) || atoi( num   ) < 1 )
    ||   ( !is_number( type  ) || atoi( type  ) < 1 ) 
    ||   ( !is_number( bonus ) || atoi( bonus ) < 0 ) )
    {
	send_to_char( syntax, ch );
	return FALSE;
    }

    pMob->hit[DICE_NUMBER] = atoi( num   );
    pMob->hit[DICE_TYPE]   = atoi( type  );
    pMob->hit[DICE_BONUS]  = atoi( bonus );

    send_to_char( "Hitdice set.\n\r", ch );
    return TRUE;
}

MEDIT( medit_manadice )
{
    static char syntax[] = "Syntax:  manadice <number> d <type> + <bonus>\n\r";
    char *num, *type, *bonus, *cp;
    MOB_INDEX_DATA *pMob;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
	send_to_char( syntax, ch );
	return FALSE;
    }

    num = cp = argument;

    while ( isdigit( *cp ) ) ++cp;
    while ( *cp != '\0' && !isdigit( *cp ) )  *(cp++) = '\0';

    type = cp;

    while ( isdigit( *cp ) ) ++cp;
    while ( *cp != '\0' && !isdigit( *cp ) ) *(cp++) = '\0';

    bonus = cp;

    while ( isdigit( *cp ) ) ++cp;
    if ( *cp != '\0' ) *cp = '\0';

    if ( !( is_number( num ) && is_number( type ) && is_number( bonus ) ) )
    {
	send_to_char( syntax, ch );
	return FALSE;
    }

    if ( ( !is_number( num   ) || atoi( num   ) < 1 )
    ||   ( !is_number( type  ) || atoi( type  ) < 1 ) 
    ||   ( !is_number( bonus ) || atoi( bonus ) < 0 ) )
    {
	send_to_char( syntax, ch );
	return FALSE;
    }

    pMob->mana[DICE_NUMBER] = atoi( num   );
    pMob->mana[DICE_TYPE]   = atoi( type  );
    pMob->mana[DICE_BONUS]  = atoi( bonus );

    send_to_char( "Manadice set.\n\r", ch );
    return TRUE;
}

MEDIT( medit_damdice )
{
    static char syntax[] = "Syntax:  damdice <number> d <type> + <bonus>\n\r";
    char *num, *type, *bonus, *cp;
    MOB_INDEX_DATA *pMob;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
	send_to_char( syntax, ch );
	return FALSE;
    }

    num = cp = argument;

    while ( isdigit( *cp ) ) ++cp;
    while ( *cp != '\0' && !isdigit( *cp ) )  *(cp++) = '\0';

    type = cp;

    while ( isdigit( *cp ) ) ++cp;
    while ( *cp != '\0' && !isdigit( *cp ) ) *(cp++) = '\0';

    bonus = cp;

    while ( isdigit( *cp ) ) ++cp;
    if ( *cp != '\0' ) *cp = '\0';

    if ( !( is_number( num ) && is_number( type ) && is_number( bonus ) ) )
    {
	send_to_char( syntax, ch );
	return FALSE;
    }

    if ( ( !is_number( num   ) || atoi( num   ) < 1 )
    ||   ( !is_number( type  ) || atoi( type  ) < 1 ) 
    ||   ( !is_number( bonus ) || atoi( bonus ) < 0 ) )
    {
	send_to_char( syntax, ch );
	return FALSE;
    }

    pMob->damage[DICE_NUMBER] = atoi( num   );
    pMob->damage[DICE_TYPE]   = atoi( type  );
    pMob->damage[DICE_BONUS]  = atoi( bonus );

    send_to_char( "Damdice set.\n\r", ch );
    return TRUE;
}

MEDIT( medit_language )
{
    MOB_INDEX_DATA *pMob;
    int i;
    char buf[MAX_STRING_LENGTH];

    if (argument[0] != '\0')
 	for (i = 0; i < MAX_LANGUAGE; i++)
	    if (!str_cmp(argument, lang_data[i].name))
	    {
		EDIT_MOB(ch, pMob);
		if (IS_SET(pMob->lang_flags, (1 << i)))
		{
		    REMOVE_BIT(pMob->lang_flags, (1 << i));
		    send_to_char("Language removed.\n\r", ch);
		}
		else
		{
		    SET_BIT(pMob->lang_flags, (1 << i));
		    send_to_char("Language set.\n\r", ch);
		}
		return TRUE;
	    }

    send_to_char("Invalid language.  Available languages are:\n\r", ch);

    for (i = 0; i < MAX_LANGUAGE; i++)
    {
	sprintf(buf, "%s ", lang_data[i].name);
	send_to_char(buf, ch);
    }
    send_to_char("\n\r", ch);
    return FALSE;
}


MEDIT( medit_race )
{
    MOB_INDEX_DATA *pMob;
    int race, i;
    bool updateres = TRUE;

    if ( argument[0] != '\0'
    && ( race = race_lookup( argument ) ) != 0 )
    {
	EDIT_MOB( ch, pMob );

        for (i = 0; i < MAX_RESIST; i++)
	    if (race_table[pMob->race].resist[i] != pMob->resist[i])
		updateres = FALSE;

	if (pMob->lang_flags == (1 << race_table[pMob->race].native_tongue))
	    pMob->lang_flags = (1 << race_table[race].native_tongue);

	pMob->race = race;
	pMob->off_flags   |= race_table[race].off;
	pMob->imm_flags   |= race_table[race].imm;
	pMob->res_flags   |= race_table[race].res;
	pMob->vuln_flags  |= race_table[race].vuln;
	pMob->form        |= race_table[race].form;
	pMob->parts       |= race_table[race].parts;

	if (updateres)
	    for (i = 0; i < MAX_RESIST; i++)
		pMob->resist[i] = race_table[pMob->race].resist[i];

	send_to_char( "Race set.\n\r", ch );
	return TRUE;
    }

    if ( argument[0] == '?' )
    {
	char buf[MAX_STRING_LENGTH];

	send_to_char( "Available races are:", ch );

	for ( race = 0; race_table[race].name != NULL; race++ )
	{
	    if ( ( race % 3 ) == 0 )
		send_to_char( "\n\r", ch );
	    sprintf( buf, " %-15s", race_table[race].name );
	    send_to_char( buf, ch );
	}

	send_to_char( "\n\r", ch );
	return FALSE;
    }

    send_to_char( "Syntax:  race [race]\n\r"
		  "Type 'race ?' for a list of races.\n\r", ch );
    return FALSE;
}

MEDIT( medit_class )
{
    MOB_INDEX_DATA *pMob;
    int class_num;

    EDIT_MOB( ch, pMob );

    if (!IS_SET(pMob->nact, ACT_CLASSED))
    {
	send_to_char("Set 'classed' act flag first.\n\r", ch);
	return FALSE;
    }

    if ( argument[0] != '\0'
    && ( class_num = class_lookup( argument ) ) != 0 )
    {
	pMob->class_num = class_num;

	send_to_char( "Class set.\n\r", ch );
	return TRUE;
    }

    if (argument[0] == '\0')
	send_to_char("Syntax: class [class]\n\r", ch);
    else
	send_to_char("Invalid class.\n\r", ch);

    return FALSE;
}



MEDIT( medit_position )
{
    MOB_INDEX_DATA *pMob;
    char arg[MAX_INPUT_LENGTH];
    int value;

    argument = one_argument( argument, arg );

    switch ( arg[0] )
    {
    default:
	break;

    case 'S':
    case 's':
	if ( str_prefix( arg, "start" ) )
	    break;

	if ( ( value = flag_value( position_flags, argument ) ) == NO_FLAG )
	    break;

	EDIT_MOB( ch, pMob );

	pMob->start_pos = value;
	send_to_char( "Start position set.\n\r", ch );
	return TRUE;

    case 'D':
    case 'd':
	if ( str_prefix( arg, "default" ) )
	    break;

	if ( ( value = flag_value( position_flags, argument ) ) == NO_FLAG )
	    break;

	EDIT_MOB( ch, pMob );

	pMob->default_pos = value;
	send_to_char( "Default position set.\n\r", ch );
	return TRUE;
    }

    send_to_char( "Syntax:  position [start/default] [position]\n\r"
		  "Type '? position' for a list of positions.\n\r", ch );
    return FALSE;
}


MEDIT( medit_gold )
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
	send_to_char( "Syntax:  wealth [number]\n\r", ch );
	return FALSE;
    }

    pMob->wealth = atoi( argument );

    send_to_char( "Wealth set.\n\r", ch);
    return TRUE;
}

MEDIT( medit_hitroll )
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
	send_to_char( "Syntax:  hitroll [number]\n\r", ch );
	return FALSE;
    }

    pMob->hitroll = atoi( argument );

    send_to_char( "Hitroll set.\n\r", ch);
    return TRUE;
}

void show_matlist(CHAR_DATA *ch)
{
	BUFFER *buffer;
	int mat;
	char buf[MAX_STRING_LENGTH];

	buffer = new_buf();
	for (mat = 0; mat < MAX_MATERIALS; mat += 5)
	{
		sprintf(buf, "%-15s %-15s %-15s %-15s %-15s\n\r", (mat < MAX_MATERIALS ? material_table[mat].name : ""),
			(mat+1 < MAX_MATERIALS ? material_table[mat+1].name : ""), 
			(mat+2 < MAX_MATERIALS ? material_table[mat+2].name : ""),
			(mat+3 < MAX_MATERIALS ? material_table[mat+3].name : ""),
			(mat+4 < MAX_MATERIALS ? material_table[mat+4].name : ""));
		add_buf(buffer, buf);
	}
	page_to_char(buf_string(buffer),ch);
	free_buf(buffer);
	return;		
}

void show_liqlist(CHAR_DATA *ch)
{
    int liq;
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];
    
    buffer = new_buf();
    
    for ( liq = 0; liq_table[liq].liq_name != NULL; liq++)
    {
	if ( (liq % 21) == 0 )
	    add_buf(buffer,"Name                 Color          Proof Full Thirst Food Ssize\n\r");

	sprintf(buf, "%-20s %-14s %5d %4d %6d %4d %5d\n\r",
		liq_table[liq].liq_name,liq_table[liq].liq_color,
		liq_table[liq].liq_affect[0],liq_table[liq].liq_affect[1],
		liq_table[liq].liq_affect[2],liq_table[liq].liq_affect[3],
		liq_table[liq].liq_affect[4] );
	add_buf(buffer,buf);
    }

    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);

return;
}

void show_damlist(CHAR_DATA *ch)
{
    int att;
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];
    
    buffer = new_buf();
    
    for ( att = 0; damtype_table[att].name != NULL; att++)
    {
	if ( (att % 21) == 0 )
	    add_buf(buffer,"Name                 Noun\n\r");

	sprintf(buf, "%-20s\n\r",
		damtype_table[att].name);
	add_buf(buffer,buf);
    }

    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);

return;
}

void show_herblist(CHAR_DATA *ch)
{
    int herb;
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];
    
    buffer = new_buf();
   
   add_buf(buffer,"Name                  MLvl  Rar  Spell 1/3            Spell 2/4\n\r");

    for ( herb = 0; herb_table[herb].name; herb++)
    {
	sprintf(buf, "%-21s %-4d  %-3d  %-21s %-21s\n\r",
	    herb_table[herb].name, herb_table[herb].min_level,
	    herb_table[herb].rarity,
	    (herb_table[herb].spell[0] ? skill_table[*herb_table[herb].spell[0]].name : "none"),
	    (herb_table[herb].spell[1] ? skill_table[*herb_table[herb].spell[1]].name : "none"));
	add_buf(buffer,buf);
	sprintf(buf, "                                 %-21s %-21s\n\r",
	    (herb_table[herb].spell[2] ? skill_table[*herb_table[herb].spell[2]].name : "none"),
	    (herb_table[herb].spell[3] ? skill_table[*herb_table[herb].spell[3]].name : "none"));
	add_buf(buffer, buf);
    }

    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);

return;
}


