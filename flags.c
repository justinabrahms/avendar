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
#include "tables.h"

int flag_lookup args( ( const char *name, const struct flag_type *flag_table) );

int DetermineDirection(const char * arg)
{
	if      (!str_prefix(arg, "north")) return 0;
	else if (!str_prefix(arg, "east" )) return 1;
	else if (!str_prefix(arg, "south")) return 2;
	else if (!str_prefix(arg, "west" )) return 3;
	else if (!str_prefix(arg, "up"   )) return 4;
	else if (!str_prefix(arg, "down" )) return 5;
	return -1;
}

void do_flag(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH],arg3[MAX_INPUT_LENGTH];
    char word[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    char type;
    int pos;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    type = argument[0];

    if (arg1[0] == '\0' || arg2[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  flag mob  <name> <field> [+,-,=] <flags>\n\r",ch);
	send_to_char("  flag char <name> <field> [+,-,=] <flags>\n\r",ch);
	send_to_char("  flag exit <direction>    [+,-,=] <flags>\n\r",ch);
	send_to_char("  flag acct <character>    [+,-,=] <flags>\n\r",ch);
	send_to_char("\n\r", ch);
	send_to_char("  +: add flag, -: remove flag, =: set equal to\n\r",ch);
	send_to_char("  otherwise flag toggles the flags listed.\n\r",ch);
	return;
    }

    if (!str_prefix(arg1, "acct"))
    {
	CHAR_DATA *victim;

	if ((victim = get_char_world(ch, arg2)) == NULL)
	{
	    send_to_char("You cannot locate that person in the world.\n\r", ch);
	    return;
	}

	if (IS_NPC(victim))
	{
	    send_to_char("Not on NPCs.\n\r", ch);
	    return;
	}

	if (!victim->desc || !victim->desc->acct)
	{
	    send_to_char("Character is not associated with an account.\n\r", ch);
	    return;
	}

	if ((type == '+') || (type == '-') || (type == '='))
	    argument = one_argument(argument, word);

	if (argument[0] == '\0')
	{
	    send_to_char("Error: No flags specified.\n\r", ch);
	    send_to_char("Syntax: flag acct <character> [+,-,=] <flags>\n\r", ch);
	    return;
	}

        for ( ; ; )
	{
	    argument = one_argument(argument, word);

	    if (word[0] == '\0')
		break;

	    pos = flag_lookup(word, acct_flags);
	    if (pos == 0)
	    {
		sprintf(buf, "Flag '%s' does not exist.\n\r", word);
		send_to_char(buf, ch);
	    }
	    else
	    {
		switch (type)
		{
		    case '=':
			victim->desc->acct->flags = pos;
			break;
		    case '+':
			SET_BIT(victim->desc->acct->flags, pos);
			sprintf(buf, "Flag '%s' set.\n\r", word);
			send_to_char(buf, ch);
			break;
		    case '-':
			REMOVE_BIT(victim->desc->acct->flags, pos);
			sprintf(buf, "Flag '%s' removed.\n\r", word);
			send_to_char(buf, ch);
			break;
		    default:
			if (IS_SET(victim->desc->acct->flags, pos))
			    sprintf(buf, "Flag '%s' removed.\n\r", word);
			else
			    sprintf(buf, "Flag '%s' set.\n\r", word);
			TOGGLE_BIT(victim->desc->acct->flags, pos);
			send_to_char(buf, ch);
			break;
		}
	    }
	}
    }
    else if (!str_prefix(arg1,"exit"))
    {
        int dir = -1;
 
        if (arg2[0] == '\0')
	{
	    send_to_char("Error: Direction missing.\n\r", ch);
	    send_to_char("Syntax: flag exit <direction> [+,-,=] <flags>\n\r", ch);
	    return;
        }   

	dir = DetermineDirection(arg2);
	if (dir == -1)
	{
	    send_to_char("Error: Invalid direction.\n\r", ch);
	    send_to_char("Syntax: flag exit <direction> [+,-,=] <flags>\n\r", ch);
	    return;
	}

	if (!ch->in_room || (ch->in_room->exit[dir] == NULL))
	{
	    send_to_char("Error: Exit does not exist.\n\r", ch);
	    return;
        }

	if ((type == '+') || (type == '-') || (type == '='))
	    argument = one_argument(argument, word);

	if (argument[0] == '\0')
	{
	    send_to_char("Error: No flags specified.\n\r", ch);
	    send_to_char("Syntax: flag exit <direction> [+,-,=] <flags>\n\r", ch);
	    return;
	}

        for ( ; ; )
	{
	    argument = one_argument(argument, word);

	    if (word[0] == '\0')
		break;

	    pos = flag_lookup(word, exit_flags);
	    if (pos == 0)
	    {
		sprintf(buf, "Flag '%s' does not exist.\n\r", word);
		send_to_char(buf, ch);
	    }
	    else
	    {
		switch (type)
		{
		    case '=':
			ch->in_room->exit[dir]->exit_info = pos;
			break;
		    case '+':
			SET_BIT(ch->in_room->exit[dir]->exit_info, pos);
			sprintf(buf, "Flag '%s' set.\n\r", word);
			send_to_char(buf, ch);
			break;
		    case '-':
			REMOVE_BIT(ch->in_room->exit[dir]->exit_info, pos);
			sprintf(buf, "Flag '%s' removed.\n\r", word);
			send_to_char(buf, ch);
			break;
		    default:
			if (IS_SET(ch->in_room->exit[dir]->exit_info, pos))
			{
			    REMOVE_BIT(ch->in_room->exit[dir]->exit_info, pos);
			    sprintf(buf, "Flag '%s' removed.\n\r", word);
			}
			else
			{
			    SET_BIT(ch->in_room->exit[dir]->exit_info, pos);
			    sprintf(buf, "Flag '%s' set.\n\r", word);
			}
			send_to_char(buf, ch);
			break;
		}
	    }
	}
    }
    else if (!str_prefix(arg1,"mob") || !str_prefix(arg1,"char"))
    {
	CHAR_DATA *victim;
        long *flag, old = 0, new_flag = 0, marked = 0;
        const struct flag_type *flag_table;

	argument = one_argument(argument, arg3);
	type = argument[0];

	if (arg2[0] == '\0')
	{
	    send_to_char("Syntax: flag <mob/char> <name> <field> [+,-,=] <flags>\n\r", ch);
	    return;
        }

	victim = get_char_world(ch,arg2);
	if (victim == NULL)
	{
	    send_to_char("You can't find them.\n\r",ch);
	    return;
	}

        if (arg3[0] == '\0')
	{
	    send_to_char("Syntax: flag <mob/char> <name> <field> [+,-,=] <flags>\n\r", ch);
	    return;
        }

        /* select a flag to set */
	if (!str_prefix(arg3,"act"))
	{
	    if (!IS_NPC(victim))
	    {
		send_to_char("Use plr for PCs.\n\r",ch);
		return;
	    }

	    flag = &victim->act;
	    flag_table = act_flags;
	}

	else if (!str_prefix(arg3,"plr"))
	{
	    if (IS_NPC(victim))
	    {
		send_to_char("Use act for NPCs.\n\r",ch);
		return;
	    }

	    flag = &victim->act;
	    flag_table = plr_flags;
	}

 	else if (!str_prefix(arg3,"aff"))
	{
	    flag = &victim->affected_by;
	    flag_table = affect_flags;
	}

  	else if (!str_prefix(arg3,"immunity"))
	{
	    flag = &victim->imm_flags;
	    flag_table = imm_flags;
	}

	else if (!str_prefix(arg3,"resist"))
	{
	    flag = &victim->res_flags;
	    flag_table = imm_flags;
	}

	else if (!str_prefix(arg3,"vuln"))
	{
	    flag = &victim->vuln_flags;
	    flag_table = imm_flags;
	}
	
	else if (!str_prefix(arg3, "off"))
	{
	    flag = &victim->off_flags;
	    flag_table = off_flags;
	}

	else if (!str_prefix(arg3, "lang"))
	{
	    if (!IS_NPC(victim))
	    {
		send_to_char("Languages cannot be set on PCs\n\r", ch);
		return;
	    }

	    flag = &victim->lang_flags;
	    flag_table = lang_flags;
	}

	else if (!str_prefix(arg3,"form"))
	{
	    if (!IS_NPC(victim))
	    {
	 	send_to_char("Form can't be set on PCs.\n\r",ch);
		return;
	    }

	    flag = &victim->form;
	    flag_table = form_flags;
	}

	else if (!str_prefix(arg3,"parts"))
	{
	    if (!IS_NPC(victim))
	    {
		send_to_char("Parts can't be set on PCs.\n\r",ch);
		return;
	    }

	    flag = &victim->parts;
	    flag_table = part_flags;
	}

	else if (!str_prefix(arg3,"comm"))
	{
	    if (IS_NPC(victim))
	    {
		send_to_char("Comm can't be set on NPCs.\n\r",ch);
		return;
	    }

	    flag = &victim->comm;
	    flag_table = comm_flags;
	}

	else 
	{
	    send_to_char("Error: Invalid field specification.\n\r", ch);
	    if (IS_NPC(victim))
	        send_to_char("Valid fields are: act, aff, off, imm, res, vuln, form, part, lang.\n\r",ch);
	    else
	        send_to_char("Valid fields are: plr, comm, aff, imm, res, vuln.\n\r",ch);
	    return;
	}

	if (type == '+' || type == '-' || type == '=')
	    argument = one_argument(argument, word);

	if (argument[0] == '\0')
	{
	    send_to_char("Syntax: flag <mob/char> <name> <field> [+,-,=] <flags>\n\r", ch);
	    return;
        }

	old = *flag;
	victim->zone = NULL;

	if (type != '=')
	    new_flag = old;

        /* mark the words */
        for (; ;)
        {
	    argument = one_argument(argument,word);

	    if (word[0] == '\0')
		break;

	    pos = flag_lookup(word,flag_table);
	    if (pos == 0)
	    {
		send_to_char("That flag doesn't exist!\n\r",ch);
		return;
	    }
	    else
		SET_BIT(marked,pos);
	}

	for (pos = 0; flag_table[pos].name != NULL; pos++)
	{
	    if (!flag_table[pos].settable)
	    {
		if (IS_SET(old,flag_table[pos].bit))
		    SET_BIT(new_flag,flag_table[pos].bit);

		continue;
	    }

	    if (IS_SET(marked,flag_table[pos].bit))
	    {
		switch(type)
		{
		    case '=':
		    case '+':
			SET_BIT(new_flag,flag_table[pos].bit);
			break;
		    case '-':
			REMOVE_BIT(new_flag,flag_table[pos].bit);
			break;
		    default:
			if (IS_SET(new_flag, flag_table[pos].bit))
			    REMOVE_BIT(new_flag, flag_table[pos].bit);
			else
			    SET_BIT(new_flag, flag_table[pos].bit);
		}
	    }
	}
	*flag = new_flag;
	return;
    }
}



    
