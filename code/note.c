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
#if defined(unix)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "languages.h"

/* globals from db.c for load_notes */
#if !defined(macintosh)
extern  int     _filbuf         args( (FILE *) );
#endif
extern FILE *                  fpArea;
extern char                    strArea[MAX_INPUT_LENGTH];

extern	void	save_area	args( ( AREA_DATA *pArea ) );
extern	void 	save_room_desc	args( ( ROOM_INDEX_DATA *room ) );
extern	char *  format_obj_to_char(OBJ_DATA *obj, CHAR_DATA *ch, bool fShort);
/* local procedures */
void append_note(NOTE_DATA *pnote);
void load_thread(char *name, NOTE_DATA **list, int type, time_t free_time);
void parse_note(CHAR_DATA *ch, char *argument, int type);
bool hide_note(CHAR_DATA *ch, NOTE_DATA *pnote);
void agent_note(CHAR_DATA *ch, int room);
void inquire_note(CHAR_DATA *ch, AFFECT_DATA *paf);
void investigate_note(CHAR_DATA *ch, OBJ_DATA *obj, int type);

char *note_from(CHAR_DATA *ch, NOTE_DATA *note);

NOTE_DATA *note_list;
NOTE_DATA *idea_list;
NOTE_DATA *penalty_list;
NOTE_DATA *news_list;
NOTE_DATA *changes_list;
NOTE_DATA *roomnote_list;

void investigate_note(CHAR_DATA *ch, OBJ_DATA *obj, int type)
{
    NOTE_DATA *pnote;
    char buf[MAX_STRING_LENGTH],text[MAX_STRING_LENGTH];
    AFFECT_DATA *paf, *poisoned;
    int i, masked_gsn, vials=0;
    char *strtime;

    pnote = new_note();

    pnote->sender		= str_dup("The Office of Records");
    pnote->to_list		= str_dup(ch->name);
    sprintf(buf,"Our investigation of %s",obj->short_descr);
    pnote->subject		= str_dup(buf);
    pnote->forged		= FALSE;
    pnote->forger		= NULL;
    pnote->next                 = NULL;
    strtime                     = ctime( &current_time );
    strtime[strlen(strtime)-1]  = '\0';
    pnote->date                 = str_dup(strtime);
    pnote->date_stamp           = current_time;

    sprintf(buf,"Here are the results for the investigation of:\n\r%s\n\r",obj->short_descr);
    strcpy(text,buf);
    if (type == 5)
    {
	sprintf(buf,"\n\rThis corpse belongs to %s, who was killed by %s.\n\r",
	  obj->owner,obj->killed_by);
	strcat(text,buf);
    }
    else if (type >= 2)
    {
	sprintf(buf,"\n\rObject: '%s' is type %s.\n\rExtra flags %s",
	  obj->short_descr,item_name(obj->item_type),
	  extra_bit_name(obj->extra_flags[0],0));
	strcat(text,buf);
	if (str_cmp(extra_bit_name(obj->extra_flags[1], 1), "none"))
	{
	    sprintf(buf, " %s",extra_bit_name(obj->extra_flags[1],1));
	    strcat(text,buf);
	}
	if (str_cmp(extra_bit_name(obj->extra_flags[2], 2), "none"))
	{
	    sprintf(buf, " %s", extra_bit_name(obj->extra_flags[2],2));
	    strcat(text,buf);
	}
	sprintf(buf,".\n\rWeight is %d.%d, level is %d.\n\r",
	  obj->weight / 10, obj->weight % 10, obj->level);
	strcat(text,buf);
	sprintf(buf,"Material is %s.\n\r",material_table[obj->material].name);
	strcat(text,buf);
    	switch ( obj->item_type )
    	{
    	    case ITEM_SCROLL:
    	    case ITEM_POTION:
	    case ITEM_PILL:
	    case ITEM_OIL:
        
	        masked_gsn = -1;
	        if ((poisoned = get_obj_affect(obj, gsn_subtlepoison)) != NULL)
	    	    masked_gsn = poisoned->modifier;
	  
	        sprintf( buf, "Level %d spells of:", obj->value[0] );
	        strcat(text, buf);

	        if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
       	        {
		    strcat(text, " '");
            	    strcat(text,masked_gsn > 0 ? skill_table[masked_gsn].name : skill_table[obj->value[1]].name );
            	    strcat(text, "'" );
                }

                if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
	        {
		    strcat(text, " '" );
        	    strcat(text, skill_table[obj->value[2]].name );
        	    strcat(text, "'");
                }

	        if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
	        {
		    strcat(text, " '" );
		    strcat(text, skill_table[obj->value[3]].name );
		    strcat(text, "'" );
	        }

                if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL)
	        {
		    strcat(text," '");
		    strcat(text,skill_table[obj->value[4]].name);
		    strcat(text,"'");
	        }

	        strcat(text, ".\n\r" );
            break;
	    case ITEM_WAND:
	    case ITEM_STAFF:
        	sprintf( buf, "Has %d charges of level %d",
            	  obj->value[2], obj->value[0] );
        	strcat(text, buf );

        	if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
        	{
            	    strcat(text, " '" );
            	    strcat(text, skill_table[obj->value[3]].name );
            	    strcat(text, "'" );
        	}

		strcat(text, ".\n\r" );
	    break;

	    case ITEM_ARROW:
		buf[0] = '\0';
		for (i = 2; i < 5; i++)
		    if (obj->value[i] > 0)
			
			if (buf[0] != '\0')
                    	    sprintf(buf, "%s, %s", buf, skill_table[obj->value[i]].name);
                	else
                    	    sprintf(buf, "Imbued with %s", skill_table[obj->value[i]].name);
            
                if (buf[0] != '\0')
        	{
            	    sprintf(buf, "%s.\n\r", buf);
            	    strcat(text,buf);
        	}
            break;

    	    case ITEM_DRINK_CON:
        	sprintf(buf,"It holds %s-colored %s.\n\r",
            	  liq_table[obj->value[2]].liq_color,
            	  liq_table[obj->value[2]].liq_name);
        	strcat(text,buf);
            break;

	    case ITEM_POTIONCONTAINER:
        	vials=obj->value[0]+obj->value[1]+obj->value[2]+obj->value[3]+obj->value[4];
        	if (vials == 0)
        	{
            	    sprintf(buf,"It can hold %i vials, and contains none.\n\r",obj->weight);
            	    strcat(text,buf);
        	}
        	else
        	{
            	    sprintf(buf,"It can hold %i vials, and contains %i:",obj->weight,vials);
            	    act(buf,ch,obj,NULL,TO_CHAR);
		    buf[0] = '\0';
            	    if (obj->value[0] > 0)
                	sprintf(buf,"%i explosive",obj->value[0]);
            	    if (obj->value[1] > 0)
                	if (buf[0] == '\0')
                    	    sprintf(buf,"%i adhesive",obj->value[1]);
                	else
                    	    sprintf(buf,"%s, %i adhesive",str_dup(buf),obj->value[1]);
            	    if (obj->value[2] > 0)
                	if (buf[0] == '\0')
                    	    sprintf(buf,"%i anesthetic",obj->value[2]);
                	else
                    	    sprintf(buf,"%s, %i anesthetic",str_dup(buf),obj->value[2]);
            	    if (obj->value[3] > 0)
                	if (buf[0] == '\0')
                    	    sprintf(buf,"%i toxin",obj->value[3]);
                	else
                    	    sprintf(buf,"%s, %i toxin",str_dup(buf),obj->value[3]);
            	    if (obj->value[4] > 0)
                	if (buf[0] == '\0')
                    	    sprintf(buf,"%i suppresive",obj->value[4]);
                	else
                    	    sprintf(buf,"%s, %i suppressive",str_dup(buf),obj->value[4]);
            	    strcat(buf,".\n\r");
            	    strcat(text,buf);
        	}
            break;

	    case ITEM_CONTAINER:
        	sprintf(buf,"Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
           	  obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));
        	strcat(text,buf);
        	if (obj->value[4] != 100)
        	{
            	    sprintf(buf,"Weight multiplier: %d%%\n\r",obj->value[4]);
		    strcat(text,buf);
		}
            break;

	    case ITEM_WEAPON:
		strcat(text,"Weapon type is ");
	        switch (obj->value[0])
        	{
		    case(WEAPON_KNIFE)  : strcat(text,"knife.\n\r");        break;
		    case(WEAPON_STAFF)  : strcat(text,"staff.\n\r");        break;
		    case(WEAPON_EXOTIC) : strcat(text,"exotic.\n\r");       break;
		    case(WEAPON_SWORD)  : strcat(text,"sword.\n\r");        break;
		    case(WEAPON_DAGGER) : strcat(text,"dagger.\n\r");       break;
		    case(WEAPON_SPEAR)  : strcat(text,"spear.\n\r");        break;
		    case(WEAPON_MACE)   : strcat(text,"mace/club.\n\r");    break;
		    case(WEAPON_AXE)    : strcat(text,"axe.\n\r");          break;
		    case(WEAPON_FLAIL)  : strcat(text,"flail.\n\r");        break;
		    case(WEAPON_WHIP)   : strcat(text,"whip.\n\r");         break;
		    case(WEAPON_POLEARM): strcat(text,"polearm.\n\r");      break;
		    default             : strcat(text,"unknown.\n\r");      break;
		}
		if (obj->pIndexData->new_format)
		    sprintf(buf,"Damage is %dd%d (average %d).\n\r",
		      obj->value[1],obj->value[2],
		      (1 + obj->value[2]) * obj->value[1] / 2);
		else
		    sprintf( buf, "Damage is %d to %d (average %d).\n\r",
		      obj->value[1], obj->value[2], 
		      ( obj->value[1] + obj->value[2] ) / 2 );
		strcat(text, buf );
		if (obj->value[4])  
		{
		    sprintf(buf,"Weapons flags: %s\n\r",weapon_bit_name(obj->value[4]));
		    strcat(text,buf);
		}
	    break;

	    case ITEM_BOW:
		if (obj->pIndexData->new_format)
		    sprintf(buf,"Damage is %dd%d (average %d).\n\r",
		      obj->value[1],obj->value[2],
		      (1 + obj->value[2]) * obj->value[1] / 2);
		else
		    sprintf( buf, "Damage is %d to %d (average %d).\n\r",
		      obj->value[1], obj->value[2],
		      ( obj->value[1] + obj->value[2] ) / 2 );
	        strcat(text,buf);
	    break;

	    case ITEM_ARMOR:
		sprintf( buf, "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\n\r",
		  obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
		strcat(text, buf );
	    break;
	}

	if (!obj->enchanted)
	for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
	{
	    if ( paf->location != APPLY_NONE && paf->location != APPLY_HIDE && paf->modifier != 0 )
	    {
		sprintf( buf, "Affects %s by %d.\n\r",
		  affect_loc_name( paf->location ), paf->modifier );
		strcat(text,buf);
		if (paf->bitvector)
		{
		    switch(paf->where)
		    {
			case TO_AFFECTS:
			    sprintf(buf,"Adds %s affect.\n",affect_bit_name(paf));
                        break;
                        case TO_OBJECT:
                            sprintf(buf,"Adds %s object flag.\n",extra_bit_name(paf->bitvector, 0));
                       	break;
                    	case TO_OBJECT1:  
			    sprintf(buf,"Adds %s object flag.\n", extra_bit_name(paf->bitvector, 1)); 
			break;
                    	case TO_OBJECT2:  
			    sprintf(buf,"Adds %s object flag.\n", extra_bit_name(paf->bitvector, 2)); 
			break;
			case TO_IMMUNE:
			    sprintf(buf,"Adds immunity to %s.\n", imm_bit_name(paf->bitvector));
			break;
			case TO_RESIST:
			    sprintf(buf,"Adds resistance to %s.\n\r", imm_bit_name(paf->bitvector));
			break;
			case TO_VULN:
			    sprintf(buf,"Adds vulnerability to %s.\n\r", imm_bit_name(paf->bitvector));
                        break;
			default:
                            sprintf(buf,"Unknown bit %d: %d\n\r",paf->where,paf->bitvector);
	                break;
        	    }
		    strcat(text, buf );
            	}
       	    }
	}
    
	for ( paf = obj->affected; paf != NULL; paf = paf->next )
	{
            if ( paf->location != APPLY_NONE && paf->location != APPLY_HIDE && paf->modifier != 0 )
            {
                sprintf( buf, "Affects %s by %d", affect_loc_name( paf->location ), paf->modifier );
	        strcat(text, buf );
                if ( paf->duration > -1)
                    sprintf(buf,", %d%s hours.\n\r", paf->duration,
		      ((paf->duration % 2) == 0) ? "" : ".5");
            	else
                    sprintf(buf,".\n\r");
	        strcat(text,buf);
        	if (paf->bitvector)
            	{
                    switch(paf->where)
                    {
                        case TO_AFFECTS:
                    	    sprintf(buf,"Adds %s affect.\n", affect_bit_name(paf));
			break;
			case TO_OBJECT:
                            sprintf(buf,"Adds %s object flag.\n", extra_bit_name(paf->bitvector, 0));
	                break;
        	        case TO_OBJECT1: 
			    sprintf(buf,"Adds %s object flag.\n", extra_bit_name(paf->bitvector, 1)); 
			break;
                    	case TO_OBJECT2: 
			    sprintf(buf,"Adds %s object flag.\n", extra_bit_name(paf->bitvector, 2)); 
			break;
                    	case TO_WEAPON:
                            sprintf(buf,"Adds %s weapon flags.\n", weapon_bit_name(paf->bitvector));
	                break;
        	        case TO_IMMUNE:
                	    sprintf(buf,"Adds immunity to %s.\n", imm_bit_name(paf->bitvector));
	                break;
        	        case TO_RESIST:
                	    sprintf(buf,"Adds resistance to %s.\n\r", imm_bit_name(paf->bitvector));
	                break;
        	        case TO_VULN:
                	    sprintf(buf,"Adds vulnerability to %s.\n\r", imm_bit_name(paf->bitvector));
	                break;
        	        default:
                	    sprintf(buf,"Unknown bit %d: %d\n\r", paf->where,paf->bitvector);
	                break;
        	    }
                    strcat(text,buf);
            	}
      	    }
	}
	if (type == 3)
	{
	    if (obj->lore && obj->lore[0] != '\0')
	        sprintf(buf,"\n\rThe following additional information is available about %s:\n\r%s",
	         obj->short_descr,obj->lore ? obj->lore : obj->pIndexData->lore);
	    else
	        sprintf(buf,"\n\rNo additional information is available about %s.\n\r",obj->short_descr);
	    strcat(text,buf);
	}
    }
    if (type >= 1 && type <= 3)
	if (obj->lastowner[0][0] != '\0')
	{
	    sprintf(buf,"\n\rThis item was last owned by: %s%s%s%s%s.\n\r",
	      obj->lastowner[0],
	      obj->lastowner[1][0] == '\0' ? "" : ", ",
	      obj->lastowner[1],
	      obj->lastowner[2][0] == '\0' ? "" : ", ",
	      obj->lastowner[2]);
	    strcat(text,buf);
	}
    strcat(text,"\n\rWe hope you are pleased by our services and will make use of them again.\n\r");
    pnote->text=str_dup(text);
    append_note(pnote);

    send_to_char("You have a new note waiting.\n\r", ch);
    return;
}

void inquire_note(CHAR_DATA *ch, AFFECT_DATA *paf)
{
    NOTE_DATA *pnote;
    CHAR_DATA *vch = NULL;
    DESCRIPTOR_DATA *d;
    OBJ_DATA *obj;
    int iWear;
    bool found = FALSE;

    char *strtime;
    char buf[MAX_STRING_LENGTH],tmp[MAX_STRING_LENGTH];

    for (d = descriptor_list; d; d = d->next)
    {
	vch = d->original ? d->original : d->character;

	if (vch && (vch->id == paf->modifier))
	    break;
    }

    pnote = new_note();

    pnote->sender		= "The Office of Records";
    pnote->to_list		= str_dup(ch->name);
    if (d)
	sprintf(buf,"Our report on %s",PERS(d->character,ch));
    else
	sprintf(buf,"Your request");
    pnote->subject		= str_dup(buf);
    pnote->next                 = NULL;
    strtime                     = ctime( &current_time );
    strtime[strlen(strtime)-1]  = '\0';
    pnote->date                 = str_dup(strtime);
    pnote->date_stamp           = current_time;

    if (!d)
	strcpy(buf, "We apologize, but the subject of your target does not appear to be anyone\n\rstill in the realm.  Perhaps they left?\n\r\n\r- The Office of Records -\n\r");
    else
    {
	if (!dec_player_bank(ch,value_to_coins(1000,FALSE)))
	    if (!dec_player_coins(ch,value_to_coins(1000,FALSE)))
	    {
	    	send_to_char("You hastily withdraw your request as you realize you haven't the funds to pay for it.\n\r",ch);
	    	free_note(pnote);
	    	return;
	    }
	sprintf(buf, "Information recorded about %s:\n\r\n\r", vch->name);

	sprintf(tmp, "%s%s%s is a %i year old %s %s %s, and is level %d.\n\r\n\r",
	  vch->name, vch->pcdata->surname[0] == '\0' ? "" : " ",
	  vch->pcdata->surname[0] == '\0' ? "" : vch->pcdata->surname,
	  get_age(vch),
	  IS_MALE(vch) ? "male" : (IS_FEMALE(vch) ? "female" : "sexless"),
	  pc_race_table[vch->race].name,
	  class_table[vch->class_num].name,
	  vch->level);
	strcat(buf,tmp);
	for (iWear = 0; iWear < MAX_WEAR; iWear++)
	    if ((obj = get_eq_char(vch, iWear)) != NULL
	      && can_see_obj(ch,obj))
	    {
		if (!found)
		{
		    sprintf(tmp,"%s is using:\n\r",vch->name);
		    strcat(buf,tmp);
		    found = TRUE;
		}
		if (iWear == WEAR_CONCEAL1 || iWear == WEAR_CONCEAL2)
		    if (ch != vch)
			continue;
		sprintf(tmp,"%s%s\n\r",where_name[iWear],format_obj_to_char(obj,ch,TRUE));
		strcat(buf,tmp);
		if (obj->item_type == ITEM_BOW)
		    if (vch->nocked)
		    {
			sprintf(tmp,"<nocked>            %s\n\r",format_obj_to_char(vch->nocked,ch,TRUE));
			strcat(buf,tmp);
		    }
	    }
    	if (!found)
	{
	    sprintf(tmp,"%s is using:\n\rNothing\n\r",vch->name);
	    strcat(buf,tmp);
	}

	if (vch->pcdata->times_wanted)
	{
	    sprintf(tmp, "\n\r%s has been marked as a criminal %d time%s.\n\r",vch->name,vch->pcdata->times_wanted,vch->pcdata->times_wanted == 1 ? "" : "s");
	    strcat(buf,tmp);
	}
	else
	{
	    sprintf(tmp, "\n\r%s has never been marked as a criminal.\n\r",vch->name);
	    strcat(buf,tmp);
	}
	if (vch->pcdata->record)
	    strcat(buf, vch->pcdata->record);
        else
	    strcat(buf, "\n\rNo other information was available.\n\r");
    }
    if (d)
	strcat(buf,"\n\rWe hope you are pleased by our services and will make use of them again.\n\r");
    pnote->text	= str_dup(buf);
    
    append_note(pnote);

    send_to_char("You have a new note waiting.\n\r", ch);

    return;
}

void agent_note(CHAR_DATA *ch, int room, CHAR_DATA *victim)
{
    NOTE_DATA *pnote;
    char *strtime;
    char buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *pRoom=NULL;

    pnote = (NOTE_DATA*)alloc_perm(sizeof(*pnote));
    g_num_notes++;
    
    if (room > 0)
	pRoom = get_room_index(room);

    if (!dec_player_bank(ch,value_to_coins(4000,FALSE)))
	if (!dec_player_coins(ch,value_to_coins(4000,FALSE)))
	{
	    send_to_char("You hastily withdraw your request as you realize you haven't the funds to pay.\n\r",ch);
	    return;
	}

    pnote->sender = str_dup("The Office of Records");
    sprintf(buf, "%s", ch->name);
    pnote->to_list = str_dup(buf);
    sprintf(buf, "Our search for %s",
      victim ? ((CHAR_DATA* ) (victim))->name : "the person you inquired about");
    pnote->subject = str_dup(buf);
    if (pRoom != NULL)
    {
	sprintf(buf, "Our agents have located %s in: %s\n\r\n\r", 
	    victim ? ((CHAR_DATA *) (victim))->name : "the person you inquired about",
	    pRoom->area->name);
	strcat(buf,"We hope you are pleased with our services and will make use of them again.\n\r");
    }
    else
	sprintf(buf, "The person you inquired about cannot be found by conventional means.\n\r");
	
    pnote->text = str_dup(buf);
    pnote->next = NULL;
    strtime = ctime( &current_time );
    strtime[strlen(strtime)-1] = '\0';
    pnote->date = str_dup(strtime);
    pnote->date_stamp = current_time;

    append_note(pnote);
    send_to_char("You have a new note waiting.\n\r", ch);
        
    return;
}

void find_items_note(CHAR_DATA *ch, char * argument)
{
    NOTE_DATA *pnote;
    char      *strtime;
    char    buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;
    found = FALSE;
    number = 0;
    max_found = IS_IMMORTAL(ch) ? 200 : 2 * ch->level;
    buffer = new_buf();
    AFFECT_DATA *paf = affect_find(ch->affected,gsn_finditems);

    if (paf == NULL)
	return;
									 
    pnote = (NOTE_DATA*)alloc_perm(sizeof(*pnote));
    g_num_notes++;
    if (ch->class_num == global_int_class_watcher)
	pnote->sender = str_dup("The Office of Records");
    else
	pnote->sender = str_dup("The Guild of Alchemy");
    sprintf(buf, "%s", ch->name);
    pnote->to_list = str_dup(buf);
    sprintf(buf, "Our search for %s",argument);
    pnote->subject = str_dup(buf);

    add_buf(buffer, "Our sources have discovered the following information about the items you seek:\n\r");
    sprintf(buf, "Requested item: %s\n\n\r", argument);
    add_buf(buffer, buf);
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
	if ( !can_see_obj( ch, obj ) 
	  || !is_name( argument, obj->name )
	  || IS_OBJ_STAT(obj,ITEM_NOLOCATE) 
	  || paf->level < obj->level)
	    continue;
	found = TRUE;
	number++;
	for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj );
	if ( in_obj->carried_by != NULL && can_see(ch,in_obj->carried_by))
	{
	    sprintf( buf, "%s is carried by %s\n\r",
	    obj->short_descr,
	    PERS(in_obj->carried_by, ch) );
	}
	else
	    if (IS_IMMORTAL(ch) && in_obj->in_room != NULL)
	      sprintf( buf, "%s is in %s [Room %d]\n\r",
	      obj->short_descr,
	      in_obj->in_room->name, in_obj->in_room->vnum);
	    else
	      sprintf( buf, "%s is in %s\n\r",
	      obj->short_descr,
	      in_obj->in_room == NULL
		? "somewhere" : in_obj->in_room->name );
	
	buf[0] = UPPER(buf[0]);
	add_buf(buffer,buf);
	if (number >= max_found)
	    break;
    }

    add_buf(buffer, "\n");
    if ( !found ) add_buf(buffer, "We regret to inform you that our inquiries have yielded no results.\n\r");
    else add_buf(buffer, "We hope you are pleased with our services and will make use of them again.\n\r");
    pnote->text = str_dup(buf_string(buffer));
    free_buf(buffer);	
    pnote->next = NULL;
    strtime = ctime( &current_time );
    strtime[strlen(strtime)-1] = '\0';
    pnote->date = str_dup( strtime );
    pnote->date_stamp = current_time;
    append_note(pnote);
	
    send_to_char("You have a new note waiting.\n\r", ch);
    return;
}

int count_spool(CHAR_DATA *ch, NOTE_DATA *spool)
{
    int count = 0;
    NOTE_DATA *pnote;

    for (pnote = spool; pnote != NULL; pnote = pnote->next)
	if (!hide_note(ch,pnote))
	    count++;

    return count;
}

void do_unread(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int count;
    bool found = FALSE;

    if (IS_NPC(ch))
	return; 

    if ((count = count_spool(ch,news_list)) > 0)
    {
	found = TRUE;
	sprintf(buf,"There %s %d new news article%s waiting.\n\r",
	    count > 1 ? "are" : "is",count, count > 1 ? "s" : "");
	send_to_char(buf,ch);
    }
    if ((count = count_spool(ch,changes_list)) > 0)
    {
	found = TRUE;
	sprintf(buf,"There %s %d change%s waiting to be read.\n\r",
	    count > 1 ? "are" : "is", count, count > 1 ? "s" : "");
        send_to_char(buf,ch);
    }
    if ((count = count_spool(ch,note_list)) > 0)
    {
	found = TRUE;
	sprintf(buf,"You have %d new note%s waiting.\n\r",
	    count, count > 1 ? "s" : "");
	send_to_char(buf,ch);
    }
    if ((count = count_spool(ch,idea_list)) > 0)
    {
	found = TRUE;
	sprintf(buf,"You have %d unread idea%s to peruse.\n\r",
	    count, count > 1 ? "s" : "");
	send_to_char(buf,ch);
    }
    if ((count = count_spool(ch,roomnote_list)) > 0)
    {
	found = TRUE;
	sprintf(buf,"There %s %d roomnote%s waiting to be read.\n\r",
	    count > 1 ? "are" : "is", count, count > 1 ? "s" : "");
        send_to_char(buf,ch);
    }
    if (IS_TRUSTED(ch,ANGEL) && (count = count_spool(ch,penalty_list)) > 0)
    {
	found = TRUE;
	sprintf(buf,"%d %s been added.\n\r",
	    count, count > 1 ? "penalties have" : "penalty has");
	send_to_char(buf,ch);
    }

    if (!found && (argument[0] == '\0' || str_cmp(argument, "new")))
	send_to_char("You have no unread notes.\n\r",ch);
}

void do_note(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_NOTE);
}

void do_idea(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_IDEA);
}

void do_penalty(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_PENALTY);
}

void do_news(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_NEWS);
}

void do_changes(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_CHANGES);
}

void do_rnote(CHAR_DATA *ch, char *argument)
{
    parse_note(ch,argument,NOTE_ROOM);
}

void save_notes(int type)
{
    FILE *fp;
    char *name;
    NOTE_DATA *pnote;

    if (type == -1)
    {
	save_notes(NOTE_NOTE);
	save_notes(NOTE_ROOM);
	return;
    }

    switch (type)
    {
	default:
	    return;
	case NOTE_NOTE:
	    name = NOTE_FILE;
	    pnote = note_list;
	    break;
	case NOTE_IDEA:
	    name = IDEA_FILE;
	    pnote = idea_list;
	    break;
	case NOTE_PENALTY:
	    name = PENALTY_FILE;
	    pnote = penalty_list;
	    break;
	case NOTE_NEWS:
	    name = NEWS_FILE;
	    pnote = news_list;
	    break;
	case NOTE_CHANGES:
	    name = CHANGES_FILE;
	    pnote = changes_list;
	    break;
	case NOTE_ROOM:
	    name = ROOMNOTE_FILE;
	    pnote = roomnote_list;
	    break;
    }

    fclose( fpReserve );
    if ( ( fp = fopen( name, "w" ) ) == NULL )
    {
	perror( name );
    }
    else
    {
	for ( ; pnote != NULL; pnote = pnote->next )
	{
	    fprintf( fp, "Sender  %s~\n", fwrite_format(pnote->sender));
	    fprintf( fp, "Forger  %s~\n", fwrite_format(pnote->forger));
	    fprintf( fp, "Forged  %i\n",  pnote->forged);
	    fprintf( fp, "Date    %s~\n", fwrite_format(pnote->date));
	    fprintf( fp, "Stamp   %ld\n", (long) pnote->date_stamp);
	    fprintf( fp, "Flags   %s\n", print_flags(pnote->flags));
	    fprintf( fp, "To      %s~\n", fwrite_format(pnote->to_list));
	    fprintf( fp, "Subject %s~\n", fwrite_format(pnote->subject));
	    fprintf( fp, "Text\n%s~\n",   fwrite_format(pnote->text));
	}
	fclose( fp );
	fpReserve = fopen( NULL_FILE, "r" );
   	return;
    }
}

void load_notes(void)
{
    load_thread(NOTE_FILE,&note_list, NOTE_NOTE, 14*24*60*60);
    load_thread(IDEA_FILE,&idea_list, NOTE_IDEA, 28*24*60*60);
    load_thread(PENALTY_FILE,&penalty_list, NOTE_PENALTY, 0);
    load_thread(NEWS_FILE,&news_list, NOTE_NEWS, 0);
    load_thread(CHANGES_FILE,&changes_list,NOTE_CHANGES, 0);
    load_thread(ROOMNOTE_FILE,&roomnote_list, NOTE_ROOM, 0);
}

void load_thread(char *name, NOTE_DATA **list, int type, time_t free_time)
{
    FILE *fp;
    NOTE_DATA *pnotelast;
    int elvl;
    char buf[MAX_STRING_LENGTH];
 
    if ( ( fp = fopen( name, "r" ) ) == NULL )
	return;
	 
    pnotelast = NULL;
    for ( ; ; )
    {
	NOTE_DATA *pnote;
	char letter;
	 
	do
	{
	    letter = getc( fp );
            if ( feof(fp) )
            {
                fclose( fp );
                return;
            }
        }
        while ( isspace(letter) );
        ungetc( letter, fp );
 
        pnote           = (NOTE_DATA*)alloc_perm( sizeof(*pnote) );
	g_num_notes++;

	elvl = 0; 
	for ( ; ; )
	{
	    strcpy(buf, fread_word(fp));

            if ( !str_cmp( buf, "sender" ) )
	        pnote->sender   = fread_string( fp );
	    else if ( !str_cmp( buf, "forger" ) )
		pnote->forger   = fread_string( fp );
	    else if ( !str_cmp( buf, "forged" ) )
		pnote->forged   = fread_number( fp );
	    else if ( !str_cmp( buf, "date" ) )
	        pnote->date     = fread_string( fp );
            else if ( !str_cmp( buf, "stamp" ) )
                pnote->date_stamp = fread_number(fp);
	    else if ( !str_cmp( buf, "flags" ) )
		pnote->flags = fread_flag(fp);
	    else if ( !str_cmp( buf, "to" ) )
                pnote->to_list  = fread_string( fp );
	    else if ( !str_cmp( buf, "subject" ) )
                pnote->subject  = fread_string( fp );
	    else if ( !str_cmp( buf, "faketo" ) )
		pnote->fake_to  = fread_string( fp );
	    else if ( !str_cmp( buf, "text" ) )
	    {
                pnote->text     = fread_string( fp );
 
                if (free_time && !IS_SET(pnote->flags, NFLAG_STICKY)
                 && pnote->date_stamp < current_time - free_time)
                {
	            free_note(pnote);
		    elvl = 1;
                }
		break;
	    }
	    else
	    {
	        free_note(pnote);
		elvl = 2;
	        break;
	    }
	}

	if (!elvl)
	{
	    pnote->type = type;
  
            if (*list == NULL)
                *list           = pnote;
            else
                pnotelast->next     = pnote;
 
            pnotelast       = pnote;
	}
	else if (elvl == 2)
	    break;
    }
 
    strcpy( strArea, NOTE_FILE );
    fpArea = fp;
    bug( "Load_notes: bad key word.", 0 );
    exit( 1 );
    return;
}

void append_note(NOTE_DATA *pnote)
{
    FILE *fp;
    char *name;
    NOTE_DATA **list;
    NOTE_DATA *last;

    switch(pnote->type)
    {
	default:
	    return;
	case NOTE_NOTE:
	    name = NOTE_FILE;
	    list = &note_list;
	    break;
	case NOTE_IDEA:
	    name = IDEA_FILE;
	    list = &idea_list;
	    break;
	case NOTE_PENALTY:
	    name = PENALTY_FILE;
	    list = &penalty_list;
	    break;
	case NOTE_NEWS:
	     name = NEWS_FILE;
	     list = &news_list;
	     break;
	case NOTE_CHANGES:
	     name = CHANGES_FILE;
	     list = &changes_list;
	     break;
	case NOTE_ROOM:
	     name = ROOMNOTE_FILE;
	     list = &roomnote_list;
    }

    if (*list == NULL)
	*list = pnote;
    else
    {
	for ( last = *list; last->next != NULL; last = last->next);
	last->next = pnote;
    }

    fclose(fpReserve);
    if ( ( fp = fopen(name, "a" ) ) == NULL )
    {
        perror(name);
    }
    else
    {
        fprintf( fp, "Sender  %s~\n", pnote->sender);
	fprintf( fp, "Forger  %s~\n", pnote->forger);
	fprintf( fp, "Forged  %i\n", pnote->forged);
        fprintf( fp, "Date    %s~\n", pnote->date);
        fprintf( fp, "Stamp   %ld\n", (long) pnote->date_stamp);
	fprintf( fp, "Flags   %s\n", print_flags(pnote->flags));
// brazen: Ticket #252: This input sanitation makes it possible
// to use the tilde character in notes. fwrite_format parses the
// tilde character into \$ before being written to the file.
        fprintf( fp, "To      %s~\n", fwrite_format(pnote->to_list));
        fprintf( fp, "Subject %s~\n", fwrite_format(pnote->subject));

	if (pnote->fake_to)
	    fprintf( fp, "FakeTo %s~\n", fwrite_format(pnote->fake_to));

        fprintf( fp, "Text\n%s~\n", fwrite_format(pnote->text));
        fclose( fp );
    }
    fpReserve = fopen( NULL_FILE, "r" );
}

bool is_note_to( CHAR_DATA *ch, NOTE_DATA *pnote )
{
    int i;
    OBJ_DATA *sigil;
    char buf[80];
    char *str_list;

    if ( !str_cmp( ch->name, pnote->sender ) 
      && (pnote->forger == NULL 
	|| pnote->forger && pnote->forger[0] == '\0') )
	return TRUE;

    if (pnote->to_list == NULL)
	return FALSE;

    if ( is_name( "all", pnote->to_list ) /*&& IS_IMMORTAL(ch)*/ )
	return TRUE;

    if ( IS_IMMORTAL(ch) && (is_name( "immortal", pnote->to_list ) || is_name( "immortals", pnote->to_list ) || is_name("imm", pnote->to_list) || is_name("imms", pnote->to_list)))
	return TRUE;

    if (ch->clan)
    {
	if (is_name(clan_table[ch->clan].who_name,pnote->to_list))
	    return TRUE;

	sprintf(buf, "%ss", clan_table[ch->clan].who_name);
	if (is_name(buf, pnote->to_list))
	    return TRUE;
    }

    if (is_name(race_table[ch->race].name, pnote->to_list))
	return TRUE;

    if ( is_name( ch->name, pnote->to_list ) )
	return TRUE;
    
    str_list = pnote->to_list;
    while (*str_list != '\0')
    {
	str_list = one_argument(str_list, buf);

	if (strlen(buf) <= 5)
	    continue;

	if (!str_prefix("sigil", buf))
	{
	    if (((i = atoi(buf+5)) < 1) || ((sigil = get_eq_char(ch, WEAR_SIGIL)) == NULL))
		continue;

	    if (sigil->pIndexData->vnum == i)
		return TRUE;
	
	    continue;
	}

	if (!str_prefix("level", buf))
	{
	    if ((i = atoi(buf+5)) < 1)
		continue;

	    if (get_trust(ch) >= i)
		return TRUE;

	    continue;
	}
    }
	    
    
    return FALSE;
}



void note_attach( CHAR_DATA *ch, int type )
{
    NOTE_DATA *pnote;

    if ( ch->pnote != NULL )
	return;

    pnote = new_note();

    pnote->next		= NULL;
    pnote->sender	= IS_NPC(ch) ? str_dup(ch->short_descr) : str_dup( ch->name );
    pnote->forger	= str_dup( "" );
    pnote->forged	= 0;
    pnote->date		= str_dup( "" );
    pnote->to_list	= str_dup( "" );
    pnote->subject	= str_dup( "" );
    pnote->text		= str_dup( "" );
    pnote->fake_to	= NULL;
    pnote->type		= type;
    ch->pnote		= pnote;
    return;
}



void note_remove( CHAR_DATA *ch, NOTE_DATA *pnote, bool shouldDelete)
{
    char to_new[MAX_INPUT_LENGTH];
    char to_one[MAX_INPUT_LENGTH];
    NOTE_DATA *prev;
    NOTE_DATA **list;
    char *to_list;

    if (!shouldDelete)
    {
	/* make a new list */
        to_new[0]	= '\0';
        to_list	= pnote->to_list;
        while ( *to_list != '\0' )
        {
    	    to_list = one_argument( to_list, to_one );
    	    if ( to_one[0] != '\0' && str_cmp( ch->name, to_one ) )
	    {
	        strcat( to_new, " " );
	        strcat( to_new, to_one );
	    }
        }
        /* Just a simple recipient removal? */
       if ( str_cmp( ch->name, pnote->sender ) && to_new[0] != '\0' )
       {
	   free_string( pnote->to_list );
	   pnote->to_list = str_dup( to_new + 1 );
	   return;
       }
    }
    /* nuke the whole note */

    switch(pnote->type)
    {
	default:
	    return;
	case NOTE_NOTE:
	    list = &note_list;
	    break;
	case NOTE_IDEA:
	    list = &idea_list;
	    break;
	case NOTE_PENALTY:
	    list = &penalty_list;
	    break;
	case NOTE_NEWS:
	    list = &news_list;
	    break;
	case NOTE_CHANGES:
	    list = &changes_list;
	    break;
	case NOTE_ROOM:
	    list = &roomnote_list;
	    break;
    }

    /*
     * Remove note from linked list.
     */
    if ( pnote == *list )
    {
	*list = pnote->next;
    }
    else
    {
	for ( prev = *list; prev != NULL; prev = prev->next )
	{
	    if ( prev->next == pnote )
		break;
	}

	if ( prev == NULL )
	{
	    bug( "Note_remove: pnote not found.", 0 );
	    return;
	}

	prev->next = pnote->next;
    }

    save_notes(pnote->type);
    free_note(pnote);
    return;
}

bool hide_note (CHAR_DATA *ch, NOTE_DATA *pnote)
{
    time_t last_read;

    if (IS_NPC(ch))
	return TRUE;

    switch (pnote->type)
    {
	default:
	    return TRUE;
	case NOTE_NOTE:
	    last_read = ch->pcdata->last_note;
	    break;
	case NOTE_IDEA:
	    last_read = ch->pcdata->last_idea;
	    break;
	case NOTE_PENALTY:
	    last_read = ch->pcdata->last_penalty;
	    break;
	case NOTE_NEWS:
	    last_read = ch->pcdata->last_news;
	    break;
	case NOTE_CHANGES:
	    last_read = ch->pcdata->last_changes;
	    break;
	case NOTE_ROOM:
	    last_read = ch->pcdata->last_roomnote;
	    break;
    }
    
    if (pnote->date_stamp <= last_read)
	return TRUE;

    if (!str_cmp(ch->name,pnote->sender))
	return TRUE;

    if (!is_note_to(ch,pnote))
	return TRUE;

    return FALSE;
}

void update_read(CHAR_DATA *ch, NOTE_DATA *pnote)
{
    time_t stamp;

    if (IS_NPC(ch))
	return;

    stamp = pnote->date_stamp;

    switch (pnote->type)
    {
        default:
            return;
        case NOTE_NOTE:
	    ch->pcdata->last_note = UMAX(ch->pcdata->last_note,stamp);
            break;
        case NOTE_IDEA:
	    ch->pcdata->last_idea = UMAX(ch->pcdata->last_idea,stamp);
            break;
        case NOTE_PENALTY:
	    ch->pcdata->last_penalty = UMAX(ch->pcdata->last_penalty,stamp);
            break;
        case NOTE_NEWS:
	    ch->pcdata->last_news = UMAX(ch->pcdata->last_news,stamp);
            break;
        case NOTE_CHANGES:
	    ch->pcdata->last_changes = UMAX(ch->pcdata->last_changes,stamp);
            break;
	case NOTE_ROOM:
	    ch->pcdata->last_roomnote = UMAX(ch->pcdata->last_roomnote,stamp);
	    break;
    }
}

bool can_send_to(CHAR_DATA *ch, char *argument)
{
    char *str_list;
    char buf[MAX_INPUT_LENGTH];
    int i;

    if (IS_IMMORTAL(ch))
	return TRUE;

    for (i = 0; i < MAX_PC_RACE; i++)
	if (!str_cmp(argument, pc_race_table[i].name))
	{
    	    send_to_char("You are not allowed to pen a note to an entire race.\n\r", ch);
	    return FALSE;
	}

    str_list = argument;
    while (*str_list != '\0')
    {
	str_list = one_argument(str_list, buf);

	if (!str_prefix("sigil", buf))
	{
	    send_to_char("You cannot pen notes to bearers of sigils.\n\r", ch);
	    return FALSE;
	}

	if (!str_prefix("level", buf))
	{
	    send_to_char("You cannot pen notes based on level.\n\r", ch);
	    return FALSE;
	}
	if (!str_cmp("all", buf) && ((ch->class_num != global_int_class_bard) || (ch->level < 40)))
	{
	    send_to_char("Only the experienced heralds of the land can send notes to everyone.\n\r",ch);
	    return FALSE;
	}
    }

    return TRUE;
}

void show_note(CHAR_DATA *ch, NOTE_DATA *pnote, int vnum)
{
    char buf[MAX_STRING_LENGTH];

    sprintf( buf, "[%3d] %s: %s\n\r%s\n\r",
	vnum,
	note_from(ch,pnote),
        pnote->subject,
        pnote->date);

    if (ch->level > 51 || (!pnote->fake_to && !is_name("hidden", pnote->to_list)))
    {
	strcat(buf, "To: ");
	strcat(buf, pnote->to_list);
	strcat(buf, "\n\r");
    }

    if (pnote->fake_to)
    {
	strcat(buf, "To: ");
	strcat(buf, pnote->fake_to);
	strcat(buf, "\n\r");
    }

    send_to_char( buf, ch );
    page_to_char( pnote->text, ch );
}

void parse_note( CHAR_DATA *ch, char *argument, int type )
{
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    NOTE_DATA *pnote = NULL;
    NOTE_DATA **list;
    char *list_name;
    int vnum;
    int anum;

    argument = one_argument( argument, arg );
    smash_tilde( argument );

    switch(type)
    {
	default:
	    return;
        case NOTE_NOTE:
            list = &note_list;
	    list_name = "notes";
            break;
        case NOTE_IDEA:
	    send_to_char("Use notes to immortal.\n\r", ch);
	    return;
        case NOTE_PENALTY:
	    send_to_char("Use notes to immortal.\n\r", ch);
	    return;
        case NOTE_NEWS:
	    send_to_char("Check the MOTD and help RECENT.\n\r", ch);
	    return;
        case NOTE_CHANGES:
	    send_to_char("Check the MOTD and help RECENT.\n\r", ch);
	    return;
	case NOTE_ROOM:
	    list = &roomnote_list;
	    list_name = "roomnotes";
	    break;
    }

    if ( arg[0] == '\0' || !str_prefix( arg, "read" ) )
    {
        bool fAll, use_name = FALSE;
 
        if ( !str_cmp( argument, "all" ) )
        {
            fAll = TRUE;
            anum = 0;
        }
 
        else if ( argument[0] == '\0' || !str_prefix(argument, "next"))
        /* read next unread note */
        {
            vnum = 0;
            for ( pnote = *list; pnote != NULL; pnote = pnote->next)
            {
                if (!hide_note(ch,pnote))
                {
		    show_note(ch, pnote, vnum);
		    ch->lastnote = pnote;
                    update_read(ch,pnote);
                    return;
                }
                else if (is_note_to(ch,pnote))
                    vnum++;
            }
	    sprintf(buf,"You have no unread %s.\n\r",list_name);
	    send_to_char(buf,ch);
            return;
        }
 
        else if ( is_number( argument ) )
        {
            fAll = FALSE;
            anum = atoi( argument );
        }
        else
        {
	    if (IS_IMM_TRUST(ch))
	    {
		argument = one_argument(argument, arg);

		if (!is_number(argument))
		{
		    send_to_char("Read which number?\n\r", ch);
		    return;
		}

		fAll = FALSE;
		anum = atoi(argument);

		use_name = TRUE;
	    }
	    else
	    {
                send_to_char( "Read which number?\n\r", ch );
                return;
	    }
        }
 
        vnum = 0;
        for ( pnote = *list; pnote != NULL; pnote = pnote->next )
        {
            if ( (use_name ? is_name(arg, pnote->to_list) : is_note_to( ch, pnote )) && ( vnum++ == anum || fAll ) )
            {
		show_note(ch, pnote, vnum - 1);
		ch->lastnote = pnote;
		update_read(ch,pnote);
                return;
            }
        }
 
	sprintf(buf,"There aren't that many %s.\n\r",list_name);
	send_to_char(buf,ch);
        return;
    }

    if ( !str_prefix( arg, "list" ) )
    {
	bool found = FALSE, use_name = FALSE;
	char dbuf[15];

	if (IS_IMM_TRUST(ch) && argument[0] != '\0')
	    use_name = TRUE;

	vnum = 0;
	for ( pnote = *list; pnote != NULL; pnote = pnote->next )
	{
	    if ( use_name ? is_name(argument, pnote->to_list) : is_note_to( ch, pnote ) )
	    {
		if (!found)
		{
		    send_to_char("[Note] From          Subject                                  Date     Time\n\r", ch);
		    send_to_char("============================================================================\n\r", ch);
		    found = TRUE;
		}

		strftime(dbuf, 15, "%m/%d/%y %H:%M", localtime(&pnote->date_stamp));

		sprintf( buf, "[%3d%s] %-12.12s  %-40.40s %-15s\n\r", vnum, hide_note(ch, pnote) ? (IS_SET(pnote->flags, NFLAG_STICKY) ? "S" : " ") : "N",
			note_from(ch,pnote),
			pnote->subject, dbuf);
		
		send_to_char( buf, ch );
		vnum++;
	    }
	}
	if (!vnum)
	{
	    switch(type)
	    {
		case NOTE_NOTE:	
		    sprintf(buf, "There are no notes for %s.\n\r", use_name ? argument : "you");
		    break;
		case NOTE_IDEA:
		    sprintf(buf, "There are no ideas for %s.\n\r", use_name ? argument : "you");
		    break;
		case NOTE_PENALTY:
		    sprintf(buf, "There are no penalties for %s.\n\r", use_name ? argument : "you");
		    break;
		case NOTE_NEWS:
		    sprintf(buf, "There is no news for %s.\n\r", use_name ? argument : "you");
		    break;
		case NOTE_CHANGES:
		    sprintf(buf, "There are no changes for %s.\n\r", use_name ? argument : "you");
		    break;
		case NOTE_ROOM:
		    sprintf(buf, "There are no room notes waiting.\n\r");
		    break;
	    }

	    send_to_char(buf, ch);
	}
	return;
    }

    if ( !str_prefix( arg, "remove" ) )
    {
        if ( !is_number( argument ) )
        {
            send_to_char( "Note remove which number?\n\r", ch );
            return;
        }
 
        anum = atoi( argument );
        vnum = 0;
        for ( pnote = *list; pnote != NULL; pnote = pnote->next )
        {
            if ( is_note_to( ch, pnote ) && vnum++ == anum )
            {
                note_remove( ch, pnote, FALSE );
                send_to_char( "Ok.\n\r", ch );
                return;
            }
        }
 
	sprintf(buf,"There aren't that many %s.\n\r",list_name);
	send_to_char(buf,ch);
        return;
    }
 
    if ( !str_prefix( arg, "delete" ) && get_trust(ch) >= MAX_LEVEL - 1)
    {
        if ( !is_number( argument ) )
        {
            send_to_char( "Note delete which number?\n\r", ch );
            return;
        }
 
        anum = atoi( argument );
        vnum = 0;
        for ( pnote = *list; pnote != NULL; pnote = pnote->next )
        {
            if (is_note_to(ch,pnote) && vnum++ == anum)
            {
                note_remove( ch, pnote,TRUE );
                send_to_char( "Ok.\n\r", ch );
                return;
            }
        }

 	sprintf(buf,"There aren't that many %s.",list_name);
	send_to_char(buf,ch);
        return;
    }

    if (!str_prefix(arg,"catchup"))
    {
	switch(type)
	{
	    case NOTE_NOTE:	
		ch->pcdata->last_note = current_time;
		break;
	    case NOTE_IDEA:
		ch->pcdata->last_idea = current_time;
		break;
	    case NOTE_PENALTY:
		ch->pcdata->last_penalty = current_time;
		break;
	    case NOTE_NEWS:
		ch->pcdata->last_news = current_time;
		break;
	    case NOTE_CHANGES:
		ch->pcdata->last_changes = current_time;
		break;
	    case NOTE_ROOM:
		ch->pcdata->last_roomnote = current_time;
		break;
	}
	return;
    }

    if (type == NOTE_ROOM && !str_prefix(arg, "approve"))
    {
	char *kstr;
	ROOM_INDEX_DATA *pRoom;
	EXTRA_DESCR_DATA *ed;
	int rnum;

        if ( !is_number( argument ) )
        {
            send_to_char( "Note approve which number?\n\r", ch );
            return;
        }
 
        anum = atoi( argument );
        vnum = 0;
        for ( pnote = *list; pnote != NULL; pnote = pnote->next )
        {
            if ( is_note_to( ch, pnote ) && vnum++ == anum )
            {
		kstr = one_argument(pnote->subject, arg);

		rnum = atoi(arg);

		if ((pRoom = get_room_index(rnum)) == NULL)
		{
		    send_to_char("Error editing room description: room not found.\n\r", ch);
		    return;
		}

		if (!kstr || kstr[0] == '\0')    // main room description
		{
		    free_string(pRoom->description);
		    pRoom->description = str_dup(pnote->text);
		    send_to_char("Room description changed.\n\r", ch);
		}
		else
		{
		    for ( ed = pRoom->extra_descr; ed; ed = ed->next )
		    {
	    		if ( is_name( kstr, ed->keyword ) )
			    break;
		    }
		    
		    if (!ed)
		    {
			ed			=   new_extra_descr();
			ed->keyword		=   str_dup( kstr );
			ed->next		=   pRoom->extra_descr;
			pRoom->extra_descr	=   ed;
			send_to_char("Room exdesc added.\n\r", ch);
		    }
		    else
		    {
			free_string(ed->description);
			send_to_char("Room exdesc changed.\n\r", ch);
		    }

		    ed->description = str_dup(pnote->text);
		}

		save_area(pRoom->area);
		save_room_desc(pRoom);

                note_remove( ch, pnote, TRUE );
                return;
            }
        }
 
	sprintf(buf,"There aren't that many %s.",list_name);
	send_to_char(buf,ch);
        return;
    }	

    if (type == NOTE_ROOM)
    {
	send_to_char("You may only read, list, delete, and approve roomnotes.\n\r", ch);
	return;
    }

    /* below this point only certain people can edit notes */
    if ((type == NOTE_NEWS && !IS_TRUSTED(ch,ANGEL))
    ||  (type == NOTE_CHANGES && !IS_TRUSTED(ch,CREATOR)))
    {
	sprintf(buf,"You aren't high enough level to write %s.",list_name);
	send_to_char(buf,ch);
	return;
    }

    if ( !str_cmp( arg, "edit") )
    {
	note_attach(ch, type);
	if (ch->pnote->type != type)
	{
	    send_to_char("You already have a different note in progress.\n\r", ch);
	    return;
	}
	string_append(ch, &ch->pnote->text);
	return;
    }

    if ( !str_cmp( arg, "+" ) )
    {
	note_attach( ch,type );
	if (ch->pnote->type != type)
	{
	    send_to_char(
		"You already have a different note in progress.\n\r",ch);
	    return;
	}

	if (strlen(ch->pnote->text)+strlen(argument) >= 8192)
	{
	    send_to_char( "Note too long.\n\r", ch );
	    return;
	}

 	buffer = new_buf();

	add_buf(buffer,ch->pnote->text);
	add_buf(buffer,argument);
	add_buf(buffer,"\n\r");
	free_string( ch->pnote->text );
	ch->pnote->text = str_dup( buf_string(buffer) );
	free_buf(buffer);
	send_to_char( "Ok.\n\r", ch );
	return;
    }

    if (!str_cmp(arg,"-"))
    {
 	int len;
	bool found = FALSE;

	note_attach(ch,type);
        if (ch->pnote->type != type)
        {
            send_to_char(
                "You already have a different note in progress.\n\r",ch);
            return;
        }

	if (ch->pnote->text == NULL || ch->pnote->text[0] == '\0')
	{
	    send_to_char("No lines left to remove.\n\r",ch);
	    return;
	}

	strcpy(buf,ch->pnote->text);

	for (len = strlen(buf); len > 0; len--)
 	{
	    if (buf[len] == '\r')
	    {
		if (!found)  /* back it up */
		{
		    if (len > 0)
			len--;
		    found = TRUE;
		}
		else /* found the second one */
		{
		    buf[len + 1] = '\0';
		    free_string(ch->pnote->text);
		    ch->pnote->text = str_dup(buf);
		    return;
		}
	    }
	}
	buf[0] = '\0';
	free_string(ch->pnote->text);
	ch->pnote->text = str_dup(buf);
	return;
    }

    if ( !str_prefix( arg, "subject" ) )
    {
	note_attach( ch,type );
        if (ch->pnote->type != type)
        {
            send_to_char(
                "You already have a different note in progress.\n\r",ch);
            return;
        }

	free_string( ch->pnote->subject );
	ch->pnote->subject = str_dup( argument );
	send_to_char( "Ok.\n\r", ch );
	return;
    }

    if (!str_cmp(arg, "sticky") && IS_IMMORTAL(ch))
    {
	if (!is_number(argument))
	{
            note_attach( ch,type );
            if (ch->pnote->type != type)
            {
                send_to_char("You already have a different note in progress.\n\r",ch);
                 return;
            }

	    if (IS_SET(ch->pnote->flags, NFLAG_STICKY))
	    {
		send_to_char("Note in progress is no longer flagged 'sticky'.\n\r", ch);
		REMOVE_BIT(ch->pnote->flags, NFLAG_STICKY);
	    }
	    else
	    {
		send_to_char("Note in progress is now flagged 'sticky'.\n\r", ch);
		SET_BIT(ch->pnote->flags, NFLAG_STICKY);
	    }
	}
	else
	{
	    anum = atoi( argument );
	    vnum = 0;
	    for ( pnote = *list; pnote != NULL; pnote = pnote->next )
	    {
		if (is_note_to(ch,pnote) && vnum++ == anum)
		{
		    if (IS_SET(pnote->flags, NFLAG_STICKY))
		    {
			sprintf(buf, "Note #%d is no longer flagged 'sticky'.\n\r", anum);
			send_to_char(buf, ch);
			REMOVE_BIT(pnote->flags, NFLAG_STICKY);
		    }
		    else
		    {
			sprintf(buf, "Note #%d is now flagged 'sticky'.\n\r", anum);
			send_to_char(buf, ch);
			SET_BIT(pnote->flags, NFLAG_STICKY);
		    }

		    save_notes(pnote->type);
		    return;
		}
	    }

 	    sprintf(buf,"There aren't that many %s.",list_name);
	    send_to_char(buf,ch);
            return;
	}

	return;
    }

    if ( !str_prefix( arg, "from" ) && (IS_IMMORTAL(ch)))
    {
        note_attach( ch,type );
        if (ch->pnote->type != type)
        {
            send_to_char(
                "You already have a different note in progress.\n\r",ch);
             return;
        }
        free_string( ch->pnote->sender );
        ch->pnote->sender = str_dup( argument );
        send_to_char( "Ok.\n\r", ch );
        return;
    }
    if (!str_prefix(arg, "forge" ))
    {
	int chance;
	if ((chance = get_skill(ch,gsn_forgery)) == 0)
	{
	    send_to_char("You have no skill at forging notes.\n\r",ch);
	    return;
	}
        note_attach( ch,type );
	if (ch->pnote->type != type)
	{
	    send_to_char("You already have a different note in progress.\n\r",ch);
	    return;
	}
	if (number_percent() < chance)
	    ch->pnote->forged = 1;
	else
	    ch->pnote->forged = 0;
	for (chance = 0;chance < MAX_GODS;chance++)
	    if (god_table[chance].can_follow
	      && is_name(argument,god_table[chance].name))
	    {
		if (ch->hit > 1)
		    ch->hit /= 2;
		if (ch->mana > 1)
		    ch->mana /= 2;
		if (ch->move > 1)
		    ch->move /= 2;
		send_to_char("Bad idea.\n\r",ch);
		sprintf(buf, "%s tried to forge a note from %s, and was smited from the heavens.", IS_NPC(ch) ? ch->short_descr : ch->name, god_table[chance].name);
		wiznet(buf, ch, NULL, WIZ_ROLEPLAY, 0, 0);
		log_string(buf);
		free_note(ch->pnote);
		return;
	    }
	if (is_name(argument, "immortal"))
	{
	    if (ch->hit > 1)
	        ch->hit /= 2;
	    if (ch->mana > 1)
	        ch->mana /= 2;
	    if (ch->move > 1)
	        ch->move /= 2;
	    send_to_char("Bad idea.\n\r",ch);
	    sprintf(buf, "%s tried to forge a note from immortal, and was smited from the heavens.", IS_NPC(ch) ? ch->short_descr : ch->name);
	    wiznet(buf, ch, NULL, WIZ_ROLEPLAY, 0, 0);
	    log_string(buf);
	    free_note(ch->pnote);
	    return;
	}
	free_string( ch->pnote->sender );
	ch->pnote->sender = str_dup(argument);
	ch->pnote->forger = str_dup(ch->name);
	check_improve(ch,ch,gsn_forgery,TRUE,1);
	send_to_char("You painstakingly forge the note to your satisfaction.\n\r",ch);
	WAIT_STATE(ch,UMAX(ch->wait,PULSE_VIOLENCE));
	return;
    }
    if (!str_prefix(arg, "faketo") && IS_IMMORTAL(ch))
    {
	note_attach(ch, type);
        if (ch->pnote->type != type)
        {
            send_to_char("You already have a different note in progress.\n\r",ch);
            return;
        }

	free_string(ch->pnote->fake_to);
	if (argument[0] != '\0')
	    ch->pnote->fake_to = str_dup(argument);
	else
	    ch->pnote->fake_to = NULL;

	send_to_char("Ok.\n\r", ch);
	return;
    }

    if ( !str_prefix( arg, "to" ) )
    {
	note_attach( ch,type );

        if (ch->pnote->type != type)
        {
            send_to_char(
                "You already have a different note in progress.\n\r",ch);
            return;
        }

	if (!can_send_to(ch, argument))
	    return;

	free_string( ch->pnote->to_list );
	ch->pnote->to_list = str_dup( argument );
	send_to_char( "Ok.\n\r", ch );
	return;
    }

    if ( !str_prefix( arg, "clear" ) )
    {
	if ( ch->pnote != NULL )
	{
	    free_note(ch->pnote);
	    ch->pnote = NULL;
	}

	send_to_char( "Ok.\n\r", ch );
	return;
    }

    if ( !str_prefix( arg, "show" ) )
    {
	if ( ch->pnote == NULL )
	{
	    send_to_char( "You have no note in progress.\n\r", ch );
	    return;
	}

	if (ch->pnote->type != type)
	{
	    send_to_char("You aren't working on that kind of note.\n\r",ch);
	    return;
	}

	sprintf( buf, "%s: %s\n\rTo: %s\n\r",
	    ch->pnote->sender,
	    ch->pnote->subject,
	    ch->pnote->to_list
	    );

	if (ch->pnote->fake_to)
	{
	    strcat(buf, "FakeTo: ");
	    strcat(buf, ch->pnote->fake_to);
	    strcat(buf, "\n\r");
	}

	send_to_char( buf, ch );
	send_to_char( ch->pnote->text, ch );
	return;
    }

    if (!str_cmp(arg, "reply") || !str_cmp(arg, "replyall"))
    {
	NOTE_DATA *rnote = NULL;
	char *rp, *pp;

	if (argument[0] == '\0' || !is_number(argument))
	{
	    if (!ch->lastnote)
	    {
	        send_to_char("You do not have a last read note to reply to.  Use 'note reply/replyall <number>'.\n\r", ch);
		return;
	    }

	    rnote = ch->lastnote;
	}
	else
	{
            anum = atoi( argument );
            vnum = 0;

            for ( pnote = *list; pnote != NULL; pnote = pnote->next )
                if ( is_note_to( ch, pnote ) && vnum++ == anum )
                {
                    rnote = pnote;
		    break;
                }

	    if (!rnote)
	    {
	        sprintf(buf,"There aren't that many %s.\n\r",list_name);
		send_to_char(buf,ch);
		return;
	    }
	}

	note_attach(ch, type);

        if (ch->pnote->type != type)
        {
            send_to_char("You already have a different note in progress.\n\r",ch);
            return;
        }
 
	if (!str_cmp(arg, "replyall"))
	{
	    if ((rnote->fake_to || is_name("hidden", rnote->to_list)) && !IS_IMMORTAL(ch))
	    {
		send_to_char("You cannot replyall to this note.\n\r", ch);
		return;
	    }

	    if (!can_send_to(ch, rnote->to_list))
		return;

	    if (strstr(rnote->to_list, rnote->sender))
		strcpy(buf, rnote->to_list);
	    else
	        sprintf(buf, "%s %s", rnote->sender, rnote->to_list);

	    if (rnote->fake_to)
	    {
		if (ch->pnote->fake_to)
		    free_string(ch->pnote->fake_to);

		ch->pnote->fake_to = str_dup(rnote->fake_to);
	    }
	}
	else
	    strcpy(buf, rnote->sender);

	free_string(ch->pnote->to_list);
	ch->pnote->to_list = str_dup( buf );

	sprintf(buf, "Re: %s", rnote->subject);
	free_string( ch->pnote->subject );
	ch->pnote->subject = str_dup( buf );

	strcpy(buf, ">");

	rp = rnote->text;
	pp = buf;

	pp++;

	while (*rp != '\0')
	{
	    *pp++ = *rp;
	    if (*rp == '\n')
	    {
		rp++;

		while (*rp == '\r')
		{
		   *pp++ = *rp;
		   rp++;
		}

		if (*rp != '\0')
		    *pp++ = '>';
	    }
	    else
	        rp++;
	}

	*pp++ = '\n';
	*pp++ = '\r';
	*pp = '\0';

	free_string(ch->pnote->text);
	ch->pnote->text = str_dup(buf);		

	sprintf( buf, "%s: %s\n\rTo: %s\n\r",
	    ch->pnote->sender,
	    ch->pnote->subject,
	    ch->pnote->to_list
	    );
	send_to_char( buf, ch );
	send_to_char( ch->pnote->text, ch );
	send_to_char( "Note is being replied to.  Use \"note +\" or \"note edit\" to continue editing.\n\r", ch);
	return;
    }

    if ( !str_prefix( arg, "post" ) || !str_prefix(arg, "send"))
    {
	char *strtime;
	DESCRIPTOR_DATA *d;
	CHAR_DATA *dch;

	if ( ch->pnote == NULL )
	{
	    send_to_char( "You have no note in progress.\n\r", ch );
	    return;
	}

        if (ch->pnote->type != type)
        {
            send_to_char("You aren't working on that kind of note.\n\r",ch);
            return;
        }

	if (!str_cmp(ch->pnote->to_list,""))
	{
	    send_to_char(
		"You need to provide a recipient (name, all, or immortal).\n\r",
		ch);
	    return;
	}

	if (!str_cmp(ch->pnote->subject,""))
	{
	    send_to_char("You need to provide a subject.\n\r",ch);
	    return;
	}

	ch->pnote->next			= NULL;
	strtime				= ctime( &current_time );
	strtime[strlen(strtime)-1]	= '\0';
	ch->pnote->date			= str_dup( strtime );
	ch->pnote->date_stamp		= current_time;
	if (IS_NPC(ch) && ch->master)
	{
	    ch->pnote->forger = str_dup(ch->master->short_descr);
	    ch->pnote->forged = 1;
	}

	if (ch->pnote->forger[0] != '\0')
	{
	    ch->pnote->fake_to = str_dup(ch->pnote->to_list);
	    sprintf(buf,"%s %s",ch->pnote->fake_to,"forged");
	    free_string(ch->pnote->to_list);
	    ch->pnote->to_list = str_dup(buf);
	}
	append_note(ch->pnote);

	for (d = descriptor_list; d; d = d->next)
	{
	    if ((d->connected == CON_PLAYING)
	     && d->character)
	    {
		dch = d->original ? d->original : d->character;	
		if (!IS_NPC(dch) 
		 && (IS_IMMORTAL(ch) || IS_SET(dch->nact, PLR_NOTEIFY))
		 && (dch != ch) && is_note_to(dch, ch->pnote))
		    send_to_char("You have a new note waiting.\n\r", dch);
	    }
	}

	ch->pnote = NULL;

	send_to_char("Note sent.\n\r", ch);
	return;
    }

    send_to_char( "You can't do that.\n\r", ch );
    return;
}

void do_notify(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (argument[0] == '\0')
    {
	if (IS_SET(ch->nact, PLR_NOTEIFY))
	{
	    send_to_char("You will no longer receive instant new note notification.\n\r", ch);
	    REMOVE_BIT(ch->nact, PLR_NOTEIFY);
	}
	else
	{
	    send_to_char("You will now receive instant new note notification.\n\r", ch);
	    SET_BIT(ch->nact, PLR_NOTEIFY);
	}
	return;
    }

    if (!strcmp(argument, "off"))
    {
	send_to_char("You will no longer receive instant new note notification.\n\r", ch);
	REMOVE_BIT(ch->nact, PLR_NOTEIFY);
	return;
    }

    if (!str_cmp(argument, "on"))
    {
	send_to_char("You will now receive instant new note notification.\n\r", ch);
	SET_BIT(ch->nact, PLR_NOTEIFY);
    }

    send_to_char("Invalid argument.  Use 'notify' without an argument to toggle, or specify 'on' or 'off'.\n\r", ch);
    return;
}

void do_room(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    if (!ch->in_room || !is_name(ch->name, ch->in_room->owner))
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (IS_NPC(ch))
	return;

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "desc"))
    {
	if (ch->pcdata->redit_name && ch->pcdata->redit_name[0] != '\0')
	{
	    send_to_char("You are already editting a room exdesc for submittal.\n\r", ch);
	    send_to_char("Please use 'room submit' to submit for approval, or 'room clear' to cancel.\n\r", ch);
	    return;
	}

	if (!ch->pcdata->redit_desc)
	    ch->pcdata->redit_desc = str_dup("");

	string_append(ch, &ch->pcdata->redit_desc);
	return;
    }
    else if (!str_cmp(arg, "ed"))
    {
	if (ch->pcdata->redit_desc && ch->pcdata->redit_desc[0] != '\0' &&
	 (!ch->pcdata->redit_name || str_cmp(ch->pcdata->redit_name, argument)))
	{
	    send_to_char("You are already working on a different room description.\n\r", ch);
	    send_to_char("Please use 'room submit' to submit for approval, or 'room clear' to cancel.\n\r", ch);
	    return;
	}

	if (!ch->pcdata->redit_name || ch->pcdata->redit_name[0] == '\0')
	    ch->pcdata->redit_name = str_dup(argument);

	if (!ch->pcdata->redit_desc)
	    ch->pcdata->redit_desc = str_dup("");

	string_append(ch, &ch->pcdata->redit_desc);
	return;
    }
    else if (!str_cmp(arg, "show"))
    {
	if ((!ch->pcdata->redit_desc || ch->pcdata->redit_desc[0] == '\0')
	 && (!ch->pcdata->redit_name || ch->pcdata->redit_name[0] == '\0'))
	    send_to_char("You are not currently working on a room description for submittal.\n\r", ch);
	else
	{
	    if (ch->pcdata->redit_name && ch->pcdata->redit_name[0] != '\0')
	    {
		sprintf(buf, "Keyword(s): %s\n\rDescription:\n\r", ch->pcdata->redit_name);
		send_to_char(buf, ch);
	    }
	    else
		send_to_char("Main Description:\n\r", ch);

	    send_to_char(ch->pcdata->redit_desc, ch);
	    send_to_char("\n\r", ch);
	}

	return;
    }
    else if (!str_cmp(arg, "clear"))
    {
	if (ch->pcdata->redit_name)
	{
	    free_string(ch->pcdata->redit_name);
	    ch->pcdata->redit_name = str_dup("");
	}

	if (ch->pcdata->redit_desc)
	{
	    free_string(ch->pcdata->redit_desc);
	    ch->pcdata->redit_desc = str_dup("");
	}

	send_to_char("Cleared.\n\r", ch);
	return;
    }
    else if (!str_cmp(arg, "submit"))
    {
	NOTE_DATA *pnote;
	char *strtime;
	DESCRIPTOR_DATA *d;
	CHAR_DATA *dch;

	if (!ch->pcdata->redit_desc || ch->pcdata->redit_desc[0] == '\0')
	{
	    send_to_char("You have nothing in progress to submit.\n\r", ch);
	    return;
	}

	pnote = new_note();

	pnote->next	= NULL;
    	pnote->sender	= IS_NPC(ch) ? ch->short_descr : ch->name;
	strtime		= ctime( &current_time );
	strtime[strlen(strtime)-1]	= '\0';
	pnote->date	= str_dup( strtime );
	pnote->date_stamp = current_time;
	pnote->to_list	= str_dup("immortal");

	sprintf(buf, "%d%s%s", ch->in_room->vnum, ((!ch->pcdata->redit_name || ch->pcdata->redit_name[0] == '\0') ? "" : " "),
		((!ch->pcdata->redit_name || ch->pcdata->redit_name[0] == '\0') ? "" : ch->pcdata->redit_name));
	pnote->subject	= str_dup(buf);

    	pnote->text	= str_dup(ch->pcdata->redit_desc);
    	pnote->type	= NOTE_ROOM;

	append_note(pnote);

	for (d = descriptor_list; d; d = d->next)
	{
	    if ((d->connected == CON_PLAYING)
	     && d->character)
	    {
		dch = d->original ? d->original : d->character;	
		if (!IS_NPC(dch) 
		 && (IS_IMMORTAL(ch) || IS_SET(dch->nact, PLR_NOTEIFY))
		 && (dch != ch) && is_note_to(dch, pnote))
		    send_to_char("You have a new roomnote waiting.\n\r", dch);
	    }
	}

	if (ch->pcdata->redit_name)
	{
	    free_string(ch->pcdata->redit_name);
	    ch->pcdata->redit_name = str_dup("");
	}

	if (ch->pcdata->redit_desc)
	{
	    free_string(ch->pcdata->redit_desc);
	    ch->pcdata->redit_desc = str_dup("");
	}

	return;
    }
    else
    {
	send_to_char("Usage: room [desc/show/clear/submit] or\n\r", ch);
	send_to_char("       room ed [keywords]\n\r", ch);
	return;
    }
}

char *note_from(CHAR_DATA *ch, NOTE_DATA *note)
{
    char from[MAX_STRING_LENGTH];
    strcpy(from,note->sender);
    if (note->forger && note->forger[0]!='\0')
	if (IS_IMMORTAL(ch))
	    sprintf(from,"%s <%s>",note->sender,note->forger);
	else
	    if (!is_name(ch->name,note->forger))
	    	if (!note->forged)
		    strcpy(from,note->forger);

    return str_dup(from);
}

