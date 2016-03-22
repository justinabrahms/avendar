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
 **************************************************************************/

/***************************************************************************
*       ROM 2.4 is copyright 1993-1996 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@efn.org)                                  *
*           Gabrielle Taylor                                               *
*           Brian Moore (zump@rom.org)                                     *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <set>
#include <vector>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "interp.h"
#include "languages.h"
#include "spells_spirit.h"
#include "spells_spirit_air.h"
#include "spells_void.h"
#include "Faction.h"

/* command procedures needed */
DECLARE_DO_FUN(do_quit  );

extern	CHAR_DATA *	get_random_pc_world	args( ( CHAR_DATA *ch ) );

bool validcolour(CHAR_DATA *ch, char *argument);
void spoke_ashur(CHAR_DATA *ch);

void	comm_send	args( ( CHAR_DATA *ch, void *vo, char *argument,
				int channel ) );
void	demon_possession args(( CHAR_DATA *ch, void *vo, int channel ) );

void	char_from_acct	args(( CHAR_DATA *ch, char *reason ));

void	write_newbielog	args(( char *str ));

/* save.c */
extern	void	fwrite_account	args(( ACCOUNT_DATA *account ));

extern bool rip_process;

extern void append_note	args( (NOTE_DATA *pnote) );

bool demon_pos = FALSE;

void demon_possession(CHAR_DATA *ch, void *vo, int channel)
{
    int dphrase = number_range(0, 59);

    if (channel == CHAN_ESAY)
	channel = CHAN_SAY;

    demon_pos = TRUE;
    switch(dphrase)
    {
	case 0:
	    comm_send(ch, vo, "Shigrak logor esli Tikkuru!", channel);
	    do_emote(ch, "laughs nightmarishly, at some private joke.");
	    break;
	case 1:
	    comm_send(ch, vo, "I prefer a feast of flesh, and the taste of blood.", channel);
	    break;
	case 2:
	    do_emote(ch, "sniffs at the air.");
	    do_say(ch, "Do I smell fear?");
	    break;
	case 3:
	    comm_send(ch, vo, "A fall can be marvelously invigorating.", channel);
	    break;
	case 4:
	    comm_send(ch, vo, "A moment I wish I could frame, and hang upon my mantle.", channel);
	    break;
	case 5:
	    comm_send(ch, vo, "I have so much pain to give, and so little time.", channel);
	    break;
	case 6:
	    comm_send(ch, vo, "Mortal flesh tears so easily.", channel);
	    break;
	case 7:
	    do_emote(ch, "gurgles sickeningly.");
	    break;
	case 8:
	    comm_send(ch, vo, "Pain is the rule, pleasure the exception.", channel);
	    break;
	case 9:
	    comm_send(ch, vo, "If you do not fear Me, then you are against Me.", channel);
	    break;
	case 10:
	    comm_send(ch, vo, "A scream provides a sweeter song than any bardic melody.", channel);
	    break;
	case 11:
	    comm_send(ch, vo, "Blasphemy, sodomy, or heresy -- I'm not sure which is my favorite sin.", channel);
	    break;
	case 12:
	    do_emote(ch, "makes a slurping, glibbering sound.");
	    break;
	case 13:
	    comm_send(ch, vo, "The more mortals I see, the more I'm convinced even the gods can make mistakes.", channel);
	    break;
	case 14:
	    comm_send(ch, vo, "What can be sweeter than the embrace of the night?", channel);
	    break;
	case 15:
	    comm_send(ch, vo, "A thousand lifetimes, and I have yet to meet my equal.", channel);
	    break;
	case 16:
	    comm_send(ch, vo, "An aberration in that perfect, divine plan.", channel);
	    break;
	case 17:
	    comm_send(ch, vo, "And what a glorious reversal of fortunes!", channel);
	    break;
	case 18:
	    comm_send(ch, vo, "Anue aeksa giredash'shu gireshya.", channel);
	    break;
	case 19:
	    comm_send(ch, vo, "Aranurug kunashketl morums suneshumas.", channel);
	    break;
	case 20:
	    comm_send(ch, vo, "Igray'iitu, Lord of the Wastes of Oame, I name thee!", channel);
	    break;
	case 21:
	    comm_send(ch, vo, "Bend mortals, or be broken!", channel);
	    break;
	case 22:
	    comm_send(ch, vo, "Bahhaoth, Watcher O'er the Dead Sea, I name thee!", channel);
	    break;
	case 23:
	    comm_send(ch, vo, "By Zyal's sword, you will perish!", channel);
	    break;
	case 24:
	    comm_send(ch, vo, "Daneshm invri ketl nra muru alledlakmel.", channel);
	    break;
	case 25:
	    comm_send(ch, vo, "Even in this pitiful form, I am a god!", channel);
	    break;
	case 26:
	    comm_send(ch, vo, "Greet the coming night!", channel);
	    break;
	case 27:
	    comm_send(ch, vo, "I will bathe my blade in your blood!", channel);
	    break;
	case 28:
	    comm_send(ch, vo, "I will grind your skull into dust!", channel);
	    break;
	case 29:
	    comm_send(ch, vo, "Isetaton Chosanth Khebt-Ut Ameras!", channel);
	    break;
	case 30:
	    comm_send(ch, vo, "It is a fearsome thing to know that the multiverse can be painted with the blackness of your own being.", channel);
	    break;
	case 31:
	    comm_send(ch, vo, "It is amazing how fear compels.", channel);
	    break;
	case 32:
	    comm_send(ch, vo, "Khalen Iag Garusth!", channel);
	    break;
	case 33:
	    comm_send(ch, vo, "Ksashask ikakral jakulr!", channel);
	    break;
	case 34:
	    comm_send(ch, vo, "Let me sip your blood.", channel);
	    break;
	case 35:
	    comm_send(ch, vo, "Let's be to it then.", channel);
	    break;
	case 36:
	    comm_send(ch, vo, "Mintiya naya jalsuma dija'llsuma sunumas.", channel);
	    break;
	case 37:
	    comm_send(ch, vo, "Naluzdandur, Shadow of the Deep, I name thee!", channel);
	    break;
	case 38:
	    comm_send(ch, vo, "No god can bear to contemplate the things that we will do.", channel);
	    break;
	case 39:
	    comm_send(ch, vo, "Nyogthua, Sorcerer of the Arcanum, I name thee!", channel);
	    break;
	case 40:
	    comm_send(ch, vo, "Pricina, Devourer of Hearts, I name thee!", channel);
	    break;
	case 41:
	    comm_send(ch, vo, "Qllemneng ch'umas darideqketl.", channel);
	    break;
	case 42:
	    comm_send(ch, vo, "Scream for me!", channel);
	    break;
	case 43:
	    comm_send(ch, vo, "Selkra Iag Nihvglak!", channel);
	    break;
	case 44:
	    comm_send(ch, vo, "The chains rattle eagerly, for soon your soul will be bound!", channel);
	    break;
	case 45:
	    comm_send(ch, vo, "The grave is calling... the Dragon is calling.", channel);
	    break;
	case 46:
	    comm_send(ch, vo, "The likelihood of defeating me is small.", channel);
	    break;
	case 47:
	    comm_send(ch, vo, "The sanity of mortals is merely a fear of the truth.", channel);
	    break;
	case 48:
	    comm_send(ch, vo, "There is a song in the City of Iron; the sound of razors through flesh.", channel);
	    break;
	case 49:
	    comm_send(ch, vo, "This is but the beginning.", channel);
	    break;
	case 50:
	    comm_send(ch, vo, "To devour is to live.", channel);
	    break;
	case 51:
	    comm_send(ch, vo, "To die is to hunger.", channel);
	    break;
	case 52:
	    comm_send(ch, vo, "Tzakmeqiel the Desecrator, I name thee!", channel);
	    break;
	case 53:
	    comm_send(ch, vo, "Will your enemies even bother to bury you?", channel);
	    break;
	case 54:
	    comm_send(ch, vo, "Xthjich, the Eternal Demon, I name thee!", channel);
	    break;
	case 55:
	    comm_send(ch, vo, "Your pain, I will savor most of all.", channel);
	    break;
	case 56:
	    comm_send(ch, vo, "Unless you suffer, how can you be sure you exist?", channel);
	    do_emote(ch, "grins wickedly.");
	    break;
	case 57:
	    comm_send(ch, vo, "Yes. A tentacle.", channel);
	    do_emote(ch, "chortles mischeviously.");
	    break;
	case 58:
	    comm_send(ch, vo, "Pray!", channel);
	    break;
	case 59:
	    comm_send(ch, vo, "Rejoice, mortal! You will soon be a martyr.", channel);
	    break;
    }
    demon_pos = FALSE;

    return;
}




void comm_send(CHAR_DATA *ch, void *vo, char *argument, int channel)
{
    char buf[MAX_STRING_LENGTH];
    char mbuf[MAX_INPUT_LENGTH * 2 + 36];
    char cynbuf[MAX_INPUT_LENGTH * 2 + 53];
    char arg[MAX_INPUT_LENGTH]; /* for esay */
    CHAR_DATA *vch, *origin = ch;
    int cnum = -1, x;
    DESCRIPTOR_DATA *d;
    AFFECT_DATA *paf;
    bool use_esp;
    LANG_STRING *trans_new;
    bool imp_check = FALSE, mimic_failed = FALSE;
  //  int i;
   
    if (!ch->in_room)
        return;

    for (x = 0; channel_table[x].channel != CHAN_NONE; x++)
	if (channel_table[x].channel == channel) 
	{
	    cnum = x;
	    break;
	}

    if (cnum == -1)
    {
        bug("comm_send: Invalid channel.", 0);
        return;
    }

    origin = global_mimic ? global_mimic : ch;
    use_esp = is_affected(origin, gsn_esp);

    if (IS_SET(origin->comm, COMM_NOCHANNELS))
    {
        send_to_char("The gods have revoked your channel privleges.\n\r", origin);
        return;
    }

    if (channel == CHAN_ESAY)
    {
        argument = one_argument(argument, arg, true);
        if (argument[0] == '\0')
            return;
    }

    if (IS_SET(origin->comm, COMM_DEAF))
    {
        send_to_char("Your message didn't get through.\n\r", origin);
        return;
    }

    if (channel != CHAN_THINK && channel != CHAN_OOC)
    {
        // Check for avatar, astral projection, and larva
        int avatarType(type_of_avatar(origin));
        if ((avatarType != -1 && ((channel != CHAN_YELL && channel != CHAN_CYELL) || avatarType != gsn_avatarofthelodestar))
        || is_affected(origin, gsn_astralprojection) || IS_NAFFECTED(origin, AFF_LARVA))
  		{
			send_to_char("You try to communicate, but can't seem to.\n", origin);
			return;
		}
        
        // Check for drowning
        if (is_affected(origin, gsn_drown) && (number_percent() <= 25 || !is_affected(origin, gsn_waterbreathing)))
        {
            send_to_char("You only manage odd gurgling sounds.\n", origin);
            act("$n gurgles awkwardly, flecks of water flying from $s mouth.", origin, NULL, NULL, TO_ROOM);
            return;
        }
    }


    if (is_affected(origin, gsn_lycanthropy) && channel != CHAN_THINK)
    {
	    act("You manage only vicious snarling instead of speech.", ch, NULL, NULL, TO_CHAR);
	    act("$n snarls ferociously!", ch, NULL, NULL, TO_ROOM);
	    return;
    }

    if (is_affected(origin, gsn_aviancurse) && channel != CHAN_THINK)
    {
	    act("You manage only a clucking sound instead of speech.", ch, NULL, NULL, TO_CHAR);
	    act("$n clucks softly.", ch, NULL, NULL, TO_ROOM);
	    return;
    }

    if (IS_NAFFECTED(origin, AFF_GARROTE_VICT) && !use_esp)
    {
          send_to_char("Your neck burns, and you struggle to speak, but can't.\n\r", origin);
          return;
    }

    if (is_affected(origin, gsn_gag) && !use_esp && channel != CHAN_THINK)
    {
          send_to_char("Your mouth is gagged, you cannot speak!\n\r", origin);
          return;
    }
    if (get_modifier(origin->affected, gsn_wrathofthevoid) == 1 && !use_esp 
      && !(channel == CHAN_THINK) )
    {
	send_to_char("The green ichor still binds your mouth.\n\r",origin);
	return;
    }

    if (IS_OAFFECTED(origin, AFF_DEMONPOS) && !demon_pos)
    {
	demon_possession(origin, vo, channel);
	return;
    }

    if (!validcolour (origin, argument))
    {
        send_to_char("You aren't allowed to use colour.\n\r", origin);
        return;
    }
	
    if (is_affected(origin, gsn_moonray))
        argument = confuse_text(argument);

    trans_new = translate_spoken_new(origin, argument);
    trans_new->orig_string = argument;
    if (!IS_NPC(origin) && IS_SET(origin->comm, COMM_TRANSLATE))
    {
        sprintf(buf, "Translation: '%s'\n\r", trans_new->full_trans);
	send_to_char(buf, origin);
    }

	size_t argLength(strlen(argument));
    if (!IS_NPC(origin) && (get_skill(origin, *lang_data[ch->speaking].sn) >= 90)
     && (channel != CHAN_OOC) && !global_mimic)
    {
        /* generate checksum to check against language spamming */
            unsigned int cur_checksum = 0, i;

        for (i = 0; i < argLength; i++)
            cur_checksum = (cur_checksum + argument[i]) % UINT_MAX;

        if (cur_checksum != origin->pcdata->lang_checksum)
        {
            imp_check = TRUE;
            origin->pcdata->lang_checksum = cur_checksum;
        }
    }

    /* This.. should suffice */
    if (!IS_NPC(ch) && !IS_IMM_TRUST(ch))
    {
	if (channel == CHAN_ESAY)
	{
	    sprintf(buf, "comm: %s> {Y%s %s, '%s'{x", ch->name, ch->name, arg, argument);
	    wiznet(buf,ch,NULL,WIZ_COMM,0,0);
	}
	else if (channel == CHAN_HOUSE)
	{
	    sprintf(buf, "comm: %s> [{W%s{x] %s: %s", ch->name, clan_table[ch->clan].who_name, ch->name,argument);
	    wiznet(buf,ch,NULL,WIZ_COMM,0,0);
	}
	else
	{
	    sprintf(buf, "comm: %s> %s", ch->name, channel_table[cnum].to_self);
	    if (channel_table[cnum].ctarg == CTO_VICT)
	    	wiznet(buf, (CHAR_DATA *) vo, (void *) argument, WIZ_COMM, 0, 0);
	    else
	    	wiznet(buf, ch, (void *) argument, WIZ_COMM, 0, 0);
	}
    }

    // Check for distance hearing effects
    if (channel != CHAN_THINK && channel != CHAN_OOC && !IS_IMMORTAL(origin) && origin->in_room != NULL)
    {
        // Check for gralcian funnel
        for (AFFECT_DATA * funnel(get_area_affect(origin->in_room->area, gsn_gralcianfunnel)); funnel != NULL; funnel = get_area_affect(origin->in_room->area, gsn_gralcianfunnel, funnel))
        {
            CHAR_DATA * recipient(get_char_by_id(funnel->modifier));
            if (recipient == NULL || recipient == origin)
                continue;

            // Found a gralcian recipient; translate the text if necessary
            const char * text(channel_table[cnum].use_lang ? translate_out_new(recipient, trans_new) : trans_new->orig_string);
            send_to_char("{WThe winds whisper to you, '{Y", recipient);
            send_to_char(text, recipient);
            send_to_char("{W'{x\n", recipient);
        }

        // Check for corpsesense
        if (should_trigger_corpsesense_hearing(*origin->in_room))
        {
            // Inform all the appropriate PCs
            for (DESCRIPTOR_DATA * desc(descriptor_list); desc != NULL; desc = desc->next)
            {
                if (desc->connected != CON_PLAYING || desc->character->in_room == origin->in_room || !is_affected(desc->character, gsn_corpsesense))
                    continue;
                    
                if (number_percent() > get_skill(desc->character, gsn_corpsesense))
                {
                    check_improve(desc->character, NULL, gsn_corpsesense, false, 2);
                    continue;
                }

                check_improve(desc->character, NULL, gsn_corpsesense, true, 2);
                const char * text(channel_table[cnum].use_lang ? translate_out_new(desc->character, trans_new) : trans_new->orig_string);
                send_to_char("{WA voice whispers to you from beyond the grave, '{Y", desc->character);
                send_to_char(text, desc->character);
                send_to_char("{W'{x\n", desc->character);
            }
        }
    }

    if (global_mimic && !IS_IMMORTAL(origin))   // immortals can't fail
    {
        int mchance = get_skill(origin, gsn_mimic);

        if (origin->sex != ch->sex)
            mchance /= 2;

        if (origin->race != ch->race)
            mchance = mchance * 4 / 5;

        mchance = URANGE(5, mchance, 95);

        if (number_percent() > mchance)
        {
            mimic_failed = TRUE;
            check_improve(ch, NULL, gsn_mimic, FALSE, 4);
        }
        else
            check_improve(ch, NULL, gsn_mimic, TRUE, 4);

            sprintf(mbuf, "%s attempted to mimic %s, but failed.\n\r", IS_NPC(origin) ? origin->short_descr : origin->name, IS_NPC(ch) ? ch->short_descr : ch->name);
    }

    if (channel_table[cnum].ctarg == CTO_ROOM)
    {
	if (!vo)
	    if ((vo = (void *) origin->in_room) == NULL)
	    {
		free(trans_new);
		return;
	    }

        for (vch = ((ROOM_INDEX_DATA *) vo)->people; vch; vch = vch->next_in_room)
        {
	    if ((use_esp || !IS_OAFFECTED(vch, AFF_DEAFEN))
	     && (vch->position > POS_SLEEPING))
	    {
		if (vch != origin)
		{
		    if (mimic_failed)
			send_to_char(mbuf, vch);
		    else if (global_mimic && !IS_NPC(vch) && BIT_GET(vch->pcdata->traits, TRAIT_CYNIC) && number_bits(2) != 0)
		    {
			sprintf(cynbuf, "%s attempted to mimic %s, but you see right through it.\n\r",
				IS_NPC(origin) ? origin->short_descr : origin->name,
				IS_NPC(ch) ? ch->short_descr : ch->name);

			send_to_char(cynbuf, vch);
		    }
		}

		if (IS_IMMORTAL(vch) && global_mimic)
		    act("<Mimicry from $N>", vch, NULL, origin, TO_CHAR);

        if ((ch == vch) && (ch == origin))
		{
		    if ((ch->speaking != LANG_COMMON) && channel_table[cnum].use_lang)
            {
                if (channel == CHAN_ESAY)
                    sprintf(buf, channel_table[cnum].to_self_lang, arg, lang_data[ch->speaking].name);
                else
                    sprintf(buf, channel_table[cnum].to_self_lang, lang_data[ch->speaking].name);
                c_act(buf, vch, argument, ch, TO_CHAR);
            }
		    else
            {
                if (channel == CHAN_ESAY)
                {
                    sprintf(buf, channel_table[cnum].to_self, arg);
                    c_act(buf, vch, argument, ch, TO_CHAR);
                }
                else
                    c_act(channel_table[cnum].to_self, vch, argument, ch, TO_CHAR);
            }
		}
        else
		{
            if ((origin->speaking != LANG_COMMON) && channel_table[cnum].use_lang
            //&& ((!IS_NPC(vch) && (vch->pcdata->learned[*lang_data[origin->speaking].sn] >= 25)) 
            && (get_skill(vch, *lang_data[origin->speaking].sn) >= 25 
            || (IS_NPC(vch) && IS_SET(vch->lang_flags, (1 << origin->speaking)))))
            {
                if (channel == CHAN_ESAY)
                    sprintf(buf, channel_table[cnum].to_rest_lang, arg, lang_data[origin->speaking].name);
                else
                    sprintf(buf, channel_table[cnum].to_rest_lang, lang_data[origin->speaking].name);
                c_act(buf, vch, translate_out_new(vch, trans_new), ch, TO_CHAR);
            }
		    else
            {
                if (channel == CHAN_ESAY)
                {
                    sprintf(buf, channel_table[cnum].to_rest, arg);
                    c_act(buf, vch, translate_out_new(vch, trans_new), ch, TO_CHAR);
                }
                else
                    c_act(channel_table[cnum].to_rest, vch, translate_out_new(vch, trans_new), ch, TO_CHAR);
            }
		    if (imp_check)
                check_improve(vch, NULL, *lang_data[origin->speaking].sn, TRUE, 2);
		}

	        if (IS_NAFFECTED(vch, AFF_LIGHTSLEEP))
                send_to_char("Someone said something.\n\r", vch);
	    }
        }
    }
    else
    {
	if (IS_SET(origin->comm, COMM_QUIET))
	{
	    send_to_char("You must turn off quiet mode first.\n\r", origin);
	    return;
	}
    }

    if (channel_table[cnum].ctarg == CTO_HOUSE)
    {
	int house = *((int *) vo);

	if (!vo)
	{
	    if (!IS_NPC(ch))
		vo = &ch->clan;
	    else
	    {
		free(trans_new);
		return;
	    }
	}

	for (d = descriptor_list; d; d = d->next)
	{
	    if ((d->connected == CON_PLAYING)
	     && (use_esp || !IS_OAFFECTED(d->character, AFF_DEAFEN))
	     && (d->character->clan == house)
	     && !IS_SET(d->character->comm, COMM_QUIET)
	     && (use_esp || !get_room_song(origin->in_room, gsn_soundbubble) || (origin->in_room == d->character->in_room)))
	    {
		if (d->character != origin)
		{
		    if (mimic_failed)
			send_to_char(mbuf, d->character);
		    else if (global_mimic && !IS_NPC(d->character) && BIT_GET(d->character->pcdata->traits, TRAIT_CYNIC) && number_bits(2) != 0)
		    {
			sprintf(cynbuf, "%s attempted to mimic %s, but you see right through it.\n\r",
				IS_NPC(origin) ? origin->short_descr : origin->name,
				IS_NPC(ch) ? ch->short_descr : ch->name);

			send_to_char(cynbuf, d->character);
		    }
		}

		if (IS_IMMORTAL(d->character) && global_mimic)
		    act("<Mimicry from $N>", d->character, NULL, origin, TO_CHAR);

		if ((d->character == ch) && (ch == origin))
		{
		    if (channel == CHAN_HOUSE)
			sprintf(buf, channel_table[cnum].to_self, clan_table[house].who_name);
		    else
			sprintf(buf, channel_table[cnum].to_self); 
		    c_act(buf, d->character, argument, ch, TO_CHAR);

		}
		else
		{
		    if (channel == CHAN_HOUSE)
			sprintf(buf, channel_table[cnum].to_rest, clan_table[house].who_name);
		    else
			sprintf(buf, channel_table[cnum].to_rest);
		    c_act(buf, d->character, translate_out_new(d->character, trans_new), ch, TO_CHAR);
		    if (imp_check)
			check_improve(d->character, NULL, *lang_data[origin->speaking].sn, TRUE, 2);
		}

	    }
	}
    }

    if (channel_table[cnum].ctarg == CTO_CLASS)
    {
	int class_num = *((int *) vo);

	if (!vo)
	{
	    if (!IS_NPC(ch))
		vo = &ch->class_num;
	    else
	    {
		free(trans_new);
		return;
	    }
	}

	for (d = descriptor_list; d; d = d->next)
	{
	    if ((d->connected == CON_PLAYING)
	     && !IS_NPC(d->character)
	     && (use_esp || !IS_OAFFECTED(d->character, AFF_DEAFEN))
	     && (d->character->class_num == class_num)
	     && !IS_SET(d->character->comm, COMM_QUIET)
	     && ((channel != CHAN_DRUID) || is_affected(d->character, gsn_commune_nature))
	     && (use_esp || !get_room_song(origin->in_room, gsn_soundbubble) || (origin->in_room == d->character->in_room)))
	    {
		if (d->character != origin)
		{
		    if (mimic_failed)
			send_to_char(mbuf, d->character);
		    else if (global_mimic && !IS_NPC(d->character) && BIT_GET(d->character->pcdata->traits, TRAIT_CYNIC) && number_bits(2) != 0)
		    {
			sprintf(cynbuf, "%s attempted to mimic %s, but you see right through it.\n\r",
				IS_NPC(origin) ? origin->short_descr : origin->name,
				IS_NPC(ch) ? ch->short_descr : ch->name);

			send_to_char(cynbuf, d->character);
		    }
		}

		if (IS_IMMORTAL(d->character) && global_mimic)
		    act("<Mimicry from $N>", d->character, NULL, origin, TO_CHAR);

		if ((d->character == ch) && (ch == origin))
		{
		    if ((ch->speaking != LANG_COMMON) && channel_table[cnum].use_lang)
		    {
			sprintf(buf, channel_table[cnum].to_self_lang, lang_data[ch->speaking].name);
			c_act(buf, d->character, argument, ch, TO_CHAR);
		    }
		    else
		        c_act(channel_table[cnum].to_self,d->character,argument,ch,TO_CHAR);
		}
		else
		{
		    if ((origin->speaking != LANG_COMMON) && channel_table[cnum].use_lang
                     && (!IS_NPC(d->character) && (d->character->pcdata->learned[*lang_data[origin->speaking].sn] >= 25)))
                    {
			sprintf(buf, channel_table[cnum].to_rest_lang, lang_data[origin->speaking].name);
			c_act(buf, d->character, translate_out_new(d->character, trans_new), ch, TO_CHAR);
		    }
		    else
		        c_act(channel_table[cnum].to_rest,d->character,translate_out_new(d->character, trans_new),ch,TO_CHAR);
		    if (imp_check)
			check_improve(d->character, NULL, *lang_data[origin->speaking].sn, TRUE, 2);
		}
	    }
	}
    }

    if (channel_table[cnum].ctarg == CTO_AREA)
    {
	if (!vo)
	{
	    if (ch->in_room && ch->in_room->area)
		vo = (void *) ch->in_room->area;
	    else
	    {
		free(trans_new);
		return;
	    }
	}

	for (d = descriptor_list; d; d = d->next)
	{
	    if ((d->connected == CON_PLAYING)
	     && (use_esp || (!IS_OAFFECTED(d->character, AFF_DEAFEN) && (!get_room_song(origin->in_room, gsn_soundbubble) || (origin->in_room == d->character->in_room))))
	     && d->character->in_room
	     && ((!IS_SET(ch->in_room->room_flags, ROOM_NOYELL) && (d->character->in_room->area == (AREA_DATA *) vo))
	      || (d->character->in_room == ch->in_room))
	     && !IS_SET(d->character->comm, COMM_QUIET))
	    {
		if (d->character != origin)
		{
		    if (mimic_failed)
			send_to_char(mbuf, d->character);
		    else if (global_mimic && !IS_NPC(d->character) && BIT_GET(d->character->pcdata->traits, TRAIT_CYNIC) && number_bits(2) != 0)
		    {
			sprintf(cynbuf, "%s attempted to mimic %s, but you see right through it.\n\r",
				IS_NPC(origin) ? origin->short_descr : origin->name,
				IS_NPC(ch) ? ch->short_descr : ch->name);

			send_to_char(cynbuf, d->character);
		    }
		}

		if (IS_IMMORTAL(d->character) && global_mimic)
		    act("<Mimicry from $N>", d->character, NULL, origin, TO_CHAR);

		if ((d->character == ch) && (ch == origin))
		{
		    if ((ch->speaking != LANG_COMMON) && channel_table[cnum].use_lang)
                    {
			sprintf(buf, channel_table[cnum].to_self_lang, lang_data[ch->speaking].name);
			c_act(buf, d->character, argument, ch, TO_CHAR);
		    }
		    else
		        c_act(channel_table[cnum].to_self,d->character,argument,ch,TO_CHAR);
		}
		else
		{
		    if ((channel == CHAN_YELL)
		     && is_affected(d->character, gsn_perception))
		    {
			if (global_mimic)
			    act("You sense $N is using the art of mimicry.", d->character, NULL, origin, TO_CHAR);

		        if ((origin->speaking != LANG_COMMON) && channel_table[cnum].use_lang
                         && (!IS_NPC(d->character) && (d->character->pcdata->learned[*lang_data[origin->speaking].sn] >= 25)))
			{
			    sprintf(buf, channel_table[cnum].to_vict_lang, lang_data[origin->speaking].name);
			    c_act(buf, d->character, argument, ch, TO_CHAR);
			}
			else
                    	    c_act(channel_table[cnum].to_vict,d->character,argument,ch,TO_CHAR);
		    }
		    else
		    {
		        if ((origin->speaking != LANG_COMMON) && channel_table[cnum].use_lang
                         && (!IS_NPC(d->character) && (d->character->pcdata->learned[*lang_data[origin->speaking].sn] >= 25)))
                        {
			    sprintf(buf, channel_table[cnum].to_rest_lang, lang_data[origin->speaking].name);
			    c_act(buf, d->character, translate_out_new(d->character, trans_new), ch, TO_CHAR);
			}
			else
			    c_act(channel_table[cnum].to_rest,d->character,translate_out_new(d->character, trans_new),ch,TO_CHAR);
		        if (imp_check)
			    check_improve(d->character, NULL, *lang_data[origin->speaking].sn, TRUE, 2);
		    }

		}
	    }
	}
    }

    if (channel_table[cnum].ctarg == CTO_VICT)
    {
        if (!vo)
        {
            free(trans_new);
            return;
        }
        else
            vch = (CHAR_DATA *) vo;

        if (get_room_song(origin->in_room, gsn_soundbubble) && !use_esp && (origin->in_room != vch->in_room))
        {
            send_to_char("You cannot communicate outside of the sound bubble.\n\r", ch) ;
            return;
        }

        if (use_esp || !IS_OAFFECTED(origin, AFF_DEAFEN))
        {
            if ((origin->speaking != LANG_COMMON) && channel_table[cnum].use_lang)
            {
            sprintf(buf, channel_table[cnum].to_self_lang, lang_data[origin->speaking].name);
            c_act(buf, origin, argument, vch, TO_CHAR);
            }
            else
                c_act(channel_table[cnum].to_self, origin, argument, vch, TO_CHAR);
        }
        
        if (use_esp || !IS_OAFFECTED(vch, AFF_DEAFEN))
        {
            if (vch != origin)
            {
            if (mimic_failed)
                send_to_char(mbuf, vch);
            else if (global_mimic && !IS_NPC(vch) && BIT_GET(vch->pcdata->traits, TRAIT_CYNIC) && number_bits(2) != 0)
            {
                sprintf(cynbuf, "%s attempted to mimic %s, but you see right through it.\n\r",
                IS_NPC(origin) ? origin->short_descr : origin->name,
                IS_NPC(ch) ? ch->short_descr : ch->name);
                send_to_char(cynbuf, vch);
            }
            }

            if (IS_IMMORTAL(vch) && global_mimic)
            act("<Mimicry from $N>", vch, NULL, origin, TO_CHAR);

            if ((origin->speaking != LANG_COMMON) && channel_table[cnum].use_lang
                 && ((!IS_NPC(vch) && (vch->pcdata->learned[*lang_data[origin->speaking].sn] >= 25)) 
                 || (IS_NPC(vch) && IS_SET(ch->lang_flags, (1 << origin->speaking)))))
            {
            sprintf(buf, channel_table[cnum].to_vict_lang, lang_data[ch->speaking].name);
            c_act(buf, vch, translate_out_new(vch, trans_new), ch, TO_CHAR);
            }
            else
                c_act(channel_table[cnum].to_vict, vch, translate_out_new(vch, trans_new), ch, TO_CHAR);
            if (imp_check)
            check_improve(vch, NULL, *lang_data[origin->speaking].sn, TRUE, 2);
        }

        switch (channel)
        {
            case CHAN_TELL: vch->reply = ch; break;
            case CHAN_OOC: vch->oocreply = ch; break;
        }
    }

    if (channel == CHAN_GTELL)
    {
	int num_group = 0;

	for (vch = char_list; vch; vch = vch->next)
	{
	    if ((ch != vch) && is_same_group(ch, vch))
	    {
		num_group++;
		if (use_esp
                 || (!IS_OAFFECTED(vch, AFF_DEAFEN)
                  && (!get_room_song(ch->in_room, gsn_soundbubble) || (ch->in_room == vch->in_room))))
		{
		    if ((ch->speaking != LANG_COMMON) && channel_table[cnum].use_lang
                     && ((!IS_NPC(vch) && (vch->pcdata->learned[*lang_data[ch->speaking].sn] >= 25)) 
                     || (IS_NPC(vch) && IS_SET(ch->lang_flags, (1 << ch->speaking)))))
		    {
			sprintf(buf, channel_table[cnum].to_rest_lang, lang_data[ch->speaking].name);
			c_act(buf, vch, translate_out_new(vch, trans_new), ch, TO_CHAR);
		    }
		    else
		        c_act(channel_table[cnum].to_rest, vch, translate_out_new(vch, trans_new), ch, TO_CHAR);
		    if (imp_check)
			check_improve(vch, NULL, *lang_data[ch->speaking].sn, TRUE, 2);
		}

	    }
	}

	if (num_group > 0)
	{
	    if ((ch->speaking != LANG_COMMON) && channel_table[cnum].use_lang)
            {
		sprintf(buf, channel_table[cnum].to_self_lang, lang_data[ch->speaking].name);
		c_act(buf, ch, argument, NULL, TO_CHAR);
	    }
	    else
	        c_act(channel_table[cnum].to_self, ch, argument, NULL, TO_CHAR);
	    if (ch->in_room)
	        for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
		    for (paf = vch->affected; paf; paf = paf->next)
			if ((paf->type == gsn_listen) && (paf->point == (void *) ch))
			{
		    	    if ((ch->speaking != LANG_COMMON) && channel_table[cnum].use_lang
                     	     && ((!IS_NPC(vch) && (vch->pcdata->learned[*lang_data[ch->speaking].sn] >= 25)) 
                     	     || (IS_NPC(vch) && IS_SET(ch->lang_flags, (1 << ch->speaking)))))
			    {
				sprintf(buf, channel_table[cnum].to_rest_lang, lang_data[ch->speaking].name);
				c_act(buf, vch, translate_out_new(vch, trans_new), ch, TO_CHAR);
			    }
			    else
			        c_act(channel_table[cnum].to_rest, vch, translate_out_new(vch, trans_new), ch, TO_CHAR);
		    	    if (imp_check)
				check_improve(vch, NULL, *lang_data[ch->speaking].sn, TRUE, 2);
			    check_improve(vch,ch,gsn_listen,TRUE,1);
			    break;
			}
	}
	else
	{
	   send_to_char("Quit talking to yourself!  You're not in a group.\n\r", ch);
	   return;
	}
    }
		

    if (channel == CHAN_THINK)
    {
        c_act(channel_table[cnum].to_self, ch, argument, ch, TO_CHAR);
        wiznet(channel_table[cnum].to_rest, ch, (void *) argument, WIZ_ROLEPLAY, 0, 0);
    }

    if (((channel == CHAN_SAY) || (channel == CHAN_ESAY)) && !get_room_song(origin->in_room, gsn_soundbubble))
    {
	int door;
	OBJ_DATA *obj, *obj_next;

	if (origin->in_room)
	{
	    for (door = 0; door < 6; door++)
	    {
		if (!origin->in_room->exit[door])
		    continue;

		for (vch = origin->in_room->exit[door]->u1.to_room->people; vch; vch = vch->next_in_room)
		    if (is_affected(vch, gsn_listen) && (vch->in_room->exit[get_modifier(vch->affected, gsn_listen)]->u1.to_room == ch->in_room))
		    {
			sprintf(buf, "{YYou overhear %s say '$T'{x", CPERS(origin, vch));
			c_act( buf, vch, NULL, translate_out_new(vch, trans_new), TO_CHAR);
			if (imp_check)
			    check_improve(vch, NULL, *lang_data[ch->speaking].sn, TRUE, 2);
			check_improve(vch,ch,gsn_listen,TRUE,1);
		    }
	    }
	}
		   
	mprog_speech_trigger(trans_new, origin);

	if (origin->in_room)
	    rprog_speech_trigger(argument, origin->in_room, ch);

	for (obj = origin->in_room->contents; obj; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    oprog_speech_trigger( argument, obj );
	}
	for (obj = origin->carrying; obj; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    oprog_speech_trigger( argument, obj );
	}
    }

    if (((channel == CHAN_SAY) || (channel == CHAN_ESAY)) && !str_prefix("ksashask ikakral jakulr", argument))
    {
	CHAR_DATA * target;
        OBJ_DATA * targetitem;
	
	for (target = ch->in_room->people; target != NULL; target = target->next_in_room)
	    for (targetitem = target->carrying; targetitem != NULL; targetitem = targetitem->next_content)
		if ((paf = affect_find(targetitem->affected, gsn_cursebarkja)) && (paf->modifier == ch->id))
		{
		    act("At your words, thin ribbons of darkness writhe free from $p, fading eventually into nothingness.", ch, NULL, targetitem, TO_CHAR);
		    act("At $n's words, thin ribbons of darkness writhe free from $p, fading eventually into nothingness.", ch, NULL, targetitem, TO_ROOM);
		    affect_remove_obj(targetitem, paf);
	        }
    }
    
    if (!IS_AFFECTED(origin, AFF_CHARM) && !IS_NPC(origin) && !IS_IMMORTAL(origin))
    {
	memset(buf, 0, sizeof(buf));
	strip_punc(argument, buf);

	if (is_name_nopunc("ashur", buf))
	    spoke_ashur(origin);
    }

    free(trans_new);

    return;
}

void char_from_acct(CHAR_DATA *ch, char *reason)
{
    char *cf, *cp, *ct;
    char buf[MAX_STRING_LENGTH];
    int	j = 0;
    unsigned int i;

    if (!ch->desc || !ch->desc->acct)
	return;

    if (ch->level > 1)
    {
	sprintf(buf, "%s%s (Level %d %s %s, %s, %d points)\n\r", ch->desc->acct->deleted, ch->name, ch->level, race_table[ch->race].name, class_table[ch->class_num].name, reason, ch->pcdata->award_points);
	free_string(ch->desc->acct->deleted);
	ch->desc->acct->deleted = str_dup(buf);
    }

    memset((void *) buf, 0, MAX_STRING_LENGTH);
    ct = buf;

    if (ch->name != NULL && (cp = strstr(ch->desc->acct->chars, capitalize(ch->name))) != NULL)
    {
	cf = ch->desc->acct->chars;
	for (i = 0; i < strlen(ch->desc->acct->chars); i++)
	{
	    if (cf == cp)
		j = strlen(ch->name);

	    if (j == 0)
		*ct++ = *cf;
	    else
		j -= 1;

	    cf++;
	}

	free_string(ch->desc->acct->chars);
	ch->desc->acct->chars = str_dup(buf);
    }
    fwrite_account(ch->desc->acct);
    
}

/* RT code to delete yourself */

void do_delet( CHAR_DATA *ch, char *argument)
{
    send_to_char("You must type the full command to delete yourself.\n\r",ch);
}

void do_delete( CHAR_DATA *ch, char *argument)
{
//   char strsave[MAX_INPUT_LENGTH];

   if (IS_NPC(ch))
        return;

   if (ch->pcdata->confirm_delete)
   {
        if (argument[0] != '\0')
        {
            send_to_char("Delete status removed.\n\r",ch);
            ch->pcdata->confirm_delete = FALSE;
            return;
        }
        else
        {
            wiznet("$N decides to leave the realm forever.",ch,NULL,0,0,0);
            stop_fighting_all(ch);
            ch->pcdata->adrenaline = 0;
            
			if (ch->played < 25 * NUM_SECONDS_HOUR)
            {
                act("$n decides to never return to this realm again.",ch,
                NULL,NULL,TO_ROOM);
                char_from_room(ch);
                char_to_room(ch, get_room_index(ROOM_VNUM_TEMPLE));
				delete_char(ch);
				char_from_acct(ch, "deleted");   /* must be after do_quit */
                return;
            }

			// Put them on ice for a week
			send_to_char("Your character will be deleted in one week's time, unless you play again.\n\r", ch);
			ch->pcdata->delete_requested = time(0);

			// Move them to the limbo room and force a quit
			ch->was_in_room = ch->in_room;
			char_from_room(ch);
			char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
			do_quit(ch, "");
			return;
		}
	}

		//deny_char(ch);
		//char_from_acct(ch, "deleted");

    if (argument[0] != '\0')
    {
        send_to_char("Just type delete. No argument.\n\r",ch);
        return;
    }

    if (ch->pcdata->adrenaline > 0)
    {
	send_to_char("You cannot delete while your adrenaline is gushing.  Please take a moment to calm down.\n\r", ch);
	return;
    }

    send_to_char("Type delete again to confirm this command.\n\r",ch);
	if (ch->played < 25 * NUM_SECONDS_HOUR)
	    send_to_char("{rWARNING{x: this command is irreversible.\n\r",ch);
	else
		send_to_char("{rNOTICE{x: your character will be put on delete status for a week.\n\rAt any point during that week, you can change your mind and log back in, which will automatically cancel the delete.\n\r", ch);
    send_to_char("Typing delete with an argument will undo delete status.\n\r", ch);
    ch->pcdata->confirm_delete = TRUE;
    wiznet("$N is contemplating deletion.",ch,NULL,0,0,get_trust(ch));
}

void do_xmusic( CHAR_DATA *ch, char *argument )
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->comm, COMM_XMUSIC))
    {
	send_to_char("MSP Music Extentions have now been turned off.\n\r", ch);
	REMOVE_BIT(ch->comm, COMM_XMUSIC);
    }
    else
    {
	send_to_char("MSP Music Extentions have now been turned on.\n\r", ch);
	SET_BIT(ch->comm, COMM_XMUSIC);
    }

    return;
}

void do_playmusic( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
	return;

    if (!IS_SET(ch->comm, COMM_XMUSIC))
    {
	send_to_char("MSP Music Extentions are currently disabled.\n\r", ch);
	send_to_char("You may toggle this with the 'xmusic' command.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("!!MUSIC(Off)Music file stopped.\n\r", ch);
	return;
    }

    sprintf(buf, "!!MUSIC(%s L=-1 C=0 U=http://www.ender.com/pantheon/sounds/%s)", argument, argument);
    send_to_char(buf, ch);

    return;
}

void do_translate( CHAR_DATA *ch, char *argument )
{

    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->comm, COMM_TRANSLATE))
    {
	send_to_char("You will no longer see the translated text when speaking.\n\r", ch);
	REMOVE_BIT(ch->comm, COMM_TRANSLATE);
    }
    else
    {
	send_to_char("You will now see the fully translated text when speaking.\n\r", ch);
	SET_BIT(ch->comm, COMM_TRANSLATE);
    }

    return;
}

/* RT code to display channel status */

void do_channels( CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    /* lists all channels and their status */
    send_to_char("   channel     status\n\r",ch);
    send_to_char("---------------------\n\r",ch);

    send_to_char("auction        ",ch);
    if (!IS_SET(ch->comm,COMM_NOAUCTION))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    send_to_char("music          ",ch);
    if (!IS_SET(ch->comm,COMM_NOMUSIC))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    send_to_char("Q/A            ",ch);
    if (!IS_SET(ch->comm,COMM_NOQUESTION))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    if (IS_IMMORTAL(ch))
    {
      send_to_char("god channel    ",ch);
      if(!IS_SET(ch->comm,COMM_NOWIZ))
        send_to_char("ON\n\r",ch);
      else
        send_to_char("OFF\n\r",ch);
    }

    send_to_char("tells          ",ch);
    if (!IS_SET(ch->comm,COMM_DEAF))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);

    send_to_char("quiet mode     ",ch);
    if (IS_SET(ch->comm,COMM_QUIET))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    if (IS_SET(ch->nact, PLR_NOWEATHER))
	send_to_char("You are currently not receiving weather messages.\n\r", ch);

    if (IS_SET(ch->comm,COMM_AFK))
        send_to_char("You are AFK.\n\r",ch);

    if (IS_SET(ch->comm,COMM_SNOOP_PROOF))
        send_to_char("You are immune to snooping.\n\r",ch);

    if (ch->lines != PAGELEN)
    {
        if (ch->lines)
        {
            sprintf(buf,"You display %d lines of scroll.\n\r",ch->lines+2);
            send_to_char(buf,ch);
        }
        else
            send_to_char("Scroll buffering is off.\n\r",ch);
    }

    if (ch->prompt != NULL)
    {
        sprintf(buf,"Your current prompt is: %s\n\r",ch->prompt);
        send_to_char(buf,ch);
    }

    if ((ch->desc) && (ch->desc->acct) && (ch->desc->acct->def_prompt))
    {
	sprintf(buf,"Your default prompt is: %s\n\r",ch->desc->acct->def_prompt);
	send_to_char(buf,ch);
    }
    
    if (IS_SET(ch->comm,COMM_NOSHOUT))
      send_to_char("You cannot shout.\n\r",ch);

    if (IS_SET(ch->comm,COMM_NOTELL))
      send_to_char("You cannot use tell.\n\r",ch);

    if (IS_SET(ch->comm,COMM_NOCHANNELS))
     send_to_char("You cannot use channels.\n\r",ch);

    if (IS_SET(ch->comm,COMM_NOEMOTE))
      send_to_char("You cannot show emotions.\n\r",ch);

}

/* RT deaf blocks out all shouts */

void do_deaf( CHAR_DATA *ch, char *argument)
{

   if (IS_SET(ch->comm,COMM_DEAF))
   {
     send_to_char("You can now hear tells again.\n\r",ch);
     REMOVE_BIT(ch->comm,COMM_DEAF);
   }
   else
   {
     send_to_char("From now on, you won't hear tells.\n\r",ch);
     SET_BIT(ch->comm,COMM_DEAF);
   }
}

/* RT quiet blocks out all communication */

/* Currently removed from interp list.  July 18/00.  Aeolis

void do_quiet ( CHAR_DATA *ch, char * argument)
{
    if (IS_SET(ch->comm,COMM_QUIET))
    {
      send_to_char("Quiet mode removed.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_QUIET);
    }
   else
   {
     send_to_char("From now on, you will only hear says and emotes.\n\r",ch);
     SET_BIT(ch->comm,COMM_QUIET);
   }
}

*/

void do_oocoff ( CHAR_DATA *ch, char * argument)
{
    if (IS_SET(ch->comm,COMM_OOCOFF))
    {
      send_to_char("You are willing to speak OOC again.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_OOCOFF);
    }
   else
   {
     send_to_char("You close your ears to OOC talk.\n\r",ch);
     SET_BIT(ch->comm,COMM_OOCOFF);
   }

}

/* afk command */

void do_afk ( CHAR_DATA *ch, char * argument)
{
    if (IS_SET(ch->comm,COMM_AFK))
    {
      send_to_char("AFK mode removed. Type 'replay' to see tells.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_AFK);
    }
   else
   {
     send_to_char("You are now in AFK mode.\n\r",ch);
     SET_BIT(ch->comm,COMM_AFK);
   }
}

void do_replay (CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
    {
        send_to_char("You can't replay.\n\r",ch);
        return;
    }

    if (buf_string(ch->pcdata->buffer)[0] == '\0')
    {
        send_to_char("You have no tells to replay.\n\r",ch);
        return;
    }

    page_to_char(buf_string(ch->pcdata->buffer),ch);
    clear_buf(ch->pcdata->buffer);
}

void check_comm ( CHAR_DATA *ch, char *str )
{
    int i;
    char *oocpoint;
    char sbuf[MAX_STRING_LENGTH];

    if (IS_NPC(ch) && !ch->desc)
	return;

    if (!IS_NPC(ch))
	if (IS_SET(ch->act, PLR_SLOG))
	    write_slog(ch, stripcolor(sbuf, str));

    sprintf(sbuf, "%s> %s", ch->name, str);

    if (ch->desc && ch->desc->snooped)
	for (i = 0; i < MAX_SNOOP; i++)
	    if (ch->desc->snoop_by[i] && ch->desc->snoop_type[i] == SNOOP_COMM)
	        send_to_char(sbuf, ch->desc->snoop_by[i]->character);

    if (!IS_NPC(ch))
    {
	oocpoint = strstr(str, "OOC");

	if (IS_OAFFECTED(ch, AFF_READTHOUGHTS) && (oocpoint != (str+2)) && (oocpoint != (str+3)))
	{
	    AFFECT_DATA *paf;

	    for (paf = ch->affected; paf; paf = paf->next)
		{
			if (paf->type == gsn_read_thoughts)
		    	send_to_char(sbuf, (CHAR_DATA*)paf->point);
		}
	}
    }
}

/* RT auction rewritten in ROM style */
void do_auction( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) && ch->master != NULL )
        return;

    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOAUCTION))
      {
        send_to_char("Auction channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOAUCTION);
      }
      else
      {
        send_to_char("Auction channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOAUCTION);
      }
    }
    else  /* auction message sent, turn auction on if it is off */
    {
        REMOVE_BIT(ch->comm,COMM_NOAUCTION);

	if (!ch->in_room)
	    return;

        comm_send(ch, ch->in_room->area, argument, CHAN_AUCTION); 
    }
    return;
}

void do_think( CHAR_DATA *ch, char *argument )
{
    if (IS_NPC(ch))
    {
        send_to_char("You lack the ability to think willfully.\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        send_to_char("What do you wish to think?\n\r", ch);
        return;
    }

    comm_send(ch, ch, argument, CHAN_THINK);
}

/* RT music channel */
void do_music( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) && ch->master != NULL )
        return;

    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOMUSIC))
      {
        send_to_char("Music channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOMUSIC);
      }
      else
      {
        send_to_char("Music channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOMUSIC);
      }
    }
    else  /* music sent, turn music on if it isn't already */
    {
        REMOVE_BIT(ch->comm,COMM_NOMUSIC);

	if (!ch->in_room)
	    return;

	comm_send(ch, ch->in_room->area, argument, CHAN_MUSIC);
    }
}

void do_druidtalk( CHAR_DATA *ch, char *argument )
{
    if (ch->class_num != global_int_class_druid)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (!is_affected(ch, gsn_commune_nature))
    {
	send_to_char("Your mind is not open to the voices of nature.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("What do you wish to commune to nature?\n\r", ch);
	return;
    }

    comm_send(ch, &ch->class_num, argument, CHAN_DRUID);

    return;
}    
 

/* clan channels */
void do_clantalk( CHAR_DATA *ch, char *argument )
{
    if (!is_clan(ch) || clan_table[ch->clan].independent)
    {
        send_to_char("You are not a member of a house.\n\r",ch);
        return;
    }
    if ( argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOCLAN))
      {
        send_to_char("House channel is now ON\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOCLAN);
      }
      else
      {
        send_to_char("House channel is now OFF\n\r",ch);
        SET_BIT(ch->comm,COMM_NOCLAN);
      }
      return;
    }

    REMOVE_BIT(ch->comm,COMM_NOCLAN);

    comm_send(ch, &ch->clan, argument, CHAN_HOUSE);

    return;
}

void do_newbietalk( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char sbuf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;

    if ( argument[0] == '\0' )
    {
	if (IS_SET(ch->comm,COMM_NONEWBIE))
	    send_to_char("Newbie channel is now {WON{x.\n\r",ch);
	else
	    send_to_char("Newbie channel is now {WOFF{x.\n\r",ch);
	TOGGLE_BIT(ch->comm, COMM_NONEWBIE);
	return;
    }

    if (IS_SET(ch->comm, COMM_NONEWBIE))
    {
	send_to_char("Newbie channel is currently off.\n\r", ch);
	return;
    }

    sprintf(log_buf, "O: [Newbie][%s]: %s\n\r", ch->name, argument );
    check_comm(ch, log_buf);

	const char * nameString(ch->short_descr);
	const char * immNameString(ch->short_descr);
	if (IS_NPC(ch))
	{
		// If an imm is switched in, use the immortal name rather than mob name
		if (ch->desc != NULL && ch->desc->original != NULL)
		{
// We only want to sub immortal name if the switched person is actually an
// immortal. Astral projections are usually not.
		    if (IS_IMMORTAL(ch))
		        nameString = "Staff";
	            else
		        nameString = ch->desc->original->name;
		    immNameString = ch->desc->original->name;
		}
	}
	else
	{
		// Imms always see the character name
		immNameString = ch->name;

		if (IS_IMMORTAL(ch))
			nameString = "Immortal";
		else
			nameString = ch->name;
	}

    sprintf(buf, "{c[{gNewbie{c][{G%s{c]: {C%s{x\n\r", nameString, argument);

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        victim = d->original ? d->original : d->character;

        if ( d->connected == CON_PLAYING
			 && !IS_IMMORTAL(victim)
			 && (victim->level < NEWBIE_CHANNEL_LEVEL
	  		 || (d->acct && IS_SET(d->acct->flags, ACCT_MENTOR)))
	         && !IS_SET(victim->comm, COMM_NONEWBIE))
	    send_to_char(buf, d->character);
    }

    sprintf(buf, "{c[{gNewbie{c][{G%s{c]: {C%s{x", immNameString, argument);
    write_newbielog( stripcolor(sbuf, buf) );

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        victim = d->original ? d->original : d->character;

        if ( d->connected == CON_PLAYING
	 && IS_IMMORTAL(victim)
         && !IS_SET(victim->comm, COMM_NONEWBIE))
	{
	    send_to_char(buf, d->character);
	    send_to_char("\n\r", d->character);
	}
    }

    return;
}

void do_imptalk( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;

    if ( argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOWIZ))
      {
        send_to_char("Immortal channel is now ON\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOWIZ);
      }
      else
      {
        send_to_char("Immortal channel is now OFF\n\r",ch);
        SET_BIT(ch->comm,COMM_NOWIZ);
      }
      return;
    }

    REMOVE_BIT(ch->comm,COMM_NOWIZ);

    act_new("{c[IMP][{r$n{c]: {y$t{x",ch,argument,NULL,TO_CHAR,POS_DEAD);

    sprintf(log_buf, "O: [IMP][%s]: %s\n\r", ch->name, argument );
    check_comm(ch, log_buf);

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( d->connected == CON_PLAYING &&
                d->character->trust > 59 &&
             !IS_SET(d->character->comm,COMM_NOWIZ) )
        {
            act_new("{c[IMP][{r$n{c]: {y$t{x",ch,argument,d->character,TO_VICT,POS_DEAD);
        }
    }
}

void do_immtalk( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if ( argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOWIZ))
      {
        send_to_char("Immortal channel is now ON\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOWIZ);
      }
      else
      {
        send_to_char("Immortal channel is now OFF\n\r",ch);
        SET_BIT(ch->comm,COMM_NOWIZ);
      }
      return;
    }

    REMOVE_BIT(ch->comm,COMM_NOWIZ);

    sprintf( buf, "{c[{y$n{c]: %s{x", argument );
    act_new("{B[{r$n{B]: {g$t{x",ch,argument,NULL,TO_CHAR,POS_DEAD);

    sprintf(log_buf, "O: [%s]: %s\n\r", ch->name, argument );
    check_comm(ch, log_buf);

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
    CHAR_DATA *victim;

        victim = d->original ? d->original : d->character;

        if ( d->connected == CON_PLAYING &&
             victim->trust > 51 &&
             !IS_SET(victim->comm,COMM_NOWIZ) )
        {
            c_act_new("{B[{r$n{B]: {g$t{x",ch,argument,d->character,TO_VICT,POS_DEAD);
        }
    }

    return;
}

void do_wiztalk( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int targetlevel;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOWIZ))
      {
        send_to_char("Immortal channel is now ON\n\r",ch);
        send_to_char("Immortal channel is now ON\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOWIZ);
      }
      else
      {
        send_to_char("Immortal channel is now OFF\n\r",ch);
        SET_BIT(ch->comm,COMM_NOWIZ);
      }
      return;
    }

    if (argument[0] == '\0' )
    {
       send_to_char("Tell them what?\n\r",ch);
       return;
    }

    targetlevel = atoi( arg );

    if (targetlevel < 52 )
    {
       send_to_char("Tell what ranks what?\n\r",ch);
       return;
    }

    REMOVE_BIT(ch->comm,COMM_NOWIZ);

    sprintf(buf, "{B[{r$n{B][%d]: {g$t{x", targetlevel);
    act_new(buf,ch,argument,NULL,TO_CHAR,POS_DEAD);

    sprintf(log_buf, "O: [%s][%d]: %s\n\r", ch->name, targetlevel, argument );
    check_comm(ch, log_buf);

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( d->connected == CON_PLAYING &&
             d->character->trust > 51 &&
             !IS_SET(d->character->comm,COMM_NOWIZ) &&
             (d->character->trust >= targetlevel))
        {
            c_act_new(buf,ch,argument,d->character,TO_VICT,POS_DEAD);
        }
    }

    return;
}

void do_mimic(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *vch, *victim = NULL;

    if (argument[0] == '\0')
    {
	send_to_char("Whom do you wish to mimic?\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if ((vch = get_char_world(ch, arg)) == NULL)
    {
	send_to_char("You cannot mimic those not in the world.\n\r", ch);
	return;
    }

    if (IS_NPC(vch))
    {
	send_to_char("Only other players may be mimicked.\n\r", ch);
	return;
    }

    if (vch == ch)
    {
	send_to_char("You attempt to mimic yourself... and fail.\n\r", ch);
	return;
    }

    if (IS_IMMORTAL(vch) && !IS_IMMORTAL(ch))
    {
	char buf[MAX_STRING_LENGTH];
	
	send_to_char("Immortals may not be mimicked.\n\rThe gods are angered by your actions!\n\r", ch);
        act("A bolt of lightning from above {rstrikes{x you!", ch, NULL, NULL, TO_CHAR);
        act("A bolt of lightning from above {rstrikes{x $n!",  ch, NULL, NULL, TO_ROOM);

        if (ch->hit > 1)
	    ch->hit /= 2;
	if (ch->mana > 1)
            ch->mana /= 2;
	if (ch->move > 1)
            ch->move /= 2;

	sprintf(buf, "%s tried to mimic %s, and was smited from the heavens.", IS_NPC(ch) ? ch->short_descr : ch->name, vch->name);
	wiznet(buf, ch, NULL, WIZ_ROLEPLAY, 0, 0);

	return;
    }

    argument = one_argument(argument, arg);

    if ((argument[0] == '\0') || (arg[0] == '\0'))
    {
	send_to_char("What do you wish to mimic?\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "tell"))
    {
	char arg1[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *d;

	if ( IS_SET(ch->comm, COMM_NOTELL) || IS_SET(ch->comm,COMM_DEAF))
        {
            send_to_char("Your message didn't get through.\n\r", ch);
            return;
        }

	argument = one_argument( argument, arg1 );

	if ( arg1[0] == '\0' || argument[0] == '\0' )
        {
            send_to_char( "Tell whom what?\n\r", ch );
            return;
        }

        if (!str_cmp(arg1, "self"))
	    victim = ch;
        else if (is_affected(ch, gsn_confusion) && (number_bits(1) == 0))
	    victim = get_random_pc_world(ch);
        else 
        {
	
	    for (d = descriptor_list; d; d = d->next)
	        if ((d->connected == CON_PLAYING) && d->character
	         && !IS_NPC(d->character) && is_name(arg1, d->character->name)
	         && can_see_comm(ch, d->character))
	        {
		    victim = d->character;
		    break;
	        }

	    if (!victim)
	    {
 
	        if (((victim = get_char_world(ch, arg)) == NULL)
                 || (IS_NPC(victim) && (victim->in_room != ch->in_room))
	         || !can_see_comm(ch, victim))
    	        {
        	    send_to_char( "They aren't here.\n\r", ch );
        	    return;
    	        }
	    }
        }

        if ( victim->desc == NULL && !IS_NPC(victim))
        {
            c_act("$N seems to have misplaced $S link...", ch,NULL,victim,TO_CHAR);
            return;
        }

        if ( !(IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL) && !IS_AWAKE(victim)
            && !is_affected(ch, gsn_dreamspeak))
        {
            c_act( "$E can't hear you.", ch, 0, victim, TO_CHAR );
            return;
        }

        if ((IS_SET(victim->comm,COMM_QUIET) || IS_SET(victim->comm,COMM_DEAF))
        && !IS_OAFFECTED(victim, AFF_DEAFEN)
        && !IS_IMMORTAL(ch))
        {
            c_act( "$E is not receiving tells.", ch, 0, victim, TO_CHAR );
            return;
        }

        if (IS_SET(victim->comm,COMM_AFK))
        {
            c_act("$E is AFK, and not receiving tells.",ch,NULL,victim,TO_CHAR);
            return;
        }

	global_mimic = ch;
	comm_send(vch, victim, argument, CHAN_TELL);
    }
    else
    {
        global_mimic = ch;

	if (!str_cmp(arg, "say"))
	    comm_send(vch, ch->in_room, argument, CHAN_SAY);
	else if (!str_cmp(arg, "yell"))
	    comm_send(vch, ch->in_room->area, argument, CHAN_YELL);
	else if (!str_cmp(arg, "music"))
	    comm_send(vch, ch->in_room->area, argument, CHAN_MUSIC);
	else if (!str_cmp(arg, "druid") || !str_cmp(arg, "dt"))
	    comm_send(vch, &ch->class_num, argument, CHAN_DRUID);
	else if (!str_cmp(arg, "house") || !str_cmp(arg, "ht"))
	    comm_send(vch, &ch->clan, argument, CHAN_HOUSE);
	else if (!str_cmp(arg, "auction"))
	    comm_send(vch, ch->in_room->area, argument, CHAN_AUCTION);
	else if (!str_cmp(arg, "question"))
	    comm_send(vch, ch->in_room->area, argument, CHAN_QUESTION);
	else if (!str_cmp(arg, "answer"))
	    comm_send(vch, ch->in_room->area, argument, CHAN_ANSWER);
	else if (!str_cmp(arg, "sing"))
	    comm_send(vch, ch->in_room, argument, CHAN_SING);
	else if (!str_cmp(arg, "esay"))
	    comm_send(vch, ch->in_room, argument, CHAN_ESAY);
	else
	{
	    global_mimic = NULL;
	    send_to_char("You cannot mimic that.\n\r", ch);
	    return;
	}
    }

    global_mimic = NULL;

    if (get_skill(ch, gsn_mimic) > 0)
	check_improve(ch, NULL, gsn_mimic, TRUE, 4);

    return;
}

/*
 * Written for the Carrion Fields by Derit
 *
 * Pray channel for immortals to hear mortal questions
 *
 */
void do_pray(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];

    if (IS_SET(ch->comm,COMM_NOCHANNELS))
    {
	send_to_char("The gods refuse to listen to you right now.",ch);
        return;
    }

    if (argument[0] == '\0')
    {
        send_to_char("Pray what?\n\r", ch);
        send_to_char("This is not an emote, but a channel to the immortals.\n\r", ch);
        send_to_char("For best results, tell us what or why you are praying.\n\r", ch);
        send_to_char("See also: 'help pray' and 'help prayextra'\n\r", ch);
        return;
    }

    send_to_char("You pray to the heavens for help!\n\r",ch);

    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_SLOG))
    {
        sprintf(log_buf, "O: You pray '%s'", argument );
        write_slog(ch, log_buf);
    }

    sprintf(buf, "{m%s is PRAYING for [Room %d]: %s{x\n\r", (IS_NPC(ch) ? ch->short_descr : ch->name), (ch->in_room == NULL ? -1 : ch->in_room->vnum), argument);
    log_string(buf);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING && (d->original ? (d->original->trust > 51) : (d->character->trust > 51)) && !IS_SET(d->character->comm,COMM_NOWIZ))
    	    send_to_char(buf, d->character);
    }
}

void do_cant( CHAR_DATA *ch, char *argument )
{
    bool failed = FALSE;
    CHAR_DATA *vch;
    int skill;

    if ((skill = get_skill(ch,gsn_thievescant)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Cant what?\n\r", ch );
        return;
    }

    if (skill < number_percent())
    {
	failed = TRUE;
        check_improve(ch,NULL, gsn_thievescant, FALSE, 8);
    }

    if (!validcolour (ch, argument))
    {
        send_to_char("You aren't allowed to use colour.\n\r", ch);
        return;
    }

    act( "{WYou gesture, {G'$t'{x", ch, argument, NULL, TO_CHAR );

    sprintf(log_buf, "O: You gesture, '%s'\n\r", argument );
    check_comm(ch, log_buf);

    for (vch = ch->in_room->people; vch !=NULL;vch = vch->next_in_room)
        if (get_skill(vch, gsn_thievescant) > 0)
	    act( "{W$n gestures, {G'$t'{x", ch, argument, vch, TO_VICT );
        else if (failed)
	    act( "$n gestures in the thieves cant, failing to be subtle.", ch, NULL, vch, TO_VICT);

    for (vch = ch->in_room->people; vch ; vch = vch->next_in_room)
	if (check_detectstealth(vch,ch))	
	    act( "{W$n gestures, {G'$t'{x", ch, argument, vch, TO_VICT );
        else if (failed)
	    act( "$n gestures in the thieves cant, failing to be subtle.", ch, NULL, vch, TO_VICT);

    return;
}

void do_say( CHAR_DATA *ch, char *argument )
{
    if ( argument[0] == '\0' )
    {
        send_to_char( "Say what?\n\r", ch );
        return;
    }

    comm_send(ch, ch->in_room, argument, CHAN_SAY);

    return;
}

void do_esay( CHAR_DATA *ch, char *argument )
{
    if (argument[0] == '\0')
    {
        send_to_char( "Say what?\n\r", ch );
        return;
    }
 
    comm_send(ch, ch->in_room, argument, CHAN_ESAY);

    return;
} 

void do_tell_target(CHAR_DATA * ch, CHAR_DATA * victim, char * argument)
{
    char buf[MAX_STRING_LENGTH];
    if (victim->desc == NULL && !IS_NPC(victim))
    {
        c_act("$N seems to have misplaced $S link...", ch,NULL,victim,TO_CHAR);
        sprintf(buf,"%s tells you '%s'\n\r",APERS(ch,victim),argument);
        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer,buf);
        return;
    }

    if ( !(IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL) && !IS_AWAKE(victim)
        && !is_affected(ch, gsn_dreamspeak))
    {
        c_act( "They can't hear you.", ch, 0, victim, TO_CHAR );
        return;
    }

    if ((IS_SET(victim->comm,COMM_QUIET) || IS_SET(victim->comm,COMM_DEAF))
    && !IS_OAFFECTED(victim, AFF_DEAFEN)
    && !IS_IMMORTAL(ch))
    {
        c_act( "$E is not receiving tells.", ch, 0, victim, TO_CHAR );
        return;
    }

    if (IS_SET(victim->comm,COMM_AFK))
    {
        if (IS_NPC(victim))
        {
            c_act("$E is AFK, and not receiving tells.",ch,NULL,victim,TO_CHAR);
            return;
        }

        c_act("$E is AFK, but your tell will go through when $E returns.",
            ch,NULL,victim,TO_CHAR);
        sprintf(buf,"%s tells you '%s'\n\r",APERS(ch,victim),argument);
        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer,buf);
        return;
    }

    comm_send(ch, victim, argument, CHAN_TELL);
}

void do_tell( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim = NULL;
    DESCRIPTOR_DATA *d;

    if ( IS_SET(ch->comm, COMM_NOTELL) || IS_SET(ch->comm,COMM_DEAF))
    {
        send_to_char( "Your message didn't get through.\n\r", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
        send_to_char( "Tell whom what?\n\r", ch );
        return;
    }

    /*
     * Can tell to PC's anywhere, but NPC's only in same room.
     * -- Furey
     */
    if (!str_cmp(arg, "reply"))
    {
	victim = ch->reply;

	if (!victim || !IS_VALID(victim))
	{
	   send_to_char("You have no one to reply to.\n\r", ch);
	   return;
	}
    }
    else if (!str_cmp(arg, "self"))
	victim = ch;
    else if ( is_affected(ch, gsn_confusion) && (number_bits(1) == 0))
	victim = get_random_pc_world(ch);
    else 
    {
	/* First we'll go through all the descriptors.  This change brought */
	/* about due to mirror image naming conflicts.	-- Aeolis.	    */

	for (d = descriptor_list; d; d = d->next)
	    if ((d->connected == CON_PLAYING) && d->character
	     && !IS_NPC(d->character) && is_name(arg, d->character->name)
	     && can_see_comm(ch, d->character))
	    {
		victim = d->character;
		break;
	    }

        if (!victim)
        {

            if (((victim = get_char_world(ch, arg)) == NULL)
                 || (IS_NPC(victim) && (victim->in_room != ch->in_room))
             || !can_see_comm(ch, victim))
                {
                send_to_char( "They aren't here.\n\r", ch );
                return;
                }
        }
    }
    do_tell_target(ch, victim, argument);
}

void do_reply( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if (!ch->reply)
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    /* Honestly, I realize this is a bit of a hack, but I can't see any */
    /* logical reason this would ever cause problems.  -- Aeolis.       */

    sprintf(buf, "reply %s", argument);
    do_tell(ch, buf);
}

static void ooctell_helper( CHAR_DATA *ch, CHAR_DATA * victim, const char * argument)
{
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0')
    {
        send_to_char("What did you want to say?\n", ch);
        return;
    }

    if (IS_SET(ch->comm, COMM_NOTELL))
    {
        send_to_char( "Your message didn't get through.\n\r", ch );
        return;
    }

    if (IS_SET(ch->comm, COMM_NOOOC))
    {
        send_to_char("Your ooc privileges have been revoked.\n\r", ch);
        return;
    }

    if (IS_SET(ch->comm,COMM_OOCOFF))
    {
        send_to_char("You must open yourself to OOC first.\n\r",ch);
        return;
    }

    if ( victim->desc == NULL && !IS_NPC(victim))
    {
        c_act("$N seems to have misplaced $S link...try again later.", ch,NULL,victim,TO_CHAR);
        sprintf(buf,"[OOC] %s: %s\n\r",PERS(ch,victim),argument);
        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer,buf);
        return;
    }

    if ((IS_SET(victim->comm,COMM_QUIET) || IS_SET(victim->comm,COMM_DEAF)) && !IS_IMMORTAL(ch))
    {
        c_act( "$E is not receiving tells.", ch, 0, victim, TO_CHAR );
        return;
    }

    if (IS_SET(victim->comm,COMM_OOCOFF))
    {
        c_act( "$E does not wish to speak OOC.", ch, 0, victim, TO_CHAR );
        return;
    }

    if (IS_SET(victim->comm,COMM_AFK))
    {
        if (IS_NPC(victim))
        {
            c_act("$E is AFK, and not receiving tells.",ch,NULL,victim,TO_CHAR);
            return;
        }

        c_act("$E is AFK, but your tell will go through when $E returns.", ch,NULL,victim,TO_CHAR);
        sprintf(buf,"[OOC] %s: %s\n\r",PERS(ch,victim),argument);
        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer,buf);
        return;
    }

    comm_send(ch, victim, const_cast<char*>(argument), CHAN_OOC);
}


void do_ooctell( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim = NULL;
    DESCRIPTOR_DATA *d;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
        send_to_char( "Tell whom what?\n\r", ch );
        return;
    }

    /* First we'll go through all the descriptors.  This change brought */
    /* about due to mirror image naming conflicts.	-- Aeolis.      */

    for (d = descriptor_list; d; d = d->next)
	if ((d->connected == CON_PLAYING) && d->character
	 && !IS_NPC(d->character) && is_name(arg, d->character->name)
	 && can_see_comm(ch, d->character))
	{
	    victim = d->character;
	    break;
	}

    if (!victim)
    {
        if (((victim = get_char_world(ch, arg)) == NULL)
         || (IS_NPC(victim) && (victim->in_room != ch->in_room))
	 || !can_see_comm(ch, victim))
	{
            send_to_char( "They aren't here.\n\r", ch );
            return;
    	}
    }

    ooctell_helper(ch, victim, argument);
}

void do_oocreply(CHAR_DATA * ch, char * argument)
{
    if (!ch->oocreply)
    {
        send_to_char( "They aren't here.\n", ch );
        return;
    }

    ooctell_helper(ch, ch->oocreply, argument);
}

void do_freetell( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;

    if (argument[0] == '\0')
        {
        send_to_char("Free who's tell?\n\r", ch);
        return;
        }

    if ((victim = get_char_world(ch, argument)) == NULL)
        {
        send_to_char("You can't find them.\n\r", ch);
        return;
        }

    if (!IS_NPC(victim))
        victim->reply = NULL;

}

void do_char_yell( CHAR_DATA *ch, char *argument )
{
    if (!ch->in_room)
	return;

    if ( IS_SET(ch->comm, COMM_NOSHOUT) )
    {
        send_to_char( "You can't yell.\n\r", ch );
        return;
    }

    if ( IS_NPC(ch) && ch->master != NULL )
        return;

    if ( argument[0] == '\0' )
    {
        send_to_char( "Yell what?\n\r", ch );
        return;
    }

    comm_send(ch, ch->in_room->area, argument, CHAN_CYELL); 

    return;
}

void do_autoyell( CHAR_DATA *ch, char *argument )
{
    if (IS_NPC(ch) || !IS_SET(ch->nact, PLR_NOYELL))
	do_yell(ch, argument);

    return;
}

void do_yell( CHAR_DATA *ch, char *argument )
{
    bool use_esp = is_affected(ch, gsn_esp);

    if (!ch->in_room)
	return;

    if ( IS_SET(ch->comm, COMM_NOSHOUT) )
    {
        send_to_char( "You can't yell.\n\r", ch );
        return;
    }

/*
    if ( IS_NPC(ch) && ch->master != NULL )
        return;
*/

    if ( argument[0] == '\0' )
    {
        send_to_char( "Yell what?\n\r", ch );
        return;
    }

    if (!use_esp && IS_NAFFECTED(ch, AFF_MUFFLE))
    {
          send_to_char("You try to yell, but your screams are muffled by a quick blow to the neck.\n\r", ch);
          return;
    }

    if (!IS_NPC(ch) && ch->in_room && area_is_affected(ch->in_room->area, gsn_pillage))
        argument = "Help! Help!";

    comm_send(ch, ch->in_room->area, argument, CHAN_YELL);
    return;
}

void do_noyell( CHAR_DATA *ch, char *argument )
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->nact, PLR_NOYELL))
    {
	send_to_char("You will now yell under normal circumstances.\n\r", ch);
	REMOVE_BIT(ch->nact, PLR_NOYELL);
    }
    else
    {
	send_to_char("You will no longer yell under most involuntary circumstances.\n\r", ch);
	SET_BIT(ch->nact, PLR_NOYELL);
    }

    return;
}

void do_sing( CHAR_DATA *ch, char *argument )
{
    if (( ch->class_num < 24 || ch->class_num > 25) ||
        IS_SET(ch->comm, COMM_NOSHOUT))
    {
        send_to_char( "You can't sing.\n\r", ch );
        return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Sing what?\n\r", ch );
        return;
    }

    comm_send(ch, ch->in_room, argument, CHAN_SING);
    return;
}

void do_emote( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    bool use_esp = is_affected(ch, gsn_esp);

    if (!ch->in_room)
	return;

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
    {
        send_to_char( "You can't show your emotions.\n\r", ch );
        return;
    }

        if (!use_esp && IS_NAFFECTED(ch, AFF_GARROTE_VICT))
        {
          send_to_char("Your neck burns, and you struggle to speak, but can't.\n\r", ch);
          return;
        }

	if (!use_esp && is_affected(ch, gsn_gag))
	{
	  send_to_char("Your mouth is gagged, you cannot speak!\n\r", ch);
	  return;
	}
    if (!use_esp && get_modifier(ch->affected,gsn_wrathofthevoid) == 1)
    {
	send_to_char("The green ichor still binds your mouth.\n\r",ch);
	return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Emote what?\n\r", ch );
        return;
    }

    if (!validcolour (ch, argument))
        {
        send_to_char("You aren't allowed to use colour.\n\r", ch);
        return;
        }

    if (is_affected(ch, gsn_moonray))
        argument = confuse_text(argument);


    if (use_esp || !IS_OAFFECTED(ch, AFF_DEAFEN))
        emote_act( "$n $T", ch, NULL, argument, TO_CHAR );

    sprintf(log_buf, "O: %s %s\n\r", ch->name, argument );
    check_comm(ch, log_buf);

    for (vch = ch->in_room->people;vch != NULL;vch = vch->next_in_room)
        if ((use_esp || !IS_OAFFECTED(vch, AFF_DEAFEN)) && (ch != vch))
                emote_act( "$n $t", ch, argument, vch, TO_VICT );

    return;
}

void do_pmote( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    char *letter,*name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    unsigned int matches = 0;
    bool use_esp = is_affected(ch, gsn_esp);

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
    {
        send_to_char( "You can't show your emotions.\n\r", ch );
        return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Emote what?\n\r", ch );
        return;
    }

    if (is_affected(ch, gsn_moonray))
        argument = confuse_text(argument);

    act( "$n $t", ch, argument, NULL, TO_CHAR );

    sprintf(log_buf, "O: (pmote) %s %s\n\r", ch->name, argument );
    check_comm(ch, log_buf);

	if (ch->in_room != NULL)
	{
	    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    	{
	        if (vch->desc == NULL || vch == ch || !can_see(vch, ch))
        	    continue;

    	    if ((letter = strstr(argument,vch->name)) == NULL)
	        {
    	    	if (use_esp || !IS_OAFFECTED(vch, AFF_DEAFEN))
    		        act("$N $t",vch,argument,ch,TO_CHAR);
	            continue;
        	}

    	    strcpy(temp,argument);
	        temp[strlen(argument) - strlen(letter)] = '\0';
        	last[0] = '\0';
    	    name = vch->name;

	        for (; *letter != '\0'; letter++)
        	{
    	        if (*letter == '\'' && matches == strlen(vch->name))
	            {
            	    strcat(temp,"r");
        	        continue;
    	        }

	            if (*letter == 's' && matches == strlen(vch->name))
            	{
        	        matches = 0;
    	            continue;
	            }

            	if (matches == strlen(vch->name))
        	    {
    	            matches = 0;
	            }

        	    if (*letter == *name)
    	        {
	                matches++;
                	name++;
            	    if (matches == strlen(vch->name))
        	        {
    	                strcat(temp,"you");
	                    last[0] = '\0';
                    	name = vch->name;
                	    continue;
            	    }
        	        strncat(last,letter,1);
    	            continue;
	            }

            	matches = 0;
        	    strcat(temp,last);
    	        strncat(temp,letter,1);
	            last[0] = '\0';
            	name = vch->name;
        	}

    	    emote_act("$N $t",vch,temp,ch,TO_CHAR);
	    }
	}
}

void do_smote(CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    char *letter,*name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH], log_buf[MAX_STRING_LENGTH];
    unsigned int matches = 0;
 
    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
    {
        send_to_char( "You can't show your emotions.\n\r", ch );
        return;
    }
 
    if ( argument[0] == '\0' )
    {
        send_to_char( "Emote what?\n\r", ch );
        return;
    }
    
    if (strstr(argument,ch->name) == NULL)
    {
	send_to_char("You must include your name in an smote.\n\r",ch);
	return;
    }
   
    send_to_char(argument,ch);
    send_to_char("\n\r",ch);

    sprintf(log_buf, "O: (smote) %s\n\r", argument);
    check_comm(ch, log_buf);
 
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->desc == NULL || vch == ch)
            continue;
 
        if ((letter = strstr(argument,vch->name)) == NULL)
        {
	    send_to_char(argument,vch);
	    send_to_char("\n\r",vch);
            continue;
        }
 
        strcpy(temp,argument);
        temp[strlen(argument) - strlen(letter)] = '\0';
        last[0] = '\0';
        name = vch->name;
 
        for (; *letter != '\0'; letter++)
        {
            if (*letter == '\'' && matches == strlen(vch->name))
            {
                strcat(temp,"r");
                continue;
            }
 
            if (*letter == 's' && matches == strlen(vch->name))
            {
                matches = 0;
                continue;
            }
 
            if (matches == strlen(vch->name))
            {
                matches = 0;
            }
 
            if (*letter == *name)
            {
                matches++;
                name++;
                if (matches == strlen(vch->name))
                {
                    strcat(temp,"you");
                    last[0] = '\0';
                    name = vch->name;
                    continue;
                }
                strncat(last,letter,1);
                continue;
            }
 
            matches = 0;
            strcat(temp,last);
            strncat(temp,letter,1);
            last[0] = '\0';
            name = vch->name;
        }
 
	send_to_char(temp,vch);
	send_to_char("\n\r",vch);
	sprintf(log_buf, "I: (smote) %s\n\r", temp);
        check_comm(vch, log_buf);
    }
 
    return;
}


/*
 * All the posing stuff.
 */
struct  pose_table_type
{
    char *      message[2*MAX_CLASS];
};

const   struct  pose_table_type pose_table      []      =
{
    {
        {
            "You sizzle with energy.",
            "$n sizzles with energy.",
            "You feel very holy.",
            "$n looks very holy.",
            "You perform a small card trick.",
            "$n performs a small card trick.",
            "You show your bulging muscles.",
            "$n shows $s bulging muscles."
        }
    },

    {
        {
            "You turn into a butterfly, then return to your normal shape.",
            "$n turns into a butterfly, then returns to $s normal shape.",
            "You nonchalantly turn wine into water.",
            "$n nonchalantly turns wine into water.",
            "You wiggle your ears alternately.",
            "$n wiggles $s ears alternately.",
            "You crack nuts between your fingers.",
            "$n cracks nuts between $s fingers."
        }
    },

    {
        {
            "Blue sparks fly from your fingers.",
            "Blue sparks fly from $n's fingers.",
            "A halo appears over your head.",
            "A halo appears over $n's head.",
            "You nimbly tie yourself into a knot.",
            "$n nimbly ties $mself into a knot.",
            "You grizzle your teeth and look mean.",
            "$n grizzles $s teeth and looks mean."
        }
    },

    {
        {
            "Little red lights dance in your eyes.",
            "Little red lights dance in $n's eyes.",
            "You recite words of wisdom.",
            "$n recites words of wisdom.",
            "You juggle with daggers, apples, and eyeballs.",
            "$n juggles with daggers, apples, and eyeballs.",
            "You hit your head, and your eyes roll.",
            "$n hits $s head, and $s eyes roll."
        }
    },

    {
        {
            "A slimy green monster appears before you and bows.",
            "A slimy green monster appears before $n and bows.",
            "Deep in prayer, you levitate.",
            "Deep in prayer, $n levitates.",
            "You steal the underwear off every person in the room.",
            "Your underwear is gone!  $n stole it!",
            "Crunch, crunch -- you munch a bottle.",
            "Crunch, crunch -- $n munches a bottle."
        }
    },

    {
        {
            "You turn everybody into a little pink elephant.",
            "You are turned into a little pink elephant by $n.",
            "An angel consults you.",
            "An angel consults $n.",
            "The dice roll ... and you win again.",
            "The dice roll ... and $n wins again.",
            "... 98, 99, 100 ... you do pushups.",
            "... 98, 99, 100 ... $n does pushups."
        }
    },

    {
        {
            "A small ball of light dances on your fingertips.",
            "A small ball of light dances on $n's fingertips.",
            "Your body glows with an unearthly light.",
            "$n's body glows with an unearthly light.",
            "You count the money in everyone's pockets.",
            "Check your money, $n is counting it.",
            "Arnold Schwarzenegger admires your physique.",
            "Arnold Schwarzenegger admires $n's physique."
        }
    },

    {
        {
            "Smoke and fumes leak from your nostrils.",
            "Smoke and fumes leak from $n's nostrils.",
            "A spot light hits you.",
            "A spot light hits $n.",
            "You balance a pocket knife on your tongue.",
            "$n balances a pocket knife on your tongue.",
            "Watch your feet, you are juggling granite boulders.",
            "Watch your feet, $n is juggling granite boulders."
        }
    },

    {
        {
            "The light flickers as you rap in magical languages.",
            "The light flickers as $n raps in magical languages.",
            "Everyone levitates as you pray.",
            "You levitate as $n prays.",
            "You produce a coin from everyone's ear.",
            "$n produces a coin from your ear.",
            "Oomph!  You squeeze water out of a granite boulder.",
            "Oomph!  $n squeezes water out of a granite boulder."
        }
    },

    {
        {
            "Your head disappears.",
            "$n's head disappears.",
            "A cool breeze refreshes you.",
            "A cool breeze refreshes $n.",
            "You step behind your shadow.",
            "$n steps behind $s shadow.",
            "You pick your teeth with a spear.",
            "$n picks $s teeth with a spear."
        }
    },

    {
        {
            "A fire elemental singes your hair.",
            "A fire elemental singes $n's hair.",
            "The sun pierces through the clouds to illuminate you.",
            "The sun pierces through the clouds to illuminate $n.",
            "Your eyes dance with greed.",
            "$n's eyes dance with greed.",
            "Everyone is swept off their foot by your hug.",
            "You are swept off your feet by $n's hug."
        }
    },

    {
        {
            "The sky changes color to match your eyes.",
            "The sky changes color to match $n's eyes.",
            "The ocean parts before you.",
            "The ocean parts before $n.",
            "You deftly steal everyone's weapon.",
            "$n deftly steals your weapon.",
            "Your karate chop splits a tree.",
            "$n's karate chop splits a tree."
        }
    },

    {
        {
            "The stones dance to your command.",
            "The stones dance to $n's command.",
            "A thunder cloud kneels to you.",
            "A thunder cloud kneels to $n.",
            "The Grey Mouser buys you a beer.",
            "The Grey Mouser buys $n a beer.",
            "A strap of your armor breaks over your mighty thews.",
            "A strap of $n's armor breaks over $s mighty thews."
        }
    },

    {
        {
            "The heavens and grass change colour as you smile.",
            "The heavens and grass change colour as $n smiles.",
            "The Burning Man speaks to you.",
            "The Burning Man speaks to $n.",
            "Everyone's pocket explodes with your fireworks.",
            "Your pocket explodes with $n's fireworks.",
            "A boulder cracks at your frown.",
            "A boulder cracks at $n's frown."
        }
    },

    {
        {
            "Everyone's clothes are transparent, and you are laughing.",
            "Your clothes are transparent, and $n is laughing.",
            "An eye in a pyramid winks at you.",
            "An eye in a pyramid winks at $n.",
            "Everyone discovers your dagger a centimeter from their eye.",
            "You discover $n's dagger a centimeter from your eye.",
            "Mercenaries arrive to do your bidding.",
            "Mercenaries arrive to do $n's bidding."
        }
    },

    {
        {
            "A black hole swallows you.",
            "A black hole swallows $n.",
            "Valentine Michael Smith offers you a glass of water.",
            "Valentine Michael Smith offers $n a glass of water.",
            "Where did you go?",
            "Where did $n go?",
            "Four matched Percherons bring in your chariot.",
            "Four matched Percherons bring in $n's chariot."
        }
    },

    {
        {
            "The world shimmers in time with your whistling.",
            "The world shimmers in time with $n's whistling.",
            "The gods give you a staff.",
            "The gods give $n a staff.",
            "Click.",
            "Click.",
            "Atlas asks you to relieve him.",
            "Atlas asks $n to relieve him."
        }
    }
};

void do_pose( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    
    sprintf(buf,"You are currently ");
    switch(ch->position)
    {
        case POS_DEAD:
	case POS_MORTAL:
	case POS_INCAP:
	case POS_STUNNED: strcat(buf,"near death.\n\r"); break;
	case POS_SLEEPING: strcat(buf,"sleeping.\n\r"); break;
	case POS_RESTING: strcat(buf,"resting.\n\r"); break;
	case POS_SITTING: strcat(buf,"sitting.\n\r"); break;
	case POS_FIGHTING:
	case POS_STANDING: if(is_flying(ch)) strcat(buf,"flying.\n\r");
	  		   else strcat(buf,"standing.\n\r");
			   break;
    }
    send_to_char(buf,ch);

    if(strlen(argument) == 0)
    {
        if(ch->pose == NULL)
            send_to_char("You have no pose set.\n\r",ch);
        else
        {
            sprintf(buf,"Your pose is set to: %s\n\r",ch->pose);
            send_to_char(buf,ch);
        }
        return;
    }
    if (!strcmp(argument,"clear"))
    {
        free_string(ch->pose);
        send_to_char("Pose cleared.\n\r",ch);
        return;
    }

    copy_string(ch->pose, argument);
    sprintf(buf,"Your pose has been set to: %s\n\r",argument);
    send_to_char(buf,ch);
}

void do_bug( CHAR_DATA *ch, char *argument )
{
    append_file( ch, BUG_FILE, argument );
    NOTE_DATA *pnote;
    char *notetime;
    char buf[MAX_STRING_LENGTH];

    if (!argument || argument[0] == '\0')
    {
	send_to_char("Syntax: bugs <text describing the bug-like behavior>\n\r",ch);
	return;
    }
    if (!ch->in_room)
    {
	send_to_char("You must be in a room to log a bug.\n\r",ch);
	return;
    }
    pnote = new_note();
    pnote->sender = ch->desc->original 
			? str_dup(ch->desc->original->name) : str_dup(ch->name);
    pnote->to_list = str_dup("bugs immortal");
    sprintf(buf,"Bug: Room %d",ch->in_room->vnum);
    pnote->subject = str_dup(buf);
    sprintf(buf,"%s\n\r",argument);
    pnote->text = str_dup(buf);
    notetime = ctime(&current_time);
    notetime[strlen(notetime)-1] = '\0';
    pnote->date = str_dup(notetime);
    pnote->date_stamp = current_time;
    append_note(pnote);
    sprintf(buf,"Bugs has a new note waiting.");
    wiznet(buf,NULL,NULL,WIZ_BUGS, 0, 0);
    log_string(buf);

    send_to_char( "Bug logged.\n\r", ch );
    return;
}

void do_typo( CHAR_DATA *ch, char *argument )
{
    append_file( ch, TYPO_FILE, argument );
    NOTE_DATA *pnote;
    char *notetime;
    char buf[MAX_STRING_LENGTH];
    if (!argument || argument[0] == '\0')
    {
	send_to_char("Syntax: typo <text describing the typo>\n\r",ch);
	return;
    }
    if (!ch->in_room)
    {
	send_to_char("You must be in a room to log a typo.\n\r",ch);
	return;
    }
    pnote = new_note();
    pnote->sender = ch->desc->original 
			? str_dup(ch->desc->original->name) : str_dup(ch->name);
    pnote->to_list = str_dup("typo");
    sprintf(buf,"Typo: Room %d",ch->in_room->vnum);
    pnote->subject = str_dup(buf);
    sprintf(buf,"%s\n\r",argument);
    pnote->text = str_dup(buf);
    notetime = ctime(&current_time);
    notetime[strlen(notetime)-1] = '\0';
    pnote->date = str_dup(notetime);
    pnote->date_stamp = current_time;
    append_note(pnote);
    sprintf(buf,"Typo has a new note waiting.");
    wiznet(buf,NULL,NULL,WIZ_BUGS, 0, 0);
    log_string(buf);
    send_to_char( "Typo logged.\n\r", ch );
    return;
}

void do_rent( CHAR_DATA *ch, char *argument )
{
    send_to_char( "There is no rent here.  Just save and quit.\n\r", ch );
    return;
}

void do_qui( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to QUIT, you have to spell it out.\n\r", ch );
    return;
}

void do_quit( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *ich;
//    DESCRIPTOR_DATA *d;
    OBJ_DATA *obj, *obj_next;
   /* DESCRIPTOR_DATA *d_next; */
    AFFECT_DATA af, *paf;
af.valid = TRUE;
af.point = NULL;
    int id;

    if ( IS_NPC(ch) )
        return;

    if (ch->pcdata->adrenaline > 0 && !IS_SET(ch->act, PLR_DENY))
    {
	send_to_char("Your adrenaline is gushing! Not now!\n\r",ch);
        return;
    }

    if (IS_SET(ch->act, PLR_DENY) && ch->position == POS_FIGHTING)
        stop_fighting_all(ch);

    if ( ch->position == POS_FIGHTING )
    {
        send_to_char( "No way! You are fighting.\n\r", ch );
        return;
    }
    
    if (ch->in_room)
        if (ch->in_room->clan != 0 && ch->in_room->clan != ch->clan &&
          !IS_SET(ch->act, PLR_DENY))
        {
            send_to_char( "You are in too hostile a place to quit.\n\r", ch);
            return;
        }

   if (rip_process && !IS_SET(ch->act, PLR_DENY))
	{
	  send_to_char("You cannot quit right now. Try again in a minute.\n\r", ch);
	  return;
	}

    if (ch->in_room
     && !IS_SET(ch->in_room->room_flags, ROOM_GUILD)
     && !IS_SET(ch->in_room->room_flags, ROOM_LAW)
     && ch->in_room->clan == 0
     && ch->trust < 52
     && !IS_SET(ch->act, PLR_DENY)
     && !(((paf = affect_find(ch->in_room->affected,gsn_encamp)) != NULL)
	&& paf->modifier == ch->id))
    {
	send_to_char( "This place is too uncivilized for that.\n\r", ch);
        return;
    }

    if ( ch->position  < POS_STUNNED  && !IS_SET(ch->act, PLR_DENY))
    {
        send_to_char( "You're not DEAD yet.\n\r", ch );
        return;
    }

    send_to_char("Alas, all good things must come to an end.\n\r",ch);

    if (ch->in_room)
        for ( ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room )
        {
            if (!((!can_see(ich,ch)) && (IS_IMMORTAL(ch))))
		act_new("$n leaves the land.",ch,ch,ich,TO_VICT,POS_RESTING);
	}

    if (ch->desc)
	sprintf( log_buf, "%s@%s has quit.", ch->name, ch->desc->host);
    else
        sprintf( log_buf,"%s has quit.", ch->name );

    log_string( log_buf );

    sprintf(log_buf, "$N@%s rejoins the real world. [Room %d]",
	ch->desc ? ch->desc->host : "unknown",
	ch->in_room ? ch->in_room->vnum : 0 );
    wiznet(log_buf,ch,NULL,WIZ_LOGINS,0,get_trust(ch));

    if (ch->familiar && ch->familiar->in_room && ch->in_room && ch->in_room != ch->familiar->in_room)
    {
        extract_char(ch->familiar, TRUE);
        affect_strip(ch, gsn_findfamiliar);
    }
    affect_strip(ch, gsn_finditems);

    af.where     = TO_AFFECTS;
    af.type      = gsn_curseofthevoid;
    af.level     = 60;
    af.duration  = -1;
    af.modifier  = 9999;
    af.location  = APPLY_HIDE;
    af.bitvector = 0;
	af.point = NULL;

    /* no escape from the wraith that easy */
    for (ich = char_list;ich != NULL;ich = ich->next)
        if (ich->memfocus[0] == ch 
	 && IS_NPC(ich) 
	 && ich->pIndexData->vnum == MOB_VNUM_ASHUR_EVIL)
	    affect_to_char(ch, &af);

    if (is_affected(ch, gsn_disguise))
        affect_strip(ch, gsn_disguise);

    if (is_affected(ch, gsn_setambush))
        affect_strip(ch,gsn_setambush);

    struct BondStripper
    {
        static void Strip(CHAR_DATA * ch, int sn)
        {
            if (!is_affected(ch, sn))
                return;

            int mod(get_high_modifier(ch->affected, sn));
	        CHAR_DATA *vch, *vch_next;
            for (vch = char_list; vch != NULL; vch = vch_next)
            {
                vch_next = vch->next;
                if (vch->id == mod)
                {
                    send_to_char("You feel your spirit bond fade away.\n\r", vch);
                    affect_strip(vch, sn);
                }
            }

            affect_strip(ch, sn);
        }
    };

    BondStripper::Strip(ch, gsn_spiritbond);
    BondStripper::Strip(ch, gsn_bondofsouls);

// No keeping house powers through quit (except soul reaver, criminal status).
    if (is_affected(ch, gsn_runeofeyes))
	affect_strip(ch, gsn_runeofeyes);

    if (is_affected(ch, gsn_hooded))
        affect_strip(ch, gsn_hooded);

    if (is_affected(ch, gsn_perception))
        affect_strip(ch, gsn_perception);

    if (is_affected(ch, gsn_aegisoflaw))
        affect_strip(ch, gsn_aegisoflaw);

    if (ch->pet != NULL && ch->pet->pIndexData->vnum == MOB_VNUM_BIRD_PREY)
        extract_char(ch->pet,TRUE);

    if (is_affected(ch, gsn_coverofdarkness)
      && IS_PAFFECTED(ch, AFF_SHROUD_OF_NYOGTHUA))
        affect_strip(ch, gsn_coverofdarkness);

    if (is_affected(ch, gsn_mantleoffear))
        affect_strip(ch, gsn_mantleoffear);

    if (is_affected(ch, gsn_dark_insight))
        affect_strip(ch, gsn_dark_insight);

    if (is_affected(ch, gsn_demonic_might))
        affect_strip(ch, gsn_demonic_might);

    if (is_affected(ch, gsn_demonic_focus))
        affect_strip(ch, gsn_demonic_focus);

    if (is_affected(ch, gsn_compact_of_Logor))
        affect_strip(ch, gsn_compact_of_Logor);

    if (is_affected(ch, gsn_aura_of_corruption)
      && IS_PAFFECTED(ch, AFF_AURA_OF_CORRUPTION))
        affect_strip(ch, gsn_aura_of_corruption);

    for ( obj = ch->carrying; obj != NULL; obj = obj->next )
        if (obj_is_affected(obj,gsn_holyavenger))
	    object_affect_strip(obj, gsn_holyavenger);

    /*
     * After extract_char the ch is no longer valid!
     */
    save_char_obj( ch );

    /*
     * This is what is known as a cheap-ass hack.
     * To fix the verb_prog/sigil/quit bug, because sigils aren't
     * removed by extract_char.  This is such a hack, I'm ashamed.
     *                                       -- Erinos
     *
     * Note: This code is also found in char_equip_crumbling().
     *
     */
    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
        obj_next = obj->next_content;
        if (IS_SET(obj->wear_flags, ITEM_WEAR_SIGIL) || IS_SET(obj->wear_flags, ITEM_PROG))
           extract_obj( obj );
    }

    id = ch->id;
    extract_char( ch, TRUE );
}

void do_save( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) )
        return;

    if (rip_process)
    {
      send_to_char("You can't save right now. Try again in a minute.\n\r", ch);
      return;
    }

    save_char_obj( ch );
    send_to_char("Saving. Remember that Avendar will save you automatically.\n\r", ch);
    return;
}

void do_befriend(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim;
    int skill;

    if ((skill = get_skill(ch,gsn_befriend)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (IS_OAFFECTED(ch, AFF_GHOST))
    {
        send_to_char("You can't do that as a ghost.\n\r",ch);
	return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
        {
        send_to_char("You don't see any such animal here.\n\r", ch);
        return;
        }

    if (IS_AFFECTED(victim, AFF_CHARM))

        {
        send_to_char("They're already following another.\n\r", ch);
        return;
        }

    if (!IS_NPC(victim) || !IS_SET(victim->act,ACT_ANIMAL))
        {
        send_to_char("You can only befriend animals!\n\r", ch);
        return;
        }

    if (!IS_AWAKE(victim))
        {
        send_to_char("You can't really befriend a sleeping animal.\n\r", ch);
        return;
        }

    if (!can_charm_another(ch))
        {
        send_to_char("You can't control any more followers!\n\r", ch);
        multi_hit(victim, ch, TYPE_UNDEFINED);
        return;
        }

    if (ch->mana < skill_table[gsn_befriend].min_mana)
        {
        send_to_char("You can't muster the energy to befriend an animal now.\n\r", ch);
        return;
        }

    if ( IS_AFFECTED(victim, AFF_CHARM)
    ||   IS_AFFECTED(ch, AFF_CHARM)
    ||   ch->level < victim->level
    ||   IS_SET(victim->imm_flags,IMM_CHARM) 
    ||   saves_spell(ch->level, ch, victim,DAM_CHARM))
        {
// brazen: Ticket #46: Befriend should echo something when you autofail
        act("You try to befriend $N, but $N becomes enraged and attacks!", ch, NULL, victim, TO_CHAR);
        act("$n tries to befriend $N, but $N becomes enraged and attacks!", ch, NULL, victim, TO_ROOM);

	multi_hit(victim, ch, TYPE_UNDEFINED);
        return;
        }

        if (number_percent() > skill)
        {
        act("You try to befriend $N, but $N becomes enraged and attacks!", ch, NULL, victim, TO_CHAR);
        act("$n tries to befriend $N, but $N becomes enraged and attacks!", ch, NULL, victim, TO_ROOM);
        check_improve(ch,victim,gsn_befriend, FALSE, 1);
        multi_hit(victim, ch, TYPE_UNDEFINED);
        expend_mana(ch, skill_table[gsn_befriend].min_mana/2);
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_befriend].beats));
        return;
        }

    if ( victim->master )
        stop_follower( victim );
    add_follower( victim, ch );
    set_leader(victim, ch);
    af.where     = TO_AFFECTS;
    af.type      = gsn_befriend;
    af.level     = ch->level;
    af.duration  = number_fuzzy( ch->level / 2 );
	af.point = NULL;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char( victim, &af );
    act( "Isn't $n just so nice?", ch, NULL, victim, TO_VICT );
    act("You befriend $N, using incredible empathy to lure $M to your aid.", ch, NULL, victim, TO_CHAR);
    act("$n befriends $N, using incredible empathy to lure $M to $s aid.", ch, NULL, victim, TO_ROOM);
    expend_mana(ch, skill_table[gsn_befriend].min_mana);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_befriend].beats));

}

void do_follow( CHAR_DATA *ch, char *argument )
{
/* RT changed to allow unlimited following and follow the NOFOLLOW rules */
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Follow whom?\n\r", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL )
    {
        act( "But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR );
        return;
    }

    if ( victim == ch )
    {
        if ( ch->master == NULL )
        {
            send_to_char( "You already follow yourself.\n\r", ch );
            return;
        }
        stop_follower(ch);
        return;
    }

    if (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOFOLLOW) && !IS_IMMORTAL(ch))
    {
        act("$N doesn't seem to want any followers.\n\r",
             ch,NULL,victim, TO_CHAR);
        return;
    }

    if (IS_NPC(victim) && IS_SET(victim->off_flags, ASSIST_GUARD) && !IS_IMMORTAL(ch) && can_see(victim, ch))
    {
	act("$n waves you off as you try to follow $m on $s patrol.", victim, NULL, ch, TO_VICT);
	return;
    }

    if (is_affected(ch, gsn_discord))
    {
	act("You feel too distrusting to follow $N.", ch, NULL, victim, TO_CHAR);
	return;
    }

    REMOVE_BIT(ch->act,PLR_NOFOLLOW);

    if ( ch->master != NULL )
        stop_follower( ch );

    add_follower( ch, victim );
}

void add_follower( CHAR_DATA *ch, CHAR_DATA *master )
{
    if ( ch->master != NULL )
    {
        bug( "Add_follower: non-null master.", 0 );
        return;
    }

    ch->master        = master;
    set_leader(ch, NULL);

    if ( can_see( master, ch ) )
        act( "$n now follows you.", ch, NULL, master, TO_VICT );

    act( "You now follow $N.",  ch, NULL, master, TO_CHAR );
}

void set_leader(CHAR_DATA * ch, CHAR_DATA * leader)
{
    // Get the aethereal communion state of everyone in the room
    unsigned int dummy;
    std::set<CHAR_DATA*> preCommunion;
    if (ch->in_room != NULL)
    {
        for (CHAR_DATA * gch(ch->in_room->people); gch != NULL; gch = gch->next_in_room)
        {
            if (MembersOfAetherealCommunion(gch, dummy).size() > 1)
                preCommunion.insert(gch);
        }
    }

    // Set the leader
    ch->leader = leader;

    // Now check the new aethereal communion state against the old one
    if (ch->in_room != NULL)
    {
        for (CHAR_DATA * gch(ch->in_room->people); gch != NULL; gch = gch->next_in_room)
        {
            if (MembersOfAetherealCommunion(gch, dummy).size() > 1)
            {
                // Currently in communion; if previously not, echo
                if (preCommunion.find(gch) == preCommunion.end())
                    send_to_char("A wave of joy washes over you as your spirit links to a greater Whole.\n", gch);
            }
            else
            {
                // Currently not in communion; if previously in communion, echo
                if (preCommunion.find(gch) != preCommunion.end())
                    send_to_char("You feel a brief sense of loss as your spirit unlinks from the aethereal communion.\n", gch);
            }
        }
    }
}

void stop_follower(CHAR_DATA *ch, bool fromExtract)
{

    if (!ch->master)
    {
        bug( "Stop_follower: null master.", 0 );
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM))
    {
        REMOVE_BIT(ch->affected_by, AFF_CHARM);
        affect_strip(ch, gsn_charm_person);
        affect_strip(ch, gsn_befriend);
    	affect_strip(ch, gsn_dominance);
	    affect_strip(ch, gsn_enslave);
    	affect_strip(ch, gsn_demoniccontrol);
    }

    if (can_see(ch->master, ch) && ch->in_room != NULL)
    {
        act( "$n stops following you.",     ch, NULL, ch->master, TO_VICT    );
        act( "You stop following $N.",      ch, NULL, ch->master, TO_CHAR    );
    }

    if ((ch->master->familiar == ch))
    {
        affect_strip(ch->master, gsn_findfamiliar);

	if (ch->master->in_room)
            send_to_char("The agony of the loss of your familiar burns within you.\n\r", ch->master);

        AFFECT_DATA af = {0};
        af.where     = TO_AFFECTS;
        af.type      = gsn_findfamiliar;
        af.level     = ch->master->level;
        af.duration  = -1;
        af.location  = APPLY_HIT;
        af.modifier  = -4 * ch->master->level;
        affect_to_char(ch->master, &af);

        af.location  = APPLY_MANA;
        af.modifier  = -3 * ch->master->level;
        affect_to_char(ch->master, &af);

        ch->master->familiar = NULL;
    }

    if (ch->master->pet == ch)
        ch->master->pet = NULL;

    ch->master = NULL;
    set_leader(ch, NULL);

    if (IS_NPC(ch) && IS_SET(ch->act, ACT_ILLUSION))
    {
    	act("$n shimmers and fades out of existence.", ch, NULL, NULL, TO_ROOM);
        if (!fromExtract)
            extract_char(ch, TRUE);
    }
}

/* nukes charmed monsters and pets */
static void nuke_pet(CHAR_DATA & pet)
{
    stop_follower(&pet);
    if (pet.in_room != NULL)
        act("$n slowly fades away.", &pet, NULL, NULL, TO_ROOM);

    extract_char(&pet, true);
}

void nuke_pets( CHAR_DATA *ch )
{
    if (ch->pet != NULL) nuke_pet(*ch->pet);
    if (ch->familiar != NULL) nuke_pet(*ch->familiar);
    ch->pet = NULL;
    ch->familiar = NULL;

    // Wipe out any other pets in the room
    if (ch->in_room != NULL)
    {
        CHAR_DATA * pet_next;
        for (CHAR_DATA * pet(ch->in_room->people); pet != NULL; pet = pet_next)
        {
            pet_next = pet->next_in_room;
            if (IS_NPC(pet) && pet->leader == ch && pet->master == ch)
                nuke_pet(*pet);
        }
    }
}

void die_follower(CHAR_DATA *ch, bool fromExtract, bool retainCharmed)
{
    CHAR_DATA *fch, *fch_next;

    if ( ch->master != NULL )
    {
        if (ch->master->pet == ch)
            ch->master->pet = NULL;
        stop_follower( ch, fromExtract );
    }

    set_leader(ch, NULL);

    for ( fch = char_list; fch != NULL; fch = fch_next )
    {
        fch_next = fch->next;
        if (retainCharmed && fch->master == ch && IS_NPC(fch) && IS_AFFECTED(fch, AFF_CHARM))
            continue;

        if (fch->master == ch && (fch->master->familiar != fch || fch->fighting == fch->master))
            stop_follower( fch );
        
        if ( fch->leader == ch && (fch->master->familiar != fch || fch->fighting == fch->master))
            set_leader(fch, fch);
    }
}

static void release_follower(CHAR_DATA *ch, CHAR_DATA * victim)
{
    // Handle illusions
    if (IS_NPC(victim) && IS_SET(victim->act, ACT_ILLUSION))
    {
        act("$N vanishes as $n's hold on it slips away.", ch, NULL, victim,TO_ROOM);
        act("You let slip your control of $N, and it fades slowly away.",ch,NULL, victim, TO_CHAR);
        die_follower(victim);
        return;
    }

    // General echoes
    act("$n releases $N from service.", ch, NULL, victim, TO_NOTVICT);
    act("$n releases you from service.", ch, NULL, victim, TO_VICT);
    act("You release $N from service.", ch, NULL, victim, TO_CHAR);

    // Handle demons
    if (IS_NPC(victim) && is_affected(victim, gsn_demoniccontrol))
    {
        act("You feel your control over $N fade.", ch, NULL, victim, TO_CHAR);

        stop_follower(victim);
        victim->tracking = ch;
        victim->demontrack = ch;

        // Demons attack if released
        if (victim->in_room == ch->in_room)
        {
            if (victim->position != POS_FIGHTING)
                set_fighting(victim, ch);
            else
                victim->fighting = ch;

            act("$n screams in fury and attacks!", victim, NULL, NULL, TO_ROOM);
            multi_hit(victim, ch, TYPE_UNDEFINED);
        }
        return;
    }

    // Bail out if attacking
    if (victim->tracking == ch)
        return;

    // Handle NPCs wandering home
    if (IS_NPC(victim))
    {
        act("$n wanders home.", victim, NULL, NULL, TO_ROOM);
        extract_char(victim, TRUE);
    }
}

void do_release(CHAR_DATA *ch, char *argument)
{
    if ( argument[0] == '\0')
    {
        send_to_char( "Order whom to do what?\n\r", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_CHARM ) )
    {
        send_to_char( "You feel like taking, not giving, orders.\n\r", ch );
        return;
    }

    // Check for 'all' argument
    if (!str_prefix(argument, "all"))
    {
        // Iterate the char list, looking for all pets of ch
        std::vector<CHAR_DATA*> pets;
        for (CHAR_DATA * victim(char_list); victim != NULL; victim = victim->next)
        {
            if (IS_AFFECTED(victim, AFF_CHARM) && victim->master == ch)
                pets.push_back(victim);
        }

        // Now iterate the pets, releasing each one in turn
        for (size_t i(0); i < pets.size(); ++i)
            release_follower(ch, pets[i]);

        return;
    }
    
    // Check for specific victim
    CHAR_DATA * victim(get_char_room(ch, argument));
    if (victim == NULL)
    {
        send_to_char("You don't see them here.\n\r", ch);
        return;
    }

    if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch)
    {
        send_to_char("They won't listen to you.\n\r", ch);
        return;
    }

    release_follower(ch, victim);
}

void do_command(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    AFFECT_DATA *spirit;

    argument = one_argument(argument, arg);
    one_argument(argument, arg2);

    if ((ch->class_num != global_int_class_voidscholar)
     || (arg[0] == '\0'))
    {
	do_commands(ch, "");
	return;
    }

    if (ch->position < POS_STANDING)
    {
	send_to_char("You must be standing to issue commands.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Command them to do what?\n\r", ch);
	return;
    }

    if (!str_prefix(arg2, "mp"))
	return;

    if ((victim = get_char_room(ch, arg)) == NULL)
    {
	send_to_char("You do not see them here.\n\r", ch);
	return;
    }

	if (IsOrderBanned(arg2))
    {
        send_to_char("That will NOT be done.\n\r",ch);
        return;
    }

    if (IS_IMM_TRUST(victim)
     || ((spirit = affect_find(ch->affected, gsn_uncleanspirit)) == NULL)
     || (spirit->modifier != (ch->id * -1)))
    {
	act("$E is not yours to command.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (victim->fighting == ch)
    {
	act("You cannot command $M now!", ch, NULL, victim, TO_CHAR);
	return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE);
    act("You command $N to '$x'.", ch, argument, victim, TO_CHAR);
    act("$n commands $N to '$x'.", ch, argument, victim, TO_NOTVICT);
    act("$n commands you to '$x'.", ch, argument, victim, TO_VICT);

    if (saves_spell(ch->level, ch, victim, DAM_MENTAL))
    {
	act("$N resists your commands!", ch, NULL, victim, TO_CHAR);
	act("You resist $s command!", ch, NULL, victim, TO_VICT);
        if (can_see(victim, ch))
        {
	    sprintf(buf, "I will not obey you, %s!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	}
        check_killer(ch, victim);
	multi_hit(victim, ch, TYPE_UNDEFINED);
	return;
    }

    interpret(victim, argument);
}
       
void do_order( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *och;
    CHAR_DATA *och_next;
    bool found;
    bool fAll;

    smash_dollar(argument);
    argument = one_argument(argument, arg );
    one_argument(argument,arg2);

    if (IsOrderBanned(arg2))
    {
        send_to_char("That will NOT be done.\n\r",ch);
        return;
    }

    if (IS_OAFFECTED(ch, AFF_GHOST))
    {
	send_to_char("They will not answer your ghostly commands.\n\r", ch);
	return;
    }

    if (IS_PAFFECTED(ch, AFF_VOIDWALK))
    {
	send_to_char("You cannot order others in your current state.\n\r", ch);
	return;
    }

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
        send_to_char( "Order whom to do what?\n\r", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_CHARM ) )
    {
        send_to_char( "You feel like taking, not giving, orders.\n\r", ch );
        return;
    }

    if (arg2[0] == 'm' && arg2[1] == 'p')
        return;

    if ( !str_cmp( arg, "all" ) )
    {
        fAll   = TRUE;
        victim = NULL;
    }
    else
    {
        fAll   = FALSE;
        if ( ( victim = get_char_room( ch, arg ) ) == NULL )
        {
            send_to_char( "They aren't here.\n\r", ch );
            return;
        }

        if ( victim == ch )
        {
            send_to_char( "Aye aye, right away!\n\r", ch );
            return;
        }

        if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch
        ||  (IS_IMMORTAL(victim) && victim->trust >= ch->trust))
        {
            send_to_char( "Do it yourself!\n\r", ch );
            return;
        }
    }

    found = FALSE;
    for ( och = ch->in_room->people; och != NULL; och = och_next )
    {
        och_next = och->next_in_room;

        if ( IS_AFFECTED(och, AFF_CHARM)
        &&   och->master == ch
        && ( fAll || och == victim ) )
        {
            found = TRUE;
            sprintf( buf, "$n orders you to '%s'.", argument );
            act( buf, ch, NULL, och, TO_VICT );
	    if (is_affected(och, gsn_dominance) && number_bits(1) == 0)
	    {
		send_to_char("You resist the order.\n\r", och);
		act("$N resists your order.", ch, NULL, och, TO_CHAR);
		if (number_bits(3) == 0)
		{
		    int i;

		    act("You break yourself away from $n's dominating presence!", ch, NULL, och, TO_VICT);
		    act("$N breaks free of your dominance!", ch, NULL, och, TO_CHAR);
		    if (och->master)
			stop_follower(och);
		    for (i = 0; i < MAX_FOCUS; i++)
			if (ch->pcdata->focus_on[i] == (ch->pcdata->focus_sn[i] == gsn_dominance) && (ch->pcdata->focus_ch[i] == och))
			    unfocus(ch, i, TRUE);
		}
	    }
	    else
                interpret( och, argument );
        }
    }

    if ( found )
    {
        WAIT_STATE(ch,PULSE_VIOLENCE);
        send_to_char( "Ok.\n\r", ch );
    }
    else
        send_to_char( "You have no followers here.\n\r", ch );
    return;
}

void show_group_listing(CHAR_DATA * ch, CHAR_DATA * viewer)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *gch;
    CHAR_DATA *leader;

    leader = (ch->leader != NULL) ? ch->leader : ch;
    sprintf( buf, "%s's group:\n\r", CPERS(leader, ch) );
    send_to_char(buf, viewer);

    for ( gch = char_list; gch != NULL; gch = gch->next )
    {
        if ( is_same_group( gch, ch ) )
        {
            // Choose the prefix
            const char * prefix("Mob");
            if (IS_NPC(gch))
            {
                // Check for bilocation
                CHAR_DATA * original(find_bilocated_body(gch));
                if (original != NULL)
                    prefix = class_table[original->class_num].who_name;
            }
            else
                prefix = class_table[gch->class_num].who_name;

            sprintf( buf,
            "[%2d %s] %-16s %4d/%4d hp %4d/%4d mana %4d/%4d mv \n\r",
                gch->level,
                prefix,
                capitalize( CPERS(gch, ch) ),
                (IS_NAFFECTED(gch, AFF_ASHURMADNESS) || IS_AFFECTED(gch, AFF_BERSERK) || is_loc_not_affected(gch, gsn_anesthetize, APPLY_NONE)) ? 0 : gch->hit,   gch->max_hit,
                gch->mana,  gch->max_mana,
                gch->move,  gch->max_move );
            send_to_char(buf, viewer);
        }
    }
 
}

void do_group( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        show_group_listing(ch, ch);
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( ch->master != NULL || ( ch->leader != NULL && ch->leader != ch ) )
    {
        send_to_char( "But you are following someone else!\n\r", ch );
        return;
    }

    if ( victim->master != ch && ch != victim )
    {
        act_new("$N isn't following you.",ch,NULL,victim,TO_CHAR,POS_SLEEPING);
        return;
    }

    if (IS_AFFECTED(victim,AFF_CHARM))
    {
        send_to_char("You can't remove charmed mobs from your group.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM))
    {
        act_new("You like your master too much to leave $m!",
            ch,NULL,victim,TO_VICT,POS_SLEEPING);
        return;
    }

    if ( is_same_group( victim, ch ) && ch != victim )
    {
        // Already in the same group, so remove the target
        act_new("$n removes $N from $s group.", ch,NULL,victim,TO_NOTVICT,POS_RESTING);
        act_new("$n removes you from $s group.", ch,NULL,victim,TO_VICT,POS_SLEEPING);
        act_new("You remove $N from your group.", ch,NULL,victim,TO_CHAR,POS_SLEEPING);
        set_leader(victim, NULL);
        return;
    }
    
    // Not already in the same group, so add the target
    act_new("$N joins $n's group.",ch,NULL,victim,TO_NOTVICT,POS_RESTING);
    act_new("You join $n's group.",ch,NULL,victim,TO_VICT,POS_SLEEPING);
    act_new("$N joins your group.",ch,NULL,victim,TO_CHAR,POS_SLEEPING);
    set_leader(victim, ch);
}

void do_intimidate( CHAR_DATA *ch, char *argument )
{
        char buf[MAX_STRING_LENGTH];
        CHAR_DATA *victim;
        OBJ_DATA *obj;
        char arg1[MAX_INPUT_LENGTH];

        if (!stone_check(ch, gsn_intimidate))
        {
        send_to_char("You cannot draw enough power from the stones.\n\r", ch);
        return;
        }

        if (argument[0] == '\0')
        {
        send_to_char("Intimidate what away from whom?\n\r", ch);
        return;
        }

        argument = one_argument(argument, arg1);

        if (argument[0] == '\0')
        {
        send_to_char("Intimidate what away from whom?\n\r", ch);
        return;
        }

        if ((victim = get_char_room(ch, argument)) == NULL)
        {
        send_to_char("You don't see them here.\n\r", ch);
        return;
        }

        if (IS_OAFFECTED(ch, AFF_GHOST))
        {
        send_to_char("You can't intimidate someone while you're a ghost!\n\r", ch);
        return;
        }

        if (!IS_NPC(victim) || !IS_AWAKE(victim))
        {
        return;
        }

        if ((obj = get_obj_list(victim, arg1, victim->carrying))==NULL)
        {
        act("$N doesn't seem to know what you want.", ch, NULL, victim, TO_CHAR);
        return;
        }

        if (!can_see_obj(ch, obj))
        {
        send_to_char("You can't see any such thing.\n\r", ch);
        return;
        }

        if (IS_SET(obj->extra_flags[0], ITEM_NODROP))
        {
        act("$N can't seem to let go of it.", ch, NULL, victim, TO_CHAR);
        return;
        }

	if (IS_SET(obj->extra_flags[0], ITEM_NOREMOVE) && obj->worn_on)
	{
	    act("$N can't seem to take it off.", ch, NULL, victim, TO_CHAR);
	    return;
	}

        if (IS_SET(obj->extra_flags[0], ITEM_INVENTORY))
        {
          act("$N won't be intimidated, and threatens to call the Merchant Council's wrath on you.", ch, NULL, victim, TO_CHAR);
          return;
        }

        act("You intimidate $N, trying to get $p away.", ch, obj, victim, TO_CHAR);
        act("$n intimidates $N, trying to get $p away.", ch, obj, victim, TO_ROOM);

        if (saves_spell(ch->level + 3, ch, victim,  DAM_CHARM))
        {
        act("$N is angered by $n's intimidation tactics!", ch, NULL, victim, TO_ROOM);
        act("$N is angered by your intimidation tactics!", ch, NULL, victim, TO_CHAR);
        multi_hit(victim, ch, TYPE_UNDEFINED);
        return;
        }

	if (obj->worn_on)
        {
            unequip_char(victim, obj);
	    oprog_remove_trigger(obj);
            sprintf(buf, "'%s' %s", obj->name, ch->name);
            do_give(victim, buf);
            WAIT_STATE(ch, 3*PULSE_VIOLENCE);
        }
        else
        {
            sprintf(buf, "'%s' %s", obj->name, ch->name);
            do_give(victim, buf);
            WAIT_STATE(ch, 3*PULSE_VIOLENCE);
        }
}


void do_request( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    char arg1[MAX_INPUT_LENGTH];
    int kreq, cost;

    if (is_affected(ch, gsn_request))
    {
        send_to_char("You're still thinking carefully about your last request.\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        send_to_char("Request what from whom?\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg1);

    if (argument[0] == '\0')
    {
        send_to_char("Request what from whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
    {
        send_to_char("You don't see them here.\n\r", ch);
        return;
    }

    if (!IS_NPC(victim) || !IS_AWAKE(victim) || IS_NPC(ch))
        return;

    if (ch->alignment < 700)
    {
        act("$N looks indifferent to you, and ignores your request.", ch, NULL, victim, TO_CHAR);
        WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
        return;
    }

    if (victim->alignment < 700)
    {
        act("$N looks indifferent to you, and ignores your request.", ch, NULL, victim, TO_CHAR);
        WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
        return;
    }

    if ( IS_NPC(victim) && victim->pIndexData->pShop != NULL )
    {
        do_say(victim, "If I give away my things, I will be bankrupt!");
        return;
    }

    if ((obj = get_obj_list(victim, arg1, victim->carrying))==NULL)
    {
        act("$N looks confused by that request.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (!can_see_obj(ch, obj))
    {
        send_to_char("You can't see any such thing.\n\r", ch);
        return;
    }

    if (IS_SET(obj->extra_flags[0], ITEM_NODROP))
    {
        act("$N can't seem to let go of it.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (IS_SET(obj->extra_flags[0], ITEM_NOREMOVE) && obj->worn_on)
    {
        act("$N can't seem to take it off.", ch, NULL, victim, TO_CHAR);
        return;
    }
	
    if (!can_see(victim, ch)) 
    {
        act("$N can't see you to give you $p.", ch, obj, victim, TO_CHAR);
        return;
    }

    kreq = UMAX(25,victim->level + obj->level - ch->level);
    cost = kreq * 6;
    cost -= (ch->pcdata->faction_standing->Value(*ch, victim->pIndexData->factionNumber) * 100) / FactionStanding::Maximum;

    if (((victim->level - ch->level) > 9) 
      || ((ch->pcdata->request_points / 50) > -kreq) 
      || ((ch->pcdata->request_points + cost) > PALEGOLDENAURA))
    {
	act("You are not worthy enough to receive $p from $N.", ch, obj, victim, TO_CHAR);
	act("$n is not worthy enough to receive $p from $N.", ch, obj, victim, TO_ROOM);
	return;
    }
	
    act("You request $p from $N.", ch, obj, victim, TO_CHAR);
    act("$n requests $p from $N.", ch, obj, victim, TO_ROOM);

    if (obj->worn_on)
    {
        unequip_char(victim, obj);
	oprog_remove_trigger(obj);
        sprintf(buf, "\"%s\" %s", obj->name, ch->name);
        do_give(victim, buf);
            
	if (obj->carried_by != ch)
	{
	    act("$N can't seem to give you $p.", ch, obj, victim, TO_CHAR);
	    return;
    	}
	    
	if (ch->level < 51)
            WAIT_STATE(ch, 4*PULSE_VIOLENCE);
    }
    else
    {
        sprintf(buf, "'%s' %s", obj->name, ch->name);
        do_give(victim, buf);
        
	if (obj->carried_by != ch)
	{
	    act("$N can't seem to give you $p.", ch, obj, victim, TO_CHAR);
	    return;
   	}
	    
	WAIT_STATE(ch, 10*PULSE_VIOLENCE);
    }

    ch->pcdata->request_points = URANGE(SILVERAURA,ch->pcdata->request_points + cost,BLACKAURA);

    act("You pause to think on the responsibility that comes with gifts.", ch, NULL, NULL, TO_CHAR);
    af.where     = TO_AFFECTS;
    af.type      = gsn_request;
    af.level     = ch->level;
    af.duration  = 7;
    af.point = NULL;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);
}

/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    int members, ctype = -1, i, j;
    bool multiple = FALSE, cFound = FALSE;
    long amount;
    float cvalue;
    long coins[MAX_COIN], split_coins[MAX_COIN], to_coins[MAX_COIN];

	for (i = 0; i < MAX_COIN; i++)
	{
		coins[i] = 0;
		split_coins[i] = 0;
		to_coins[i] = 0;
	}

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
        send_to_char( "Split how much?\n\r", ch );
        return;
    }

    if (!is_number(arg1))
    {
	send_to_char("Syntax: split <amount> <type>\n\r", ch);
	send_to_char("    or: split <platinum> <gold> <silver> <copper> to split multiple coin types.\n\r", ch);
	return;
    }

    amount = atol( arg1 );

    if (amount < 0)
    {
	send_to_char("Invalid amount.\n\r", ch);
	return;
    }

    if (arg2[0] == '\0')
    {
	send_to_char("What type of coin are you trying to split?\n\r", ch);
	return;
    }

    if (is_number(arg2))
    {
	multiple = TRUE;

	if (ch->coins[0] < amount)
	{
	    sprintf(buf, "You don't have that much %s to split.\n\r", coin_table[0].name);
	    send_to_char(buf, ch);
	    return;
	}

	coins[0] = amount;

	for (i = 1; i < MAX_COIN; i++)
	{
	    if (arg2[0] != '\0')
	    {
		if (is_number(arg2))
		{
		    amount = atol(arg2);
		    if (amount < 0)
		    {
			send_to_char("Invalid amount.\n\r", ch);
			return;
		    }

		    if (ch->coins[i] < amount)
		    {
			sprintf(buf, "You don't have that much %s to split.\n\r", coin_table[i].name);
			send_to_char(buf, ch);
			return;
		    }

		    coins[i] = amount;
		}
		else
		{
		    send_to_char("Multiple currency splits may not include word values.\n\r", ch);
		    return;
		}

		argument = one_argument(argument, arg2);
	    }
	    else
		break;
	}
    }
    else
    {
	if ((ctype = coin_lookup(arg2)) == -1)
	{
	    send_to_char("Invalid currency type.\n\r", ch);
	    send_to_char("Valid options are:", ch);
	    for (i = 0; i < MAX_COIN; i++)
	    {
		send_to_char(" ", ch);
		send_to_char(coin_table[i].name, ch);
	    }
	    send_to_char(".\n\r", ch);
	    return;
	}

	if (ch->coins[ctype] < amount)
	{
	    sprintf(buf, "You don't have that much %s.\n\r", coin_table[ctype].name);
	    send_to_char(buf, ch);
	    return;
	}

	for (i = 0; i < MAX_COIN; i++)
	    coins[i] = 0;

	coins[ctype] = amount;
    }

    if ((cvalue = coins_to_value(coins)) < 0)
    {
        send_to_char( "Your group wouldn't like that.\n\r", ch );
        return;
    }

    if (cvalue == 0)
    {
        send_to_char( "You hand out zero coins, but no one notices.\n\r", ch );
        return;
    }

    members = 0;
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( is_same_group( gch, ch ) && !IS_AFFECTED(gch,AFF_CHARM) && !IS_NPC(gch))
            members++;
    }

    if ( members < 2 )
    {
        send_to_char( "Just keep it all.\n\r", ch );
        return;
    }

    for (i = 0; i < MAX_COIN; i++)
    {
	if (coins[i] > 0)
	    cFound = TRUE;

        split_coins[i] = coins[i] / members;
        coins[i] = coins[i] % members;
    }

    if (!cFound)
    {
	send_to_char("Don't even bother, cheapskate.\n\r", ch);
	return;
    }

    gch = ch->in_room->people;
    while (!is_same_group(gch, ch) || IS_NPC(gch) || IS_AFFECTED(gch, AFF_CHARM))
        gch = gch->next_in_room;

    for (j = members; j > 0; j--)
    {
	for (i = 0; i < MAX_COIN; i++)
	{
	    to_coins[i] = split_coins[i];
	    if ((coins[i] > 0) && (number_range(1, j) <= coins[i]))
	    {
		coins[i] -= 1;
		to_coins[i] += 1;
	    }
	}

	if (gch == ch)
	{
	    sprintf(buf, "You split some coins.  Your share is %s.\n\r", coins_to_str(to_coins));
	    send_to_char(buf, ch);
	}
	else
	{
	    sprintf(buf, "$n splits some coins.  Your share is %s.", coins_to_str(to_coins));
	    act(buf, ch, NULL, gch, TO_VICT);
	}

	if (ch != gch)
	{
	    dec_player_coins(ch, to_coins);
	    inc_player_coins(gch, to_coins);
	}

	gch = gch->next_in_room;
	while (gch && (!is_same_group(gch, ch) || IS_NPC(gch) || IS_AFFECTED(gch, AFF_CHARM)))
	    gch = gch->next_in_room;
    }

    return;
}

void do_gtell( CHAR_DATA *ch, char *argument )
{
    if ( argument[0] == '\0' )
    {
        send_to_char( "Tell your group what?\n\r", ch );
        return;
    }

    comm_send(ch, NULL, argument, CHAN_GTELL);
    return;
}

/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group( CHAR_DATA *ach, CHAR_DATA *bch )
{
    if (ach == NULL || bch == NULL)
        return false;

    // In the case of charmies, step up to the master
    if (IS_NPC(ach) && IS_AFFECTED(ach, AFF_CHARM) && ach->leader != NULL) ach = ach->leader;
    if (IS_NPC(bch) && IS_AFFECTED(bch, AFF_CHARM) && bch->leader != NULL) bch = bch->leader;

    // Check the immediate leaders
    if (ach->leader != NULL) ach = ach->leader;
    if (bch->leader != NULL) bch = bch->leader;
    return (ach == bch);
}

/*
 * Colour setting and unsetting, way cool, Lope Oct '94
 */
void do_colour( CHAR_DATA *ch, char *argument )
{
    char        arg[ MAX_STRING_LENGTH ];

    argument = one_argument( argument, arg );

    if( !*arg )
    {
        if( !IS_SET( ch->act, PLR_COLOUR ) )
        {
            SET_BIT( ch->act, PLR_COLOUR );
            send_to_char( "{bC{ro{yl{co{mu{gr{x is now {rON{x.\n\r", ch );
        }
        else
        {
            send_to_char_bw( "Colour is now OFF, <sigh>\n\r", ch );
            REMOVE_BIT( ch->act, PLR_COLOUR );
	    REMOVE_BIT( ch->nact, PLR_EXCOLOUR );
        }
        return;
    }
    else
    {
        send_to_char_bw( "Colour Configuration is unavailable in this\n\r", ch );
        send_to_char_bw( "version of colour, sorry\n\r", ch );
    }

    return;
}

void do_excolour( CHAR_DATA *ch, char *argument )
{
    if (!IS_SET(ch->act, PLR_COLOUR))
    {
	send_to_char_bw("You must have normal colours on before you can use extended colours.\n\r", ch);
	return;
    }

    if( !IS_SET( ch->nact, PLR_EXCOLOUR ) )
    {
        SET_BIT( ch->nact, PLR_EXCOLOUR );
	send_to_char( "Extended }rc}Ro}Yl}Go}Bu}mr}Ms}x on.\n\r", ch );
    }
    else
    {
        REMOVE_BIT( ch->nact, PLR_EXCOLOUR );
	send_to_char("Extended colours deactivated.\n\r", ch);
    }
    return;
}

DO_FUNC( do_kprompt )
{
    if (!IS_SET(ch->comm, COMM_KILLPROMPT))
    {
	SET_BIT(ch->comm, COMM_KILLPROMPT);
	send_to_char("Your prompt will now be killed before any output.\n\r", ch);
    }
    else
    {
	REMOVE_BIT(ch->comm, COMM_KILLPROMPT);
	send_to_char("Your prompt will no longer be killed before output.\n\r", ch);
    }

    return;
}

void do_display( CHAR_DATA *ch, char *argument )
{
        if( !IS_SET( ch->act, PLR_DISPLAY ) )
        {
            SET_BIT( ch->act, PLR_DISPLAY );
            send_to_char( "Display codes for graphics are now on.\n\r", ch );
            send_to_char( "If you receive !!DISPLAY messages, your client is not compatible.\n\r", ch );
        }
        else
        {
            send_to_char( "Display codes for graphics disabled.\n\r", ch );
            REMOVE_BIT( ch->act, PLR_DISPLAY );
        }
        return;
    return;
}

void play_sound_room(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *vch;

    if (!ch->in_room)
	return;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	play_sound(vch, argument);

    return;
}

bool play_sound(CHAR_DATA *ch, char *argument)
{
    return play_sound_adv(ch, argument, 100, 50, NULL);
}

bool play_sound_adv(CHAR_DATA *ch, char *argument, int volume, int priority, char *stype)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch) || !IS_SET(ch->act, PLR_SOUND))
	return FALSE;
    else
    {
	sprintf(buf, "!!SOUND(%s.wav V=%d P=%d%s%s U=http://www.ender.com/pantheon/sounds/%s.wav)", argument, volume, priority, stype ? " T=" : "", stype ? stype : "", argument);
	send_to_char(buf, ch);
	return TRUE;
    }
}
	

void do_sounds( CHAR_DATA *ch, char *argument )
{
        if( !IS_SET( ch->act, PLR_SOUND ) )
        {
            SET_BIT( ch->act, PLR_SOUND );
            send_to_char( "Mud Sound Protocol is now active.\n\r", ch );
            send_to_char( "If you receive !!SOUND or !!MUSIC tags, your client is not compatible.\n\r", ch );
            send_to_char("!!SOUND(2.wav V=80 L=1 P=100)\n\r", ch);
        }
        else
        {
            send_to_char("!!SOUND(1.wav V=80 L=1 P=100)\n\r", ch);
            send_to_char( "Mud Sound Protocol messages deactivated.\n\r", ch );
            REMOVE_BIT( ch->act, PLR_SOUND );
        }
        return;
}

void do_greet( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim = NULL;
    int greetnum;
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0')
    {
	send_to_char("Whom do you wish to greet?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("You do not see that person here.\n\r", ch);
	return;
    }

    act("You greet $N.", ch, NULL, victim, TO_CHAR);
    act("$n greets you.", ch, NULL, victim, TO_VICT);
    act("$n offers a greeting to $N.", ch, NULL, victim, TO_ROOM);

    greetnum = number_range(0, 2);

    switch (greetnum) {
	case 0:
	    sprintf(buf, "Greetings, %s.", IS_NPC(victim) ? victim->short_descr : victim->name);
	    break;
	case 1:
	    sprintf(buf, "Hail, %s.", IS_NPC(victim) ? victim->short_descr : victim->name);
	    break;
	case 2:
	    sprintf(buf, "Good day to you, %s.", IS_NPC(victim) ? victim->short_descr : victim->name);
	    break;
    }	

    do_say(ch, buf);

    if (IS_NPC(victim))
	mprog_hail_trigger(ch, victim);

    return;
}	
