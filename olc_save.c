/***************************************************************************
 *  File: olc_save.c                                                       *
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
/*#define AREADEBUG*/
/* Uncomment that line above to get massive logfiles to tell you why
OLC-generated areas are crshing you when you save, look for changed dates in
the logfiles. fucked up */

/* OLC_SAVE.C
 * This takes care of saving all the .are information.
 * Notes:
 * -If a good syntax checker is used for setting vnum ranges of areas
 *  then it would become possible to just cycle through vnums instead
 *  of using the iHash stuff and checking that the room or reset or
 *  mob etc is part of that area.
 */

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
#include "tables.h"
#include "LeyGroup.h"
#include "Shades.h"
#include "Faction.h"

char * mprog_type_to_name ( int type );
void save_resets_2( FILE *fp, ROOM_INDEX_DATA *pRoom );

void make_binary_string(int value, char * buffer)
{
	int i;
	for (i = 0; i < 32; i++)
	{
		if (value & (1 << i)) buffer[i] = '1';
		else buffer[i] = '0';
	}
	buffer[32] = '\0';
}

extern	void	save_room_desc	args( ( ROOM_INDEX_DATA *pRoom ) );

/*
 *  Verbose writes reset data in plain english into the comments
 *  section of the resets.  It makes areas considerably larger but
 *  may aid in debugging.
 */

int determine_start_vnum(int my_vnum, AREA_DATA * pArea, VNUM_RANGE * my_range, int which_to_convert);
int convert_vnum(int vnum, AREA_DATA * pArea, int which_to_convert);
int total_nulls(VNUM_RANGE * prange, int vnum, int which_to_convert);		

/* #define VERBOSE */

/*****************************************************************************
 Name:		fix_string
 Purpose:	Returns a string without \r and ~.
 ****************************************************************************/
char *fix_string( const char *str )
{
    static char strfix[MAX_STRING_LENGTH];
    int i;
    int o;

    if ( str == NULL )
        return '\0';

    for ( o = i = 0; str[i+o] != '\0'; i++ )
    {
        if (str[i+o] == '\r' || str[i+o] == '~')
            o++;
        strfix[i] = str[i+o];
    }
    strfix[i] = '\0';
    return strfix;
}


char *fix_crap( const char *str )
{
    static char strfix[MAX_STRING_LENGTH];
    int i;
    int o;

    if ( str == NULL )
        return '\0';

    for ( o = i = 0; str[i+o] != '\0'; i++ )
    {
        if (str[i+o] == '$')
            o++;
        strfix[i] = str[i+o];
    }
    strfix[i] = '\0';
    return strfix;
}



/*****************************************************************************
 Name:		save_area_list
 Purpose:	Saves the listing of files to be loaded at startup.
 Called by:	do_asave(olc_save.c).
 ****************************************************************************/
void save_area_list()
{
    FILE *fp;
    AREA_DATA *pArea;

    if ( ( fp = fopen( "area.lst", "w" ) ) == NULL )
    {
	bug( "Save_area_list: fopen", 0 );
	perror( "area.lst" );
    }
    else
    {
	/*
	 * Add any help files that need to be loaded at
	 * startup to this section.
	 */
//	fprintf( fp, "help.are\n"   );
	fprintf( fp, "social.are\n" );    /* ROM OLC */
//	fprintf( fp, "rom.are\n"    );    /* ROM OLC */
//	fprintf( fp, "group.are\n"  );    /* ROM OLC */
//	fprintf( fp, "olc.hlp\n"    );

	for( pArea = area_first; pArea; pArea = pArea->next )
	{
	    fprintf( fp, "%s\n", fwrite_format(pArea->file_name));
	}

	fprintf( fp, "$\n" );
	fclose( fp );
    }

    return;
}


/*
 * ROM OLC
 * Used in save_mobile and save_object below.  Writes
 * flags on the form fread_flag reads.
 * 
 * buf[] must hold at least 32+1 characters.
 *
 * -- Hugin
 */
char *fwrite_flag( long flags, char buf[] )
{
    char offset;
    char *cp;

    buf[0] = '\0';

    if ( flags == 0 )
    {
	strcpy( buf, "0" );
	return buf;
    }

    /* 32 -- number of bits in a long */

    for ( offset = 0, cp = buf; offset < 32; offset++ )
	if ( flags & ( (long)1 << offset ) )
	{
	    if ( offset <= 'Z' - 'A' )
		*(cp++) = 'A' + offset;
	    else
		*(cp++) = 'a' + offset - ( 'Z' - 'A' + 1 );
	}

    *cp = '\0';

    return buf;
}

/*****************************************************************************
 Name:		save_mobile
 Purpose:	Save one mobile to file, new format -- Hugin
 Called by:	save_mobiles (below).
 ****************************************************************************/
void save_mobile( FILE *fp, MOB_INDEX_DATA *pMobIndex )
{
    int race = pMobIndex->race;
    char buf[MAX_STRING_LENGTH];
    MPROG_DATA *mprg;
    int i;

  //V4 represents mobile version.

    fprintf( fp, "V4 %d\n",         pMobIndex->vnum );
    fprintf( fp, "%s~\n",         fwrite_format(pMobIndex->player_name));
    fprintf( fp, "%s~\n",         fwrite_format(pMobIndex->short_descr));
    fprintf( fp, "%s~\n",         fwrite_format(pMobIndex->long_descr));
    fprintf( fp, "%s~\n",         fwrite_format(pMobIndex->description));
    fprintf( fp, "%s~\n",         fwrite_format(race_table[race].name));
    fprintf( fp, "%s~\n",	  fwrite_format(class_table[pMobIndex->class_num].name));
    fprintf( fp, "%s ",	          fwrite_flag( pMobIndex->act,         buf ) );
    fprintf( fp, "%s ",		  fwrite_flag( pMobIndex->nact,	       buf ) );
    fprintf( fp, "%s ",	          fwrite_flag( pMobIndex->affected_by, buf ) );
    fprintf( fp, "%c %d\n",       UPPER(align_names[pMobIndex->alignment][0]), pMobIndex->group);
    fprintf( fp, "%d ",	          pMobIndex->level );
    fprintf( fp, "%d ",	          pMobIndex->hitroll );
    fprintf( fp, "%dd%d+%d ",     pMobIndex->hit[DICE_NUMBER], 
	     	     	          pMobIndex->hit[DICE_TYPE], 
	     	     	          pMobIndex->hit[DICE_BONUS] );
    fprintf( fp, "%dd%d+%d ",     pMobIndex->mana[DICE_NUMBER], 
	     	     	          pMobIndex->mana[DICE_TYPE], 
	     	     	          pMobIndex->mana[DICE_BONUS] );
    fprintf( fp, "%dd%d+%d ",     pMobIndex->damage[DICE_NUMBER], 
	     	     	          pMobIndex->damage[DICE_TYPE], 
	     	     	          pMobIndex->damage[DICE_BONUS] );
    fprintf( fp, "%s~\n",         damtype_table[pMobIndex->dam_type].name );
    fprintf( fp, "%s~\n",	  pMobIndex->dam_verb);
    fprintf( fp, "%d %d %d %d\n", pMobIndex->ac[AC_PIERCE] / 10, 
	     	     	          pMobIndex->ac[AC_BASH]   / 10, 
	     	     	          pMobIndex->ac[AC_SLASH]  / 10, 
	     	     	          pMobIndex->ac[AC_EXOTIC] / 10 );
    fprintf( fp, "%s ",           fwrite_flag( pMobIndex->off_flags,  buf ) );
    fprintf( fp, "%s\n",          fwrite_flag( pMobIndex->imm_flags,  buf ) );
/*  fprintf( fp, "%s ",           fwrite_flag( pMobIndex->res_flags,  buf ) );
    fprintf( fp, "%s\n",          fwrite_flag( pMobIndex->vuln_flags, buf ) );
*/

    fprintf( fp, "%d ", MAX_RESIST);
    for (i = 0; i < MAX_RESIST; i++)
      fprintf(fp, "%d ", pMobIndex->resist[i]);
    fprintf( fp, "\n");

    fprintf( fp, "%d", MAX_ASSIST_VNUM);
    for (i = 0; i < MAX_ASSIST_VNUM; i++)
	fprintf(fp, " %d", pMobIndex->assist_vnum[i]);
    fprintf( fp, "\n");

    fprintf( fp, "%s %s %s %ld %d\n",
	                          position_table[pMobIndex->start_pos].short_name,
	         	     	  position_table[pMobIndex->default_pos].short_name,
	         	     	  sex_table[pMobIndex->sex].name,
	         	     	  pMobIndex->wealth,
				  pMobIndex->factionNumber);
    fprintf( fp, "%s ",           fwrite_flag( pMobIndex->form,  buf ) );
    fprintf( fp, "%s ",      	  fwrite_flag( pMobIndex->parts, buf ) );
    fprintf( fp, "%s ",		  fwrite_flag( pMobIndex->lang_flags, buf ) );

/*
    switch ( pMobIndex->size )
    {
        default:          letter2 = "medium"; break;
        case SIZE_TINY:   letter2 = "tiny"; break;
        case SIZE_SMALL:  letter2 = "small"; break;
    	case SIZE_MEDIUM: letter2 = "medium"; break;
        case SIZE_LARGE:  letter2 = "large"; break;
        case SIZE_HUGE:   letter2 = "huge"; break;
        case SIZE_GIANT:  letter2 = "giant"; break;
    }
*/

    fprintf( fp, "%s ",           size_table[pMobIndex->size].name );
    fprintf( fp, "%s\n" , material_table[pMobIndex->material].name );

    mprg = pMobIndex->mobprogs;

    while (mprg != NULL)
    {
      fprintf( fp, ">%s ", fwrite_format(mprog_type_to_name(mprg->type)));
      fprintf( fp, "%s~\n",fwrite_format(mprg->arglist));
      fprintf( fp, "%s~\n", fwrite_format(mprg->comlist));
      mprg = mprg->next;
    }

    if (pMobIndex->mobprogs != NULL) fprintf( fp, "|\n" );

    return;
}


/*****************************************************************************
 Name:		save_mobiles
 Purpose:	Save #MOBILES secion of an area file.
 Called by:	save_area(olc_save.c).
 Notes:         Changed for ROM OLC.
 ****************************************************************************/
void save_mobiles( FILE *fp, AREA_DATA *pArea )
{
    int i;
    MOB_INDEX_DATA *pMob;
    VNUM_RANGE *vRange;

    fprintf( fp, "#MOBILES\n" );

    for (vRange = pArea->vnums; vRange; vRange = vRange->next)
        for( i = vRange->min_vnum; i <= vRange->max_vnum; i++ )
        {
	    if ( (pMob = get_mob_index( i )) )
	        save_mobile( fp, pMob );
        }

    fprintf( fp, "#0\n\n\n\n" );
    return;
}

/*****************************************************************************
 Name:		save_object
 Purpose:	Save one object to file.
                new ROM format saving -- Hugin
 Called by:	save_objects (below).
 ****************************************************************************/
void save_object( FILE *fp, OBJ_INDEX_DATA *pObjIndex )
{
    char letter;
    AFFECT_DATA *pAf;
    EXTRA_DESCR_DATA *pEd;
    MPROG_DATA *oprg;
    char buf[MAX_STRING_LENGTH];

    fprintf( fp, "V%d\n",    13);   /** current obj version number **/
    fprintf( fp, "%d\n",     pObjIndex->vnum );
    fprintf( fp, "%s~\n",    fwrite_format(pObjIndex->name));
    fprintf( fp, "%s~\n",    fwrite_format(pObjIndex->short_descr));
    fprintf( fp, "%s~\n",    fwrite_format(pObjIndex->description));
    fprintf( fp, "%s~\n",    fwrite_format(pObjIndex->lore));
    fprintf( fp, "%s~\n",    fwrite_format(material_table[pObjIndex->material].name));
    fprintf( fp, "%s ",      item_name(pObjIndex->item_type));
    fprintf( fp, "%s ",      fwrite_flag( pObjIndex->extra_flags[0], buf ) );
    fprintf( fp, "%s ",	     fwrite_flag( pObjIndex->extra_flags[1], buf ) );
    fprintf( fp, "%s ",	     fwrite_flag( pObjIndex->extra_flags[2], buf ) );
    fprintf( fp, "%s\n",     fwrite_flag( pObjIndex->wear_flags,  buf ) );

/*
 *  Using fwrite_flag to write most values gives a strange
 *  looking area file, consider making a case for each
 *  item type later.
 */

    switch ( pObjIndex->item_type )
    {
        default:
	    fprintf( fp, "%s ",  fwrite_flag( pObjIndex->value[0], buf ) );
	    fprintf( fp, "%s ",  fwrite_flag( pObjIndex->value[1], buf ) );
	    fprintf( fp, "%s ",  fwrite_flag( pObjIndex->value[2], buf ) );
	    fprintf( fp, "%s ",  fwrite_flag( pObjIndex->value[3], buf ) );
	    fprintf( fp, "%s\n", fwrite_flag( pObjIndex->value[4], buf ) );
	    break;

        case ITEM_LIGHT:
	    fprintf( fp, "0 %s %d 0 0\n",
                fwrite_flag(pObjIndex->value[1], buf),
		        (pObjIndex->value[2] < 1 ? 999  /* infinite */
		        : pObjIndex->value[2]) );
	    break;

        case ITEM_MONEY:
            fprintf( fp, "%d %d 0 0 0\n",
                     pObjIndex->value[0],
                     pObjIndex->value[1]);
            break;
            
        case ITEM_DRINK_CON:
            fprintf( fp, "%d %d '%s' %d %d\n",
                     pObjIndex->value[0],
                     pObjIndex->value[1],
                     liq_table[pObjIndex->value[2]].liq_name,
/*                   fwrite_flag( pObjIndex->value[3], buf ) ); */
		     pObjIndex->value[3],
		     pObjIndex->value[4]);
            break;
                    
	case ITEM_FOUNTAIN:
	    fprintf( fp, "%d %d '%s' %d 0\n",
	             pObjIndex->value[0],
	             pObjIndex->value[1],
	             liq_table[pObjIndex->value[2]].liq_name,
		     pObjIndex->value[3]);
	    break;
	    
        case ITEM_CONTAINER:
            fprintf( fp, "%d %s %d %d %d\n",
                     pObjIndex->value[0],
                     fwrite_flag( pObjIndex->value[1], buf ),
                     pObjIndex->value[2],
                     pObjIndex->value[3],
                     pObjIndex->value[4]);
            break;
            
        case ITEM_FOOD:
            fprintf( fp, "%d %d 0 %s 0\n",
                     pObjIndex->value[0],
                     pObjIndex->value[1],
                     fwrite_flag( pObjIndex->value[3], buf ) );
            break;
            
        case ITEM_PORTAL:
            fprintf( fp, "%d %s %s %d 0\n",
                     pObjIndex->value[0],
                     fwrite_flag( pObjIndex->value[1], buf ),
                     fwrite_flag( pObjIndex->value[2], buf ),
                     pObjIndex->value[3]);
            break;
            
        case ITEM_FURNITURE:
            fprintf( fp, "%d %d %s %d %d\n",
                     pObjIndex->value[0],
                     pObjIndex->value[1],
                     fwrite_flag( pObjIndex->value[2], buf),
                     pObjIndex->value[3],
                     pObjIndex->value[4]);
            break;
            
        case ITEM_WEAPON:
            fprintf( fp, "%s %d %d %d %s\n",
                     weapon_name(pObjIndex->value[0]),
                     pObjIndex->value[1],
                     pObjIndex->value[2],
                     pObjIndex->value[3],
                     fwrite_flag( pObjIndex->value[4], buf ) );
	    fprintf( fp, "%s~\n", fwrite_format(pObjIndex->obj_str));
            break;

	case ITEM_SPECIAL:
	    fprintf( fp, "%d %d %d %d %d\n",
		pObjIndex->value[0],
		pObjIndex->value[1],
		pObjIndex->value[2],
		pObjIndex->value[3],
		pObjIndex->value[4]);
	    break;

	case ITEM_WRITING:
	    fprintf( fp, "%d %d %d 0 0\n",
		pObjIndex->value[0],
		pObjIndex->value[1],
		pObjIndex->value[2]);
	    break;
          
        case ITEM_ARMOR:
            fprintf( fp, "%d %d %d %d %d\n",
                     pObjIndex->value[0],
                     pObjIndex->value[1],
                     pObjIndex->value[2],
                     pObjIndex->value[3],
                     pObjIndex->value[4]);
            break;
            
        case ITEM_PILL:
        case ITEM_POTION:
        case ITEM_SCROLL:
	case ITEM_OIL:
	    fprintf( fp, "%d '%s' '%s' '%s' '%s'\n",
		     pObjIndex->value[0] > 0 ? /* no negative numbers */
		     pObjIndex->value[0]
		     : 0,
		     pObjIndex->value[1] != -1 ?
		     skill_table[pObjIndex->value[1]].name
		     : "",
		     pObjIndex->value[2] != -1 ?
		     skill_table[pObjIndex->value[2]].name
		     : "",
		     pObjIndex->value[3] != -1 ?
		     skill_table[pObjIndex->value[3]].name
		     : "",
		     pObjIndex->value[4] != -1 ?
		     skill_table[pObjIndex->value[4]].name
		     : "");
	    break;

        case ITEM_STAFF:
        case ITEM_WAND:
	    fprintf( fp, "%d ", pObjIndex->value[0] );
	    fprintf( fp, "%d ", pObjIndex->value[1] );
	    fprintf( fp, "%d '%s' 0\n",
		     pObjIndex->value[2],
		     pObjIndex->value[3] != -1 ?
		       skill_table[pObjIndex->value[3]].name
		       : 0 );
	    break;

	case ITEM_ARROW:
	    fprintf( fp, "%d %d %d %d %d\n", 
		     pObjIndex->value[0],
		     pObjIndex->value[1], pObjIndex->value[2], 
		     pObjIndex->value[3], pObjIndex->value[4]);
	    fprintf( fp, "%s~\n", fwrite_format(pObjIndex->obj_str));
	    break;
    }

    fprintf( fp, "%d ", pObjIndex->level );
    fprintf( fp, "%d ", pObjIndex->weight );
    fprintf( fp, "%d ", pObjIndex->cost );
    fprintf( fp, "%d ", pObjIndex->size );

         if ( pObjIndex->condition >  90 ) letter = 'P';
    else if ( pObjIndex->condition >  75 ) letter = 'G';
    else if ( pObjIndex->condition >  50 ) letter = 'A';
    else if ( pObjIndex->condition >  25 ) letter = 'W';
    else if ( pObjIndex->condition >  10 ) letter = 'D';
    else if ( pObjIndex->condition >   0 ) letter = 'B';
    else                                   letter = 'R';

    fprintf( fp, "%c\n", letter );
    fprintf( fp, "%d\n", pObjIndex->limit_factor);

    for( pAf = pObjIndex->affected; pAf; pAf = pAf->next )
    {
        fprintf( fp, "A\n%d %d\n",  pAf->location, pAf->modifier );
    }

    for( pEd = pObjIndex->extra_descr; pEd; pEd = pEd->next )
    {
        fprintf( fp, "E\n%s~\n", fwrite_format(pEd->keyword));
	fprintf( fp, "%s~\n", fwrite_format(pEd->description));
    }

    if (pObjIndex->obj_str != NULL)
    	fprintf( fp, "O %s~\n", fwrite_format(pObjIndex->obj_str));

    oprg = pObjIndex->objprogs;

    while (oprg != NULL)
    {
      fprintf( fp, ">%s ", fwrite_format(mprog_type_to_name(oprg->type)));
      fprintf( fp, "%s~\n", fwrite_format(oprg->arglist));
      fprintf( fp, "%s~\n", fwrite_format(oprg->comlist));
      oprg = oprg->next;
    }

    if (pObjIndex->objprogs != NULL) fprintf( fp, "|\n" );
}
 
/*****************************************************************************
 Name:		save_objects
 Purpose:	Save #OBJECTS section of an area file.
 Called by:	save_area(olc_save.c).
 Notes:         Changed for ROM OLC.
 ****************************************************************************/
void save_objects( FILE *fp, AREA_DATA *pArea )
{
    int i;
    OBJ_INDEX_DATA *pObj;
    VNUM_RANGE *vRange;

    fprintf( fp, "#OBJECTS\n" );

    for (vRange = pArea->vnums; vRange; vRange = vRange->next)
        for( i = vRange->min_vnum; i <= vRange->max_vnum; i++ )
        {
	   if ( (pObj = get_obj_index( i )) )
	        save_object( fp, pObj );
        }

    fprintf( fp, "#0\n\n\n\n" );
    return;
}
 
int convert_vnum(int vnum, AREA_DATA * pArea, int which_to_convert)
{
	VNUM_RANGE * prange;
	FILE * fp = fopen("vnumconv.dat", "a+");
	if (which_to_convert == 1 && (vnum == 22850 || vnum == 22901))
	{
		int x;
		x = 5;
	}
	int flag = 0;
	fprintf(fp, "%s: (%d) %d -> ", pArea->file_name, which_to_convert, vnum);
	for (prange = pArea->vnums; prange != NULL; prange = prange->next)
	{					
		if (vnum >= prange->min_vnum && vnum <= prange->max_vnum)
		{
			int temp = determine_start_vnum(vnum, pArea, prange, which_to_convert);
			vnum = (vnum - prange->min_vnum) + temp;
			flag = 1;
			break;
		}
	}
	fprintf(fp, "%d\n", vnum);
	fclose(fp);
	if (flag == 0) return -1;
	return vnum;	
}

int determine_start_vnum(int my_vnum, AREA_DATA * pArea, VNUM_RANGE * my_range, int which_to_convert)
{
	VNUM_RANGE * prange = pArea->vnums;
	int curr_max = prange->max_vnum, curr_min = prange->min_vnum;
	int arr[50][3];
	int index = 0;
	int curr_index = 0;
	int total_null = total_nulls(prange, my_vnum, which_to_convert);
	arr[0][0] = curr_min;
	arr[0][1] = curr_max;
	int beg_index = -1;

	if (my_range->min_vnum == prange->min_vnum && my_range->max_vnum == prange->max_vnum)
	{
		beg_index = curr_index;
	}
	
	prange = prange->next;	
	while (prange != NULL)
	{
		total_null += total_nulls(prange, my_vnum, which_to_convert);
		++index;
		int diff = prange->max_vnum - prange->min_vnum;
		if (prange->min_vnum > curr_max)
		{
			arr[index][0] = curr_max + 1;
			arr[index][1] = arr[index][0] + diff;
			curr_max = arr[index][1];
			curr_index = index;
		}
		else if (prange->max_vnum < curr_min)
		{
			int i;
			for (i = index; i > 0; --i)
			{
				arr[index][0] = arr[index-1][0];
				arr[index][1] = arr[index-1][1];
			}
			arr[0][1] = curr_min - 1;
			arr[0][0] = arr[0][1] - diff;
			curr_min = arr[0][0];
			curr_index = 0;
			if (beg_index >= 0)
			{
				++beg_index;
			}
		}
		if (my_range->min_vnum == prange->min_vnum && my_range->max_vnum == prange->max_vnum)
		{
			beg_index = curr_index;
		}
		prange = prange->next;
	}

	int i;
	FILE * fp = fopen("vnumconv.dat", "a+");
	for (i = 0; i <= index; ++i)
		fprintf(fp, "{%d - %d} ", arr[i][0], arr[i][1]);
	fclose(fp);
	
	if (beg_index < 0) beg_index = 0;
	return (arr[beg_index][0] - total_null - arr[0][0]);
}

int total_nulls(VNUM_RANGE * prange, int vnum, int which_to_convert)
{
	int i;
	int total = 0;
	FILE * fp = fopen("vnumconv.dat", "a+");
	for (i = prange->min_vnum; i <= UMIN(prange->max_vnum, vnum); ++i)
	{
		switch (which_to_convert)
		{
			case 0: if (get_mob_index(i) == NULL) ++total; break;
			case 1: if (get_obj_index(i) == NULL) ++total; break;
			case 2: if (get_room_index(i) == NULL) ++total; break;
		}
	}
	fprintf(fp, "(%d - %d) [%d]", prange->min_vnum, UMIN(prange->max_vnum, vnum), total);
	fclose(fp);
	return total;
}

/*****************************************************************************
 Name:		save_rooms
 Purpose:	Save #ROOMS section of an area file.
 Called by:	save_area(olc_save.c).
 ****************************************************************************/
void save_rooms( FILE *fp, AREA_DATA *pArea )
{
#ifdef AREADEBUG
    char buf[MAX_STRING_LENGTH];
#endif
    ROOM_INDEX_DATA *pRoomIndex;
    EXTRA_DESCR_DATA *pEd;
    EXIT_DATA *pExit;
    MPROG_DATA *mprg;
    int iHash;
    int door;

    fprintf( fp, "#ROOMS\n" );
    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
        {
            if ( pRoomIndex->area == pArea )
            {
#ifdef AREADEBUG
		sprintf(buf, "SAVING %d", pRoomIndex->vnum);
		bug(buf, 0);
#endif
                fprintf( fp, "#%d\n",		pRoomIndex->vnum );
#ifdef AREADEBUG
		sprintf(buf, "VNUM %d", pRoomIndex->vnum);
		bug(buf, 0);
#endif
                fprintf( fp, "%s~\n",		fwrite_format(pRoomIndex->name));
#ifdef AREADEBUG
		sprintf(buf, "NAME %d", pRoomIndex->vnum);
		bug(buf, 0);
#endif
             /*  fprintf( fp, "%s~\n",		fix_string( pRoomIndex->description ) );*/
                fprintf( fp, "%s~\n",		fwrite_format(pRoomIndex->description));

		fprintf( fp, "%s~\n", fwrite_format(pRoomIndex->owner));
#ifdef AREADEBUG
		sprintf(buf, "DESC %d", pRoomIndex->vnum);
		bug(buf, 0);
#endif
		fprintf( fp, "%d\n",		pRoomIndex->danger);
#ifdef AREADEBUG
		sprintf(buf, "DANGER %d", pRoomIndex->danger);
		bug(buf, 0);
#endif
		fprintf( fp, "0 " );
#ifdef AREADEBUG
		sprintf(buf, "BIGZERO %d", pRoomIndex->vnum);
		bug(buf, 0);
#endif
                fprintf( fp, "%d ",		pRoomIndex->room_flags );
#ifdef AREADEBUG
		sprintf(buf, "FLAGS %d", pRoomIndex->vnum);
		bug(buf, 0);
#endif
		if (room_is_affected(pRoomIndex, gsn_plantgrowth))
		    fprintf( fp, "%d\n", get_modifier(pRoomIndex->affected, gsn_plantgrowth));
		else if (room_is_affected(pRoomIndex, gsn_wallofwater))
		    fprintf( fp, "%d\n", get_modifier(pRoomIndex->affected, gsn_wallofwater));
		else if (room_is_affected(pRoomIndex, gsn_flood))
		    fprintf( fp, "%d\n", get_modifier(pRoomIndex->affected, gsn_flood));
		else
                    fprintf( fp, "%d\n",		pRoomIndex->sector_type );
#ifdef AREADEBUG
		sprintf(buf, "SECTOR %d", pRoomIndex->vnum);
		bug(buf, 0);
#endif
		if (!pRoomIndex->owner || pRoomIndex->owner[0] == '\0')
		{
                    for ( pEd = pRoomIndex->extra_descr; pEd;
                          pEd = pEd->next )
                    {
                        fprintf( fp, "E\n%s~\n", fwrite_format(pEd->keyword));
		        fprintf( fp, "%s~\n", fwrite_format(pEd->description));
#ifdef AREADEBUG
		        sprintf(buf, "EXTRADESC %d", pRoomIndex->vnum);
		        bug(buf, 0);
#endif
                    }
		}

                for( door = 0; door < MAX_DIR; door++ )	/* I hate this! */
                {
                    if ( ( pExit = pRoomIndex->exit[door] )
                          && pExit->u1.to_room )
                    {
			int locks = 0;
			if ( IS_SET( pExit->rs_flags, EX_ISDOOR ) 
			&& ( !IS_SET( pExit->rs_flags, EX_PICKPROOF ) ) 
		    	&& ( !IS_SET( pExit->rs_flags, EX_NOPASS ) ) )
			    locks = 1;
			if ( IS_SET( pExit->rs_flags, EX_ISDOOR )
			&& ( IS_SET( pExit->rs_flags, EX_PICKPROOF ) )
		        && ( !IS_SET( pExit->rs_flags, EX_NOPASS ) ) )
			    locks = 2;
			if ( IS_SET( pExit->rs_flags, EX_ISDOOR )
			&& ( !IS_SET( pExit->rs_flags, EX_PICKPROOF ) )
		        && ( IS_SET( pExit->rs_flags, EX_NOPASS ) ) )
			    locks = 3;
			if ( IS_SET( pExit->rs_flags, EX_ISDOOR )
			&& ( IS_SET( pExit->rs_flags, EX_PICKPROOF ) )
		        && ( IS_SET( pExit->rs_flags, EX_NOPASS ) ) )
			    locks = 4;
			
                        fprintf( fp, "D%d\n",      pExit->orig_door );
                        fprintf( fp, "%s~\n",      fwrite_format(pExit->description));
                        fprintf( fp, "%s~\n",      fwrite_format(pExit->keyword));
                        fprintf( fp, "%lu %d %d\n", pExit->rs_flags,
                                                   pExit->key,
                                                   pExit->u1.to_room->vnum );
#ifdef AREADEBUG
		sprintf(buf, "DOOR %d", pRoomIndex->vnum);
		bug(buf, 0);
#endif
                    }
                }

		if (pRoomIndex->gods_altar != 0)
			fprintf(fp, "A %d\n", pRoomIndex->gods_altar);
		if (pRoomIndex->mana_rate != 100 || pRoomIndex->heal_rate != 100 || pRoomIndex->move_rate != 100)
		 fprintf ( fp, "M %d H %d V %d\n",pRoomIndex->mana_rate,
		                 pRoomIndex->heal_rate, pRoomIndex->move_rate);
         if (pRoomIndex->stone_type >= 0 && pRoomIndex->stone_type < MAX_MATERIALS && material_table[pRoomIndex->stone_type].stone)
             fprintf(fp, "T %d\n", pRoomIndex->stone_type);
#ifdef AREADEBUG
		sprintf(buf, "HEALRATE %d", pRoomIndex->vnum);
		bug(buf, 0);
#endif
		if (pRoomIndex->clan > 0)
		 fprintf ( fp, "C %s~\n" , fwrite_format(clan_table[pRoomIndex->clan].who_name));
#ifdef AREADEBUG
		sprintf(buf, "CLAN %d", pRoomIndex->vnum);
		bug(buf, 0);
#endif

        fprintf(fp, "W %d %d %d\n", pRoomIndex->fount_frequency, pRoomIndex->fount_order_power, pRoomIndex->fount_positive_power);
        fprintf(fp, "Z %d %d\n", pRoomIndex->shade_density, pRoomIndex->shade_power);

		if ((mprg = pRoomIndex->mobprogs) != NULL)
		{
		    fprintf( fp, "P\n" );

		    while (mprg != NULL)
		    {
      			fprintf( fp, ">%s ", fwrite_format(mprog_type_to_name(mprg->type)));
      			fprintf( fp, "%s~\n",fwrite_format(mprg->arglist));
      			fprintf( fp, "%s~\n", fwrite_format(mprg->comlist));
      			mprg = mprg->next;
    		    }

    		    fprintf( fp, "|\n" );
		}
		 			     
		fprintf( fp, "S\n" );
#ifdef AREADEBUG
		sprintf(buf, "DONE SAVING %d", pRoomIndex->vnum);
		bug(buf, 0);
#endif
            }
        }
    }
    fprintf( fp, "#0\n\n\n\n" );
    return;
}

void save_specials_2( FILE *fp, AREA_DATA *pArea )
{
	int iHash;
	MOB_INDEX_DATA *pMobIndex;

	fprintf( fp, "#BEGIN_SPECIALS\n" );

	for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
	{
		for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
		{
			if ( pMobIndex && pMobIndex->area == pArea && pMobIndex->spec_fun )
			{
				fprintf( fp, "#SPEC %d %s\n", pMobIndex->vnum,
				spec_name( pMobIndex->spec_fun ) );
			}
		}
	}

	fprintf( fp, "#END_SPECIALS\n\n\n\n" );
	return;
}


/*****************************************************************************
 Name:		save_specials
 Purpose:	Save #SPECIALS section of area file.
 Called by:	save_area(olc_save.c).
 ****************************************************************************/
void save_specials( FILE *fp, AREA_DATA *pArea )
{
    int iHash;
    MOB_INDEX_DATA *pMobIndex;
    
    fprintf( fp, "#SPECIALS\n" );

    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
        {
            if ( pMobIndex && pMobIndex->area == pArea && pMobIndex->spec_fun )
            {
#if defined( VERBOSE )
                fprintf( fp, "M %d %s Load to: %s\n", pMobIndex->vnum,
                                                      spec_name( pMobIndex->spec_fun ),
                                                      pMobIndex->short_descr );
#else
                fprintf( fp, "M %d %s\n", pMobIndex->vnum,
                              spec_name( pMobIndex->spec_fun ) );
#endif
            }
        }
    }

    fprintf( fp, "S\n\n\n\n" );
    return;
}

void save_door_resets_2( FILE * fp, AREA_DATA * pArea )
{
	int iHash;
	ROOM_INDEX_DATA *pRoomIndex;
	EXIT_DATA *pExit;
	int door;

	for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
	{
		for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
		{
			if ( pRoomIndex->area == pArea )
			{
				for( door = 0; door < MAX_DIR; door++ )
				{
					if ( ( pExit = pRoomIndex->exit[door] )
					&& pExit->u1.to_room
					&& pExit->rs_flags > 0 )
					fprintf( fp, "D 0 %d %d %lu 0\n",
					pRoomIndex->vnum,
					pExit->orig_door,
					pExit->rs_flags);
				}
			}
		}
	}
	return;
}


/*
 * This function is obsolete.  It it not needed but has been left here
 * for historical reasons.  It is used currently for the same reason.
 *
 * I don't think it's obsolete in ROM -- Hugin.
 */
void save_door_resets( FILE *fp, AREA_DATA *pArea )
{
    int iHash;
    ROOM_INDEX_DATA *pRoomIndex;
    EXIT_DATA *pExit;
    int door;

    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
        {
            if ( pRoomIndex->area == pArea )
            {
                for( door = 0; door < MAX_DIR; door++ )
                {
                    if ( ( pExit = pRoomIndex->exit[door] )
                          && pExit->u1.to_room 
			  && pExit->rs_flags > 0 )
#if defined( VERBOSE )
			fprintf( fp, "D 0 %d %d %d The %s door of %s is %s\n", 
				pRoomIndex->vnum,
				pExit->orig_door,
				IS_SET( pExit->rs_flags, EX_LOCKED) ? 2 : 1,
				dir_name[ pExit->orig_door ],
				pRoomIndex->name,
				IS_SET( pExit->rs_flags, EX_LOCKED) ? "closed and locked"
				    : "closed" );
#endif
#if !defined( VERBOSE )
			fprintf( fp, "D 0 %d %d %lu\n", 
				pRoomIndex->vnum,
				pExit->orig_door,
				/*IS_SET( pExit->rs_flags, EX_LOCKED) ? 2 : 1 */
				pExit->rs_flags);
#endif
		}
	    }
	}
    }
    return;
}

/*****************************************************************************
 Name:		save_resets
 Purpose:	Saves the #RESETS section of an area file.
 Called by:	save_area(olc_save.c)
 ****************************************************************************/
void save_resets( FILE *fp, AREA_DATA *pArea )
{
    RESET_DATA *pReset;
    MOB_INDEX_DATA *pLastMob = NULL;
    OBJ_INDEX_DATA *pLastObj;
    ROOM_INDEX_DATA *pRoom;
    char buf[MAX_STRING_LENGTH];
    int iHash;

    fprintf( fp, "#RESETS\n" );

    save_door_resets( fp, pArea );

    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pRoom = room_index_hash[iHash]; pRoom; pRoom = pRoom->next )
        {
            if ( pRoom->area == pArea )
	    {
    for ( pReset = pRoom->reset_first; pReset; pReset = pReset->next )
    {
	switch ( pReset->command )
	{
	default:
	    //bug( "Save_resets: bad command %c.", pReset->command );
	    break;

#if defined( VERBOSE )
	case 'M':
            pLastMob = get_mob_index( pReset->arg1 );
	    fprintf( fp, "M 0 %d %d %d %d Load %s\n", 
	        pReset->arg1,
                pReset->arg2,
                pReset->arg3,
		pReset->arg4,
                pLastMob->short_descr );
            break;

	case 'O':
            pLastObj = get_obj_index( pReset->arg1 );
            pRoom = get_room_index( pReset->arg3 );
	    fprintf( fp, "O 0 %d 0 %d %s loaded to %s\n", 
	        pReset->arg1,
                pReset->arg3,
                capitalize(pLastObj->short_descr),
                pRoom->name );
            break;

	case 'P':
            pLastObj = get_obj_index( pReset->arg1 );
	    fprintf( fp, "P 0 %d %d %d %d %s put inside %s\n", 
	        pReset->arg1,
	        pReset->arg2,
                pReset->arg3,
                pReset->arg4,
                capitalize(get_obj_index( pReset->arg1 )->short_descr),
                pLastObj->short_descr );
            break;

	case 'G':
	    fprintf( fp, "G 0 %d 0 %s is given to %s\n",
	        pReset->arg1,
	        capitalize(get_obj_index( pReset->arg1 )->short_descr),
                pLastMob ? pLastMob->short_descr : "!NO_MOB!" );
            if ( !pLastMob )
            {
                sprintf( buf, "Save_resets: !NO_MOB! in [%s]", pArea->file_name );
                bug( buf, 0 );
            }
            break;

	case 'E':
	    fprintf( fp, "E 0 %d 0 %d %s is loaded %s of %s\n",
	        pReset->arg1,
                pReset->arg3,
                capitalize(get_obj_index( pReset->arg1 )->short_descr),
                flag_string( wear_loc_strings, pReset->arg3 ),
                pLastMob ? pLastMob->short_descr : "!NO_MOB!" );
            if ( !pLastMob )
            {
                sprintf( buf, "Save_resets: !NO_MOB! in [%s]", pArea->file_name );
                bug( buf, 0 );
            }
            break;

	case 'D':
            break;

	case 'R':
            pRoom = get_room_index( pReset->arg1 );
	    fprintf( fp, "R 0 %d %d Randomize %s\n", 
	        pReset->arg1,
                pReset->arg2,
                pRoom->name );
            break;
            }
#endif
#if !defined( VERBOSE )
	case 'M':
            pLastMob = get_mob_index( pReset->arg1 );
	    fprintf( fp, "M 0 %d %d %d %d\n", 
	        pReset->arg1,
                pReset->arg2,
                pReset->arg3,
                pReset->arg4 );
            break;

	case 'O':
            pLastObj = get_obj_index( pReset->arg1 );
            pRoom = get_room_index( pReset->arg3 );
	    fprintf( fp, "O 0 %d 0 %d\n", 
	        pReset->arg1,
                pReset->arg3 );
            break;

	case 'P':
            pLastObj = get_obj_index( pReset->arg1 );
	    fprintf( fp, "P 0 %d %d %d %d\n", 
	        pReset->arg1,
	        pReset->arg2,
                pReset->arg3,
                pReset->arg4 );
            break;

	case 'G':
	    fprintf( fp, "G 0 %d 0\n", pReset->arg1 );
            if ( !pLastMob )
            {
                sprintf( buf,
                    "Save_resets: !NO_MOB! in [%s]", pArea->file_name );
                bug( buf, 0 );
            }
            break;

	case 'E':
	    fprintf( fp, "E 0 %d 0 %d\n",
	        pReset->arg1,
                pReset->arg3 );
            if ( !pLastMob )
            {
                sprintf( buf,
                    "Save_resets: !NO_MOB! in [%s]", pArea->file_name );
                bug( buf, 0 );
            }
            break;

	case 'D':
            break;

	case 'R':
            pRoom = get_room_index( pReset->arg1 );
	    fprintf( fp, "R 0 %d %d\n", 
	        pReset->arg1,
                pReset->arg2 );
            break;
            }
#endif
        }
	    }	/* End if correct area */
	}	/* End for pRoom */
    }	/* End for iHash */
    fprintf( fp, "S\n\n\n\n" );
    return;
}

void save_gms_2( FILE *fp, AREA_DATA *pArea )
{
	GM_DATA     *pGm;
	MOB_INDEX_DATA *pMobIndex;
	int iHash;

	fprintf( fp, "#BEGIN_GUILDS\n" );

	for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
	{
		for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
		{
			if ( pMobIndex && pMobIndex->area == pArea && (pMobIndex->gm != NULL) )
			{
				pGm = pMobIndex->gm;

				fprintf( fp, "#BEGIN_GUILD\n#INDEX %d\n", pMobIndex->vnum /*convert_vnum(pMobIndex->vnum, pArea)*/);
				for (  ; pGm != NULL; pGm = pGm->next )
				{
					if (pGm->sn > -1) fprintf(fp, "#SKILL %s\n", skill_table[pGm->sn].name);
				}
				fprintf( fp, "#END_GUILD\n\n" );
			}
		}
	}

	fprintf( fp, "#END_GUILDS\n\n\n" );
	return;
}


/*****************************************************************************
 Name:		save_gms
 Purpose:	Saves the #GUILD section of an area file.
 Called by:	save_area(olc_save.c)
 ****************************************************************************/
void save_gms( FILE *fp, AREA_DATA *pArea )
{
    GM_DATA 	*pGm;
    MOB_INDEX_DATA *pMobIndex;
    int iHash;
    
    fprintf( fp, "#GUILD\n" );

    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
        {
            if ( pMobIndex && pMobIndex->area == pArea && (pMobIndex->gm != NULL) )
            {
		        pGm = pMobIndex->gm;

                fprintf( fp, "#%d\n", pMobIndex->vnum );
                for (  ; pGm != NULL; pGm = pGm->next )
                {
        		    if (pGm->sn > -1)
			            fprintf(fp, "\"%s\"\n", skill_table[pGm->sn].name);
                }
        	    fprintf( fp, "#0\n\n" );
            }
        }
    }

    fprintf( fp, "S\n\n\n" );
    return;
}


void save_shops_2( FILE *fp, AREA_DATA *pArea )
{
	SHOP_DATA *pShopIndex;
	MOB_INDEX_DATA *pMobIndex;
	int iTrade;
	int iHash;

	fprintf( fp, "#BEGIN_SHOPS\n" );

	for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
	{
		for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
		{
			if ( pMobIndex && pMobIndex->area == pArea && pMobIndex->pShop )
			{
				pShopIndex = pMobIndex->pShop;

				fprintf( fp, "#SHOP %d ", pShopIndex->keeper );
				for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
				{
					if ( pShopIndex->buy_type[iTrade] != 0 )
					{
						fprintf( fp, "%d ", pShopIndex->buy_type[iTrade] );
					}
					else fprintf( fp, "0 ");
				}
				fprintf( fp, "%d %d ", pShopIndex->profit_buy, pShopIndex->profit_sell );
				fprintf( fp, "%d %d ", pShopIndex->open_hour, pShopIndex->close_hour );
				fprintf( fp, "%ld\n",  pShopIndex->max_buy);
			}
		}
	}

	fprintf( fp, "#END_SHOPS\n\n\n\n" );
	return;
}



/*****************************************************************************
 Name:		save_shops
 Purpose:	Saves the #SHOPS section of an area file.
 Called by:	save_area(olc_save.c)
 ****************************************************************************/
void save_shops( FILE *fp, AREA_DATA *pArea )
{
    SHOP_DATA *pShopIndex;
    MOB_INDEX_DATA *pMobIndex;
    int iTrade;
    int iHash;
    
    fprintf( fp, "#SHOPS\n" );

    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
        {
            if ( pMobIndex && pMobIndex->area == pArea && pMobIndex->pShop )
            {
                pShopIndex = pMobIndex->pShop;

                fprintf( fp, "%d ", pShopIndex->keeper );
                for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
                {
                    if ( pShopIndex->buy_type[iTrade] != 0 )
                    {
                       fprintf( fp, "%d ", pShopIndex->buy_type[iTrade] );
                    }
                    else
                       fprintf( fp, "0 ");
                }
                fprintf( fp, "%d %d ", pShopIndex->profit_buy, pShopIndex->profit_sell );
                fprintf( fp, "%d %d ", pShopIndex->open_hour, pShopIndex->close_hour );
		fprintf( fp, "%ld\n",  pShopIndex->max_buy);
            }
        }
    }

    fprintf( fp, "0\n\n\n\n" );
    return;
}



/*****************************************************************************
 Name:		save_area
 Purpose:	Save an area, note that this format is new.
 Called by:	do_asave(olc_save.c).
 ****************************************************************************/
void save_area( AREA_DATA *pArea )
{
    VNUM_RANGE *vRange;

#ifdef AREADEBUG
    char buf[200];
#endif
    FILE *fp;

    fclose( fpReserve );
    if ( !( fp = fopen( pArea->file_name, "w" ) ) )
    {
	bug( "Open_area: fopen", 0 );
	perror( pArea->file_name );
    }

    /* This is a ridiculous way to do this - Aeolis */

    SET_BIT(pArea->ainfo_flags, AINFO_NEWRES);
    SET_BIT(pArea->ainfo_flags, AINFO_AVNUMS);
    SET_BIT(pArea->ainfo_flags, AINFO_MOBCLASS);
    SET_BIT(pArea->ainfo_flags, AINFO_NOWORDLIST);
    SET_BIT(pArea->ainfo_flags, AINFO_HASOWNER);

#ifdef AREADEBUG
    sprintf(buf, "Saving area: %s", pArea->name);
    bug(buf, 0);
#endif

    fprintf( fp, "#AREADATA\n" );
    fprintf( fp, "Name %s~\n",        fwrite_format(pArea->name));
    fprintf( fp, "Builders %s~\n",        fwrite_format(pArea->builders) );

    for (vRange = pArea->vnums; vRange; vRange = vRange->next)
        fprintf( fp, "VNUMs %d %d\n",      vRange->min_vnum, vRange->max_vnum );

    fprintf( fp, "Credits %s~\n",	 fwrite_format(pArea->credits));
    fprintf( fp, "Danger %d\n",         pArea->danger );
    fprintf( fp, "Security %d\n",         pArea->security );
    fprintf( fp, "Areainfo %d\n",         pArea->ainfo_flags );
    fprintf( fp, "Herbs %d\n",		pArea->herbs );
    fprintf( fp, "Weather %d %d %d %d %d\n",
		pArea->base_precip,
		pArea->base_temp,
		pArea->base_wind_mag,
		pArea->base_wind_dir,
		pArea->geography );
    fprintf( fp, "Weave %d %d %d\n", pArea->fount_frequency, pArea->fount_order_bias, pArea->fount_positive_bias);
    fprintf( fp, "Shades %d %d\n", pArea->shade_density, pArea->shade_power);
    fprintf( fp, "Stone %d\n", pArea->stone_type);
    fprintf( fp, "End\n\n\n\n" );

    save_mobiles( fp, pArea );
#ifdef AREADEBUG
    sprintf(buf, "mobiles: %s", pArea->name);
    bug(buf, 0);
#endif

    save_objects( fp, pArea );
#ifdef AREADEBUG
    sprintf(buf, "objects: %s", pArea->name);
    bug(buf, 0);
#endif
    save_rooms( fp, pArea );
#ifdef AREADEBUG
    sprintf(buf, "rooms: %s", pArea->name);
    bug(buf, 0);
#endif

    save_specials( fp, pArea );
#ifdef AREADEBUG
    sprintf(buf, "specials: %s", pArea->name);
    bug(buf, 0);
#endif

    save_resets( fp, pArea );
#ifdef AREADEBUG
    sprintf(buf, "resets: %s", pArea->name);
    bug(buf, 0);
#endif

    save_gms( fp, pArea );
#ifdef AREADEBUG
    sprintf(buf, "gms: %s", pArea->name);
    bug(buf, 0);
#endif

    save_shops( fp, pArea );
#ifdef AREADEBUG
    sprintf(buf, "shops: %s", pArea->name);
    bug(buf, 0);
#endif


    fprintf( fp, "#$\n" );

    fclose( fp );
#ifdef AREADEBUG
    sprintf(buf, "mobiles: Saving area: %s", pArea->name);
    bug(buf, 0);
#endif

    fpReserve = fopen( NULL_FILE, "r" );
    return;
}


/*****************************************************************************
 Name:		do_asave
 Purpose:	Entry point for saving area data.
 Called by:	interpreter(interp.c)
 ****************************************************************************/
void do_asave( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    AREA_DATA *pArea;
    FILE *fp;
    int value;

    fp = NULL;

    if ( !ch )       /* Do an autosave */
    {
	save_area_list();
	for( pArea = area_first; pArea; pArea = pArea->next )
	{
		save_area( pArea );
	    REMOVE_BIT( pArea->area_flags, AREA_CHANGED );
	}
	return;
    }
    smash_tilde( argument );
    strcpy( arg1, argument );
    if ( arg1[0] == '\0' )
    {
    send_to_char( "Syntax:\n\r", ch );
    send_to_char( "  asave <vnum>   - saves a particular area\n\r",	ch );
    send_to_char( "  asave list     - saves the area.lst file\n\r",	ch );
    send_to_char( "  asave area     - saves the area being edited\n\r",	ch );
    send_to_char( "  asave changed  - saves all changed zones\n\r",	ch );
    send_to_char( "  asave world    - saves the world! (db dump)\n\r",	ch );
    send_to_char( "\n\r", ch );
        return;
    }

    /* Snarf the value (which need not be numeric). */
    value = atoi( arg1 );
    if ( !( pArea = get_area_data( value ) ) && is_number( arg1 ) )
    {
	send_to_char( "That area does not exist.\n\r", ch );
	return;
    }
    /* Save area of given vnum. */
    /* ------------------------ */

    if ( is_number( arg1 ) )
    {
        if ( !IS_BUILDER( ch, pArea ) )
        {
            send_to_char( "You are not a builder for this area.\n\r", ch );
            return;
        }
        save_area_list();
        save_area( pArea );
        send_to_char("Area saved.\n", ch);
        return;
    }
    /* Save the world, only authorized areas. */
    /* -------------------------------------- */

    if ( !str_cmp( "world", arg1 ) )
    {
	save_area_list();
	for( pArea = area_first; pArea; pArea = pArea->next )
	{
	    /* Builder must be assigned this area. */
	    if ( !IS_BUILDER( ch, pArea ) )
		continue;	  

	    save_area( pArea );
	    REMOVE_BIT( pArea->area_flags, AREA_CHANGED );
	}
	send_to_char( "You saved the world.\n\r", ch );
	return;
    }

    /* Save changed areas, only authorized areas. */
    /* ------------------------------------------ */

    if ( !str_cmp( "changed", arg1 ) )
    {
#ifdef AREADEBUG
        char debug[MAX_INPUT_LENGTH];
#endif
	char buf[MAX_INPUT_LENGTH];

	save_area_list();

	send_to_char( "Saved zones:\n\r", ch );
	sprintf( buf, "None.\n\r" );

	for( pArea = area_first; pArea; pArea = pArea->next )
	{
#ifdef AREADEBUG
	sprintf(debug, "Checking area: %s", pArea->name);
	bug(debug, 0);
#endif

	    /* Builder must be assigned this area. */
	    if ( !IS_BUILDER( ch, pArea ) )
		continue;

	    /* Save changed areas. */
	    if ( IS_SET(pArea->area_flags, AREA_CHANGED) )
	    {
#ifdef AREADEBUG
		sprintf(debug, "Changed -- SAVE needed -- area: %s", pArea->name);
		bug(debug, 0);
#endif

		save_area( pArea );
#ifdef AREADEBUG
		sprintf(debug, "Done saving: %s", pArea->name);
		bug(debug, 0);
#endif

		sprintf( buf, "%24s - '%s'\n\r", pArea->name, pArea->file_name );
		send_to_char( buf, ch );
		REMOVE_BIT( pArea->area_flags, AREA_CHANGED );
	    }
        }
	if ( !str_cmp( buf, "None.\n\r" ) )
	    send_to_char( buf, ch );
        return;
    }

    /* Save the area.lst file. */
    /* ----------------------- */
    if ( !str_cmp( arg1, "list" ) )
    {
	save_area_list();
	return;
    }

    /* Save area being edited, if authorized. */
    /* -------------------------------------- */
    if ( !str_cmp( arg1, "area" ) )
    {
	/* Is character currently editing. */
	if ( ch->desc->editor == 0 )
	{
	    send_to_char( "You are not editing an area, "
		"therefore an area vnum is required.\n\r", ch );
	    return;
	}
	
	/* Find the area to save. */
	switch (ch->desc->editor)
	{
	    case ED_AREA:
		pArea = (AREA_DATA *)ch->desc->pEdit;
		break;
	    case ED_ROOM:
		pArea = ch->in_room->area;
		break;
	    case ED_OBJECT:
		pArea = ( (OBJ_INDEX_DATA *)ch->desc->pEdit )->area;
		break;
	    case ED_MOBILE:
		pArea = ( (MOB_INDEX_DATA *)ch->desc->pEdit )->area;
		break;
	    default:
		pArea = ch->in_room->area;
		break;
	}

	if ( !IS_BUILDER( ch, pArea ) )
	{
	    send_to_char( "You are not a builder for this area.\n\r", ch );
	    return;
	}

	save_area_list();
	save_area( pArea );
	REMOVE_BIT( pArea->area_flags, AREA_CHANGED );
	send_to_char( "Area saved.\n\r", ch );
	return;
    }

    /* Show correct syntax. */
    /* -------------------- */
    do_asave( ch, "" );
    return;
}
