/***************************************************************************
 *  File: mem.c                                                            *
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
#include <time.h>
#include "merc.h"
#include "olc.h"
#include "LeyGroup.h"
#include "Shades.h"
#include "Faction.h"

/*
 * Globals
 */
extern          int                     top_reset;
extern          int                     top_area;
extern          int                     top_exit;
extern          int                     top_ed;
extern          int                     top_room;

RUMORS			*	rumor_free = 0;
AREA_DATA		*	area_free = 0;
EXTRA_DESCR_DATA	*	extra_descr_free = 0;
EXIT_DATA		*	exit_free = 0;
ROOM_INDEX_DATA		*	room_index_free = 0;
OBJ_INDEX_DATA		*	obj_index_free = 0;
SHOP_DATA		*	shop_free = 0;
MOB_INDEX_DATA		*	mob_index_free = 0;
RESET_DATA		*	reset_free = 0;
// HELP_DATA		*	help_free;

// HELP_DATA		*	help_last;

void free_extra_descr( EXTRA_DESCR_DATA *pExtra );
void free_affect( AFFECT_DATA *af );

RESET_DATA *new_reset_data( void )
{
    RESET_DATA *pReset;

    if ( !reset_free )
    {
        pReset          =   (RESET_DATA*)alloc_perm( sizeof(*pReset) );
        top_reset++;
    }
    else
    {
        pReset          =   reset_free;
        reset_free      =   reset_free->next;
    }

    pReset->next        =   NULL;
    pReset->command     =   'X';
    pReset->arg1        =   0;
    pReset->arg2        =   0;
    pReset->arg3        =   0;
    pReset->arg4	=   0;

    return pReset;
}



void free_reset_data( RESET_DATA *pReset )
{
    pReset->next            = reset_free;
    reset_free              = pReset;
    return;
}

HELP_DATA *new_help( void )
{
    HELP_DATA *pHelp;

    pHelp = (HELP_DATA*)malloc(sizeof(*pHelp));
    g_num_helps++;

    pHelp->title = NULL;
    pHelp->keyword = NULL;
    pHelp->level = LEVEL_IMMORTAL;
    pHelp->text = NULL;

    return pHelp;
}

void free_help(HELP_DATA *pHelp)
{
    if (pHelp->title)
	free_string(pHelp->title);
    if (pHelp->keyword)
	free_string(pHelp->keyword);
    if (pHelp->text)
	free_string(pHelp->text);

    free(pHelp);
    g_num_helps--;
}

AREA_DATA *new_area( void )
{
    AREA_DATA *pArea;
    char buf[MAX_INPUT_LENGTH];

    if ( !area_free )
    {
        pArea   =   (AREA_DATA*)alloc_perm( sizeof(*pArea) );
        top_area++;
    }
    else
    {
        pArea       =   area_free;
        area_free   =   area_free->next;
    }

    pArea->next             =   NULL;
    pArea->name             =   str_dup( "New area" );
    pArea->area_flags       =   AREA_ADDED;
    pArea->security         =   1;
    pArea->builders         =   str_dup( "None" );
    pArea->vnums	    =  NULL;
    pArea->age              =   0;
    pArea->nplayer          =   0;
    pArea->empty            =   TRUE;              /* ROM patch */
    sprintf( buf, "area%d.are", pArea->vnum );
    pArea->file_name        =   str_dup( buf );
    pArea->vnum             =   top_area-1;

    // Weave-related    
    pArea->fount_frequency = Fount::Default;
    pArea->fount_order_bias = 0;
    pArea->fount_positive_bias = 0;

    // Shade-related
    pArea->shade_density = Shades::DefaultDensity;
    pArea->shade_power = Shades::DefaultPower;

    // Earth-related
    pArea->stone_type = -1;
    return pArea;
}

void free_area( AREA_DATA *pArea )
{
    free_string( pArea->name );
    free_string( pArea->file_name );
    free_string( pArea->builders );

    pArea->next         =   area_free->next;
    area_free           =   pArea;
    return;
}



EXIT_DATA *new_exit( void )
{
    EXIT_DATA *pExit;

    if ( !exit_free )
    {
        pExit           =   (EXIT_DATA*)alloc_perm( sizeof(*pExit) );
        top_exit++;
    }
    else
    {
        pExit           =   exit_free;
        exit_free       =   exit_free->next;
    }

    pExit->u1.to_room   =   NULL;                  /* ROM OLC */
    pExit->next         =   NULL;
/*  pExit->vnum         =   0;                        ROM OLC */
    pExit->exit_info    =   0;
    pExit->key          =   0;
    pExit->keyword      =   &str_empty[0];
    pExit->description  =   &str_empty[0];
    pExit->rs_flags     =   0;

    return pExit;
}

void free_exit( EXIT_DATA *pExit )
{
    free_string( pExit->keyword );
    free_string( pExit->description );

    pExit->next         =   exit_free;
    exit_free           =   pExit;
    return;
}

ROOM_INDEX_DATA *new_room_area( AREA_DATA *pArea )
{
    ROOM_INDEX_DATA *pRoom;
    
    pRoom = new_room_index();
    pRoom->area = pArea;
    pRoom->next_temp = pArea->temp_room;
    pArea->temp_room = pRoom;

    pRoom->next = room_index_hash[0];
    room_index_hash[0] = pRoom;

    return pRoom;
}

void free_room_area( ROOM_INDEX_DATA *pRoom )
{
    ROOM_INDEX_DATA *tRoom;

    if (pRoom == pRoom->area->temp_room)
	pRoom->area->temp_room = pRoom->next_temp;
    else
    {
    	for (tRoom = pRoom->area->temp_room; tRoom; tRoom = tRoom->next_temp)
    	{
	    if (tRoom->next_temp)
	    { 
	    	if (tRoom->next_temp == pRoom)
		{
		    tRoom->next_temp = pRoom->next_temp;
		    break;
	        }
 	    }
	    else
	    {
	    	bug("free_area_room: can't locate temporary room", 0);
	        break;
	    }
        }
    }

    if (pRoom == room_index_hash[0])
	room_index_hash[0] = pRoom->next;
    else
    {
    	for (tRoom = room_index_hash[0]; tRoom; tRoom = tRoom->next)
    	{
	    if (tRoom->next)
	    { 
	    	if (tRoom->next == pRoom)
	    	{
		    tRoom->next = pRoom->next;
		    break;
	        }
 	    }
	    else
	    {
	    	bug("free_area_room: can't locate temporary room", 0);
	        break;
	    }
        }
    }

    free_room_index(pRoom);
}

ROOM_INDEX_DATA *new_room_index( void )
{
    TRACK_DATA *tracks;
    ROOM_INDEX_DATA *pRoom;
    int door;

    if ( !room_index_free )
    {
        pRoom           =   (ROOM_INDEX_DATA*)alloc_perm( sizeof(*pRoom) );
        top_room++;
    }
    else
    {
        pRoom           =   room_index_free;
        room_index_free =   room_index_free->next;
    }

    pRoom->next             =   NULL;
    pRoom->next_temp	    =   NULL;
    pRoom->people           =   NULL;
    pRoom->contents         =   NULL;
    pRoom->extra_descr      =   NULL;
    pRoom->area             =   NULL;
    memset(pRoom->roomvalue, 0, sizeof(pRoom->roomvalue));
    memset(pRoom->stringvalue, 0, sizeof(pRoom->stringvalue));
    
    for ( door=0; door < MAX_DIR; door++ )
        pRoom->exit[door]   =   NULL;

    pRoom->name             =   &str_empty[0];
    pRoom->description      =   &str_empty[0];
    pRoom->vnum             =   0;
    pRoom->room_flags       =   0;
    pRoom->light            =   0;
    pRoom->sector_type      =   0;
    pRoom->clan		    =	0;
    pRoom->heal_rate	    =   100;
    pRoom->move_rate	    =   100;
    pRoom->mana_rate	    =   100;
	pRoom->gods_altar		=	0;
    tracks = (TRACK_DATA*)alloc_perm(sizeof(*tracks));
    g_num_tracks++;
    pRoom->tracks = tracks;
    pRoom->tracks->next = NULL;
    pRoom->tracks->ch = NULL;
    pRoom->tracks->direction = 0;                       

    pRoom->ley_group = NULL;
    pRoom->fount_frequency = Fount::Default;
    pRoom->fount_order_power = 0;
    pRoom->fount_positive_power = 0;

    pRoom->shade_density = Shades::DefaultDensity;
    pRoom->shade_power = Shades::DefaultPower;

    pRoom->stone_type = -1;

    pRoom->arrived_from = NULL;
    pRoom->arrived_from_door = 0;
    pRoom->path_generation_id = 0;
    return pRoom;
}



void free_room_index( ROOM_INDEX_DATA *pRoom )
{
    int door;
    EXTRA_DESCR_DATA *pExtra;
    RESET_DATA *pReset;
    AFFECT_DATA *paf;
    OBJ_DATA *pObj, *obj_next;

    free_string( pRoom->name );
    free_string( pRoom->description );
    for (int i(0); i < MAX_MOBVALUE; ++i)
        free_string(pRoom->stringvalue[0]);

    for (paf = pRoom->affected; paf; paf = paf->next)
	    free_affect(paf);
    pRoom->affected = NULL;

    for (pObj = pRoom->contents; pObj; pObj = obj_next)
    {
    	obj_next = pObj->next_content;
    	extract_obj(pObj);
    }
    pRoom->contents = NULL;

    for ( door = 0; door < MAX_DIR; door++ )
    {
        if ( pRoom->exit[door] )
            free_exit( pRoom->exit[door] );
    }

    delete pRoom->ley_group;
    pRoom->ley_group = NULL;

    for ( pExtra = pRoom->extra_descr; pExtra; pExtra = pExtra->next )
        free_extra_descr( pExtra );

    for ( pReset = pRoom->reset_first; pReset; pReset = pReset->next )
        free_reset_data( pReset );

    pRoom->next     =   room_index_free;
    room_index_free =   pRoom;
}

extern AFFECT_DATA *affect_free;

char *get_word( FILE *fp )
{
char c;
char *word;
char buf[1024];
int x = 0;

if (feof(fp))
  return NULL;

do {
    c = fgetc(fp);
    buf[x++] = c;
 } while (!feof(fp) && c != '\n');

buf[--x] = (char)0;
if ((word = (char*)malloc(sizeof(char)*x+1)) == NULL)
  bug("get_word malloc failed", 0);
strcpy(word, buf);
return word;
}

SHOP_DATA *new_shop( void )
{
    SHOP_DATA *pShop;
    int buy;

    if ( !shop_free )
    {
        pShop           =   (SHOP_DATA*)alloc_perm( sizeof(*pShop) );
        top_shop++;
    }
    else
    {
        pShop           =   shop_free;
        shop_free       =   shop_free->next;
    }

    pShop->next         =   NULL;
    pShop->keeper       =   0;

    for ( buy=0; buy<MAX_TRADE; buy++ )
        pShop->buy_type[buy]    =   0;

    pShop->profit_buy   =   100;
    pShop->profit_sell  =   100;
    pShop->open_hour    =   0;
    pShop->close_hour   =   23;
    pShop->max_buy	=   0;

    return pShop;
}



void free_shop( SHOP_DATA *pShop )
{
    pShop->next = shop_free;
    shop_free   = pShop;
    return;
}



OBJ_INDEX_DATA *new_obj_index( void )
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if ( !obj_index_free )
    {
        pObj           =   (OBJ_INDEX_DATA*)alloc_perm( sizeof(*pObj) );
        top_obj_index++;
    }
    else
    {
        pObj            =   obj_index_free;
        obj_index_free  =   obj_index_free->next;
    }

    pObj->next          =   NULL;
    pObj->extra_descr   =   NULL;
    pObj->affected      =   NULL;
    pObj->area          =   NULL;
    pObj->objprogs	=   NULL;
    pObj->name          =   str_dup( "no name" );
    pObj->short_descr   =   str_dup( "(no short description)" );
    pObj->description   =   str_dup( "(no description)" );
    pObj->obj_str	=   &str_empty[0];
    pObj->vnum          =   0;
    pObj->item_type     =   ITEM_TRASH;
    pObj->extra_flags[0]=   0;
    pObj->extra_flags[1]=   0;
    pObj->wear_flags    =   0;
    pObj->count         =   0;
    pObj->weight        =   0;
    pObj->cost          =   0;
    pObj->limit_factor  =   0;
    pObj->limit         =   0;
    pObj->current       =   0;
    pObj->material      =   MATERIAL_UNKNOWN;      /* ROM */
    pObj->condition     =   100;                        /* ROM */
    for ( value = 0; value < 5; value++ )               /* 5 - ROM */
        pObj->value[value]  =   0;

    pObj->new_format    = TRUE; /* ROM */

    return pObj;
}



void free_obj_index( OBJ_INDEX_DATA *pObj )
{
    EXTRA_DESCR_DATA *pExtra;
    AFFECT_DATA *pAf;

    free_string( pObj->name );
    free_string( pObj->short_descr );
    free_string( pObj->description );

    for ( pAf = pObj->affected; pAf; pAf = pAf->next )
    {
        free_affect( pAf );
    }

    for ( pExtra = pObj->extra_descr; pExtra; pExtra = pExtra->next )
    {
        free_extra_descr( pExtra );
    }
    
    pObj->next              = obj_index_free;
    obj_index_free          = pObj;
    return;
}



MOB_INDEX_DATA *new_mob_index( void )
{
    MOB_INDEX_DATA *pMob;

    if ( !mob_index_free )
    {
        pMob           =   (MOB_INDEX_DATA*)alloc_perm( sizeof(*pMob) );
        top_mob_index++;
    }
    else
    {
        pMob            =   mob_index_free;
        mob_index_free  =   mob_index_free->next;
    }

    pMob->next          =   NULL;
    pMob->spec_fun      =   NULL;
    pMob->pShop         =   NULL;
    pMob->area          =   NULL;
    pMob->mount_data	=   NULL;
    pMob->player_name   =   str_dup( "no name" );
    pMob->short_descr   =   str_dup( "(no short description)" );
    pMob->long_descr    =   str_dup( "(no long description)\n\r" );
    pMob->description   =   &str_empty[0];
    pMob->dam_verb	=   &str_empty[0];
    pMob->vnum          =   0;
    pMob->count         =   0;
    pMob->killed        =   0;
    pMob->sex           =   0;
    pMob->level         =   0;
    pMob->act           =   ACT_IS_NPC;
    pMob->affected_by   =   0;
    pMob->alignment     =   1;
    pMob->hitroll	=   0;
    pMob->race          =   global_int_race_human; /* - Hugin */
    pMob->form          =   0;           /* ROM patch -- Hugin */
    pMob->parts         =   0;           /* ROM patch -- Hugin */
    pMob->imm_flags     =   0;           /* ROM patch -- Hugin */
    pMob->res_flags     =   0;           /* ROM patch -- Hugin */
    pMob->vuln_flags    =   0;           /* ROM patch -- Hugin */
    pMob->material      =   MATERIAL_FLESH; /* -- Hugin */
    pMob->off_flags     =   0;           /* ROM patch -- Hugin */
    pMob->size          =   SIZE_MEDIUM; /* ROM patch -- Hugin */
    pMob->ac[AC_PIERCE]	=   0;           /* ROM patch -- Hugin */
    pMob->ac[AC_BASH]	=   0;           /* ROM patch -- Hugin */
    pMob->ac[AC_SLASH]	=   0;           /* ROM patch -- Hugin */
    pMob->ac[AC_EXOTIC]	=   0;           /* ROM patch -- Hugin */
    pMob->hit[DICE_NUMBER]	=   0;   /* ROM patch -- Hugin */
    pMob->hit[DICE_TYPE]	=   0;   /* ROM patch -- Hugin */
    pMob->hit[DICE_BONUS]	=   0;   /* ROM patch -- Hugin */
    pMob->mana[DICE_NUMBER]	=   0;   /* ROM patch -- Hugin */
    pMob->mana[DICE_TYPE]	=   0;   /* ROM patch -- Hugin */
    pMob->mana[DICE_BONUS]	=   0;   /* ROM patch -- Hugin */
    pMob->damage[DICE_NUMBER]	=   0;   /* ROM patch -- Hugin */
    pMob->damage[DICE_TYPE]	=   0;   /* ROM patch -- Hugin */
    pMob->damage[DICE_NUMBER]	=   0;   /* ROM patch -- Hugin */
    pMob->start_pos             =   POS_STANDING; /*  -- Hugin */
    pMob->default_pos           =   POS_STANDING; /*  -- Hugin */
    pMob->wealth                =   0;
    pMob->factionNumber         = Faction::None;

    pMob->new_format            = TRUE;  /* ROM */

    return pMob;
}



void free_mob_index( MOB_INDEX_DATA *pMob )
{
    free_string( pMob->player_name );
    free_string( pMob->short_descr );
    free_string( pMob->long_descr );
    free_string( pMob->description );

    free_shop( pMob->pShop );

    pMob->next              = mob_index_free;
    mob_index_free          = pMob;
    return;
}

ACCOUNT_DATA *new_acct( void )
{
    ACCOUNT_DATA *acct = (ACCOUNT_DATA*)alloc_perm(sizeof(*acct));
    g_num_accts++;

    acct->name = str_dup("");
    acct->chars = str_dup("");
    acct->pwd = str_dup("");
    acct->email = str_dup("");
    acct->flags = 0;
    acct->deleted = str_dup("");
    acct->immrecord = str_dup("");
    acct->socket_info = str_dup("");
    acct->def_prompt = str_dup("");
    acct->award_points = 0;

    return acct;
}

