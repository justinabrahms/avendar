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
#if defined(WIN32)
#include <Winsock2.h>
#else
#define INVALID_SOCKET -1
#endif
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <cmath>
#include <stdlib.h>
#include <list>
#include <fstream>
#include <sstream>
#include "merc.h"
#include "recycle.h"
#include "lookup.h"
#include "tables.h"
#include "languages.h"
#include "dictionary.h"
#include "PyreInfo.h"
#include "PhantasmInfo.h"
#include "Faction.h"
#include "NameMaps.h"
 
#if !defined(macintosh)
extern  int     _filbuf         args( (FILE *) );
#endif

extern bool rip_process;

int rename(const char *oldfname, const char *newfname);

char *print_flags(int flag)
{
    int count, pos = 0;
    static char buf[52];


    for (count = 0; count < 32;  count++)
    {
        if (IS_SET(flag,1<<count))
        {
            if (count < 26)
                buf[pos] = 'A' + count;
            else
                buf[pos] = 'a' + (count - 26);
            pos++;
        }
    }

    if (pos == 0)
    {
        buf[pos] = '0';
        pos++;
    }

    buf[pos] = '\0';

    return buf;
}


/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST	100
static	OBJ_DATA *	rgObjNest	[MAX_NEST];

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

/* provided to free strings */
#if defined(KEYS)
#undef KEYS
#endif

#define KEYS( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    free_string(field);			\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

/*
 * Local functions.
 */
void	fwrite_char	args( ( CHAR_DATA *ch,  FILE *fp ) );
void	fwrite_account  args( ( ACCOUNT_DATA *acct ) );
void 	fwrite_obj_list(CHAR_DATA *ch, OBJ_DATA *obj, FILE *fp, int iNest);
void	fwrite_obj	args( ( CHAR_DATA *ch,  OBJ_DATA  *obj,
			    FILE *fp, int iNest ) );
void fwrite_obj_room	args(( ROOM_INDEX_DATA *room, OBJ_DATA *obj, FILE *fp, int iNest ));
void	fwrite_pet	args( ( CHAR_DATA *pet, FILE *fp) );
void	fread_char	args( ( CHAR_DATA *ch,  FILE *fp ) );
void    fread_pet	args( ( CHAR_DATA *ch,  FILE *fp ) );
void	fread_obj	args( ( CHAR_DATA *ch,  FILE *fp ) );

static std::set<int> s_savedRooms;

void add_save_room(int vnum)
{
    s_savedRooms.insert(vnum);
}

void clear_room_obj(ROOM_INDEX_DATA *room)
{
    std::set<int>::iterator iter(s_savedRooms.find(room->vnum));
    if (iter == s_savedRooms.end())
        return;

    char strsave[MAX_INPUT_LENGTH];
    sprintf(strsave, "%s%d", ROOMSAVE_DIR, room->vnum);
    s_savedRooms.erase(iter);
    if (remove(strsave) != 0)
        bug("Failed to delete room file %d", room->vnum);
}

void save_room_obj( ROOM_INDEX_DATA *room )
{
    char strsave[MAX_INPUT_LENGTH];
    FILE *fp;

    if (room == NULL)
        return;
        
    fclose( fpReserve );
    add_save_room(room->vnum);
    sprintf( strsave, "%s%d", ROOMSAVE_DIR, room->vnum );
    if ( ( fp = fopen( TEMP_FILE, "w" ) ) == NULL )
    {
	bug( "Save_char_obj: fopen", 0 );
	perror( strsave );
    }
    else
    {
	fprintf(fp, "\n\rRoom %d\n\r", room->vnum);
	if (room->contents)
	    fwrite_obj_room( room, room->contents, fp, 0 );
        fprintf( fp, "#DONE\n" );
    }
    fclose( fp );
    rename(TEMP_FILE,strsave);
    fpReserve = fopen( NULL_FILE, "r" );
}

void save_room_desc( ROOM_INDEX_DATA *room )
{
    char strsave[MAX_INPUT_LENGTH];
    FILE *fp;
    EXTRA_DESCR_DATA *ed;

    fclose(fpReserve);
    sprintf(strsave, "%s%d.desc", ROOMSAVE_DIR, room->vnum);
    if ( ( fp = fopen( TEMP_FILE, "w" ) ) == NULL )
    {
	bug( "Save_char_obj: fopen", 0 );
	perror( strsave );
    }
    else
    {
	fprintf(fp, "Rnum %d\n", room->vnum);
	fprintf(fp, "Desc %s~\n", fwrite_format(room->description));
	
	for (ed = room->extra_descr; ed; ed = ed->next)
	{
	    fprintf(fp, "ExDe %s~\n", ed->keyword);
	    fprintf(fp, "%s~\n", ed->description);
	}
    }

    fprintf(fp, "#DONE\n");
    fclose(fp);
    rename(TEMP_FILE, strsave);
    fpReserve = fopen(NULL_FILE, "r");
    return;
}

HEADER_DATA *add_denied(char *name)
{
    HEADER_DATA *hdp;
    char buf[MAX_STRING_LENGTH];
    FILE *fp;

    hdp = new_header();
    hdp->name = str_dup(name);
    hdp->next = g_denied_headers;
    g_denied_headers = hdp;

    dict_insert(g_denied_names, name, hdp);

    sprintf(buf, "%s%s", PLAYER_DIR, DENIEDNAME_FILE);

    if ((fp = fopen(buf, "a+")) == NULL)
    {
	bug("add_denied: error opening playerlist.", 0);
	return NULL;
    }

    fprintf(fp, "%s\n", name);

    fclose(fp);

    return hdp;
}

bool unlink_header(HEADER_DATA *header, HEADER_DATA **hlist)
{
    HEADER_DATA *hdp;

    if (header == *hlist)
    {
	*hlist = header->next;
    }
    else
    {
	hdp = *hlist;

	while (hdp && hdp->next != header)
	    hdp = hdp->next;

	if (!hdp)
	    return FALSE;

	hdp->next = hdp->next->next;
    }

    return TRUE;
}

void remove_denied(HEADER_DATA *header)
{
    HEADER_DATA *hdp;
    char tempname[MAX_STRING_LENGTH];
    char realname[MAX_STRING_LENGTH];
    FILE *fp;

    if (!header)
	return;

    if (!unlink_header(header, &g_denied_headers))
	return;

    dict_insert(g_denied_names, header->name, NULL);
    free_header(header);

    sprintf(tempname, "%s%s", PLAYER_DIR, TEMP_FILE);
    if ((fp = fopen(tempname, "w")) == NULL)
    {
	bug("remove_header: error opening temporary file.", 0);
	return;
    }

    for (hdp = g_denied_headers; hdp; hdp = hdp->next)
	fprintf(fp, "%s\n", hdp->name);

    fclose(fp);

    sprintf(realname, "%s%s", PLAYER_DIR, DENIEDNAME_FILE);
    remove(realname);
    rename(tempname, realname);

    return;
}

void remove_header(HEADER_DATA *header)
{
    HEADER_DATA *hdp;
    char tempname[MAX_STRING_LENGTH];
    char realname[MAX_STRING_LENGTH];
    FILE *fp;

    if (!header)
	return;

    if (!unlink_header(header, &g_active_headers))
    {
	bug("unlink header returned false.", 0);
	return;
    }

    dict_insert(g_char_names, header->name, NULL);
    free_header(header);

    sprintf(tempname, "%s%s", PLAYER_DIR, TEMP_FILE);
    if ((fp = fopen(tempname, "w")) == NULL)
    {
	bug("remove_header: error opening temporary file.", 0);
	return;
    }

    for (hdp = g_active_headers; hdp; hdp = hdp->next)
	fprintf(fp, "%s\n", hdp->name);

    fclose(fp);

    sprintf(realname, "%s%s", PLAYER_DIR, PLAYERLIST);
    remove(realname);
    rename(tempname, realname);

    return;
}

HEADER_DATA *add_header(char *name)
{
    HEADER_DATA *hdp;
    char buf[MAX_STRING_LENGTH];
    FILE *fp;

    hdp = new_header();
    hdp->name = str_dup(name);
    hdp->next = g_active_headers;
    g_active_headers = hdp;

    dict_insert(g_char_names, name, hdp);

    sprintf(buf, "%s%s", PLAYER_DIR, PLAYERLIST);

    if ((fp = fopen(buf, "a+")) == NULL)
    {
	bug("add_header: error opening playerlist.", 0);
	return NULL;
    }

    fprintf(fp, "%s\n", name);

    fclose(fp);

    return hdp;
}

void move_header(CHAR_DATA *ch, bool to_denied)
{
    char *name;
    name = str_dup(ch->name);
    if (to_denied)
    {
	if (!ch->pcdata->header)
	    ch->pcdata->header = (HEADER_DATA*)dict_lookup(g_char_names, name);

	if (ch->pcdata->header)
	    remove_header(ch->pcdata->header);

	ch->pcdata->header = add_denied(name);
    }
    else
    {
	if (!ch->pcdata->header)
	    ch->pcdata->header = (HEADER_DATA*)dict_lookup(g_denied_names, name);

	if (ch->pcdata->header)
	    remove_denied(ch->pcdata->header);

	ch->pcdata->header = add_header(name);
    }
}


    
/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj( CHAR_DATA *ch )
{
    char strsave[MAX_INPUT_LENGTH];
    FILE *fp;

    if (IS_NPC(ch) || rip_process)
	return;

    if ( ch->desc != NULL && ch->desc->original != NULL )
	ch = ch->desc->original;

    if (ch->level <= 0)
	return;

    if (!ch->pcdata->header)
    {
	// Step 1: Look for a header.
	if ((ch->pcdata->header = (HEADER_DATA*)dict_lookup(g_char_names, ch->name)) == NULL)
	{
	    // Step 2: No header found, create a new header.
	    if (!IS_SET(ch->act, PLR_DENY))
		ch->pcdata->header = add_header(ch->name);
	    else
	    {
		if ((ch->pcdata->header = (HEADER_DATA*)dict_lookup(g_denied_names, ch->name)) == NULL)
		    add_denied(ch->name);
	    }
	}
    }

    fclose( fpReserve );
    sprintf( strsave, "%s%s", (IS_SET(ch->act, PLR_DENY) ? DENIED_DIR : PLAYER_DIR), capitalize( ch->name ) );
    if ( ( fp = fopen( TEMP_FILE, "w" ) ) == NULL )
    {
        bug( "Save_char_obj: fopen", 0 );
        perror( strsave );
    }
    else
    {
    	fwrite_char( ch, fp );
	    if ( ch->carrying != NULL )
	        fwrite_obj_list( ch, ch->carrying, fp, 0 );

        // Save off pets
        if (ch->in_room == NULL)
        {
            // If character has no room, iterate the world
            for (CHAR_DATA * pet(char_list); pet != NULL; pet = pet->next)
            {
                // Save off the pet
                if (IS_NPC(pet) && pet->master == ch && pet->leader == ch)
                    fwrite_pet(pet, fp);
            }
        }
        else
        {
            // Iterate the characters in the room
            for (CHAR_DATA * pet(ch->in_room->people); pet != NULL; pet = pet->next_in_room)
            {
                // Save off this character's pets
                if (IS_NPC(pet) && pet->master == ch && pet->leader == ch)
                    fwrite_pet(pet, fp);
            }
        }
	
        fprintf( fp, "#END\n" );
    }
    fclose( fp );
    rename(TEMP_FILE,strsave);

    if (ch->desc && ch->desc->acct)
    {
	if (!is_name(ch->name, ch->desc->acct->chars))
	{
	    char buf[MAX_STRING_LENGTH];
	    sprintf(buf, "%s %s", ch->desc->acct->chars, ch->name);
	    free_string(ch->desc->acct->chars);
	    ch->desc->acct->chars = str_dup(buf);
	}

	fwrite_account(ch->desc->acct);
    }

    fpReserve = fopen( NULL_FILE, "r" );
}

void fread_rumors( void )
{
    FILE *fp;
    RUMORS *nr(NULL);
    const char *word(0);
    bool rc = TRUE;

    if ((fp = fopen(RUMOR_FILE, "r")) != NULL)
    {
	for ( ; ; )
	{
	    if (feof(fp))
	    {
		if (!rc)
		    log_string("rumor incomplete");
		return;
	    }
	    else
		word = fread_word(fp);
	    if (!str_cmp(word,"END"))
	    {
		if (!rc)
		{
		    log_string("rumor incomplete");
		    INVALIDATE(nr);
		}
		fclose(fp);
		return;
	    }		
	    else if (!str_cmp(word,"Name"))
	    {
		nr = new_rumor();
		nr->name = fread_string(fp);
		rc = FALSE;
	    }
	    else if (!str_cmp(word,"Stamp"))
		nr->stamp = fread_number(fp);
	    else if (!str_cmp(word,"Sticky"))
		nr->sticky = fread_number(fp);
	    else if (!str_cmp(word,"Text"))
	    {
		nr->text = fread_string(fp);
		rc = TRUE;
	    }
	    else
	    {
		fclose(fp);
		return;
	    }
	}
    }
};

void fwrite_rumors( void )
{
    FILE *fp;
    RUMORS *nr;

    if((fp = fopen(RUMOR_FILE, "w")) != NULL)
    {
	for (nr = rumor_list;nr;nr=nr->next)
	    if (IS_VALID(nr))
	    {
		fprintf(fp, "Name %s~\n", nr->name);
		fprintf(fp, "Stamp %ld\n", nr->stamp);
		fprintf(fp, "Sticky %i\n", nr->sticky);
		fprintf(fp, "Text %s~\n", nr->text);
	    }
	fprintf(fp, "END\n\r");
	fclose(fp);
    }
}

void fwrite_account( ACCOUNT_DATA *acct )
{
    char strsave[MAX_INPUT_LENGTH];
    FILE *fp;

    sprintf(strsave, "%s%s", ACCT_DIR, capitalize(acct->name));
    if ((fp = fopen(strsave, "w")) != NULL)
    {
	fprintf(fp, "Name %s~\n", acct->name);
	fprintf(fp, "Flag %s\n", print_flags(acct->flags));
	fprintf(fp, "Char %s~\n", acct->chars);
	fprintf(fp, "Pwd  %s~\n", acct->pwd);
	fprintf(fp, "Email %s~\n", acct->email);
	fprintf(fp, "Dele %s~\n", acct->deleted);
	fprintf(fp, "Data %s~\n", acct->immrecord);
	fprintf(fp, "Pnts %d\n", acct->award_points);
	fprintf(fp, "Sock %s~\n", acct->socket_info);
	fprintf(fp, "Pmpt %s~\n", acct->def_prompt);
	fprintf(fp, "End\n\n");
	fclose(fp);
    }
}


/*
 * Write the char.
 */
void fwrite_char( CHAR_DATA *ch, FILE *fp )
{
    FILE *sFile;
    AFFECT_DATA *paf,haf;
    EXTRA_DESCR_DATA *ed;
    int sn, gn, pos, x;
    char fname[128];
    char buf[MAX_STRING_LENGTH];
    
    if (!IS_NPC(ch))
    {
        sprintf(fname, "%s%s", TRAVELPATH, ch->name);

        if ((sFile = fopen(fname, "w")) != NULL)
        {
            fwrite(ch->pcdata->travelptr, 1, 8192, sFile);
            fclose(sFile);
        }

        sprintf(fname, "%s%s", MEMPATH, ch->name);

        if ((sFile = fopen(fname, "w")) != NULL)
        {
            fwrite(ch->pcdata->bitptr, 1, 8192, sFile);
            fclose(sFile);
        }

        sprintf(fname, "%s%s.fac", FACTION_DIR, ch->name);
        std::ofstream out(fname);
        if (out.is_open())
            out << ch->pcdata->faction_standing->Serialize();
        else
        {
            std::ostringstream mess;
            mess << "Failed to open faction file for writing '" << fname << "'";
            bug(mess.str().c_str(), 0);
        }
    }

    if (ch->desc && ch->desc->acct)
        SET_BIT(ch->act, PLR_ACCOUNT);

    fprintf( fp, "#%s\n", IS_NPC(ch) ? "MOB" : "PLAYER"	);

    fprintf( fp, "Name %s~\n",	fwrite_format(ch->name)	);
    fprintf( fp, "Id   %ld\n",  ch->id			);
    
    if (ch != ol_char)
    {
	fprintf( fp, "LogO %ld\n",	(long) current_time	);
	fprintf( fp, "LastOn %s\n", (char *) ctime( &current_time ));
	fprintf( fp, "Plyd %d\n", ch->played + (int) (current_time - ch->logon));
	if ((current_time - ch->logon) >= 2700)
	    ch->lasthour = current_time;
    }
    else
    {
	fprintf( fp, "LogO %ld\n",	(long) ch->laston	);
	fprintf( fp, "LastOn %s\n", (char *) ctime( &ch->laston ));
	fprintf( fp, "Plyd %d\n", ch->played);
    }

    fprintf( fp, "Hour %ld\n",	ch->lasthour);
    fprintf( fp, "Vers %d\n",  PFILE_VERSION);
    if (ch->short_descr[0] != '\0')
      	fprintf( fp, "ShD  %s~\n",	fwrite_format(ch->short_descr)	);
    if (ch->long_descr[0] != '\0')
	fprintf( fp, "LnD  %s~\n",	fwrite_format(ch->long_descr)	);
    if (ch->orig_long[0] != '\0')
	fprintf( fp, "OLng %s~\n",	fwrite_format(ch->orig_long)	);
    if (ch->orig_description[0] != '\0')
      	fprintf( fp, "ODes %s~\n",	fwrite_format(ch->orig_description));
    if (ch->pcdata->immrecord && ch->pcdata->immrecord[0] != '\0')
    	fprintf( fp, "IRec %s~\n",	fwrite_format(ch->pcdata->immrecord));
    if (ch->pcdata->record && ch->pcdata->record[0] != '\0')
    	fprintf( fp, "Rec %s~\n",	fwrite_format(ch->pcdata->record));
    if (ch->description && ch->description[0] != '\0')
    	fprintf( fp, "Desc %s~\n",	fwrite_format(ch->description)	);
    if (ch->pcdata->background && ch->pcdata->background[0] != '\0')
	fprintf( fp, "Back %s~\n",	fwrite_format(ch->pcdata->background) );
    if (ch->pcdata->recycle_log && ch->pcdata->recycle_log[0] != '\0')
	fprintf( fp, "Recy %s~\n",	fwrite_format(ch->pcdata->recycle_log) );

    if (ch->pcdata->socket_info && ch->pcdata->socket_info[0] != '\0')
	fprintf( fp, "Sock %s~\n", 	fwrite_format(ch->pcdata->socket_info) );

    for (ed = ch->pcdata->extra_descr; ed; ed = ed->next)
    {
	fprintf( fp, "ExDesc %d %s~ ",	ed->can_edit, fwrite_format(ed->keyword));
	fprintf( fp, "%s~\n", 		fwrite_format(ed->description));
    }

    for (x = 0; x < MAX_SONGBOOK; x++)
	if (ch->pcdata->songbook[x])
	    fprintf(fp, "SongB %d %s~\n", x, fwrite_format(ch->pcdata->songbook[x]));

    if (ch->prompt != NULL || !str_cmp(ch->prompt,"<%hhp %mm %vmv> "))
        fprintf( fp, "Prom %s~\n",      fwrite_format(ch->prompt)	);
    if (ch->battlecry != NULL)
	fprintf( fp, "Bcry %s~\n",	fwrite_format(ch->battlecry)   );
    fprintf( fp, "Race %s~\n", fwrite_format(race_table[ch->race].name) );
    if (ch->fake_race >= 0)
	fprintf(fp, "FRace %s~\n", fwrite_format(race_table[ch->fake_race].name));
    if (ch->clan)
    	fprintf( fp, "Clan %s~\n", fwrite_format(clan_table[ch->clan].who_name));
    fprintf( fp, "Sex  %d\n",	ch->sex			);
    fprintf( fp, "Speak  %d\n", ch->speaking		);
    fprintf( fp, "AgeG  %d\n",  ch->pcdata->age_group   );
    fprintf( fp, "AgeL	%d\n",	ch->pcdata->last_age	);
    fprintf( fp, "AgeM  %d\n",  ch->pcdata->max_age     );
    fprintf( fp, "AgeP  %d\n",  ch->pcdata->age_mod_perm);
    fprintf( fp, "Cla  %d\n",	ch->class_num		);
    fprintf( fp, "Levl %d\n",	ch->level		);
    fprintf( fp, "Tru  %d\n",	ch->trust	);
    fprintf( fp, "Sec  %d\n",    ch->pcdata->security	);	/* OLC */
    fprintf( fp, "Ethos  %d\n",    ch->pcdata->ethos    );
    fprintf( fp, "Major  %d\n",    ch->pcdata->major_sphere  );
    fprintf( fp, "Minor  %d\n",    ch->pcdata->minor_sphere  );
    fprintf( fp, "ChosenPath  %d\n", ch->pcdata->chosen_path);
    fprintf( fp, "Dailyxp  %d\n", ch->pcdata->dailyxp);
    for (x = 0; x < MAX_CLORE; x++)
	if (ch->lcreature[x] > 0)
	    fprintf(fp, "LrnC %d %d %d\n", x, ch->lcreature[x], ch->lpercent[x]);
    fprintf( fp, "Wanted  %d\n",   ch->pcdata->times_wanted  );
    fprintf( fp, "Deaths  %d\n",   ch->pcdata->death_count   );
    fprintf( fp, "MxDths  %d\n",   ch->pcdata->max_deaths    );
    fprintf( fp, "PKills  %d\n",   ch->pcdata->pkills	     );
    fprintf( fp, "AKills  %d %d %d\n",
	ch->pcdata->align_kills[0],
	ch->pcdata->align_kills[1],
	ch->pcdata->align_kills[2]);
    fprintf( fp, "Not  %ld %ld %ld %ld %ld %ld\n",		
	(long) ch->pcdata->last_note, (long) ch->pcdata->last_idea,
	(long) ch->pcdata->last_penalty, (long) ch->pcdata->last_news,
	(long) ch->pcdata->last_changes, (long) ch->pcdata->last_roomnote	);
	fprintf( fp, "Delete %ld\n", (long)ch->pcdata->delete_requested);
    fprintf( fp, "Scro %d\n", 	ch->lines		);
    fprintf( fp, "Room %d\n",
        ( /* ch->in_room == get_room_index( ROOM_VNUM_LIMBO ) 
        && */ ch->was_in_room != NULL )
            ? ch->was_in_room->vnum
            : ch->in_room == NULL ? 3001 : ch->in_room->vnum );

    fprintf( fp, "HMV  %d %d %d %d %d %d\n",
	ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move );

    fprintf(fp, "Coin %d", MAX_COIN);
    for (x = 0; x < MAX_COIN; x++)
        fprintf(fp, " %ld", ch->coins[x]);
    fprintf(fp, "\n");

    fprintf(fp, "Bank %d", MAX_COIN);
    for (x = 0; x < MAX_COIN; x++)
        fprintf(fp, " %ld", ch->bank[x]);
    fprintf(fp, "\n");
    fprintf(fp, "Bounty  %lu\n", ch->pcdata->bounty);

    if (BIT_GET(ch->pcdata->traits, TRAIT_RETAINER)
      && ch->pcdata->retainername && ch->pcdata->retainergender != 0)
    {
	fprintf(fp, "RetName %s~\n", ch->pcdata->retainername);
	fprintf(fp, "RetSex %d\n", ch->pcdata->retainergender);
    }

    if (ch->pcdata->familiarname != 0) fprintf(fp, "FamName %s~\n", ch->pcdata->familiarname);
    if (ch->pcdata->familiargender != 0) fprintf(fp, "FamSex %d\n", ch->pcdata->familiargender);

    fprintf( fp, "Exp  %d\n",	ch->exp			);
    fprintf( fp, "Ep   %d\n",	ch->ep			);
    fprintf( fp, "Awrd %d\n",	ch->pcdata->award_points);
    if (ch->recall_old)
	fprintf(fp, "Rcll %d\n", ch->recall_old->vnum   );
    else if (ch->recall_to)
	fprintf(fp, "Rcll %d\n", ch->recall_to->vnum	);
    if (ch->act != 0)
	fprintf( fp, "Act  %s\n",   print_flags(ch->act));
    if (ch->nact != 0)
	fprintf( fp, "NAct %s\n",  print_flags(ch->nact));
    if (ch->naffected_by != 0)
	fprintf( fp, "NAfB %s\n",   print_flags(ch->naffected_by));
    if (ch->oaffected_by != 0)
	fprintf( fp, "OAfB %s\n",   print_flags(ch->oaffected_by));
    if (ch->paffected_by != 0)
	fprintf( fp, "PAfB %s\n",   print_flags(ch->paffected_by));
    if (ch->affected_by != 0)
	fprintf( fp, "AfBy %s\n",   print_flags(ch->affected_by));
    fprintf( fp, "Comm %s\n",       print_flags(ch->comm));
    if (ch->extra_flags != 0)
	fprintf( fp, "Xtra %s\n",   print_flags(ch->extra_flags));
    if (ch->wiznet)
    	fprintf( fp, "Wizn %s\n",   print_flags(ch->wiznet));
    if (ch->pcdata->pcdata_flags)
	fprintf( fp, "Pcd  %s\n",   print_flags(ch->pcdata->pcdata_flags));
    if (ch->symbols_known)
	fprintf( fp, "Symb %s\n",   print_flags(ch->symbols_known));

    for (x = 0; trait_table[x].name; x++)
	if (BIT_GET(ch->pcdata->traits, trait_table[x].bit))
	    fprintf( fp, "Trait %s~\n", fwrite_format(trait_table[x].name));

    if (ch->invis_level)
	fprintf( fp, "Invi %d\n", 	ch->invis_level	);
    if (ch->incog_level)
	fprintf(fp,"Inco %d\n",ch->incog_level);
    fprintf( fp, "Pos  %d\n",	
	ch->position == POS_FIGHTING ? POS_STANDING : ch->position );
    if (ch->practice != 0)
    	fprintf( fp, "Prac %d\n",	ch->practice	);
    if (ch->train != 0)
	fprintf( fp, "Trai %d\n",	ch->train	);
    if (ch->pcdata->hp_trains > 0)
	fprintf( fp, "TrainHP %d\n",	ch->pcdata->hp_trains);
    if (ch->pcdata->mana_trains > 0)
	fprintf( fp, "TrainMN %d\n",	ch->pcdata->mana_trains);
    if (ch->saving_throw != 0) fprintf( fp, "Save  %d\n",	ch->saving_throw);
    if (ch->luck != 0) fprintf( fp, "Luck  %d\n",	ch->luck);
    fprintf( fp, "Alig  %d\n",	ch->alignment		);
    fprintf( fp, "Taint %d\n",  ch->pcdata->karma	);
    fprintf( fp, "Request %d\n", ch->pcdata->request_points);
    if (ch->hitroll != 0)
	fprintf( fp, "Hit   %d\n",	ch->hitroll	);
    if (ch->damroll != 0)
	fprintf( fp, "Dam   %d\n",	ch->damroll	);
    fprintf( fp, "ACs %d %d %d %d\n",	
	ch->armor[0],ch->armor[1],ch->armor[2],ch->armor[3]);
    if (ch->wimpy !=0 )
	fprintf( fp, "Wimp  %d\n",	ch->wimpy	);
    fprintf( fp, "Attr %d %d %d %d %d %d\n",
	ch->perm_stat[STAT_STR],
	ch->perm_stat[STAT_INT],
	ch->perm_stat[STAT_WIS],
	ch->perm_stat[STAT_DEX],
	ch->perm_stat[STAT_CON],
	ch->perm_stat[STAT_CHR] );
    
    fprintf(fp, "APrime %d %d %d %d %d %d\n",
        ch->attr_prime[STAT_STR],
	ch->attr_prime[STAT_INT],
	ch->attr_prime[STAT_WIS],
	ch->attr_prime[STAT_DEX],
	ch->attr_prime[STAT_CON],
	ch->attr_prime[STAT_CHR] );
    
    fprintf (fp, "AMod %d %d %d %d %d %d\n",
	ch->mod_stat[STAT_STR],
	ch->mod_stat[STAT_INT],
	ch->mod_stat[STAT_WIS],
	ch->mod_stat[STAT_DEX],
	ch->mod_stat[STAT_CON],
	ch->mod_stat[STAT_CHR] );

    fprintf (fp, "AMax %d %d %d %d %d %d\n",
	ch->max_stat[STAT_STR],
	ch->max_stat[STAT_INT],
	ch->max_stat[STAT_WIS],
	ch->max_stat[STAT_DEX],
	ch->max_stat[STAT_CON],
	ch->max_stat[STAT_CHR] );
 
    /** Erinos: Added for bows and arrows **/
  
    fprintf( fp, "FTyp %d\n",	ch->familiar_type	);

    fprintf( fp, "Ltsn %d\n",	ch->lastsn		);

    if ( IS_NPC(ch) )
    {
	fprintf( fp, "Vnum %d\n",	ch->pIndexData->vnum	);
    }
    else
    {
	fprintf( fp, "Pass %s~\n",	fwrite_format(ch->pcdata->pwd)	);
	if (ch->pcdata->bamfin[0] != '\0')
	    fprintf( fp, "Bin  %s~\n",	ch->pcdata->bamfin);
	if (ch->pcdata->bamfout[0] != '\0')
		fprintf( fp, "Bout %s~\n",	ch->pcdata->bamfout);
	fprintf( fp, "Titl %s~\n",	fwrite_format(ch->pcdata->title));
        fprintf( fp, "EXTitl %s~\n",    fwrite_format(ch->pcdata->extitle));
	fprintf( fp, "PreT %s~\n", 	fwrite_format(ch->pcdata->pretitle));
	fprintf( fp, "SurN %s~\n",	fwrite_format(ch->pcdata->surname));
	if (ch->religion >= 0)
	    fprintf( fp, "Reli %s~\n",	fwrite_format(god_table[ch->religion].name));
    	fprintf( fp, "Pnts %d\n",   	ch->pcdata->points      );
	fprintf( fp, "TSex %d\n",	ch->pcdata->true_sex	);
	fprintf( fp, "LLev %d\n",	ch->pcdata->last_level	);
	fprintf( fp, "HMVP %d %d %d\n", ch->pcdata->perm_hit, 
						   ch->pcdata->perm_mana,
						   ch->pcdata->perm_move);
	fprintf( fp, "Cnd  %d %d %d %d\n",
	    ch->pcdata->condition[0],
	    ch->pcdata->condition[1],
	    ch->pcdata->condition[2],
	    ch->pcdata->condition[3] );

	fprintf( fp, "FQua %d\n",	ch->pcdata->food_quality);

	/* write sacrifice history */
	for (x=0;x<MAX_GODS;x++)
	    if (ch->pcdata->sacrifices[x].level > 0 
	      || ch->pcdata->sacrifices[x].number > 0)
		fprintf(fp,"Sac %s %i %d %d\n",god_table[x].name,x, 
		  ch->pcdata->sacrifices[x].level, 
		  ch->pcdata->sacrifices[x].number);

	/* write alias */
        for (pos = 0; pos < MAX_ALIAS; pos++)
	{
	    if (ch->pcdata->alias[pos] == NULL
	    ||  ch->pcdata->alias_sub[pos] == NULL)
		break;

	    fprintf(fp,"Alias %s %s~\n",ch->pcdata->alias[pos],
		    fwrite_format(ch->pcdata->alias_sub[pos]));
	}

	for ( sn = 0; sn < MAX_SKILL; sn++ )
	{
	    if ( skill_table[sn].name != NULL && ch->pcdata->learned[sn] > 0 )
	    	fprintf( fp, "Sk %d %s~ %d\n", ch->pcdata->learned[sn], fwrite_format(skill_table[sn].name), 0);
	}

	for ( gn = 0; gn < MAX_GROUP; gn++ )
        {
            if ( group_table[gn].name != NULL && ch->pcdata->group_known[gn])
            {
                fprintf( fp, "Gr '%s'\n",group_table[gn].name);
            }
        }
    }

    if (ch->pcdata->somatic_arts_info != NULL)
        fprintf(fp, "Som %s~\n", ch->pcdata->somatic_arts_info->Serialize().c_str());

    if (ch->pcdata->phantasmInfo != NULL)
        fprintf(fp, "Phan %s~\n", ch->pcdata->phantasmInfo->serialize().c_str());

    haf.level=0;
    if ((paf=affect_find(ch->affected,gsn_hooded)) != NULL)
    {
	haf.type = paf->type;
	haf.valid = paf->valid;
	haf.where = paf->where;
	haf.level = paf->level;
	haf.point = paf->point;
	haf.duration = paf->duration;
	haf.location = paf->location;
	haf.modifier = paf->modifier;
	haf.bitvector = paf->bitvector;
	affect_remove(ch,paf);
    }
    paf = NULL;
    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
    	if (paf->type < 0 || paf->type>= MAX_SKILL || paf->type == gsn_runeofspirit || paf->type == gsn_archery 
        || paf->type == gsn_inquire || paf->type == gsn_spiritbond || paf->type == gsn_bondofsouls || paf->type == gsn_manaconduit 
        || paf->type == gsn_brotherhood || paf->type == gsn_coven || paf->type == gsn_submissionhold || paf->type == gsn_cloak 
        || paf->type == gsn_delayreaction || paf->type == gsn_ready || paf->type == gsn_glaciersedge)
	        continue;

        if (paf->type == gsn_stonecraft && paf->location != APPLY_NONE)
            continue;

    	fprintf( fp, "Affc '%s' %3d %3d %3d %3d %3d %10d%s%s\n",
	        fwrite_format(skill_table[paf->type].name),
    	    paf->where,
	        paf->level,
	        paf->duration,
    	    paf->modifier,
	        paf->location,
	        paf->bitvector,
    	    ((paf->type == gsn_generic || paf->type == gsn_sacrifice)
	          && paf->point) ? (char *) paf->point : "",
                (paf->type == gsn_generic || paf->type == gsn_sacrifice) ? "~" : ""
	    );
    }

    /* Data for swordmasters favored blade. */
    for ( x = 0; x < MAX_SWORDS; x++)
	if (ch->pcdata->favored_vnum[x] > 0)
	    fprintf(fp, "Favv %d %d\n", ch->pcdata->favored_vnum[x], ch->pcdata->favored_bonus[x]);
	
    fprintf( fp, "End\n\n" );
    if (haf.level)
    {
        if (ch->orig_long[0] == '\0' && ch->long_descr[0] != '\0')
        {
            free_string(ch->orig_long);
            ch->orig_long = str_dup(ch->long_descr);
        }
        free_string(ch->long_descr);
        ch->long_descr = str_dup("A hooded figure stands here.\n\r");
        setFakeName(*ch, "hooded figure rogue");
        free_string(ch->short_descr);
        ch->short_descr = str_dup("a hooded rogue");
        if (ch->orig_description[0] == '\0')
        {
            free_string(ch->orig_description);
            ch->orig_description = str_dup(ch->description);
        }
        free_string(ch->description);
        sprintf(buf, "A hooded %s is here, its identity cunningly hidden.\n\r",
            race_table[ch->race].name);
        ch->description = str_dup(buf);

        sprintf(buf, "hooded%ld", ch->id);
        setUniqueName(*ch, buf);

	affect_to_char(ch, &haf);
    }
}

/* write a pet */
void fwrite_pet( CHAR_DATA *pet, FILE *fp)
{
    AFFECT_DATA *paf;
    
    if (IS_SET(pet->act, ACT_FAMILIAR))
    	fprintf(fp,"#FAMILIAR\n");
    else
    	fprintf(fp,"#PET\n");
    
    fprintf(fp,"Vnum %d\n",pet->pIndexData->vnum);
    
    fprintf(fp,"Name %s~\n", fwrite_format(pet->name));
    fprintf(fp,"LogO %ld\n", (long) current_time);
    if (pet->short_descr != pet->pIndexData->short_descr)
    	fprintf(fp,"ShD  %s~\n", fwrite_format(pet->short_descr));
    if (pet->long_descr != pet->pIndexData->long_descr)
    	fprintf(fp,"LnD  %s~\n", fwrite_format(pet->long_descr));
    if (pet->description != pet->pIndexData->description)
    	fprintf(fp,"Desc %s~\n", fwrite_format(pet->description));
    if (pet->race != pet->pIndexData->race)
    	fprintf(fp,"Race %s~\n", fwrite_format(race_table[pet->race].name));
    if (pet->clan)
        fprintf( fp, "Clan %s~\n", fwrite_format(clan_table[pet->clan].who_name));
    fprintf(fp,"Sex  %d\n", pet->sex);
    if (pet->level != pet->pIndexData->level)
    	fprintf(fp,"Levl %d\n", pet->level);
    fprintf(fp, "HMV  %d %d %d %d %d %d\n",
    	pet->hit, pet->max_hit, pet->mana, pet->max_mana, pet->move, pet->max_move);

    fprintf(fp, "Coin %d", MAX_COIN);
    for (int i = 0; i < MAX_COIN; i++)
        fprintf(fp, " %ld", pet->coins[i]);
    fprintf(fp, "\n");

    for (int i(0); i < MAX_RESIST; ++i)
    {
        if (pet->resist[i] != 0)
            fprintf(fp, "Resist %d %d\n", i, pet->resist[i]);
    }

    if (pet->dam_type != pet->pIndexData->dam_type)
        fprintf(fp, "DamType %d\n", pet->dam_type);
    
    if (strcmp(pet->dam_verb, pet->pIndexData->dam_verb) != 0)
        fprintf(fp, "DamVerb %s~\n", fwrite_format(pet->dam_verb));

    if (pet->ep > 0)
    	fprintf(fp, "Ep  %d\n", pet->exp);
    if (pet->exp > 0)
    	fprintf(fp, "Exp  %d\n", pet->exp);
    if (pet->act != pet->pIndexData->act)
    	fprintf(fp, "Act  %s\n", print_flags(pet->act));
    if (pet->affected_by != pet->pIndexData->affected_by)
    	fprintf(fp, "AfBy %s\n", print_flags(pet->affected_by));
    if (pet->comm != 0)
    	fprintf(fp, "Comm %s\n", print_flags(pet->comm));
    fprintf(fp,"Pos  %d\n", pet->position = POS_FIGHTING ? POS_STANDING : pet->position);
    if (pet->saving_throw != 0)	fprintf(fp, "Save %d\n", pet->saving_throw);
    if (pet->luck != 0) fprintf( fp, "Luck  %d\n",	pet->luck);
    fprintf(fp, "Alig %d\n", pet->alignment);
    if (pet->hitroll != pet->pIndexData->hitroll)
    	fprintf(fp, "Hit  %d\n", pet->hitroll);
    fprintf(fp, "Dam  %d %d %d\n", pet->damage[DICE_NUMBER],
				       pet->damage[DICE_TYPE],
				       pet->damroll);
    fprintf(fp, "ACs  %d %d %d %d\n",
    	pet->armor[0],pet->armor[1],pet->armor[2],pet->armor[3]);
    fprintf(fp, "Attr %d %d %d %d %d %d\n",
    	pet->perm_stat[STAT_STR], pet->perm_stat[STAT_INT],
    	pet->perm_stat[STAT_WIS], pet->perm_stat[STAT_DEX],
    	pet->perm_stat[STAT_CON], pet->perm_stat[STAT_CHR]);
    fprintf(fp, "AMod %d %d %d %d %d %d\n",
    	pet->mod_stat[STAT_STR], pet->mod_stat[STAT_INT],
    	pet->mod_stat[STAT_WIS], pet->mod_stat[STAT_DEX],
    	pet->mod_stat[STAT_CON], pet->mod_stat[STAT_CHR]);
    fprintf(fp, "AMax %d %d %d %d %d %d\n",
    	pet->max_stat[STAT_STR], pet->max_stat[STAT_INT],
    	pet->max_stat[STAT_WIS], pet->max_stat[STAT_DEX],
    	pet->max_stat[STAT_CON], pet->max_stat[STAT_CHR]);
   
   	fprintf( fp, "Mval %d %d %d %d %d %d %d %d %d %d\n", pet->mobvalue[0], pet->mobvalue[1],
		pet->mobvalue[2], pet->mobvalue[3], pet->mobvalue[4],
		pet->mobvalue[5], pet->mobvalue[6], pet->mobvalue[7],
		pet->mobvalue[8], pet->mobvalue[9]);

    for (int i = 0; i < MAX_MOBVALUE; ++i)
    {
        if (pet->stringvalue[i] != NULL)
            fprintf(fp, "Mstr %d %s~\n", i, pet->stringvalue[i]);
    }

	for (int i = 0; i < MAX_MOBFOCUS; i++)
	{
    	if (pet->memfocus[i] != NULL)
	    	fprintf( fp, "Mfoc %d %s~\n", i, pet->memfocus[i]->name);
	}

    for ( paf = pet->affected; paf != NULL; paf = paf->next )
    {
    	if (paf->type < 0 || paf->type >= MAX_SKILL)
    	    continue;
    	    
    	fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10d%s%s\n",
    	    skill_table[paf->type].name,
    	    paf->where, paf->level, paf->duration, paf->modifier,paf->location,
    	    paf->bitvector,
	    ((paf->type == gsn_generic || paf->type == gsn_sacrifice)
	      && paf->point) ? (char *) paf->point : "",
            (paf->type == gsn_generic || paf->type == gsn_sacrifice) ? "~" : "");
    }
    
    fprintf(fp,"End\n");
    return;
}
    
/*
 * Write an object and its contents.
 */
void fwrite_obj_list(CHAR_DATA *ch, OBJ_DATA *obj, FILE *fp, int iNest)
{
	// Build a list of the objects in reverse order so that they are
	// read in the correct order
	// TODO: write these in the correct order and -read- them in
	// the reverse order, since reading is more infrequent
	std::list<OBJ_DATA*> objList;
	for (OBJ_DATA * iter(obj); iter != NULL; iter = iter->next_content)
		objList.push_front(iter);

	// Now write the objects
	for (std::list<OBJ_DATA*>::const_iterator iter(objList.begin()); iter != objList.end(); ++iter)
		fwrite_obj(ch, *iter, fp, iNest);
}

void fwrite_obj(CHAR_DATA * ch, OBJ_DATA * obj, FILE * fp, int iNest)
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;
    int i;
    bool savepoint;

    /*
     * Castrate storage characters.
     **** This BS removed by Matt
     */
    if (obj->item_type == ITEM_KEY
      || IS_SET(obj->extra_flags[0], ITEM_QUEST)
      || (obj->item_type == ITEM_MAP && !obj->value[0])
      || (obj->pIndexData->vnum == OBJ_VNUM_STONE_POWER))
    {
        if (ch->desc && ch->desc->inlast[0] == 'q')
	     extract_obj(obj);
        return;
    }
// brazen: Separating this allows us to log limited EQ purging on quit for
// characters < 10. Since this function is called from save and quit, we only
// want to log these on quit.
// Also, added an extract_obj for these items, because not doing so creates
// a memory leak and prevents these items from repopulating until the next
// crash

    if ((obj->pIndexData->limit_factor > 0 && ch->level < 10))
    {
        if (ch->desc && ch->desc->inlast[0] == 'q')
	{
	    char buf[MAX_STRING_LENGTH];
            sprintf(buf,"PURGE: %s (level %d) quit with limited item %s [%d]",ch->name,ch->level,obj->short_descr,obj->pIndexData->vnum);
	    log_string(buf);
	    extract_obj(obj);
	}
	return;
    }

    fprintf( fp, "#O\n" );
    fprintf( fp, "Vnum %d\n",   obj->pIndexData->vnum        );
    if (!obj->pIndexData->new_format)
	fprintf( fp, "Oldstyle\n");
    if (obj->enchanted)
	fprintf( fp,"Enchanted\n");
    fprintf( fp, "Nest %d\n",	iNest	  	     );

    /* these data are only used if they do not match the defaults */

    if ( obj->name != obj->pIndexData->name)
    	fprintf( fp, "Name %s~\n",	fwrite_format(obj->name)     );
    if ( obj->short_descr != obj->pIndexData->short_descr)
        fprintf( fp, "ShD  %s~\n",	fwrite_format(obj->short_descr));
    if ( obj->description != obj->pIndexData->description)
        fprintf( fp, "Desc %s~\n",	fwrite_format(obj->description));
    if ( obj->lore != obj->pIndexData->lore)
	fprintf( fp, "Lore %s~\n", 	fwrite_format(obj->lore)     );
    if ( obj->obj_str != obj->pIndexData->obj_str)
	fprintf( fp, "OStr %s~\n",	fwrite_format(obj->obj_str)  );
    if ( obj->extra_flags[0] != obj->pIndexData->extra_flags[0])
        fprintf( fp, "ExF0 %ld\n",	obj->extra_flags[0]	     );
    if ( obj->extra_flags[1] != obj->pIndexData->extra_flags[1])
        fprintf( fp, "ExF1 %ld\n",	obj->extra_flags[1]	     );
    if ( obj->wear_flags != obj->pIndexData->wear_flags)
        fprintf( fp, "WeaF %d\n",	obj->wear_flags		     );
    if ( obj->item_type != obj->pIndexData->item_type)
        fprintf( fp, "Ityp %d\n",	obj->item_type		     );
    if ( obj->material != obj->pIndexData->material)
	fprintf( fp, "Mat %d\n",	obj->material		     );
    if ( obj->weight != obj->pIndexData->weight)
        fprintf( fp, "Wt   %d\n",	obj->weight		     );
    if ( obj->condition != obj->pIndexData->condition)
	fprintf( fp, "Cond %d\n",	obj->condition		     );

    /* variable data */

    fprintf( fp, "TStmp %d\n",	obj->timestamp		     );
    fprintf( fp, "Wear %d\n",   obj->wear_loc                );
    if (obj->level != obj->pIndexData->level)
        fprintf( fp, "Lev  %d\n",	obj->level		     );
    if (obj->timer != 0)
        fprintf( fp, "Time %d\n",	obj->timer	     );
    fprintf( fp, "Cost %d\n",	obj->cost		     );
    fprintf( fp, "WornOn %d\n", obj->worn_on		     );

    if (obj->lastowner[0][0] != '\0')
    {
	fprintf( fp, "LstO %d %s~", MAX_LASTOWNER, obj->lastowner[0]);
	for (i = 1; i < MAX_LASTOWNER; i++)
	   fprintf( fp, " %s~", obj->lastowner[i]);
	fprintf( fp, "\n" );
    }

    if (obj->value[0] != obj->pIndexData->value[0]
    ||  obj->value[1] != obj->pIndexData->value[1]
    ||  obj->value[2] != obj->pIndexData->value[2]
    ||  obj->value[3] != obj->pIndexData->value[3]
    ||  obj->value[4] != obj->pIndexData->value[4]) 
    	fprintf( fp, "Val  %d %d %d %d %d\n",
	    obj->value[0], obj->value[1], obj->value[2], obj->value[3],
	    obj->value[4]	     );
	fprintf( fp, "Oval %d %d %d %d %d %d %d %d %d %d\n", obj->objvalue[0], obj->objvalue[1],
		obj->objvalue[2], obj->objvalue[3], obj->objvalue[4],
		obj->objvalue[5], obj->objvalue[6], obj->objvalue[7],
		obj->objvalue[8], obj->objvalue[9]);

    for (i = 0; i < MAX_MOBVALUE; ++i)
    {
        if (obj->stringvalue[i] != NULL)
            fprintf(fp, "Ostring %d %s~\n", i, obj->stringvalue[i]);
    }

	for (i = 0; i < MAX_OBJFOCUS; i++)
	{
	if (obj->objfocus[i] != NULL)
		fprintf( fp, "Ofoc %d %s~\n", i, obj->objfocus[i]->name);
	}

    switch ( obj->item_type )
    {
    case ITEM_POTION:
    case ITEM_SCROLL:
    case ITEM_PILL:
    case ITEM_OIL:
	if ( obj->value[1] > 0 )
	{
	    fprintf( fp, "Spell 1 '%s'\n", 
		skill_table[obj->value[1]].name );
	}

	if ( obj->value[2] > 0 )
	{
	    fprintf( fp, "Spell 2 '%s'\n", 
		skill_table[obj->value[2]].name );
	}

	if ( obj->value[3] > 0 )
	{
	    fprintf( fp, "Spell 3 '%s'\n", 
		skill_table[obj->value[3]].name );
	}

	break;

    case ITEM_STAFF:
    case ITEM_WAND:
	if ( obj->value[3] > 0 )
	{
	    fprintf( fp, "Spell 3 '%s'\n", 
		skill_table[obj->value[3]].name );
	}

	break;
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if (paf->type == gsn_generic || paf->type == gsn_sacrifice
          || paf->type == gsn_firebrand || paf->type == gsn_defilement
          || paf->type == gsn_lightningbrand || paf->type == gsn_frostbrand 
          || paf->type == gsn_consecrate)
            savepoint = TRUE;
        else
            savepoint = FALSE;
       
        const char * skillName("");
        if (paf->type >= 0 && paf->type < MAX_SKILL)
            skillName = skill_table[paf->type].name;
             
        fprintf( fp, "Affc '%s' %3d %3d %3d %3d %3d %10d%s%s\n",
            skillName,
            paf->where,
            paf->level,
            paf->duration,
            paf->modifier,
            paf->location,
            paf->bitvector,
            (paf->point && savepoint) ? (char *) paf->point : "",
            savepoint ? "~" : ""
            );
    }

    for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
    {
        fprintf( fp, "ExDe %s~ ", fwrite_format(ed->keyword));
        fprintf( fp, "%s~\n", fwrite_format(ed->description));
    }

    fprintf( fp, "End\n\n" );

    if ( obj->contains != NULL )
        fwrite_obj_list( ch, obj->contains, fp, iNest + 1 );
}

void fwrite_obj_room( ROOM_INDEX_DATA *room, OBJ_DATA *obj, FILE *fp, int iNest )
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;

    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    if (obj->next_content != NULL )
	fwrite_obj_room( room, obj->next_content, fp, iNest );

    /*
     * Castrate storage characters.
     **** This BS removed by Matt
     */
    if (  obj->item_type == ITEM_KEY
    || IS_SET(obj->extra_flags[0], ITEM_QUEST)
    ||  (obj->item_type == ITEM_MAP && !obj->value[0]))
	return;

    fprintf( fp, "#O\n" );
    fprintf( fp, "Vnum %d\n",   obj->pIndexData->vnum        );
    if (!obj->pIndexData->new_format)
	fprintf( fp, "Oldstyle\n");
    if (obj->enchanted)
	fprintf( fp,"Enchanted\n");
    fprintf( fp, "Nest %d\n",	iNest	  	     );

    /* these data are only used if they do not match the defaults */

    if ( obj->name != obj->pIndexData->name)
    	fprintf( fp, "Name %s~\n",	fwrite_format(obj->name)     );
    if ( obj->short_descr != obj->pIndexData->short_descr)
        fprintf( fp, "ShD  %s~\n",	fwrite_format(obj->short_descr));
    if ( obj->description != obj->pIndexData->description)
        fprintf( fp, "Desc %s~\n",	fwrite_format(obj->description));
    if ( obj->lore != obj->pIndexData->lore)
	fprintf( fp, "Lore %s~\n",	fwrite_format(obj->lore)     );
    if ( obj->extra_flags[0] != obj->pIndexData->extra_flags[0])
        fprintf( fp, "ExF0 %ld\n",	obj->extra_flags[0]	     );
    if ( obj->extra_flags[1] != obj->pIndexData->extra_flags[1])
        fprintf( fp, "ExF1 %ld\n",	obj->extra_flags[1]	     );
    if ( obj->wear_flags != obj->pIndexData->wear_flags)
        fprintf( fp, "WeaF %d\n",	obj->wear_flags		     );
    if ( obj->item_type != obj->pIndexData->item_type)
        fprintf( fp, "Ityp %d\n",	obj->item_type		     );
    if ( obj->weight != obj->pIndexData->weight)
        fprintf( fp, "Wt   %d\n",	obj->weight		     );
    if ( obj->condition != obj->pIndexData->condition)
	fprintf( fp, "Cond %d\n",	obj->condition		     );

    /* variable data */

    fprintf( fp, "Wear %d\n",   obj->wear_loc                );
    if (obj->level != obj->pIndexData->level)
        fprintf( fp, "Lev  %d\n",	obj->level		     );
    if (obj->timer != 0)
        fprintf( fp, "Time %d\n",	obj->timer	     );
    fprintf( fp, "Cost %d\n",	obj->cost		     );
    if (obj->value[0] != obj->pIndexData->value[0]
    ||  obj->value[1] != obj->pIndexData->value[1]
    ||  obj->value[2] != obj->pIndexData->value[2]
    ||  obj->value[3] != obj->pIndexData->value[3]
    ||  obj->value[4] != obj->pIndexData->value[4]) 
    	fprintf( fp, "Val  %d %d %d %d %d\n",
	    obj->value[0], obj->value[1], obj->value[2], obj->value[3],
	    obj->value[4]	     );
	fprintf( fp, "Oval %d %d %d %d %d %d %d %d %d %d\n", obj->objvalue[0], obj->objvalue[1],
		obj->objvalue[2], obj->objvalue[3], obj->objvalue[4],
		obj->objvalue[5], obj->objvalue[6], obj->objvalue[7],
		obj->objvalue[8], obj->objvalue[9]);

    switch ( obj->item_type )
    {
    case ITEM_POTION:
    case ITEM_SCROLL:
    case ITEM_PILL:
    case ITEM_OIL:
	if ( obj->value[1] > 0 )
	{
	    fprintf( fp, "Spell 1 '%s'\n", 
		skill_table[obj->value[1]].name );
	}

	if ( obj->value[2] > 0 )
	{
	    fprintf( fp, "Spell 2 '%s'\n", 
		skill_table[obj->value[2]].name );
	}

	if ( obj->value[3] > 0 )
	{
	    fprintf( fp, "Spell 3 '%s'\n", 
		skill_table[obj->value[3]].name );
	}

	break;

    case ITEM_STAFF:
    case ITEM_WAND:
	if ( obj->value[3] > 0 )
	{
	    fprintf( fp, "Spell 3 '%s'\n", 
		skill_table[obj->value[3]].name );
	}

	break;
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
	if (paf->type < 0 || paf->type >= MAX_SKILL)
	    continue;
        fprintf( fp, "Affc '%s' %3d %3d %3d %3d %3d %10d%s%s\n",
            skill_table[paf->type].name,
            paf->where,
            paf->level,
            paf->duration,
            paf->modifier,
            paf->location,
            paf->bitvector,
	    ((paf->type == gsn_generic || paf->type == gsn_sacrifice)
	      && paf->point) ? (char *) paf->point : "",
            (paf->type == gsn_generic || paf->type == gsn_sacrifice) ? "~" : ""
            );
    }

    for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
    {
	fprintf( fp, "ExDe %s~ ", fwrite_format(ed->keyword));
	fprintf( fp, "%s~\n", fwrite_format(ed->description));
    }

    fprintf( fp, "End\n\n" );

    if ( obj->contains != NULL )
	fwrite_obj_room( room, obj->contains, fp, iNest + 1 );

    return;
}


/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj( char *name, CHAR_DATA *ch )
{
    char strsave[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf, *paf_next;
//    CHAR_DATA *ch, *evil;
    CHAR_DATA *evil;
    FILE *fp;
    bool found;
    int stat;
    int i, x;
    char buf[100];
 
//    ch = new_char();
    ch->pcdata = new_pcdata();

//    d->character			= ch;
//    ch->desc				= d;
    setName(*ch, name);
    sprintf(buf, "%s0", name);
    setUniqueName(*ch, buf);
    ch->race				= global_int_race_human;
    ch->fake_race			= -1;
    ch->act				= PLR_NOSUMMON;
    ch->nact				= PLR_NOTEIFY;
    ch->skilled				= FALSE;
    ch->comm				= COMM_COMBINE 
					| COMM_PROMPT;
/*    if ((d->acct) && (d->acct->def_prompt))
        ch->prompt			= str_dup( d->acct->def_prompt );
    else */
//    ch->prompt	 			= str_dup( "{c<%hhp %mm %vmv>{x " );
    ch->prompt				= str_dup( "" );
    ch->pcdata->confirm_delete		= FALSE;
    ch->pcdata->pwd			= str_dup( "" );
    ch->pcdata->bamfin			= str_dup( "" );
    ch->pcdata->bamfout			= str_dup( "" );
    ch->pcdata->title			= str_dup( "" );
    ch->pcdata->extitle                 = str_dup( "" );
    ch->pcdata->pretitle		= str_dup( "" );
    ch->pcdata->surname			= str_dup( "" );
    ch->pcdata->socket_info		= str_dup( "" );
    ch->pcdata->max_age			= 0;
    ch->pcdata->age_group		= AGE_NONE;
    ch->pcdata->age_mod			= 0;
    ch->pcdata->age_mod_perm		= 0;
    ch->pcdata->hp_trains		= 0;
    ch->pcdata->mana_trains		= 0;
    ch->pcdata->retainername		= NULL;
    ch->pcdata->retainergender		= 0;
    ch->pcdata->familiarname    = NULL;
    ch->pcdata->familiargender  = 0;
    ch->start_pos                       = POS_STANDING;
    ch->objlevels			= 0;

    for (x = 0;x < MAX_SONGBOOK;x++)
	ch->pcdata->songbook[x]		= NULL;
    ch->pcdata->performing		= NULL;

    for (x = 0;x < MAX_SWORDS;x++)
    {
      ch->pcdata->favored_vnum[x]		= 0;
      ch->pcdata->favored_bonus[x]		= 0;
    }
    for (x = 0; x < MAX_CLORE; x++)
    {
	ch->lcreature[x] = 0;
	ch->lpercent[x] = 0;
    }
    for (stat =0; stat < MAX_STATS; stat++)
	ch->perm_stat[stat]		= 13;
    ch->pcdata->condition[COND_THIRST]	= 48; 
    ch->pcdata->condition[COND_FULL]	= 48;
    ch->pcdata->condition[COND_HUNGER]	= 48;
    ch->pcdata->food_quality		= 100;
    ch->pcdata->security		= 0;	/* OLC */
    ch->pcdata->ethos			= 0;	/* ethos */
    ch->pcdata->major_sphere		= 0;	/* sphere for mages */
    ch->pcdata->minor_sphere		= 0;	/* and minor sphere */
    ch->pcdata->chosen_path         = PATH_NONE;
    ch->pcdata->karma			= 0;
    ch->pcdata->request_points  = 0;

    ch->pcdata->travelptr		= (unsigned char*)malloc(8192*sizeof(unsigned char));
    g_num_travelptr++;
    memset((void *) ch->pcdata->travelptr, 0, 8192); /* initialize to all false */

    ch->pcdata->bitptr			= (unsigned char*)malloc(8192*sizeof(unsigned char));
    g_num_bitptr++;
    memset((void *) ch->pcdata->bitptr, 0, 8192); /* initialize to all false */

    ch->demonstate			= 0;
    ch->demontrack			= NULL;
    ch->familiar_type			= 0;
    ch->memgroup			= NULL;
    ch->conloss				= FALSE;
    ch->lastsn				= 0;
//    ch->orig_description		= str_dup("");   // this is set in new_char()
    ch->pcdata->background		= NULL;
    ch->pcdata->record			= NULL;
    ch->pcdata->immrecord		= NULL;
    ch->pcdata->recycle_log		= str_dup("");
    ch->pcdata->award_points		= 0;

    ch->pcdata->gamesock = INVALID_SOCKET;

    loading_char = TRUE;
    found = FALSE;
    fclose( fpReserve );
    
    #if defined(unix)
    /* decompress if .gz file exists */
    sprintf( strsave, "%s%s%s", PLAYER_DIR, capitalize(name),".gz");
    if ( ( fp = fopen( strsave, "r" ) ) != NULL )
    {
	fclose(fp);
	sprintf(buf,"gzip -dfq %s",strsave);
	system(buf);
    }
    #endif

    sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( name ) );
    if ( ( fp = fopen( strsave, "r" ) ) != NULL )
    {
        int iNest;

        for ( iNest = 0; iNest < MAX_NEST; iNest++ )
            rgObjNest[iNest] = NULL;

        found = TRUE;
        for ( ; ; )
        {
            char letter;
            char *word;

            letter = fread_letter( fp );
            if ( letter == '*' )
            {
            fread_to_eol( fp );
            continue;
            }

            if ( letter != '#' )
            {
            bug( "Load_char_obj: # not found.", 0 );
            break;
            }

            word = fread_word( fp );
            if      ( !str_cmp( word, "PLAYER" ) ) fread_char ( ch, fp );
            else if ( !str_cmp( word, "OBJECT" ) ) fread_obj  ( ch, fp );
            else if ( !str_cmp( word, "O"      ) ) fread_obj  ( ch, fp );
            else if ( !str_cmp( word, "PET"    ) ) fread_pet  ( ch, fp );
            else if ( !str_cmp( word, "FAMILIAR" ) ) fread_pet (ch, fp);
            else if ( !str_cmp( word, "END"    ) ) break;
            else
            {
                bug( "Load_char_obj: bad section.", 0 );
                break;
            }
        }

        if (!IS_NPC(ch))
        {
            FILE *pc_fp;
            char fname[128];
            sprintf(fname, "%s%s", MEMPATH, ch->name);
      
            if ((pc_fp = fopen(fname, "r")) != NULL)
            {
                if ((fread(ch->pcdata->bitptr, 1, 8192, pc_fp)) < 8192)
                {
                    sprintf(fname, "Memory read failure on %s. Short count.", ch->name);
                    bug(fname, 0);
                }
                fclose(pc_fp);
            } 
            else
            {
                sprintf(fname, "load_char_obj: Failed to open for memory read: %s", ch->name);
                bug(fname, 0);
            }

            sprintf(fname, "%s%s", TRAVELPATH, ch->name);

            if ((pc_fp = fopen(fname, "r")) != NULL)
            {
                if ((fread(ch->pcdata->travelptr, 1, 8192, pc_fp)) < 8192)
                {
                    sprintf(fname, "Travel read failure on %s. Short count.", ch->name);
                    bug(fname, 0);
                }
                fclose(pc_fp);
            } 
            else
            {
                sprintf(fname, "load_char_obj: Failed to open for travel read: %s", ch->name);
                bug(fname, 0);
            }

            sprintf(fname, "%s%s.fac", FACTION_DIR, ch->name);
            std::ifstream input(fname);
            if (input.is_open())
            {
                std::string line;
                std::getline(input, line);
                ch->pcdata->faction_standing->Deserialize(line);
            }
            else
            {
                std::ostringstream mess;
                mess << "Failed to open faction file for input '" << fname << "'";
                bug(mess.str().c_str(), 0);
            }
        }

        fclose( fp );
    }

    fpReserve = fopen( NULL_FILE, "r" );
    loading_char = FALSE;


    /* initialize race */
    if (found)
    {
	long faff;

	if (ch->race == 0)
	    ch->race = global_int_race_human;

	ch->size = pc_race_table[ch->race].size;

	if (ch->race != global_int_race_kankoran)
	{
	    ch->dam_type = DAM_BASH; /*punch */
	    ch->dam_verb = str_dup("punch");
	}
	else
	{
	    ch->dam_type = DAM_SLASH;
	    ch->dam_verb = str_dup("claw");
	}

	for (i = 0; i < 5; i++)
	{
	    if (pc_race_table[ch->race].skills[i] == NULL)
		break;
	    group_add(ch,pc_race_table[ch->race].skills[i],FALSE);
	}

	faff = race_table[ch->race].aff;
	REMOVE_BIT(faff, AFF_FLY_NATURAL);

	ch->affected_by |= faff;
	ch->imm_flags	|= race_table[ch->race].imm;
	ch->res_flags	|= race_table[ch->race].res;
	ch->vuln_flags	|= race_table[ch->race].vuln;
	ch->form	= race_table[ch->race].form;
	ch->parts	= race_table[ch->race].parts;
    }

    for (i = 0; i < MAX_RESIST; i++)
	ch->resist[i] = race_table[ch->race].resist[i];

    /* RT initialize skills */

    if (found && ch->version < 2)  /* need to add the new skills */
    {
	group_add(ch,"rom basics",FALSE);
	group_add(ch,class_table[ch->class_num].base_group,FALSE);
	group_add(ch,class_table[ch->class_num].default_group,TRUE);
	ch->pcdata->learned[gsn_recall] = 50;
    }
 
    /* fix levels */
    if (found && ch->version < 3 && (ch->level > 35 || ch->trust > 35))
    {
	switch (ch->level)
	{
	    case(40) : ch->level = 60;	break;  /* imp -> imp */
	    case(39) : ch->level = 58; 	break;	/* god -> supreme */
	    case(38) : ch->level = 56;  break;	/* deity -> god */
	    case(37) : ch->level = 53;  break;	/* angel -> demigod */
	}

        switch (ch->trust)
        {
            case(40) : ch->trust = 60;  break;	/* imp -> imp */
            case(39) : ch->trust = 58;  break;	/* god -> supreme */
            case(38) : ch->trust = 56;  break;	/* deity -> god */
            case(37) : ch->trust = 53;  break;	/* angel -> demigod */
            case(36) : ch->trust = 51;  break;	/* hero -> hero */
        }
    }

    /* Hometown fix */
    if (found && ch->version < 6)
    {
/*	if (ch->clan && (clan_table[ch->clan].altar > 0))
	    ch->recall_to = get_room_index(clan_table[ch->clan].altar); */

	if (!ch->recall_to)
	    ch->recall_to = get_default_hometown(ch);
    }

    if (found && ch->version < 7)
	ch->pcdata->max_age = 0;

    if (found && ch->version < 8)
    {
	ch->pcdata->learned[*lang_data[race_table[ch->race].native_tongue].sn] = 100;
	if (ch->race == global_int_race_human)
	    ch->train++;
	else
	    ch->pcdata->learned[*lang_data[LANG_COMMON].sn] = 100;

	if (ch->class_num < 12)
	    ch->pcdata->learned[*lang_data[LANG_ARCANE].sn] = 100;
    }

    if (found && ch->version < 11)
	ch->pcdata->death_count = 20 - ch->pcdata->death_count;

    if (found && ch->version < 12)
	SET_BIT(ch->nact, PLR_NOTEIFY);

    if (found && ch->version < 13)
    {
	OBJ_DATA *obj;

	for (obj = ch->carrying; obj; obj = obj->next_content)
	    if (!obj->worn_on && (obj->wear_loc != -1))
	    {
		obj->worn_on = (1 << obj->wear_loc);
		obj->wear_loc = -1;
	    }
    }

    if (found && ch->version < 15)
	SET_BIT(ch->act, PLR_AUTODATE);

    if (found && ch->version < 17)
    {
	if (coins_to_value(ch->coins) > (ch->level * 50))
	    memcpy(ch->coins, value_to_coins((float) ch->level * 50, FALSE), sizeof(long) * MAX_COIN);
	else
	    memcpy(ch->coins, convert_coins(ch->coins), sizeof(long) * MAX_COIN);

	if (coins_to_value(ch->bank) > (ch->level * 50))
	    memcpy(ch->bank, value_to_coins((float) ch->level * 50, FALSE), sizeof(long) * MAX_COIN);
	else
	    memcpy(ch->bank, convert_coins(ch->bank), sizeof(long) * MAX_COIN);
    }

    if (found && ch->version < 18)
    {
	OBJ_DATA *obj;

	for (obj = ch->carrying; obj; obj = obj->next_content)
	    obj->cost = obj->pIndexData->cost;
    }

    if (found && ch->version < 19)
    {
	ch->pcdata->condition[COND_HUNGER] = pc_race_table[ch->race].hunger_max;
	ch->pcdata->food_quality = 100;
    }

    if (found && ch->version < 20)   // Big stat update
    {
	ch->perm_stat[STAT_STR] = pc_race_table[ch->race].stats[STAT_STR];
	ch->perm_stat[STAT_INT] = pc_race_table[ch->race].stats[STAT_INT];
	ch->perm_stat[STAT_WIS] = pc_race_table[ch->race].stats[STAT_WIS];
	ch->perm_stat[STAT_DEX] = pc_race_table[ch->race].stats[STAT_DEX];
	ch->perm_stat[STAT_CON] = pc_race_table[ch->race].stats[STAT_CON];
	ch->perm_stat[STAT_CHR] = pc_race_table[ch->race].stats[STAT_CHR];

	if (ch->race == global_int_race_human)
	    for (i = 0; i < MAX_STATS; i++)	
	        ch->perm_stat[i] += ch->attr_prime[i];

	ch->train += 17;
	ch->train += (ch->level / 5);

	ch->pcdata->max_deaths = number_range(111, 118) - pc_race_table[ch->race].deaths_mod;

	if (ch->pcdata->max_deaths <= (ch->pcdata->death_count + 5))
	    ch->pcdata->max_deaths = ch->pcdata->death_count + 5;
    }

    if (found && ch->version < 21)
    {
	ch->pcdata->perm_hit = 20;
	ch->pcdata->perm_mana = 100;

	for (x = 1; x < ch->level; x++)
	{
	    ch->pcdata->perm_hit += number_range(class_table[ch->class_num].hp_min, class_table[ch->class_num].hp_max);
	    ch->pcdata->perm_mana += UMAX(1, round((dice(2, 6) - 1) * class_table[ch->class_num].fMana / 100.0));
	}

	ch->max_hit = ch->pcdata->perm_hit + get_hp_bonus(ch);
	ch->max_mana = ch->pcdata->perm_mana + get_mana_bonus(ch);

	if (ch->level >= 25)
	    ch->max_hit += 10;

	if (ch->level >= 50)
	{
	    ch->max_hit += 10;
	    ch->max_mana += 10;
	}

	ch->hit = ch->max_hit;
	ch->mana = ch->max_mana;
    }

    if (found && ch->version < 22)
    {
	ch->pcdata->perm_move = 100;

	for (x = 1; x < ch->level; x++)
	    ch->pcdata->perm_move += number_range(2,3);

	ch->max_move = ch->pcdata->perm_move;

	ch->move = ch->max_move;
    }

    if (found && ch->version < 23)
    {
	if (IS_SET(race_table[ch->race].aff, AFF_FLY_NATURAL))
	    REMOVE_BIT(ch->affected_by, AFF_FLYING);
    }

// brazen: Grandfather existing characters in by adding 6 trains, one for each
// starting stat point increase
    if (found && ch->version < 24)
        ch->train += 6;

	// Common now costs 3 practices; giving people who had it at 100% two trains,
	// and those with less or humans three practices
    if (found && ch->version < 25)
    {
	if (ch->race != global_int_race_human && ch->pcdata != NULL && ch->pcdata->learned[gsn_language_common] == 100)
	    ch->train += 2;
	else
	   ch->practice += 3;
    }
	
// brazen: request requires karma, so let's give it a boost here for existing
// characters
    if (found && ch->version < 26)
        if (ch->pcdata->karma < 0)
	    ch->pcdata->karma = URANGE(SILVERAURA, ch->pcdata->karma * 5, ch->pcdata->karma);

    if (found && ch->version < 27)
    {
	ch->train += UMAX(0,(ch->level - 10) / 10 - (ch->practice / 10));
	if (ch->class_num < global_int_class_watertemplar)
	    if (BIT_GET(ch->pcdata->traits, TRAIT_MAGAPT))
		ch->train += 1;
	    else
		BIT_SET(ch->pcdata->traits, TRAIT_MAGAPT);
    }

    if (found && ch->version < 28)
    {
	if (BIT_SET(ch->pcdata->traits, TRAIT_CRITICAL))
	    BIT_CLEAR(ch->pcdata->traits, TRAIT_CRITICAL);
	ch->train += 1;
    }

    if (found && ch->version < 29)
    {
	if (BIT_GET(ch->pcdata->traits, TRAIT_CHARMING))
	    ch->train += 1;
	if (get_skill(ch,gsn_peek))
	    SET_BIT(ch->nact,PLR_AUTOPEEK);
	if (get_skill(ch,gsn_trackless_step) || get_skill(ch,gsn_covertracks))
	    SET_BIT(ch->nact,PLR_AUTOTRACKS);
    }
    
    if (found && ch->version < 30)
    {
        ch->pcdata->perm_move = 100;
        int add_move=0;
        if (ch->race == global_int_race_chaja
          || ch->race == global_int_race_alatharya)
            add_move += (ch->level * 7) + ch->level/2;	
        else if (ch->race == global_int_race_nefortu)
            add_move += (ch->level * 6) + ch->level/2;
        else
            add_move += ch->level * 7;
        ch->pcdata->perm_move += add_move;
        ch->move = ch->max_move = ch->pcdata->perm_move;
        ch->coins[C_COPPER] += 50;
    }

    if (found && ch->version < 31)
    {
        // Add in 80 hp to adjust for new starting totals
        ch->max_hit += 80;
        ch->hit += 80;
    }
    
    /* remove psionic focuses */
    for (x = 0; focus_table[x].focus_fun; x++)
	affect_strip(ch, *focus_table[x].sn);	

    REMOVE_BIT(ch->oaffected_by, AFF_SYMBIONT);

    for (paf = ch->affected; paf; paf = paf_next)
    {
	paf_next = paf->next;

	if ((paf->type == gsn_findfamiliar) && !ch->familiar && paf->modifier > 0)
	    affect_remove(ch, paf);
	else if (paf->type == gsn_berserk)
	    add_event((void *) ch, 1, &event_berserk);
	else if (paf->type == gsn_setambush)
	    add_event((void *) ch, 1, &event_setambush);
	else if (paf->type == gsn_plague_madness)
	    add_event((void *) ch, 1, &event_plague_madness);
    }	

    if (is_affected(ch, gsn_wrathofthevoid))
    {
        evil = create_mobile(get_mob_index(MOB_VNUM_ASHUR_EVIL));
        evil->level = ch->level + 5;
        evil->max_hit = ch->level * 60;
        evil->hit     = evil->max_hit;
        evil->damroll = evil->level*2;
        evil->hitroll = evil->level*2;
        evil->damage[1] = evil->level/5;
        evil->damage[2] = evil->level/8;
        evil->armor[0] = 0 - evil->level;
        evil->armor[1] = 0 - evil->level;
        evil->armor[2] = 0 - evil->level;
        evil->armor[3] = 0 - evil->level;
        evil->memfocus[0] = ch;
	affect_strip(ch, gsn_wrathofthevoid);
        char_to_room(evil, get_room_index(83));
    }

    affect_strip(ch, gsn_createhomonculus);
    affect_strip(ch, gsn_inquire);
    affect_strip(ch, gsn_runeofspirit);
    affect_strip(ch, gsn_archery);
    affect_strip(ch, gsn_spiritbond);
    affect_strip(ch, gsn_bondofsouls);
    affect_strip(ch, gsn_glaciersedge);
    affect_strip(ch, gsn_englamour);

    if ((paf = get_affect(ch, gsn_phantasmalmirror)) != NULL)
    {
        paf->modifier = 0;
        paf->point = NULL;
    }

    if ((paf = get_affect(ch, gsn_bloodpyre)) != NULL)
    {
        // If you've been logged off for two hours, just strip the bloodpyre effect so you can cast it again
        if (ch->laston > 0 && current_time - ch->laston >= 2 * NUM_SECONDS_HOUR)
            affect_strip(ch, gsn_bloodpyre);
        else
        {
            paf->valid = false;
            delete static_cast<PyreInfo*>(paf->point);
            paf->point = NULL;
        }
    }

    // Just in case, make sure to clear out the bilocation effect (but leave the raw effect for cooldown purposes)
    if ((paf = get_affect(ch, gsn_bilocation)) != NULL)
    {
        paf->modifier = -1;
        paf->point = NULL;
    }

    if ((paf = get_affect(ch, gsn_finditems)) != NULL)
    {
	    paf->point = NULL;
	    affect_strip(ch, gsn_finditems);
    }

    // Handle stripping of appropriate object affects (this might need to be made recursive)
    for (OBJ_DATA * obj(ch->carrying); obj != NULL; obj = obj->next_content)
        object_affect_strip(obj, gsn_tuningstone);

    free_string(ch->pose);
    
    if (ch->level > 40 & !IS_SET(ch->act, PLR_SLOG))
        SET_BIT(ch->act, PLR_SLOG);

    return found;

}

/*
 * Read in a char.
 */
void fread_char( CHAR_DATA *ch, FILE *fp )
{
    char buf[MAX_STRING_LENGTH];
    const char *word(0);
    bool fMatch;
    int count = 0;
    int percent;
    int x, i;

    for ( ; ; )
    {
	word   = feof( fp ) ? "End" : fread_word( fp );
	fMatch = FALSE;

	switch ( UPPER(word[0]) )
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol( fp );
	    break;

	case 'A':
	    KEY( "Act",		ch->act,		fread_flag( fp ) );
	    KEY( "AgeG",	ch->pcdata->age_group,	fread_number( fp ) );
	    KEY( "AgeM",	ch->pcdata->max_age,	fread_number( fp ) );
	    KEY( "AgeP",	ch->pcdata->age_mod_perm,fread_number(fp ) );
	    KEY( "AgeL",	ch->pcdata->last_age,	fread_number( fp ) );
	    KEY( "AffectedBy",	ch->affected_by,	fread_flag( fp ) );
	    KEY( "AfBy",	ch->affected_by,	fread_flag( fp ) );
	    KEY( "Alignment",	ch->alignment,		fread_number( fp ) );
	    KEY( "Alig",	ch->alignment,		fread_number( fp ) );
	    KEY( "Awrd",	ch->pcdata->award_points,fread_number(fp ) );

	    if (!str_cmp( word, "Alia"))
	    {
		if (count >= MAX_ALIAS)
		{
		    fread_to_eol(fp);
		    fMatch = TRUE;
		    break;
		}

		ch->pcdata->alias[count] 	= str_dup(fread_word(fp));
		ch->pcdata->alias_sub[count]	= str_dup(fread_word(fp));
		count++;
		fMatch = TRUE;
		break;
	    }

            if (!str_cmp( word, "Alias"))
            {
                if (count >= MAX_ALIAS)
                {
                    fread_to_eol(fp);
                    fMatch = TRUE;
                    break;
                }
 
                ch->pcdata->alias[count]        = str_dup(fread_word(fp));
                ch->pcdata->alias_sub[count]    = fread_string(fp);
                count++;
                fMatch = TRUE;
                break;
            }

	    if (!str_cmp( word, "AC") || !str_cmp(word,"Armor"))
	    {
		fread_to_eol(fp);
		fMatch = TRUE;
		break;
	    }

	    if (!str_cmp(word,"ACs"))
	    {
		int i;

		for (i = 0; i < 4; i++)
		    ch->armor[i] = fread_number(fp);
		fMatch = TRUE;
		break;
	    }

	    if (!str_cmp(word, "AKills"))
	    {
		int i;
		
		for (i = 0; i < 3; i++)
		    ch->pcdata->align_kills[i] = fread_number(fp);
		fMatch = TRUE;
		break;
	    }

            if (!str_cmp(word,"Arws"))
            {
	        int i,atemp;
		OBJ_DATA *arrow;
	
	        for (i = 0; i < ARROW_TYPES; i++)
		    ch->arrows[i] = fread_number(fp);
	        for (i = 0; i < ARROW_TYPES; i++)
		{
		    for (atemp = 0;atemp < ch->arrows[i];atemp++)
		    {
			arrow=create_object(get_obj_index(OBJ_VNUM_FIRST_ARROW+i),0);
			obj_to_char(arrow,ch);
		    }
		    ch->arrows[i] = 0;
		}
	        fMatch = TRUE;
	        break;
            }

	    if (!str_cmp(word, "AffD"))
	    {
		AFFECT_DATA *paf;
		int sn;

		paf = new_affect();

		sn = skill_lookup_full(fread_word(fp));
		if (sn < 0)
		    bug("Fread_char: unknown skill.",0);
		else
		    paf->type = sn;

		paf->level	= fread_number( fp );
		paf->duration	= fread_number( fp );
		paf->modifier	= fread_number( fp );
		paf->location	= fread_number( fp );
		paf->bitvector	= fread_number( fp );
		paf->next	= ch->affected;
		ch->affected	= paf;
		fMatch = TRUE;
		break;
	    }

            if (!str_cmp(word, "Affc"))
            {
                AFFECT_DATA *paf;
                int sn;
 
                paf = new_affect();
 
		sn = skill_lookup_full(fread_word(fp));
                if (sn < 0)
                    bug("Fread_char: unknown skill.",0);
                else
                    paf->type = sn;
 
                paf->where  = fread_number(fp);
                paf->level      = fread_number( fp );
                paf->duration   = fread_number( fp );
                paf->modifier   = fread_number( fp );
                paf->location   = fread_number( fp );
                paf->bitvector  = fread_number( fp );

		if (paf->type == gsn_generic || paf->type == gsn_sacrifice)
		    paf->point = (void *) fread_string(fp);

                paf->next       = ch->affected;
                ch->affected    = paf;

                fMatch = TRUE;
                break;
            }

	    if ( !str_cmp(word,"AMax"))
	    {
		int stat;
		for (stat = 0; stat < MAX_STATS; stat ++)
		   ch->max_stat[stat] = fread_number(fp);
		fMatch = TRUE;
		break;
	    }

	    if ( !str_cmp( word, "AttrMod"  ) || !str_cmp(word,"AMod"))
	    {
		int stat;
		for (stat = 0; stat < MAX_STATS; stat ++)
		   ch->mod_stat[stat] = fread_number(fp);
		fMatch = TRUE;
		break;
	    }

	    if ( !str_cmp( word, "AttrPerm" ) || !str_cmp(word,"Attr"))
	    {
		int stat;

		for (stat = 0; stat < MAX_STATS; stat++)
		    ch->perm_stat[stat] = fread_number(fp);
		fMatch = TRUE;
		break;
	    }

	    if ( !str_cmp( word, "AttrPrime" ) || !str_cmp(word,"APrime"))
	    {
		int stat;
                for (stat = 0; stat < MAX_STATS; stat++) {
		    ch->attr_prime[stat] = fread_number(fp);
		}    
		fMatch = TRUE;
		break;
	    }				    
	    break;

	case 'B':
	    KEY( "Back",	ch->pcdata->background, fread_string( fp ) );
	    KEY( "Bamfin",	ch->pcdata->bamfin,	fread_string( fp ) );
	    KEY( "Bamfout",	ch->pcdata->bamfout,	fread_string( fp ) );
	    KEY( "Bin",		ch->pcdata->bamfin,	fread_string( fp ) );
	    KEY( "Bout",	ch->pcdata->bamfout,	fread_string( fp ) );
	    KEY( "Bcry",	ch->battlecry,		fread_string( fp ) );
	    KEY( "Bounty",	ch->pcdata->bounty,	fread_number( fp ) );

	    if (ch->version <= 16)
	    {
		KEY( "Bank",	ch->bank[0],		fread_number( fp ) );
	    }
	    else if (!str_cmp(word, "Bank"))
	    {
		i = fread_number(fp);
		
		for (x = 0; x < i; x++)
		{
		    if (x >= MAX_COIN)
		    {
			fread_to_eol(fp);
			break;
		    }
		    else
		        ch->bank[x] = fread_number(fp);
		}
		fMatch = TRUE;
		break;
	    }
		
	    break;

	case 'C':
        KEY( "ChosenPath", ch->pcdata->chosen_path, fread_number(fp));
	    KEY( "Class",	ch->class_num,		fread_number( fp ) );
	    KEY( "Cla",		ch->class_num,		fread_number( fp ) );
	    KEY( "Clan",	ch->clan,	clan_lookup(fread_string(fp)));

	    if ( !str_cmp( word, "CDat"	) )  fread_to_eol(fp);

	    if ( !str_cmp( word, "Condition" ) || !str_cmp(word,"Cond"))
	    {
		ch->pcdata->condition[0] = fread_number( fp );
		ch->pcdata->condition[1] = fread_number( fp );
		ch->pcdata->condition[2] = fread_number( fp );
		fMatch = TRUE;
		break;
	    }
            if (!str_cmp(word,"Cnd"))
            {
                ch->pcdata->condition[0] = fread_number( fp );
                ch->pcdata->condition[1] = fread_number( fp );
                ch->pcdata->condition[2] = fread_number( fp );
		ch->pcdata->condition[3] = fread_number( fp );
                fMatch = TRUE;
                break;
            }

	    if (!str_cmp(word, "Coin"))
	    {
		i = fread_number(fp);
		
		for (x = 0; x < i; x++)
		{
		    if (x >= MAX_COIN)
		    {
			fread_to_eol(fp);
			break;
		    }
		    else
		        ch->coins[x] = fread_number(fp);
		}
		fMatch = TRUE;
		break;
	    }

	    KEY("Comm",		ch->comm,		fread_flag( fp ) ); 
          
	    break;

	case 'D':
	    KEY( "Damroll",	ch->damroll,		fread_number( fp ) );
	    KEY( "Dam",		ch->damroll,		fread_number( fp ) );
	    KEY( "Deaths", 	ch->pcdata->death_count,fread_number(fp));
	    KEY( "Delete", 	ch->pcdata->delete_requested,fread_number(fp));
	    KEY( "Description",	ch->description,	fread_string( fp ) );
	    KEY( "Desc",	ch->description,	fread_string( fp ) );
	    KEY( "Dailyxp",	ch->pcdata->dailyxp,	fread_number (fp));
	    break;

	case 'E':
	    if ( !str_cmp( word, "End" ) )
	    {
    		/* adjust hp mana move up  -- here for speed's sake */
    		percent = (current_time - ch->laston) * 25 / ( 2 * 60 * 60);

		percent = UMIN(percent,100);

		if (ch->familiar_type == 0)
		{
			char bugbuf[1024];
			sprintf(bugbuf, "%s had a 0 familiar at level %d. Reset it.",
			ch->name, ch->level);
			bug(bugbuf, 0);
			ch->familiar_type = number_range(50, 59);
		}

    		if (percent > 0 && !IS_AFFECTED(ch,AFF_POISON)
    		&&  !IS_AFFECTED(ch,AFF_PLAGUE))
    		{
        	    ch->hit	+= (ch->max_hit - ch->hit) * percent / 100;
        	    ch->mana    += (ch->max_mana - ch->mana) * percent / 100;
        	    ch->move    += (ch->max_move - ch->move)* percent / 100;
    		}
		return;

	    }
	    KEY( "Exp",		ch->exp,		fread_number( fp ) );
	    KEY( "Ep",		ch->ep,		fread_number( fp ) );
	    KEY( "Ethos",	ch->pcdata->ethos,	fread_number( fp ));
            if ( !str_cmp( word, "EXTitle" )  || !str_cmp( word, "EXTitl"))
            {
                ch->pcdata->extitle = fread_string( fp );
                if (ch->pcdata->extitle[0] != '.' && ch->pcdata->extitle[0] != ','
                &&  ch->pcdata->extitle[0] != '!' && ch->pcdata->extitle[0] != '?')
                {
                    sprintf( buf, "%s", ch->pcdata->extitle );
                    free_string( ch->pcdata->extitle );
                    ch->pcdata->extitle = str_dup( buf );
                }
                fMatch = TRUE;
                break;
            }

	    if (!str_cmp(word, "ExDesc"))
	    {
		EXTRA_DESCR_DATA *ed = new_extra_descr();

		ed->next = ch->pcdata->extra_descr;
		ch->pcdata->extra_descr = ed;

		if (ch->version >= 14)
		    ed->can_edit = fread_number(fp);

		ed->keyword = fread_string(fp);
		ed->description = fread_string(fp);
		fMatch = TRUE;
	    }

	    break;

	case 'F':
	    KEY( "FTyp", 	ch->familiar_type,		fread_number(fp) );
	    KEY( "FQua",	ch->pcdata->food_quality,	fread_number(fp) );
	    KEY( "FRace",       ch->fake_race,	 race_lookup(fread_string( fp )) );
	    KEY( "FamName",	ch->pcdata->familiarname, fread_string(fp));
	    KEY( "FamSex",	ch->pcdata->familiargender, fread_number(fp));

	    if (!strcmp(word, "Favv" ))
	    {
	    fMatch = TRUE;
	      for (x = 0;x < MAX_SWORDS;x++)
	      {
	        if (ch->pcdata->favored_vnum[x] == 0)
		{
		  ch->pcdata->favored_vnum[x] = fread_number(fp);
		  ch->pcdata->favored_bonus[x] = fread_number(fp);
		  break;
		}
	      }
	    }

	    break;

	case 'G':
	    KEY( "Gold",	ch->coins[0],		fread_number( fp ) );
            if ( !str_cmp( word, "Group" )  || !str_cmp(word,"Gr"))
            {
                int gn;
                char *temp;
 
                temp = fread_word( fp ) ;
                gn = group_lookup(temp);
                /* gn    = group_lookup( fread_word( fp ) ); */
                if ( gn < 0 )
                {
                    fprintf(stderr,"%s",temp);
                    bug( "Fread_char: unknown group. ", 0 );
                }
                else
		    gn_add(ch,gn);
                fMatch = TRUE;
            }
	    break;

	case 'H':
	    KEY( "Hitroll",	ch->hitroll,		fread_number( fp ) );
	    KEY( "Hit",		ch->hitroll,		fread_number( fp ) );
	    KEY( "Hour",	ch->lasthour,		fread_number( fp ) );

	    if ( !str_cmp( word, "HpManaMove" ) || !str_cmp(word,"HMV"))
	    {
		ch->hit		= fread_number( fp );
		ch->max_hit	= fread_number( fp );
		ch->mana	= fread_number( fp );
		ch->max_mana	= fread_number( fp );
		ch->move	= fread_number( fp );
		ch->max_move	= fread_number( fp );
		fMatch = TRUE;
		break;
	    }

            if ( !str_cmp( word, "HpManaMovePerm" ) || !str_cmp(word,"HMVP"))
            {
                ch->pcdata->perm_hit	= fread_number( fp );
                ch->pcdata->perm_mana   = fread_number( fp );
                ch->pcdata->perm_move   = fread_number( fp );
                fMatch = TRUE;
                break;
            }
      
	    break;

	case 'I':
	    KEY( "IRec",	ch->pcdata->immrecord,	fread_string(fp));
	    KEY( "InvisLevel",	ch->invis_level,	fread_number( fp ) );
	    KEY( "Inco",	ch->incog_level,	fread_number( fp ) );
	    KEY( "Invi",	ch->invis_level,	fread_number( fp ) );
	    if (!str_cmp(word, "Id"))
	    {
	        ch->id = fread_number(fp);
            if (ch->id == 0)
	            ch->id = get_pc_id();
            fMatch = TRUE;
            break;
        }

	    break;

	case 'L':
	    KEY( "LastLevel",	ch->pcdata->last_level, fread_number( fp ) );
	    KEY( "LLev",	ch->pcdata->last_level, fread_number( fp ) );
	    KEY( "Level",	ch->level,		fread_number( fp ) );
	    KEY( "Lev",		ch->level,		fread_number( fp ) );
	    KEY( "Levl",	ch->level,		fread_number( fp ) );
	    KEY( "LogO",	ch->laston,		fread_number( fp ) );
	    KEY( "LongDescr",	ch->long_descr,		fread_string( fp ) );
	    KEY( "LnD",		ch->long_descr,		fread_string( fp ) );
	    KEY( "Ltsn",	ch->lastsn,		fread_number( fp ) );
        KEY( "Luck",    ch->luck,       fread_number(fp));

	
	    if (!str_cmp(word, "LastOn"))
	    {
		fread_to_eol(fp);
		fMatch = TRUE;
		break;
	    }

	    if (!str_cmp(word, "LrnC"))
	    {
		int illn;
	
		illn = fread_number(fp);
		ch->lcreature[illn] = fread_number(fp);
		ch->lpercent[illn] = fread_number(fp);
		fMatch = TRUE;
                break;
	    }	
	    break;

	case 'M':
	    KEY( "Major", 	ch->pcdata->major_sphere,fread_number(fp));
	    KEY( "Minor",	ch->pcdata->minor_sphere,fread_number(fp));
	    KEY( "MxDths",	ch->pcdata->max_deaths, fread_number(fp));   
	    break;

	case 'N':
        if (!str_cmp(word, "Name"))
        {
            setName(*ch, fread_string(fp));
            fMatch = TRUE;
            break;
        }

	    KEY( "NAct",	ch->nact,		fread_flag( fp ) );
	    KEY( "NAfB",	ch->naffected_by,	fread_flag( fp ) );
	    KEY( "Note",	ch->pcdata->last_note,	fread_number( fp ) );
	    if (!str_cmp(word,"Not"))
	    {
		ch->pcdata->last_note			= fread_number(fp);
		ch->pcdata->last_idea			= fread_number(fp);
		ch->pcdata->last_penalty		= fread_number(fp);
		ch->pcdata->last_news			= fread_number(fp);
		ch->pcdata->last_changes		= fread_number(fp);

		if (ch->version >= 16)
		    ch->pcdata->last_roomnote		= fread_number(fp);

		fMatch = TRUE;
		break;
	    }
	    break;

	case 'O':
	    KEYS( "ODes",	ch->orig_description,	fread_string( fp ) );
	    KEY( "OAfB",	ch->oaffected_by,	fread_flag( fp ) );
	    KEYS( "OLng",	ch->orig_long,		fread_string( fp ) );
	break;


	case 'P':
	    KEY( "Password",	ch->pcdata->pwd,	fread_string( fp ) );
	    KEY( "Pass",	ch->pcdata->pwd,	fread_string( fp ) );
	    KEY( "PKills",	ch->pcdata->pkills,	fread_number( fp ) );
	    KEY( "Played",	ch->played,		fread_number( fp ) );
	    KEY( "Plyd",	ch->played,		fread_number( fp ) );
	    KEY( "Points",	ch->pcdata->points,	fread_number( fp ) );
	    KEY( "Pnts",	ch->pcdata->points,	fread_number( fp ) );
	    KEY( "Position",	ch->position,		fread_number( fp ) );
	    KEY( "Pos",		ch->position,		fread_number( fp ) );
	    KEY( "Practice",	ch->practice,		fread_number( fp ) );
	    KEY( "Prac",	ch->practice,		fread_number( fp ) );
            KEYS( "Prompt",     ch->prompt,             fread_string( fp ) );
 	    KEY( "Prom",	ch->prompt,		fread_string( fp ) );
	    KEYS( "PreT",	ch->pcdata->pretitle,	fread_string( fp ) );
	    KEY( "Pcd",		ch->pcdata->pcdata_flags, fread_flag( fp ) );
	    KEY( "PAfB",	ch->paffected_by,	fread_flag( fp ) );

        if (!str_cmp(word, "Phan")) // Phantasms
        {
            delete ch->pcdata->phantasmInfo;
            ch->pcdata->phantasmInfo = PhantasmInfo::deserialize(fread_string(fp));
            fMatch = true;
            break;
        }

	    break;

	case 'R':
	    KEY( "Rec",		ch->pcdata->record,	fread_string(fp));
	    KEY( "Recy",	ch->pcdata->recycle_log,fread_string(fp));
	    KEY( "Race",        ch->race,	
				race_lookup(fread_string( fp )) );
	    KEY( "RetName",	ch->pcdata->retainername,fread_string(fp));
	    KEY( "RetSex",	ch->pcdata->retainergender,fread_number(fp));
        KEY( "Request", ch->pcdata->request_points, fread_number(fp));

	    if ( !str_cmp( word, "Reli" ) )
	    {
		ch->religion = god_lookup(fread_string(fp));
		fMatch = TRUE;
		break;
	    }

	    if ( !str_cmp( word, "Rcll" ) )
	    {
		ch->recall_to = get_room_index( fread_number( fp ) );
		fMatch = TRUE;
		break;
	    }

	    if ( !str_cmp( word, "Room" ) )
	    {
		ch->in_room = get_room_index( fread_number( fp ) );
		if ( ch->in_room == NULL )
		    ch->in_room = get_room_index( ROOM_VNUM_LIMBO );
		fMatch = TRUE;
		break;
	    }

	    break;

	case 'S':
	    KEY( "SavingThrow",	ch->saving_throw,	fread_number( fp ) );
	    KEY( "Save",	ch->saving_throw,	fread_number( fp ) );
	    KEY( "Scro",	ch->lines,		fread_number( fp ) );
	    KEY( "Sex",		ch->sex,		fread_number( fp ) );
	    KEYS( "ShortDescr",	ch->short_descr,	fread_string( fp ) );
	    KEYS( "ShD",	ch->short_descr,	fread_string( fp ) );
	    KEY( "Sec",         ch->pcdata->security,	fread_number( fp ) );	/* OLC */
            KEY( "Silv",        ch->coins[0],             fread_number( fp ) );
	    KEYS( "Sock",	ch->pcdata->socket_info,fread_string( fp ) );
	    KEY( "Speak",	ch->speaking,		fread_number( fp ) );
	    KEY( "Symb",	ch->symbols_known,	fread_flag(   fp ) );
	    KEYS( "SurN",	ch->pcdata->surname,	fread_string( fp ) );

        if (!str_cmp(word, "Som")) // somatic arts
        {
            delete ch->pcdata->somatic_arts_info;
            ch->pcdata->somatic_arts_info = new SomaticArtsInfo();
            ch->pcdata->somatic_arts_info->Deserialize(fread_string(fp));
            fMatch = true;
            break;
        }

	    if ( !str_cmp( word, "Skill" ) || !str_cmp(word,"Sk"))
	    {
		int sn, i, value;
		char *temp;

		value = fread_number( fp );

		if (ch->version >= 9)
		{
		    temp = fread_string(fp);
		    sn = skill_lookup_full(temp);
		    if (sn < 0)
		    {
			// This can be removed someday, I'm just being lazy.
			if (!str_cmp(temp, "ethereal flame"))
			    ch->pcdata->learned[skill_lookup("coronal glow")] = value;
			else
			{
			    sprintf(log_buf, "Fread_char (%s): unknown skill: '%s'.", ch->name, temp);
			    bug(log_buf, 0);
			}
		    }
		    else
			ch->pcdata->learned[sn] = value;

		    free_string(temp);

		    i = fread_number( fp );
		}
		else
		{
		    temp = fread_word( fp ) ;
		    sn = skill_lookup_full(temp);
		    /* sn    = skill_lookup( fread_word( fp ) ); */
		    if ( sn < 0 )
		    {
		        sprintf(log_buf, "Fread_char (%s): unknown skill: '%s'.", ch->name, temp);
		        bug(log_buf, 0);
		    }
		    else
		        ch->pcdata->learned[sn] = value;
	        }
		fMatch = TRUE;
		break;
	    }

	    if (!str_cmp(word, "SongB"))
	    {
		int entry;

		entry = fread_number(fp);
		if ((entry >= 0) && (entry < MAX_SONGBOOK))
		    ch->pcdata->songbook[entry] = fread_string(fp);
		fMatch = TRUE;
	    }

	    if (!str_cmp(word, "Sac"))
	    {
		int god;
		fread_word(fp); // We don't really need this, human-readability
		if ((god = fread_number(fp)) < MAX_GODS)
		{
		    ch->pcdata->sacrifices[god].level = fread_number(fp);
		    ch->pcdata->sacrifices[god].number = fread_number(fp);
		}
		fMatch = TRUE;
	    }

	    break;

	case 'T':
	    KEY( "Taint",	ch->pcdata->karma,	fread_number( fp ) );
            KEY( "TrueSex",     ch->pcdata->true_sex,  	fread_number( fp ) );
	    KEY( "TSex",	ch->pcdata->true_sex,   fread_number( fp ) );
	    KEY( "Trai",	ch->train,		fread_number( fp ) );
	    KEY( "TrainHP",	ch->pcdata->hp_trains,	fread_number( fp ) );
	    KEY( "TrainMN",	ch->pcdata->mana_trains,fread_number( fp ) );
	    KEY( "Trust",	ch->trust,		fread_number( fp ) );
	    KEY( "Tru",		ch->trust,		fread_number( fp ) );

//brazen: catch for shuddeni at load to make sure they are tainted
	    if(ch->race == global_int_race_shuddeni)
	        if(ch->pcdata->karma < FAINTREDAURA) ch->pcdata->karma = FAINTREDAURA;

	    if ( !str_cmp( word, "Trait" ))
	    {
		char *temp;

		temp = fread_string( fp );

		for (x = 0; trait_table[x].name; x++)
		    if (!str_cmp(temp, trait_table[x].name))
		    {
			BIT_SET(ch->pcdata->traits, trait_table[x].bit);
			break;
		    }

		free_string(temp);
		fMatch = TRUE;
		break;
	    }

	    if ( !str_cmp( word, "Title" )  || !str_cmp( word, "Titl"))
	    {
		ch->pcdata->title = fread_string( fp );
    		if (ch->pcdata->title[0] != '.' && ch->pcdata->title[0] != ',' 
		&&  ch->pcdata->title[0] != '!' && ch->pcdata->title[0] != '?')
		{
		    sprintf( buf, " %s", ch->pcdata->title );
		    free_string( ch->pcdata->title );
		    ch->pcdata->title = str_dup( buf );
		}
		fMatch = TRUE;
		break;
	    }

	    break;

	case 'V':
	    KEY( "Version",     ch->version,		fread_number ( fp ) );
	    KEY( "Vers",	ch->version,		fread_number ( fp ) );
	    if ( !str_cmp( word, "Vnum" ) )
	    {
		ch->pIndexData = get_mob_index( fread_number( fp ) );
		fMatch = TRUE;
		break;
	    }
	    break;

	case 'W':
	    KEY( "Wanted",	ch->pcdata->times_wanted,fread_number(fp ) );
	    KEY( "Wimpy",	ch->wimpy,		fread_number( fp ) );
	    KEY( "Wimp",	ch->wimpy,		fread_number( fp ) );
	    KEY( "Wizn",	ch->wiznet,		fread_flag( fp ) );
	    break;

	case 'X':
	    KEY( "Xtra",	ch->extra_flags,	fread_flag( fp ) );
	    break;
	}

	if ( !fMatch )
	{
	    bug( "Fread_char: no match.", 0 );
	    bug( word, 0);
	    fread_to_eol( fp );
	}
    }
}

/* load a pet from the forgotten reaches */
void fread_pet( CHAR_DATA *ch, FILE *fp )
{
    const char *word(0);
    CHAR_DATA *pet;
    bool fMatch;
    int percent, i, x;

    /* first entry had BETTER be the vnum or we barf */
    word = feof(fp) ? "END" : fread_word(fp);
    if (!str_cmp(word,"Vnum"))
    {
    	int vnum;
    	
    	vnum = fread_number(fp);
    	if (get_mob_index(vnum) == NULL)
        {
    	    bug("Fread_pet: bad vnum %d.",vnum);
            pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
        }
    	else
    	    pet = create_mobile(get_mob_index(vnum));
    }
    else
    {
        bug("Fread_pet: no vnum in file.",0);
        pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
    }
    
    for ( ; ; )
    {
    	word 	= feof(fp) ? "END" : fread_word(fp);
    	fMatch = FALSE;
    	
    	switch (UPPER(word[0]))
    	{
    	case '*':
    	    fMatch = TRUE;
    	    fread_to_eol(fp);
    	    break;
    		
    	case 'A':
    	    KEY( "Act",		pet->act,		fread_flag(fp));
    	    KEY( "AfBy",	pet->affected_by,	fread_flag(fp));
    	    KEY( "Alig",	pet->alignment,		fread_number(fp));
    	    
    	    if (!str_cmp(word,"ACs"))
    	    {
    	    	int i;
    	    	
    	    	for (i = 0; i < 4; i++)
    	    	    pet->armor[i] = fread_number(fp);
    	    	fMatch = TRUE;
    	    	break;
    	    }
    	    
    	    if (!str_cmp(word,"AffD"))
    	    {
    	    	AFFECT_DATA *paf;
    	    	int sn;
    	    	
    	    	paf = new_affect();
    	    	
    	    	sn = skill_lookup_full(fread_word(fp));
    	     	if (sn < 0)
    	     	    bug("Fread_pet: unknown skill.",0);
    	     	else
    	     	   paf->type = sn;
    	     	   
    	     	paf->level	= fread_number(fp);
    	     	paf->duration	= fread_number(fp);
    	     	paf->modifier	= fread_number(fp);
    	     	paf->location	= fread_number(fp);
    	     	paf->bitvector	= fread_number(fp);
    	     	paf->next	= pet->affected;
    	     	pet->affected	= paf;
    	     	fMatch		= TRUE;
    	     	break;
    	    }

            if (!str_cmp(word,"Affc"))
            {
                AFFECT_DATA *paf;
                int sn;
 
                paf = new_affect();
 
                sn = skill_lookup_full(fread_word(fp));
                if (sn < 0)
                    bug("Fread_pet: unknown skill.",0);
                else
                   paf->type = sn;
 
		paf->where	= fread_number(fp);
                paf->level      = fread_number(fp);
                paf->duration   = fread_number(fp);
                paf->modifier   = fread_number(fp);
                paf->location   = fread_number(fp);
                paf->bitvector  = fread_number(fp);

		if (paf->type == gsn_generic || paf->type == gsn_sacrifice)
		    paf->point = (void *) fread_string(fp);

                paf->next       = pet->affected;
                pet->affected   = paf;
                fMatch          = TRUE;
                break;
            }

    	    if (!str_cmp(word,"AMax"))
    	    {
    	     	int stat;
    	     	
    	     	for (stat = 0; stat < MAX_STATS; stat++)
    	     	    pet->max_stat[stat] = fread_number(fp);
    	     	fMatch = TRUE;
    	     	break;
    	    }
    	     
    	    if (!str_cmp(word,"AMod"))
    	    {
    	     	int stat;
    	     	
    	     	for (stat = 0; stat < MAX_STATS; stat++)
    	     	    pet->mod_stat[stat] = fread_number(fp);
    	     	fMatch = TRUE;
    	     	break;
    	    }
    	     
    	    if (!str_cmp(word,"Attr"))
    	    {
    	         int stat;
    	         
    	         for (stat = 0; stat < MAX_STATS; stat++)
    	             pet->perm_stat[stat] = fread_number(fp);
    	         fMatch = TRUE;
    	         break;
    	    }
    	    break;
    	     
    	 case 'C':
             KEY( "Clan",       pet->clan,       clan_lookup(fread_string(fp)));
    	     KEY( "Comm",	pet->comm,		fread_flag(fp));
	
	     if (!str_cmp(word, "Coin"))
	     {
		i = fread_number(fp);
		
		for (x = 0; x < i; x++)
		{
		    if (x >= MAX_COIN)
		    {
			fread_to_eol(fp);
			break;
		    }
		    else
		        pet->coins[x] = fread_number(fp);
		}
		fMatch = TRUE;
		break;
	     }

    	     break;
    	     
    	 case 'D':
    	     KEY("Desc",	pet->description,	fread_string(fp));
    	     KEY("DamVerb",	pet->dam_verb,	fread_string(fp));
             KEY("DamType", pet->dam_type, fread_number(fp));
	   
	     if (!str_cmp(word, "Dam"))
	     {
		/* strange read method for compatibility with older pfiles */
		/* which only stored damroll information (ie. one number)  */
		/* code mostly stolen from fread_number                    */

		int i;
		char c;
		bool isneg;

		i = fread_number(fp);
		c = getc(fp);
		if ((c == '\r') || (c == '\n'))
		    pet->damroll = i;
  		else
		{
		    isneg = FALSE;
		    pet->damage[0] = i;
		    if (isdigit(c) || (c == '-'))
		    {
			if (c == '-')
			{
			    isneg = TRUE;
			    pet->damage[1] = 0;
			}
			else
		            pet->damage[1] = c - '0';
                        c = getc(fp);
		        if (isdigit(c))
		            while (isdigit(c))
		 	    {
				if (isneg)
				{
				    if (pet->damage[1] == 0)
					pet->damage[1] = ((c - '0') * -1);
				    else
					pet->damage[1] = pet->damage[1] * 10 - c + '0';
				}
				else
			            pet->damage[1] = pet->damage[1] * 10 + c - '0';
				c = getc(fp);
			    }
		        pet->damroll = fread_number(fp);
		    }
		}
		fMatch = TRUE;
		break;
	     }	
    	     break;
    	     
    	 case 'E':
    	     if (!str_cmp(word,"End"))
	     {
		pet->leader = ch;
		pet->master = ch;
		if (IS_SET(pet->act, ACT_FAMILIAR))
			ch->familiar = pet;
		else
			ch->pet = pet;
    		/* adjust hp mana move up  -- here for speed's sake */
    		percent = (current_time - ch->laston) * 25 / ( 2 * 60 * 60);
 
    		if (percent > 0 && !IS_AFFECTED(ch,AFF_POISON)
    		&&  !IS_AFFECTED(ch,AFF_PLAGUE))
    		{
		    percent = UMIN(percent,100);
    		    pet->hit	+= (pet->max_hit - pet->hit) * percent / 100;
        	    pet->mana   += (pet->max_mana - pet->mana) * percent / 100;
        	    pet->move   += (pet->max_move - pet->move)* percent / 100;
    		}
    	     	return;
	     }
    	     KEY( "Exp",	pet->exp,		fread_number(fp));
    	     KEY( "Ep",		pet->ep,		fread_number(fp));
    	     break;
    	     
    	 case 'G':
    	     KEY( "Gold",	pet->coins[0],		fread_number(fp));
    	     break;
    	     
    	 case 'H':
    	     KEY( "Hit",	pet->hitroll,		fread_number(fp)); 

    	     if (!str_cmp(word,"HMV"))
    	     {
    	     	pet->hit	= fread_number(fp);
    	     	pet->max_hit	= fread_number(fp);
    	     	pet->mana	= fread_number(fp);
    	     	pet->max_mana	= fread_number(fp);
    	     	pet->move	= fread_number(fp);
    	     	pet->max_move	= fread_number(fp);
    	     	fMatch = TRUE;
    	     	break;
    	     }
    	     break;
    	     
     	case 'L':
    	    KEY( "Levl",	pet->level,		fread_number(fp));
    	    KEY( "LnD",	pet->long_descr,	fread_string(fp));
	        KEY( "LogO",	pet->laston,		fread_number(fp));
            KEY( "Luck",    pet->luck,       fread_number(fp));
    	    break;

        case 'M':
            if ( !str_cmp( word, "Mval"))
            {
                for (int i = 0; i < MAX_MOBVALUE; ++i)
                    pet->mobvalue[i] = fread_number( fp );
                fMatch		= TRUE;
                break;
            }

            if (!str_cmp(word, "Mstr"))
            {
                int i(fread_number(fp));
                pet->stringvalue[i] = fread_string(fp);
                fMatch = TRUE;
                break;
            }

            if ( !str_cmp( word, "Mfoc"))
            {
                int i(fread_number(fp));
                pet->memfocus[i] = get_char_room(ch, fread_string(fp));
                fMatch = TRUE;
                break;
            }
            break;
    	     
    	case 'N':
            if (!str_cmp(word, "Name"))
            {
                setName(*pet, fread_string(fp));
                fMatch = TRUE;
                break;
            }
    	    break;
    	     
    	case 'P':
    	     KEY( "Pos",	pet->position,		fread_number(fp));
    	     break;
    	     
        case 'R':
    	    KEY( "Race",	pet->race, race_lookup(fread_string(fp)));
            if (!str_cmp(word, "Resist"))
            {
                int resistType(fread_number(fp));
                int resistValue(fread_number(fp));
                if (resistType < 0 || resistType >= MAX_RESIST) bug("Invalid resist type read in pet load [%d]", resistType);
                else pet->resist[resistType] = resistValue;
                fMatch = true;
                break;
            }
    	    break;
 	    
    	case 'S' :
    	    KEY( "Save",	pet->saving_throw,	fread_number(fp));
    	    KEY( "Sex",		pet->sex,		fread_number(fp));
    	    KEY( "ShD",		pet->short_descr,	fread_string(fp));
            KEY( "Silv",        pet->coins[0],            fread_number( fp ) );
    	    break;
	}    
	
    	if ( !fMatch )
    	{
    	    bug("Fread_pet: no match.",0);
    	    fread_to_eol(fp);
    	}
    }
}

extern	OBJ_DATA	*obj_free;

void fread_obj_room( ROOM_INDEX_DATA *room, FILE *fp )
{
    OBJ_DATA *obj(0);
    const char *word(0);
    int iNest;
    bool fMatch;
    bool fNest;
    bool fVnum;
    bool first;
    bool new_format;  /* to prevent errors */
    bool make_new;    /* update object */
    
    fVnum = FALSE;
    obj = NULL;
    first = TRUE;  /* used to counter fp offset */
    new_format = FALSE;
    make_new = FALSE;

    word   = feof( fp ) ? "End" : fread_word( fp );
    if (!str_cmp(word,"Vnum" ))
    {
        int vnum;
	first = FALSE;  /* fp will be in right place */
 
        vnum = fread_number( fp );
        if (  get_obj_index( vnum )  == NULL )
	{
            bug( "Fread_obj: bad vnum %d.", vnum );
	    while (!str_cmp(word, "End"))
		{
		fread_to_eol(fp);
		word = fread_word(fp);
		}
	}
        else
	{
	    obj = create_object(get_obj_index(vnum),-1);
	    new_format = TRUE;
	}
	    
    }

    if (obj == NULL)  /* either not found or old style */
    {
    	obj = new_obj();
    	setName(*obj, "");
    	obj->short_descr	= str_dup( "" );
    	obj->description	= str_dup( "" );
	obj->owner		= str_dup( "" );
	obj->lastowner[0] 	= str_dup( "" );
    }

    fNest		= FALSE;
    fVnum		= TRUE;
    iNest		= 0;

    for ( ; ; )
    {
	if (first)
	    first = FALSE;
	else
	    word   = feof( fp ) ? "End" : fread_word( fp );
	fMatch = FALSE;

	switch ( UPPER(word[0]) )
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol( fp );
	    break;

	case 'A':
	    if (!str_cmp(word,"AffD"))
	    {
		AFFECT_DATA *paf;
		int sn;

		paf = new_affect();

		sn = skill_lookup_full(fread_word(fp));
		if (sn < 0)
		    bug("Fread_obj_room: unknown skill.",0);
		else
		    paf->type = sn;

		paf->level	= fread_number( fp );
		paf->duration	= fread_number( fp );
		paf->modifier	= fread_number( fp );
		paf->location	= fread_number( fp );
		paf->bitvector	= fread_number( fp );
		paf->next	= obj->affected;
		obj->affected	= paf;
		fMatch		= TRUE;
		break;
	    }
            if (!str_cmp(word,"Affc"))
            {
                AFFECT_DATA *paf;
                int sn;
 
                paf = new_affect();
 
                sn = skill_lookup_full(fread_word(fp));
                if (sn < 0)
                    bug("Fread_obj_room: unknown skill.",0);
                else
                    paf->type = sn;
 
		paf->where	= fread_number( fp );
                paf->level      = fread_number( fp );
                paf->duration   = fread_number( fp );
                paf->modifier   = fread_number( fp );
                paf->location   = fread_number( fp );
                paf->bitvector  = fread_number( fp );

		if (paf->type == gsn_generic || paf->type == gsn_sacrifice)
		    paf->point = (void *) fread_string(fp);

                paf->next       = obj->affected;
                obj->affected   = paf;
                fMatch          = TRUE;
                break;
            }
	    break;

	case 'C':
	    KEY( "Cond",	obj->condition,		fread_number( fp ) );
	    KEY( "Cost",	obj->cost,		fread_number( fp ) );
	    break;

	case 'D':
	    KEY( "Description",	obj->description,	fread_string( fp ) );
	    KEY( "Desc",	obj->description,	fread_string( fp ) );
	    break;

	case 'E':

	    if ( !str_cmp( word, "Enchanted"))
	    {
		obj->enchanted = TRUE;
	 	fMatch 	= TRUE;
		break;
	    }

	    KEY( "ExtraFlags",	obj->extra_flags[0],	fread_number( fp ) );
	    KEY( "ExtF",	obj->extra_flags[0],	fread_number( fp ) );
	    KEY( "ExF0",	obj->extra_flags[0],	fread_number( fp ) );
	    KEY( "ExF1",	obj->extra_flags[1],	fread_number( fp ) );

	    if ( !str_cmp( word, "ExtraDescr" ) || !str_cmp(word,"ExDe"))
	    {
		EXTRA_DESCR_DATA *ed;

		ed = new_extra_descr();

		ed->keyword		= fread_string( fp );
		ed->description		= fread_string( fp );
		ed->next		= obj->extra_descr;
		obj->extra_descr	= ed;
		fMatch = TRUE;
	    }

	    if ( !str_cmp( word, "End" ) )
	    {
		if ( !fNest || ( fVnum && obj->pIndexData == NULL ) )
		{
		    bug( "Fread_obj: incomplete object.", 0 );
            setName(*obj, str_empty);
		    free_string( obj->description );
		    free_string( obj->short_descr );
		    obj->next = obj_free;
		    obj_free  = obj;
		    return;
		}
		else
        {
		    if ( !fVnum )
		    {
                setName(*obj, str_empty);
		        free_string( obj->description );
                free_string( obj->short_descr );
                obj->next = obj_free;
                obj_free  = obj;

                obj = create_object( get_obj_index( OBJ_VNUM_DUMMY ), 0 );
		    }

		    if (!new_format)
		    {
		    	obj->next	= object_list;
		    	object_list	= obj;
		    	obj->pIndexData->count++;
		    }

		    if (!obj->pIndexData->new_format 
		    && obj->item_type == ITEM_ARMOR
		    &&  obj->value[1] == 0)
		    {
			obj->value[1] = obj->value[0];
			obj->value[2] = obj->value[0];
		    }
		    if (make_new)
		    {
			int wear;
			
			wear = obj->wear_loc;
			extract_obj(obj);

			obj = create_object(obj->pIndexData,0);
			obj->wear_loc = wear;
		    }
// brazen: Ticket #263: Put de lime in de coconut
		    if ( iNest == 0 || rgObjNest[iNest] == NULL || obj == rgObjNest[iNest-1])
			obj_to_room( obj, room );
		    else if (rgObjNest[iNest-1]->item_type == ITEM_CONTAINER 
			|| rgObjNest[iNest-1]->item_type == ITEM_CORPSE_PC)
			obj_to_obj( obj, rgObjNest[iNest-1] );
		    else
		        obj_to_room( obj, room);
		    if (rgObjNest[iNest-1] != NULL &&
		        rgObjNest[iNest-1]->item_type == ITEM_CORPSE_PC) 
		    {
/*		        SET_BIT(room->room_flags, ROOM_SAVE);*/
		        rgObjNest[iNest-1]->owner = str_dup(strrchr(rgObjNest[iNest-1]->short_descr,' ')+1);
		        rgObjNest[iNest-1]->lastowner[0] = str_dup(rgObjNest[iNest-1]->owner);
			rgObjNest[iNest]->owner = str_dup(rgObjNest[iNest-1]->owner);
			rgObjNest[iNest]->lastowner[0] = str_dup(rgObjNest[iNest-1]->owner);
		    }
		    return;
		}
	    }
	    break;

	case 'I':
	    KEY( "ItemType",	obj->item_type,		fread_number( fp ) );
	    KEY( "Ityp",	obj->item_type,		fread_number( fp ) );
	    break;

	case 'L':
	    KEY( "Level",	obj->level,		fread_number( fp ) );
	    KEY( "Lev",		obj->level,		fread_number( fp ) );
	    KEYS("Lore",	obj->lore,		fread_string( fp ) );
	    break;

	case 'N':
        if (!str_cmp(word, "Name"))
        {
            setName(*obj, fread_string(fp));
            fMatch = TRUE;
            break;
        }

	    if ( !str_cmp( word, "Nest" ) )
	    {
		iNest = fread_number( fp );
		if ( iNest < 0 || iNest >= MAX_NEST )
		{
		    bug( "Fread_obj: bad nest %d.", iNest );
		}
		else
		{
		    rgObjNest[iNest] = obj;
		    fNest = TRUE;
		}
		fMatch = TRUE;
	    }
	    break;

   	case 'O':
	    if ( !str_cmp( word, "Oval"))
	    {
		obj->objvalue[0] = fread_number( fp );
		obj->objvalue[1] = fread_number( fp );
		obj->objvalue[2] = fread_number( fp );
		obj->objvalue[3] = fread_number( fp );
		obj->objvalue[4] = fread_number( fp );
		obj->objvalue[5] = fread_number( fp );
		obj->objvalue[6] = fread_number( fp );
		obj->objvalue[7] = fread_number( fp );
		obj->objvalue[8] = fread_number( fp );
		obj->objvalue[9] = fread_number( fp );
		fMatch		= TRUE;
		break;
	    }

	    if ( !str_cmp( word,"Oldstyle" ) )
	    {
		if (obj->pIndexData != NULL && obj->pIndexData->new_format)
		    make_new = TRUE;
		fMatch = TRUE;
	    }
	    break;
		    

	case 'S':
	    KEY( "ShortDescr",	obj->short_descr,	fread_string( fp ) );
	    KEY( "ShD",		obj->short_descr,	fread_string( fp ) );

	    if ( !str_cmp( word, "Spell" ) )
	    {
		int iValue;
		int sn;

		iValue = fread_number( fp );
		sn     = skill_lookup_full( fread_word( fp ) );
		if ( iValue < 0 || iValue > 3 )
		{
		    bug( "Fread_obj_room: bad iValue %d.", iValue );
		}
		else if ( sn < 0 )
		{
		    bug( "Fread_obj_room: unknown skill.", 0 );
		}
		else
		{
		    obj->value[iValue] = sn;
		}
		fMatch = TRUE;
		break;
	    }

	    break;

	case 'T':
	    KEY( "Timer",	obj->timer,		fread_number( fp ) );
	    KEY( "Time",	obj->timer,		fread_number( fp ) );
	    break;

	case 'V':
	    if ( !str_cmp( word, "Values" ) || !str_cmp(word,"Vals"))
	    {
		obj->value[0]	= fread_number( fp );
		obj->value[1]	= fread_number( fp );
		obj->value[2]	= fread_number( fp );
		obj->value[3]	= fread_number( fp );
		if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
		   obj->value[0] = obj->pIndexData->value[0];
		fMatch		= TRUE;
		break;
	    }

	    if ( !str_cmp( word, "Val" ) )
	    {
		obj->value[0] 	= fread_number( fp );
	 	obj->value[1]	= fread_number( fp );
	 	obj->value[2] 	= fread_number( fp );
		obj->value[3]	= fread_number( fp );
		obj->value[4]	= fread_number( fp );
		fMatch = TRUE;
		break;
	    }

	    if ( !str_cmp( word, "Vnum" ) )
	    {
		int vnum;

		vnum = fread_number( fp );
		if ( ( obj->pIndexData = get_obj_index( vnum ) ) == NULL )
		    {
		    bug( "Fread_obj: bad vnum %d.", vnum );
		    while (!str_cmp(word, "End"))
			{
			fread_to_eol(fp);
			word = fread_word(fp);
			}
		    }
		else
		    fVnum = TRUE;
		fMatch = TRUE;
		break;
	    }
	    break;

	case 'W':
	    KEY( "WearFlags",	obj->wear_flags,	fread_number( fp ) );
	    KEY( "WeaF",	obj->wear_flags,	fread_number( fp ) );
	    KEY( "WearLoc",	obj->wear_loc,		fread_number( fp ) );
	    KEY( "Wear",	obj->wear_loc,		fread_number( fp ) );
	    KEY( "Weight",	obj->weight,		fread_number( fp ) );
	    KEY( "Wt",		obj->weight,		fread_number( fp ) );
	    break;

	}

	if ( !fMatch )
	{
	    bug( "Fread_obj: no match.", 0 );
	    fread_to_eol( fp );
	}
    }
}

void fread_obj( CHAR_DATA *ch, FILE *fp )
{
    OBJ_DATA *obj(0);
	const char *word(0);
    int iNest;
    bool fMatch;
    bool fNest;
    bool fVnum;
    bool first;
    bool new_format;  /* to prevent errors */
    bool make_new;    /* update object */
    char buf[MAX_STRING_LENGTH];
    int counter = 0; 
    fVnum = FALSE;
    obj = NULL;
    first = TRUE;  /* used to counter fp offset */
    new_format = FALSE;
    make_new = FALSE;

    word   = feof( fp ) ? "End" : fread_word( fp );
    counter++;
    if (!str_cmp(word,"Vnum" ))
    {
        int vnum;
	first = FALSE;  /* fp will be in right place */
 
        vnum = fread_number( fp );
        if (  get_obj_index( vnum )  == NULL )
	{
            bug( "Fread_obj: bad vnum %d.", vnum );
	    while (!str_cmp(word, "End"))
		{
		fread_to_eol(fp);
		word = fread_word(fp);
		}
	}
        else
	{
	    obj = create_object(get_obj_index(vnum),-1);
	    new_format = TRUE;
	}
	    
    }

    if (obj == NULL)  /* either not found or old style */
    {
    	obj = new_obj();
    	setName(*obj, "");
    	obj->short_descr	= str_dup( "" );
    	obj->description	= str_dup( "" );
    }

    fNest		= FALSE;
    fVnum		= TRUE;
    iNest		= 0;

    for ( ; ; )
    {
	if (first)
	    first = FALSE;
	else
	    word   = feof( fp ) ? "End" : fread_word( fp );
	fMatch = FALSE;

	switch ( UPPER(word[0]) )
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol( fp );
	    break;

	case 'A':
	    if (!str_cmp(word,"AffD"))
	    {
		AFFECT_DATA *paf;
		int sn;

		paf = new_affect();

		sn = skill_lookup_full(fread_word(fp));
		if (sn < 0)
		    bug("Fread_obj: unknown skill.",0);
		else
		    paf->type = sn;

		paf->level	= fread_number( fp );
		paf->duration	= fread_number( fp );
		paf->modifier	= fread_number( fp );
		paf->location	= fread_number( fp );
		paf->bitvector	= fread_number( fp );
		paf->next	= obj->affected;
		obj->affected	= paf;
		fMatch		= TRUE;
		break;
	    }
        
        if (!str_cmp(word,"Affc"))
        {
            AFFECT_DATA *paf;
            int sn;

            paf = new_affect();

            const char * effectName(fread_word(fp));
            if (effectName[0] == '\0')
                sn = -1;
            else
            {
                sn = skill_lookup_full(effectName);
                if (sn < 0)
                    bug("Fread_obj: unknown skill.",0);
                else
                    paf->type = sn;
            }

            paf->where	    = fread_number( fp );
            paf->level      = fread_number( fp );
            paf->duration   = fread_number( fp );
            paf->modifier   = fread_number( fp );
            paf->location   = fread_number( fp );
            paf->bitvector  = fread_number( fp );

            if (paf->type == gsn_generic || paf->type == gsn_sacrifice
            || paf->type == gsn_firebrand || paf->type == gsn_lightningbrand
            || paf->type == gsn_frostbrand || paf->type == gsn_consecrate
            || paf->type == gsn_defilement)
                paf->point = (void *) fread_string(fp);

            paf->next       = obj->affected;
            obj->affected   = paf;
            fMatch          = TRUE;
            break;
        }
	    break;

	case 'C':
	    KEY( "Cost",	obj->cost,		fread_number( fp ) );

	    if ( !str_cmp( word, "Cond" ))
	    {
		obj->condition = fread_number(fp);
		if (ch->version <= 17)
		    obj->condition = 100;
		fMatch = TRUE;
	    }

	    break;

	case 'D':
	    KEY( "Description",	obj->description,	fread_string( fp ) );
	    KEY( "Desc",	obj->description,	fread_string( fp ) );
	    break;

	case 'E':

	    if ( !str_cmp( word, "Enchanted"))
	    {
		obj->enchanted = TRUE;
	 	fMatch 	= TRUE;
		break;
	    }

	    KEY( "ExtraFlags",	obj->extra_flags[0],	fread_number( fp ) );
	    KEY( "ExtF",	obj->extra_flags[0],	fread_number( fp ) );
	    KEY( "ExF0",	obj->extra_flags[0],	fread_number( fp ) );
	    KEY( "ExF1",	obj->extra_flags[1],	fread_number( fp ) );

	    if ( !str_cmp( word, "ExtraDescr" ) || !str_cmp(word,"ExDe"))
	    {
		EXTRA_DESCR_DATA *ed;

		ed = new_extra_descr();

		ed->keyword		= fread_string( fp );
		ed->description		= fread_string( fp );
		ed->next		= obj->extra_descr;
		obj->extra_descr	= ed;
		fMatch = TRUE;
	    }

	    if ( !str_cmp( word, "End" ) )
	    {
		if ( !fNest || ( fVnum && obj->pIndexData == NULL ) )
		{
		    bug( "Fread_obj: incomplete object.", 0 );
		    setName(*obj, str_empty);
		    free_string( obj->description );
		    free_string( obj->short_descr );
		    obj->next = obj_free;
		    obj_free  = obj;
		    return;
		}
		else
        {
		    if ( !fVnum )
		    {
		        setName(*obj, str_empty);
		        free_string( obj->description );
                free_string( obj->short_descr );
                obj->next = obj_free;
                obj_free  = obj;

                obj = create_object( get_obj_index( OBJ_VNUM_DUMMY ), 0 );
		    }

		    if (!new_format)
		    {
		    	obj->next	= object_list;
		    	object_list	= obj;
		    	obj->pIndexData->count++;
		    }

		    if (!obj->pIndexData->new_format 
		    && obj->item_type == ITEM_ARMOR
		    &&  obj->value[1] == 0)
		    {
			obj->value[1] = obj->value[0];
			obj->value[2] = obj->value[0];
		    }
		    if (make_new)
		    {
			int wear;
			
			wear = obj->wear_loc;
			extract_obj(obj);

			obj = create_object(obj->pIndexData,0);
			obj->wear_loc = wear;
		    }
		    if ( iNest == 0 || rgObjNest[iNest] == NULL || obj == rgObjNest[iNest-1])
			obj_to_char( obj, ch );
		    else if (rgObjNest[iNest-1] == obj ||
			rgObjNest[iNest-1]->item_type != ITEM_CONTAINER)
			obj_to_char( obj, ch );
		    else
			obj_to_obj( obj, rgObjNest[iNest-1] );
		    return;
		}
	    }
	    break;

	case 'I':
	    KEY( "ItemType",	obj->item_type,		fread_number( fp ) );
	    KEY( "Ityp",	obj->item_type,		fread_number( fp ) );
	    break;

	case 'L':
	    KEY( "Level",	obj->level,		fread_number( fp ) );
	    KEY( "Lev",		obj->level,		fread_number( fp ) );
	    KEYS( "Lore",	obj->lore,		fread_string( fp ) );

	    if (!str_cmp(word, "LstO"))
	    {
		int numlast = fread_number( fp );
		int numread, i;
		
		numread = UMIN(numlast, MAX_LASTOWNER);

		for (i = 0; i < numread; i++)
		    obj->lastowner[i] = fread_string( fp ) ;

		if (numread < numlast)
		    fread_to_eol( fp );

		fMatch = TRUE;
		break;
	    }
	    break;

	case 'M':
	    KEY( "Mat",		obj->material,		fread_number( fp ) );
	    break;
	    
	case 'N':
        if (!str_cmp(word, "Name"))
        {
            setName(*obj, fread_string(fp));
            fMatch = TRUE;
            break;
        }

	    if ( !str_cmp( word, "Nest" ) )
	    {
		iNest = fread_number( fp );
		if ( iNest < 0 || iNest >= MAX_NEST )
		{
		    bug( "Fread_obj: bad nest %d.", iNest );
		}
		else
		{
		    rgObjNest[iNest] = obj;
		    fNest = TRUE;
		}
		fMatch = TRUE;
	    }
	    break;

   	case 'O':

	    KEY( "OStr",	obj->obj_str,		fread_string(fp));

	    if ( !str_cmp( word, "Oval"))
	    {
            for (int i = 0; i < MAX_MOBVALUE; ++i)
        		obj->objvalue[i] = fread_number( fp );
            fMatch		= TRUE;
            break;
	    }

        if (!str_cmp(word, "Ostring"))
        {
            int i(fread_number(fp));
            obj->stringvalue[i] = fread_string(fp);
            fMatch = TRUE;
            break;
        }

	    if ( !str_cmp( word, "Ofoc"))
		{
            int i(fread_number(fp));
			obj->objfocus[i] = get_char_room(ch, fread_string(fp));
            fMatch = TRUE;
            break;
		}

	    if ( !str_cmp( word,"Oldstyle" ) )
        {
            if (obj->pIndexData != NULL && obj->pIndexData->new_format)
                make_new = TRUE;
            fMatch = TRUE;
            break;
        }
	    break;

	case 'S':
	    KEY( "ShortDescr",	obj->short_descr,	fread_string( fp ) );
	    KEY( "ShD",		obj->short_descr,	fread_string( fp ) );

	    if ( !str_cmp( word, "Spell" ) )
	    {
		int iValue;
		int sn;

		iValue = fread_number( fp );
		sn     = skill_lookup_full( fread_word( fp ) );
		if ( iValue < 0 || iValue > 3 )
		{
		    bug( "Fread_obj: bad iValue %d.", iValue );
		}
		else if ( sn < 0 )
		{
		    bug( "Fread_obj: unknown skill.", 0 );
		}
		else
		{
		    obj->value[iValue] = sn;
		}
		fMatch = TRUE;
		break;
	    }

	    break;

	case 'T':
	    KEY( "Timer",	obj->timer,		fread_number( fp ) );
	    KEY( "Time",	obj->timer,		fread_number( fp ) );
	    KEY( "TStmp",	obj->timestamp,		fread_number( fp ) );
	    break;

	case 'V':
	    if ( !str_cmp( word, "Values" ) || !str_cmp(word,"Vals"))
	    {
		obj->value[0]	= fread_number( fp );
		obj->value[1]	= fread_number( fp );
		obj->value[2]	= fread_number( fp );
		obj->value[3]	= fread_number( fp );
		if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
		   obj->value[0] = obj->pIndexData->value[0];
		fMatch		= TRUE;
		break;
	    }

	    if ( !str_cmp( word, "Val" ) )
	    {
		obj->value[0] 	= fread_number( fp );
	 	obj->value[1]	= fread_number( fp );
	 	obj->value[2] 	= fread_number( fp );
		obj->value[3]	= fread_number( fp );
		obj->value[4]	= fread_number( fp );
		fMatch = TRUE;
		break;
	    }

	    if ( !str_cmp( word, "Vnum" ) )
	    {
		int vnum;

		vnum = fread_number( fp );
		if ( ( obj->pIndexData = get_obj_index( vnum ) ) == NULL )
		    {
		    bug( "Fread_obj: bad vnum %d.", vnum );
		    while (!str_cmp(word, "End"))
			{
			fread_to_eol(fp);
			word = fread_word(fp);
			}
		    }
		else
		    fVnum = TRUE;
		fMatch = TRUE;
		break;
	    }
	    break;

	case 'W':
	    KEY( "WearFlags",	obj->wear_flags,	fread_number( fp ) );
	    KEY( "WeaF",	obj->wear_flags,	fread_number( fp ) );
	    KEY( "WearLoc",	obj->wear_loc,		fread_number( fp ) );
	    KEY( "Wear",	obj->wear_loc,		fread_number( fp ) );
	    KEY( "Weight",	obj->weight,		fread_number( fp ) );
	    KEY( "WornOn",	obj->worn_on,		fread_number( fp ) );
	    KEY( "Wt",		obj->weight,		fread_number( fp ) );
	    break;

	}

	if ( !fMatch )
	{
	    sprintf(buf, "Fread_obj: no match. File: %s word: %s vnum: %i line: %i", ch->name, word, obj->pIndexData->vnum,counter);
	    bug( buf, 0 );
	}
    }
}

ACCOUNT_DATA *find_account(char *argument)
{
    char strsave[MAX_INPUT_LENGTH];
    FILE *fp(0);
    const char *word(0);
    ACCOUNT_DATA *acct = NULL;
    bool fMatch;

    sprintf(strsave, "%s%s", ACCT_DIR, capitalize(argument));
    if ((fp = fopen(strsave, "r")) != NULL)
    {
	acct = new_acct();

	for ( ; ; )
	{
	    word   = feof( fp ) ? "End" : fread_word( fp );
	    fMatch = FALSE;

	    switch (UPPER(word[0]))
	    {
	    	case '*':
	    	    fMatch = TRUE;
	    	    fread_to_eol( fp );
	    	    break;
		
		case 'C':
		    KEYS("Char",	acct->chars,	fread_string(fp));
		    break;

		case 'D':
		    KEYS("Dele",	acct->deleted,	fread_string(fp));
		    KEYS("Data",	acct->immrecord,fread_string(fp));

		case 'E':
		    KEYS("Email",	acct->email,	fread_string(fp));

		    if (!str_cmp(word, "End"))
		    {
			fclose(fp);
			return acct;
		    }
		    break;

		case 'F':
		    KEY("Flag",		acct->flags,	fread_flag(fp));
		    break;

		case 'N':
		    KEYS("Name",	acct->name,	fread_string(fp));
		    break;

		case 'P':
		    KEY("Pnts",		acct->award_points, fread_number(fp));
		    KEYS("Pwd",		acct->pwd,	fread_string(fp));
		    KEYS("Pmpt",	acct->def_prompt,	fread_string(fp));
		    break;

		case 'S':
		    KEYS("Sock",	acct->socket_info, fread_string(fp));
	 	    break;
	    }

	    if ( !fMatch )
	    {
	        bug( "find_account: no match.", 0 );
	        bug( word, 0);
	        fread_to_eol( fp );
	    }
	}
    }

    return acct;
}

