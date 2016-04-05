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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <map>
#include "merc.h"
#include "interp.h"
#include "CommandTrie.h"
#include "Player.h"

bool is_social(CHAR_DATA * ch, const char * command, int & index);
bool	check_social	args( ( CHAR_DATA *ch, char *command,
			    char *argument ) );



/*
 * Command logging types.
 */
#define LOG_NORMAL	0
#define LOG_ALWAYS	1
#define LOG_NEVER	2


extern int segv_counter;
extern char segv_cmd[MAX_INPUT_LENGTH+1];
extern char segv_char[128];
extern void explosive_alchemy args((CHAR_DATA * ch, int power));

/*
 * Log-all switch.
 */
bool				fLogAll		= FALSE;


/* can_hide, info-command, while-mounted */

/*
 * Command table.
 */
const	struct	cmd_type	cmd_table	[] =
{
    /*
     * Common movement commands.
     */
    { "north",		do_north,	0,POS_STANDING,	0, LOG_NEVER,  0,1,0,1,0},
    { "east",		do_east,	0,POS_STANDING,	0, LOG_NEVER,  0,1,0,1,0},
    { "south",          do_south,	0,POS_STANDING,	0, LOG_NEVER,  0,1,0,1,0},
    { "west",           do_west,	0,POS_STANDING,	0, LOG_NEVER,  0,1,0,1,0},
    { "up",             do_up,		0,POS_STANDING,	0, LOG_NEVER,  0,1,0,1,0},
    { "down",           do_down,	0,POS_STANDING,	0, LOG_NEVER,  0,1,0,1,0},

    /*
     * Common other commands.
     * Placed here so one and two letter abbreviations work.
     */
    { "sleep",          do_sleep,	0,POS_SLEEPING,	0, LOG_NORMAL, 1,1,0,0,0},
    { "ban",            do_ban,		0,POS_DEAD,	L1,LOG_ALWAYS, 1,1,1,1,0},
    { "look",           do_look,	0,POS_RESTING,	0, LOG_NORMAL, 1,1,1,1,0},
    { "where",          do_where,	0,POS_RESTING,	0, LOG_NORMAL, 1,1,1,1,0},
    { "who",            do_newwho,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "exits",          do_exits,	0,POS_RESTING,	0, LOG_NORMAL, 1,1,1,1,0},
    { "nightvision",    do_nightvision,	0,POS_RESTING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "inventory",      do_inventory,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "group",          do_group,	0,POS_SLEEPING,	0, LOG_NORMAL, 1,1,1,1,0},
    { "stand",          do_stand,	0,POS_SLEEPING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "rest",           do_rest,	0,POS_SLEEPING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "lightsleep",     do_lightsleep,	0,POS_SLEEPING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "sit",            do_sit,		0,POS_SLEEPING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "at",             do_at,		0,POS_DEAD,	L8,LOG_NORMAL, 1,1,1,1,0},
    { "assume",         do_assume,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "brand",          do_brand,	0,POS_DEAD,	L6,LOG_ALWAYS, 1,1,1,1,0},
    { "cast",           do_cast,	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "auction",        do_auction,	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "buy",            do_buy,		0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "freetell",       do_freetell,	0,POS_DEAD,	L7,LOG_NORMAL, 1,0,1,1,0},
    { "get",            do_get,		0,POS_RESTING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "goto",           do_goto,	0,POS_DEAD,	L8,LOG_NORMAL, 1,0,1,1,0},
    { "shift",          do_shift,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "hit",            do_kill,	0,POS_FIGHTING,	0, LOG_NORMAL, 0,0,0,1,0},
    { "induct",         do_guild,	0,POS_SITTING,	0, LOG_ALWAYS, 1,1,0,1,0},
    { "force",          do_force,	0,POS_DEAD,	L7,LOG_NORMAL, 1,1,1,1,0},
    { "focus",		do_focus,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,0,1,0},
    { "unfocus",	do_unfocus,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,0,1,0},
    { "kill",           do_kill,	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "drink",          do_drink,	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "deposit",	do_deposit,	0,POS_SITTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "dt", 		do_druidtalk,	0,POS_SLEEPING,	0, LOG_NORMAL, 1,0,0,1,1},
    { "druid",		do_druidtalk,	0,POS_SLEEPING,	0, LOG_NORMAL, 1,0,0,1,1},
    { "corpsewhere",    do_corpsewhere,	0,POS_SLEEPING,	0, LOG_NORMAL, 1,0,0,1,1},
    { "ht",             do_clantalk,	0,POS_SLEEPING,	0, LOG_NORMAL, 1,0,0,1,1},
    { "house",          do_clantalk,	0,POS_SLEEPING,	0, LOG_NORMAL, 1,0,0,1,1},
    { "multisoc",       do_multisoc,	0,POS_DEAD,	L5,LOG_NORMAL, 1,0,1,1,0},
//    { "music",          do_music,	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,1}, 
    { "ooctell",        do_ooctell,	0,POS_DEAD,	0, LOG_NORMAL, 1,0,0,1,1},
    { "order",          do_order,	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "practice",       do_practice,	0,POS_SLEEPING,	0, LOG_NORMAL, 1,1,1,1,0},
    { "pray",           do_pray,	0,POS_DEAD,	0, LOG_ALWAYS, 1,1,1,1,1},
    { "release",        do_release,	0,POS_RESTING,	0, LOG_NORMAL, 1,1,1,1,0},
    { "sockets",        do_sockets,	0,POS_DEAD,	L3,LOG_NORMAL, 1,0,1,1,0},
    { "tell",           do_tell,	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,1},
    { "unlock",         do_unlock,	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "wield",          do_wear,	0,POS_RESTING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "vanish",         do_vanish,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "request",	do_request,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "inspect",	do_inspect,	0,POS_RESTING,	0, LOG_NORMAL, 1,1,1,1,0},
    { "intimidate",	do_intimidate,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "wizhelp",        do_wizhelp,	0,POS_DEAD,	IM,LOG_NORMAL, 1,1,1,1,0},
    { "open",           do_open,	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "opstat",         do_opstat,	0,POS_DEAD,	IM,LOG_NORMAL, 0,1,1,1,0},
    { "rcheck",         do_rcheck,	0,POS_DEAD,	L5,LOG_NORMAL, 1,0,1,1,0},
    { "cgen",           do_cgen,	0,POS_DEAD,	L2,LOG_NORMAL, 1,0,1,1,0},
    { "materials",      do_materials,	0,POS_DEAD,	L6,LOG_NORMAL, 1,0,1,1,0},
    { "trip",           do_trip,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "withdrawal",	do_withdrawal, 	0,POS_SITTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "mindlink",	do_mindlink,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "idle",		do_idle,	0,POS_DEAD,	L8,LOG_NORMAL, 1,0,1,1,0},
    { "areaupdate",	do_areaupdate,	0,POS_DEAD,	L1,LOG_NORMAL, 1,0,1,1,0},	

    /*
     * Informational commands.
     */
    { "affects",        do_affects,	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "abilities",	do_abilities,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "areas",          do_areas,    	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "balance",	do_balance,  	0,POS_SITTING,	0, LOG_NORMAL, 1,0,1,1,0},
    { "bug",            do_bug,      	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "changes",        do_changes,  	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "command",	do_command,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "commands",       do_commands, 	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "compare",        do_compare,  	0,POS_RESTING,  0, LOG_NORMAL, 1,1,1,1,0},
    { "consider",       do_consider, 	0,POS_RESTING,  0, LOG_NORMAL, 1,1,1,1,0},
    { "convert",	do_convert,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "count",          do_count,    	0,POS_SLEEPING, 0, LOG_NORMAL, 1,1,1,1,0},
    { "credits",        do_credits,  	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "equipment",      do_equipment,	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "examine",        do_examine,  	0,POS_RESTING,  0, LOG_NORMAL, 1,1,1,1,0},
    { "factions",       do_factions,    0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
//  { "groups",         do_groups,   	0,POS_SLEEPING, 0, LOG_NORMAL, 1,0,1,1,0},
    { "help",           do_help,     	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "idea",           do_idea,     	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "info",           do_groups,   	0,POS_SLEEPING, IM, LOG_NORMAL, 1,1,1,1,0},
    { "languages",	do_languages,	0,POS_SLEEPING, 0, LOG_NORMAL, 1,1,1,1,0},
    { "lore",		do_lore,     	0,POS_RESTING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "motd",           do_motd,     	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "read",           do_read,     	0,POS_RESTING,  0, LOG_NORMAL, 1,1,1,1,0},
    { "report",         do_report,   	0,POS_RESTING,  0, LOG_NORMAL, 1,0,1,1,0},
    { "rules",          do_rules,    	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "score",          do_score,    	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "animals",	do_rangermenu,  0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "oldscore",	do_oldscore, 	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "scan",           do_scan,     	0,POS_RESTING,  0, LOG_NORMAL, 1,0,1,1,0},
    { "skills",         do_skills,   	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "forms",		do_forms,    	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "socials",        do_socials,  	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "show",           do_show,     	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "spells",         do_spells,   	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "story",          do_story,    	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "time",           do_time,     	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "typo",           do_typo,     	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "noweather",	do_noweather,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "weather",        do_weather,  	0,POS_RESTING,  0, LOG_NORMAL, 1,1,1,1,0},
    { "whois",          do_newwhois, 	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "wizlist",        do_wizlist,  	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "worth",          do_worth,    	0,POS_SLEEPING, 0, LOG_NORMAL, 1,1,1,1,0},
    { "write",		do_write,	0,POS_RESTING,	0, LOG_NORMAL, 1,1,0,1,0},
    /*
     * Configuration commands.
     */
    { "alia",           do_alia,     	0,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "alias",          do_alias,    	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "extraspam",	do_extraspam,	0,POS_DEAD,	ML,LOG_NORMAL, 1,1,1,1,0},
    { "autolist",       do_autolist, 	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "autopeek",       do_autopeek, 	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "autooath",       do_autooath, 	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "autotracks",     do_autotracks, 	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "autoattack",	do_autoattack,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "autodefend",	do_autodefend,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "autoassist",     do_autoassist,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "showlines",      do_showlines,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "accept",		do_accept,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "epshow",     	do_epshow,   	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "autoexit",       do_autoexit, 	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "autogold",       do_autogold, 	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "autoloot",       do_autoloot, 	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "autodestroy",    do_autodestroy, 0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "autosplit",      do_autosplit,	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "autodate",	do_autodate,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "brief",          do_brief,    	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "color",          do_colour,   	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "colour",		do_colour,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "excolour",	do_excolour,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "excolor",	do_excolour,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "sounds",         do_sounds,   	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "display",        do_display,  	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "combine",        do_combine,  	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "compact",        do_compact,  	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "description",    do_description, 0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "delet",          do_delet,    	0,POS_DEAD,     0, LOG_ALWAYS, 0,1,1,1,0},
    { "delete",         do_delete,   	0,POS_STANDING, 0, LOG_ALWAYS, 1,1,1,1,0},
    { "nofollow",       do_nofollow, 	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "nosummon",       do_nosummon, 	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "outfit",         do_outfit,   	0,POS_RESTING,  0, LOG_NORMAL, 1,1,1,1,0},
    { "password",       do_password, 	0,POS_DEAD,     0, LOG_NEVER,  1,1,1,1,0},
    { "prompt",         do_prompt,   	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "defaultprompt",  do_defaultprompt,0,POS_DEAD,    0, LOG_NORMAL, 1,1,1,1,0},
    { "battlecry",      do_battlecry,	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "scroll",         do_scroll,   	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "speak",		do_speak,    	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "title",          do_title,    	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "unalias",        do_unalias,  	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "wimpy",          do_wimpy,    	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "room",		do_room,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},

    /*
     * Communication commands.
     */
    { "deaf",           do_deaf,     	0,POS_DEAD,     L7,LOG_NORMAL, 1,1,1,1,0},
    { "emote",          do_emote,    	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,1},
    { "pmote",          do_pmote,    	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,1},
    { "gtell",          do_gtell,    	0,POS_SLEEPING, 0, LOG_NORMAL, 1,0,0,1,1},
    { "cant",           do_cant,     	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "note",           do_note,     	0,POS_SLEEPING, 0, LOG_NORMAL, 1,1,0,1,1},
    { "notify",		do_notify,   	0,POS_SLEEPING,	0, LOG_NORMAL, 1,1,1,1,0},
    { "oocoff",         do_oocoff,   	0,POS_DEAD, 0, LOG_NORMAL, 1,1,1,1,0},
    { "reply",          do_reply,    	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,1},
    { "oocreply",       do_oocreply,    0,POS_DEAD,  0, LOG_NORMAL, 1,0,0,1,1},
    { "replay",         do_replay,   	0,POS_SLEEPING, 0, LOG_NORMAL, 1,1,1,1,0},
    { "say",            do_say,      	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,1},
    { "esay",		do_esay,     	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,1},
    { "sing",           do_sing,     	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "unread",         do_unread,   	0,POS_SLEEPING, 0, LOG_NORMAL, 1,1,1,1,0},
    { "yell",           do_char_yell,	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,1},
    { "think",		do_think,    	0,POS_RESTING,	0, LOG_NORMAL, 1,1,1,1,1},
    { "greet",		do_greet,    	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,1},
    { "mimic",		do_mimic,    	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "rnote",		do_rnote,	0,POS_SLEEPING, 0, LOG_NORMAL, 1,1,1,1,0},
    { "newbietalk",	do_newbietalk,	0,POS_SLEEPING, 0, LOG_NORMAL, 1,1,1,1,1},
    { "pose",           do_pose,    	0,POS_SLEEPING,  0, LOG_NORMAL, 1,0,0,1,1},

    /*
     * Object manipulation commands.
     */
    { "brandish",       do_brandish, 	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "close",          do_close,    	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "wildcraft",      do_wildcraft,  	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "drop",           do_drop,     	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "stash",        	do_stash,    	0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "conceal",       	do_conceal,    	0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "forage",    	do_forage,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "findwater",    	do_findwater,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "findherbs",	do_findherbs,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "huntersense",   	do_huntersense,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "pursuit",   	do_pursuit,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "callanimal",   	do_callanimal,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "search",        	do_search,   	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "investigate",    do_investigate,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "sentry",    	do_sentry,   	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "trap",           do_trap,     	0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "daggertrap",     do_daggertrap, 	0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "findcover",      do_findcover, 	0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "tripwire",       do_tripwire, 	0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "track",          do_track,    	0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "pursue",         do_pursue,   	0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "cover",		do_cover,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "contactagents",  do_contact_agents,0,POS_STANDING,0,LOG_NORMAL, 1,0,0,0,0},
    { "eat",            do_eat,      	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "envenom",        do_envenom,  	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "eyefocus",	do_eyefocus, 	0,POS_RESTING,	0, LOG_NORMAL, 1,1,0,1,0},
//    { "ignite",         do_ignite,   	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "fill",           do_fill,     	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "give",           do_give,     	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "heal",           do_heal,     	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0}, 
    { "hold",           do_wear,     	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "list",           do_list,     	0,POS_RESTING,  0, LOG_NORMAL, 1,0,1,1,0},
    { "lock",           do_lock,     	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,0,0},
    { "pick",           do_pick,     	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,0,0},
    { "relock",         do_relock,   	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,0,0},
    { "ram",            do_ram,      	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "pour",           do_pour,     	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "put",            do_put,      	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "quaff",          do_quaff,    	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "recite",         do_recite,   	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "remove",         do_remove,   	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "sell",           do_sell,     	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "itemreturn",     do_returnitem,  0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "take",           do_get,      	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "taint",		do_taint,    	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "prepare",	do_prepare,  	0,POS_STANDING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "craftdart",      do_craftdart,	0,POS_STANDING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "tan",		do_tan,	     	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "bloodsigil",	do_bloodsigil,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "cook",		do_cook,     	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "fetish",		do_fetish,   	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "sacrifice",      do_sacrifice,	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "junk",           do_sacrifice,	0,POS_RESTING,  0, LOG_NORMAL, 0,0,0,1,0},
    { "destroy",	do_destroy,	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "value",          do_value,    	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "wear",           do_wear,     	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "zap",            do_zap,      	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "identifyowner",  do_identifyowner,0,POS_RESTING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "inscribe",	do_inscribe, 	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "erase",		do_erase,    	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,0,0}, 
    { "debone",		do_debone,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "appraise",	do_appraise,	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "estimate",	do_estimate,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "repair",		do_repair,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,0,0},

    /*
     * Combat commands.
     */
    { "backstab",       do_backstab, 	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "stab",           do_stab,     	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "circle",         do_circle,   	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "cower",		do_cower,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "bash",           do_bash,     	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "shieldbash",	do_shieldbash,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "muffle",         do_muffle,   	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "bs",             do_backstab, 	0,POS_FIGHTING, 0, LOG_NORMAL, 0,0,0,0,0},
    { "berserk",        do_berserk,  	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "befriend",       do_befriend, 	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "dirt",           do_dirt,     	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "disarm",         do_disarm,   	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "dive",		do_dive,     	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "offhanddisarm",  do_offhanddisarm,0,POS_FIGHTING,0, LOG_NORMAL, 1,0,0,1,0},
    { "shielddisarm",   do_shielddisarm,0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "strip",         	do_strip,    	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "caltraps",       do_caltraps, 	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "garrote",        do_garrote,  	0,POS_FIGHTING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "submission",     do_submissionhold,0,POS_FIGHTING,0,LOG_NORMAL, 1,0,0,0,0},
    { "disbelieve",     do_disbelieve,	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "flee",           do_flee,     	0,POS_FIGHTING, 0, LOG_NORMAL, 1,1,0,1,0},
    { "kick",           do_kick,     	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "murde",          do_murde,    	0,POS_FIGHTING, 0, LOG_NORMAL, 0,0,1,1,0},
    { "murder",         do_murder,   	0,POS_FIGHTING, 5, LOG_ALWAYS, 1,0,0,1,0},
    { "rescue",         do_rescue,   	0,POS_FIGHTING, 0, LOG_NORMAL, 0,0,0,1,0},
    { "distract",       do_distract,   	0,POS_FIGHTING, 0, LOG_NORMAL, 0,0,0,1,0},
    { "flank",          do_flank,    	0,POS_FIGHTING, 0, LOG_NORMAL, 0,0,0,1,0},
    { "bludgeon",       do_bludgeon, 	0,POS_FIGHTING, 0, LOG_NORMAL, 0,0,0,1,0},
    { "vitalstrike",    do_vitalstrike,	0,POS_FIGHTING, 0, LOG_NORMAL, 0,0,0,0,0},
    { "legsweep",       do_legsweep, 	0,POS_FIGHTING, 0, LOG_NORMAL, 0,0,0,0,0},
    { "throw",          do_throw,    	0,POS_FIGHTING, 0, LOG_NORMAL, 0,0,0,0,0}, 
    { "dart",           do_dart,     	0,POS_FIGHTING, 0, LOG_NORMAL, 0,0,0,1,0}, 
    { "bolo",           do_bolo,     	0,POS_FIGHTING, 0, LOG_NORMAL, 0,0,0,1,0}, 
    { "waylay",		do_waylay,   	0,POS_STANDING,	0, LOG_NORMAL, 0,1,0,0,0},
    { "swordrepair",	 do_swordrepair,0,POS_RESTING, 	0, LOG_NORMAL, 1,0,0,1,0},
    { "pommel",		do_pommel,   	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "rout",		do_rout,   	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "pummel",		do_pummel,   	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "uppercut",	do_uppercut, 	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "forcedmarch",	do_forcedmarch,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "dash",		do_dash,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "scouting",	do_scouting, 	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "hew",		do_hew,	     	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "pounce",		do_pounce,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "rob",		do_rob,		0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "predatoryattack",do_predatoryattack,0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "nerve",		do_nerve,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "lunge",		do_lunge,   	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "jab",		do_jab,   	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "headbutt",	do_headbutt,   	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "rally",		do_rally,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "maul",		do_maul,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "healingward",	do_healingward, 0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "warcry",         do_warcry,   	0,POS_FIGHTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "bravado",        do_bravado,   	0,POS_FIGHTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "enthusiasm",     do_enthusiasm, 	0,POS_FIGHTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "endure",	 	do_endure, 	0,POS_FIGHTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "taunt",		do_taunt,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "fury",           do_fury,     	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "whirl",          do_whirl,     	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "rage",           do_rage,     	0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "guard",		do_guard,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "readiness",	do_readiness,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "warpath",	do_warpath,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "agility",	do_agility,  	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "wariness",	do_wariness, 	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "bandage",	do_bandage,  	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "bandarrow",	do_bandarrow,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "drive",		do_drive,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "entangle",	do_entangle, 	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "grip",		do_grip,     	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "cross",		do_cross,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "feint",		do_feint,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "lash",		do_lash,     	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "flay",           do_flay,     	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "sortie",         do_sortie,   	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "choke",		do_choke,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "batter",		do_batter,   	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "cleave",		do_cleave,   	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "inertialstrike",	do_inertialstrike,0,POS_FIGHTING,0,LOG_NORMAL, 1,0,0,1,0},
    { "disruption",	do_disruption,	0,POS_FIGHTING,0,LOG_NORMAL, 1,0,0,1,0},
    { "shockwave",	do_shockwave,	0,POS_DEAD,	0, LOG_NORMAL, 1,0,0,0,0},
    { "tame",		do_tame,     	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "flashpowder",	do_flashpowder,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "blindingdust",	do_blindingdust,0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "blindingcloud",	do_blindingcloud,0,POS_FIGHTING,0, LOG_NORMAL, 1,0,0,1,0},
    { "ambush",		do_ambush,   	0,POS_STANDING,	0, LOG_NORMAL, 1,1,0,0,0},
    { "loot",		do_loot,     	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "hamstring",	do_hamstring,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "slice",		do_slice,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "grapple",	do_grapple,  	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "boneshatter",	do_boneshatter,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "ensnare",	do_ensnare,  	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "gouge",		do_gouge,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "impale",		do_impale,   	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "savagery",	do_savagery, 	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "aggression",	do_aggression, 	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "fortitude",	do_fortitude, 	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "pugil",		do_pugil,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "twirl",		do_twirl,    	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "bind",		do_bind,     	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "escape",		do_escape,   	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "shoot",          do_shoot,    	0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,1,0},
    { "ready",          do_ready,    	0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,1,0},
    { "retreat",	do_retreat,  	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "beg",		do_beg,	     	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "grantmercy",	do_grantmercy,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "mercy",		do_mercy,    	0,POS_RESTING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "gash",		do_gash,	0,POS_FIGHTING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "sweep",		do_sweep,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,1,0,0,0},	
    /*
     * Alchemist commands
     */
    { "mix",		do_mix,		0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "brew",		do_brew,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "make",		do_make,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "pitch",		do_pitch,	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "distill",        do_distill,     0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "pulverize",      do_pulverize,   0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "reforge",        do_reforge,     0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "boil",           do_boil,        0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "dissolve",       do_dissolve,    0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "dilute",         do_dilute,      0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "disfigure",      do_disfigure,      0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "delayreaction", do_delayreaction,0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "animategolem",	do_animategolem,0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "dye",		do_dye,		0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "discernmagic",	do_discernmagic,0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "finditems",	do_finditems,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "harrudimtalc",   do_harrudimtalc,0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "talc",		do_harrudimtalc,0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "smokescreen",	do_smokescreen, 0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "acidicbrew",	do_rynathsbrew, 0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "alchemicstone",do_alchemicstone,0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "applyoil",	do_applyoil,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "applybarbs",	do_applybarbs,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "applypoison",	do_applypoison,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "fletch",		do_fletch,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "createwand",	do_createwand,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "colorlist",	do_colorlist,	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "createhomonculus",do_createhomonculus,0,POS_STANDING,0,LOG_NORMAL,1,0,0,1,0},

	/* Chirurgeon Commands */
    { "cauterize",	do_cauterize,	0, POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "whipstitch",	do_whipstitch,	0, POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "anesthetize",	do_anesthetize,	0, POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "animatedead",	do_animate_dead,	0, POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    
    /*
     * Miscellaneous commands.
     */
    { "shieldcover",	do_shieldcover, 0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "nock",           old_do_nock,    0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,1,0, do_nock},
    { "drag",           do_drag,        0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "gag",            do_gag,         0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},    
    { "listen",		do_listen,	0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,1,0},
    { "inquire",	do_inquire,	0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,1,0},
    { "record",		do_record,	0,POS_STANDING, 0, LOG_NORMAL, 1,1,1,1,0},
    { "bounty",		do_bounty,	0,POS_STANDING, 0, LOG_NORMAL, 1,1,1,1,0},
    { "runicmessage",	do_runicmessage,0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "background",	do_background,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "bg",		do_background,  0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "surpriseattack", do_surpriseattack,0,POS_STANDING,0,LOG_NORMAL, 1,1,0,1,0},
    { "disguise",	do_disguise,	0,POS_STANDING,	0, LOG_NORMAL, 1,1,0,0,0},
    { "hoodedrogue",    do_hooded,      0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "encamp",         do_encamp,      0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,0,0},
    { "enter",          do_enter,       0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "exdesc",		do_exdesc,	0,POS_SLEEPING,	0, LOG_NORMAL, 1,1,1,1,0},
    { "exsanguinate",	do_exsanguinate,0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "follow",         do_follow,      0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "gain",           do_gain,        0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "channels",       do_channels,    0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "go",             do_enter,       0,POS_STANDING, 0, LOG_NORMAL, 0,0,0,1,0},
    { "detecthidden",   do_detecthidden,0,POS_RESTING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "sixthsense",  	do_sixthsense,	0,POS_RESTING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "perception",  	do_perception,	0,POS_RESTING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "camouflage",  	do_camouflage,	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "camoblind",      do_camoublind,  0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "sharpvision",  	do_sharp_vision,0,POS_RESTING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "poultice",  	do_poultice,	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "herbalmedicine", do_herbalmedicine,0,POS_RESTING,0, LOG_NORMAL, 1,0,0,0,0},
    { "hide",           do_hide,        0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "shadowmastery",  do_shadowmastery,0,POS_STANDING,0, LOG_NORMAL, 1,1,0,0,0},
//    { "cloak",          do_cloak,       0,POS_RESTING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "perform",	do_perform,	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "play",           do_play,        0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "qui",            do_qui,         0,POS_DEAD,     0, LOG_NORMAL, 0,1,0,1,0},
    { "quit",           do_quit,        0,POS_DEAD,     0, LOG_NORMAL, 1,1,0,1,0},
    { "spiritrecall",   do_spiritrecall,0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "recall",         do_recall,      0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "save",           do_save,        0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "suicide",	do_suicide,	0,POS_DEAD,	0, LOG_ALWAYS, 1,0,1,1,0},
    { "sneak",          do_sneak,       0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "split",          do_split,       0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "songs",		do_songs,	0,POS_SLEEPING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "songbook",	do_songbook,	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "steal",          do_steal,       0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "wildmove",       do_wildmovement,0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "plant",          do_plant,       0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "cutpurse",       do_cutpurse,    0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "train",          do_train,       0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "visible",        do_visible,     0,POS_SLEEPING, 0, LOG_NORMAL, 1,1,0,1,0},
    { "wake",           do_wake,        0,POS_SLEEPING, 0, LOG_NORMAL, 1,1,0,1,0},
    { "noyell",		do_noyell,	0,POS_SLEEPING, 0, LOG_NORMAL, 1,1,1,1,0},
    { "mount",		do_mount,	0,POS_STANDING, ML,LOG_NORMAL, 1,0,0,1,0},
    { "dismount",	do_dismount,	0,POS_STANDING, ML,LOG_NORMAL, 1,0,0,1,0},
    { "retainer",	do_retainer,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "harmonize",	do_harmonize,	0,POS_RESTING,	0, LOG_NORMAL, 1,0,0,1,0},
    { "fly",		do_fly,		0,POS_SLEEPING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "land",		do_land,	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "obscure",	do_obscure,	0,POS_RESTING,	0, LOG_NORMAL, 1,1,0,1,0},
    { "finquiry",	do_finquiry,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},

    /*
     * Account-related commands
     */
    { "award",		do_award,	0,POS_SLEEPING, L5,LOG_NORMAL, 1,1,1,1,0},
    { "acinfo",		do_acctinfo,	0,POS_SLEEPING, L6,LOG_NORMAL, 1,1,1,1,0},
    { "aclist",		do_aclist,	0,POS_SLEEPING, L6,LOG_NORMAL, 1,1,1,1,0},
    { "acctinfo",	do_acctinfo,	0,POS_SLEEPING, L6,LOG_NORMAL, 1,1,1,1,0},
    { "acctdata",	do_acctdata,	0,POS_SLEEPING, L6,LOG_NORMAL, 1,1,1,1,0},
    { "acctpwd",	do_acctpwd,	0,POS_SLEEPING, 0, LOG_NEVER, 1,1,1,1,0},

    /*
     * HOUSE commands
     */
    { "criminal",	do_criminal,	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},

    /*
     * fun commands.
     */
    { "chess",		do_chess, 	0,POS_RESTING,  0, LOG_NORMAL, 1,0,0,1,0},
    { "cards",		do_cards,	0,POS_RESTING,  L3,LOG_NORMAL, 1,0,0,0,0},
    { "puerto",		do_puerto,	0,POS_SLEEPING,	L3,LOG_NORMAL, 1,0,0,0,0},
    { "coloretto",	do_coloretto,	0,POS_SLEEPING, L3,LOG_NORMAL, 1,0,0,0,0},

    /*
     * Temporary commands.
     */
    { "xmusic",		do_xmusic,	0,POS_DEAD,	ML,LOG_NORMAL, 1,1,1,1,0},
    { "playmusic",	do_playmusic,	0,POS_DEAD,	ML,LOG_NORMAL, 1,1,1,1,0},

    // Fire scholar commands
    { "infernaldance", do_firedancer, 0, POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "incinerate", do_incinerate, 0, POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "bloodpyre", do_bloodpyre, 0, POS_DEAD, 0, LOG_NORMAL, 1,0,0,1,0},

    // Spirit scholar commands
    { "weavesense", do_weavesense, 0, POS_SLEEPING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "chantlitany", do_chantlitany, 0, POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "radiateaura", do_radiateaura, 0, POS_RESTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "triumphantshout", do_triumphantshout, 0, POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "assesssoul", do_assesssoul, 0, POS_RESTING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "sensedreamers", do_sensedreamers, 0, POS_SLEEPING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "relocate", do_relocate, 0, POS_SLEEPING, 0, LOG_NORMAL, 1,0,0,1,0},

    // Water scholar commands
    { "somaticarts", do_somaticarts, 0, POS_DEAD, 0, LOG_NORMAL, 1,0,0,1,0},

    // Air scholar commands
    {"shockcraft", do_shockcraft, 0, POS_DEAD, 0, LOG_NORMAL, 1,0,0,1,0},
    {"phantasm", do_phantasm, 0, POS_DEAD, 0, LOG_NORMAL, 1,0,0,1,0},
    {"deliver", do_deliver, 0, POS_RESTING, 0, LOG_NORMAL, 1,0,0,1,0},

    // Earth scholar commands
    {"adamantineinvocation", do_adamantineinvocation, 0,POS_RESTING, 0, LOG_NORMAL, 1,0,0,1,0},
    {"carve", do_carve, 0, POS_RESTING, 0, LOG_NORMAL, 1,0,0,1,0},
    {"rune", do_rune, 0, POS_DEAD, 0, LOG_NORMAL, 1,0,0,1,0},
    {"constructwaterwheel", do_constructwaterwheel, 0, POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    {"forgeweapon", do_forgeweapon, 0, POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    {"nameweapon", do_nameweapon, 0, POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    {"wakenstone", do_wakenstone, 0, POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    {"clockworksoul", do_clockworksoul, 0, POS_DEAD, 0, LOG_NORMAL, 1,0,0,1,0},

    // Void scholar commands
    {"fellpurpose", do_fellpurpose, 0, POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    {"corpsesense", do_corpsesense, 0, POS_SLEEPING, 0, LOG_NORMAL, 1,0,0,1,0},
    {"repossess", do_repossess, 0, POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},

    /*
     * MOBprogram commands.
     */
    { "rpstat",		do_rpstat,	0,POS_DEAD,	L7,LOG_NORMAL, 0,1,1,1,0},
    { "mpstat",         do_mpstat,      0,POS_DEAD,     L7,LOG_NORMAL, 0,1,1,1,0},

    { "mpstatdown",     0,prog_statdown  ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpstatup",       0,prog_statup    ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpasound",       0,prog_asound    ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpjunk",         0,prog_junk      ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpecho",         0,prog_echo      ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpzecho",        0,prog_zecho     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpgecho",	0,prog_gecho     ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpechoat",       0,prog_echoat    ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpechoaround",	0,prog_echoaround,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpwander",   	0,prog_wander    ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpkill",         0,prog_kill      ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mprangekill",    0,prog_rangekill ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mptrap",         0,prog_trap      ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdammess",	0,prog_dammess   ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdamfrom",	0,prog_damfrom   ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdealdamage",	0,prog_dealdamage   ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdamfromtype",	0,prog_damfromtype,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpbrand",        0,prog_brand     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpmload",        0,prog_mload     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpoload",        0,prog_oload     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mptellimm",	0,prog_tellimm   ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mppurge",        0,prog_purge     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpget",		0,prog_get       ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpbitset",       0,prog_bitset    ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpset",          0,prog_set       ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpsetskills",    0,prog_set       ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpflag",         0,prog_flag      ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpstring",       0,prog_string    ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpgoto",         0,prog_goto      ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpsetskilled",   0,prog_setskilled,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpunsetskilled", 0,prog_unsetskilled,POS_DEAD,   0, LOG_NORMAL, 0,1,1,1,0},
    { "mpat",           0,prog_at        ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpsetdata",	0,prog_setdata   ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpnextdata",	0,prog_nextdata  ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpadddata",	0,prog_adddata  ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdeletedata", 0,prog_deletedata  ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mplag",     	0,prog_lag       ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mptransfer",     0,prog_transfer  ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mploot",     	0,prog_loot      ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpforce",        0,prog_force     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpexit",		0,prog_exit      ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpwizi",        	0,prog_wizi      ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpunwizi",      	0,prog_unwizi    ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpseewizi",     	0,prog_seewizi   ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpunseewizi",   	0,prog_unseewizi ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpxp",        	0,prog_xp        ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mptrack",       	0,prog_track     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mptrackstep",    0,prog_trackstep ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdamage",      	0,prog_damage    ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mplock",		0,prog_lock      ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mphitdamage",	0,prog_hitdamage ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpmath",		0,prog_math      ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdice",		0,prog_dice      ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mppermexdesc",	0,prog_permexdesc,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdamtype",      0,prog_damtype   ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpsilverset",	0,prog_silverset ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpunlock",	0,prog_unlock    ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpslay",      	0,prog_slay      ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpremember",     0,prog_remember  ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpforget",       0,prog_forget    ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpcopyvalues",   0,prog_copyvalues ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpfocus",        0,prog_focus     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpgrant",        0,prog_grant     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpunfocus",      0,prog_unfocus   ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpmemvset",      0,prog_memvset   ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpmemvup",       0,prog_memvup    ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpmemvdown",     0,prog_memvdown  ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpmemvrand",     0,prog_memvrand  ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpsetexit",      0,prog_setexit   ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpvalueset",     0,prog_valueset  ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpvalueup",      0,prog_valueup   ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpvaluedown",    0,prog_valuedown ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpvaluerand",    0,prog_valuerand ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpcast",		0,prog_cast      ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mproomcmd",	0,prog_roomcmd   ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mptrigger",	0,prog_trigger   ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpinterpret",	0,prog_interpret ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpaddaffect",	0,prog_addaffect ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpaddskill",	0,prog_addskill  ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpremoveskill",	0,prog_removeskill,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpremskill",	0,prog_removeskill,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mproomaffect",	0,prog_roomaffect,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpknockout",	0,prog_knockout  ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpstopfollow",   0,prog_stopfollow,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mptentacle",	0,prog_tentacle  ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpvaultprep",	0,prog_vaultprep ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpvaultshow",	0,prog_vaultshow ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpvaultgive",	0,prog_vaultgive ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mprevolt", 	0,prog_revolt    ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpescape", 	0,prog_escape    ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpvaultdrop",	0,prog_vaultdrop ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpbittrigger", 	0,prog_bittrigger,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpunbittrigger",	0,prog_unbittrigger,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpmasstrigger", 	0,prog_masstrigger,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpremaffect",	0,prog_remaffect ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpslotremove",	0,prog_slotremove,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpslotpurge",	0,prog_slotpurge ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpverbstop",	0,prog_verbstop  ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdemonid",	0,prog_demonid   ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdemonpcid",	0,prog_demonpcid ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpaddroomaffect",0,prog_addroomaffect,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mppoisonroom",	0,prog_poisonroom,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdemonbind",	0,prog_demonbind ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpstealrandom",	0,prog_stealrandom,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpgetroomvnum",	0,prog_getroomvnum,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpremovehand",	0,prog_removehand,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpaddsymbol",	0,prog_addsymbol ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpremsymbol",	0,prog_removesymbol,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpfindpath",	0,prog_findpath  ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mppathstep",	0,prog_pathstep  ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpaddtree",	0,prog_addtree	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpremareaaffect",0,prog_remareaaffect,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mptreefruit",	0,prog_treefruit ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdoppel",	0,prog_doppel	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdisarm",	0,prog_disarm	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mptakecoins",	0,prog_takecoins ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpdesc",		0,prog_desc      ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mploaddesc",		0,prog_loaddesc      ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mploadstring",	0,prog_loadstring      ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpresolve",	0,prog_resolve      ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpconcat",	0,prog_concat	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpstrreplace",	0,prog_strreplace,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpstrlen",	0,prog_strlen    ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpaddevent",	0,prog_addevent	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpcursebarkja",	0,prog_cursebarkja,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpfaction",	0,prog_faction	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpfollow",	0,prog_follow	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpcallsub",	0,prog_callsub	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},

//    { "mpabishai",	do_mpabishai,   0,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mporzub",	do_mporzub,	0,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "mpchagrob",	do_mpchagrob,   0,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpbarkjaset",	do_mpbarkjaset, 0,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpbarkjayes",	do_mpbarkjayes, 0,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpgharku", 	do_mpgharku,	0,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpisetaton", 	do_mpisetaton,	0,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpagduk", 	do_mpagduk,	0,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
//    { "mpkhamurn", 	do_mpkhamurn,	0,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mppricina", 	do_mppricina,	0,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mplistbounty",	0,prog_listbounty ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},


    /*
     * OBJprogram commands.
     */
    { "opasound",       0,prog_asound	 ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opjunk",         0,prog_junk	 ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opflag",		0,prog_flag	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "opecho",         0,prog_echo	 ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opecho_fight",   0,prog_echo_fight,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opechoat",       0,prog_echoat	 ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opechoaround",   0,prog_echoaround,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opmload",        0,prog_mload     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opoload",        0,prog_oload     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "oppurge",        0,prog_purge     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opgoto",         0,prog_goto      ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opat",           0,prog_at        ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "optransfer",     0,prog_transfer  ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opforce",        0,prog_force     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opxp",        	0,prog_xp     	 ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opset",          0,prog_set       ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opslotremove",   0,prog_slotremove,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "opslotpurge",    0,prog_slotpurge ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opstring",	0,prog_string	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "opgrant",        0,prog_grant     ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opdamage",       0,prog_damage    ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opzap",          0,prog_zap       ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opinterpret",	0,prog_interpret ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "opslay",         0,prog_slay      ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opstatdown",     0,prog_statdown  ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opstatup",       0,prog_statup    ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "oplag",          0,prog_lag       ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opvalueset",     0,prog_valueset  ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opvalueup",      0,prog_valueup   ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opvaluedown",    0,prog_valuedown ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opvaluerand",    0,prog_valuerand ,POS_DEAD,     0, LOG_NORMAL, 0,1,1,1,0},
    { "opcast",		0,prog_cast	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "opcastoff",	0,prog_castoff	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "opbitset",	0,prog_bitset	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "opaddaffect",	0,prog_addaffect ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "opremaffect",	0,prog_remaffect ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "opfocus",	0,prog_focus	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "opunfocus",	0,prog_unfocus	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "opverbstop",	0,prog_verbstop	 ,POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},

    /*
     * Immortal commands.
     */
    { "advance",        do_advance,     0,POS_DEAD,     L1,LOG_ALWAYS, 1,1,1,1,0},
//  { "brand",		do_brand,	0,POS_DEAD,	L7,LOG_ALWAYS, 1,0,1,1,0},
    { "dump",           do_dump,        0,POS_DEAD,     ML,LOG_ALWAYS, 0,0,1,1,0},
    { "showdata",	do_showdata,	0,POS_DEAD,	L7,LOG_NORMAL, 1,1,1,1,0},
    { "data",		do_data,	0,POS_DEAD,	L7,LOG_NORMAL, 1,1,1,1,0},
    { "whodata",	do_whodata,	0,POS_DEAD,	L7,LOG_NORMAL, 1,1,1,1,0},
    { "hostlist",	do_hostlist,	0,POS_DEAD,	L3,LOG_NORMAL, 1,1,1,1,0},
    { "trust",          do_trust,       0,POS_DEAD,     ML,LOG_ALWAYS, 1,1,1,1,0},
    { "violate",        do_violate,     0,POS_DEAD,     ML,LOG_NORMAL, 1,1,1,1,0},
    { "alinks",		do_alinks,	0,POS_DEAD,	L3,LOG_NORMAL, 1,1,1,1,0},
    { "permexdesc",	do_permexdesc,	0,POS_DEAD,	L4,LOG_NORMAL, 1,1,1,1,0},
    { "showmem",	do_showmem,	0,POS_DEAD,	L6,LOG_NORMAL, 1,1,1,1,0},
    { "showtracks",	do_showtracks,	0,POS_DEAD,	L5,LOG_NORMAL, 1,1,1,1,0},
    { "allow",          do_allow,       0,POS_DEAD,     L1,LOG_ALWAYS, 1,1,1,1,0},
    { "astrip",         do_astrip,      0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "deny",           do_deny,        0,POS_DEAD,     L2,LOG_ALWAYS, 1,1,1,1,0},
    { "undeny",		do_undeny,	0,POS_DEAD,	L2,LOG_ALWAYS, 1,1,1,1,0},
    { "disconnect",     do_disconnect,  0,POS_DEAD,     L2,LOG_ALWAYS, 1,1,1,1,0},
    { "flag",           do_flag,        0,POS_DEAD,     L4,LOG_NORMAL, 1,1,1,1,0},
    { "freeze",         do_freeze,      0,POS_DEAD,     L4,LOG_ALWAYS, 1,1,1,1,0},
    { "permban",        do_permban,     0,POS_DEAD,     L1,LOG_ALWAYS, 1,1,1,1,0},
    { "emitstats",      do_emitstats,   0,POS_DEAD,     ML,LOG_ALWAYS, 1,1,1,1,0},
    { "reboo",          do_reboo,       0,POS_DEAD,     ML,LOG_NORMAL, 0,1,1,1,0},
    { "reboot",         do_reboot,      0,POS_DEAD,     ML,LOG_ALWAYS, 1,1,1,1,0},
    { "reb00t",         do_reboot,      0,POS_DEAD,     ML,LOG_ALWAYS, 1,1,1,1,0},
    { "causesegv",      do_causesegv,   0,POS_DEAD,     ML,LOG_ALWAYS, 1,1,1,1,0},
    { "set",            do_set,         0,POS_DEAD,     L4,LOG_NORMAL, 1,1,1,1,0},
    { "shutdow",        do_shutdow,     0,POS_DEAD,     L1,LOG_NORMAL, 0,1,1,1,0},
    { "shutdown",       do_shutdown,    0,POS_DEAD,     ML,LOG_ALWAYS, 1,1,1,1,0},
//  { "sockets",        do_sockets,     0,POS_DEAD,     L4,LOG_NORMAL, 1,0,1,1,0},
    { "wizlock",        do_wizlock,     0,POS_DEAD,     L1,LOG_ALWAYS, 1,1,1,1,0},

//  { "force",          do_force,       0,POS_DEAD,     L4,LOG_NORMAL, 1,1,1,1,0},
    { "fwap",           do_fwap,        0,POS_DEAD,     ML,LOG_NORMAL, 1,1,1,1,0},
    { "load",           do_load,        0,POS_DEAD,     L4,LOG_NORMAL, 1,1,1,1,0},
    { "newlock",        do_newlock,     0,POS_DEAD,     L1,LOG_ALWAYS, 1,1,1,1,0},
    { "nochannels",     do_nochannels,  0,POS_DEAD,     L2,LOG_ALWAYS, 1,1,1,1,0},
    { "noemote",        do_noemote,     0,POS_DEAD,     L2,LOG_ALWAYS, 1,1,1,1,0},
    { "noshout",        do_noshout,     0,POS_DEAD,     L2,LOG_ALWAYS, 1,1,1,1,0},
    { "notell",         do_notell,      0,POS_DEAD,     L2,LOG_ALWAYS, 1,1,1,1,0},
    { "noooc",          do_noooc,       0,POS_DEAD,     L3,LOG_ALWAYS, 1,1,1,1,0},
    { "pecho",          do_pecho,       0,POS_DEAD,     L7,LOG_NORMAL, 1,1,1,1,0}, 
    { "pardon",         do_pardon,      0,POS_DEAD,     ML,LOG_ALWAYS, 1,1,1,1,0},
    { "purge",          do_purge,       0,POS_DEAD,     L4,LOG_NORMAL, 1,1,1,1,0},
    { "restore",        do_restore,     0,POS_DEAD,     L5,LOG_NORMAL, 1,1,1,1,0},
    { "sla",            do_sla,         0,POS_DEAD,     L3,LOG_NORMAL, 0,1,1,1,0},
    { "slay",           do_slay,        0,POS_DEAD,     L4,LOG_ALWAYS, 1,1,1,1,0},
    { "rename",         do_rename,      0,POS_DEAD,     L7,LOG_ALWAYS, 1,1,1,1,0},
    { "smit",           do_smi,         0,POS_DEAD,     L3,LOG_NORMAL, 0,1,1,1,0},
    { "smite",          do_smite,       0,POS_DEAD,     L4,LOG_ALWAYS, 1,1,1,1,0},
    { "teleport",       do_transfer,    0,POS_DEAD,     L7,LOG_NORMAL, 1,1,1,1,0}, 
    { "transfer",       do_transfer,    0,POS_DEAD,     L7,LOG_NORMAL, 1,1,1,1,0},

//  { "at",             do_at,          0,POS_DEAD,     L6,LOG_NORMAL, 1,0,1,1,0},
    { "poofin",         do_bamfin,      0,POS_DEAD,     IM,LOG_NORMAL, 1,1,1,1,0},
    { "poofout",        do_bamfout,     0,POS_DEAD,     IM,LOG_NORMAL, 1,1,1,1,0},
    { "gecho",          do_echo,        0,POS_DEAD,     L5,LOG_NORMAL, 1,1,1,1,0},
    { "lgecho",         do_lgecho,      0,POS_DEAD,     L5,LOG_NORMAL, 1,1,1,1,0},
    { "agecho",         do_agecho,      0,POS_DEAD,     L5,LOG_NORMAL, 1,1,1,1,0},
//  { "goto",           do_goto,        0,POS_DEAD,     L8,LOG_NORMAL, 1,0,1,1,0},
    { "groupadd", 	do_wizgroupadd, 0,POS_DEAD,	L4,LOG_NORMAL, 1,0,1,1,0},
    { "nopk",		do_nopk,	0,POS_DEAD,	IM,LOG_NORMAL, 1,1,1,1,0},
    { "showdam",        do_showdam,     0,POS_DEAD,     IM,LOG_NORMAL, 1,1,1,1,0},
    { "holylight",      do_holylight,   0,POS_DEAD,     IM,LOG_NORMAL, 1,1,1,1,0},
    { "incognito",      do_incognito,   0,POS_DEAD,     IM,LOG_NORMAL, 1,1,1,1,0},
    { "invis",          do_invis,       0,POS_DEAD,     IM,LOG_NORMAL, 0,1,1,1,0},
    { "slog",		do_slog,	0,POS_DEAD,	IM,LOG_ALWAYS, 1,1,1,1,0},
    { "log",            do_log,         0,POS_DEAD,     ML,LOG_ALWAYS, 1,1,1,1,0},
    { "mobcount",       do_mobcount,    0,POS_DEAD,     L3,LOG_NORMAL, 1,1,1,1,0},
    { "memory",         do_memory,      0,POS_DEAD,     L3,LOG_NORMAL, 1,1,1,1,0},
    { "mwhere",         do_mwhere,      0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "nolag",		do_nolag,	0,POS_DEAD,	IM,LOG_NORMAL, 1,1,1,1,0},
    { "objcount",	do_objcount,	0,POS_DEAD,	L3,LOG_NORMAL, 1,1,1,1,0},
    { "owhere",         do_owhere,      0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "olimit",		do_olimit,	0,POS_DEAD,	L3,LOG_NORMAL, 1,1,1,1,0},
    { "peace",          do_peace,       0,POS_DEAD,     L7,LOG_NORMAL, 1,1,1,1,0},
    { "penalty",        do_penalty,     0,POS_DEAD,     ML,LOG_NORMAL, 1,1,1,1,0},
    { "echo",           do_recho,       0,POS_DEAD,     L7,LOG_NORMAL, 1,1,1,1,0},
    { "return",         do_return,      0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "relinquish",     do_relinquish,  0,POS_STANDING, 0, LOG_NORMAL, 1,1,1,1,0},
    { "revert",		do_revert,	0,POS_FIGHTING,	0, LOG_NORMAL, 1,0,1,1,0},    
    { "csnoop",		do_csnoop,	0,POS_DEAD,	L6,LOG_NORMAL, 1,1,1,1,0},
    { "snoop",          do_snoop,       0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "bsnoop",		do_bsnoop,	0,POS_DEAD,	ML,LOG_NORMAL, 1,1,1,1,0},
    { "showbit",        do_showbit,     0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "flipbit",        do_flipbit,     0,POS_DEAD,     L5,LOG_NORMAL, 1,1,1,1,0},
    { "snoopprotect",   do_snoopprotect,0,POS_DEAD,     L1,LOG_ALWAYS, 1,1,1,1,0},
    { "fsave",		do_fsave,	0,POS_DEAD,	L7,LOG_NORMAL, 1,1,1,1,0},
    { "biaslist",	do_biaslist,	0,POS_DEAD,	IM,LOG_NORMAL, 1,1,1,1,0},
    { "version",	do_version,	0,POS_DEAD,	IM,LOG_NORMAL, 1,1,1,1,0},
	{ "showlimits", do_showlimits,  0,POS_DEAD, IM,LOG_NORMAL, 1,1,1,1,0},
    { "stat",           do_stat,        0,POS_DEAD,     IM,LOG_NORMAL, 1,1,1,1,0},
    { "fstat",          do_fstat,       0,POS_DEAD,     L5,LOG_NORMAL, 1,1,1,1,0},
    { "wstat",		do_wstat,	0,POS_DEAD,	L5,LOG_NORMAL, 1,1,1,1,0},
    { "string",         do_string,      0,POS_DEAD,     L5,LOG_NORMAL, 1,1,1,1,0},
    { "switch",         do_switch,      0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "wizinvis",       do_invis,       0,POS_DEAD,     IM,LOG_NORMAL, 1,1,1,1,0},
    { "vnum",           do_vnum,        0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "zecho",          do_zecho,       0,POS_DEAD,     L7,LOG_NORMAL, 1,1,1,1,0},
    { "badscan",	do_badscan,	0,POS_DEAD,	L7,LOG_NORMAL, 1,1,1,1,0},
    { "approve",	do_approve,	0,POS_DEAD,	L7,LOG_NORMAL, 1,1,1,1,0},

    { "clone",          do_clone,       0,POS_DEAD,     L4,LOG_NORMAL, 1,1,1,1,0},

    { "wiznet",         do_wiznet,      0,POS_DEAD,     IM,LOG_NORMAL, 1,1,1,1,0},
    { "immtalk",        do_immtalk,     0,POS_DEAD,     IM,LOG_NORMAL, 1,1,1,1,1},
    { "wiztalk",        do_wiztalk,     0,POS_DEAD,     L7,LOG_NORMAL, 1,1,1,1,1},
    { "imotd",          do_imotd,       0,POS_DEAD,     IM,LOG_NORMAL, 1,1,1,1,0},
    { "imptalk",        do_imptalk,     0,POS_DEAD,     ML,LOG_NORMAL, 0,1,1,1,1},
    { "smote",          do_smote,       0,POS_DEAD,     0, LOG_NORMAL, 1,1,0,1,1},
    { "prefi",          do_prefi,       0,POS_DEAD,     IM,LOG_NORMAL, 0,1,1,1,0},
    { "prefix",         do_prefix,      0,POS_DEAD,     IM,LOG_NORMAL, 1,1,1,1,0},
    { "unbrand",	do_unbrand,	0,POS_DEAD,	L6,LOG_ALWAYS, 1,1,1,1,0},
    { "mrefresh",	do_mrefresh,	0,POS_DEAD,	L1,LOG_NORMAL, 1,1,1,1,0},
    { "castalign",	do_castalign,	0,POS_DEAD,	L4,LOG_NORMAL, 1,1,1,1,0},
    { "checkfollow",	do_checkfollow,	0,POS_DEAD,	IM,LOG_NORMAL, 1,1,1,1,0},
    { "trait",		do_trait,	0,POS_DEAD,	L5,LOG_NORMAL, 1,1,1,1,0},
    { "test",		do_test,	0,POS_DEAD,	ML,LOG_NORMAL, 1,1,1,1,0},
    { "vsearch",	do_vsearch,	0,POS_DEAD,	L6,LOG_NORMAL, 1,1,1,1,0},
    { "debug",		do_debug,	0,POS_DEAD,	L3,LOG_NORMAL, 1,1,1,1,0},

    
    /* stuff moved back because of conflicts */
    { "translate",	do_translate,	0,POS_DEAD,	0, LOG_NORMAL, 1,1,1,1,0},
    { "trophy",		do_trophy,	0,POS_STANDING,	0, LOG_NORMAL, 1,0,0,0,0},
    { "setambush",      do_setambush,   0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "setsnare",       do_setsnare,    0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,0,0},
    { "sleeve",         do_sleeve,      0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,1,0},
    { "bleed",		do_bleed,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,1,0},
    { "news",           do_news,     	0,POS_DEAD,     0, LOG_NORMAL, 1,1,1,1,0},
    { "oath",       do_oath,    0,POS_DEAD, 0, LOG_NORMAL, 1,1,1,1,0},

    /*
     * OLC
     */
    { "edit",           do_olc,         0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "asave",          do_asave,       0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "alist",          do_alist,       0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "vnumlist",       do_vnumlist,    0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "resets",         do_resets,      0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "redit",          do_redit,       0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "medit",          do_medit,       0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "aedit",          do_aedit,       0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "oedit",          do_oedit,       0,POS_DEAD,     L6,LOG_NORMAL, 1,1,1,1,0},
    { "hedit",		do_hedit,	0,POS_DEAD,	L6,LOG_NORMAL, 1,1,1,1,0},
    { "fedit",		do_fedit,	0,POS_DEAD,	L6,LOG_NORMAL, 1,1,1,1,0},
    { "rlist",		do_rlist,	0,POS_DEAD,	L6,LOG_NORMAL, 1,1,1,1,0},
    { "mlist",		do_mlist,	0,POS_DEAD,	L6,LOG_NORMAL, 1,1,1,1,0},
    { "olist",		do_olist,	0,POS_DEAD,	L6,LOG_NORMAL, 1,1,1,1,0},
    { "flist",		do_flist,	0,POS_DEAD,	L6,LOG_NORMAL, 1,1,1,1,0},
    { "darkinsight",    do_dark_insight,0,POS_STANDING,  0, LOG_NORMAL, 1,1,0,1,0},
    { "demonicmight",	do_demonic_might,0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,1,0},
    { "demonicfocus",   do_demonic_focus,0,POS_STANDING, 0, LOG_NORMAL, 1,1,0,1,0},
    { "immtime",        do_immtime,	0,POS_DEAD, 	L2,LOG_NORMAL, 1,1,1,1,0},
    { "charge",	 	do_charge,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "rush",	 	do_rush,	0,POS_STANDING, 0, LOG_NORMAL, 1,0,0,0,0},
    { "thunderstrike", 	do_thunderstrike,0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,0,0},    
    { "shadowstrike", 	do_shadowstrike,0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,0,0},    
    { "meditativestrike", do_medstrike,	0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,0,0},    //moved here because it was fucking up my chi
    { "froststrike", 	do_froststrike,0,POS_FIGHTING, 0, LOG_NORMAL, 1,0,0,0,0},    
    { "rumors",         do_rumors,	0,POS_DEAD,	L2, LOG_NORMAL, 1,1,1,1,0},
    { "rumormod",       do_rumormod,	0,POS_DEAD,	L2, LOG_NORMAL, 1,1,1,1,0},
    { "rumoradd",       do_rumoradd,	0,POS_DEAD,	L2, LOG_NORMAL, 1,1,1,1,0},
    { "mprumor",	0,prog_rumor,	  POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "mpaddrumor",	0,prog_addrumor,  POS_DEAD,	0, LOG_NORMAL, 0,1,1,1,0},
    { "listherbs",        do_listherbs,	0,POS_DEAD, 	L2,LOG_NORMAL, 1,1,1,1,0},
    
    /*
     * End of list.
     */
    { "",               0,              0,POS_DEAD,        0,  LOG_NORMAL, 0, 0, 0, 0, 0}
};

static const CommandTrie & BuildCommandTrie()
{
    static CommandTrie commands;
    for (int cmd(0); cmd_table[cmd].name[0] != '\0'; ++cmd)
    {
        if (!commands.Add(cmd_table[cmd].name, cmd, cmd_table[cmd].level))
            bug("Failed to add command to trie: %d", cmd);
    }
    return commands;
}

static int LookupCommand(const char * command, int trust)
{
    static const CommandTrie & commands(BuildCommandTrie());
    return commands.Lookup(command, trust);
}

/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
void interpret( CHAR_DATA *ch, char *argument )
{
    char command[MAX_INPUT_LENGTH];
    char logline[MAX_INPUT_LENGTH * 2];
    char wordline[MAX_INPUT_LENGTH];
    int cmd;
    int trust = get_trust(ch);
    bool found = FALSE;
    int x;

    /*
     * Strip leading spaces.
     */
    while ( isspace(*argument) )
	argument++;
    if ( argument[0] == '\0' )
	return;

    strcpy(segv_cmd, argument);
    strcpy(segv_char, ch->name);

    /*
     * Implement freeze command.
     */
    if (!IS_NPC(ch) && (ch->level < 60)
      && IS_SET(ch->act, PLR_FREEZE))
    {
        if (IS_NAFFECTED(ch, AFF_FLESHTOSTONE))
    	    send_to_char( "You're encased in stone!\n\r", ch );
	else if (is_affected(ch, gsn_bindingtentacle))
	    send_to_char( "You are bound too tightly to move.\n\r", ch);
        else
	    send_to_char( "You're totally frozen!\n\r", ch );

	return;
    }
	
	strcpy(logline, argument);

    /*
     * Grab the command word.
     * Special parsing so ' can be a command,
     *   also no spaces needed after punctuation.
     */
    if (!isalpha(argument[0]) && !isdigit(argument[0]))
    {
		switch (argument[0])
		{
	    	case '\'': strcpy(command, "say"); break;
		    case ',': strcpy(command, "emote"); break;
		    case ';': strcpy(command, "gtell"); break;
	    	case '.': strcpy(command, "ht"); break;
		    case '/': strcpy(command, "impta"); break;
		    case ':': strcpy(command, "immta"); break;
	    	case '?': strcpy(command, "impta"); break;
		    default: return; break;
		}

		argument++;
		while (isspace(*argument))
	    	argument++;
    }
    else
		argument = one_argument( argument, command );

    // Perform command lookup
    cmd = LookupCommand(command, trust);
    found = (cmd >= 0);

#ifdef HERODAY
    if (found
    && !IS_NPC(ch)
    && IS_SET(ch->in_room->room_flags,ROOM_ARENA)
    && !BIT_GET(ch->pcdata->bitptr,18653)
    && !IS_IMMORTAL(ch))
        if(!(cmd_table[cmd].do_fun == do_north
	|| cmd_table[cmd].do_fun == do_south
	|| cmd_table[cmd].do_fun == do_east
	|| cmd_table[cmd].do_fun == do_west
	|| cmd_table[cmd].do_fun == do_up
	|| cmd_table[cmd].do_fun == do_down
	|| cmd_table[cmd].do_fun == do_look
	|| cmd_table[cmd].do_fun == do_follow
	|| cmd_table[cmd].do_fun == do_where
	|| cmd_table[cmd].do_fun == do_newwho)
	)
    {
        send_to_char("You can't do that while spectating.\n\r",ch);
	return;
    }
#endif

    if (found && ch->mount && !cmd_table[cmd].canmount)
    {
	send_to_char("You cannot do that while mounted.\n\r", ch);
	return;
    }
// brazen: This is a lot of spacing.
if (found && cmd_table[cmd].position != POS_DEAD)
{

    if (IS_NPC(ch) && (ch->pIndexData->vnum == MOB_VNUM_EYE_RUNE))
    {
	CHAR_DATA *vch = ch->desc->original;

	send_to_char("You return your attentions to your physical surroundings.\n\r", ch);
	ch->desc->character 	= ch->desc->original;
	ch->desc->original  	= NULL;
	ch->desc->character->desc	= ch->desc;
	ch->desc			= NULL;
	extract_char(ch, TRUE);
	REMOVE_BIT(vch->oaffected_by, AFF_EYEFOCUS);
	interpret(vch, argument);
	return;
    }
    
    if (IS_OAFFECTED(ch, AFF_CONSUMPTION) && found && (cmd_table[cmd].info == 0))
    {
	send_to_char("You cannot act while performing the ritual of the silver veil.\n\r", ch);
	return;
    }

    if (IS_OAFFECTED(ch, AFF_INSCRIBE) && found && (cmd_table[cmd].info == 0))
    {
	send_to_char("You are too busy inscribing a symbol of power to do that.\n\r", ch);
	return;
    }
	
	if (ch->mercy_from != 0 && found && cmd_table[cmd].info == 0 && cmd_table[cmd].comm == 0)
	
	    // Try parsing the command as a social; if that fails, tell them they can't perform the action
	    // In either case, block any further action
	    // brazen: unless they are a ghost.
	    if (!IS_OAFFECTED(ch, AFF_GHOST))
	    {
	        if (!check_social(ch, command, argument))
	    	    act("You cannot do that with $N poised above you.", ch, NULL, ch->mercy_from, TO_CHAR);
		return;
	    }
	

    if (IS_OAFFECTED(ch, AFF_SHADOWMASTERY))
        REMOVE_BIT(ch->oaffected_by, AFF_SHADOWMASTERY);

    if (IS_OAFFECTED(ch, AFF_NOVA_CHARGE))
    {
	if (number_range(1,10) == 1)
	{
            int ndam;
            AFFECT_DATA *vaf;

            send_to_char("You lose control of the charging nova, and its destructive power consumes you!\n\r", ch);
            act("$n loses control of the charging nova, and its destructive power consumes $m!", ch, NULL, NULL, TO_ROOM);

            for (vaf = ch->affected; vaf; vaf = vaf->next)
                if (vaf->type == gsn_nova)
            	    break;

            ndam = dice(ch->level, 5) * UMAX(1, vaf->modifier - vaf->duration);
            affect_strip(ch, gsn_nova);

            damage_old(ch, ch, ndam, gsn_nova, DAM_FIRE, TRUE);

            if (!ch || IS_OAFFECTED(ch, AFF_GHOST))
                return;
	}
	else
	{
	    send_to_char("You break your concentration, and the power of the nova fades away.\n\r", ch);
	    affect_strip(ch, gsn_nova);
	}
    }

    if (is_affected(ch, gsn_delayreaction))
    {
	    if (number_percent() < 10)
	    {
		    act("$n's attention drifts from $s reaction, and it begins to boil out of control.", 
				ch, NULL, NULL, TO_ROOM);
		    act("Your attention drifts from your reaction, and it begins to boil out of control.",
				ch, NULL, NULL, TO_CHAR);
		    explosive_alchemy(ch, ch->level / 5);
	    }
	    else
	    {
		    act("$n hastily cools $s reaction, abandoning work on it.", ch, NULL, NULL, TO_ROOM);
		    act("You hastily cool your reaction, abandoning work on it.", ch, NULL, NULL, TO_CHAR);
	    }
	    affect_strip(ch, gsn_delayreaction);
    }

    if (is_affected(ch, gsn_meldwithstone))
    {
	act("You grow restless in the earth, and rise up from the ground.", ch, NULL, NULL, TO_CHAR);
	act("With a low rumble, the ground seems to part as $n flows up from it and reforms.", ch, NULL, NULL, TO_ROOM);
	affect_strip(ch, gsn_meldwithstone);
    }

    if (is_affected(ch, gsn_flameunity))
    {
        affect_strip(ch, gsn_flameunity);
        act("You stir, recorporealizing from the flames.", ch, NULL, NULL, TO_CHAR);
        act("$n emerges from the flames, unharmed despite the ash which swirls about $m.", ch, NULL, NULL, TO_ROOM);
    }

    if (is_affected(ch, gsn_airrune) || is_affected(ch, gsn_runeofair))
    {
        send_to_char("You disturb the air rune, and shimmer back into view.\n\r", ch);
        act("$n shimmers into view.", ch, NULL, NULL, TO_ROOM);
        affect_strip(ch, gsn_airrune);
        affect_strip(ch, gsn_runeofair);
    }

    if (IS_OAFFECTED(ch, AFF_COVEN) && found && (cmd_table[cmd].info == 0))
    {
	send_to_char("You cease chanting.\n\r", ch);
        act("$n ceases $s chanting.", ch, NULL, NULL, TO_ROOM);
	affect_strip(ch, gsn_coven);
    }

    if (is_affected(ch, gsn_enslave) && !IS_NPC(ch) && found && (cmd_table[cmd].info == 0))
    {
	CHAR_DATA *vch, *vch_next;
	
	send_to_char("You stop concentrating on enslaving your victim.\n\r", ch);
	affect_strip(ch, gsn_enslave);
	
	for (vch = char_list; vch; vch = vch_next)
	{
	    vch_next = vch->next;

	    if (IS_NPC(vch)
	     && is_affected(vch, gsn_enslave)
	     && !IS_AFFECTED(vch, AFF_CHARM)
	     && (get_modifier(vch->affected, gsn_enslave) == ch->id))
	    {
		act("$N roars in fury and attacks you!", ch, NULL, vch, TO_CHAR);
		act("$N roars in fury and attacks $n!", ch, NULL, vch, TO_NOTVICT);
		act("You roar in fury and attack $n!", ch, NULL, vch, TO_VICT);
		affect_strip(vch, gsn_enslave);	
		multi_hit(vch, ch, TYPE_UNDEFINED);
	    }
	}
    }
	    
    if (is_affected(ch, gsn_listen) && found 
      && cmd_table[cmd].do_fun != do_shadowmastery && cmd_table[cmd].info == 0)
    {
	send_to_char("You stop listening.\n\r", ch);
	affect_strip(ch, gsn_listen);
    }
}
// end of funky spacing
    if (is_affected(ch, gsn_cloudkill) && IS_AFFECTED(ch,AFF_POISON)
      && (number_bits(3) == 0)
      && found && (cmd_table[cmd].info == 0))
    {
	send_to_char("You cough and choke, as dry heaves clutch your body.\n\r", ch);
	act("$n is taken by a coughing fit, dry heaves clutching $s body.", ch, NULL, NULL, TO_ROOM);
	WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	return;
    }

    if (is_affected(ch, gsn_pox) && (number_bits(4) == 0)
     && found && (cmd_table[cmd].info == 0))
    {
	send_to_char("You writhe in agony as the pox devours you from the inside.\n\r", ch);
	act("$n writhes in agony as the pox devours $m from the inside.", ch, NULL, NULL, TO_ROOM);
	WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	return;
    }

    if (IS_OAFFECTED(ch, AFF_UNCLEANSPIRIT) && (number_range(1, 5) == 1)
     && found && (cmd_table[cmd].info == 0))
    {
	send_to_char("The dark presence inside you holds your body at bay.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	return;
    }

    if (is_affected(ch, gsn_vertigo) && (number_bits(3) == 0)
     && found && (cmd_table[cmd].info == 0))
    {
	send_to_char("You reel as your vertigo overwhelms you.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	return;
    } 

    if (IS_OAFFECTED(ch, AFF_PETRIFY) && (number_percent() < 15)
     && found && (cmd_table[cmd].info == 0))
    {
        send_to_char("Your muscles fail to respond as the petrification takes its toll.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	return;
    }

    if (IS_NAFFECTED(ch, AFF_PURSUE) && found && (cmd_table[cmd].info == 0))	
    {
	send_to_char("You stop your pursuit.\n\r", ch);
	affect_strip(ch, gsn_pursue);
	ch->tracking = NULL;
    }
		
    if (IS_NAFFECTED(ch, AFF_GARROTE_VICT) && found && (cmd_table[cmd].info == 0))
    {
        send_to_char("You struggle to get away from the garrote, but can't do anything!\n\r", ch);
        return;
    }

    if (IS_NAFFECTED(ch, AFF_GARROTE_CH) && found && (cmd_table[cmd].info == 0)
      && cmd_table[cmd].do_fun != do_say && cmd_table[cmd].do_fun != do_esay)
    {
        CHAR_DATA *gch;
        AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
        if (!ch->in_room)
	    return;
	
        free_string(ch->pose);
        gch = (CHAR_DATA *) get_aff_point(ch->affected, gsn_garrote);

	if (gch)
	{
	    if (IS_NAFFECTED(gch, AFF_GARROTE_VICT)) 
	    {
		act("$n lets the garrote slip.", ch, NULL, NULL, TO_ROOM);
		act("You let the garrote slip.", ch, NULL, NULL, TO_CHAR);
		REMOVE_BIT(gch->naffected_by, AFF_GARROTE_VICT);
	    }

	    free_string(gch->pose);

	    REMOVE_BIT(ch->naffected_by, AFF_GARROTE_CH);
	    affect_strip(ch, gsn_garrote);
	    af.where        = TO_AFFECTS;
	    af.type         = gsn_garrote;
	    af.level        = ch->level;
	    af.duration     = 6;
            af.location     = 0;
            af.modifier     = 0;
            af.bitvector    = 0;
            affect_to_char(ch, &af);
	}
    }

    if (IS_NAFFECTED(ch, AFF_SUBMISSION_VICT) && found 
      && (cmd_table[cmd].info == 0) 
      && cmd_table[cmd].do_fun != do_say
      && cmd_table[cmd].do_fun != do_esay)
    {
	send_to_char("You struggle to escape the submission hold, but can't do anything!",ch);
	return;
    }

    if (IS_NAFFECTED(ch, AFF_SUBMISSION_CH) && found 
      && (cmd_table[cmd].info == 0) 
      && cmd_table[cmd].do_fun != do_say
      && cmd_table[cmd].do_fun != do_esay)
    {
        CHAR_DATA *gch;
        AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
        if (!ch->in_room)
	    return;
        for (gch = ch->in_room->people;gch != NULL;gch = gch->next_in_room)
        {
	    if (IS_NAFFECTED(gch, AFF_SUBMISSION_VICT))
	    {
	        act("$n lets the submission hold go.", ch, NULL, NULL, TO_ROOM);
	  	act("You let the submission hold go.", ch, NULL, NULL, TO_CHAR);
	  	REMOVE_BIT(gch->naffected_by, AFF_SUBMISSION_VICT);
	  	break;
	    }
        }
        free_string(ch->pose);
        if (gch)
            free_string(gch->pose);

        REMOVE_BIT(ch->naffected_by, AFF_SUBMISSION_CH);
        affect_strip(ch, gsn_garrote);
        af.where        = TO_NAFFECTS;
        af.type         = gsn_submissionhold;
        af.level        = ch->level;
        af.duration     = 6;
        af.location     = 0;
        af.modifier     = 0;
        af.bitvector    = 0;
        affect_to_char(ch, &af);
    }
    

    if ((is_affected(ch, gsn_possession) || (IS_NPC(ch) && (ch->pIndexData->vnum == MOB_VNUM_SHUNNED_DEMON)))
     && command[0] == 'm' && command[1] == 'p')
	return;

    /*
     * verb prog. R0x0r out. Check for verbs before interpreting normally,
     * so builder defined verbs can beat out commands (ah, or nullify them...?
     * "Do not use such tactics in the sacred house"? heh)
     */
    
    if (ch->position > POS_SLEEPING && (command[0] != 'm' || command[1] != 'p') && str_cmp(command, "play")) 
    {
		const char * subbed_command(0);
		const char * subbed_text(0);

		// Replace the word they actually typed with the full command, if found
		if (found)
		{
			// Create the substitued parameters
			strcpy(wordline, cmd_table[cmd].name);
        	strcat(wordline, " ");
	        strcat(wordline, argument);

			// Set the strings
			subbed_command = cmd_table[cmd].name;
			subbed_text = wordline;
		}

		// Rebuild the command now that dot substitution is out of the way
		char text[MAX_INPUT_LENGTH * 2];
		strcpy(text, command);
		strcat(text, " ");
		strcat(text, argument);
		
		// Check the verbs
        if (check_verbs(ch, command, subbed_command, text, subbed_text))
			return;
    }
	    
    if (found && cmd_table[cmd].canhide == 0 && cmd > 5)
	unhide_char(ch);

    if (found && cmd < 6 && !IS_AFFECTED(ch, AFF_SNEAK))
	unhide_char(ch);	

    /*
     * Log and snoop.
     */
    if (found && cmd_table[cmd].log == LOG_NEVER )
	strcpy( logline, "" );

    bool log_command_to_wiznet((!IS_NPC(ch) && IS_SET(ch->act, PLR_LOG)) || fLogAll || (found && cmd_table[cmd].log == LOG_ALWAYS));
    bool log_command(log_command_to_wiznet || IS_IMM_TRUST(ch) || (ch->desc && ch->desc->original && IS_IMM_TRUST(ch->desc->original)));

    if (log_command)
    {
	if (ch->level > LEVEL_IMMORTAL 
	  || (ch->desc && ch->desc->original && ch->desc->original->level > LEVEL_IMMORTAL)) 
	{
	    sprintf(log_buf, "Log ImmInput %s: %s", ch->name, logline);
	    if (log_command_to_wiznet)
 	        wiznet(fix_crap(log_buf),ch,NULL,WIZ_SECURE,0,get_trust(ch));
	    log_string(log_buf);
	}
	
    }

    if (ch->desc != NULL && ch->desc->snooped)
    {
	for (x = 0; x < MAX_SNOOP; x++)
        {
            if (!ch->desc->snoop_by[x] || ch->desc->snoop_type[x] == SNOOP_COMM)
		continue;

	    write_to_buffer( ch->desc->snoop_by[x], "% ",    2 );
	    write_to_buffer( ch->desc->snoop_by[x], logline, 0 );
	    write_to_buffer( ch->desc->snoop_by[x], "\n\r",  2 );
        }
    }

    if ( !found )
    {
        // Look for command in socials table.
        if ( !check_social( ch, command, argument ) )
            send_to_char( "Huh?\n\r", ch);
        return;
    }

    /*
     * Character not in position for command?
     */
    if ( ch->position < cmd_table[cmd].position )
    {
	switch( ch->position )
	{
	case POS_DEAD:
	    send_to_char( "Lie still; you are DEAD.\n\r", ch );
	    break;

	case POS_MORTAL:
	case POS_INCAP:
	    send_to_char( "You are hurt far too bad for that.\n\r", ch );
	    break;

	case POS_STUNNED:
	    send_to_char( "You are too stunned to do that.\n\r", ch );
	    break;

	case POS_SLEEPING:
	    send_to_char( "In your dreams, or what?\n\r", ch );
	    break;

	case POS_RESTING:
	    send_to_char( "Nah... You feel too relaxed...\n\r", ch);
	    break;

	case POS_SITTING:
	    send_to_char( "Better stand up first.\n\r",ch);
	    break;

	case POS_FIGHTING:
	    send_to_char( "No way!  You are still fighting!\n\r", ch);
	    break;

	}
	return;
    }

    /*
     * Dispatch the command.
     */
    if (is_affected( ch, gsn_painful_thoughts))
    {
	ch->hit -= 1;
	send_to_char ("You cry a little as your mind is distracted by the music.\n\r", ch);
	act("$n cries a little as $e listens to an echo of a song.", ch, NULL,NULL, TO_ROOM);
    }	

    if (cmd_table[cmd].do_fun != 0)
      // TODO(abrahms): I'm considering adding YET ANOTHER CONDITIONAL
      // to check if the command is a known command in the "new
      // syntax" where it'll wrap the ch in a Player class.
      if (cmd_table[cmd].obj_do_fun) {
        Player player(ch);
        (*cmd_table[cmd].obj_do_fun) ( &player, argument );
      } else {
        (*cmd_table[cmd].do_fun) ( ch, argument );
      }
    else if (cmd_table[cmd].prog_fun != 0)
    {
	if ((IS_NPC(ch) && (!ch->desc)) || (ch->level > LEVEL_HERO))
            (*cmd_table[cmd].prog_fun) (ch, NULL, NULL, argument, MOB_PROG);
	else
	    send_to_char("Huh?\n\r", ch);
    }

    if (!IS_OAFFECTED(ch, AFF_SHADOWMASTERY) && is_affected(ch, gsn_shadowmastery)
      && cmd_table[cmd].do_fun != do_listen)
    {
	affect_strip(ch, gsn_shadowmastery);
	send_to_char("Your actions bring you out of the concealing shadows.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_shadowmastery].beats));
    }
}

/* This function can be called from a mob_prog to force a character
 * to "do" their original command without triggering a verb_prog again
 * Useful for instances where ifchecks occur within the confines of a prog,
 * that makes you not want to treat the verb prog as such.
 */
void proginterpret( CHAR_DATA *ch, char *argument )
{
    char command[MAX_INPUT_LENGTH];
    char logline[MAX_INPUT_LENGTH];
    int trust;
    bool found;
    int x;

    /*
     * Strip leading spaces.
     */
    while ( isspace(*argument) )
	argument++;
    if ( argument[0] == '\0' )
	return;

    /*
     * No hiding.
     */
    /* Um -- wrong. This needs to be done on a command by command basis.
     *  Set it up post-command grab. Extra interp.c cmd_table field
     * will determine if you hide or not.
     *
     *    REMOVE_BIT( ch->affected_by, AFF_HIDE );
     *
     */


	/*
	 * No listening if you won't keep still, and other goodies
	 */

	if (is_affected(ch, gsn_shadowmastery))
	{
	    affect_strip(ch, gsn_shadowmastery);
	    send_to_char("You slip out of the concealment of the shadows.\n\r", ch);
	    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	}

	if (is_affected(ch, gsn_meldwithstone))
		{
		act("You grow restless in the earth, and rise up from the ground.", ch, NULL, NULL, TO_CHAR);
		act("With a low rumble, the ground seems to part as $n flows up from it and reforms.", ch, NULL, NULL, TO_ROOM);
		affect_strip(ch, gsn_meldwithstone);
		}

    if (is_affected(ch, gsn_flameunity))
    {
        affect_strip(ch, gsn_flameunity);
        act("You stir, recorporealizing from the flames.", ch, NULL, NULL, TO_CHAR);
        act("$n emerges from the flames, unharmed despite the ash which swirls about $m.", ch, NULL, NULL, TO_ROOM);
    }

	if (is_affected(ch, gsn_airrune) || is_affected(ch, gsn_runeofair))
    {
		send_to_char("You disturb the air rune, and shimmer back into view.\n\r", ch);
        act("$n shimmers into view.", ch, NULL, NULL, TO_ROOM);
		affect_strip(ch, gsn_airrune);
        affect_strip(ch, gsn_runeofair);
    }

	if (is_affected(ch, gsn_listen))
	{
	    send_to_char("You stop listening.\n\r", ch);
	    affect_strip(ch, gsn_listen);
	}
	if (is_affected(ch, gsn_cloudkill) && IS_AFFECTED(ch,AFF_POISON)
	  && number_bits(3)==0)
	{
	     send_to_char("You cough and choke, as dry heaves clutch your body.\n\r", ch);
	     act("$n is taken my a coughing fit, dry heaves clutching $s body.", ch, NULL, NULL, TO_ROOM);
	     WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	     return;
	}
	if (is_affected(ch, gsn_pox) && number_bits(4)==0)
	{
	    send_to_char("You writhe in agony as the pox devours you from the inside.\n\r", ch);
	    act("$n writhes in agony as the pox devours $m from the inside.", ch, NULL, NULL, TO_ROOM);
	    WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	    return;
	}
	if (IS_NAFFECTED(ch, AFF_BOLO) && number_bits(2)==0)
		{
		  send_to_char("You stop, trying to untangle yourself from the bolo.\n\r", ch);
		  act("$n stops and tries to untangle $mself from the bolo.", ch, NULL, NULL, TO_ROOM);
		  WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
		  return;
		}
	if (IS_NAFFECTED(ch, AFF_PURSUE))	
		{
		  send_to_char("You stop your pursuit.\n\r", ch);
		  affect_strip(ch, gsn_pursue);
		  ch->tracking = NULL;
		}
		

    /*
     * Implement freeze command.
     */
    if (!IS_NPC(ch) && ch->level < 60)
      if (IS_SET(ch->act, PLR_FREEZE))
        if (IS_NAFFECTED(ch, AFF_FLESHTOSTONE))
    {
	send_to_char( "You're encased in stone!\n\r", ch );
	return;
    }
        else
    {
	send_to_char( "You're totally frozen!\n\r", ch );
	return;
    }

    if (IS_NAFFECTED(ch, AFF_GARROTE_VICT))
    {
       send_to_char("You struggle to get away from the garrote, but can't do anything!\n\r", ch);
       return;
    }

    if (IS_NAFFECTED(ch, AFF_SUBMISSION_VICT))
    {
         send_to_char("You struggle to escape the submission hold, but can't do anything!\n\r", ch);
         return;
    }

    if (IS_NAFFECTED(ch, AFF_SUBMISSION_CH))
    {
	CHAR_DATA *gch;
	AFFECT_DATA af;
	af.valid = TRUE;
	af.point = NULL;
	if (!ch->in_room)
	    return;
	for (gch = ch->in_room->people;gch != NULL;gch = gch->next_in_room)
	{
	    if (IS_NAFFECTED(gch, AFF_SUBMISSION_VICT))
	    {
		act("$n lets the submission hold go.", ch, NULL, NULL, TO_ROOM);
		act("You let the submission hold go.", ch, NULL, NULL, TO_CHAR);
		REMOVE_BIT(gch->naffected_by, AFF_SUBMISSION_VICT);
		affect_strip(gch, gsn_submissionhold);
		break;
	    }
	}
	REMOVE_BIT(ch->naffected_by, AFF_SUBMISSION_CH);
	affect_strip(ch, gsn_submissionhold);
        af.where        = TO_NAFFECTS;
        af.type         = gsn_submissionhold;
        af.level        = ch->level;
        af.duration     = 6;
        af.location     = 0;
        af.modifier     = 0;
        af.bitvector    = 0;
        affect_to_char(ch, &af);
    }

    if (IS_NAFFECTED(ch, AFF_GARROTE_CH))
    {
      CHAR_DATA *gch;
      AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
      if (!ch->in_room)
	return;
      for (gch = ch->in_room->people;gch != NULL;gch = gch->next_in_room)
      {
	if (IS_NAFFECTED(gch, AFF_GARROTE_VICT))
	{
	  act("$n lets the garrote slip.", ch, NULL, NULL, TO_ROOM);
	  act("You let the garrote slip.", ch, NULL, NULL, TO_CHAR);
	  REMOVE_BIT(gch->naffected_by, AFF_GARROTE_VICT);
	  break;
	}
      }
      REMOVE_BIT(ch->naffected_by, AFF_GARROTE_CH);
      affect_strip(ch, gsn_garrote);
        af.where        = TO_NAFFECTS;
        af.type         = gsn_garrote;
        af.level        = ch->level;
        af.duration     = 6;
        af.location     = 0;
        af.modifier     = 0;
        af.bitvector    = AFF_GARROTE;
        affect_to_char(ch, &af);
    }
    /*
     * Grab the command word.
     * Special parsing so ' can be a command,
     *   also no spaces needed after punctuation.
     */
    strcpy( logline, argument );
    if ( !isalpha(argument[0]) && !isdigit(argument[0]) )
    {
	switch (argument[0])
	{
	    case '\'': strcpy(command, "say"); break;
	    case ',': strcpy(command, "emote"); break;
	    case ';': strcpy(command, "gtell"); break;
	    case '.': strcpy(command, "ht"); break;
	    case '/': strcpy(command, "recall"); break;
	    case ':': strcpy(command, "immta"); break;
	    case '?': strcpy(command, "impta"); break;
	    default: return; break;
	}

	argument++;
	while ( isspace(*argument) )
	    argument++;
    }
    else
    {
	argument = one_argument( argument, command );
    }

    /* this is a cheap hack to block people from using mp commands while 
     * they're using possession
     */ 
    if (is_affected(ch, gsn_possession) && command[0] == 'm' && command[1] == 'p')
	return;

 	/* verb prog would go here. No verb progs in proginterpret */

    found = FALSE;
    trust = get_trust( ch );
    int cmd;
    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
    {
	if ( command[0] == cmd_table[cmd].name[0]
	&&   !str_prefix( command, cmd_table[cmd].name )
	&&   cmd_table[cmd].level <= trust )
	{
	    found = TRUE;
	    break;
	}
    }

	if (found && cmd_table[cmd].canhide == 0 && cmd > 5)
		unhide_char(ch);

	if (found && cmd < 6 && !IS_AFFECTED(ch, AFF_SNEAK))
		unhide_char(ch);	
		

    /*
     * Log and snoop.
     */
    if ( cmd_table[cmd].log == LOG_NEVER )
	strcpy( logline, "" );

    if ( ( !IS_NPC(ch) && IS_SET(ch->act, PLR_LOG) )
    ||   fLogAll
    ||   cmd_table[cmd].log == LOG_ALWAYS )
    {
	sprintf( log_buf, "Log %s: %s", ch->name, logline );
	wiznet(fix_crap(log_buf),ch,NULL,WIZ_SECURE,0,get_trust(ch));
	log_string( log_buf );
    }

    if (ch->desc != NULL && ch->desc->snooped)
    {
        for (x = 0; x < MAX_SNOOP; x++)
        {
            if (!ch->desc->snoop_by[x] || ch->desc->snoop_type[x] == SNOOP_COMM)
	        continue;

	    write_to_buffer( ch->desc->snoop_by[x], "% ",    2 );
	    write_to_buffer( ch->desc->snoop_by[x], logline, 0 );
	    write_to_buffer( ch->desc->snoop_by[x], "\n\r",  2 );
        }
    }

    if ( !found )
    {
	/*
	 * Look for command in socials table.
	 */
	if ( !check_social( ch, command, argument ) )
	    send_to_char( "Huh?\n\r", ch);
	return;
    }

    /*
     * Character not in position for command?
     */
    if ( ch->position < cmd_table[cmd].position )
    {
	switch( ch->position )
	{
	case POS_DEAD:
	    send_to_char( "Lie still; you are DEAD.\n\r", ch );
	    break;

	case POS_MORTAL:
	case POS_INCAP:
	    send_to_char( "You are hurt far too bad for that.\n\r", ch );
	    break;

	case POS_STUNNED:
	    send_to_char( "You are too stunned to do that.\n\r", ch );
	    break;

	case POS_SLEEPING:
	    send_to_char( "In your dreams, or what?\n\r", ch );
	    break;

	case POS_RESTING:
	    send_to_char( "Nah... You feel too relaxed...\n\r", ch);
	    break;

	case POS_SITTING:
	    send_to_char( "Better stand up first.\n\r",ch);
	    break;

	case POS_FIGHTING:
	    send_to_char( "No way!  You are still fighting!\n\r", ch);
	    break;

	}
	return;
    }

    /*
     * Dispatch the command.
     */
    if (is_affected( ch, gsn_painful_thoughts))
	{
	ch->hit -= 1;
send_to_char ("You cry a little as your mind is distracted by the music.\n\r", 
	ch);
	act("$n cries a little as $e listens to an echo of a song.",
		ch, NULL,NULL, TO_ROOM);
	}	

    if (cmd_table[cmd].do_fun != 0)
      (*cmd_table[cmd].do_fun) (ch, argument);
    else if (IS_NPC(ch) && (cmd_table[cmd].prog_fun != 0))
      (*cmd_table[cmd].prog_fun) (ch, NULL, NULL, argument, MOB_PROG);
}

void ointerpret( OBJ_DATA *ch, char *argument )
{
    char command[MAX_INPUT_LENGTH];
    char logline[MAX_INPUT_LENGTH];

    /*
     * Strip leading spaces.
     */
    while ( isspace(*argument) )
        argument++;
    if ( argument[0] == '\0' )
        return;

    /*
     * Grab the command word.
     * Special parsing so ' can be a command,
     *   also no spaces needed after punctuation.
     */
    strcpy( logline, argument );
    if ( !isalpha(argument[0]) && !isdigit(argument[0]) )
    {
        command[0] = argument[0];
        command[1] = '\0';
        argument++;
        while ( isspace(*argument) )
            argument++;
    }
    else
    {
        argument = one_argument( argument, command );
    }

    // Perform command lookup
    int cmd = LookupCommand(command, ch->level);
    if (cmd < 0)
        return;
    
    // Log and snoop.
    if ( cmd_table[cmd].log == LOG_NEVER )
        strcpy( logline, "" );

    if ( fLogAll || cmd_table[cmd].log == LOG_ALWAYS )
    {
        sprintf( log_buf, "Log %s: %s", ch->name, logline );
        log_string( log_buf );
    }

    // Dispatch the command.
    if (cmd_table[cmd].prog_fun != 0)
      (*cmd_table[cmd].prog_fun) ( NULL, ch, NULL, argument, OBJ_PROG );
}

void rinterpret( ROOM_INDEX_DATA *ch, char *argument )
{
    char command[MAX_INPUT_LENGTH];
    char logline[MAX_INPUT_LENGTH];

    /*
     * Strip leading spaces.
     */
    while ( isspace(*argument) )
        argument++;
    if ( argument[0] == '\0' )
        return;

    /*
     * Grab the command word.
     * Special parsing so ' can be a command,
     *   also no spaces needed after punctuation.
     */
    strcpy( logline, argument );
    if ( !isalpha(argument[0]) && !isdigit(argument[0]) )
    {
        command[0] = argument[0];
        command[1] = '\0';
        argument++;
        while ( isspace(*argument) )
            argument++;
    }
    else
    {
        argument = one_argument( argument, command );
    }

    // Perform command lookup
    int cmd = LookupCommand(command, LEVEL_IMMORTAL);
    if (cmd < 0)
        return;
    
    // Log and snoop.
    if ( cmd_table[cmd].log == LOG_NEVER )
        strcpy( logline, "" );

    if ( fLogAll || cmd_table[cmd].log == LOG_ALWAYS )
    {
        sprintf( log_buf, "Log %s: %s", ch->name, logline );
        log_string( log_buf );
    }

    // Dispatch the command.
    if (cmd_table[cmd].prog_fun != 0)
      (*cmd_table[cmd].prog_fun) ( NULL, NULL, ch, argument, ROOM_PROG );
}

bool is_social(CHAR_DATA * ch, const char * command, int & index)
{
	for (index = 0; social_table[index].name[0] != '\0'; ++index)
    {
		if (command[0] == social_table[index].name[0] && !str_prefix(command, social_table[index].name))
			return true;
	}
	return false;
}

bool check_social( CHAR_DATA *ch, char *command, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int cmd(0);

	if (!is_social(ch, command, cmd))
		return false;

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
    {
	send_to_char( "You are anti-social!\n\r", ch );
	return TRUE;
    }

    switch ( ch->position )
    {
    case POS_DEAD:
	send_to_char( "Lie still; you are DEAD.\n\r", ch );
	return TRUE;

    case POS_INCAP:
    case POS_MORTAL:
	send_to_char( "You are hurt far too bad for that.\n\r", ch );
	return TRUE;

    case POS_STUNNED:
	send_to_char( "You are too stunned to do that.\n\r", ch );
	return TRUE;

    case POS_SLEEPING:
	/*
	 * I just know this is the path to a 12" 'if' statement.  :(
	 * But two players asked for it already!  -- Furey
	 */
	if ( !str_cmp( social_table[cmd].name, "snore" ) )
	    break;
	send_to_char( "In your dreams, or what?\n\r", ch );
	return TRUE;

    }

    one_argument( argument, arg );
    victim = NULL;
    if ( arg[0] == '\0' )
    {
	act( social_table[cmd].others_no_arg, ch, NULL, victim, TO_ROOM    );
	act( social_table[cmd].char_no_arg,   ch, NULL, victim, TO_CHAR    );
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
    }
    else if ( victim == ch )
    {
	act( social_table[cmd].others_auto,   ch, NULL, victim, TO_ROOM    );
	act( social_table[cmd].char_auto,     ch, NULL, victim, TO_CHAR    );
    }
    else
    {
	act( social_table[cmd].others_found,  ch, NULL, victim, TO_NOTVICT );
	act( social_table[cmd].char_found,    ch, NULL, victim, TO_CHAR    );
	act( social_table[cmd].vict_found,    ch, NULL, victim, TO_VICT    );

/* God. Autoresponding socials? no.
 *
	if ( !IS_NPC(ch) && IS_NPC(victim)
	&&   !IS_AFFECTED(victim, AFF_CHARM)
	&&   IS_AWAKE(victim) 
	&&   victim->desc == NULL)
	{
	    switch ( number_bits( 4 ) )
	    {
	    case 0:

	    case 1: case 2: case 3: case 4:
	    case 5: case 6: case 7: case 8:
		act( social_table[cmd].others_found,
		    victim, NULL, ch, TO_NOTVICT );
		act( social_table[cmd].char_found,
		    victim, NULL, ch, TO_CHAR    );
		act( social_table[cmd].vict_found,
		    victim, NULL, ch, TO_VICT    );
		break;

	    case 9: case 10: case 11: case 12:
		act( "$n slaps $N.",  victim, NULL, ch, TO_NOTVICT );
		act( "You slap $N.",  victim, NULL, ch, TO_CHAR    );
		act( "$n slaps you.", victim, NULL, ch, TO_VICT    );
		break;
	    }
	}
 *
 * NONONONONO
 */
    }

    return TRUE;
}



/*
 * Return true if an argument is completely numeric.
 */
bool is_number (const char *arg )
{
 
    if ( *arg == '\0' )
        return FALSE;
 
    if ( *arg == '+' || *arg == '-' )
        arg++;
 
    for ( ; *arg != '\0'; arg++ )
    {
        if ( !isdigit( *arg ) )
            return FALSE;
    }
 
    return TRUE;
}



/* 
 * Given a string like 14*foo, return 14 and 'foo'
*/
int mult_argument(char *argument, char *arg)
{
    char *pdot;
    int number;

    for ( pdot = argument; *pdot != '\0'; pdot++ )
    {
        if ( *pdot == '*' )
        {
            *pdot = '\0';
            number = atoi( argument );
            *pdot = '*';
            strcpy( arg, pdot+1 );
            return number;
        }
    }
 
    strcpy( arg, argument );
    return 1;
}


/*
 * Contributed by Alander.
 */
void do_commands( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    int cmd;
    int col;
 
    col = 0;
    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
    {
        if ( cmd_table[cmd].level <  LEVEL_HERO
        &&   cmd_table[cmd].level <= get_trust( ch ) 
	&&   cmd_table[cmd].show)
	{
	    sprintf( buf, "%-15s", cmd_table[cmd].name );
	    send_to_char( buf, ch );
	    if ( ++col % 5 == 0 )
		send_to_char( "\n\r", ch );
	}
    }
 
    if ( col % 5 != 0 )
	send_to_char( "\n\r", ch );
    return;
}

void do_wizhelp( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    int cmd;
    int col;
 
    col = 0;
    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
    {
        if ( cmd_table[cmd].level >= LEVEL_HERO
        &&   cmd_table[cmd].level <= get_trust( ch ) 
        &&   cmd_table[cmd].show)
	{
	    sprintf( buf, "%-12s", cmd_table[cmd].name );
	    send_to_char( buf, ch );
	    if ( ++col % 6 == 0 )
		send_to_char( "\n\r", ch );
	}
    }
 
    if ( col % 6 != 0 )
	send_to_char( "\n\r", ch );
    return;
}

