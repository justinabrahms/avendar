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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <cassert>
#include <mysql/mysql.h>
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#if defined(unix)
#include <sys/time.h>
#else
#include <time.h>
#endif

#include "merc.h"
#include "db.h"
#include "lookup.h"
#include "interp.h"

/* local function declaration */
long		obj_default_price	args((OBJ_INDEX_DATA *pObjIndex));

extern		MYSQL		mysql;

/* values for db2.c */
struct		social_type	social_table		[MAX_SOCIALS];
int		social_count;

/* snarf a socials file */
void load_socials( FILE *fp)
{
    for ( ; ; ) 
    {
    	struct social_type social;
    	char *temp;
        /* clear social */
	social.char_no_arg = NULL;
	social.others_no_arg = NULL;
	social.char_found = NULL;
	social.others_found = NULL;
	social.vict_found = NULL; 
	social.char_not_found = NULL;
	social.char_auto = NULL;
	social.others_auto = NULL;

    	temp = fread_word(fp);
    	if (!strcmp(temp,"#0"))
	    return;  /* done */
#if defined(social_debug) 
	else 
	    fprintf(stderr,"%s\n\r",temp);
#endif

    	strcpy(social.name,temp);
    	fread_to_eol(fp);

	temp = fread_string_eol(fp);
	if (!strcmp(temp,"$"))
	     social.char_no_arg = NULL;
	else if (!strcmp(temp,"#"))
	{
	     social_table[social_count] = social;
	     social_count++;
	     continue; 
	}
        else
	    social.char_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.others_no_arg = NULL;
        else if (!strcmp(temp,"#"))
        {
	     social_table[social_count] = social;
             social_count++;
             continue;
        }
        else
	    social.others_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.char_found = NULL;
        else if (!strcmp(temp,"#"))
        {
	     social_table[social_count] = social;
             social_count++;
             continue;
        }
       	else
	    social.char_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.others_found = NULL;
        else if (!strcmp(temp,"#"))
        {
	     social_table[social_count] = social;
             social_count++;
             continue;
        }
        else
	    social.others_found = temp; 

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.vict_found = NULL;
        else if (!strcmp(temp,"#"))
        {
	     social_table[social_count] = social;
             social_count++;
             continue;
        }
        else
	    social.vict_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.char_not_found = NULL;
        else if (!strcmp(temp,"#"))
        {
	     social_table[social_count] = social;
             social_count++;
             continue;
        }
        else
	    social.char_not_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.char_auto = NULL;
        else if (!strcmp(temp,"#"))
        {
	     social_table[social_count] = social;
             social_count++;
             continue;
        }
        else
	    social.char_auto = temp;
         
        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.others_auto = NULL;
        else if (!strcmp(temp,"#"))
        {
             social_table[social_count] = social;
             social_count++;
             continue;
        }
        else
	    social.others_auto = temp; 
	
	social_table[social_count] = social;
    	social_count++;
   }
   return;
}
    





/*
 * Snarf a mob section.  new style
 */
void load_mobiles( FILE *fp )
{
    MOB_INDEX_DATA *pMobIndex;
    char *ctemp;
 
    if ( !area_last )   /* OLC */
    {
        bug( "Load_mobiles: no #AREA seen yet.", 0 );
        exit( 1 );
    }

    for ( ; ; )
    {
        int vnum;
	int mobver = 1;
        char letter;
        int iHash;
 
        letter                          = fread_letter( fp );
        if ((letter != '#') && (letter != 'V'))
        {
            bug( "Load_mobiles: '#' or 'V' not found.", 0 );
            exit( 1 );
        }
 
        if (letter == 'V')
	    mobver			= fread_number( fp );
 
        vnum                            = fread_number( fp );
        if ( vnum == 0 )
            break;
 
        fBootDb = FALSE;
        if ( get_mob_index( vnum ) != NULL )
        {
            bug( "Load_mobiles: vnum %d duplicated.", vnum );
            exit( 1 );
        }
        fBootDb = TRUE;
 
        pMobIndex                       = (MOB_INDEX_DATA*)alloc_perm( sizeof(*pMobIndex) );
        pMobIndex->vnum                 = vnum;
        pMobIndex->area                 = area_last;               /* OLC */
	pMobIndex->new_format		= TRUE;
        pMobIndex->count		= 0;
        pMobIndex->total_count		= 0;
	newmobs++;
        pMobIndex->player_name          = fread_string( fp );
        pMobIndex->short_descr          = fread_string( fp );
        pMobIndex->long_descr           = fread_string( fp );
        pMobIndex->description          = fread_string( fp );
	pMobIndex->race		 	= race_lookup(fread_string( fp ));

	if (IS_SET(pMobIndex->area->ainfo_flags, AINFO_MOBCLASS))
	    pMobIndex->class_num		= class_lookup(fread_string( fp ));
 
        pMobIndex->long_descr[0]        = UPPER(pMobIndex->long_descr[0]);
        pMobIndex->description[0]       = UPPER(pMobIndex->description[0]);
 
        pMobIndex->act                  = fread_flag( fp ) | ACT_IS_NPC
					| race_table[pMobIndex->race].act;
	pMobIndex->nact			= fread_flag( fp );  
        pMobIndex->affected_by          = fread_flag( fp )
					| race_table[pMobIndex->race].aff;
        pMobIndex->pShop                = NULL;
 //     pMobIndex->alignment            = fread_number( fp );
	ctemp	= fread_word( fp );

	if (is_number(ctemp))  /* Support for old format */
	{
	    int i = atoi(ctemp);
	    if (i < 0)
		pMobIndex->alignment = ALIGN_EVIL;
	    else if (i > 0)
		pMobIndex->alignment = ALIGN_GOOD;
	    else
		pMobIndex->alignment = ALIGN_NEUTRAL;
	}
	else
	{
	    switch(UPPER(*ctemp))
	    {
		case 'E':
		    pMobIndex->alignment = ALIGN_EVIL;
		    break;
		case 'N':
		    pMobIndex->alignment = ALIGN_NEUTRAL;
		    break;
		case 'G':
		    pMobIndex->alignment = ALIGN_GOOD;
		    break;
		case 'R':
		    pMobIndex->alignment = ALIGN_RANDOM;
		    break;
		default:
		    bug ("Bad alignment, mob index %d.", pMobIndex->vnum);
		    pMobIndex->alignment = ALIGN_NEUTRAL;
		    break;
	    }
	}

        pMobIndex->group                = fread_number( fp );

        pMobIndex->level                = fread_number( fp );
        pMobIndex->hitroll              = fread_number( fp );  

	/* read hit dice */
        pMobIndex->hit[DICE_NUMBER]     = fread_number( fp );  
        /* 'd'          */                fread_letter( fp ); 
        pMobIndex->hit[DICE_TYPE]   	= fread_number( fp );
        /* '+'          */                fread_letter( fp );   
        pMobIndex->hit[DICE_BONUS]      = fread_number( fp ); 

 	/* read mana dice */
	pMobIndex->mana[DICE_NUMBER]	= fread_number( fp );
					  fread_letter( fp );
	pMobIndex->mana[DICE_TYPE]	= fread_number( fp );
					  fread_letter( fp );
	pMobIndex->mana[DICE_BONUS]	= fread_number( fp );

	/* read damage dice */
	pMobIndex->damage[DICE_NUMBER]	= fread_number( fp );
					  fread_letter( fp );
	pMobIndex->damage[DICE_TYPE]	= fread_number( fp );
					  fread_letter( fp );
	pMobIndex->damage[DICE_BONUS]	= fread_number( fp );

	if (mobver == 1)
	{
	     int oldtype = attack_lookup(fread_word(fp));
	     pMobIndex->dam_type = attack_table[oldtype].damage;
	     pMobIndex->dam_verb = str_dup(attack_table[oldtype].noun);
	}
	else
	{
	     pMobIndex->dam_type = damtype_lookup(fread_string(fp));
	     pMobIndex->dam_verb = fread_string(fp);
	}

	/* read armor class_num */
	pMobIndex->ac[AC_PIERCE]	= fread_number( fp ) * 10;
	pMobIndex->ac[AC_BASH]		= fread_number( fp ) * 10;
	pMobIndex->ac[AC_SLASH]		= fread_number( fp ) * 10;
	pMobIndex->ac[AC_EXOTIC]	= fread_number( fp ) * 10;

	/* read flags and add in data from the race table */
	pMobIndex->off_flags		= fread_flag( fp ) 
					| race_table[pMobIndex->race].off;
	pMobIndex->imm_flags		= fread_flag( fp )
					| race_table[pMobIndex->race].imm;

        if (!IS_SET(pMobIndex->area->ainfo_flags, AINFO_NEWRES))
        {
	    long curres, curvuln;
	    int i;

	    pMobIndex->res_flags	= fread_flag( fp )
	    				| race_table[pMobIndex->race].res;
	    pMobIndex->vuln_flags	= fread_flag( fp )
					| race_table[pMobIndex->race].vuln;

            curres = pMobIndex->res_flags;
            curvuln = pMobIndex->vuln_flags;

            for (i = 0; i < MAX_RESIST; i++)
            {

                if ((curres % 2) == 1)
                    pMobIndex->resist[i] += PERC_RES;
                if ((curvuln % 2) == 1)
                    pMobIndex->resist[i] += PERC_VULN;

                curres = curres >> 1;
                curvuln = curvuln >> 1;

                if ((curres == 0) && (curvuln == 0))
                    break;
            }
	}
	else
	{
	    int maxres, numres, i;
		
	    maxres = fread_number( fp );
	    numres = UMIN(MAX_RESIST, maxres);
	    for (i = 0; i < numres; i++)
		pMobIndex->resist[i] = fread_number( fp );
	
	    if (maxres > MAX_RESIST)
		fread_to_eol( fp );
	}	

	if (IS_SET(pMobIndex->area->ainfo_flags, AINFO_AVNUMS))
	{
	    int i, maxvnum, numvnum;

	    maxvnum = fread_number( fp );
	    numvnum = UMIN(MAX_ASSIST_VNUM, maxvnum);
	    for (i = 0; i < numvnum; i++)
		pMobIndex->assist_vnum[i] =fread_number(fp);

	    if (MAX_ASSIST_VNUM < maxvnum)
		fread_to_eol( fp );
	}

	/* vital statistics */
	pMobIndex->start_pos		= position_lookup(fread_word(fp));
	pMobIndex->default_pos		= position_lookup(fread_word(fp));
	pMobIndex->sex			= sex_lookup(fread_word(fp));

	pMobIndex->wealth		= fread_number( fp );

	if (mobver >= 4)
	    pMobIndex->factionNumber	= fread_number( fp );

	pMobIndex->form			= fread_flag( fp )
					| race_table[pMobIndex->race].form;
	pMobIndex->parts		= fread_flag( fp )
					| race_table[pMobIndex->race].parts;

	pMobIndex->lang_flags		= fread_flag( fp );

	/* size */
	pMobIndex->size			= size_lookup(fread_word(fp));
	pMobIndex->material		= material_lookup(fread_word( fp ));
	pMobIndex->gm			= NULL;
 
	for ( ; ; )
        {
            letter = fread_letter( fp );

            if (letter == 'F')
            {
		char *word;
		long vector;

                word                    = fread_word(fp);
		vector			= fread_flag(fp);

		if (!str_prefix(word,"act"))
		    REMOVE_BIT(pMobIndex->act,vector);
                else if (!str_prefix(word,"aff"))
		    REMOVE_BIT(pMobIndex->affected_by,vector);
		else if (!str_prefix(word,"off"))
		    REMOVE_BIT(pMobIndex->off_flags,vector);
		else if (!str_prefix(word,"imm"))
		    REMOVE_BIT(pMobIndex->imm_flags,vector);
		else if (!str_prefix(word,"res"))
		    REMOVE_BIT(pMobIndex->res_flags,vector);
		else if (!str_prefix(word,"vul"))
		    REMOVE_BIT(pMobIndex->vuln_flags,vector);
		else if (!str_prefix(word,"for"))
		    REMOVE_BIT(pMobIndex->form,vector);
		else if (!str_prefix(word,"par"))
		    REMOVE_BIT(pMobIndex->parts,vector);
		else
		{
		    bug("Flag remove: flag not found.",0);
		    exit(1);
		}
	     }
	     else
	     {
		ungetc(letter,fp);
		break;
	     }
	}


        letter = fread_letter( fp );
        if ( letter == '>' )
        {
          ungetc( letter, fp );
          mprog_read_programs( fp, pMobIndex );
        }
        else ungetc( letter,fp );

	if (mobver <= 2)
	{
	    if (pMobIndex->wealth != 0)
		pMobIndex->wealth = 100;

	    if ((pMobIndex->level >= 6) && (pMobIndex->level <= 50))
	    {
		pMobIndex->hit[DICE_NUMBER] = (int) (pMobIndex->hit[DICE_NUMBER] * pow(2, (1.0 - (pMobIndex->level / 50.0))));
		pMobIndex->hit[DICE_BONUS] = (int) (pMobIndex->hit[DICE_BONUS] * pow(2, (1.0 - (pMobIndex->level / 50.0))));
	    }
	}

        iHash                   = vnum % MAX_KEY_HASH;
        pMobIndex->next         = mob_index_hash[iHash];
        mob_index_hash[iHash]   = pMobIndex;
        top_mob_index++;
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;  /* OLC */
//        assign_area_vnum( vnum );                                  /* OLC */
        kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL-1)].number++;
    }
 
    return;
}

/*
 * Snarf an obj section. new style
 */
void load_objects( FILE *fp )
{
    OBJ_INDEX_DATA *pObjIndex;
 
    if ( !area_last )   /* OLC */
    {
        bug( "Load_objects: no #AREA seen yet.", 0 );
        exit( 1 );
    }

    for ( ; ; )
    {
        int vnum;
        int objver = 1;
        char letter;
        int oldtype;
        int iHash;

/** This, my friends, it what we call a hack, to make up for the laughable
    lack of foresight with the original ROM code into adding additional
    fields into the basic object index data **/
 
        letter                          = fread_letter( fp );
        if ((letter != '#') && (letter != 'V'))
        {
            bug( "Load_objects: '#' or 'V' not found.", 0 );
            exit( 1 );
        }
 
        if (letter == 'V')
	    objver			= fread_number( fp );

        vnum                            = fread_number( fp );
        if ( vnum == 0 )
            break;
 
        fBootDb = FALSE;
        if ( get_obj_index( vnum ) != NULL )
        {
            bug( "Load_objects: vnum %d duplicated.", vnum );
            exit( 1 );
        }
        fBootDb = TRUE;
 
        pObjIndex                       = (OBJ_INDEX_DATA*)alloc_perm( sizeof(*pObjIndex) );
        pObjIndex->vnum                 = vnum;
        pObjIndex->area                 = area_last;            /* OLC */
        pObjIndex->new_format           = TRUE;
	pObjIndex->reset_num		= 0;
	newobjs++;
        pObjIndex->name                 = fread_string( fp );
        pObjIndex->short_descr          = fread_string( fp );
        pObjIndex->description          = fread_string( fp );

	if (objver >= 2)
	    pObjIndex->lore		= fread_string( fp );
	
        pObjIndex->material		= material_lookup(fread_string( fp ));
 
        pObjIndex->item_type            = item_lookup(fread_word( fp ));
        pObjIndex->extra_flags[0]       = fread_flag( fp );

	if (objver >= 12)
	    pObjIndex->extra_flags[1]   = fread_flag( fp );

    if (objver >= 13)
        pObjIndex->extra_flags[2]   = fread_flag(fp);

        pObjIndex->wear_flags           = fread_flag( fp );
	switch(pObjIndex->item_type)
	{
	case ITEM_WEAPON:
	    pObjIndex->value[0]		= weapon_type(fread_word(fp));
	    pObjIndex->value[1]		= fread_number(fp);
	    pObjIndex->value[2]		= fread_number(fp);

	    if (objver <= 3)
	    {
	   	oldtype			= attack_lookup(fread_word(fp));
		pObjIndex->obj_str	= str_dup(attack_table[oldtype].noun);
		pObjIndex->value[3]	= attack_table[oldtype].damage;
	    }
	    else
		pObjIndex->value[3]	= fread_number(fp);

	    pObjIndex->value[4]		= fread_flag(fp);

	    if (objver >= 4)
		pObjIndex->obj_str	= fread_string(fp);
	
	    break;
	case ITEM_ARROW:
	    if (objver <= 3)
	    {
	   	oldtype			= attack_lookup(fread_word(fp));
		pObjIndex->obj_str	= str_dup(attack_table[oldtype].noun);
		pObjIndex->value[0]	= attack_table[oldtype].damage;
	    }
	    else
	        pObjIndex->value[0]	= fread_number(fp);

            pObjIndex->value[1]         = fread_number(fp);
	    pObjIndex->value[2]         = fread_number(fp);
	    pObjIndex->value[3]         = fread_number(fp);
	    pObjIndex->value[4]         = fread_number(fp);

	    if (objver >= 4)
		pObjIndex->obj_str	= fread_string(fp);

	    break;
	case ITEM_CONTAINER:
	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= fread_flag(fp);
	    pObjIndex->value[2]		= fread_number(fp);
	    pObjIndex->value[3]		= fread_number(fp);
	    pObjIndex->value[4]		= fread_number(fp);
	    break;
        case ITEM_DRINK_CON:
	case ITEM_FOUNTAIN:
            pObjIndex->value[0]         = fread_number(fp);
            pObjIndex->value[1]         = fread_number(fp);
            pObjIndex->value[2]         = liq_lookup(fread_word(fp));
            pObjIndex->value[3]         = fread_number(fp);
            pObjIndex->value[4]         = fread_number(fp);
            break;
	case ITEM_WAND:
	case ITEM_STAFF:
	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= fread_number(fp);
	    pObjIndex->value[2]		= fread_number(fp);
	    pObjIndex->value[3]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[4]		= fread_number(fp);
	    break;
	case ITEM_POTION:
	case ITEM_PILL:
	case ITEM_SCROLL:
	case ITEM_OIL:
 	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[2]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[3]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[4]		= skill_lookup(fread_word(fp));
	    break;
	default:
            pObjIndex->value[0]             = fread_flag( fp );
            pObjIndex->value[1]             = fread_flag( fp );
            pObjIndex->value[2]             = fread_flag( fp );
            pObjIndex->value[3]             = fread_flag( fp );
	    pObjIndex->value[4]		    = fread_flag( fp );
	    break;
	}
	pObjIndex->level		= fread_number( fp );
        pObjIndex->weight               = fread_number( fp );
        pObjIndex->cost                 = fread_number( fp );

	if (objver < 5)
	{
	    pObjIndex->cost /= 100;

	    if (pObjIndex->cost > 500)
	        pObjIndex->cost = 500 + ((pObjIndex->cost - 500) / 10);
        }

	if ((objver < 13) && (pObjIndex->item_type == ITEM_FOOD))
	    pObjIndex->value[1] = 50;
	
	if (objver >= 3)
	    pObjIndex->size		= fread_number( fp );
	else
	{
	    switch (pObjIndex->item_type)
	    {

		case ITEM_TREASURE:
		case ITEM_ARMOR:
		case ITEM_CLOTHING:
		case ITEM_JEWELRY:
		    if ((IS_SET(pObjIndex->wear_flags, ITEM_WEAR_BODY))
		     || (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_HEAD))
		     || (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_LEGS))
		     || (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_FEET))
		     || (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_ARMS))
		     || (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_SHIELD))
		     || (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_ABOUT))
		     || (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_WAIST))
		     || (IS_SET(pObjIndex->wear_flags, ITEM_WIELD))
		     || (IS_SET(pObjIndex->wear_flags, ITEM_HOLD)))
			pObjIndex->size = SIZE_MEDIUM;
		    else if ((IS_SET(pObjIndex->wear_flags, ITEM_WEAR_NECK))
			  || (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_HANDS))
			  || (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_WRIST)))
			pObjIndex->size = SIZE_SMALL;
		    else
			pObjIndex->size = SIZE_TINY;
		    break;

		case ITEM_KEY:
		case ITEM_MONEY:
		case ITEM_PILL:
		case ITEM_GEM:
		case ITEM_ROOM_KEY:
		    pObjIndex->size = SIZE_TINY;
		    break;

		case ITEM_LIGHT:
		case ITEM_SCROLL:
		case ITEM_WAND:
		case ITEM_POTION:
		case ITEM_OIL:
		case ITEM_DRINK_CON:
		case ITEM_FOOD:
		case ITEM_MAP:
		case ITEM_ARROW:
		case ITEM_SPECIAL:
		    pObjIndex->size = SIZE_SMALL;
		    break;

		case ITEM_INSTRUMENT:
		case ITEM_WARP_STONE:
		case ITEM_PROTECT:
		case ITEM_TRASH:
		    pObjIndex->size = SIZE_MEDIUM;
		    break;

		case ITEM_STAFF:
		case ITEM_BOW:
		case ITEM_NET:
		    pObjIndex->size = SIZE_LARGE;
		    break;

		case ITEM_CORPSE_NPC:
		case ITEM_CORPSE_PC:
		case ITEM_FURNITURE:
		case ITEM_BOAT:
		case ITEM_JUKEBOX:
		case ITEM_PORTAL:
		    pObjIndex->size = SIZE_HUGE;
		    break;

		case ITEM_FOUNTAIN:
		    pObjIndex->size = SIZE_GIANT;
		    break;

		case ITEM_CONTAINER:
		    if (IS_SET(pObjIndex->wear_flags, ITEM_TAKE))
			pObjIndex->size = SIZE_MEDIUM;
		    else
			pObjIndex->size = SIZE_HUGE;
		    break;

		case ITEM_WEAPON:
		    switch (pObjIndex->value[0])
		    {
			case WEAPON_DAGGER:
			case WEAPON_KNIFE:
			    pObjIndex->size = SIZE_SMALL;
			    break;
	
			case WEAPON_EXOTIC:
			case WEAPON_SWORD:
			case WEAPON_MACE:
			case WEAPON_AXE:
			case WEAPON_FLAIL:
			case WEAPON_WHIP:
			    pObjIndex->size = SIZE_MEDIUM;
			    break;

			case WEAPON_SPEAR:
			case WEAPON_POLEARM:
			case WEAPON_STAFF:
			    pObjIndex->size = SIZE_LARGE;
		    }
		    break;
	    }
	}

        /* condition */
        letter 				= fread_letter( fp );
	switch (letter)
 	{
	    case ('P') :		pObjIndex->condition = 100; break;
	    case ('G') :		pObjIndex->condition =  90; break;
	    case ('A') :		pObjIndex->condition =  75; break;
	    case ('W') :		pObjIndex->condition =  50; break;
	    case ('D') :		pObjIndex->condition =  25; break;
	    case ('B') :		pObjIndex->condition =  10; break;
	    case ('R') :		pObjIndex->condition =   0; break;
	    default:			pObjIndex->condition = 100; break;
	}
	/* Object Limits */
	pObjIndex->limit_factor		= fread_number( fp );
	pObjIndex->limit		= 0;
 
        for ( ; ; )
        {
            char letter;
 
            letter = fread_letter( fp );
 
            if ( letter == 'A' )
            {
                AFFECT_DATA *paf;
 
                paf                     = (AFFECT_DATA*)alloc_perm( sizeof(*paf) );
		paf->where		= TO_OBJECT;
                paf->type               = -1;
                paf->level              = pObjIndex->level;
                paf->duration           = -1;
                paf->location           = fread_number( fp );
                paf->modifier           = fread_number( fp );
                paf->bitvector          = 0;
                paf->next               = pObjIndex->affected;
                pObjIndex->affected     = paf;
                top_affect++;
            }

            else if (letter == 'F')
            {
                AFFECT_DATA *paf;
 
                paf                     = (AFFECT_DATA*)alloc_perm( sizeof(*paf) );
                letter 			= fread_letter(fp);
                switch (letter)
                {
                    case 'A':
                                paf->where          = TO_AFFECTS;
                        break;
                    case 'I':
                        paf->where		= TO_IMMUNE;
                        break;
                    case 'R':
                        paf->where		= TO_RESIST;
                        break;
                    case 'V':
                        paf->where		= TO_VULN;
                        break;
                    default:
                                bug( "Load_objects: Bad where on flag set.", 0 );
                               exit( 1 );
                }
                paf->type               = -1;
                paf->level              = pObjIndex->level;
                paf->duration           = -1;
                paf->location           = fread_number(fp);
                paf->modifier           = fread_number(fp);
                paf->bitvector          = fread_flag(fp);
                paf->next               = pObjIndex->affected;
                pObjIndex->affected     = paf;
                top_affect++;
            }
 
            else if ( letter == 'E' )
            {
                EXTRA_DESCR_DATA *ed;
 
                ed                      = (EXTRA_DESCR_DATA*)alloc_perm( sizeof(*ed) );
                ed->keyword             = fread_string( fp );
                ed->description         = fread_string( fp );
                ed->next                = pObjIndex->extra_descr;
                pObjIndex->extra_descr  = ed;
                top_ed++;
            }
            else if (letter == 'O')
                pObjIndex->obj_str = fread_string(fp);
            else
            {
                ungetc( letter, fp );
                break;
            }
        }

	// Put down here because we need all affect data and what not to calculate.
	if (objver < 7)
	{
	    pObjIndex->cost = obj_default_price(pObjIndex);
	}

	if (objver < 8)
	{
	    AFFECT_DATA *paf;

	    for (paf = pObjIndex->affected; paf; paf = paf->next)
	    {
		if (((paf->location >= APPLY_RESIST_SUMMON) && (paf->location <= APPLY_RESIST_IRON))
		 || (((paf->location == APPLY_HIT) || (paf->location == APPLY_MANA) || (paf->location == APPLY_MOVE)) && paf->modifier >= 29))
		{
	    	    pObjIndex->cost = obj_default_price(pObjIndex);
		    break;
		}
	    }
	}

	if (objver < 9)
	{
	    AFFECT_DATA *paf;

	    for (paf = pObjIndex->affected; paf; paf = paf->next)
	    {
		if ( (((paf->location == APPLY_SAVES) || (paf->location == APPLY_AC)) && (paf->modifier >= 0))
		 || (((paf->location != APPLY_SAVES) && (paf->location != APPLY_AC)) && (paf->modifier <= 0)))
		{
		    pObjIndex->cost = obj_default_price(pObjIndex);
		    break;
		}
	    }
	}

	if (objver < 10)
	{
	    AFFECT_DATA *paf;
	    int wavg = (1 + pObjIndex->value[2]) * pObjIndex->value[1] / 2;
	    int ne = 0;

	    if ((pObjIndex->item_type == ITEM_ARMOR) || ((pObjIndex->item_type == ITEM_WEAPON) && (wavg >= 22) && (wavg <= 24)))
		pObjIndex->cost = obj_default_price(pObjIndex);
	    else
	    {
	        for (paf = pObjIndex->affected; paf; paf = paf->next)
	        {
		    ne++;

		    if ((ne >= 2) || (paf->location == APPLY_AC))
		    {
			pObjIndex->cost = obj_default_price(pObjIndex);
			break;
		    }
		}
	    }
	}

	if (objver < 11)
	{
	    if ((pObjIndex->item_type == ITEM_ARMOR)
	     || (pObjIndex->item_type == ITEM_WEAPON)
	     || (pObjIndex->item_type == ITEM_BOW))
		pObjIndex->cost /= 2;
	}
	

        letter = fread_letter( fp );
        if ( letter == '>' )
        {
          ungetc( letter, fp );
          oprog_read_programs( fp, pObjIndex );
        }
        else ungetc( letter,fp );
 
        iHash                   = vnum % MAX_KEY_HASH;
        pObjIndex->next         = obj_index_hash[iHash];
        obj_index_hash[iHash]   = pObjIndex;
        top_obj_index++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;   /* OLC */
//        assign_area_vnum( vnum );                                   /* OLC */
    }
 
    return;
}

/*****************************************************************************
 Name:	        convert_objects
 Purpose:	Converts all old format objects to new format
 Called by:	boot_db (db.c).
 Note:          Loops over all resets to find the level of the mob
                loaded before the object to determine the level of
                the object.
		It might be better to update the levels in load_resets().
		This function is not pretty.. Sorry about that :)
 Author:        Hugin
 ****************************************************************************/
void convert_objects( void )
{
    int vnum;
    AREA_DATA  *pArea;
    RESET_DATA *pReset;
    MOB_INDEX_DATA *pMob = NULL;
    OBJ_INDEX_DATA *pObj;
    ROOM_INDEX_DATA *pRoom;
    VNUM_RANGE *vrange;

    if ( newobjs == top_obj_index ) return; /* all objects in new format */

    for ( pArea = area_first; pArea; pArea = pArea->next )
    {
	for ( vrange = pArea->vnums; vrange; vrange = vrange->next )
	{
        for ( vnum = vrange->min_vnum; vnum <= vrange->max_vnum; vnum++ )
	{
	    if ( !( pRoom = get_room_index( vnum ) ) ) continue;

	    for ( pReset = pRoom->reset_first; pReset; pReset = pReset->next )
	    {
		switch ( pReset->command )
		{
		case 'M':
		    if ( !( pMob = get_mob_index( pReset->arg1 ) ) )
			bug( "Convert_objects: 'M': bad vnum %d.", pReset->arg1 );
		    break;

		case 'O':
		    if ( !( pObj = get_obj_index( pReset->arg1 ) ) )
		    {
			bug( "Convert_objects: 'O': bad vnum %d.", pReset->arg1 );
			break;
		    }

		    if ( pObj->new_format )
			continue;

		    if ( !pMob )
		    {
			bug( "Convert_objects: 'O': No mob reset yet.", 0 );
			break;
		    }

		    pObj->level = pObj->level < 1 ? pMob->level - 2
			: UMIN(pObj->level, pMob->level - 2);
		    break;

		case 'P':
		    {
			OBJ_INDEX_DATA *pObj, *pObjTo;

			if ( !( pObj = get_obj_index( pReset->arg1 ) ) )
			{
			    bug( "Convert_objects: 'P': bad vnum %d.", pReset->arg1 );
			    break;
			}

			if ( pObj->new_format )
			    continue;

			if ( !( pObjTo = get_obj_index( pReset->arg3 ) ) )
			{
			    bug( "Convert_objects: 'P': bad vnum %d.", pReset->arg3 );
			    break;
			}

			pObj->level = pObj->level < 1 ? pObjTo->level
			    : UMIN(pObj->level, pObjTo->level);
		    }
		    break;

		case 'G':
		case 'E':
		    if ( !( pObj = get_obj_index( pReset->arg1 ) ) )
		    {
			bug( "Convert_objects: 'E' or 'G': bad vnum %d.", pReset->arg1 );
			break;
		    }

		    if ( !pMob )
		    {
			bug( "Convert_objects: 'E' or 'G': null mob for vnum %d.",
			     pReset->arg1 );
			break;
		    }

		    if ( pObj->new_format )
			continue;

		    if ( pMob->pShop )
		    {
			switch ( pObj->item_type )
			{
			default:
			    pObj->level = UMAX(0, pObj->level);
			    break;
			case ITEM_PILL:
			case ITEM_POTION:
			case ITEM_OIL:
			    pObj->level = UMAX(5, pObj->level);
			    break;
			case ITEM_SCROLL:
			case ITEM_ARMOR:
			case ITEM_WEAPON:
			    pObj->level = UMAX(10, pObj->level);
			    break;
			case ITEM_WAND:
			case ITEM_TREASURE:
			    pObj->level = UMAX(15, pObj->level);
			    break;
			case ITEM_STAFF:
			    pObj->level = UMAX(20, pObj->level);
			    break;
			}
		    }
		    else
			pObj->level = pObj->level < 1 ? pMob->level
			    : UMIN( pObj->level, pMob->level );
		    break;
		} /* switch ( pReset->command ) */
	    }
	}
	}
    }

    /* do the conversion: */

    for ( pArea = area_first; pArea ; pArea = pArea->next )
	for (vrange = pArea->vnums; vrange; vrange = vrange->next )
	    for ( vnum = vrange->min_vnum; vnum <= vrange->max_vnum; vnum++ )
	        if ( (pObj = get_obj_index( vnum )) )
 		    if ( !pObj->new_format )
		        convert_object( pObj );

    return;
}



/*****************************************************************************
 Name:		convert_object
 Purpose:	Converts an old_format obj to new_format
 Called by:	convert_objects (db2.c).
 Note:          Dug out of create_obj (db.c)
 Author:        Hugin
 ****************************************************************************/
void convert_object( OBJ_INDEX_DATA *pObjIndex )
{
    int level;
    int number, type;  /* for dice-conversion */

    if ( !pObjIndex || pObjIndex->new_format ) return;

    level = pObjIndex->level;

    pObjIndex->level    = UMAX( 0, pObjIndex->level ); /* just to be sure */
    pObjIndex->cost     = 10*level;

    switch ( pObjIndex->item_type )
    {
        default:
            bug( "Obj_convert: vnum %d bad type.", pObjIndex->item_type );
            break;

        case ITEM_LIGHT:
        case ITEM_TREASURE:
        case ITEM_FURNITURE:
        case ITEM_TRASH:
        case ITEM_CONTAINER:
        case ITEM_DRINK_CON:
        case ITEM_KEY:
        case ITEM_FOOD:
        case ITEM_BOAT:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
        case ITEM_FOUNTAIN:
        case ITEM_MAP:
        case ITEM_CLOTHING:
        case ITEM_SCROLL:
 	case ITEM_WRITING:
	    break;

        case ITEM_WAND:
        case ITEM_STAFF:
            pObjIndex->value[2] = pObjIndex->value[1];
	    break;

        case ITEM_WEAPON:

	    /*
	     * The conversion below is based on the values generated
	     * in one_hit() (fight.c).  Since I don't want a lvl 50 
	     * weapon to do 15d3 damage, the min value will be below
	     * the one in one_hit, and to make up for it, I've made 
	     * the max value higher.
	     * (I don't want 15d2 because this will hardly ever roll
	     * 15 or 30, it will only roll damage close to 23.
	     * I can't do 4d8+11, because one_hit there is no dice-
	     * bounus value to set...)
	     *
	     * The conversion below gives:

	     level:   dice      min      max      mean
	       1:     1d8      1( 2)    8( 7)     5( 5)
	       2:     2d5      2( 3)   10( 8)     6( 6)
	       3:     2d5      2( 3)   10( 8)     6( 6)
	       5:     2d6      2( 3)   12(10)     7( 7)
	      10:     4d5      4( 5)   20(14)    12(10)
	      20:     5d5      5( 7)   25(21)    15(14)
	      30:     5d7      5(10)   35(29)    20(20)
	      50:     5d11     5(15)   55(44)    30(30)

	     */

	    number = UMIN(level/4 + 1, 5);
	    type   = (level + 7)/number;

            pObjIndex->value[1] = number;
            pObjIndex->value[2] = type;
	    break;

        case ITEM_ARMOR:
            pObjIndex->value[0] = level / 5 + 3;
            pObjIndex->value[1] = pObjIndex->value[0];
            pObjIndex->value[2] = pObjIndex->value[0];
	    break;

        case ITEM_POTION:
        case ITEM_PILL:
	case ITEM_OIL:
            break;

        case ITEM_MONEY:
	    pObjIndex->value[0] = pObjIndex->cost;
	    break;
    }

    pObjIndex->new_format = TRUE;
    ++newobjs;

    return;
}




/*****************************************************************************
 Name:		convert_mobile
 Purpose:	Converts an old_format mob into new_format
 Called by:	load_old_mob (db.c).
 Note:          Dug out of create_mobile (db.c)
 Author:        Hugin
 ****************************************************************************/
void convert_mobile( MOB_INDEX_DATA *pMobIndex )
{
    int i;
    int type, number, bonus;
    int level;

    if ( !pMobIndex || pMobIndex->new_format ) return;

    level = pMobIndex->level;

    pMobIndex->act              |= ACT_WARRIOR;

    /*
     * Calculate hit dice.  Gives close to the hitpoints
     * of old format mobs created with create_mobile()  (db.c)
     * A high number of dice makes for less variance in mobiles
     * hitpoints.
     * (might be a good idea to reduce the max number of dice)
     *
     * The conversion below gives:

       level:     dice         min         max        diff       mean
         1:       1d2+6       7(  7)     8(   8)     1(   1)     8(   8)
	 2:       1d3+15     16( 15)    18(  18)     2(   3)    17(  17)
	 3:       1d6+24     25( 24)    30(  30)     5(   6)    27(  27)
	 5:      1d17+42     43( 42)    59(  59)    16(  17)    51(  51)
	10:      3d22+96     99( 95)   162( 162)    63(  67)   131(    )
	15:     5d30+161    166(159)   311( 311)   145( 150)   239(    )
	30:    10d61+416    426(419)  1026(1026)   600( 607)   726(    )
	50:    10d169+920   930(923)  2610(2610)  1680(1688)  1770(    )

	The values in parenthesis give the values generated in create_mobile.
        Diff = max - min.  Mean is the arithmetic mean.
	(hmm.. must be some roundoff error in my calculations.. smurfette got
	 1d6+23 hp at level 3 ? -- anyway.. the values above should be
	 approximately right..)
     */
    type   = level*level*27/40;
    number = UMIN(type/40 + 1, 10); /* how do they get 11 ??? */
    type   = UMAX(2, type/number);
    bonus  = UMAX(0, level*(8 + level)* 9 / 10 - number*type);

    pMobIndex->hit[DICE_NUMBER]    = number;
    pMobIndex->hit[DICE_TYPE]      = type;
    pMobIndex->hit[DICE_BONUS]     = bonus;

    pMobIndex->mana[DICE_NUMBER]   = level;
    pMobIndex->mana[DICE_TYPE]     = 10;
    pMobIndex->mana[DICE_BONUS]    = 100;

    /*
     * Calculate dam dice.  Gives close to the damage
     * of old format mobs in damage()  (fight.c)
     */
    type   = level*7/4;
    number = UMIN(type/8 + 1, 5);
    type   = UMAX(2, type/number);
    bonus  = UMAX(0, level*9/4 - number*type);

    pMobIndex->damage[DICE_NUMBER] = number;
    pMobIndex->damage[DICE_TYPE]   = type;
    pMobIndex->damage[DICE_BONUS]  = bonus;

    switch ( number_range( 1, 3 ) )
    {
        case (1): pMobIndex->dam_type =  3;       break;  /* slash  */
        case (2): pMobIndex->dam_type =  7;       break;  /* pound  */
        case (3): pMobIndex->dam_type = 11;       break;  /* pierce */
    }

    for (i = 0; i < 3; i++)
        pMobIndex->ac[i]         = interpolate( level, 100, -100);
    pMobIndex->ac[3]             = interpolate( level, 100, 0);    /* exotic */

    pMobIndex->wealth           /= 100;
    pMobIndex->size              = SIZE_MEDIUM;
    pMobIndex->material          = MATERIAL_FLESH;

    pMobIndex->new_format        = TRUE;
    ++newmobs;

    return;
}

/* the functions */

/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. This allows the use of the words in the
 *  mob/script files.
 */

int mprog_name_to_type ( char *name )
{
   if ( !str_cmp( name, "in_file_prog"   ) )	return IN_FILE_PROG;
   if ( !str_cmp( name, "act_prog"       ) )    return ACT_PROG;
   if ( !str_cmp( name, "speech_prog"    ) )	return SPEECH_PROG;
   if ( !str_cmp( name, "rand_prog"      ) ) 	return RAND_PROG;
   if ( !str_cmp( name, "tick_prog"      ) ) 	return TICK_PROG;
   if ( !str_cmp( name, "hour_prog"      ) ) 	return HOUR_PROG;
   if ( !str_cmp( name, "fight_prog"     ) )	return FIGHT_PROG;
   if ( !str_cmp( name, "hitprcnt_prog"  ) )	return HITPRCNT_PROG;
   if ( !str_cmp( name, "death_prog"     ) )	return DEATH_PROG;
   if ( !str_cmp( name, "entry_prog"     ) )	return ENTRY_PROG;
   if ( !str_cmp( name, "greet_prog"     ) )	return GREET_PROG;
   if ( !str_cmp( name, "all_greet_prog" ) )	return ALL_GREET_PROG;
   if ( !str_cmp( name, "give_prog"      ) ) 	return GIVE_PROG;
   if ( !str_cmp( name, "bribe_prog"     ) )	return BRIBE_PROG;
   if ( !str_cmp( name, "exit_prog"      ) )    return EXIT_PROG;
   if ( !str_cmp( name, "wear_prog"	 ) )	return WEAR_PROG;
   if ( !str_cmp( name, "remove_prog"    ) )    return REMOVE_PROG;
   if ( !str_cmp( name, "demon_prog"	 ) )	return DEMON_PROG;
   if ( !str_cmp( name, "time_prog"	 ) )	return TIME_PROG;
   if ( !str_cmp( name, "load_prog"	 ) )	return LOAD_PROG;
   if ( !str_cmp( name, "take_prog"	 ) )	return TAKE_PROG;
   if ( !str_cmp( name, "all_death_prog" ) )	return ALL_DEATH_PROG;
   if ( !str_cmp( name, "verb_prog"	 ) )	return VERB_PROG;
   if ( !str_cmp( name, "trigger_prog"	 ) )	return TRIGGER_PROG;
   if ( !str_cmp( name, "sac_prog"	 ) )	return SAC_PROG;
   if ( !str_cmp( name, "data_prog"	 ) )    return DATA_PROG;
   if ( !str_cmp( name, "hit_prog"	 ) )	return HIT_PROG;
   if ( !str_cmp( name, "hail_prog"	 ) )	return HAIL_PROG;
   if ( !str_cmp( name, "attack_prog"	 ) )	return ATTACK_PROG;
   if ( !str_cmp( name, "eat_prog"	 ) )	return EAT_PROG;
   if ( !str_cmp( name, "drink_prog"	 ) )	return DRINK_PROG; 
   if ( !str_cmp( name, "sub_prog"	 ) )	return SUB_PROG; 

   return( ERROR_PROG );
}

void determine_exact_match(MPROG_DATA * mprg)
{
	assert(mprg != 0);

	char arg[MAX_STRING_LENGTH];
	mprg->exact_match = false;

	// If 'p' or 'P', require exact match
	if (mprg->type & VERB_PROG)
    {
		one_argument(mprg->arglist, arg);
		if (!str_cmp(arg, "p") || !str_cmp(arg, "P"))
			mprg->exact_match = true;
	}
}

/* This routine reads in scripts of MOBprograms from a file */

MPROG_DATA* mprog_file_read( char *f, MPROG_DATA *mprg,
			    MOB_INDEX_DATA *pMobIndex )
{

  char        MOBProgfile[ MAX_INPUT_LENGTH ];
  MPROG_DATA *mprg2;
  FILE       *progfile;
  char        letter;
  bool        done = FALSE;

  sprintf( MOBProgfile, "%s%s", MOB_DIR, f );

  progfile = fopen( MOBProgfile, "r" );
  if ( !progfile )
  {
     bug( "Mob: %d couldnt open mobprog file", pMobIndex->vnum );
     exit( 1 );
  }

  mprg2 = mprg;
  switch ( letter = fread_letter( progfile ) )
  {
    case '>':
     break;
    case '|':
       bug( "empty mobprog file.", 0 );
       exit( 1 );
     break;
    default:
       bug( "in mobprog file syntax error.", 0 );
       exit( 1 );
     break;
  }

  while ( !done )
  {
    mprg2->type = mprog_name_to_type( fread_word( progfile ) );
    switch ( mprg2->type )
    {
     case ERROR_PROG:
        bug( "mobprog file type error", 0 );
        exit( 1 );
      break;
     case IN_FILE_PROG:
        bug( "mprog file contains a call to file.", 0 );
        exit( 1 );
      break;
     default:
        pMobIndex->progtypes = pMobIndex->progtypes | mprg2->type;
        mprg2->arglist       = fread_string( progfile );
		determine_exact_match(mprg2);

        mprg2->comlist       = fread_string( progfile );
        switch ( letter = fread_letter( progfile ) )
        {
          case '>':
             mprg2->next = (MPROG_DATA *)alloc_perm( sizeof( MPROG_DATA ));
	     g_num_progs++;
             mprg2       = mprg2->next;
             mprg2->next = NULL;
           break;
          case '|':
             done = TRUE;
           break;
          default:
             bug( "in mobprog file syntax error.", 0 );
             exit( 1 );
           break;
        }
      break;
    }
  }
  fclose( progfile );
  return mprg2;
}


/* This procedure is responsible for reading any in_file MOBprograms.
 */

void mprog_read_programs( FILE *fp, MOB_INDEX_DATA *pMobIndex)
{
  MPROG_DATA *mprg;
  char        letter;
  bool        done = FALSE;

  if ( ( letter = fread_letter( fp ) ) != '>' )
  {
      bug( "Load_mobiles: vnum %d MOBPROG char", pMobIndex->vnum );
      exit( 1 );
  }
  pMobIndex->mobprogs = (MPROG_DATA *)alloc_perm( sizeof( MPROG_DATA ) );
  g_num_progs++;
  mprg = pMobIndex->mobprogs;

  while ( !done )
  {
    mprg->type = mprog_name_to_type( fread_word( fp ) );
    switch ( mprg->type )
    {
     case ERROR_PROG:
        bug( "Load_mobiles: vnum %d MOBPROG type.", pMobIndex->vnum );
        exit( 1 );
      break;
     case IN_FILE_PROG:
        mprg = mprog_file_read( fread_string( fp ), mprg,pMobIndex );
        fread_to_eol( fp );
        switch ( letter = fread_letter( fp ) )
        {
          case '>':
             mprg->next = (MPROG_DATA *)alloc_perm( sizeof( MPROG_DATA ));
	     g_num_progs++;
             mprg       = mprg->next;
             mprg->next = NULL;
           break;
          case '|':
             mprg->next = NULL;
             fread_to_eol( fp );
             done = TRUE;
           break;
          default:
             bug( "Load_mobiles: vnum %d bad MOBPROG.", pMobIndex->vnum );
             exit( 1 );
           break;
        }
      break;
     default:
        pMobIndex->progtypes = pMobIndex->progtypes | mprg->type;
        mprg->arglist        = fread_string( fp );
		determine_exact_match(mprg);
        
		fread_to_eol( fp );
        mprg->comlist        = fread_string( fp );
        fread_to_eol( fp );
        switch ( letter = fread_letter( fp ) )
        {
          case '>':
             mprg->next = (MPROG_DATA *)alloc_perm( sizeof( MPROG_DATA ));
	     g_num_progs++;
             mprg       = mprg->next;
             mprg->next = NULL;
           break;
          case '|':
             mprg->next = NULL;
             fread_to_eol( fp );
             done = TRUE;
           break;
          default:
             bug( "Load_mobiles: vnum %d bad MOBPROG.", pMobIndex->vnum );
             exit( 1 );
           break;
        }
      break;
    }
  }

  return;

}


/* This procedure is responsible for reading any in_file OBJprograms.
 */

void oprog_read_programs( FILE *fp, OBJ_INDEX_DATA *pObjIndex)
{
  MPROG_DATA *mprg(0);
  char        letter;
  bool        done = FALSE;

  if ( ( letter = fread_letter( fp ) ) != '>' )
  {
      bug( "Load_objects: vnum %d OBJPROG char", pObjIndex->vnum );
      exit( 1 );
  }
  pObjIndex->objprogs = (MPROG_DATA *)alloc_perm( sizeof(MPROG_DATA ) );
  g_num_progs++;
  mprg = pObjIndex->objprogs;

  while ( !done )
  {
    mprg->type = mprog_name_to_type( fread_word( fp ) );
    switch ( mprg->type )
    {
     case ERROR_PROG:
        bug( "Load_objects: vnum %d OBJPROG type.", pObjIndex->vnum );
        exit( 1 );
      break;
     default:
        pObjIndex->progtypes = pObjIndex->progtypes | mprg->type;
        mprg->arglist        = fread_string( fp );
		determine_exact_match(mprg);

        fread_to_eol( fp );
        mprg->comlist        = fread_string( fp );
        fread_to_eol( fp );
        switch ( letter = fread_letter( fp ) )
        {
          case '>':
             mprg->next = (MPROG_DATA *)alloc_perm( sizeof( MPROG_DATA ));
	     g_num_progs++;
             mprg       = mprg->next;
             mprg->next = NULL;
           break;
          case '|':
             mprg->next = NULL;
             fread_to_eol( fp );
             done = TRUE;
           break;
          default:
             bug( "Load_objects: vnum %d bad OBJPROG.", pObjIndex->vnum );
             exit( 1 );
           break;
        }
      break;
    }
  }

  return;

}

void rprog_read_programs( FILE *fp, ROOM_INDEX_DATA *pRoom)
{
  MPROG_DATA *mprg;
  char        letter;
  bool        done = FALSE;

  if ( ( letter = fread_letter( fp ) ) != '>' )
  {
      bug( "Load_mobiles: vnum %d ROOMPROG char", pRoom->vnum );
      exit( 1 );
  }
  
  pRoom->mobprogs = (MPROG_DATA *)alloc_perm( sizeof( MPROG_DATA ) );
  g_num_progs++;
  mprg = pRoom->mobprogs;

  while ( !done )
  {
    mprg->type = mprog_name_to_type( fread_word( fp ) );
    switch ( mprg->type )
    {
     case ERROR_PROG:
        bug( "Load_mobiles: vnum %d ROOMPROG type.", pRoom->vnum );
        exit( 1 );
      break;
     default:
        pRoom->progtypes = pRoom->progtypes | mprg->type;
        mprg->arglist        = fread_string( fp );
		determine_exact_match(mprg);

        fread_to_eol( fp );
        mprg->comlist        = fread_string( fp );
        fread_to_eol( fp );
        switch ( letter = fread_letter( fp ) )
        {
          case '>':
             mprg->next = (MPROG_DATA *)alloc_perm( sizeof( MPROG_DATA ));
	     g_num_progs++;
             mprg       = mprg->next;
             mprg->next = NULL;
           break;
          case '|':
             mprg->next = NULL;
             fread_to_eol( fp );
             done = TRUE;
           break;
          default:
             bug( "Load_mobiles: vnum %d bad ROOMPROG.", pRoom->vnum );
             exit( 1 );
           break;
        }
      break;
    }
  }

  return;

}

long obj_default_price(OBJ_INDEX_DATA *pObjIndex)
{
	    AFFECT_DATA *paf;
	    double newcost = 0;
	    int num_effects = 0;
	    double effect_cost = 0;

	    if (pObjIndex->item_type == ITEM_WEAPON)
	    {
		int weapavg = (1 + pObjIndex->value[2]) * pObjIndex->value[1] / 2;

		if (weapavg <= 11)
		    newcost = weapavg * 78 / 10;
		else if (weapavg >= 31)
		    newcost = 1820000 + ((weapavg - 31) * 260000);
		else
		{
		    switch (weapavg)
		    {
			case 12:	newcost = 182;		break;
			case 13:	newcost = 250;		break;
			case 14:	newcost = 375;		break;
			case 15:	newcost = 500;		break;
			case 16:	newcost = 716;		break;
			case 17:	newcost = 1300;		break;
			case 18:	newcost = 3120;		break;
			case 19:	newcost = 5200;		break;
			case 20:	newcost = 7800;		break;
			case 21:	newcost = 19500;	break;
			case 22:	newcost = 61148;	break;
			case 23:	newcost = 122897;	break;
			case 24:	newcost = 184345;	break;
			case 25:	newcost = 390000;	break;
			case 26:	newcost = 650000;	break;
			case 27:	newcost = 910000;	break;
			case 28:	newcost = 1040000;	break;
			case 29:	newcost = 1300000;	break;
			case 30:	newcost = 1560000;	break;
		    }
		}
	    }

	    if (pObjIndex->item_type == ITEM_ARMOR)
	    {
		int i;

		for (i = 0; i < 4; i++)
		{
		    if (pObjIndex->value[i] <= 4)
			newcost += 15 * pObjIndex->value[i];
		    else if (pObjIndex->value[i] <= 8)
			newcost += 50 * pObjIndex->value[i];
		    else if (pObjIndex->value[i] <= 12)
			newcost += 175 * pObjIndex->value[i];
		    else if (pObjIndex->value[i] <= 17)
			newcost += 350 * pObjIndex->value[i];
		    else
			newcost += 660 * pObjIndex->value[i];
		}
	    }

	    for (paf = pObjIndex->affected; paf; paf = paf->next)
	    {
		double this_effect_cost = 0;

		num_effects++;

		if ((paf->location >= APPLY_RESIST_SUMMON) && (paf->location <= APPLY_RESIST_IRON))
		{
		    if (paf->modifier <= 0)
		    {
			num_effects--;
			this_effect_cost = 0;
		    }
		    else if (paf->modifier <= 2)
		        this_effect_cost += 1560 * paf->modifier;
		    else if (paf->modifier == 3)
			this_effect_cost += 6240;
		    else
			this_effect_cost += 15600 + ((paf->modifier - 4) * 15600);
		}
		else
		{
		switch (paf->location)
		{
		    case APPLY_HIT:
		    {
			if (paf->modifier <= 0)
			{
			    num_effects--;
			    this_effect_cost = 0;
			}
			else if (paf->modifier <= 5)
			    this_effect_cost += 156 * paf->modifier;
			else if (paf->modifier <= 10)
			    this_effect_cost += 312 * paf->modifier;
			else if (paf->modifier <= 15)
			    this_effect_cost += 520 * paf->modifier;
			else if (paf->modifier <= 20)
			    this_effect_cost += 780 * paf->modifier;
			else if (paf->modifier <= 25)
			    this_effect_cost += 1248 * paf->modifier;
			else if (paf->modifier <= 30)
			    this_effect_cost += 2080 * paf->modifier;
			else if (paf->modifier <= 35)
			    this_effect_cost += 2675 * paf->modifier;
			else if (paf->modifier <= 40)
			    this_effect_cost += 3250 * paf->modifier;
			else if (paf->modifier <= 45)
			    this_effect_cost += 5778 * paf->modifier;
			else if (paf->modifier <= 49)
			    this_effect_cost += 10500 * paf->modifier;
			else
			    this_effect_cost += 520000 + ((paf->modifier - 50) * 52000);

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_FINGER))
			    this_effect_cost *= 1.5;
			
			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_NECK))
			    this_effect_cost *= 1.5;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_BODY))
			    this_effect_cost *= 0.85;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_LEGS))
			    this_effect_cost *= 1.5;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_HANDS))
			    this_effect_cost *= 1.5;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_ARMS))
			    this_effect_cost *= 1.5;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_SHIELD))
			    this_effect_cost *= 0.9;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_WRIST))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WIELD))
			    this_effect_cost *= 1.5;
		    }
		    break;

		    case APPLY_MANA:
		    {
			if (paf->modifier <= 0)
			{
			    num_effects--;
			    this_effect_cost = 0;
			}
			else if (paf->modifier <= 5)
			    this_effect_cost += 104 * paf->modifier;
			else if (paf->modifier <= 10)
			    this_effect_cost += 130 * paf->modifier;
			else if (paf->modifier <= 15)
			    this_effect_cost += 312 * paf->modifier;
			else if (paf->modifier <= 20)
			    this_effect_cost += 312 * paf->modifier;
			else if (paf->modifier <= 25)
			    this_effect_cost += 374.4 * paf->modifier;
			else if (paf->modifier <= 30)
			    this_effect_cost += 416 * paf->modifier;
			else if (paf->modifier <= 35)
			    this_effect_cost += 535 * paf->modifier;
			else if (paf->modifier <= 40)
			    this_effect_cost += 650 * paf->modifier;
			else if (paf->modifier <= 45)
			    this_effect_cost += 1200 * paf->modifier;
			else if (paf->modifier <= 49)
			    this_effect_cost += 2600 * paf->modifier;
			else
			    this_effect_cost += 130000 + ((paf->modifier - 50) * 15600);

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_BODY))
			    this_effect_cost *= 1.5;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_HEAD))
			    this_effect_cost *= 0.85;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_LEGS))
			    this_effect_cost *= 1.2;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_FEET))
			    this_effect_cost *= 1.2;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_SHIELD))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WIELD))
			    this_effect_cost *= 1.2;

			if (IS_SET(pObjIndex->wear_flags, ITEM_HOLD))
			    this_effect_cost *= 0.9;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_FLOAT))
			    this_effect_cost *= 0.9;

		    }
		    break; 
			
		    case APPLY_MOVE:
		    {
			if (paf->modifier <= 0)
			{
			    num_effects--;
			    this_effect_cost = 0;
			}
			else if (paf->modifier <= 15)
			    this_effect_cost += 52 * paf->modifier;
			else if (paf->modifier <= 20)
			    this_effect_cost += 58.5 * paf->modifier;
			else if (paf->modifier <= 25)
			    this_effect_cost += 62.4 * paf->modifier;
			else if (paf->modifier <= 30)
			    this_effect_cost += 62.4 * paf->modifier;
			else if (paf->modifier <= 35)
			    this_effect_cost += 104 * paf->modifier;
			else if (paf->modifier <= 40)
			    this_effect_cost += 195 * paf->modifier;
			else if (paf->modifier <= 45)
			    this_effect_cost += 208 * paf->modifier;
			else if (paf->modifier <= 49)
			    this_effect_cost += 374.4 * paf->modifier;
			else
			    this_effect_cost += 18720 + ((paf->modifier - 50) * 1560);

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_BODY))
			    this_effect_cost *= 1.5;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_HEAD))
			    this_effect_cost *= 1.2;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_LEGS))
			    this_effect_cost *= 0.85;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_FEET))
			    this_effect_cost *= 0.85;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_FLOAT))
			    this_effect_cost *= 0.9;
		    }
		    break;

		    case APPLY_HITROLL:
		    {
			if (paf->modifier <= 0)
			{
			    num_effects--;
			    this_effect_cost = 0;
			}
			else if (paf->modifier == 1)
			    this_effect_cost += 390;
			else if (paf->modifier == 2)
			    this_effect_cost += 1170;
			else if (paf->modifier == 3)
			    this_effect_cost += 3160;
			else if (paf->modifier == 4)
			    this_effect_cost += 9360;
			else
			    this_effect_cost += 23400 + ((paf->modifier - 5) * 10400);

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_NECK))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_BODY))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_ARMS))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_ABOUT))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_WAIST))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WIELD))
			    this_effect_cost *= 0.9;

			if (IS_SET(pObjIndex->wear_flags, ITEM_HOLD))
			    this_effect_cost *= 1.1;
		    }
		    break;

		    case APPLY_DAMROLL:
		    {
			if (paf->modifier <= 0)
			{
			    num_effects--;
			    this_effect_cost = 0;
			}
			else if (paf->modifier == 1)
			    this_effect_cost += 1170;
			else if (paf->modifier == 2)
			    this_effect_cost += 9360;
			else if (paf->modifier == 3)
			    this_effect_cost += 50000;
			else if (paf->modifier == 4)
			    this_effect_cost += 100000;
			else
			    this_effect_cost += 200000 + ((paf->modifier - 5) * 150000);

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_NECK))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_FINGER))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_LEGS))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_HANDS))
			    this_effect_cost *= 0.9;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_ARMS))
			    this_effect_cost *= 1.2;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_ABOUT))
			    this_effect_cost *= 1.2;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_WAIST))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WIELD))
			    this_effect_cost *= 0.9;

			if (IS_SET(pObjIndex->wear_flags, ITEM_HOLD))
			    this_effect_cost *= 1.1;
		    }
		    break;
			
		    case APPLY_AC:
		    {
			if (paf->modifier >= 0)
			{
			    num_effects--;
			    this_effect_cost = 0;
			}
			else if (paf->modifier >= -4)
			    this_effect_cost += -15 * paf->modifier;
			else if (paf->modifier >= -10)
			    this_effect_cost += -50 * paf->modifier;
			else if (paf->modifier >= -15)
			    this_effect_cost += -175 * paf->modifier;
			else if (paf->modifier >= -20)
			    this_effect_cost += -350 * paf->modifier;
			else
			    this_effect_cost += -660 * paf->modifier;

			this_effect_cost *= 4.4;
		    }
		    break;

		    case APPLY_SAVES:
//		    case APPLY_SAVING_PARA:
		    case APPLY_SAVING_ROD:
		    case APPLY_SAVING_PETRI:
	            case APPLY_SAVING_BREATH:
		    case APPLY_SAVING_SPELL:
		    {
			if (paf->modifier >= 0)
			{
			    num_effects--;
			    this_effect_cost = 0;
			}
			else if (paf->modifier == -1)
			    this_effect_cost += 1170;
			else if (paf->modifier == -2)
			    this_effect_cost += 3120;
			else if (paf->modifier == -3)
			    this_effect_cost += 6240;
			else if (paf->modifier == -4)
			    this_effect_cost += 12480;
			else
			    this_effect_cost += 20540 + ((paf->modifier + 5) * -13000);
		    }
		    break;

		    case APPLY_INT:
		    case APPLY_WIS:
		    case APPLY_DEX:
		    case APPLY_CON:
		    {
			if (paf->modifier <= 0)
			{
			    num_effects--;
			    this_effect_cost = 0;
			}
			else if (paf->modifier == 1)
			    this_effect_cost += 520;
			else if (paf->modifier == 2)
			    this_effect_cost += 1040;
			else if (paf->modifier == 3)
			    this_effect_cost += 3120;
			else if (paf->modifier == 4)
			    this_effect_cost += 6240;
			else
			    this_effect_cost += 9360 + ((paf->modifier - 5) * 13000);

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_LEGS))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_HOLD))
			    this_effect_cost *= 0.9;
		    }
		    break;
			   
		    case APPLY_STR:
		    {
			if (paf->modifier <= 0)
			{
			    num_effects--;
			    this_effect_cost = 0;
			}
			else if (paf->modifier == 1)
			    this_effect_cost += 520;
			else if (paf->modifier == 2)
			    this_effect_cost += 1040;
			else if (paf->modifier == 3)
			    this_effect_cost += 4680;
			else if (paf->modifier == 4)
			    this_effect_cost += 9360;
			else
			    this_effect_cost += 14040 + ((paf->modifier - 5) * 15600);

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_LEGS))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_HOLD))
			    this_effect_cost *= 0.9;
		    }
		    break;

		    case APPLY_CHR:
		    {
			if (paf->modifier <= 0)
			{
			    num_effects--;
			    this_effect_cost = 0;
			}
			else if (paf->modifier == 1)
			    this_effect_cost += 520;
			else if (paf->modifier == 2)
			    this_effect_cost += 1040;
			else if (paf->modifier == 3)
			    this_effect_cost += 3120;
			else if (paf->modifier == 4)
			    this_effect_cost += 6240;
			else
			    this_effect_cost += 9360 + ((paf->modifier - 5) * 11700);

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_LEGS))
			    this_effect_cost *= 1.1;

			if (IS_SET(pObjIndex->wear_flags, ITEM_HOLD))
			    this_effect_cost *= 0.9;
		    }
		    break;

		    case APPLY_SIZE:
			if (paf->modifier != 0)
			    this_effect_cost += 1560000;
			break;

            case APPLY_LUCK:
            if (paf->modifier <= 0)
            {
                --num_effects;
                this_effect_cost = 0;
            }
            else
                this_effect_cost = pow(2, paf->modifier - 1) * 300;
            break;

		    case APPLY_SEX:
			this_effect_cost = 7800;


		    default:
			num_effects--;
			break;
		}
		}

		effect_cost += this_effect_cost;
	    }

	    if (num_effects == 2)
		effect_cost *= 1.1;
	    else if (num_effects == 3)
		effect_cost *= 1.2;
	    else if (num_effects == 4)
		effect_cost *= 1.5;
	    else if (num_effects == 5)
		effect_cost *= 2.5;
	    else if (num_effects >= 6)
		effect_cost *= 4 + ((num_effects - 5) * 1.5);

	    newcost += effect_cost;

	    newcost /= 2;

	    return UMAX(0, round(newcost));
}

// Log an error for the global mysql database connection.
void log_mysql_error()
{
    char buf[MAX_STRING_LENGTH];

	int code(mysql_errno(&mysql));
    sprintf(buf, "Query error %d: %s.", code, mysql_error(&mysql));
    bug(buf, 0);

	// If we lost connection to the server (error 2006), attempt to reconnect
	if (code == 2006)
	{
		if (connect_to_mysql())
			bug("Lost connection to MySQL, but successfully reconnected", 0);
	}
}
 
