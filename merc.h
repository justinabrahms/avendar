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

#ifndef MERC_H
#define MERC_H

//#define HERODAY 1

// Another stupid test.
#if defined(unix)
#include <sys/time.h>
#include <unistd.h>
#else
#include <time.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "LeyGroup.h"
#include "Shades.h"
#include "SomaticArtsInfo.h"
#include "StringUtil.h"

class PhantasmInfo;
class FactionStanding;
class ExperienceList;

/*
 * Accommodate old non-Ansi compilers.
 */
#if defined(TRADITIONAL)
#define const
#define args( list )			( )
#define DECLARE_DO_FUN( fun )		void fun( )
#define DECLARE_ODO_FUN( fun )          void fun( )
#define DECLARE_SPEC_FUN( fun )		bool fun( )
#define DECLARE_SPELL_FUN( fun )	bool fun( )
#define DECLARE_FOCUS_FUN( fun )	bool fun( )
#define DECLARE_SONG_FUN( fun )		void fun( )
#define DECLARE_PROG_FUN( fun )		void fun( )
#define DECLARE_POISON_FUN( fun)	void fun( )
#else
#define args( list )			list
#define DECLARE_DO_FUN( fun )		DO_FUN    fun
#define DECLARE_ODO_FUN( fun )          ODO_FUN   fun
#define DECLARE_SPEC_FUN( fun )		SPEC_FUN  fun
#define DECLARE_SPELL_FUN( fun )	SPELL_FUN fun
#define DECLARE_FOCUS_FUN( fun )	FOCUS_FUN fun
#define DECLARE_SONG_FUN( fun )		SONG_FUN fun
#define DECLARE_PRO_FUN( fun )		PRO_FUN  fun
#define DECLARE_POISON_FUN( fun)	POISON_FUN fun
#endif

#define	PROG_FUN( fun )		void fun(CHAR_DATA *ch, OBJ_DATA *obj, ROOM_INDEX_DATA *room, char *argument, int ptype)
#define DO_FUNC( fun )		void fun(CHAR_DATA *ch, char *argument)

/* system calls */
int unlink();
int system();

/*
 * bit macros. Want 65000 t/f fields for every character,
 * rammed into 8k? Ok. Naturally, they'll be highly compressible
 * for at least as long as it takes my builders to abuse it a lot
 * NOTE: when you use these macros, you MUST use unsigned char *
 * for ptr -- anything else will break. These macros ought to
 * be endian non-specific (which is why they work in 1-char sizes)
 * Final note: bit get will return your compilers TRUE, which may
 * not be 1 on all compilers. Of course, since rom generally assumes
 * it is, these won't be the only thing that break.
 */

#define BITS_PER_ENTITY 	3   /* 2^3 == 8 == sizeof(unsigned char) */
#define BIT_SET(ptr,index) *(ptr+(index>>BITS_PER_ENTITY)) |= (1 << (index & ( (8) - 1 ) ) )
#define BIT_FLIP(ptr,index) *(ptr+(index>>BITS_PER_ENTITY)) ^= (1 << (index & ( (8) - 1 ) ) )
#define BIT_CLEAR(ptr,index) *(ptr+(index>>BITS_PER_ENTITY)) &= ~(1 << (index & ( (8) - 1 ) ) )
#define BIT_GET(ptr,index) ( *(ptr+(index>>BITS_PER_ENTITY) ) & (1 << (index & ( (8) - 1 ) ) ) )
#define BIT_COPY(ptr,index,val) *(ptr+(index>>BITS_PER_ENTITY) ) = *(ptr+(index>>BITS_PER_ENTITY) ) & ~( 1 << (index & ( (8) - 1 ) ) ) | ( (val) ? (1 << (index & (8) - 1 ) ) : 0 )

#if	!defined(FALSE)
#define FALSE	 0
#endif

#if	!defined(TRUE)
#define TRUE	 1
#endif

/*
 * Structure types.
 */
class RoomPathTracker;
typedef struct	rumor_type		RUMORS;
typedef struct	affect_data		AFFECT_DATA;
typedef struct	area_data		AREA_DATA;
typedef struct  area_link_data		ALINK_DATA;
typedef struct	ban_data		BAN_DATA;
typedef struct 	buf_type	 	BUFFER;
typedef struct	char_data		CHAR_DATA;
typedef struct	descriptor_data		DESCRIPTOR_DATA;
typedef struct	exit_data		EXIT_DATA;
typedef struct	extra_descr_data	EXTRA_DESCR_DATA;
typedef struct	help_data		HELP_DATA;
typedef struct	host_data		HOST_DATA;
typedef struct	kill_data		KILL_DATA;
typedef struct	mob_index_data		MOB_INDEX_DATA;
typedef struct	note_data		NOTE_DATA;
typedef struct	obj_data		OBJ_DATA;
typedef struct	obj_index_data		OBJ_INDEX_DATA;
typedef struct	pc_data			PC_DATA;
typedef struct  gen_data		GEN_DATA;
typedef struct  memory_data		MEMORY_DATA;
typedef struct  track_data		TRACK_DATA;
typedef struct	reset_data		RESET_DATA;
typedef struct	room_index_data		ROOM_INDEX_DATA;
typedef struct	shop_data		SHOP_DATA;
typedef struct	time_info_data		TIME_INFO_DATA;
typedef struct  weather_index_data	WEATHER_INDEX_DATA;
typedef struct	weather_data		WEATHER_DATA;
typedef struct  mob_prog_data           MPROG_DATA;
typedef struct  mob_prog_act_list       MPROG_ACT_LIST;
typedef struct  obj_prog_act_list       OPROG_ACT_LIST;
typedef struct	prog_rundata		PROG_RUNDATA;
typedef	struct	prog_vardata		PROG_VARDATA;
typedef struct  verb_prog_link		VLINK_DATA;
typedef struct  loop_data		LOOP_DATA;
typedef struct  gm_data			GM_DATA;
typedef struct	account_data		ACCOUNT_DATA;
typedef struct	mount_data		MOUNT_DATA;
typedef struct	event_data		EVENT_DATA;
typedef	struct	chain_data		CHAIN_DATA;
typedef struct	header_data		HEADER_DATA;

typedef	struct	vnum_range		VNUM_RANGE;

typedef struct	address_list		ADDRESS_LIST;
typedef struct	wear_type		WEAR_TYPE;
typedef struct	sac_type		SAC_TYPE;
/*
 * Function types.
 */
typedef	void DO_FUN	args( ( CHAR_DATA *ch, char *argument ) );
typedef void ODO_FUN    args( ( OBJ_DATA  *ch, char *argument ) );
typedef bool SPEC_FUN	args( ( CHAR_DATA *ch ) );
typedef void EVENT_FUN  args( ( void *vo ) );
typedef bool SPELL_FUN	args( ( int sn, int level, CHAR_DATA *ch, void *vo,
				int target ) );
typedef bool FOCUS_FUN  args( ( int sn, int level, CHAR_DATA *ch, void *vo) );
typedef bool SONG_FUN	args( ( int sn, int level, CHAR_DATA *ch, void *vo,
				char *txt ) );
typedef void PRO_FUN	args( ( CHAR_DATA *ch, OBJ_DATA *obj,
				ROOM_INDEX_DATA *room, char *argument,
				int ptype ) );
typedef void POISON_FUN args( ( CHAR_DATA *ch, CHAR_DATA *victim,
				int level,int source ) );


/*
 * String and memory management parameters.
 */
#define MAX_KEY_HASH		 2048
#define MAX_INPUT_LENGTH	  8192
#define PAGELEN			   22
#define MAX_RECORD_STRING	 8192

#define NUM_SECONDS_DAY		86400
#define NUM_SECONDS_HOUR	3600
#define NUM_SECONDS_GAME_DAY 1440

#define MAX_DOOR 6

#define CLASS_WATER_SCHOLAR 	0	
#define CLASS_EARTH_SCHOLAR	1
#define CLASS_VOID_SCHOLAR	2
#define CLASS_SPIRIT_SCHOLAR	3
#define CLASS_AIR_SCHOLAR	4
#define CLASS_FIRE_SCHOLAR	5
#define CLASS_WATER_TEMPLAR	6
#define CLASS_EARTH_TEMPLAR	7
#define CLASS_VOID_TEMPLAR	8
#define CLASS_SPIRIT_TEMPLAR	9
#define CLASS_AIR_TEMPLAR	10
#define CLASS_FIRE_TEMPLAR	11
#define CLASS_THIEF		12
#define CLASS_WATCHER		13
#define CLASS_ASSASSIN		14
#define CLASS_BANDIT		15
#define CLASS_ROGUE1		16
#define CLASS_ROGUE2		17
#define CLASS_FIGHTER		18
#define CLASS_SWORDMASTER	19
#define CLASS_BARBARIAN		20
#define CLASS_GLADIATOR		21
#define CLASS_WARRIOR		22
#define CLASS_RANGER		23
#define CLASS_GLEEMAN		24
#define CLASS_BARD		25
#define CLASS_ENTERTAINER	26
#define CLASS_ALCHEMIST		27
#define CLASS_PSIONICIST	28
#define CLASS_DRUID		29

#define MAX_CONFUSED_WORDS	179

#define PROF_SCHOLAR		0
#define PROF_TEMPLAR		1
#define PROF_ROGUE		2
#define PROF_WARRIOR		3
#define PROF_NATURALIST		4
#define PROF_ENTERTAINER	5
#define PROF_OTHER		6
#define PROF_MENTALIST		7

#define PFILE_VERSION		31
/*
 * Ethos declarations..
 */

#define ETH_LAWFUL		0
#define ETH_NEUTRAL		1
#define ETH_BALANCED		1
#define ETH_CHAOTIC		2

#define ALIGN_GOOD		0
#define ALIGN_NEUTRAL		1
#define ALIGN_EVIL		2
#define	ALIGN_RANDOM		3

#define BIT_LAWFUL		1
#define BIT_BALANCED		2
#define BIT_CHAOTIC		4

#define BIT_GOOD		1
#define BIT_NEUTRAL		2
#define BIT_EVIL		4

#define IS_LAWFUL(ch)		(is_lawful(ch))
#define IS_BALANCED(ch)		(is_balanced(ch))
#define IS_CHAOTIC(ch)		(is_chaotic(ch))

#define HOURS_PLAYED(ch)        ((int) (ch->played + current_time - ch->logon) / 3600)
#define IS_CRIMINAL(ch)		(!IS_NPC(ch) && (ch->act & PLR_REWARD))
#define GOODRUMOR(rumor)	((current_time > rumor->stamp+NUM_SECONDS_HOUR) && (rumor->sticky || (current_time <= rumor->stamp + NUM_SECONDS_DAY*NUM_DAYS_YEAR)))
/*
 * Sphere declarations..
 */

#define SPH_WATER		0
#define SPH_EARTH		1
#define SPH_VOID		2
#define SPH_SPIRIT		3
#define SPH_AIR			4
#define SPH_FIRE		5
#define SPH_NATURE		7
#define SPH_ARCANE		8
#define SPH_TRANS		9
#define SPH_HOUSE		10
#define SPH_PSION		11
#define SPH_SONG		12
#define SPH_FIRIEL		13
#define	SPH_LUNAR		14
#define SPH_GAMALOTH		15
#define SPH_NONE		16

// Path declarations
#define PATH_NONE               0
#define PATH_GOLDENFLAMES       1
#define PATH_RAGINGINFERNO      2
#define PATH_FLAMEHEART         3
#define PATH_WEAVEDANCER        4
#define PATH_SILVERLIGHT        5
#define PATH_ETERNALDAWN        6
#define PATH_LIVINGWATERS       7
#define PATH_WAVEBORNE          8
#define PATH_WINTERTIDE         9
#define PATH_ENDLESSFACADE      10
#define PATH_FLICKERINGSKIES    11
#define PATH_WINDRIDER          12
#define PATH_STONESHAPER        13
#define PATH_GEOMANCER          14
#define PATH_WAKENEDSTONE       15
#define PATH_NECROMANCER        16
#define PATH_NIGHTFALL          17
#define PATH_RIVENVEIL          18
#define MAX_PATH_COUNT          19

/*
 * Warrior sub-class declarations.
 */
#define SPH_FIGHTER		0
#define SPH_SWORDMASTER		1
#define SPH_BARBARIAN		2
#define SPH_GLADIATOR		3

/*
 * Rogue sub-class declarations.
 */
#define SPH_THIEF		0
#define SPH_THIEFTAKER		1
#define SPH_ASSASSIN		2
#define SPH_BANDIT		3

/*
 * Entertainer sub-class declarations.
 */
#define SPH_GLEEMAN		0
#define SPH_BARD		1

/*
 * Naturalists rule, rule rule
 */
#define SPH_RANGER		0
#define SPH_DRUID		1


/*
 * Constants for check_reach
 */
#define REACH_EASY		0
#define REACH_NORMAL		1
#define REACH_HARD		2

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_GODS		   29
#define MAX_SOCIALS		  300
#define MAX_SKILL		 1251
#define MAX_GROUP		  142
#define MAX_IN_GROUP		   60 
#define MAX_ALIAS		   15
#define MAX_ETHOS		    3
#define MAX_SPHERES		    6
#define MAX_CLASS		    30
#define MAX_PC_RACE		    12
#define MAX_CLAN		   10
#define MAX_DAMAGE_MESSAGE	   47
#define MAX_LEVEL		   60
#define NEWBIE_CHANNEL_LEVEL	   61
#define MAX_SWORDS		   70
#define MAX_MATERIALS		   87
#define MAX_ARROWS                 30
#define MAX_CLORE                  5
#define MAX_RESIST		   26
#define MAX_SNOOP		   10
#define MAX_HOMETOWN		   12
#define MAX_FOCUS		   8
#define MAX_ASSIST_VNUM		   5
#define MAX_LASTOWNER		   3
#define MAX_SONGBOOK		   5
#define MAX_INSCRIBE_ECHOES	   8
#define MAX_MOBVALUE           10
#define MAX_MOBFOCUS		   2
#define MAX_OBJFOCUS			2
#define MAX_ROOMFOCUS		   2
#define MAX_COIN		   4
#define LEVEL_HERO		   (MAX_LEVEL - 9)
#define LEVEL_IMMORTAL		   (MAX_LEVEL - 8)
#define MAX_DESCRIPTION_LENGTH	   10256 
#define PULSE_PER_SECOND	    4
#define PULSE_DISC            (2)
#define PULSE_LODESTONE       (3)
#define PULSE_VIOLENCE		  ( 3 * PULSE_PER_SECOND)
#define PULSE_MOBILE		  ( 4 * PULSE_PER_SECOND)
#define PULSE_TICK		  (30 * PULSE_PER_SECOND)
#define PULSE_AREA		  (120 * PULSE_PER_SECOND)
#define PULSE_OATHS       (240 * PULSE_PER_SECOND)

#define IMPLEMENTOR		MAX_LEVEL
#define	CREATOR			(MAX_LEVEL - 1)
#define SUPREME			(MAX_LEVEL - 2)
#define DEITY			(MAX_LEVEL - 3)
#define GOD			(MAX_LEVEL - 4)
#define IMMORTAL		(MAX_LEVEL - 5)
#define DEMI			(MAX_LEVEL - 6)
#define ANGEL			(MAX_LEVEL - 7)
#define AVATAR			(MAX_LEVEL - 8)
#define HERO			LEVEL_HERO

#define FACTOR_Y	9.9  // increased 10% from original value of 9.2, Jan 12/03

#define DOPPEL_SLOT	1

#define MAZE_SIZE	16

#define PERC_RES	20	
#define PERC_VULN	-20
#define FIRST_APPLY_RESIST 29

#define NUM_DAYS_YEAR   381	
#define NUM_HOURS_DAY	24

#define MAX_PROMPT_LENGTH	70

#define HOST_TIME_WARNING	600

#define HC_MULT		2

/*
 * Colour stuff by Lope of Loping Through The MUD
 */
#define CLEAR		"[0m"		/* Resets Colour	*/
#define C_RED		"[0;31m"	/* Normal Colours	*/
#define C_GREEN		"[0;32m"
#define C_YELLOW	"[0;33m"
#define C_BLUE		"[0;34m"
#define C_MAGENTA	"[0;35m"
#define C_CYAN		"[0;36m"
#define C_WHITE		"[0;37m"
#define C_D_GREY	"[1;30m"  	/* Light Colors		*/
#define C_B_RED		"[1;31m"
#define C_B_GREEN	"[1;32m"
#define C_B_YELLOW	"[1;33m"
#define C_B_BLUE	"[1;34m"
#define C_B_MAGENTA	"[1;35m"
#define C_B_CYAN	"[1;36m"
#define C_B_WHITE	"[1;37m"

#define C_BLINKING	"[5m"

bool IsOrderBanned(const char * order);
int DetermineDirection(const char * arg);

/*
 * Event chain structure.
 */

struct	chain_data
{
    CHAIN_DATA *	next;
    CHAIN_DATA *	next_chain;
    bool		valid;
    EVENT_DATA *	event;
    int			nci;
};

struct	event_data
{
    EVENT_DATA *	next;
    bool		valid;
    EVENT_FUN *		efunc;
    void *		point;
};


/*
 * Site ban structure.
 */

#define BAN_SUFFIX		A
#define BAN_PREFIX		B
#define BAN_NEWBIES		C
#define BAN_ALL			D	
#define BAN_PERMIT		E
#define BAN_PERMANENT		F
#define BAN_WARNING		G

struct	ban_data
{
    BAN_DATA *	next;
    bool	valid;
    long	ban_flags;
    int	level;
    char *	name;
};

struct buf_type
{
    BUFFER *    next;
    bool        valid;
    int      state;  /* error state of the buffer */
    int         size;   /* size in k */
    char *      string; /* buffer's string */
};

struct address_list
{
    void *	addr;
    ADDRESS_LIST * next;
};

struct wear_type
{
    int	wear_loc;
    int	wear_bit;
    char *loc_name;
};

struct sac_type
{
    int level;
    int number; 
};

#define MOB_PROG		1
#define OBJ_PROG		2
#define ROOM_PROG		3


/*
 * Time and weather stuff.
 */
#define MSIZE_TINY		0
#define MSIZE_SMALL		1
#define MSIZE_MEDIUM		2
#define MSIZE_LARGE		3
#define MSIZE_HUGE		4

#define MOON_WANING_GIBBOUS	    0
#define MOON_WANING_HALF	    1
#define MOON_WANING_CRESCENT	    2
#define MOON_NEW		    3
#define MOON_WAXING_CRESCENT	    4
#define MOON_WAXING_HALF	    5
#define MOON_WAXING_GIBBOUS         6
#define MOON_FULL		    7

#define SUN_DARK		    0
#define SUN_RISE		    1
#define SUN_LIGHT		    2
#define SUN_SET			    3

#define SKY_CLOUDLESS		    0
#define SKY_CLOUDY		    1
#define SKY_RAINING		    2
#define SKY_LIGHTNING		    3

struct	time_info_data
{
    int		hour;
    bool	half;
    int		day;
    int 	week;
    int		month;
    int		year;
    int		phase_lunus;
    int		phase_rhos;
    int		size_rhos;
    int		season;
    int 	day_year;
};

/* struct	weather_data
{
    int		mmhg;
    int		change;
    int		sky;
    int		sunlight;
};
*/

/*
 * Connected state for a channel.
 */
#define CON_PLAYING			 0
#define CON_GET_NAME			 1
#define CON_GET_OLD_PASSWORD		 2
#define CON_CONFIRM_NEW_NAME		 3
#define CON_GET_NEW_PASSWORD		 4
#define CON_CONFIRM_NEW_PASSWORD	 5
#define CON_RACE_INFO                    6
#define CON_GET_NEW_RACE		 7
#define CON_GET_NEW_SEX			 8
#define CON_GET_NEW_CLASS		 9
#define CON_PICK_NON_SCHOLAR		10 
#define CON_PICK_MAJOR		        11
#define CON_PICK_MINOR			12
#define CON_PICK_ALIGNMENT		13
#define CON_PICK_ETHOS			14
#define CON_PICK_HOMETOWN		15
// #define CON_ROLL_STATS                  16 
#define	CON_INIT_CHAR			16
// #define CON_PICK_WEAPON			17
#define CON_SECOND_LANGUAGE		17
#define CON_READ_IMOTD			18
#define CON_READ_MOTD			19
#define CON_BREAK_CONNECT		20
#define CON_ACCEPT_CONDITIONS		21

#define CON_CHOOSE_COLOUR		23
#define CON_GET_HUMAN_LANGUAGE		24
#define CON_GET_LANG_TRAINS		25

#define CON_GET_ACCT_PASSWORD		26
#define CON_ACCOUNT_INFO		27
#define CON_CONFIRM_ACCOUNT		28
#define CON_GET_ACCOUNT_EMAIL		29

#define CON_GET_HARDCORE		30

#define CON_CONFIRM_RACE		31

#define CON_GET_SURNAME			32
#define CON_CONFIRM_SURNAME		33

#define CON_TRAIT_OPTION		34
#define CON_TRAIT_GROUPSEL		35
#define CON_TRAIT_GROUP1		36
#define CON_TRAIT_GROUP2		37
#define CON_TRAIT_GROUP3		38
#define CON_TRAIT_GROUP4		39
#define CON_TRAIT_CONFIRM		40
#define CON_TRAIT_CONTINUE		41

#define CON_GOD_QUESTION		42
#define CON_GOD_SELECT			43
#define CON_BLESSING_OPTION		44

#define CON_HEIRLOOM_TYPE		45
#define CON_HEIRLOOM_TYPE2		46
#define CON_HEIRLOOM_TYPE3		47
#define CON_HEIRLOOM_MATERIAL		48
#define CON_HEIRLOOM_SHORT		49
#define CON_HEIRLOOM_ED			50

#define CON_IS_NEWBIE			51
#define CON_GET_ATTR                    52
#define CON_ACCEPT_CHAR			53
#define CON_GET_DRUID_POWER		54

#define SNOOP_NORMAL	0
#define SNOOP_COMM	1
#define SNOOP_BRIEF	2


/*
 * Descriptor (channel) structure.
 */
struct	descriptor_data
{
    DESCRIPTOR_DATA *	next;
    CHAR_DATA *		character;
    CHAR_DATA *		original;
    ACCOUNT_DATA *	acct;
    DESCRIPTOR_DATA *	snoop_by[MAX_SNOOP];
    unsigned short	snoop_type[MAX_SNOOP];
    bool		snooped;
    bool		valid;
    bool		dnsdone;
    char *		host;
    unsigned int	descriptor;
    int		connected;
    bool		fcommand;
    char		inbuf		[12 * MAX_INPUT_LENGTH];
    char		incomm		[MAX_INPUT_LENGTH];
    char		inlast		[MAX_INPUT_LENGTH];
    int			repeat;
    char *		outbuf;
    int			outsize;
    int			outtop;
    char *		showstr_head;
    char *		showstr_point;
    bool		warning;
    bool		dPrompt;
    void *              pEdit;		/* OLC */
    char **             pString;	/* OLC */
    int			editor;		/* OLC */
    int			temp;		// Currently used in trait selection.
	bool        newbie;
};


/*
 * Account flags, for use in account_data flags field.
 */

#define ACCT_NO_BAN			(A)
#define ACCT_DENY			(B)
#define ACCT_MENTOR			(C)

/*
 * Account structure.
 */
struct account_data
{
    char *	name;
    char *	chars;
    char *	pwd;
    char *	deleted;
    char *	immrecord;
    char *	email;
    char *	socket_info;
    char *	def_prompt;
    int		flags;
    int		award_points;
};

struct header_data
{
    char *	name;
    HEADER_DATA *next;
};

/*
 * Attribute bonus structures.
 */
struct	str_app_type
{
    int	todam;
    int	wield;
};

struct	int_app_type
{
    int	mana_bonus;
};

struct	wis_app_type
{
    int	save_mod;
    int	res_illusion_bonus;
    int	mana_bonus;
};

struct	dex_app_type
{
    int	defensive;
    int      tohit;
};

struct	con_app_type
{
    int	hitp;
};

struct chr_app_type
{
    int	cost_mod;
    int	request_range;
    int	mana_bonus;   // for bards
};


struct host_data
{
    HOST_DATA *		next;
    time_t		lastoff;
    char *		host_name;
    char *		char_name;
};
    

/*
 * TO types for act.
 */
#define TO_ROOM		    0
#define TO_NOTVICT	    1
#define TO_VICT		    2
#define TO_CHAR		    3
#define TO_ALL		    4
#define TO_VICTROOM     5    /** added for bow and arrows **/

/*
 * Stuff for the channel engine.
 */

#define CTO_ROOM	    0
#define CTO_VICT	    1
#define CTO_AREA	    2
#define CTO_CLASS	    3
#define CTO_HOUSE	    4
#define CTO_OTHER	    5

#define CHAN_NONE	    -1
#define CHAN_SAY	    0
#define CHAN_OOC	    1
#define CHAN_YELL	    2
#define CHAN_CYELL	    3
#define CHAN_MUSIC	    4
#define CHAN_DRUID	    5
#define CHAN_HOUSE	    6
#define CHAN_AUCTION	    7
#define CHAN_QUESTION       8
#define CHAN_ANSWER	    9
#define CHAN_SING	   10
#define CHAN_THINK         11
#define CHAN_GTELL	   12
#define CHAN_TELL	   13
#define CHAN_ESAY	   14

#define SILVER_CHARGED	    14
#define SILVER_FINAL	    16

/*
 * Help table types.
 */
struct	help_data
{
    HELP_DATA *	next;
    int	level;
    char *	keyword;
    char *	text;
    char *	title;
};

#define HELP_NOTITLE	1



/*
 * Shop types.
 */
#define MAX_TRADE	 5

struct	shop_data
{
    SHOP_DATA *	next;			/* Next shop in list		*/
    int	keeper;			/* Vnum of shop keeper mob	*/
    int	buy_type [MAX_TRADE];	/* Item types shop will buy	*/
    int	profit_buy;		/* Cost multiplier for buying	*/
    int	profit_sell;		/* Cost multiplier for selling	*/
    int	open_hour;		/* First opening hour		*/
    int	close_hour;		/* First closing hour		*/
    long	max_buy;		/* Highest priced item bought   */
};



/*
 * Per-class stuff.
 */

#define MAX_GUILD 	2
#define MAX_STATS 	6
#define STAT_STR 	0
#define STAT_INT	1
#define STAT_WIS	2
#define STAT_DEX	3
#define STAT_CON	4
#define STAT_CHR	5

struct	ethos_type
{
    char *	name;
    int	number;
};

struct	sphere_type
{
    char *	name;
    int	number;
};

typedef bool (*PathValidityFun)(CHAR_DATA*);

struct path_type
{
    char * name;
    int number;
    PathValidityFun is_valid_for;
};

struct elemental_bias_type
{
	int          bias_void;
	int          bias_spirit;
	int          bias_water;
	int          bias_fire;
	int          bias_earth;
	int          bias_air;
	int		bias_nature;
	int		bias_ancient;
	int		bias_transmogrify;
};

struct platonic_bias_type
{
	int		bias_damage;
	int		bias_malediction;
	int		bias_protection;
	int		bias_order;
	int		bias_enhancement;
	int		bias_chaos;
	int		bias_obscurement;
	int		bias_healing;
	int		bias_knowledge;
	int		bias_planar;
	int		bias_alteration;
};


struct material_type
{
    char *					name;
    bool    				shatter;
    bool                    stone;
    bool					metal;
    bool                    ferric;
    bool                    wood;
    bool                    clothlike;
    int					charges; //For wand/staff making
    const struct elemental_bias_type  	elem_bias;
    const struct platonic_bias_type		plat_bias;
};

#define MATERIAL_UNKNOWN	0
#define MATERIAL_FLESH		1


struct	class_type
{
    char *	name;			/* the full name of the class */
    char 	who_name	[4];	/* Three-letter name for 'who'	*/
    int	attr_prime;		/* Prime attributes		*/
    int	weapon;			/* First weapon			*/
    int	skill_adept;		/* Maximum skill level		*/
    int	thac0_00;		/* Thac0 for level  0		*/
    int	thac0_32;		/* Thac0 for level 32		*/
    int	hp_min;			/* Min hp gained on leveling	*/
    int	hp_max;			/* Max hp gained on leveling	*/
    int		fMana;			/* Class gains mana on level	*/
    int         races_allowed[MAX_PC_RACE];
    int         align_allowed;
    int         ethos_allowed;                           
    int	class_group;
    bool	avail_hometowns[MAX_HOMETOWN];
    char *	base_group;		/* base skills gained		*/
    char *	default_group;		/* default skills gained	*/
    int		guildrooms[20];
};

struct item_type
{
    int		type;
    char *	name;
    const struct elemental_bias_type        elem_bias;
    const struct platonic_bias_type         plat_bias;
};

struct weapon_type
{
    char *	name;
    int	vnum;
    int	type;
    int	*gsn;
};

struct wiznet_type
{
    char *	name;
    long 	flag;
    int		level;
};

struct attack_type
{
    char *	name;			/* name */
    char *	noun;			/* message */
    int   	damage;			/* damage class */
};

struct race_type
{
    char *	name;			/* call name of the race */
    bool	pc_race;		/* can be chosen by pcs */
    long	act;			/* act bits for the race */
    long	aff;			/* aff bits for the race */
    long	off;			/* off bits for the race */
    long	imm;			/* imm bits for the race */
    long        res;			/* res bits for the race */
    long	vuln;			/* vuln bits for the race */ 
    int      resist[MAX_RESIST];
    long	form;			/* default form flag for the race */
    long	parts;			/* default parts for the race */
    int	native_tongue;
};


#define AGE_NONE	0
#define AGE_IMMATURE	1
#define AGE_YOUTHFUL	2
#define AGE_MATURE	3
#define AGE_MIDDLE	4
#define AGE_OLD		5
#define AGE_ELDERLY	6
#define AGE_ANCIENT	7
#define AGE_DECREPID	8
#define AGE_DEAD	9
#define MAX_AGEGROUP   10 

struct pc_race_type  /* additional data for pc races */
{
    char *	name;			/* MUST be in race_type */
    char 	who_name[6];
    int	points;			/* cost in points of the race */
    int	class_mult[MAX_CLASS];	/* exp multiplier for class, * 100 */
    char *	skills[5];		/* bonus skills for the race */
    int 	stats[MAX_STATS];	/* starting stats */
    int	max_stats[MAX_STATS];	/* maximum stats */
    int	size;			/* aff bits for the race */
    int      allowed_aligns; 	/* allowed bitvector aligns for race */
    int      allowed_ethos; 	        /* allowed bitvector aligns for race */
    int      age_values[4];
    int	age_margins[MAX_AGEGROUP];
    bool	avail_hometowns[MAX_HOMETOWN];
    int		hunger_max;
    int		race_hunger;
    int	pracs_level;
    int	deaths_mod;
};

struct spec_type
{
    char * 	name;			/* special function name */
    SPEC_FUN *	function;		/* the function */
};



/*
 * Data structure for notes.
 */

#define NOTE_NOTE	0
#define NOTE_IDEA	1
#define NOTE_PENALTY	2
#define NOTE_NEWS	3
#define NOTE_CHANGES	4
#define NOTE_ROOM	5

#define NFLAG_STICKY	1

struct	note_data
{
    NOTE_DATA *	next;
    bool 	valid;
    int	type;
    char *	sender;
    char *	forger;
    bool 	forged;
    char *	date;
    char *	to_list;
    char *	subject;
    char *	text;
    char *	fake_to;
    time_t  	date_stamp;
    int		flags;
};

struct	rumor_type
{
    bool	valid;
    char *	name;
    char *	text;
    time_t	stamp;
    bool	sticky;
    RUMORS *	next;
};
/*
 * An affect.
 */
struct	affect_data
{
    AFFECT_DATA *	next;
    bool		valid;
    int		where;
    int		type;
    int		level;
    void *		point;	
    int		duration;
    int		location;
    int  		modifier;
    int			bitvector;
};

/* where definitions */
#define TO_AFFECTS	0
#define TO_OBJECT	1
#define TO_OBJECT0      1
#define TO_IMMUNE	2
#define TO_RESIST	3
#define TO_VULN		4
#define TO_WEAPON	5
#define TO_AREA		6
#define TO_ROOM_AFF	7
#define TO_NAFFECTS	8
#define TO_OAFFECTS     9
#define TO_LANG		10
#define TO_PAFFECTS	11
#define TO_OBJECT1	12
#define TO_OBJECT2	13


/*
 * A kill structure (indexed by level).
 */
struct	kill_data
{
    int		number;
    int		killed;
};



/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
#define MOB_VNUM_FIDO		   3090
#define MOB_VNUM_CITYGUARD	   3060

#define MOB_VNUM_PATROLMAN	   2106
#define GROUP_VNUM_TROLLS	   2100
#define GROUP_VNUM_OGRES	   2101


/* RT ASCII conversions -- used so we can have letters in this file */

#define A		  	1
#define B			2
#define C			4
#define D			8
#define E			16
#define F			32
#define G			64
#define H			128

#define I			256
#define J			512
#define K		        1024
#define L		 	2048
#define M			4096
#define N		 	8192
#define O			16384
#define P			32768

#define Q			65536
#define R			131072
#define S			262144
#define T			524288
#define U			1048576
#define V			2097152
#define W			4194304
#define X			8388608

#define Y			16777216
#define Z			33554432
#define aa			67108864 	/* doubled due to conflicts */
#define bb			134217728
#define cc			268435456    
#define dd			536870912
#define ee			1073741824
#define ff			2147483648UL

#define ATTR_STR 	(A)
#define ATTR_INT	(B)
#define ATTR_WIS	(C)
#define ATTR_DEX	(D)
#define ATTR_CON	(E)
#define ATTR_CHR	(F)

// Flag bits for skill numbers
#define SN_UNWEAVABLE   (A)
#define SN_SPIRITCAST   (B)
#define SN_STALKCAST    (C)
#define SN_BILOCATECAST (D)
#define SN_NOSKILLED    (E)
#define SN_PHYSIKERCAST (F)
#define SN_SOMATICBOOST (G)
#define SN_WINDFALL_DEF (H)
#define SN_WINDFALL_OFF (I)

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
#define ACT_IS_NPC		(A)		/* Auto set for mobs	*/
#define ACT_SENTINEL	    	(B)		/* Stays in one room	*/
#define ACT_SCAVENGER	      	(C)		/* Picks up objects	*/
#define ACT_NOTRACK		(D)		/* includes gate track  */
#define ACT_TRACK_GATE		(E)		/* gate after trackee   */
#define ACT_AGGRESSIVE		(F)    		/* Attacks PC's		*/
#define ACT_STAY_AREA		(G)		/* Won't leave area	*/
#define ACT_WIMPY		(H)
#define ACT_PET			(I)		/* Auto set for pets	*/
#define ACT_TRAIN		(J)		/* Can train PC's	*/
#define ACT_PRACTICE		(K)		/* Can practice PC's	*/
#define ACT_NOWANDER		(L)
#define ACT_GUILDGUARD		(M)
#define ACT_ANIMAL		(N)
#define ACT_UNDEAD		(O)	
#define ACT_BADASS		(P)
#define	ACT_MODIFIED		(Q)
#define ACT_ILLUSION		(R)
#define ACT_THIEF		(S)
#define ACT_WARRIOR		(T)
#define ACT_NOALIGN		(U)
#define ACT_NOPURGE		(V)
#define ACT_OUTDOORS		(W)
#define ACT_NOSUBDUE		(X)
#define ACT_INDOORS		(Y)
#define ACT_GUARDIAN		(Z)
#define ACT_IS_HEALER		(aa)
#define ACT_GAIN		(bb)
#define ACT_UPDATE_ALWAYS	(cc)
#define ACT_IS_CHANGER		(dd)
#define ACT_FAMILIAR		(ee)

#define ACT_BANK		(A)
#define ACT_OMNILINGUAL		(B)
#define ACT_PSYCHO		(C)
#define ACT_DEALER		(D)
#define ACT_PACIFIC		(E)
#define ACT_CHARMRAND		(F)
#define ACT_CLASSED		(G)
#define ACT_NORESCUE		(H)
#define ACT_MOUNT		(I)
#define ACT_SMITH		(J)
#define ACT_NODISARM		(K)
#define ACT_TRACK_FAST		(L)
#define ACT_TRACK_OPEN		(M)
#define ACT_TRACK_RAM		(N)
#define ACT_KEEPGOLD		(O)
#define ACT_FACT_LEADER		(P)
#define ACT_FACT_ELITE		(Q)
#define ACT_SHADE       (R)

#define ACT_SENTINEL_2        (A)
#define ACT_SCAVENGER_2       (B)
#define ACT_NOTRACK_2         (C)
#define ACT_AGGRESSIVE_2      (D)
#define ACT_STAY_AREA_2       (E)
#define ACT_PRACTICE_2        (F)
#define ACT_NOWANDER_2        (G)
#define ACT_GUILDGUARD_2      (H)
#define ACT_BADASS_2          (I)
#define ACT_ILLUSION_2        (J)
#define ACT_NOSUBDUE_2        (K)
#define ACT_HEALER_2          (L)
#define ACT_BANK_2            (M)
#define ACT_OMNILINGUAL_2     (N)
#define ACT_MOUNT_2           (O)
#define ACT_SMITH_2           (P)
#define ACT_TRACK_FAST_2      (Q)
#define ACT_FACTION_LEADER_2  (R)
#define ACT_FACTION_ELITE_2   (S)


/* damage classes */
#define DAM_NONE        0
#define DAM_BASH        1
#define DAM_PIERCE      2
#define DAM_SLASH       3
#define DAM_FIRE        4
#define DAM_COLD        5
#define DAM_LIGHTNING   6
#define DAM_ACID        7
#define DAM_POISON      8
#define DAM_NEGATIVE    9
#define DAM_HOLY        10
#define DAM_ENERGY      11
#define DAM_MENTAL      12
#define DAM_DISEASE     13
#define DAM_DROWNING    14
#define DAM_LIGHT		15
#define DAM_OTHER       16
#define DAM_FEAR		17
#define DAM_CHARM		18
#define DAM_SOUND		19
#define DAM_ILLUSION    20
#define DAM_DEFILEMENT	21
#define DAM_MAX         21 // Always keep this equal to the maximum damtype

/* OFF bits for mobiles */
#define OFF_AREA_ATTACK         (A)
#define OFF_BACKSTAB            (B)
#define OFF_BASH                (C)
#define OFF_BERSERK             (D)
#define OFF_DISARM              (E)
#define OFF_DODGE               (F)
#define OFF_FADE                (G)
#define OFF_FAST                (H)
#define OFF_KICK                (I)
#define OFF_KICK_DIRT           (J)
#define OFF_PARRY               (K)
#define OFF_RESCUE              (L)
#define OFF_TAIL                (M)
#define OFF_TRIP                (N)
#define OFF_CRUSH		(O)
#define ASSIST_ALL       	(P)
#define ASSIST_ALIGN	        (Q)
#define ASSIST_RACE    	     	(R)
#define ASSIST_PLAYERS      	(S)
#define ASSIST_GUARD        	(T)
#define ASSIST_VNUM		(U)
#define OFF_BITE		(V)
#define ASSIST_BOUNCER          (W)
#define OFF_SKEWER		(X)
#define OFF_THROW		(Y)
#define OFF_DUAL_WIELD		(Z)
#define ASSIST_NPCRACE		(aa)

/* return values for check_imm */
#define IS_NORMAL		0
#define IS_IMMUNE		1
#define IS_RESISTANT		2
#define IS_VULNERABLE		3

/* IMM bits for mobs */
#define IMM_SUMMON              (A)
#define IMM_CHARM               (B)
#define IMM_MAGIC               (C)
#define IMM_WEAPON              (D)
#define IMM_BASH                (E)
#define IMM_PIERCE              (F)
#define IMM_SLASH               (G)
#define IMM_FIRE                (H)
#define IMM_COLD                (I)
#define IMM_LIGHTNING           (J)
#define IMM_ACID                (K)
#define IMM_POISON              (L)
#define IMM_NEGATIVE            (M)
#define IMM_HOLY                (N)
#define IMM_ENERGY              (O)
#define IMM_MENTAL              (P)
#define IMM_DISEASE             (Q)
#define IMM_DROWNING            (R)
#define IMM_LIGHT		(S)
#define IMM_SOUND		(T)
#define IMM_ILLUSION		(U)
//#define IMM_NONE		(V)
//#define IMM_NONE2		(W)
#define IMM_DEFILEMENT          (V)
#define IMM_FEAR                (W)
#define IMM_IRON                (Z)
#define IMM_TAME		(aa)
#define IMM_BLIND		(bb)
 
/* RES bits for mobs */
#define RES_SUMMON		(A)
#define RES_CHARM		(B)
#define RES_MAGIC               (C)
#define RES_WEAPON              (D)
#define RES_BASH                (E)
#define RES_PIERCE              (F)
#define RES_SLASH               (G)
#define RES_FIRE                (H)
#define RES_COLD                (I)
#define RES_LIGHTNING           (J)
#define RES_ACID                (K)
#define RES_POISON              (L)
#define RES_NEGATIVE            (M)
#define RES_HOLY                (N)
#define RES_ENERGY              (O)
#define RES_MENTAL              (P)
#define RES_DISEASE             (Q)
#define RES_DROWNING            (R)
#define RES_LIGHT		(S)
#define RES_SOUND		(T)
#define RES_ILLUSION		(U)
//#define RES_NONE		(V)
//#define RES_NONE2		(W)
#define RES_DEFILEMENT          (V)
#define RES_FEAR                (W)
#define RES_IRON                (Z)

/* RESIST flags for new resistances */
#define RESIST_SUMMON		0
#define RESIST_CHARM		1
#define RESIST_MAGIC            2
#define RESIST_WEAPON           3
#define RESIST_BASH             4
#define RESIST_PIERCE           5     
#define RESIST_SLASH            6     
#define RESIST_FIRE             7     
#define RESIST_COLD             8     
#define RESIST_LIGHTNING        9     
#define RESIST_ACID             10   
#define RESIST_POISON           11    
#define RESIST_NEGATIVE         12    
#define RESIST_HOLY             13    
#define RESIST_ENERGY           14    
#define RESIST_MENTAL           15   
#define RESIST_DISEASE          16    
#define RESIST_DROWNING         17     
#define RESIST_LIGHT		18 
#define RESIST_SOUND		19 
#define RESIST_ILLUSION		20 
#define RESIST_NONE		21
#define RESIST_NONE2		22
#define RESIST_DEFILEMENT   23
#define RESIST_FEAR         24
#define RESIST_IRON             25    
 
/* VULN bits for mobs */
#define VULN_SUMMON		(A)
#define VULN_CHARM		(B)
#define VULN_MAGIC              (C)
#define VULN_WEAPON             (D)
#define VULN_BASH               (E)
#define VULN_PIERCE             (F)
#define VULN_SLASH              (G)
#define VULN_FIRE               (H)
#define VULN_COLD               (I)
#define VULN_LIGHTNING          (J)
#define VULN_ACID               (K)
#define VULN_POISON             (L)
#define VULN_NEGATIVE           (M)
#define VULN_HOLY               (N)
#define VULN_ENERGY             (O)
#define VULN_MENTAL             (P)
#define VULN_DISEASE            (Q)
#define VULN_DROWNING           (R)
#define VULN_LIGHT		(S)
#define VULN_SOUND		(T)
#define VULN_ILLUSION		(U)
#define VULN_DEFILEMENT (X)
#define VULN_FEAR       (Y)
#define VULN_IRON		(Z)
 
/* body form */
#define FORM_EDIBLE             (A)
#define FORM_POISON             (B)
#define FORM_MAGICAL            (C)
#define FORM_INSTANT_DECAY      (D)
#define FORM_OTHER              (E)  /* defined by material bit */
 
/* actual form */
#define FORM_ANIMAL             (G)
#define FORM_SENTIENT           (H)
#define FORM_UNDEAD             (I)
#define FORM_CONSTRUCT          (J)
#define FORM_MIST               (K)
#define FORM_INTANGIBLE         (L)
 
#define FORM_BIPED              (M)
#define FORM_CENTAUR            (N)
#define FORM_INSECT             (O)
#define FORM_SPIDER             (P)
#define FORM_CRUSTACEAN         (Q)
#define FORM_WORM               (R)
#define FORM_BLOB		(S)
 
#define FORM_MAMMAL             (V)
#define FORM_BIRD               (W)
#define FORM_REPTILE            (X)
#define FORM_SNAKE              (Y)
#define FORM_DRAGON             (Z)
#define FORM_AMPHIBIAN          (aa)
#define FORM_FISH               (bb)
#define FORM_COLD_BLOOD		(cc)
#define FORM_QUADRUPED		(dd)
 
/* body parts */
#define PART_HEAD               (A)
#define PART_ARMS               (B)
#define PART_LEGS               (C)
#define PART_HEART              (D)
#define PART_BRAINS             (E)
#define PART_GUTS               (F)
#define PART_HANDS              (G)
#define PART_FEET               (H)
#define PART_FINGERS            (I)
#define PART_EAR                (J)
#define PART_EYE		(K)
#define PART_LONG_TONGUE        (L)
#define PART_EYESTALKS          (M)
#define PART_TENTACLES          (N)
#define PART_FINS               (O)
#define PART_WINGS              (P)
#define PART_TAIL               (Q)
/* for combat */
#define PART_CLAWS              (U)
#define PART_FANGS              (V)
#define PART_HORNS              (W)
#define PART_SCALES             (X)
#define PART_TUSKS		(Y)


/* bits for cdata */
#define CDATA_ALIDRU		(A)
#define CDATA_EOLODEU		(B)
#define CDATA_IRAL		(C)
#define CDATA_IABESAO		(D)
#define CDATA_THIEF		(E)

/* Bits to 'paffected_by' */
#define AFF_AIRLESS		(A)
#define AFF_SHARP_VISION	(B)
#define AFF_SENSORY_VISION	(C)
#define AFF_VOIDWALK		(D)
#define AFF_CRYSTALIZE_MAGIC	(E)
#define AFF_OBSCURE_EVIDENCE	(F)
#define AFF_MUTE		(G)
#define AFF_AURA_OF_CORRUPTION  (H)
#define AFF_SHROUD_OF_NYOGTHUA  (I)
#define AFF_ENDURE		(J)
#define AFF_DASH		(K)
#define AFF_TWOHAND		(L)
#define AFF_TRUESHOT		(M)
#define AFF_MAUL		(O)

/* Bits to 'oaffected_by' */
#define AFF_ENCAMP		(A)
#define AFF_SHADOWMASTERY       (B)
#define AFF_ENCASE		(C)
#define AFF_RITEOFTHESUN	(D)
#define AFF_INSPIRE		(E)
#define AFF_PETRIFY		(F)
#define AFF_BEASTFORM		(G)
#define AFF_MANTLEOFFEAR	(H)
#define AFF_EYEFOCUS		(I)
#define AFF_BLINK		(J)
#define AFF_DISGUISE		(K)
#define AFF_PARANOIA		(L)
#define AFF_SYMBIONT		(M)
#define AFF_READTHOUGHTS	(N)
#define AFF_SYM_TARGET		(O)
#define AFF_COVEN		(P)
#define AFF_BLEEDING		(Q)
#define AFF_BURNOUT		(R)
#define AFF_NOVA_CHARGE		(S)
#define AFF_CONSUMPTION		(T)
#define AFF_DEMONPOS            (U)
#define AFF_POSCHAN             (V)
#define AFF_RADIANCE            (W)
#define AFF_INSCRIBE		(X)
#define AFF_DEAFEN		(Y)
#define AFF_GHOST		(Z)
#define AFF_UNCLEANSPIRIT	(aa)
#define AFF_DARKFUTURE		(bb)
#define AFF_ONEHANDED		(cc)
#define AFF_NIGHTFEARS		(dd)
#define AFF_DOPPEL		(ee)

/*
 *
 * Bits for 'naffected_by'.
 * No provision is made for use on mobs, this
 * is strictly for 2-stage effects.
 * (ie, avatar wears off, but you can't cast it for 24 hrs)
 *
 */
#define AFF_AVATAR		(A)
#define AFF_FLESHTOSTONE	(B)
#define AFF_GUARDING		(C)
#define AFF_CLOUDKILL		(D)
#define AFF_RALLY		(E)
#define AFF_FORCEDMARCH		(F)
#define AFF_SCOUTING		(G)
#define AFF_LARVA		(H)
#define AFF_ARMSHATTER		(I)
#define AFF_LEGSHATTER		(J)
#define AFF_GRAPPLE		(K)
#define AFF_RAGE		(L)
#define AFF_FURY		(M)
#define AFF_ASHURMADNESS	(N)
#define AFF_GARROTE_VICT	(O)
#define AFF_GARROTE_CH  	(P)
#define AFF_GARROTE  		(Q) /* garroted for her pleasure */
#define AFF_MUFFLE  		(R)
#define AFF_TREMBLE  		(S)
#define AFF_DELUSION  		(T) /* thats the poison, not the spell */
#define AFF_LIGHTSLEEP		(U)
#define AFF_WARINESS		(V)
#define AFF_FLASHPOWDER		(W)
#define AFF_BOLO_ARM		(X)
#define AFF_BOLO_LEG		(Y)
#define AFF_BOLO		(Z)
#define AFF_PURSUE		(aa)
#define AFF_AGILITY		(bb)
#define AFF_SUBMISSION_CH	(cc)
#define AFF_SUBMISSION_VICT	(dd)
#define AFF_SUBDUE		(ee)

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */

#define AFF_BLIND		(A)
#define AFF_INVISIBLE		(B)
#define AFF_DETECT_EVIL		(C)
#define AFF_DETECT_INVIS	(D)
#define AFF_FLY_NATURAL		(E)
#define AFF_DETECT_HIDDEN	(F)
#define AFF_DETECT_GOOD		(G)
#define AFF_SANCTUARY		(H)
#define AFF_FAERIE_FIRE		(I)
#define AFF_INFRARED		(J)
#define AFF_CURSE		(K)
#define AFF_WIZI		(L)
#define AFF_POISON		(M)
#define AFF_PROTECT_EVIL	(N)
#define AFF_PROTECT_GOOD	(O)
#define AFF_SNEAK		(P)
#define AFF_HIDE		(Q)
#define AFF_SLEEP		(R)
#define AFF_CHARM		(S)
#define AFF_FLYING		(T)
#define AFF_PASS_DOOR		(U)
#define AFF_HASTE		(V)
#define AFF_CALM		(W)
#define AFF_PLAGUE		(X)
#define AFF_WEAKEN		(Y)
#define AFF_DARK_VISION		(Z)
#define AFF_BERSERK		(aa)
#define AFF_SWIM		(bb)
#define AFF_REGENERATION        (cc)
#define AFF_SLOW		(dd)
#define AFF_DETECT_WIZI		(ee)
#define AFF_CHARM_FREEAGENT (ff)

#define TRAP_NET		10;
#define TRAP_BOULDER		20;
#define TRAP_LOG		30;
#define TRAP_WATER		40;

/*
 * Sex.
 * Used in #MOBILES.
 */
#define SEX_NEUTRAL		      0
#define SEX_MALE		      1
#define SEX_FEMALE		      2

/* AC types */
#define AC_PIERCE			0
#define AC_BASH				1
#define AC_SLASH			2
#define AC_EXOTIC			3

/* dice */
#define DICE_NUMBER			0
#define DICE_TYPE			1
#define DICE_BONUS			2

/* size */
#define SIZE_TINY			0
#define SIZE_SMALL			1
#define SIZE_MEDIUM			2
#define SIZE_LARGE			3
#define SIZE_HUGE			4
#define SIZE_GIANT			5



/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
#define OBJ_VNUM_SILVER_ONE	      1
#define OBJ_VNUM_GOLD_ONE	      2
#define OBJ_VNUM_GOLD_SOME	      3
#define OBJ_VNUM_SILVER_SOME	      4
#define OBJ_VNUM_COINS		      5

#define AREA_VNUM_SHARGOB	     43
#define AREA_VNUM_MINES		     79
#define OBJ_VNUM_CORPSE_NPC	     10
#define OBJ_VNUM_CORPSE_PC	     11
#define OBJ_VNUM_SEVERED_HEAD	     12
#define OBJ_VNUM_TORN_HEART	     13
#define OBJ_VNUM_SLICED_ARM	     14
#define OBJ_VNUM_SLICED_LEG	     15
#define OBJ_VNUM_GUTS		     16
#define OBJ_VNUM_BRAINS		     17
#define OBJ_VNUM_TAIL		     18
#define OBJ_VNUM_HERB		     68
#define OBJ_VNUM_BOUNTYOLD	     67
#define OBJ_VNUM_BLOOD_SIGIL		191
#define OBJ_VNUM_BOUNTYGRAPE         193
#define OBJ_VNUM_BOUNTYDEWBERRIES       194
#define OBJ_VNUM_BOUNTYACORNS        195
#define OBJ_VNUM_BOUNTYPORPH         196
#define OBJ_VNUM_BOUNTYPRICKLY       197
#define OBJ_VNUM_BOUNTYCATTAIL       198
#define OBJ_VNUM_BOUNTYBLACKBERRIES  199
#define OBJ_VNUM_BOUNTYFIDDLEHEADS   104
#define OBJ_VNUM_BOUNTYLOTUS	     105
#define OBJ_VNUM_BOUNTYCHANTERELLE	     106
#define OBJ_VNUM_BOUNTYYUCCA		     107 
#define OBJ_VNUM_BOUNTYTUBER		     108
#define OBJ_VNUM_BOUNTYWATERCRESS	     109
#define OBJ_VNUM_BOUNTYKELP		     131
#define OBJ_VNUM_BOUNTYDANDELION	     132
#define OBJ_VNUM_MMM_MEAT	     133
#define OBJ_VNUM_MUSHROOM	     20
#define OBJ_VNUM_FOODSTART	     170
#define OBJ_VNUM_FOODCOUNT	     20
#define OBJ_VNUM_LIGHT_BALL	     21
#define OBJ_VNUM_SPRING		     22
#define OBJ_VNUM_DISC		     23
#define OBJ_VNUM_PORTAL		     25
#define OBJ_VNUM_PORTAL_NEXUS	     25
#define OBJ_VNUM_ILLUSION	     40
#define OBJ_VNUM_RAIDER_BELT	     95
#define OBJ_VNUM_RELIEF_1	     12105
#define OBJ_VNUM_RELIEF_2	     12106
#define OBJ_VNUM_RELIEF_3	     12107
#define OBJ_VNUM_RELIEF_4	     12108

#define OBJ_VNUM_HEIRLOOM	    56

#define OBJ_VNUM_FIRST_ARROW       100
#define OBJ_VNUM_RANGER_BOW        99
#define OBJ_VNUM_RANGER_SPEAR	   98
#define OBJ_VNUM_RANGER_MACE	   126
#define OBJ_VNUM_RANGER_NET	   127
#define OBJ_VNUM_RANGER_ROPE	   128
#define OBJ_VNUM_RANGER_ARROW	   129
#define OBJ_VNUM_FIREKIT	   130

#define OBJ_VNUM_ROSE		   89 

#define OBJ_VNUM_TROPHY_FINGER		110
#define OBJ_VNUM_TROPHY_EAR		111
#define OBJ_VNUM_TROPHY_HEAD		112
#define OBJ_VNUM_TROPHY_TOOTH		113
#define OBJ_VNUM_TROPHY_HAIR		114
#define OBJ_VNUM_TROPHY_SKULL		115
#define OBJ_VNUM_TROPHY_HAND		117
#define OBJ_VNUM_TROPHY_ENTRAILS	118
#define OBJ_VNUM_TROPHY_TONGUE		119
#define OBJ_VNUM_TROPHY_HEART		122
#define OBJ_VNUM_TROPHY_BRAINS		124
#define OBJ_VNUM_TROPHY_EYE		125

#define OBJ_VNUM_FAMILIAR_SERPENT   144
#define OBJ_VNUM_FAMILIAR_TOAD      145
#define OBJ_VNUM_FAMILIAR_BAT       146
#define OBJ_VNUM_FAMILIAR_RAVEN     147
#define OBJ_VNUM_FAMILIAR_FOX       148
#define OBJ_VNUM_FAMILIAR_CAT       149

#define OBJ_VNUM_SCHOOL_MACE	   3762
#define OBJ_VNUM_SCHOOL_DAGGER	   3763
#define OBJ_VNUM_SCHOOL_SWORD	   3761
#define OBJ_VNUM_SCHOOL_SPEAR	   3717
#define OBJ_VNUM_SCHOOL_STAFF	   3783
#define OBJ_VNUM_SCHOOL_AXE	   3764
#define OBJ_VNUM_SCHOOL_FLAIL	   3720
#define OBJ_VNUM_SCHOOL_WHIP	   3721
#define OBJ_VNUM_SCHOOL_POLEARM    3722
#define OBJ_VNUM_SCHOOL_KNIFE	   3723

#define OBJ_VNUM_SCHOOL_FLUTE	   3724

#define OBJ_VNUM_SCHOOL_VEST	   3700
#define OBJ_VNUM_SCHOOL_SHIELD	   3765
#define OBJ_VNUM_SCHOOL_BANNER     3766
#define OBJ_VNUM_MAP		   3162

#define OBJ_VNUM_WHISTLE	   2116

#define OBJ_VNUM_HIDE_CAP	   26
#define OBJ_VNUM_HIDE_VEST	   27
#define OBJ_VNUM_HIDE_GLOVES	   28
#define OBJ_VNUM_HIDE_BOOTS	   29
#define OBJ_VNUM_HIDE_SLEEVES	   30
#define OBJ_VNUM_HIDE_LEGGINGS	   31
#define OBJ_VNUM_HIDE_BRACER	   32
#define OBJ_VNUM_HIDE_BELT	   33
#define OBJ_VNUM_HIDE_COLLAR	   34
#define OBJ_VNUM_HIDE_COAT	   35
#define OBJ_VNUM_RANGER_SPRING	   36

#define OBJ_VNUM_STONE_POWER	12000
#define OBJ_VNUM_RANGER_FOOD    37
#define OBJ_VNUM_ICY_SHIELD	43
#define OBJ_VNUM_LIFE_SHIELD	44
#define OBJ_VNUM_MANTLE_EARTH	48
#define OBJ_VNUM_ICY_PRISON	49
#define OBJ_VNUM_MAZE		167
#define OBJ_VNUM_NATURAL_ARMOR 69
#define OBJ_VNUM_MAGIC_CRYSTAL	50
#define OBJ_VNUM_FETISH		96
#define OBJ_VNUM_SPIRIT_STONE	42
#define OBJ_VNUM_CATALYST_VIAL	45
#define OBJ_VNUM_POISON_VIAL	46
#define OBJ_VNUM_POISON_DART	47
#define OBJ_VNUM_RUNIC_MESSAGE	90
#define OBJ_VNUM_LIGHT_WEAPON    94
#define OBJ_VNUM_FIRE_SPEAR	168
#define OBJ_VNUM_SOUL_REAVER	97

#define OBJ_VNUM_INSCRIBE_POUCH  82
#define OBJ_VNUM_REAGENT_SUMMON  83
#define OBJ_VNUM_REAGENT_BIND    84
#define OBJ_VNUM_REAGENT_PROTECT 85
#define OBJ_VNUM_SYMBOL_SUMMON   201
#define OBJ_VNUM_SYMBOL_BIND     200
#define OBJ_VNUM_SYMBOL_PROTECT  202

#define OBJ_VNUM_REAGENT_SILVER   85
#define OBJ_VNUM_REAGENT_BONEDUST 83
#define OBJ_VNUM_REAGENT_CHARCOAL 84
#define OBJ_VNUM_REAGENT_BLOOD    232
#define OBJ_VNUM_REAGENT_SALT     238
#define OBJ_VNUM_REAGENT_MUD      234
#define OBJ_VNUM_REAGENT_HERB1    235
#define OBJ_VNUM_REAGENT_HERB2    236
#define OBJ_VNUM_REAGENT_HERB3    237
#define OBJ_VNUM_REAGENT_BONES    233

#define OBJ_VNUM_BLOODVIAL	239
#define OBJ_VNUM_VOID_FRUIT	240

#define OBJ_VNUM_CAMPFIRE	3609
#define OBJ_VNUM_PYRE       251
#define OBJ_VNUM_NIGHTFLAMELASH 252
#define OBJ_VNUM_BURNINGWISP    253

#define OBJ_VNUM_QUINTESSENCE       226
#define OBJ_VNUM_POSTERN            227
#define OBJ_VNUM_CHALICEOFTRUTH     228
#define OBJ_VNUM_CHAPLETOFFERVOR    229
#define OBJ_VNUM_CELESTIALBULWARK   230
#define OBJ_VNUM_ETHEREALSKEAN      231
#define OBJ_VNUM_LENSOFDIVINATION   255
#define OBJ_VNUM_RODOFTRANSCENDENCE 256
#define OBJ_VNUM_DISTILLEDPOTION    257

#define OBJ_VNUM_WELLSPRING         268

#define OBJ_VNUM_CRAFTILLUSION      269

#define OBJ_VNUM_STONECRAFT         270
#define OBJ_VNUM_FORGEWEAPON        271
#define OBJ_VNUM_WATERWHEEL         272
#define OBJ_VNUM_WATERWHEEL_REMAINS 273
#define OBJ_VNUM_QUARRYGEM          274
#define OBJ_VNUM_QUARRYARMOR        275

#define OBJ_VNUM_DARKTALLOW         279
#define OBJ_VNUM_BONESCYTHE         280
#define OBJ_VNUM_BONESICKLE         281

#define OBJ_VNUM_SYMBOL_FIRST     200
#define OBJ_VNUM_SYMBOL_LAST      231

#define OBJ_VNUM_POTION_CONTAINER 370
#define OBJ_VNUM_ADD_FIRST	  371
#define OBJ_VNUM_ADD_EXPLOSIVE 	  371
#define OBJ_VNUM_ADD_ADHESIVE     372
#define OBJ_VNUM_ADD_ANESTHETIC   373
#define OBJ_VNUM_ADD_TOXIN 	  374
#define OBJ_VNUM_ADD_SUPPRESSIVE  375
#define OBJ_VNUM_ADD_LAST	  375

#define INSMAT_SILVER		1
#define INSMAT_BONEDUST		2
#define INSMAT_CHARCOAL		3
#define INSMAT_BLOOD_ANIMAL	4
#define INSMAT_BLOOD_UNDEAD	5
#define INSMAT_BLOOD_ALATHARYA	6
#define INSMAT_BLOOD_CELESTIAL	7
#define INSMAT_BLOOD_CASTER	8
#define INSMAT_SALT		9
#define INSMAT_MUD		10
#define INSMAT_HERB1		11
#define INSMAT_HERB2		12
#define INSMAT_HERB3		13
#define INSMAT_BONES_UNDEAD	14
#define INSMAT_BONES_DRAGON	15
#define INSMAT_BLOOD_CHTAREN	16


#define MOB_VNUM_TRAP		   1212
#define MOB_VNUM_PUNJIPIT	   101
#define MOB_VNUM_TENTACLES	90
#define MOB_VNUM_ASHUR_EVIL	22
#define MOB_VNUM_SENTRY		77
#define MOB_VNUM_WATCHER_GUARD  31
#define MOB_VNUM_CASTER		110

#define MOB_VNUM_COLD		36
#define MOB_VNUM_HEAT		37
#define MOB_VNUM_HAIL		38
#define MOB_VNUM_LIGHTNING	39

#define MOB_VNUM_SILVER_VEIL    79 

#define MOB_VNUM_BLACKBEAR	1
#define MOB_VNUM_GREYWOLF	2
#define MOB_VNUM_ASP		3
#define MOB_VNUM_HAWK		4
#define MOB_VNUM_ALLIGATOR	14
#define MOB_VNUM_SHARK		102
#define MOB_VNUM_DIREWOLF	110
#define MOB_VNUM_BADGER		120
#define MOB_VNUM_RAVEN		157
#define MOB_VNUM_SEEDLING_GAMALOTH	103
#define MOB_VNUM_LION		192
#define MOB_VNUM_RAM		193
#define MOB_VNUM_BARRACUDA	194
#define MOB_VNUM_PANTHER	195
#define MOB_VNUM_PYTHON		196
#define MOB_VNUM_SRORSIAN	197
#define MOB_VNUM_BJORCHA	198
#define MOB_VNUM_RAT		199

#define MOB_VNUM_FIREELEMENTAL 205
#define MOB_VNUM_LAVAELEMENTAL 206
#define MOB_VNUM_PYROKINETICMIRROR 207
#define MOB_VNUM_SALAMANDER     225

#define MOB_VNUM_GENERALPURPOSE 208
#define MOB_VNUM_SHADE          209
#define MOB_VNUM_BATTLESERAPH   218
#define MOB_VNUM_BILOCATIONBODY 219

#define MOB_VNUM_MIRROROFSOULS  239

#define MOB_VNUM_DRAKE          240

#define MOB_VNUM_BARROWMISTZOMBIE 249
#define MOB_VNUM_BARROWMISTWIGHT  262

#define MOB_VNUM_FOGELEMENTAL   269

#define MOB_VNUM_PHANTASMALMIRROR   376
#define MOB_VNUM_WEAVEILLUSION      377

#define MOB_VNUM_CLOCKWORKGOLEM     378

#define MOB_VNUM_FAMILIAR_RAVEN     50
#define MOB_VNUM_FAMILIAR_SERPENT   52
#define MOB_VNUM_FAMILIAR_FOX       53
#define MOB_VNUM_FAMILIAR_BAT       54
#define MOB_VNUM_FAMILIAR_TOAD      55
#define MOB_VNUM_FAMILIAR_CAT       56

#define MOB_VNUM_DRUID_RABBIT   10
#define MOB_VNUM_DRUID_OWL	11
#define MOB_VNUM_DRUID_FOX	12

#define MOB_VNUM_GUARD		16
#define MOB_VNUM_ZOMBIE		20
#define MOB_VNUM_CHIRURGEON_ZOMBIE 175
#define MOB_VNUM_SNAKE		100

#define MOB_VNUM_SHUNNED_DEMON  44

#define MOB_VNUM_AVENGING_SERAPH 42
#define MOB_VNUM_ASTRAL_PROJECTION 43
#define MOB_VNUM_WATER_ELEMENTAL 21
#define MOB_VNUM_EARTH_ELEMENTAL 33
#define MOB_VNUM_DELAYED_FIREBALL 29
#define MOB_VNUM_AIR_EFREET	 30
#define MOB_VNUM_CLOUDKILL	40
#define MOB_VNUM_RETAINER	78
#define MOB_VNUM_MIRROR_IMAGE	93
#define MOB_VNUM_EYE_RUNE	94
#define MOB_VNUM_BIRD_PREY	95
#define MOB_VNUM_PILLAGE_FIRE	99
#define MOB_VNUM_PILLAGE_BANDIT_MIN 96
#define MOB_VNUM_PILLAGE_BANDIT_MAX 98
#define MOB_VNUM_DEMON_IDCIZON 	115
#define MOB_VNUM_DEMON_VERSHAK 	116
#define MOB_VNUM_DEMON_GRMLARLOTH 117
#define MOB_VNUM_DEMON_SUCCUBI	118
#define MOB_VNUM_DEMON_LOGOR	119
#define MOB_VNUM_DEMON_SCION	113
#define MOB_VNUM_SILVER_DEMON_MIN 121
#define MOB_VNUM_SILVER_DEMON_MAX 130

#define MOB_VNUM_DEMON_GUARDIAN		238

/*
 * Item types.
 * Used in #OBJECTS.
 */
#define ITEM_LIGHT		      1
#define ITEM_SCROLL		      2
#define ITEM_WAND		      3
#define ITEM_STAFF		      4
#define ITEM_WEAPON		      5
#define ITEM_TREASURE		      8
#define ITEM_ARMOR		      9
#define ITEM_POTION		     10
#define ITEM_CLOTHING		     11
#define ITEM_FURNITURE		     12
#define ITEM_TRASH		     13
#define ITEM_CONTAINER		     15
#define ITEM_DRINK_CON		     17
#define ITEM_KEY		     18
#define ITEM_FOOD		     19
#define ITEM_MONEY		     20
#define ITEM_BOAT		     22
#define ITEM_CORPSE_NPC		     23
#define ITEM_CORPSE_PC		     24
#define ITEM_FOUNTAIN		     25
#define ITEM_PILL		     26
#define ITEM_PROTECT		     27
#define ITEM_MAP		     28
#define ITEM_PORTAL		     29
#define ITEM_WARP_STONE		     30
#define ITEM_ROOM_KEY		     31
#define ITEM_GEM		     32
#define ITEM_JEWELRY		     33
#define ITEM_JUKEBOX		     34
#define ITEM_INSTRUMENT		     35
#define ITEM_NET		     36
#define ITEM_ARROW                   37
#define ITEM_BOW		     38
#define ITEM_SPECIAL		     39
#define ITEM_WRITING		     40
#define ITEM_OIL		     41
#define ITEM_POTIONCONTAINER	     42

/*
 * Special object types.
 * Used with ITEM_SPECIAL
 */

#define SOBJ_CHESS		1
#define SOBJ_CARDS		2

/*
 * Trap types, for thief traps.
 */

#define TRAP_DAMAGE		0
#define TRAP_GREASE		1

#define FOUNT_EMPTY		(A)
/*
 * Extra flags.
 * Used in #OBJECTS.
 */
#define ITEM_GLOW		(A)
#define ITEM_HUM		(B)
#define ITEM_DARK		(C)
#define ITEM_WARM		(D)
#define ITEM_EVIL		(E)
#define ITEM_INVIS		(F)
#define ITEM_MAGIC		(G)
#define ITEM_NODROP		(H)

#define ITEM_BLESS		(I)
#define ITEM_ANTI_GOOD		(J)
#define ITEM_ANTI_EVIL		(K)
#define ITEM_ANTI_NEUTRAL	(L)
#define ITEM_NOREMOVE		(M)
#define ITEM_INVENTORY		(N)
#define ITEM_NOPURGE		(O)
#define ITEM_ROT_DEATH		(P)

#define ITEM_VIS_DEATH		(Q)
#define ITEM_AFFINITY		(R)
#define ITEM_NONMETAL		(S)
#define ITEM_NOLOCATE		(T)
#define ITEM_MELT_DROP		(U)
#define ITEM_HAD_TIMER		(V)
#define ITEM_SELL_EXTRACT	(W)
#define ITEM_QUEST		(X)

#define ITEM_BURN_PROOF		(Y)
#define ITEM_NOUNCURSE		(Z)
#define ITEM_NODESTROY		(aa)
#define ITEM_HIDDEN		(bb)
#define ITEM_STASHED		(cc)
#define ITEM_WIZI		(dd)
#define ITEM_NOLONG		(ee)
#define ITEM_NODISARM		(ff)

/* Extra extra flags */
#define ITEM_FIRE	        (A)
#define ITEM_NOMORTAL       (B)
#define ITEM_QUINTESSENCE   (C)
#define ITEM_INCENSE        (D)
#define ITEM_ANNOINTINGOIL  (E)
#define ITEM_WINDFALL       (F)
#define ITEM_NOMIRROR       (G)

#define CORPSE_MISSING_FINGER1	(A)
#define CORPSE_MISSING_FINGER2	(B)
#define CORPSE_MISSING_EAR1	(C)
#define CORPSE_MISSING_EAR2	(D)
#define CORPSE_MISSING_HEAD	(E)
#define CORPSE_DESTROYED	(F)
#define CORPSE_FETISHED		(G)
#define CORPSE_MISSING_HAND1	(H)
#define CORPSE_MISSING_HAND2	(I)
#define CORPSE_MISSING_ENTRAILS (J)
#define CORPSE_MISSING_TONGUE	(K)
#define CORPSE_MISSING_HEART	(L)
#define CORPSE_MISSING_BRAINS	(M)
#define CORPSE_MISSING_EYE1     (N)
#define CORPSE_MISSING_EYE2     (O)
#define CORPSE_POISONED		(P)
#define CORPSE_MISSING_TEETH	(Q)
#define CORPSE_MISSING_HAIR	(R)
/* weapon class */
#define WEAPON_EXOTIC		0
#define WEAPON_SWORD		1
#define WEAPON_DAGGER		2
#define WEAPON_SPEAR		3
#define WEAPON_MACE		4
#define WEAPON_AXE		5
#define WEAPON_FLAIL		6
#define WEAPON_WHIP		7	
#define WEAPON_POLEARM		8
#define WEAPON_STAFF		9
#define WEAPON_KNIFE		10

/* weapon types */
#define WEAPON_FLAMING		(A)
#define WEAPON_FROST		(B)
#define WEAPON_VAMPIRIC		(C)
#define WEAPON_SHARP		(D)
#define WEAPON_VORPAL		(E)
#define WEAPON_TWO_HANDS	(F)
#define WEAPON_SHOCKING		(G)
#define WEAPON_POISON		(H)
#define WEAPON_HEALING		(I)

/* gate flags */
#define GATE_NORMAL_EXIT	(A)
#define GATE_NOCURSE		(B)
#define GATE_GOWITH		(C)
#define GATE_BUGGY		(D)
#define GATE_RANDOM		(E)

// Light flags
#define LIGHT_ALWAYS_BURN   (A)

/* furniture flags */
#define STAND_AT		(A)
#define STAND_ON		(B)
#define STAND_IN		(C)
#define SIT_AT			(D)
#define SIT_ON			(E)
#define SIT_IN			(F)
#define REST_AT			(G)
#define REST_ON			(H)
#define REST_IN			(I)
#define SLEEP_AT		(J)
#define SLEEP_ON		(K)
#define SLEEP_IN		(L)
#define PUT_AT			(M)
#define PUT_ON			(N)
#define PUT_IN			(O)
#define PUT_INSIDE		(P)




/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
#define APPLY_NONE		      0
#define APPLY_STR		      1
#define APPLY_DEX		      2
#define APPLY_INT		      3
#define APPLY_WIS		      4
#define APPLY_CON		      5
#define APPLY_SEX		      6
#define APPLY_CLASS		      7
#define APPLY_LEVEL		      8
#define APPLY_AGE		      9
#define APPLY_HEIGHT		     10
#define APPLY_WEIGHT		     11
#define APPLY_MANA		     12
#define APPLY_HIT		     13
#define APPLY_MOVE		     14
#define APPLY_GOLD      	 15
#define APPLY_EXP            16
#define APPLY_AC		     17
#define APPLY_HITROLL		     18
#define APPLY_DAMROLL		     19
#define APPLY_SAVES		     20
#define APPLY_SAVING_PARA	     20
#define APPLY_SAVING_ROD	     21
#define APPLY_SAVING_PETRI	     22
#define APPLY_SAVING_BREATH	     23
#define APPLY_SAVING_SPELL	     24
#define APPLY_SPELL_AFFECT	     25
#define APPLY_FORM		     26
#define APPLY_HIDE		     27
#define APPLY_RANGE                  28
#define APPLY_RESIST_SUMMON	     29
#define APPLY_RESIST_CHARM	     30
#define APPLY_RESIST_MAGIC           31
#define APPLY_RESIST_WEAPON          32
#define APPLY_RESIST_BASH            33
#define APPLY_RESIST_PIERCE          34     
#define APPLY_RESIST_SLASH           35     
#define APPLY_RESIST_FIRE            36     
#define APPLY_RESIST_COLD            37     
#define APPLY_RESIST_LIGHTNING       38     
#define APPLY_RESIST_ACID            39    
#define APPLY_RESIST_POISON          40     
#define APPLY_RESIST_NEGATIVE        41     
#define APPLY_RESIST_HOLY            42     
#define APPLY_RESIST_ENERGY          43     
#define APPLY_RESIST_MENTAL          44     
#define APPLY_RESIST_DISEASE         45     
#define APPLY_RESIST_DROWNING        46      
#define APPLY_RESIST_LIGHT	     47
#define APPLY_RESIST_SOUND	     48
#define APPLY_RESIST_ILLUSION        49
#define APPLY_RESIST_DEFILEMENT      50    
#define APPLY_RESIST_FEAR            51     
#define APPLY_RESIST_IRON            52   
#define APPLY_SIZE		     53
#define APPLY_CHR		     54
#define APPLY_SKILL		     55
#define APPLY_MAXSTR		     56
#define APPLY_MAXDEX		     57
#define APPLY_MAXINT		     58
#define APPLY_MAXWIS		     59
#define APPLY_MAXCON		     60
#define APPLY_MAXCHR		     61
#define APPLY_SCANRANGE		     62
#define APPLY_LUCK          63
#define APPLY_NONE_2		0
#define APPLY_STAT_2		1
#define APPLY_MAX_STAT_2	2
#define APPLY_SEX_2			3
#define APPLY_AGE_2			4
#define APPLY_HEIGHT_2		5
#define APPLY_WEIGHT_2		6
#define APPLY_MANA_2		7
#define APPLY_HP_2			8
#define APPLY_MOVES_2		9
#define APPLY_HITROLL_2		10
#define APPLY_DAMROLL_2		11
#define APPLY_SAVES_2		12
#define APPLY_RANGE_2		13
#define APPLY_RESISTANCE_2	14
#define APPLY_MATERIAL_RESISTANCE_2 15
#define APPLY_SIZE_2		16
#define APPLY_SKILL_2		17
#define APPLY_GLOW_2		18

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE		      1
#define CONT_PICKPROOF		      2
#define CONT_CLOSED		      4
#define CONT_LOCKED		      8
#define CONT_PUT_ON		     16



/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
#define ROOM_VNUM_LIMBO		      2
#define ROOM_VNUM_AREAS		      8
#define ROOM_VNUM_CHAT		   1200
#define ROOM_VNUM_TEMPLE	   3499
#define ROOM_VNUM_ALTAR		   3499
#define ROOM_VNUM_SCHOOL	   3700
#define ROOM_VNUM_BALANCE	   4500
#define ROOM_VNUM_CIRCLE	   4400
#define ROOM_VNUM_DEMISE	   4201
#define ROOM_VNUM_HONOR		   4300
#define ROOM_VNUM_VOID		   7766
#define ROOM_VNUM_PUNJI_LOW	   101
#define ROOM_VNUM_PUNJI_HIGH	   120


/* House vault vnums */

#define ROOM_VAULT_CHAMPION	 	12067
#define ROOM_VAULT_GUARDIAN		12011
#define ROOM_VAULT_RAIDER		12119
#define ROOM_VAULT_CONQUEST		12160
#define ROOM_VAULT_SHUNNED		12202

#define MIN_VNUM_NATUREGATE		10
#define MAX_VNUM_NATUREGATE		26
#define ROOM_VNUM_NATUREGATE		10
#define MIN_VNUM_G_NATUREGATE		140
#define MAX_VNUM_G_NATUREGATE		152
#define ROOM_VNUM_G_NATUREGATE		144

/*
 * Room flags.
 * Used in #ROOMS.
 */
#define ROOM_DARK		(A)
#define ROOM_NOGATE		(B)
#define ROOM_NO_MOB		(C)
#define ROOM_INDOORS		(D)
#define ROOM_NOSUM_TO		(E)
#define ROOM_NOSUM_FROM		(F)
#define ROOM_NONEFORYOU		(G)
#define ROOM_VAULT		(H)
#define ROOM_NOYELL		(I)
#define ROOM_PRIVATE		(J)
#define ROOM_SAFE		(K)
#define ROOM_SOLITARY		(L)
#define ROOM_PET_SHOP		(M)
#define ROOM_NO_RECALL		(N)
#define ROOM_IMP_ONLY		(O)
#define ROOM_GODS_ONLY		(P)
#define ROOM_HEROES_ONLY	(Q)
#define ROOM_NEWBIES_ONLY	(R)
#define ROOM_LAW		(S)
#define ROOM_NOWHERE		(T)
#define ROOM_NOMAGIC		(U)
#define ROOM_GUILD		(V)
#define ROOM_NOWEATHER		(W)
#define ROOM_UBERDARK		(X)
#define ROOM_POWER_NEXUS	(Y)
#define ROOM_ROUGH		(Z)
#define ROOM_HAS_WATER		(aa)
#define ROOM_SAVE		(bb)
#define ROOM_LABORATORY		(cc)
#define ROOM_ARENA		(dd)

#define ROOM_DARK_2           (A)
#define ROOM_NOGATE_2         (B)
#define ROOM_NO_MOB_2         (C)
#define ROOM_NOSUM_TO_2       (D)
#define ROOM_NOSUM_FROM_2     (E)
#define ROOM_NONEFORYOU_2     (F)
#define ROOM_VAULT_2          (G)
#define ROOM_NOYELL_2         (H)
#define ROOM_PRIVATE_2        (I)
#define ROOM_SAFE_2           (J)
#define ROOM_PET_SHOP_2       (K)
#define ROOM_NO_RECALL_2      (L)
#define ROOM_IMP_ONLY_2       (M)
#define ROOM_GODS_ONLY_2      (N)
#define ROOM_NEWBIES_ONLY_2   (O)
#define ROOM_LAW_2            (P)
#define ROOM_NOWHERE_2        (Q)
#define ROOM_NOMAGIC_2        (R)
#define ROOM_GUILD_2          (S)
#define ROOM_NOWEATHER_2      (T)
#define ROOM_UBERDARK_2       (U)
#define ROOM_POWER_NEXUS_2    (V)
#define ROOM_SAVE_2           (W)
#define ROOM_LABORATORY_2     (X)
#define ROOM_ARENA_2          (dd)

/*
 * Directions.
 * Used in #ROOMS.
 */
#define DIR_NORTH		      0
#define DIR_EAST		      1
#define DIR_SOUTH		      2
#define DIR_WEST		      3
#define DIR_UP			      4
#define DIR_DOWN		      5



/*
 * Exit flags.
 * Used in #ROOMS.
 */
#define EX_ISDOOR		      (A)    // resets on area reset
#define EX_CLOSED		      (B)    // resets on area reset
#define EX_LOCKED		      (C)    // resets on area reset
#define EX_SECRET		      (D)

#define EX_PICKPROOF		      (F)
#define EX_NOPASS		      (G)
#define EX_EASY			      (H)
#define EX_HARD			      (I)
#define EX_INFURIATING		      (J)
#define EX_NOCLOSE		      (K)
#define EX_NOLOCK		      (L)
#define EX_ILLUSION		      (M)
#define EX_WALLED		      (N)
#define EX_RUNEOFEARTH		      (O)
#define EX_WALLOFFIRE		      (P)
#define EX_NORAM		      (Q)
#define EX_FAKE			      (R)
#define EX_TRIPWIRE		      (S)
#define EX_WALLOFVINES		      (T)
#define EX_NOREFRESH		      (U)
#define EX_NOFLEE		      (V)
#define EX_RUNEOFFIRE         (W)
#define EX_WEAVEWALL          (X)
#define EX_ICEWALL            (Y)


/*
 * Sector types.
 * Used in #ROOMS.
 */
#define SECT_INSIDE		      0
#define SECT_CITY		      1
#define SECT_FIELD		      2
#define SECT_FOREST		      3
#define SECT_HILLS		      4
#define SECT_MOUNTAIN		      5
#define SECT_WATER_SWIM		      6
#define SECT_WATER_NOSWIM	      7
#define SECT_UNUSED		      8
#define SECT_AIR		      9
#define SECT_DESERT		     10
#define SECT_UNDERWATER		     11
#define SECT_UNDERGROUND	     12
#define SECT_ROAD		     13
#define SECT_SWAMP		     14
#define SECT_MAX		     15



/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE		(A)
#define ITEM_WEAR_FINGER	(B)
#define ITEM_WEAR_NECK		(C)
#define ITEM_WEAR_BODY		(D)
#define ITEM_WEAR_HEAD		(E)
#define ITEM_WEAR_LEGS		(F)
#define ITEM_WEAR_FEET		(G)
#define ITEM_WEAR_HANDS		(H)
#define ITEM_WEAR_ARMS		(I)
#define ITEM_WEAR_SHIELD	(J)
#define ITEM_WEAR_ABOUT		(K)
#define ITEM_WEAR_WAIST		(L)
#define ITEM_WEAR_WRIST		(M)
#define ITEM_WIELD		(N)
#define ITEM_HOLD		(O)
#define ITEM_NO_SAC		(P)
#define ITEM_WEAR_FLOAT		(Q)
#define ITEM_WEAR_SIGIL		(R)
#define ITEM_PROG		(S)
#define ITEM_WEAR_FAMILIAR  (T)
#define ITEM_WEAR_MAX		20 // Don't ask
/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
#define WEAR_NONE		    -1
#define WEAR_LIGHT		    0
#define WEAR_FINGER_L		1
#define WEAR_FINGER_R		2
#define WEAR_NECK_1		    3
#define WEAR_NECK_2		    4
#define WEAR_BODY		    5
#define WEAR_HEAD		    6
#define WEAR_LEGS		    7
#define WEAR_FEET		    8
#define WEAR_HANDS		    9
#define WEAR_ARMS		    10
#define WEAR_SHIELD		    11
#define WEAR_ABOUT		    12
#define WEAR_WAIST		    13
#define WEAR_WRIST_L		14
#define WEAR_WRIST_R		15
#define WEAR_WIELD		    16
#define WEAR_HOLD		    17
#define WEAR_FLOAT		    18
#define WEAR_DUAL_WIELD		19
#define WEAR_SIGIL		    20
#define WEAR_CONCEAL1		21
#define WEAR_CONCEAL2		22
#define WEAR_PROGSLOT		23
#define WEAR_FAMILIAR       24
#define MAX_WEAR		    25


#define WORN_LIGHT		(A)
#define WORN_FINGER_L		(B)
#define WORN_FINGER_R		(C)
#define WORN_NECK_1		(D)
#define WORN_NECK_2		(E)
#define WORN_BODY		(F) 
#define WORN_HEAD		(G)  
#define WORN_LEGS		(H)    
#define WORN_FEET		(I)   
#define WORN_HANDS		(J)    
#define WORN_ARMS		(K)    
#define WORN_SHIELD		(L)    
#define WORN_ABOUT		(M)    
#define WORN_WAIST		(N)    
#define WORN_WRIST_L		(O)    
#define WORN_WRIST_R		(P)
#define WORN_WIELD		(Q)
#define WORN_HOLD		(R)
#define WORN_FLOAT		(S)
#define WORN_DUAL_WIELD		(T)
#define WORN_SIGIL		(U)
#define WORN_CONCEAL1		(V)
#define WORN_CONCEAL2		(W)
#define WORN_PROGSLOT		(X)
#define WORN_FAMILIAR   (Y)
#define MAX_WORN		(Z)



/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS
*
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/

/*
 * Conditions.
 */
#define COND_DRUNK		      0
#define COND_FULL		      1
#define COND_THIRST		      2
#define COND_HUNGER		      3



/*
 * Positions.
 */
#define POS_DEAD		      0
#define POS_MORTAL		      1
#define POS_INCAP		      2
#define POS_STUNNED		      3
#define POS_SLEEPING		      4
#define POS_RESTING		      5
#define POS_SITTING		      6
#define POS_FIGHTING		      7
#define POS_STANDING		      8

#define ARROW_SHORT		      0 
#define ARROW_MEDIUM                  1
#define ARROW_LONG                    2
#define ARROW_BARBED                  3

/* number of different arrow types */
#define ARROW_TYPES                   4

/*
 * ACT bits for players.
 */
#define PLR_IS_NPC		(A)		/* Don't EVER set.	*/

/* RT auto flags */
#define PLR_AUTOATTACK		(B)
#define PLR_AUTOASSIST		(C)
#define PLR_AUTOEXIT		(D)
#define PLR_AUTOLOOT		(E)
#define PLR_AUTODES             (F)
#define PLR_AUTOGOLD		(G)
#define PLR_AUTOSPLIT		(H)
#define PLR_LEARN		(I)
#define PLR_EPNOTE		(J)
#define PLR_AUTODATE		(K)
#define PLR_AUTODEFEND		(L)

#define PLR_NOLAG		(M)
#define PLR_HOLYLIGHT		(N)
#define PLR_ACCOUNT		(O)

/* RT personal flags */
#define PLR_CANLOOT		(P)
#define PLR_NOSUMMON		(Q)
#define PLR_NOFOLLOW		(R)
/* 1 bit reserved, S */
/*
 * Colour stuff by Lope of Loping Through The MUD
 */
#define PLR_COLOUR		(T)    /* Colour Flag By Lope */

/* penalty flags */
#define PLR_PERMIT		(U)
#define PLR_SLOG		(V)
#define PLR_LOG			(W)
#define PLR_DENY		(X)
#define PLR_FREEZE		(Y)
#define PLR_NOPK		(Z)
#define PLR_REWARD		(aa)
#define PLR_INDUCT		(bb)
#define PLR_SOUND		(cc)
#define PLR_DISPLAY		(dd)
#define PLR_SHOWDAM		(ee)
#define PLR_SHOWLINES   (ff)


/* PLR flags for the nact field */

#define PLR_MERCY_DEATH		(A)
#define PLR_MERCY_BEG		(B)
#define PLR_MERCY_FORCE		(C)
#define PLR_NOWEATHER		(D)
#define PLR_NOTEIFY		    (E)
#define PLR_SHOWDATA		(F)
#define PLR_NOYELL		    (G)
#define PLR_HARDCORE		(H)
#define PLR_NOACCEPT		(I)
#define PLR_EXCOLOUR		(J)
#define PLR_AUTOPEEK		(K)
#define PLR_AUTOTRACKS		(L)
#define PLR_NEWBIE		    (M)
#define PLR_EXTRASPAM		(N)
#define PLR_AUTOOATH        (O)

/* RT comm flags -- may be used on both mobs and chars */
#define COMM_QUIET              (A)
#define COMM_DEAF            	(B)
#define COMM_NOWIZ              (C)
#define COMM_NOAUCTION          (D)
#define COMM_NOGOSSIP           (E)
#define COMM_NOQUESTION         (F)
#define COMM_NOMUSIC            (G)
#define COMM_NOCLAN		(H)
#define COMM_NOQUOTE		(I)
#define COMM_SHOUTSOFF		(J)
#define COMM_NOOOC		(K)
#define COMM_NONEWBIE		(S)	// Out of Position

/* display flags */
#define COMM_COMPACT		(L)
#define COMM_BRIEF		(M)
#define COMM_PROMPT		(N)
#define COMM_COMBINE		(O)
#define COMM_TELNET_GA		(P)
#define COMM_SHOW_AFFECTS	(Q)
#define COMM_NOGRATS		(R)

/* penalties */
#define COMM_NOEMOTE		(T)
#define COMM_NOSHOUT		(U)
#define COMM_NOTELL		(V)
#define COMM_NOCHANNELS		(W) 
#define COMM_SNOOP_PROOF	(Y)
#define COMM_AFK		(Z)
#define COMM_OOCOFF             (aa)
#define COMM_TRANSLATE		(bb)
#define COMM_XMUSIC		(cc)
#define COMM_KILLPROMPT		(dd)

/* WIZnet flags */
#define WIZ_ON			(A)
#define WIZ_TICKS		(B)
#define WIZ_LOGINS		(C)
#define WIZ_SITES		(D)
#define WIZ_LINKS		(E)
#define WIZ_DEATHS		(F)
#define WIZ_RESETS		(G)
#define WIZ_MOBDEATHS		(H)
#define WIZ_FLAGS		(I)
#define WIZ_PENALTIES		(J)
#define WIZ_SACCING		(K)
#define WIZ_LEVELS		(L)
#define WIZ_SECURE		(M)
#define WIZ_SWITCHES		(N)
#define WIZ_SNOOPS		(O)
#define WIZ_RESTORE		(P)
#define WIZ_LOAD		(Q)
#define WIZ_NEWBIE		(R)
#define WIZ_PREFIX		(S)
#define WIZ_SPAM		(T)
#define WIZ_DEBUG		(U)
#define WIZ_BUGS		(V)
#define WIZ_CHEATING		(W)
#define WIZ_ROLEPLAY            (X)
#define WIZ_COMM		(Y)
#define WIZ_TIMESTAMP		(Z)
#define WIZ_ACTIVITY		(aa)
#define WIZ_ALCHEMY_VERBOSE	(bb)
#define WIZ_ALCHEMY_BRIEF	(cc)

/*
 * extra_flag entries
 */

#define CHESS_AUTOSHOW		(A)
#define CHESS_NOCOLOUR          (B)

#define FACTION_HIT_NORMAL	-10
#define FACTION_HIT_ALLY	-5
#define FACTION_GAIN_ENEMY	8

// EP Plateau Flags
#define EP_MIN_TO_HERO		4500
#define EP_PLATEAU		5000

struct mount_data
{
    CHAR_DATA *	rider;
    bool	has_owner;
    char *	owner;
};

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct	mob_index_data
{
    MOB_INDEX_DATA *	next;
    SPEC_FUN *		spec_fun;
    SHOP_DATA *		pShop;
    GM_DATA *		gm;
    AREA_DATA *		area;		/* OLC */
    MOUNT_DATA *	mount_data;
    int		vnum;
    int		group;
    bool		new_format;
    bool		sFlag;
    int		count;
    int		total_count;
    int		killed;
    char *		player_name;
    char * 		orig_short;
    char *		short_descr;
    char * 		orig_long;
    char *		long_descr;
    char * 		orig_description;
    char *		description;
    long		act;
    long		nact;
    long		affected_by;
    int		alignment;
    int		level;
    int		hitroll;
    int		hit[3];
    int		mana[3];
    int		damage[3];
    int		ac[4];
    int 		dam_type;
    char *		dam_verb;
    long		off_flags;
    long		imm_flags;
    long		res_flags;
    long		vuln_flags;
    int		resist[MAX_RESIST];
    int		start_pos;
    int		default_pos;
    int		sex;
    int		race;
    int		class_num;
    long		wealth;
    long		form;
    long		parts;
    int		size;
    int			material;
    MPROG_DATA *        mobprogs;
    int                 progtypes;
    int			assist_vnum[MAX_ASSIST_VNUM];
    long		lang_flags;
    unsigned int factionNumber;
};

// When updating mob_index_data, remember to update medit_copy if neccessary!

struct gm_data
{
GM_DATA *next;
int	sn;
};

/* memory settings */
#define MEM_CUSTOMER	A	
#define MEM_SELLER	B
#define MEM_HOSTILE	C
#define MEM_AFRAID	D

struct  prog_vardata
{
    PROG_VARDATA *  next;
    char *	    id;
    char *	    value;
};

struct	prog_rundata
{
    char *	    cmnd;
    CHAR_DATA *	    mob;
    CHAR_DATA *	    actor;
    OBJ_DATA *	    obj;
    ROOM_INDEX_DATA * room;
    void *	    vo;
    CHAR_DATA *	    rndm;
    long rndmID;
    char *	    txt;
    LOOP_DATA *	    curloop;
    PROG_VARDATA *  vars;
};

struct ProgState
{
    char * data_marker;
    MPROG_DATA * data_prog;
};

/* Mob Prog Data */
struct  mob_prog_act_list
{
    MPROG_ACT_LIST * next;
    char *           buf;
    CHAR_DATA *      ch;
    OBJ_DATA *       obj;
    void *           vo;
};
 
struct  mob_prog_data
{
    MPROG_DATA *next;
    int         type;
    char *      arglist;
    char *      comlist;
    bool	exact_match;
};
   
/* Object Prog Data */
struct  obj_prog_act_list
{
    OPROG_ACT_LIST * next;
    char *           buf;
    CHAR_DATA *      ch;
    OBJ_DATA *       obj;
    void *           vo;
};

struct  loop_data
{
    LOOP_DATA * prev;
    int		value;
    int		upper;
    int		lower;
    char *	ptr;
    int	depth;
    char *      endprog;
};
 
#define ERROR_PROG        -1
#define IN_FILE_PROG       0
#define ACT_PROG           1
#define SPEECH_PROG        2
#define RAND_PROG          4
#define FIGHT_PROG         8
#define DEATH_PROG        16
#define HITPRCNT_PROG     32
#define ENTRY_PROG        64
#define GREET_PROG       128
#define ALL_GREET_PROG   256
#define GIVE_PROG        512
#define BRIBE_PROG      1024
#define EXIT_PROG       2048
#define WEAR_PROG	4096
#define REMOVE_PROG	8192
#define DEMON_PROG	16384
#define TIME_PROG	32768
#define LOAD_PROG	65536
#define TAKE_PROG	131072
#define ALL_DEATH_PROG	262144
#define VERB_PROG	524288
#define TRIGGER_PROG	1048576
#define SAC_PROG        2097152
#define DATA_PROG	(W)
#define HIT_PROG	(X)
#define HAIL_PROG	(Y)
#define ATTACK_PROG	(Z)
#define TICK_PROG	(aa)
#define HOUR_PROG	(bb)
#define EAT_PROG	(cc)
#define DRINK_PROG	(dd)
#define SUB_PROG    (ee)

/*
 * One character (PC or NPC).
 */
struct	char_data
{
    CHAR_DATA *		next;
    CHAR_DATA *		next_in_room;
    CHAR_DATA *		master;
    CHAR_DATA *		leader;
    CHAR_DATA *		fighting;
    CHAR_DATA *		reply;
    CHAR_DATA *     oocreply;
    CHAR_DATA *		pet;
    CHAR_DATA *		tracking;
    CHAR_DATA *		familiar;
    CHAR_DATA *		hunting;
    CHAR_DATA *		guarding;
    CHAR_DATA *		mindlink;
    CHAR_DATA *		mercy_to;
    CHAR_DATA *		mercy_from;
    SPEC_FUN *		spec_fun;
    MOB_INDEX_DATA *	pIndexData;
    DESCRIPTOR_DATA *	desc;
    AFFECT_DATA *	affected;
    NOTE_DATA *		pnote;
    NOTE_DATA *		lastnote;
    OBJ_DATA *		carrying;
    OBJ_DATA *		on;
    ROOM_INDEX_DATA *	in_room;
    ROOM_INDEX_DATA *	was_in_room;
    ROOM_INDEX_DATA *   recall_to;
    ROOM_INDEX_DATA *   recall_old;	/* mainly for circle of stones */
    AREA_DATA *		zone;
    PC_DATA *		pcdata;
    GEN_DATA *		gen_data;
    RESET_DATA *	reset;
    AFFECT_DATA *	song;
    AFFECT_DATA *	harmony;
    RoomPathTracker * path;
    MOUNT_DATA *	mount_data;
    CHAR_DATA *		mount;
    CHAR_DATA *		rider;
    bool		valid;
    bool		skilled;
    char * 		name;
    char * 		fake_name;
    char * 		unique_name;
    long		id;
    long		lasthour;
    long		laston;
    int		version;
    char *		short_descr;
    char *		orig_short;
    char *		long_descr;
    char *		orig_long;
    char *		description;
    char *		orig_description;
    char *		prompt;
    char *		prefix;
    char *      pose;
    char *		battlecry; /* for fighters using rally */
    int		group;
    int		clan;
    int		sex;
    int		class_num;
    int		race;
    int		fake_race;
    int		level;
    int		trust;
    int			played;
    int			lines;  /* for the pager */
    time_t		logon;
    int		timer;
    int		wait;
    int		daze;
    int			hit;
    int			max_hit;
    int			mana;
    int			max_mana;
    int			move;
    int			max_move;
    int			fake_hit;
    long		coins[MAX_COIN];
//    long		gold;
//    long		silver;
    long		bank[MAX_COIN];
    int              arrows[4];
    int			exp;
    int			ep; /* exploration points */
    long		act;
    long		nact;
    long		comm;   /* RT added to pad the vector */
    long		wiznet; /* wiz stuff */
    long		extra_flags;  /* used by things in extras.c */
    int		resist[MAX_RESIST];
    long		imm_flags;
    long		res_flags;
    long		vuln_flags;
    int		invis_level;
    int		incog_level;
    long		affected_by;
    long		naffected_by;
    long		oaffected_by;
    long		paffected_by;
    int		position;
    int		practice;
    int		train;
    int		carry_weight;
    int		carry_number;
    int		saving_throw;
    int		alignment;
    int		hitroll;
    int		damroll;
    int		armor[4];
    int		wimpy;
    /* for feint, zephyr, etc */
    int		lost_att;
    /* stats */
    int		perm_stat[MAX_STATS];
    int		mod_stat[MAX_STATS];
    int		max_stat[MAX_STATS];
    /* parts stuff */
    long		form;
    long		parts;
    int		size;
    int         luck;
    int			material;
    /* mobile stuff */
    long		off_flags;
    int		damage[3];
    int		dam_type;
    char *		dam_verb;
    int		start_pos;
    int		default_pos;
    MPROG_ACT_LIST *    mpact;
    int                 mpactnum;
    /* For ranger creature lore */
    int                 lcreature[MAX_CLORE];
    int                 lpercent[MAX_CLORE];
    /* Ye olde familiar */
    int			familiar_type; /*this is the familiars vnum*/
    /* Shit for demons */
    int 		demonstate;
    CHAR_DATA	*	demontrack;
    CHAR_DATA   *	commander;
    /* Mobile Memory. Let the slaughter begin. */
    int			mobvalue[MAX_MOBVALUE];
    char *      stringvalue[MAX_MOBVALUE];
    CHAR_DATA	*	memfocus[MAX_MOBFOCUS];
    MEMORY_DATA *	memgroup;
    PROG_RUNDATA *	prog_data;
    /* Variables for bows'n'arrows */
    CHAR_DATA *         targetting;
    OBJ_DATA *          nocked;
    bool 		conloss; /* Has the person lost con? */
    int			lastsn; /* last sn of spell cast. For foci spam-protection */
    long		cdata; /* character data flags for setting with progs. */
    int		scoutdir; /* direction the person they scouted was seen */
    int		scoutrooms; /* how many rooms away they were */
    long		objlevels;
    CHAR_DATA *		shunned_demon;
    int		speaking;
    long		lang_flags;
    int			plength;
    int		religion;
    /* for data_progs */
    ProgState  prog_state;
    void *		point;
    int			symbols_known;
    int              attr_prime[MAX_STATS]; /* prime attributes */
    CHAR_DATA *		lastword;
    int         ticks_slept;            // For tracking how long the person has been asleep
    ExperienceList * experience_list;   // For tracking experience dole-outs
};

/*
 * Data for the rangermenu interface
 * animal = animal name, ability = option/command, trigger = trigger,
 * state_bit = ON|OFF for the toggle style items
 * hasanimal_bit = same as the has_bit for the animal each item is assoc'd with
 * data = instantiated as data_type, this is an easy way to tell the difference
 * between the animal lines, the commands, and the options.  This matters for output.
 * */
struct	callanimal_data
{
   char *		animal;
   char *		ability;
   char * 		trigger;
   int			state_bit;
   int 			hasanimal_bit;
   int			has_bit;
   enum data 		{Animal = 1, Command, Option} data_type;
};

// Data which only PC's have.
struct	pc_data
{
    PC_DATA *		next;
    BUFFER * 		buffer;
    EXTRA_DESCR_DATA *	extra_descr;
    HEADER_DATA *	header;
    bool		valid;
    char *		pwd;
    char *		bamfin;
    char *		bamfout;
    char *		pretitle;
    char *		surname;
    char *		title;
    char *              extitle;
    char *		retainername;
    int			retainergender;
    char *      familiarname;
    int         familiargender;
    char *		background;
    char *		recycle_log;
    char *		last_death_location;
    time_t              last_note;
    time_t              last_idea;
    time_t              last_penalty;
    time_t              last_news;
    time_t              last_changes;
    time_t		last_roomnote;
    time_t		delete_requested;
    int		perm_hit;
    int		perm_mana;
    int		perm_move;
    int		true_sex;
    int			last_level;
    int                 adrenaline;
    int			max_age;
    int		age_mod;
    int		age_mod_perm;
    int		age_group;
    int		last_age;
    int		max_deaths;
    int		condition	[4];
    int		learned		[MAX_SKILL];
    bool		group_known	[MAX_GROUP];
    int			favored_vnum	[MAX_SWORDS];
    int			favored_bonus	[MAX_SWORDS];
    SomaticArtsInfo * somatic_arts_info;
    PhantasmInfo * phantasmInfo;
    int		points;
    bool              	confirm_delete;
    char *		alias[MAX_ALIAS];
    char * 		alias_sub[MAX_ALIAS];
    unsigned char *	bitptr; /* 8192 bytes is mallocd - keep at 0 of that */
    unsigned char *	travelptr; /* 8192 bytes is mallocd - keep at 0 of that */
    int 		security;	/* OLC */ /* Builder security */
    int			ethos;		/* guess */
    int			major_sphere;	/* and again.. */
    int			minor_sphere;	/* and yet again... :) */
    int         chosen_path;
    int			death_count;
    int			times_wanted;
    int			focus_sn[MAX_FOCUS];
    CHAR_DATA *		focus_ch[MAX_FOCUS];
    bool		focus_on[MAX_FOCUS];
    char *		record;
    char *		immrecord;
    int			pkills;
    int			align_kills[3];
    int			pcdata_flags;
    unsigned int	lang_checksum; /* used to prevent learning from spam */
    char *		songbook[MAX_SONGBOOK];
    char *		performing;
    long		cards_id;
    char *		socket_info;
    int			award_points;
    char *		redit_name;
    char *		redit_desc;
    int			food_quality;
    unsigned char 	traits[5];
    unsigned short	hp_trains;
    unsigned short	mana_trains;
    short		karma;
    short       request_points;
    FactionStanding * faction_standing;

    unsigned int	gamesock;
    char *		cmd_buf;    // size: CBUF_LEN
    unsigned short	cbuf_size;  // size: pmsg_len
    long		prdef;	    // Puerto Rico, default game
    long		coldef;	    // Citadels, default game
    OBJ_DATA *   	purchased_item;
    int			purchased_item_cost;
    time_t		purchased_time;
    int			dailyxp;
    unsigned long	bounty;
    SAC_TYPE		sacrifices[MAX_GODS];
};	

/* pcdata_flags */

#define	PCD_APP_DESC		(A)	/* approved description */
#define PCD_APP_BG		(B)	/* approved background  */
#define PCD_CALL_RETAINER	(C)	/* dead retainer, for TRAIT_RETAINER */

#define	TRAIT_ARISTOCRAT    1
#define TRAIT_STREETWISE    2
#define TRAIT_GIFTED	    3
#define TRAIT_BLACKSMITH    4
#define TRAIT_MAGAPT	    5
#define TRAIT_AMBIDEXTROUS  6
#define TRAIT_AQUATIC	    7
#define TRAIT_ARCANE_TOUCH  8
#define TRAIT_BRAVE	    9
#define TRAIT_CHARMING	    10
#define TRAIT_COLDNATURE    11
#define TRAIT_POISONRES     12 //Added to show up in score
#define TRAIT_COWARD	    13
#define TRAIT_CRITICAL	    14
#define TRAIT_CYNIC	    15
#define TRAIT_EAGLE_EYED    16
#define TRAIT_ENDURANCE	    17
#define TRAIT_FLEET	    18
#define TRAIT_FRUGAL	    19
#define TRAIT_MARKED	    20
#define TRAIT_HOLLOWLEG	    21
#define TRAIT_IRONSTOMACH   22
#define TRAIT_LINGUIST	    23
#define TRAIT_LONGLIFE	    24
#define TRAIT_RETAINER	    25
#define TRAIT_RESISTANT	    26
#define TRAIT_HEIRLOOM	    27
#define TRAIT_NIMBLE	    28
#define TRAIT_OBSCURE	    29
#define TRAIT_PACK_HORSE    30
#define TRAIT_PIOUS	    31
#define TRAIT_TIMELESS	    32
//
#define TRAIT_SWORDMASTERY  33 //Added to show up in score
#define TRAIT_SURVIVOR      34
#define TRAIT_EXOTICMASTERY 35
#define TRAIT_LIGHTSLEEPER  36
#define TRAIT_THIEVESCANT   37
#define TRAIT_MORTICIAN     38

// REMEMBER: Increase the size of pc_data.traits if need be.
// Currently allocated: unsigned char[5] (40 bits)


/* Data for generating characters -- only used during generation */
struct gen_data
{
    GEN_DATA	*next;
    bool	valid;
    bool	skill_chosen[MAX_SKILL];
    bool	group_chosen[MAX_GROUP];
    int		points_chosen;
};

struct age_type
{
    char *	name;
    int      stat_mod[MAX_STATS];
    int      skill_mod;
};

/*
 * Liquids.
 */
#define LIQ_WATER        0

struct	liq_type
{
    char *	liq_name;
    char *	liq_color;
    int	liq_affect[5];
};



/*
 * Extra description data for a room or object.
 */
struct	extra_descr_data
{
    EXTRA_DESCR_DATA *next;	/* Next in list                     */
    bool valid;
    bool can_edit;		/* Added for PC exdescs 	    */
    char *keyword;              /* Keyword in look/examine          */
    char *description;          /* What to see                      */
};



/*
 * Prototype for an object.
 */
struct	obj_index_data
{
    OBJ_INDEX_DATA *	next;
    EXTRA_DESCR_DATA *	extra_descr;
    AFFECT_DATA *	affected;
    AREA_DATA *		area;		/* OLC */
    bool		new_format;
    bool		sFlag;
    char *		name;
    char *		short_descr;
    char *		description;
    char * 		lore;
    char *		obj_str;
    int		vnum;
    int		reset_num;
    int			material;
    int		item_type;
    unsigned long	extra_flags[3];
    int			wear_flags;
    int		level;
    int 		condition;
    int		count;
    int		weight;
    int		size;
    int			cost;
    int			value[5];
    MPROG_DATA *        objprogs;
    int			progtypes;
    int		limit_factor;
    int		limit;
    int			current;
    int			shown;
};

/*
 * One object.
 */
struct	obj_data
{
    OBJ_DATA *		next;
    OBJ_DATA *		next_content;
    OBJ_DATA *		contains;
    OBJ_DATA *		in_obj;
    OBJ_DATA *		on;
    OBJ_DATA *		next_rand;
    CHAR_DATA *		carried_by;
    EXTRA_DESCR_DATA *	extra_descr;
    AFFECT_DATA *	affected;
    OBJ_INDEX_DATA *	pIndexData;
    ROOM_INDEX_DATA *	in_room;
    bool		valid;
    bool		enchanted;
    void *		vo;
    char *	        owner;
    char * 		name;
    char *		short_descr;
    char *		description;
    char *		lore;
    char *		obj_str;
    int		item_type;
    int			wear_flags;
    unsigned long	extra_flags[3];
    int		wear_loc;
    int			worn_on;
    int		weight;
    int		size;
    int			cost;
    int		level;
    int 		condition;
    int			material;
    int		timer;
    int			value	[5];
    OPROG_ACT_LIST *    opact;
    int                 opactnum;
    char *		killed_by;
    int			objvalue[MAX_MOBVALUE];
    char *      stringvalue[MAX_MOBVALUE];
    CHAR_DATA *		objfocus[MAX_OBJFOCUS];
    char *		lastowner[MAX_LASTOWNER];
    int			timestamp;
    ProgState		prog_state;
    PROG_RUNDATA *	prog_data;
};


/*
 * Exit data.
 */
struct	exit_data
{
    union
    {
	ROOM_INDEX_DATA *	to_room;
	int			vnum;
    } u1;
    unsigned long	exit_info;
    int		key;
    char *		keyword;
    char *		description;
    EXIT_DATA *		next;		/* OLC */
    unsigned long	rs_flags;	/* OLC */
    int			orig_door;	/* OLC */
};

/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile 
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
struct	reset_data
{
    RESET_DATA *	next;
    char		command;
    int		arg1;
    int		arg2;
    int		arg3;
    int		arg4;
};

struct vnum_range
{
    VNUM_RANGE *	next;
    int		min_vnum;
    int		max_vnum;
};

/* definitions for ainfo_flags */
#define AINFO_NEWRES		(A)
#define AINFO_AVNUMS		(B)
#define AINFO_INVIS		(C)
#define AINFO_MOBCLASS		(D)
#define AINFO_NOWORDLIST	(E)
#define AINFO_HASOWNER		(F)

/*
 * Area definition.
 */
struct	area_data
{
    AREA_DATA *		next;
    RESET_DATA *	reset_first;
    RESET_DATA *	reset_last;
    AFFECT_DATA *	affected;
    ROOM_INDEX_DATA *	temp_room;
    char *		file_name;
    char *		name;
    char *		credits;
    int		age;
    int		nplayer;
//    int		low_range;
//    int		high_range;
    VNUM_RANGE *	vnums;
//    int 		min_vnum;
//    int		max_vnum;
    bool		empty;
    int			danger;
    int                 ainfo_flags;
    int			herbs;
    /* stuff for weather */
    int		base_precip;
    int		base_temp;
    int		base_wind_mag;
    int		base_wind_dir;
    int			geography;
    struct 
    {
        int		temperature;
        int		cloud_cover;
        int		wind_dir;
        int		wind_mag;
        int		storm_str;
        int		precip_index;
    	int		lightning_str;
    	int		precip_type;
	bool		tornado;
    } w_cur, w_new;
    int		storm_enter[6];

    /* stuff for OLC */
    char *		builders;	/* OLC */ /* Listing of */
    int			vnum;		/* OLC */ /* Area vnum  */
    int			area_flags;	/* OLC */
    int			security;	/* OLC */ /* Value 1-9  */

    // Weave-related
    Fount::Frequency fount_frequency;
    int fount_order_bias;
    int fount_positive_bias;

    // Shade-related
    Shades::Density shade_density;
    Shades::Power shade_power;

    // Earth-related
    int stone_type;     // Should correspond to a stone-flagged material, or be -1
};

struct weather_data
{
    int	temperature;
    int	cloud_cover;
    int	wind_dir;
    int	wind_mag;
    int	storm_str;
    int	precip_index;
    int	storm_dur;
    int	lightning_str;
    int	precip_type;
};

struct area_link_data
{
    ALINK_DATA *	next;
    AREA_DATA *		a1;
    AREA_DATA * 	a2;
    int		dir1;
    int		dir2;
};

#define PRECIP_BARREN    	0
#define PRECIP_ARID      	1
#define PRECIP_AVERAGE   	2
#define PRECIP_WET       	3
#define PRECIP_MONSOON   	4

#define TEMP_BLISTERING		0
#define TEMP_HOT		1
#define TEMP_TEMPERATE		2
#define TEMP_COOL		3
#define TEMP_FRIGID		4

#define WMAG_GALE		0
#define WMAG_WINDY		1
#define WMAG_NORMAL		2
#define WMAG_STILL		3
#define WMAG_DOLDRUMS		4

#define WCOM_DRIER		0
#define WCOM_WETTER		1
#define WCOM_WARMER		2
#define WCOM_COOLER		3
#define WCOM_WINDIER		4
#define WCOM_CALMER		5

#define GEO_OCEAN		(A)
#define GEO_COASTAL		(B)
#define GEO_MOUNTAINOUS		(C)
#define GEO_PLAINS		(D)
#define GEO_LOWLANDS		(E)
#define GEO_DESERT		(F)
#define GEO_RIVER		(G)

/*
 * Room type.
 */
struct	room_index_data
{
    ROOM_INDEX_DATA *	next;
    ROOM_INDEX_DATA *   next_temp;	/* for temporary rooms */
    CHAR_DATA *		people;
    OBJ_DATA *		contents;
    EXTRA_DESCR_DATA *	extra_descr;
    ROOM_INDEX_DATA *	next_rand;
    AREA_DATA *		area;
    EXIT_DATA *		exit	[MAX_DOOR];
    EXIT_DATA * 	old_exit[MAX_DOOR];
    RESET_DATA *	reset_first;	/* OLC */
    RESET_DATA *	reset_last;	/* OLC */
    TRACK_DATA *	tracks;
    AFFECT_DATA *	affected;
    AFFECT_DATA *	songs;
    char *		name;
    char *		description;
    char *		owner;
    int		vnum;
    int			room_flags;
    int			gods_altar;
    int		light;
    int		sector_type;
    int		heal_rate;
    int 		mana_rate;
    int 		move_rate;
    int		clan;
    int		herb_type;
    int		danger;
    int			roomvalue[MAX_MOBVALUE];
    char *      stringvalue[MAX_MOBVALUE];
    CHAR_DATA *		roomfocus[MAX_ROOMFOCUS];
    MPROG_DATA *	mobprogs;
    int			progtypes;
    ProgState      prog_state;
    PROG_RUNDATA *	prog_data;

    // Weave-related variables
    LeyGroup * ley_group;
    Fount::Frequency fount_frequency;
    int fount_order_power;
    int fount_positive_power;

    // Shade-related variables
    Shades::Density shade_density;
    Shades::Power shade_power;

    // Earth-related variables
    int stone_type; // Should correspond to a stone-flagged material, or be -1 to use area's value

    // Invasive vars used in RoomPath for performance reasons
    mutable const ROOM_INDEX_DATA * arrived_from; // The room from which the path was generated
    mutable unsigned int arrived_from_door; // The door from which the path was generated
    mutable unsigned int path_generation_id; // The id of the last time this room was used in a generated path
};

struct track_data {
TRACK_DATA	* next;
CHAR_DATA	* ch;
int		direction;
int		time;
bool		valid;
};

/* This memory data is for mobile memory. It's always NULL and should not
be touched on a PC. For a mobile, it is manipulated with mp* commands, and
expands and contracts at needed. On a fresh mob, it is NULL. */

struct memory_data {
MEMORY_DATA 	* next;
CHAR_DATA	* ch;
int		value;
};




/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_UNDEFINED               -1
#define TYPE_FLOAT		     363    /* The sn for dancing sword */
#define TYPE_HIT                     2000


/* values for htype in one_hit() */

#define HIT_PRIMARY			0
#define HIT_DUAL			1
#define HIT_FLOAT			2
#define	HIT_OPPORTUNITY			3

/*
 *  Target types.
 */
#define TAR_IGNORE		        0
#define TAR_CHAR_OFFENSIVE	    1
#define TAR_CHAR_DEFENSIVE	    2
#define TAR_CHAR_SELF		    3
#define TAR_OBJ_INV		        4
#define TAR_OBJ_CHAR_DEF	    5
#define TAR_OBJ_CHAR_OFF	    6
#define TAR_CHAR_DEF_GLOBAL	    7
#define TAR_OBJ_ROOM            8

#define TARGET_CHAR		    0
#define TARGET_OBJ		    1
#define TARGET_ROOM		    2
#define TARGET_NONE		    3


/*
 * Return code for prog functions.
 * Not in use at the moment, 09/06/00
 */
#define PROG_OK			0
#define PROG_INVARG		1
#define PROG_INVTYPE		2	

#define MAX_SKILL_SPHERE 2

/*
 * Skills include spells as a particular case.
 */
struct	skill_type
{
    char *	name;			/* Name of skill		*/
    int	skill_level[MAX_CLASS];	/* Level needed by class	*/
    int	rating[MAX_CLASS];	/* How hard it is to learn	*/	
    SPELL_FUN *	spell_fun;		/* Spell pointer (for spells)	*/
    int	target;			/* Legal targets		*/
    int	minimum_position;	/* Position for caster / user	*/
    int *	pgsn;			/* Pointer to associated gsn	*/
    int	slot;			/* Slot for #OBJECT loading	*/
    int	min_mana;		/* Minimum mana used		*/
    int	beats;			/* Waiting time after use	*/
    char *	noun_damage;		/* Damage message		*/
    char *	msg_off;		/* Wear off message		*/
    char *	msg_obj;		/* Wear off message for obects	*/
    char *	house;			/* The house that has this -- 0 *
					 *  for normal spells/skill     */
    int	attr;
    int	spheres[MAX_SKILL_SPHERE];
    unsigned int flags; 
};

struct  group_type
{
    char *	name;
    char *	spells[MAX_IN_GROUP];
};

struct  focus_type
{
    FOCUS_FUN * focus_fun;
    int *    sn;
    int      slots;
    int	afslot;
};

struct song_type
{
    SONG_FUN *  song_fun;
    int *	sn;
    int	inst_type;
};

#define INST_TYPE_VOICE		0

#define INST_TYPE_PERC		(A)
#define INST_TYPE_STRING	(B)
#define INST_TYPE_WOODWIND	(C)
#define INST_TYPE_BRASS		(D)
#define MAX_RANGER_ABILITIES 39

/*
 * These are skill_lookup return values for common skills and spells.
 */
extern struct callanimal_data rangermenu_array[MAX_RANGER_ABILITIES];
extern struct callanimal_data rangermenu_array[MAX_RANGER_ABILITIES];
extern void rangermenu_init();

extern	int	gsn_shamanicward;
extern	int	gsn_seedofgamaloth;
extern	int	gsn_naturalarmor;
extern	int	gsn_beastform;
extern	int	gsn_drawblood;
extern	int	gsn_snakebite;
extern	int	gsn_wither;
extern	int	gsn_eyesoftheforest;
extern	int	gsn_moonsight;
extern	int	gsn_shuran_gaze;
extern	int	gsn_arrowcraft;
extern	int	gsn_maul;
extern	int	gsn_aggression;
extern	int	gsn_sidestep;
extern	int	gsn_rend;
extern	int	gsn_fortitude;
extern	int	gsn_ferocity;
extern	int	gsn_lastword;
extern	int	gsn_cure_all_serious;
extern	int	gsn_awaken;
extern	int	gsn_cure_blindness;
extern	int	gsn_purify;
extern	int	gsn_refresh;
extern	int	gsn_remove_curse;
extern	int	gsn_sacrifice;
extern	int	gsn_armor;
extern	int	gsn_immolation;
extern	int	gsn_calm;
extern	int	gsn_detect_invis;
extern	int	gsn_infravision;
extern	int	gsn_shield;
extern	int	gsn_change_sex;
extern	int	gsn_regeneration;
extern	int	gsn_flamestrike;
extern	int	gsn_weaken;
extern	int	gsn_earthquake;
extern	int	gsn_demoncall;
extern	int	gsn_normal_demon_summon;
extern	int	gsn_unseen_servant;
extern	int	gsn_rite_of_nathli;
extern	int	gsn_exorcism;
extern	int	gsn_ray_of_light;
extern	int	gsn_siphon_mana;
extern	int	gsn_phase_door;
extern	int	gsn_blast;
extern	int	gsn_firebolt;
extern	int	gsn_mirror_image;
extern	int	gsn_wind_blast;
extern	int	gsn_magnetic_grasp;
extern	int	gsn_ranged_healing;
extern	int	gsn_oaklance;
extern	int	gsn_icebolt;
extern	int	gsn_life_shield;
extern	int	gsn_cone_of_cold;
extern	int	gsn_icy_shield;
extern	int	gsn_warp_wood;
extern	int	gsn_thornspray;
extern	int	gsn_lightning_breath;
extern	int	gsn_gas_breath;
extern	int	gsn_frost_breath;
extern	int	gsn_fire_breath;
extern	int	gsn_acid_breath;
extern	int	gsn_word_of_recall;
extern	int	gsn_stonefist;
extern	int	gsn_spiritwrack;
extern	int	gsn_soulburn;
extern	int	gsn_scorch;
extern	int	gsn_scare;
extern	int	gsn_sandstorm_old;
extern	int	gsn_sandspray;
extern	int	gsn_pyrotechnics;
extern	int	gsn_firestorm;
extern	int	gsn_harm;
extern	int	gsn_drain;
extern	int	gsn_colour_spray;
extern	int	gsn_coughing_dust;
extern	int	gsn_chain_lightning;
extern	int	gsn_cause_critical;
extern	int	gsn_cause_light;
extern 	int	gsn_cause_serious;
extern	int	gsn_call_lightning;
extern	int	gsn_retainercall;
extern	int	gsn_rush;
extern	int	gsn_opportunism;
extern	int	gsn_defensiveroll;
extern	int	gsn_pilfer;
extern	int	gsn_bargain;
extern	int	gsn_dash;
extern	int	gsn_dissection;
extern	int	gsn_showmanship;
extern	int	gsn_daggertrap;
extern	int	gsn_detectstealth;
extern	int	gsn_pursuit;
extern	int	gsn_forgery;
extern	int	gsn_distract;
extern	int	gsn_swift_staff;
extern	int	gsn_bounty;
extern	int	gsn_exhaustionpoison;
extern	int	gsn_erosivepoison;
extern 	int	gsn_concealremains;
extern	int	gsn_snapkick;
extern	int	gsn_acuity;
extern	int	gsn_celerity;
extern	int	gsn_shadowfist;
extern	int	gsn_invocationofconveru;
extern	int	gsn_removehex;
extern	int	gsn_continual_light;
extern	int	gsn_lesserspiritshield;
extern	int	gsn_seraphicwings;
extern	int	gsn_glowinggaze;
extern	int	gsn_froststrike;
extern	int	gsn_healingward;
extern	int	gsn_douse;
extern	int	gsn_coronalglow;
extern	int	gsn_fortify;
extern	int	gsn_giantstrength;
extern	int	gsn_disruption;
extern	int	gsn_petrify;
extern	int	gsn_runeofembers;
extern	int	gsn_blazingspear;
extern	int	gsn_passdoor;
extern	int	gsn_rob;
extern	int	gsn_headbutt;
extern	int	gsn_shieldslam;
extern	int	gsn_jab;
extern	int	gsn_brawlingblock;
extern	int	gsn_trueshot;
extern	int	gsn_ready;
extern	int	gsn_cook;
extern	int	gsn_predatoryattack;
extern	int	gsn_pounce;
extern	int	gsn_soulflare;
extern	int	gsn_huntersense;
extern	int	gsn_fletch;
extern	int	gsn_applypoison;
extern	int	gsn_applybarbs;
extern	int	gsn_forage;
extern	int	gsn_farsight;
extern	int	gsn_iceshard;
extern	int	gsn_flood;
extern	int	gsn_iceblast;
extern	int	gsn_enervatingray;
extern	int	gsn_mireofoame;
extern	int	gsn_dim;
extern	int	gsn_shadowstrike;
extern	int	gsn_desecrateweapon;
extern	int	gsn_pestilence;
extern	int	gsn_shielddisarm;
extern	int	gsn_retaliation;
extern	int	gsn_whipmastery;
extern	int	gsn_taunt;
extern	int	gsn_endure;
extern	int	gsn_bravado;
extern	int	gsn_enthusiasm;
extern	int	gsn_mantle_of_earth;
extern	int	gsn_durability;
extern	int	gsn_augmented_strength;
extern	int	gsn_momentum;
extern	int	gsn_charge;
extern	int	gsn_whirl;
extern  int	gsn_reversal;
extern	int	gsn_grace;
extern  int  gsn_medstrike;
extern  int  gsn_vengeance;
extern	int	gsn_riteofkaagn;
extern	int	gsn_teleport;
extern	int	gsn_thunderclap;
extern	int	gsn_fireball;
extern	int	gsn_jawsofthemountain;
extern	int	gsn_sweep;
extern  int	gsn_reach_mastery;
extern  int  gsn_gash;
extern  int	gsn_reaver_bind;
extern	int	gsn_aura_of_corruption;
extern  int  gsn_demonic_focus;
extern  int  gsn_compact_of_Logor;
extern  int  gsn_demonic_might;
extern  int  gsn_dark_insight;
extern  int  gsn_shadow_ward;
extern  int  gsn_windsurge;
extern	int	gsn_backstab;
extern	int	gsn_dodge;
extern  int  gsn_pugilism;
extern	int	gsn_evade;
extern  int  gsn_envenom;
extern	int	gsn_hide;
extern  int  gsn_morph;
extern	int	gsn_peek;
extern	int	gsn_pick_lock;
extern	int	gsn_sneak;
extern	int	gsn_steal;

extern	int	gsn_disarm;
extern	int	gsn_enhanced_damage;
extern	int	gsn_kick;
extern	int	gsn_parry;
extern	int	gsn_rescue;
extern	int	gsn_second_attack;
extern	int	gsn_third_attack;
extern  int  gsn_fourth_attack;

extern	int	gsn_cursebarkja;
extern	int	gsn_qwablith;
extern	int	gsn_doppelganger;
extern	int	gsn_bindingtentacle;
extern	int	gsn_desecration;
extern	int	gsn_succubuskiss;
extern	int	gsn_veilsight;
extern	int	gsn_darkfuture;
extern	int	gsn_uncleanspirit;
extern	int	gsn_devouringspirit;
extern  int	gsn_cursekhamurn;
extern	int	gsn_demoniccontrol;
extern	int	gsn_blindness;
extern  int  gsn_blight;
extern	int	gsn_charm_person;
extern	int	gsn_curse;
extern  int  gsn_cursekijjasku;
extern	int	gsn_invis;
extern	int	gsn_mass_invis;
extern	int	gsn_icyprison;
extern	int	gsn_fieldmedicine;
extern  int  gsn_plague;
extern  int  gsn_plague_madness;
extern	int	gsn_poison;
extern	int	gsn_poisonspirit;
extern	int  gsn_cower;
extern  int  gsn_taint;
extern	int	gsn_painpoison;
extern	int	gsn_sleeppoison;
extern	int	gsn_delusionpoison;
extern	int	gsn_tremblingpoison;
extern	int	gsn_deathpoison;
extern	int	gsn_sleep;
extern  int  gsn_demon_bind;
extern	int	gsn_caressofpricina;
extern  int  gsn_hungerofgrmlarloth;
extern  int  gsn_jawsofidcizon;
extern	int	gsn_defilement;
extern  int  gsn_blade_of_vershak;
extern  int  gsn_fly;
extern  int  gsn_massflying;
extern	int	gsn_skill;
extern  int  gsn_stringdata;
extern  int  gsn_sanctuary;
extern	int	gsn_cure_poison;
extern	int	gsn_cure_light;
extern	int	gsn_cure_serious;
extern	int	gsn_cure_critical;
extern	int	gsn_slow_cure;
extern	int	gsn_slow_cure_ser;
extern	int	gsn_slow_cure_crit;
extern	int	gsn_slow_heal;
extern	int	gsn_slow_cure_poison;
extern	int	gsn_slow_cure_disease;
extern	int	gsn_protectionfromcold;
extern	int	gsn_resist_poison;
extern	int	gsn_haste;
extern	int	gsn_heal;
extern	int	gsn_cure_disease;

/* language gsns */
extern	int	gsn_language_common;
extern	int	gsn_language_aelin;
extern	int	gsn_language_alatharya;
extern	int	gsn_language_nefortu;
extern	int	gsn_language_kankoran;
extern	int	gsn_language_caladaran;
extern	int	gsn_language_chtaren;
extern	int	gsn_language_ethron;
extern	int	gsn_language_srryn;
extern	int	gsn_language_shuddeni;
extern	int	gsn_language_arcane;

/* songs */
extern	int	gsn_noteofshattering;
extern	int	gsn_eleventhhour;
extern	int	gsn_marchofwar;
extern	int	gsn_sonicwave;
extern	int	gsn_soundbubble;
extern	int	gsn_wallsofjericho;
extern	int	gsn_cloakofshadows;
extern	int	gsn_auraoflight;
extern	int	gsn_psalmofhealing;
extern	int	gsn_serenadeoflife;
extern	int	gsn_baneofasjia;
extern	int	gsn_invokesympathy;
extern	int	gsn_marchingtune;
extern	int	gsn_aegisofmusic;
extern	int	gsn_noteofstriking;
extern	int	gsn_echoesoffear;
extern  int	gsn_discord;
extern	int	gsn_songofsoothing;
extern	int	gsn_manasong;
extern	int	gsn_stuttering;

extern	int	gsn_hymntotourach;
extern	int	gsn_generic;
extern	int	gsn_heirloom;
extern	int	gsn_eyes_hunter;
extern  int  gsn_strength_of_Aramril;

/* gsns for forms and their defines for use in switch */
/* This, along with my use of sn numbers in progs, means the skill table
 * had better stay damn constant, or a lot will break */

#define GSN_FORM_OF_THE_BULL	208
extern int  gsn_form_of_the_bull;
#define GSN_FORM_OF_THE_WHIRLWIND 209
extern int  gsn_form_of_the_whirlwind;
#define GSN_FORM_OF_THE_BEAR	210
extern int  gsn_form_of_the_bear;
#define GSN_FORM_OF_THE_DRAGON	211
extern int  gsn_form_of_the_dragon;
#define GSN_FORM_OF_THE_BOAR	212
extern int  gsn_form_of_the_boar;
#define GSN_FORM_OF_THE_MONGOOSE  213
extern int  gsn_form_of_the_mongoose;
#define GSN_FORM_OF_THE_EAGLE  214
extern int  gsn_form_of_the_eagle;
#define GSN_FORM_OF_THE_CRAB  215
extern int  gsn_form_of_the_crab;
#define GSN_FORM_OF_THE_CAT  216
extern int  gsn_form_of_the_cat;
#define GSN_FORM_OF_THE_SERPENT  217
extern int  gsn_form_of_the_serpent;
#define GSN_FORM_OF_THE_SPIDER  218
extern int  gsn_form_of_the_spider;
#define GSN_FORM_OF_THE_PANTHER  219
extern int  gsn_form_of_the_panther;
#define GSN_FORM_OF_THE_MOCKINGBIRD  220
extern int  gsn_form_of_the_mockingbird;
#define GSN_FORM_OF_THE_REED  221
extern int  gsn_form_of_the_reed;
#define GSN_FORM_OF_THE_ROSE  222
extern int  gsn_form_of_the_rose;
#define GSN_FORM_OF_THE_HAWK  223
extern int  gsn_form_of_the_hawk;
#define GSN_FORM_OF_THE_MONKEY  224
extern int  gsn_form_of_the_monkey;
#define GSN_FORM_OF_THE_VIPER   464
extern int  gsn_form_of_the_viper;
#define GSN_FORM_OF_THE_WASP   465
extern int  gsn_form_of_the_wasp;
#define GSN_FORM_OF_THE_LIVING_SEAS 573
extern int  gsn_form_of_the_living_seas;
#define GSN_FORM_OF_THE_WINTER_WIND 577
extern int  gsn_form_of_the_winter_wind;

#define GSN_FORM_OF_THE_CYCLONE 600
extern int  gsn_form_of_the_cyclone;
extern int  gsn_form_of_the_phantasm;
extern int  gsn_form_of_the_zephyr;
extern int  gsn_form_of_the_wraith;
extern int  gsn_form_of_the_asp;

/* new gsns */

extern int  gsn_mimic;
extern int  gsn_tumbling;
extern int  gsn_harmony;

extern int  gsn_block_vision;
extern int  gsn_mindshield;
extern int  gsn_enhance_reflexes;
extern int  gsn_paranoia;
extern int  gsn_forget;
extern int  gsn_enhance_pain;
extern int  gsn_reduce_pain;
extern int  gsn_esp;
extern int  gsn_slow_reflexes;
extern int  gsn_symbiont;     
extern int  gsn_leech;        
extern int  gsn_deflection;   
extern int  gsn_levitation;   
extern int  gsn_read_thoughts;
extern int  gsn_ignore_pain;  
extern int  gsn_detect_life;  
extern int  gsn_psychic_block;
extern int  gsn_mind_thrust;  
extern int  gsn_psionic_blast;
extern int  gsn_suggestion;   
extern int  gsn_sensory_vision;
extern int  gsn_vertigo;
extern int  gsn_shove;
extern int  gsn_foresight;
extern int  gsn_sense_danger;
extern int  gsn_dominance;
extern int  gsn_confusion;
extern int  gsn_mindlink;
extern int  gsn_backlash;
extern int  gsn_prowess;
extern int  gsn_adrenaline_rush;
extern int  gsn_overwhelm;
extern int  gsn_conceal_thoughts;
extern int  gsn_ordered_mind;
extern int  gsn_accelerated_healing;
extern int  gsn_borrow_knowledge;

extern int  gsn_ball_lightning;
extern int  gsn_lightning_bolt;
extern int  gsn_shocking_grasp;
extern int  gsn_blur;
extern int  gsn_bladebarrier;
extern int  gsn_blink;
extern int  gsn_missileattraction;
extern int  gsn_screamingarrow;
extern int  gsn_arrowgeas;
extern int  gsn_disjunction;
extern int  gsn_precision;
extern int  gsn_mirrorimage;
extern int  gsn_lightningbrand;
extern int  gsn_absorbelectricity;
extern int  gsn_wallofair;
extern int  gsn_dive;
extern int  gsn_rearrange;
extern int  gsn_thunderstrike;
extern int  gsn_reflectiveaura;

extern int  gsn_quicksand;
extern int  gsn_bindweapon;
extern int  gsn_shockwave;
extern int  gsn_earthmaw;
extern int  gsn_protnormalmissiles;
extern int  gsn_inertial_strike;
extern int  gsn_devotion;
extern int  gsn_anchor;
extern int  gsn_stonephalanx;

extern int  gsn_grease;
extern int  gsn_slow;
extern int  gsn_trackless_step;
extern int  gsn_camoublind;
extern int  gsn_hooded;
extern int  gsn_swordrepair;
extern int  gsn_pommel;
extern int  gsn_rout;
extern int  gsn_pummel;
extern int  gsn_encamp;
extern int  gsn_uppercut;
extern int  gsn_drag;
extern int  gsn_terrainlore;
extern int  gsn_aquamove;
extern int  gsn_riteofthesun;
extern int  gsn_contact_agents;
extern int  gsn_impale;
extern int  gsn_obscurealign;
extern int  gsn_pugil;
extern int  gsn_hunt;
extern int  gsn_callanimal;
extern int  gsn_holyavenger;
extern int  gsn_reveal;
extern int  gsn_brotherhood;
extern int  gsn_scourgeofdarkness;
extern int  gsn_soulblade;
extern int  gsn_mantleoffear;
extern int  gsn_enslave;
extern int  gsn_coverofdarkness;
extern int  gsn_runeofeyes;
extern int  gsn_soulreaver;
extern int  gsn_coven;
extern int  gsn_consumption;
extern int  gsn_circleofstones;
extern int  gsn_demonsummon;
extern int  gsn_resonance;
extern int  gsn_cutoff;
extern int  gsn_freeze;
extern int  gsn_frostbrand;
extern int  gsn_firebrand;
extern int  gsn_faerie_fog;
extern int  gsn_etherealblaze;
extern int  gsn_starglamour;
extern int  gsn_tangletrail;
extern int  gsn_tentacles;
extern int  gsn_agony;
extern int  gsn_wildcraft;
extern int  gsn_mendwounds;
extern int  gsn_waterelementalsummon;
extern int  gsn_protectionfromfire;
extern int  gsn_hew;
extern int  gsn_runicmessage;
extern int  gsn_submissionhold;
extern int  gsn_agility;
extern int  gsn_pursue;
extern int  gsn_bolo;
extern int  gsn_surpriseattack;
extern int  gsn_findcover;
extern int  gsn_flashpowder;
extern int  gsn_detecttheft;
extern int  gsn_tripwire;
extern int  gsn_barbs;
extern int  gsn_deflect;
extern int  gsn_twirl;
extern int  gsn_record;
extern int  gsn_inquire;
extern int  gsn_sentry;
extern int  gsn_wariness;
extern int  gsn_identifyowner;
extern int  gsn_covertracks;
extern int  gsn_lightsleep;
extern int  gsn_vitalstrike;
extern int  gsn_shadowmastery;
extern int  gsn_plantentangle;
extern int  gsn_forestwalk;
extern int  gsn_giantgrowth;
extern int  gsn_shrink;
extern int  gsn_wallofvines;
extern int  gsn_wallofwater;
extern int  gsn_plant;
extern int  gsn_blindfighting;
extern int  gsn_dart;
extern int  gsn_setsnare;
extern int  gsn_prepare;
extern int  gsn_craftdart;
extern int  gsn_nerve;
extern int  gsn_disguise;
extern int  gsn_muffle;
extern int  gsn_caltraps;
extern int  gsn_garrote;
extern int  gsn_barkskin;
extern int  gsn_elementalprotection;
extern int  gsn_toggleoffaff;
extern int  gsn_toggleonaff;
extern int  gsn_relock;
extern int  gsn_sleeve;
extern int  gsn_thievescant;
extern int  gsn_stab;
extern int  gsn_slice;
extern int  gsn_cutpurse;
extern int  gsn_curseofthevoid;
extern int  gsn_wrathofthevoid;
extern int  gsn_finalstrike;
extern int  gsn_circle;
extern int  gsn_favoredblade;
extern int  gsn_offhanddisarm;
extern int  gsn_offhandparry;
extern int  gsn_waspstrike;
extern int  gsn_viperstrike;
extern int  gsn_aspstrike;
extern int  gsn_versatility;
extern int  gsn_winterwind;
extern int  gsn_ram;
extern int  gsn_fury;
extern int  gsn_rage;
extern int  gsn_warcry;
extern int  gsn_bloodlust;
extern int  gsn_fetish;
extern int  gsn_boneshatter;
extern int  gsn_roomdeathwalk;
extern int  gsn_bloodsigil;
extern int  gsn_counterattack;
extern int  gsn_lunge;
extern int  gsn_forcedmarch;
extern int  gsn_scouting;
extern int  gsn_counter;
extern int  gsn_rally;
extern int  gsn_cover;
extern int  gsn_phalanx;
extern int  gsn_retreat;
extern int  gsn_nothing;
extern int  gsn_guard;
extern int  gsn_bandage;
extern int  gsn_readiness;
extern int  gsn_drive;
extern int  gsn_strip;
extern int  gsn_corner;
extern int  gsn_entangle;
extern int  gsn_grip;
extern int  gsn_cross;
extern int  gsn_feint;
extern int  gsn_lash;
extern int  gsn_sortie;
extern int  gsn_flay;
extern int  gsn_batter;
extern int  gsn_cleave;
extern int  gsn_choke;
extern int  gsn_divineshield;
extern int  gsn_spirit_of_freedom;
extern int  gsn_archery;
extern int  gsn_arcshot;
extern int  gsn_commandweather;
extern int  gsn_commune_nature;
extern int  gsn_createfoci;
extern int  gsn_astralform;
extern int  gsn_astralprojection;
extern int  gsn_affinity;
extern int  gsn_masspeace;
extern int  gsn_focusmind;
extern int  gsn_avatar;
extern int  gsn_bless;
extern int  gsn_spirit_sanctuary;
extern int  gsn_spiritbond;
extern int  gsn_spiritblock;
extern int  gsn_aura;
extern int  gsn_spiritstone;
extern int  gsn_truesight;
extern int  gsn_lightsword;
extern int  gsn_lightspear;
extern int  gsn_manaconduit;
extern int  gsn_consecrate;
extern int  gsn_clarity;
extern int  gsn_lucid;
extern int  gsn_lifebolt;
extern int  gsn_dreamspeak;
extern int  gsn_sanctify;
extern int  gsn_visions;
extern int  gsn_unshackle;
extern int  gsn_poschan;
extern int  gsn_radiance;
extern int  gsn_runeofspirit;
extern int  gsn_zeal;
extern int  gsn_spiritshield;
extern int  gsn_improveddetectevil;
extern int  gsn_thanatopsis;
extern int  gsn_wrathkyana;
extern int  gsn_avengingseraph;
extern int  gsn_speakwiththedead;
extern int  gsn_mindshell;
extern int  gsn_weariness;
extern int  gsn_implosion;
extern int  gsn_parch;
extern int  gsn_heat_metal;
extern int  gsn_wingsofflame;
extern int  gsn_delayedblastfireball;
extern int  gsn_beamoffire;
extern int  gsn_phoenixfire;
extern int  gsn_infernofury;
extern int  gsn_burnout;
extern int  gsn_consuming_rage;
extern int  gsn_frenzy;
extern int  gsn_smolder;
extern int  gsn_fireblast;
extern int  gsn_shiyulsfury;
extern int  gsn_blazinginferno;
extern int  gsn_blaze;
extern int  gsn_flametongue;
extern int  gsn_consume;
extern int  gsn_ignite;
extern int  gsn_enflamed;
extern int  gsn_rainoffire;
extern int  gsn_incineration;
extern int  gsn_hew;
extern int  gsn_flare;
extern int  gsn_dispersion;
extern int  gsn_livingflame;
extern int  gsn_flameshield;
extern int  gsn_lambentaura;
extern int  gsn_runeoffire;
extern int  gsn_wardoffire;
extern int  gsn_ringoffire;
extern int  gsn_aggravatewounds;
extern int  gsn_nova;
extern int  gsn_baptismoffire;
extern int  gsn_walloffire;
extern int  gsn_detect_magic;
extern int  gsn_crystalizemagic;
extern int  gsn_runeofearth;
extern int  gsn_slip;
extern int  gsn_meteorstrike;
extern int  gsn_brittleform;
extern int  gsn_strengthen;
extern int  gsn_metaltostone;
extern int  gsn_meldwithstone;
extern int  gsn_shapeweapon;
extern int  gsn_shapearmor;
extern int  gsn_gravitywell;
extern int  gsn_stickstosnakes;
extern int  gsn_density;
extern int  gsn_smoothterrain;
extern int  gsn_voiceoftheearth;
extern int  gsn_stonetomud;
extern int  gsn_calluponearth;
extern int  gsn_dispelillusions;
extern int  gsn_earthelementalsummon;
extern int  gsn_stabilize;
extern int  gsn_knock;
extern int  gsn_encase;
extern int  gsn_diamondskin;
extern int  gsn_fleshtostone;
extern int  gsn_stoneshell;
extern int  gsn_stoneskin;
extern int  gsn_wallofstone;
extern int  gsn_lightning_storm;
extern int  gsn_phantasmalcreation;
extern int  gsn_diversions;
extern int  gsn_illusionaryobject;
extern int  gsn_dancingsword;
extern int  gsn_protectionfromlightning;
extern int  gsn_airrune;
extern int  gsn_gaseousform;
extern int  gsn_conjureairefreet;
extern int  gsn_tempest;
extern int  gsn_windbind;
extern int  gsn_creaturelore;
extern int  gsn_groupteleport;
extern int  gsn_windwall;
extern int  gsn_airbubble;
extern int  gsn_obfuscation;
extern int  gsn_spectralfist;
extern int  gsn_delusions;
extern int  gsn_greaterventriloquate;
extern int  gsn_alterself;
extern int  gsn_scatter;
extern int  gsn_windwalk;
extern int  gsn_greaterinvis;
extern int  gsn_gust;
extern int  gsn_suction;
extern int  gsn_cloudkill;
extern int  gsn_meldwithwater;
extern int  gsn_scry;
extern int  gsn_heatsink;
extern int  gsn_frostbite;
extern int  gsn_icestorm;
extern int  gsn_waterwalk;
extern int  gsn_holywater;
extern int  gsn_scry;
extern int  gsn_resurrection;
extern int  gsn_revitalize;
extern int  gsn_massheal;
extern int  gsn_whirlpool;
extern int  gsn_protectionfrompoison;
extern int  gsn_cleansefood;
extern int  gsn_currents;
extern int  gsn_runeoflife;
extern int  gsn_beltoflooters;
extern int  gsn_bind;
extern int  gsn_escape;
extern int  gsn_birdofprey;
extern int  gsn_pillage;
extern int  gsn_dedication;
extern int  gsn_intimidate;
extern int  gsn_matrix;
extern int  gsn_warpath;
extern int  gsn_division;
extern int  gsn_banish;
extern int  gsn_inscribe;
extern int  gsn_greaterdemonsummon;
extern int  gsn_lesserdemonsummon;
extern int  gsn_findfamiliar;
extern int  gsn_suppress;
extern int  gsn_commandword;
extern int  gsn_cloakofthevoid;
extern int  gsn_deathwalk;
extern int  gsn_demonichowl;
extern int  gsn_demonpos;
extern int  gsn_globedarkness;
extern int  gsn_gate;
extern int  gsn_leechrune;
extern int  gsn_possession;
extern int  gsn_kneeshatter;
extern int  gsn_voidwalk;
extern int  gsn_request;
extern int  gsn_vaultaffect;
extern int  gsn_stoneofpower;
extern int  gsn_perception;
extern int  gsn_aegisoflaw;
extern int  gsn_callguards;
extern int  gsn_locatecriminal;
extern int  gsn_herbalmedicine;
extern int  gsn_hawkform;
extern int  gsn_bearform;
extern int  gsn_wolfform;
extern int  gsn_naturegate;
extern int  gsn_skycall;
extern int  gsn_creepingcurse;
extern int  gsn_animaleyes;
extern int  gsn_speakwithplants;
extern int  gsn_stampede;
extern int  gsn_calmanimals;
extern int  gsn_wildmove;
extern int  gsn_animalswarm;
extern int  gsn_moonray;
extern int  gsn_trance;
extern int  gsn_chameleon;
extern int  gsn_lunarinfluence;
extern int  gsn_befriend;
extern int  gsn_forestsense;
extern int  gsn_tame;
extern int  gsn_findwater;
extern int  gsn_endurance;
extern int  gsn_tan;
extern int  gsn_swim;
extern int  gsn_poultice;
extern int  gsn_moonbornspeed;
extern int  gsn_vanish;
extern int  gsn_sixthsense;
extern int  gsn_blindingcloud;
extern int  gsn_blindingdust;
extern int  gsn_cloak;
extern int  gsn_investigate;
extern int  gsn_obscure_evidence;
extern int  gsn_stash;
extern int  gsn_camouflage;
extern int  gsn_sharp_vision;
extern int  gsn_plunder;
extern int  gsn_setambush;
extern int  gsn_ambush;
extern int  gsn_loot;
extern int  gsn_kidneyshot;
extern int  gsn_dualbackstab;
extern int  gsn_conceal;
extern int  gsn_trap;
extern int  gsn_track;
extern int  gsn_criminal;
extern int  gsn_ghost;
extern int  gsn_pox;
extern int  gsn_powerwordfear;
extern int  gsn_nightfears;
extern int  gsn_nightterrors;
extern int  gsn_abominablerune;
extern int  gsn_smoke;
extern int  gsn_backfire;
extern int  gsn_fever;
extern int  gsn_earthbind;
extern int  gsn_findherbs;
extern int  gsn_stickstosnakes;
extern int  gsn_mushroomcircle;
extern int  gsn_plantgrowth;
extern int  gsn_insectswarm;
extern int  gsn_naturesbounty;
extern int  gsn_animaltongues;
extern int  gsn_waterbreathing;
extern int  gsn_shieldcover;
extern int  gsn_shatter;
extern int  gsn_sustenance;
extern int  gsn_manabarbs;
extern int  gsn_rebirth;
extern int  gsn_paradise;
extern int  gsn_subdue;
extern int  gsn_disintegration;
extern int  gsn_clumsiness;
extern int  gsn_brainwash;
extern int  gsn_embraceisetaton;
extern int  gsn_enfeeblement;
extern int  gsn_protective_shield;
extern int  gsn_tail;
extern int  gsn_bite;
extern int  gsn_bludgeon;
extern int  gsn_legsweep;
extern int  gsn_withdraw;
extern int  gsn_brutal_damage;
extern int  gsn_savagery;
extern int  gsn_hamstring;
extern int  gsn_grapple;
extern int  gsn_decapitate;
extern int  gsn_nightvision;
extern int  gsn_listen;
extern int  gsn_detecthidden;
extern int  gsn_ensnare;
extern int  gsn_gouge;
extern int  gsn_flank;
extern int  gsn_throw;
extern int  gsn_catch_throw;
extern int  gsn_dual_wield;
extern int  gsn_fend;
extern int  gsn_vortex;
extern int  gsn_waylay;
extern int  gsn_gag;
extern int  gsn_rout;

/* Alchemists */
extern int  gsn_mix;
extern int  gsn_brew;
extern int  gsn_make;
extern int  gsn_distill;
extern int  gsn_pulverize;
extern int  gsn_reforge;
extern int  gsn_boil;
extern int  gsn_dissolve;
extern int  gsn_dilute;
extern int  gsn_delayreaction;
extern int  gsn_caution;
extern int  gsn_animategolem;
extern int  gsn_dye;
extern int  gsn_discernmagic;
extern int  gsn_finditems;
extern int  gsn_harrudimtalc;
extern int  gsn_smokescreen;
extern int  gsn_rynathsbrew;
extern int  gsn_concentrate;
extern int  gsn_alchemicstone;
extern int  gsn_subtlepoison;
extern int  gsn_transmogrification;
extern int  gsn_lesserhandwarp;
extern int  gsn_handwarp;
extern int  gsn_greaterhandwarp;
extern int  gsn_lessertentaclegrowth;
extern int  gsn_greatertentaclegrowth;
extern int  gsn_lessercarapace;
extern int  gsn_greatercarapace;
extern int  gsn_lycanthropy;
extern int  gsn_thirdeye;
extern int  gsn_cateyes;
extern int  gsn_eagleeyes;
extern int  gsn_gills;
extern int  gsn_drunkenness;
extern int  gsn_lovepotion;
extern int  gsn_susceptibility;
extern int  gsn_age;
extern int  gsn_polyglot;
extern int  gsn_resistance;
extern int  gsn_heroism;
extern int  gsn_divinesight;
extern int  gsn_youth;
extern int  gsn_invulnerability;
extern int  gsn_perfection;
extern int  gsn_improvement;
extern int  gsn_applyoil;
extern int  gsn_createwand;
extern int  gsn_teleportcurse;
extern int  gsn_aviancurse;
extern int  gsn_prismaticray;
extern int  gsn_prismaticspray;
extern int  gsn_maze;
extern int  gsn_createhomonculus;
extern int  gsn_transmutation;
extern int  gsn_diagnose;

/* Chirurgeon */
extern int gsn_cauterize;
extern int gsn_tendrilgrowth;
extern int gsn_whipstitch;
extern int gsn_anesthetize;
extern int gsn_infectiousaura;
extern int gsn_animate_dead;
extern int gsn_nettles;
extern int gsn_deathempowerment;
extern int gsn_shurangaze;

/* Songs! */
extern int  gsn_painful_thoughts;
extern int  gsn_courage;
extern int  gsn_dispel_illusion;

extern int  gsn_drowning;

extern int  gsn_axe;
extern int  gsn_dagger;
extern int  gsn_flail;
extern int  gsn_mace;
extern int  gsn_polearm;
extern int  gsn_shield_block;
extern int  gsn_spear;
extern int  gsn_sword;
extern int  gsn_whip;
extern int  gsn_knife;
extern int  gsn_staff; 

extern int  gsn_shieldbash;
extern int  gsn_bash;
extern int  gsn_berserk;
extern int  gsn_dirt;
extern int  gsn_hand_to_hand;
extern int  gsn_trip;
 
extern int  gsn_fast_healing;
extern int  gsn_haggle;
extern int  gsn_lore;
extern int  gsn_meditation;
extern int  gsn_appraise;
 
extern int  gsn_scrolls;
extern int  gsn_staves;
extern int  gsn_wands;
extern int  gsn_recall;
extern int  gsn_communion;

extern int  gsn_warmth;
extern int  gsn_firekin;
extern int  gsn_flameheart;
extern int  gsn_firedancer;
extern int  gsn_burningmind;
extern int  gsn_flamesight;
extern int  gsn_temper;
extern int  gsn_sealofthegoldenflames;
extern int  gsn_bloodpyre;
extern int  gsn_heartoftheinferno;
extern int  gsn_heartfire;
extern int  gsn_heatmine;
extern int  gsn_wrathofanakarta;
extern int  gsn_flameunity;
extern int  gsn_conflagration;
extern int  gsn_aspectoftheinferno;
extern int  gsn_phoenixdirge;
extern int  gsn_pyrotechnicartistry;

extern int  gsn_geothermics;
extern int  gsn_moltenshield;
extern int  gsn_summonlavaelemental;
extern int  gsn_harrudimfire;

extern int  gsn_chillfireshield;
extern int  gsn_nightflamelash;
extern int  gsn_ashesoflogor;

extern int  gsn_burningwisp;
extern int  gsn_heatwave;
extern int  gsn_pyrokineticmirror;

extern int  gsn_thermalmastery;
extern int  gsn_sauna;
extern int  gsn_flamesofthemartyr;
extern int  gsn_martyrsfire;
extern int  gsn_boilblood;

extern int  gsn_soulfireshield;
extern int  gsn_oriflamme;
extern int  gsn_holyflame;
extern int  gsn_soulbrand;
extern int  gsn_manaburn;

extern int  gsn_weavesense;
extern int  gsn_shroudsight;
extern int  gsn_aetherealcommunion;
extern int  gsn_unfettermana;
extern int  gsn_bondofsouls;
extern int  gsn_aegisofgrace;
extern int  gsn_wardofgrace;
extern int  gsn_riteofablution;
extern int  gsn_undyingradiance;
extern int  gsn_discerniniquity;
extern int  gsn_searinglight;
extern int  gsn_phaseshift;
extern int  gsn_leapoffaith;
extern int  gsn_unweave;
extern int  gsn_crystallizeaether;
extern int  gsn_weavecraft;
extern int  gsn_pacify;
extern int  gsn_absolvespirit;
extern int  gsn_dreamshape;
extern int  gsn_weavetap;
extern int  gsn_decorporealize;
extern int  gsn_countermagic;
extern int  gsn_reclaimessence;
extern int  gsn_lethebane;
extern int  gsn_chantlitany;
extern int  gsn_triumphantshout;
extern int  gsn_celestialtactician;
extern int  gsn_cohortsvengeance;
extern int  gsn_radiateaura;
extern int  gsn_nourishspirit;
extern int  gsn_diakinesis;
extern int  gsn_spectrallantern;
extern int  gsn_shadeswarm;
extern int  gsn_fugue;
extern int  gsn_etherealbrethren;
extern int  gsn_attunefount;
extern int  gsn_singularity;
extern int  gsn_leypulse;
extern int  gsn_quintessencerush;
extern int  gsn_manifestweave;
extern int  gsn_sunderweave;
extern int  gsn_etherealsplendor;
extern int  gsn_rebukeofinvesi;
extern int  gsn_wardoftheshieldbearer;
extern int  gsn_callhost;
extern int  gsn_bindessence;
extern int  gsn_workessence;
extern int  gsn_forgepostern;
extern int  gsn_sealpostern;
extern int  gsn_roaroftheexalted;
extern int  gsn_canticleofthelightbringer;
extern int  gsn_requiemofthemartyr;
extern int  gsn_avatarofthelodestar;
extern int  gsn_avataroftheprotector;
extern int  gsn_avataroftheannointed;

extern int  gsn_balmofthespirit;
extern int  gsn_martyrsshield;
extern int  gsn_distillmagic;
extern int  gsn_annointedone;

extern int  gsn_holyground;
extern int  gsn_crystalsoul;
extern int  gsn_stoneloupe;
extern int  gsn_earthenvessel;

extern int  gsn_dreammastery;
extern int  gsn_drowse;
extern int  gsn_dreamstalk;
extern int  gsn_parasiticbond;

extern int  gsn_bilocation;
extern int  gsn_aurora;
extern int  gsn_diffraction;

extern int  gsn_detoxify;
extern int  gsn_restorevigor;
extern int  gsn_frostkin;
extern int  gsn_treatinfection;
extern int  gsn_glyphofulyon;
extern int  gsn_solaceoftheseas;
extern int  gsn_healerstouch;
extern int  gsn_somaticarts;
extern int  gsn_deluge;
extern int  gsn_waveborne;
extern int  gsn_rimeshard;
extern int  gsn_wintertide;
extern int  gsn_maleficinsight;
extern int  gsn_boonoftheleech;
extern int  gsn_clarifymind;
extern int  gsn_stormmastery;
extern int  gsn_refinepotion;
extern int  gsn_wellspring;
extern int  gsn_frostblast;
extern int  gsn_markofthekaceajka;
extern int  gsn_draughtoftheseas;
extern int  gsn_physikersinstinct;
extern int  gsn_oceanswell;
extern int  gsn_maelstrom;
extern int  gsn_drown;
extern int  gsn_arcticchill;
extern int  gsn_breathofelanthemir;
extern int  gsn_glaciersedge;
extern int  gsn_wintersstronghold;
extern int  gsn_wardoffrost;
extern int  gsn_hoarfrost;
extern int  gsn_ordainsanctum;
extern int  gsn_corrosion;
extern int  gsn_contaminate;
extern int  gsn_darkchillburst;
extern int  gsn_stoneofsalyra;
extern int  gsn_fogelementalsummon;
extern int  gsn_monsoon;
extern int  gsn_brume;
extern int  gsn_steam;
extern int  gsn_boilseas;

extern int  gsn_subvocalize;
extern int  gsn_controlledflight;
extern int  gsn_disillusionment;
extern int  gsn_overcharge;
extern int  gsn_clingingfog;
extern int  gsn_gralcianfunnel;
extern int  gsn_illusion;
extern int  gsn_borrowluck;
extern int  gsn_shockcraft;
extern int  gsn_conjureairefreeti;
extern int  gsn_mistralward;
extern int  gsn_fatesdoor;
extern int  gsn_mirage;
extern int  gsn_sparkingcloud;
extern int  gsn_endlessfacade;
extern int  gsn_runeofair;
extern int  gsn_unrealincursion;
extern int  gsn_figmentscage;
extern int  gsn_phantasmalmirror;
extern int  gsn_empowerphantasm;
extern int  gsn_arcshield;
extern int  gsn_conduitoftheskies;
extern int  gsn_sonicboom;
extern int  gsn_electricalstorm;
extern int  gsn_ionize;
extern int  gsn_skystrike;
extern int  gsn_joltward;
extern int  gsn_displacement;
extern int  gsn_windrider;
extern int  gsn_typhoon;
extern int  gsn_unleashtwisters;
extern int  gsn_breezestep;
extern int  gsn_curseofeverchange;
extern int  gsn_beckonwindfall;
extern int  gsn_calluponwind;
extern int  gsn_mistsofarcing;
extern int  gsn_breathoflife;
extern int  gsn_mantleofrain;
extern int  gsn_sandstorm;
extern int  gsn_channelwind;
extern int  gsn_chargestone;
extern int  gsn_hoveringshield;
extern int  gsn_miasmaofwaning;
extern int  gsn_drainbolt;
extern int  gsn_soulofthewind;
extern int  gsn_mirrorofsouls;
extern int  gsn_bewilderment;
extern int  gsn_chaoscast;
extern int  gsn_incendiaryspark;
extern int  gsn_feigndemise;
extern int  gsn_englamour;
extern int  gsn_floatingdisc;

extern int  gsn_adamantineinvocation;
extern int  gsn_clayshield;
extern int  gsn_mudfootcurse;
extern int  gsn_reforgemagic;
extern int  gsn_tuningstone;
extern int  gsn_reinforce;
extern int  gsn_honeweapon;
extern int  gsn_stonecraft;
extern int  gsn_shellofstone;
extern int  gsn_quake;
extern int  gsn_rocktomud;
extern int  gsn_shapematter;
extern int  gsn_crush;
extern int  gsn_stoneshape;
extern int  gsn_runecraft;
extern int  gsn_quarry;
extern int  gsn_gravenmind;
extern int  gsn_markofloam;
extern int  gsn_glyphofentombment;
extern int  gsn_geomancy;
extern int  gsn_latticeofstone;
extern int  gsn_saltoftheearth;
extern int  gsn_wakenedstone;
extern int  gsn_bedrockroots;
extern int  gsn_cryofthebrokenlands;
extern int  gsn_shakestride;
extern int  gsn_stonehaven;
extern int  gsn_conduitofstonesong;
extern int  gsn_clayshard;
extern int  gsn_constructwaterwheel;
extern int  gsn_causticblast;
extern int  gsn_clockworksoul;
extern int  gsn_clockworkgolem;
extern int  gsn_kaagnsplaguestone;
extern int  gsn_heartofstone;
extern int  gsn_durablemagics;
extern int  gsn_abodeofthespirit;
extern int  gsn_mindofsteel;
extern int  gsn_pillarofsparks;
extern int  gsn_dispatchlodestone;
extern int  gsn_scorchedearth;
extern int  gsn_forgemaster;
extern int  gsn_forgeweapon;
extern int  gsn_lavaforge;
extern int  gsn_tremor;
extern int  gsn_calcify;
extern int  gsn_erosion;
extern int  gsn_empowerment;

extern int gsn_gloomward;
extern int gsn_fellpurpose;
extern int gsn_corpsesense;
extern int gsn_revenant;
extern int gsn_fadeshroud;
extern int gsn_scionofnight;
extern int gsn_kissoftheblackenedtears;
extern int gsn_touchofthedesecrator;
extern int gsn_cowlofnight;
extern int gsn_hatefire;
extern int gsn_gravebeat;
extern int gsn_darktallow;
extern int gsn_wreathoffear;
extern int gsn_scritofkaagn;
extern int gsn_witherpox;
extern int gsn_callbat;
extern int gsn_callcat;
extern int gsn_callfox;
extern int gsn_callraven;
extern int gsn_calltoad;
extern int gsn_callserpent;
extern int gsn_harvestofsouls;
extern int gsn_unholymight;
extern int gsn_deathlyvisage;
extern int gsn_blackamulet;
extern int gsn_barrowmist;
extern int gsn_imbuephylactery;
extern int gsn_reaping;
extern int gsn_deathswarm;
extern int gsn_direfeast;
extern int gsn_duskfall;
extern int gsn_eyeblighttouch;
extern int gsn_dreadwave;
extern int gsn_shadowfiend;
extern int gsn_nightstalk;
extern int gsn_webofoame;
extern int gsn_gaveloflogor;
extern int gsn_fetiddivination;
extern int gsn_grimseep;
extern int gsn_bloodofthevizier;
extern int gsn_seedofmadness;
extern int gsn_bierofunmaking;
extern int gsn_stasisrift;
extern int gsn_scriptmastery;
extern int gsn_theembraceofthedeeps;
extern int gsn_theilalslastsailing;
extern int gsn_hemoplague;
extern int gsn_cryptkin;
extern int gsn_baneblade;
extern int gsn_gravemaw;
extern int gsn_reshackle;
extern int gsn_devouressence;
extern int gsn_feverwinds;
extern int gsn_masquerade;
extern int gsn_fatebones;
extern int gsn_painchannel;
extern int gsn_barbsofalthajji;
extern int gsn_focusfury;

// Utility macros.
#define IS_VALID(data)		((data) != NULL && (data)->valid)
#define VALIDATE(data)		((data)->valid = TRUE)
#define INVALIDATE(data)	((data)->valid = FALSE)
#define UMIN(a, b)		((a) < (b) ? (a) : (b))
#define UMAX(a, b)		((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)		((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define ABS(a)			((a) < 0 ? -1 * (a) : (a))
#define IS_SET(flag, bit)	((flag) & (bit) ? 1 : 0)
#define SET_BIT(var, bit)	((var) |= (bit))
#define REMOVE_BIT(var, bit)	((var) &= ~(bit))
#define TOGGLE_BIT(var, bit)    ((var) ^= (bit))

/*
 * Character macros.
 */
#define IS_NEWBIE(ch)		(ch->level < 9)
#define IS_NPC(ch)		(IS_SET((ch)->act, ACT_IS_NPC))
#define IS_IMMORTAL(ch)		(!IS_NPC(ch) && (ch)->level >= LEVEL_IMMORTAL)
#define IS_IMM_TRUST(ch)	(ch->trust > LEVEL_HERO)
#define IS_HERO(ch)		(ch->level >= LEVEL_HERO)
#define IS_TRUSTED(ch,level)	(get_trust((ch)) >= (level))
#define IS_AFFECTED(ch, sn)	(IS_SET((ch)->affected_by, (sn)) ? 1 : 0)
#define IS_NAFFECTED(ch, sn)	(IS_SET((ch)->naffected_by, (sn)) ? 1 : 0)
#define IS_OAFFECTED(ch, sn)    (IS_SET((ch)->oaffected_by, (sn)) ? 1 : 0)
#define IS_PAFFECTED(ch, sn)    (IS_SET((ch)->paffected_by, (sn)) ? 1 : 0)

#define GET_AGE(ch)		((int) (17 + ((ch)->played \
				    + current_time - (ch)->logon )/72000))

#define IS_PERM_DEAD(ch)	(!IS_IMMORTAL((ch)) && ((ch)->perm_stat[STAT_CON] < 4 || (ch)->pcdata->max_deaths <= (ch)->pcdata->death_count || (ch)->pcdata->age_group == AGE_DEAD))

/* Good and evil were defined as being <= -350 and >= 350 */
#define IS_NEUTER(ch)		((ch)->sex == SEX_NEUTRAL)
#define IS_FEMALE(ch)		((ch)->sex == SEX_FEMALE)
#define IS_MALE(ch)		((ch)->sex == SEX_MALE)
#define IS_GOOD(ch)		((ch)->alignment > 0)
#define IS_EVIL(ch)		((ch)->alignment < 0)
#define IS_NEUTRAL(ch)		(!IS_GOOD(ch) && !IS_EVIL(ch))

#define IS_TRUE_EVIL(ch)	((is_affected(ch, gsn_obscurealign) && (get_modifier(ch->affected, gsn_obscurealign) < 0)) || (!is_affected(ch, gsn_obscurealign) && (ch->alignment < 0)))
#define IS_TRUE_GOOD(ch)	((is_affected(ch, gsn_obscurealign) && (get_modifier(ch->affected, gsn_obscurealign) > 0)) || (!is_affected(ch, gsn_obscurealign) && (ch->alignment > 0)))
#define IS_TRUE_NEUTRAL(ch)	((is_affected(ch, gsn_obscurealign) && (get_modifier(ch->affected, gsn_obscurealign) == 0)) || (!is_affected(ch, gsn_obscurealign) && (ch->alignment == 0)))

//brazen: macros for karma, Ticket #225
#define FAINTREDAURA		250
#define REDAURA			1000
#define DARKREDAURA		5000
#define BLACKAURA		10000

#define PALEGOLDENAURA		-250
#define GOLDENAURA		-1000
#define BRIGHTGOLDENAURA 	-5000
#define SILVERAURA		-10000

#define IS_AWAKE(ch)		(ch->position > POS_SLEEPING)
#define GET_AC(ch,type)		((ch)->armor[type]			    \
		        + ( IS_AWAKE(ch)			    \
			? dex_app[get_curr_stat(ch,STAT_DEX)].defensive : 0 ))  
#define GET_HITROLL(ch)	\
		((ch)->hitroll+dex_app[get_curr_stat(ch,STAT_DEX)].tohit)
#define GET_DAMROLL(ch) \
		((ch)->damroll+str_app[get_curr_stat(ch,STAT_STR)].todam)

#define IS_OUTSIDE(ch)		(!IS_SET(				    \
				    (ch)->in_room->room_flags,		    \
				    ROOM_INDOORS) && ch->in_room && \
				ch->in_room->sector_type != SECT_INSIDE &&\
				ch->in_room->sector_type != SECT_UNDERGROUND &&\
				ch->in_room->sector_type != SECT_UNDERWATER)

#define ON_GROUND(ch) has_ground(*(ch)->in_room)
#define WAIT_STATE(ch, npulse)	updateWaitState(ch, npulse)
#define DAZE_STATE(ch, npulse)  ((ch)->daze = UMAX((ch)->daze, (npulse)))

// The mount thing is such a hack...
//#define IS_FLYING(ch)		(IS_AFFECTED(ch, AFF_FLYING) || IS_AFFECTED(ch, AFF_FLY_NATURAL) || ch->mount)

#define IS_PK(ch, victim) is_pk(ch, victim)

/*
 * Object macros.
 */
#define CAN_WEAR(obj, part)	(IS_SET((obj)->wear_flags,  (part)))
#define IS_OBJ_STAT(obj, stat)	(IS_SET((obj)->extra_flags[0], (stat)))
#define IS_OBJ_STAT_EXTRA(obj, stat)	(IS_SET((obj)->extra_flags[2], (stat)))
#define IS_WEAPON_STAT(obj,stat)(IS_SET((obj)->value[4],(stat)))
#define WEIGHT_MULT(obj)	((obj)->item_type == ITEM_CONTAINER ? \
	(obj)->value[4] : 100)

#define CAN_DUAL(ch)            (IS_NPC(ch) || ((ch->level >= skill_table[gsn_dual_wield].skill_level[ch->class_num]) && (get_skill(ch, gsn_dual_wield) > 0)))


/*
 * Description macros.
 */
#define APERS(ch, looker)       show_person(ch, looker, false)
#define CPERS(ch, looker)       show_person(ch, looker, true)
#define PERS(ch, looker)        show_person(ch, looker, true)
#define NPERS(ch, looker)       show_person(ch, looker, true)

/* direction opposite macro */
#define OPPOSITE(direction)	((direction == 0) ? 2 : (direction == 1) ? \
				3 : (direction == 2) ? 0 : (direction == 3) ? \
				1 : (direction == 5) ? 4 : 5)

/* time and date macros */
#define TtoD(time)		(time / ((PULSE_TICK * 2) / PULSE_PER_SECOND * NUM_HOURS_DAY))
#define DtoDY(day)		((day % NUM_DAYS_YEAR) + 1)
#define TtoDY(time)		(DtoDY(TtoD(time)))

DECLARE_POISON_FUN(	painpoison	);
DECLARE_POISON_FUN(	sleeppoison	);
DECLARE_POISON_FUN(	tremblingpoison	);
DECLARE_POISON_FUN(	exhaustionpoison);
DECLARE_POISON_FUN(	delusionpoison	);
DECLARE_POISON_FUN(	erosivepoison	);
DECLARE_POISON_FUN(	deathpoison	);

/*
 * Structure for a social in the socials table.
 */
struct	social_type
{
    char      name[20];
    char *    char_no_arg;
    char *    others_no_arg;
    char *    char_found;
    char *    others_found;
    char *    vict_found;
    char *    char_not_found;
    char *      char_auto;
    char *      others_auto;
};



/*
 * Global constants.
 */
extern	const	struct	str_app_type	str_app		[26];
extern	const	struct	int_app_type	int_app		[26];
extern	const	struct	wis_app_type	wis_app		[26];
extern	const	struct	dex_app_type	dex_app		[26];
extern	const	struct	con_app_type	con_app		[26];
extern	const	struct	chr_app_type	chr_app		[27];

extern	const	struct	poison_type	poison_table	[];
extern	const	struct	ethos_type	ethos_table	[MAX_ETHOS];
extern	const	struct	sphere_type	sphere_table	[MAX_SPHERES];
extern	const	struct	path_type	path_table	[MAX_PATH_COUNT];
extern	const	struct	class_type	class_table	[MAX_CLASS];
extern	const	struct	material_type	material_table	[MAX_MATERIALS];
extern	const	struct	weapon_type	weapon_table	[];
extern  const   struct  item_type	item_table	[];
extern	const	struct	wiznet_type	wiznet_table	[];
extern	const	struct	attack_type	attack_table	[];
extern  const	struct  race_type	race_table	[];
extern	const	struct	pc_race_type	pc_race_table 	[];
extern  const   struct  age_type	age_table	[MAX_AGEGROUP];
extern  const	struct	spec_type	spec_table	[];
extern	const	struct	liq_type	liq_table	[];
extern	const	struct	focus_type	focus_table	[];	
extern			int		ep_table	[MAX_LEVEL];
extern	const	struct	skill_type	skill_table	[MAX_SKILL];
extern  const   struct  group_type      group_table	[MAX_GROUP];
extern          struct social_type      social_table	[MAX_SOCIALS];
extern	char *	const			title_table	[MAX_CLASS]
							[MAX_LEVEL+1]
							[2];
extern	char * 	const			druid_title_table[3][MAX_LEVEL+1][2];

static const int learn_table[] = {  3,  5,  7,  8,  9, 10, 11, 12, 13, 15, 17, 20, 23, 26, 29, 32, 34, 37, 40, 43, 46, 50, 60, 70, 80, 85 };

static const char *  const day_name[] =
{
    "Lyrensday", "Iolenday", "Thelansday", "Endenday", "Nimensday",
	"Thethelsday", "Evenday"
};

size_t calculate_total_days();
size_t calculate_day_of_week(size_t totalDays);
size_t calculate_day_of_week();

/*
 * Global variables.
 */
extern		RUMORS		  *	rumor_list;
extern		HELP_DATA	  *	help_first;
extern		SHOP_DATA	  *	shop_first;

extern		CHAR_DATA	  *	char_list;
extern		DESCRIPTOR_DATA   *	descriptor_list;
extern		HOST_DATA	  * 	host_list;
extern		OBJ_DATA	  *	object_list;
extern		CHAIN_DATA	  *	event_list;

extern		CHAR_DATA	  *	ol_char;

extern		int			player_levels;
extern		char			bug_buf		[];
extern		char			master_wordlist	[];
extern		time_t			current_time;
extern		bool			fLogAll;
extern		FILE *			fpReserve;
extern		KILL_DATA		kill_table	[];
extern		char			log_buf		[];
extern		TIME_INFO_DATA		time_info;
extern		WEATHER_DATA		weather_info;

extern		HEADER_DATA *		g_active_headers;
extern		HEADER_DATA *		g_denied_headers;

extern		int			levels_total;
extern		int	global_int_class_waterscholar;
extern		int	global_int_class_earthscholar;
extern		int	global_int_class_voidscholar;
extern		int	global_int_class_spiritscholar;
extern		int	global_int_class_airscholar;
extern		int 	global_int_class_firescholar;
extern		int	global_int_class_watertemplar;
extern		int	global_int_class_earthtemplar;
extern		int	global_int_class_voidtemplar;
extern		int	global_int_class_spirittemplar;
extern		int	global_int_class_airtemplar;
extern		int	global_int_class_firetemplar;
extern		int	global_int_class_thief;
extern		int	global_int_class_watcher;
extern		int 	global_int_class_assassin;
extern		int	global_int_class_bandit;
extern		int	global_int_class_rogue1;
extern		int	global_int_class_rogue2;
extern		int	global_int_class_fighter;
extern		int	global_int_class_swordmaster;
extern		int	global_int_class_barbarian;
extern		int	global_int_class_gladiator;
extern		int	global_int_class_warrior;
extern		int     global_int_class_ranger;
extern		int	global_int_class_gleeman;
extern		int	global_int_class_bard;
extern		int	global_int_class_entertainer;
extern		int	global_int_class_alchemist;
extern		int	global_int_class_psionicist;
extern		int	global_int_class_druid;

extern		int	global_int_race_human;
extern		int	global_int_race_alatharya;
extern		int	global_int_race_srryn;
extern		int	global_int_race_kankoran;
extern		int	global_int_race_chtaren;
extern		int	global_int_race_aelin;
extern		int	global_int_race_shuddeni;
extern		int	global_int_race_nefortu;
extern		int	global_int_race_caladaran;
extern		int	global_int_race_ethron;
extern		int	global_int_race_chaja;
extern		int	global_int_race_seraph;

extern		bool			global_bool_full_damage;
extern		bool			global_bool_ranged_attack;
extern		bool			global_counter_works;
extern		bool			global_bool_sending_brief;
extern		char *			global_damage_noun;
extern		char *			global_damage_from;
extern		CHAR_DATA *		global_mimic;
extern		OBJ_DATA *		global_obj_target;
extern		bool			loading_char;
extern		bool			protect_works;
extern		bool			crumble_done;
extern		char			confused_text[MAX_STRING_LENGTH];
extern		int			silver_state;
extern		int			segv_counter;
extern		char			segv_char[128];
extern		char			segv_cmd[MAX_INPUT_LENGTH+1];
extern		char			*dictwords;
extern		unsigned short		port;
extern	const	char *			align_names[4];

extern		bool			global_song_message;

extern		bool			global_linked_move;

extern		bool			global_option_nomysql;

/* prog stuff */
extern		bool			verb_stop_issued;

extern	char *get_word(FILE *fp);

/*
 * OS-dependent declarations.
 * These are all very standard library functions,
 *   but some systems have incomplete or non-ansi header files.
 */
#if	defined(_AIX)
char *	crypt		args( ( const char *key, const char *salt ) );
#endif

#if	defined(apollo)
int	atoi		args( ( const char *string ) );
void *	calloc		args( ( unsigned nelem, size_t size ) );
char *	crypt		args( ( const char *key, const char *salt ) );
#endif

#if	defined(hpux)
char *	crypt		args( ( const char *key, const char *salt ) );
#endif

#if	defined(linux)
char *	crypt		args( ( const char *key, const char *salt ) );
#endif

#if	defined(macintosh)
#define NOCRYPT
#if	defined(unix)
#undef	unix
#endif
#endif

#if	defined(MIPS_OS)
char *	crypt		args( ( const char *key, const char *salt ) );
#endif

#if	defined(MSDOS)
#define NOCRYPT
#if	defined(unix)
#undef	unix
#endif
#endif

#if	defined(WIN32)
#define NOCRYPT
#if	defined(unix)
#undef	unix
#endif
#endif


#if	defined(NeXT)
char *	crypt		args( ( const char *key, const char *salt ) );
#endif

#if	defined(sequent)
char *	crypt		args( ( const char *key, const char *salt ) );
int	fclose		args( ( FILE *stream ) );
int	fprintf		args( ( FILE *stream, const char *format, ... ) );
int	fread		args( ( void *ptr, int size, int n, FILE *stream ) );
int	fseek		args( ( FILE *stream, long offset, int ptrname ) );
void	perror		args( ( const char *s ) );
int	ungetc		args( ( int c, FILE *stream ) );
#endif

#if	defined(sun)
char *	crypt		args( ( const char *key, const char *salt ) );
int	fclose		args( ( FILE *stream ) );
int	fprintf		args( ( FILE *stream, const char *format, ... ) );
#if	defined(SYSV)
siz_t	fread		args( ( void *ptr, size_t size, size_t n, 
			    FILE *stream) );
#else
int	fread		args( ( void *ptr, int size, int n, FILE *stream ) );
#endif
int	fseek		args( ( FILE *stream, long offset, int ptrname ) );
void	perror		args( ( const char *s ) );
int	ungetc		args( ( int c, FILE *stream ) );
#endif

#if	defined(ultrix)
char *	crypt		args( ( const char *key, const char *salt ) );
#endif



/*
 * The crypt(3) function is not available on some operating systems.
 * In particular, the U.S. Government prohibits its export from the
 *   United States to foreign countries.
 * Turn on NOCRYPT to keep passwords in plain text.
 */
#if	defined(NOCRYPT)
#define crypt(s1, s2)	(s1)
#endif



/*
 * Data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 *
 * The NULL_FILE is held open so that we have a stream handle in reserve,
 *   so players can go ahead and telnet to all the other descriptors.
 * Then we close it whenever we need to open a file (e.g. a save file).
 */
#if defined(macintosh)
#define PLAYER_DIR	""			/* Player files	*/
#define TEMP_FILE	"romtmp"
#define NULL_FILE "proto.are" /* To reserve one stream */
#endif

// #if defined(WIN32) || defined(MSDOS)
// #define ACCT_DIR	"..\accounts\"
// #else
// #define ACCT_DIR	"../accounts/"
// #endif


#define ROOMSAVE_DIR	"../rooms/"
#define PLAYER_DIR      "../player/"        	/* Player files */
#define DENIED_DIR	"../player/denied/"
#define PLAYLOG_DIR	"../playlogs/"		/* slog directory */
#define ACCT_DIR	"../accounts/"
#define FACTION_DIR	"../faction/"

#define ROOMSAVELIST    "_room.lst"
#define PLAYERLIST	"_player.lst"
#define GOD_DIR         "../gods/"  		/* list of gods */
#define TEMP_FILE	"../player/romtmp"
#define PURGEFILE	"../player/purgefile"
#define MOB_DIR         "MOBProgs/"     	/* MOBProg files */
#define MEMPATH		"../mem/"     		/*the 8192 byte bitstream*/
#define TRAVELPATH	"../travel/"     	/*the 8192 byte bitstream*/
#define MAILFILE 	"emails.txt"
#define LIMITFILE	"limits.dat" 		/* limits in a file, no more bootlag */
#define LIMITTEMP	"limits.tmp" 		/* limits in a file, no more bootlag */

#if defined(unix)
#define NULL_FILE       "/dev/null" 		/* To reserve one stream */
#define FULLPATH 	"/home/pantheon/devrom/Rom24"
#else
#define NULL_FILE	"nul"			/* To reserve one stream */
#define FULLPATH 	"/Avendar"
#endif

#define AREA_LIST       "area.lst"  		/* List of areas*/
#define PLAYER_LIST	"../player/_player.lst" /* List of pfiles for limit & level scan */
#define BUG_FILE        "bugs.txt" 		/* For 'bug' and bug()*/
#define TYPO_FILE       "typos.txt" 		/* For 'typo'*/
#define NOTE_FILE       "notes.not"		/* For 'notes'*/
#define IDEA_FILE	"ideas.not"
#define PENALTY_FILE	"penal.not"
#define NEWS_FILE	"news.not"
#define CHANGES_FILE	"chang.not"
#define ROOMNOTE_FILE	"roomnote.not"
#define SHUTDOWN_FILE   "../DOWN" 		/* For 'shutdown'*/
#define BAN_FILE	"ban.txt"
#define MUSIC_FILE	"music.txt"
#define NEWBIE_FILE	"newchat.log"
#define FACTION_FILE	"factions.txt"
#define NAME_FILE	"namelist.txt"
#define WEAVE_FILE  "weave.txt"
#define OATHS_FILE   "oaths.txt"

#define BADNAME_FILE	"_badnames.txt"
#define DENIEDNAME_FILE "_denied.txt"
#define RUMOR_FILE	"../area/rumors.txt"
/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */
#define CD	CHAR_DATA
#define MID	MOB_INDEX_DATA
#define OD	OBJ_DATA
#define OID	OBJ_INDEX_DATA
#define RID	ROOM_INDEX_DATA
#define SF	SPEC_FUN
#define AD	AFFECT_DATA

/* act_comm.c */
void show_group_listing(CHAR_DATA * ch, CHAR_DATA * viewer);
void set_leader(CHAR_DATA * ch, CHAR_DATA * leader);
void  	check_sex	args( ( CHAR_DATA *ch) );
void	add_follower	args( ( CHAR_DATA *ch, CHAR_DATA *master ) );
void	delete_char	args( ( CHAR_DATA *victim ) );
void	deny_char	args( ( CHAR_DATA *victim ) );
void	stop_follower	args( ( CHAR_DATA *ch, bool fromExtract = false ) );
void 	nuke_pets	args( ( CHAR_DATA *ch ) );
void	die_follower(CHAR_DATA *ch, bool fromExtract = false, bool retainCharmed = false);
bool	is_same_group	args( ( CHAR_DATA *ach, CHAR_DATA *bch ) );
void    show_char_to_char_0     args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
void    check_comm      args( ( CHAR_DATA *ch, char *str ) );
void	play_sound_room args( ( CHAR_DATA *ch, char *argument ) );
bool	play_sound	args( ( CHAR_DATA *ch, char *argument ) );
bool	play_sound_adv	args( ( CHAR_DATA *ch, char *argument, int volume, int priority, char *stype) );
void    show_char_to_char_1     args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
char *	stripcolor	args( ( char *buffer, const char *txt ) );
void    do_pose		args( ( CHAR_DATA *ch, char *argument ) );
void    do_tell_target(CHAR_DATA * ch, CHAR_DATA * victim, char * argument);

/* act_enter.c */
RID  *get_random_room   args ( (CHAR_DATA *ch) );
RID  *get_random_room_area   args ( (CHAR_DATA *ch) );
RID  *get_random_room_no_char   args ( ( void ) );

/* act_info.c */
void show_affects(CHAR_DATA *ch, CHAR_DATA * viewer, bool new_affect);
void	set_title	args( ( CHAR_DATA *ch, char *title ) );
void    set_extitle     args( ( CHAR_DATA *ch, char *extitle ) );
int     is_lawful               args( (CHAR_DATA *ch) );
int     is_balanced             args( (CHAR_DATA *ch) );
int     is_chaotic              args( (CHAR_DATA *ch) );
bool	check_blind		args( (CHAR_DATA *ch) );
void	show_helpfile	args( ( CHAR_DATA *ch, DESCRIPTOR_DATA *d, char *argument, int hflags ) );
char *  display_date	args( ( char *string) );
bool is_flying(const CHAR_DATA *ch);
bool flight_blocked(const CHAR_DATA & ch);
bool can_fly_natural(const CHAR_DATA & ch);
bool can_fly_magical(const CHAR_DATA & ch);
bool can_fly(const CHAR_DATA & ch);
void stop_flying(CHAR_DATA & ch);
void	sentrynotify	args( ( CHAR_DATA *ch, int enter ) );
bool tryDisbelieve(CHAR_DATA * ch, CHAR_DATA * victim);
/* act_move.c */
int	find_door	args( ( CHAR_DATA *ch, char *arg ) );
void    add_tracks      args( ( ROOM_INDEX_DATA *room, CHAR_DATA *ch, int door) );
bool	has_boat	args( ( CHAR_DATA *ch ) );
void	move_char( CHAR_DATA *ch, int door, bool follow, bool charmCanMove = false);
std::vector<CHAR_DATA *> find_cloakers(CHAR_DATA * ch);
CHAR_DATA *cloak_remove    args( ( CHAR_DATA *ch ) );
bool    unhide_char     args( ( CHAR_DATA *ch ) );
bool    uncamo_char     args( ( CHAR_DATA *ch ) );
extern const int movement_loss[SECT_MAX];
/* act_obj.c */
extern	char *	const	where_name[];
void desiccate_corpse	args( (OBJ_DATA *corpse ) );
bool can_loot		args( (CHAR_DATA *ch, OBJ_DATA *obj) );
void    get_obj         args( ( CHAR_DATA *ch, OBJ_DATA *obj,
                            OBJ_DATA *container ) );
void activate_demon_bind args((CHAR_DATA *ch, OBJ_DATA *obj) );
void	wear_obj	args( (CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace, bool mobReset, bool twohand ) );
bool    remove_obj  args( (CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace ) );


/* act_wiz.c */
void wiznet		args( (char *string, CHAR_DATA *ch, void *obj,
			       long flag, long flag_skip, int min_level ) );
ROOM_INDEX_DATA *       find_location   args( ( CHAR_DATA *ch, OBJ_DATA *obj,
						char *arg ) );
void brand_char		args( (CHAR_DATA *ch, CHAR_DATA *vch, int sn, bool prog) );

/* alias.c */
void 	substitute_alias args( (DESCRIPTOR_DATA *d, char *input) );

/* ban.c */
bool	check_ban	args( ( char *site, int type) );


/* comm.c */
const char * show_person(CHAR_DATA * ch, CHAR_DATA * looker, bool checkDisguise);
void	show_string	args( ( struct descriptor_data *d, char *input) );
void	close_socket	args( ( DESCRIPTOR_DATA *dclose ) );
void	write_to_buffer	args( ( DESCRIPTOR_DATA *d, const char *txt,
			    int length ) );
void	send_to_char	args( ( const char *txt, CHAR_DATA *ch ) );
void	page_to_char	args( ( const char *txt, CHAR_DATA *ch ) );
void	printf_to_char	args( ( CHAR_DATA *ch, char *fmt, ... ) );
void	emote_act		args( ( const char *format, CHAR_DATA *ch,
			    const void *arg1, const void *arg2, int type ) );
void	tc_act		args( ( const char *format, CHAR_DATA *ch,
			    const void *arg1, const void *arg2, int type ) );
void	c_act		args( ( const char *format, CHAR_DATA *ch,
			    const void *arg1, const void *arg2, int type ) );
void	wizact		args( ( const char *format, CHAR_DATA *ch,
			    const void *arg1, const void *arg2, int type ) );
void	act		args( ( const char *format, CHAR_DATA *ch,
			    const void *arg1, const void *arg2, int type ) );
void	emote_act_new		args( ( const char *format, CHAR_DATA *ch, 
			    const void *arg1, const void *arg2, int type,
			    int min_pos) );
void	tc_act_new		args( ( const char *format, CHAR_DATA *ch, 
			    const void *arg1, const void *arg2, int type,
			    int min_pos) );
void	c_act_new		args( ( const char *format, CHAR_DATA *ch, 
			    const void *arg1, const void *arg2, int type,
			    int min_pos) );
void	act_new		args( ( const char *format, CHAR_DATA *ch, 
			    const void *arg1, const void *arg2, int type,
			    int min_pos) );
void	wizact_new	args( ( const char *format, CHAR_DATA *ch, 
			    const void *arg1, const void *arg2, int type,
			    int min_pos) );
void    act_nnew        args( ( const char *format, CHAR_DATA *ch,
                            const void *arg1, const void *arg2, int type,
                            int min_pos) );
/*
 * Colour stuff by Lope of Loping Through The MUD
 */
int	colour		args( ( char type, CHAR_DATA *ch, char *string ) );
void	colourconv	args( ( char *buffer, const char *txt, CHAR_DATA *ch ) );
void	send_to_char_bw	args( ( const char *txt, CHAR_DATA *ch ) );
void	page_to_char_bw	args( ( const char *txt, CHAR_DATA *ch ) );

/* db.c */
char *	print_flags	args( ( int flag ));
void	boot_db		args( ( void ) );
void	area_update	args( ( void ) );
void	lunar_update	args ( (void) );
CD *	create_mobile	args( ( MOB_INDEX_DATA *pMobIndex ) );
void	clone_mobile	args( ( CHAR_DATA *parent, CHAR_DATA *clone) );

OD *	create_object	args( ( OBJ_INDEX_DATA *pObjIndex, int level ) );


void	clone_object	args( ( OBJ_DATA *parent, OBJ_DATA *clone ) );
void	clear_char	args( ( CHAR_DATA *ch ) );
char *	get_extra_descr	args( ( const char *name, EXTRA_DESCR_DATA *ed ) );
MID *	get_mob_index	args( ( int vnum ) );
OID *	get_obj_index	args( ( int vnum ) );
RID *	get_room_index	args( ( int vnum ) );
char	fread_letter	args( ( FILE *fp ) );
int	fread_number	args( ( FILE *fp ) );
long 	fread_flag	args( ( FILE *fp ) );
char *	fread_string	args( ( FILE *fp ) );
char *  fread_string_eol args(( FILE *fp ) );
void	fread_to_eol	args( ( FILE *fp ) );
char *	fread_word	args( ( FILE *fp ) );
char *	fread_dyn	args( ( FILE *fp, char *word, const unsigned long max_chars ) );
long	flag_convert	args( ( char letter) );
char *	fwrite_format	args( ( char *str ) );
void *	alloc_perm	args( ( int sMem ) );
char *	str_dup		args( ( const char *str ) );
void	free_string	(char *&pstr);
int	number_fuzzy	args( ( int number ) );
int	number_range	args( ( int from, int to ) );
int	number_percent	args( ( void ) );
int	number_door	args( ( void ) );
int	number_bits	args( ( int width ) );
long     number_mm       args( ( void ) );
int	dice		args( ( int number, int size ) );
int	interpolate	args( ( int level, int value_00, int value_32 ) );
void	smash_dollar	args( ( char *str ) );
void	smash_tilde	args( ( char *str ) );
void    strip_punc	args( ( char *str, char *nopunc ) );
void	smash_punc	args( ( char *str ) );
bool	str_infix	args( ( const char *astr, const char *bstr ) );
bool	str_suffix	args( ( const char *astr, const char *bstr ) );
char *	capitalize	args( ( const char *str ) );
char *	front_capitalize	args( ( char *str ) );
void	append_file	args( ( CHAR_DATA *ch, char *file, char *str ) );
void	bug		args( ( const char *str, int param ) );
void    write_slog	args( ( CHAR_DATA *ch, char *str ) );
void	log_string	args( ( const char *str ) );
void    char_equip_crumbling	args( ( void ) );

void	log_mysql_error args( ( void ) );

/* effect.c */
void	acid_effect	args( (void *vo, int level, int dam, int target) );
void	cold_effect	args( (void *vo, int level, int dam, int target) );
void	fire_effect	args( (void *vo, int level, int dam, int target) );
void	poison_effect	args( (void *vo, int level, int dam, int target) );
void	shock_effect	args( (void *vo, int level, int dam, int target) );


/* fight.c */
OBJ_DATA *check_sidestep args( ( CHAR_DATA *ch, CHAR_DATA *victim) );
void    raw_kill        args( ( CHAR_DATA *victim ) );
void	kill_char	args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
void 	do_flee		args( (CHAR_DATA *ch, char *argument) );
bool 	is_safe		args( (CHAR_DATA *ch, CHAR_DATA *victim ) );
bool 	is_safe_druid	args( (CHAR_DATA *ch ) );
bool 	is_safe_spell	args( (CHAR_DATA *ch, CHAR_DATA *victim, bool area ) );
bool    is_blindfighter args( (CHAR_DATA *ch, bool sword) );
void	violence_update	args( ( void ) );
int     check_extra_damage args( (CHAR_DATA *ch, int dam, OBJ_DATA *wield) );
void	multi_hit	args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
void	one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt, int htype, bool canLoseAttack = true);
int     modify_damage(CHAR_DATA * ch, CHAR_DATA *victim, OBJ_DATA * ch_weapon, int dam, int dt, int dam_type, bool & immune);
bool	damage		args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int class_num, bool show ) );
bool    damage_old      args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int class_num, bool show ) );
bool	damage_new(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int dam_type, bool show, char *attack, const char * attackerName = NULL);
bool 	damage_from_obj args( ( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj,
				int dam, int dt, int dam_type, bool show));
void	update_pos	args( ( CHAR_DATA *victim ) );
void	stop_fighting	args( ( CHAR_DATA *ch ) );
void	stop_fighting_all args((CHAR_DATA *ch ) );
void	check_killer	args( ( CHAR_DATA *ch, CHAR_DATA *victim) );
void                    death_cry       args( ( CHAR_DATA *ch ) );
bool	check_reach	args( ( CHAR_DATA *ch, CHAR_DATA *victim, int diff, int faillag ) );
void	set_fighting	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool	check_defensiveroll args( (CHAR_DATA *victim) );
/* gold.c */
void	coins_from_char	args( ( CHAR_DATA *ch, long coins, int ctype ) );
void	coins_to_char	args( ( CHAR_DATA *ch, long coins, int ctype ) );
float	coins_to_value	args( ( long *coins ) );
long *	value_to_coins	args( ( float value, bool fuzzy ) );
char *	coins_to_str	args( ( long *coins ) );
char *	coins_to_sstr	args( ( long *coins ) );
char *	value_to_str	args( ( float value ) );
char *	value_to_sstr	args( ( float value ) );
void	inc_player_coins args(( CHAR_DATA *ch, long *coins ) );
bool	dec_player_coins args(( CHAR_DATA *ch, long *coins ) );
bool	dec_player_bank  args(( CHAR_DATA *ch, long *coins ) );
long *	convert_coins	args( ( long *coins ) );
bool	coins_from_coinpool args((long *coins, long *coinpool ) );
int	aura_grade	args( ( CHAR_DATA *ch) );
int effective_karma(CHAR_DATA & ch);

/* handler.c */
bool is_pk(CHAR_DATA * ch, CHAR_DATA * victim);
int get_carry_weight(const CHAR_DATA & ch);
bool has_ground(const ROOM_INDEX_DATA & room);
void updateWaitState(CHAR_DATA * ch, int beats);
bool is_spellcaster(CHAR_DATA * ch);
void modify_karma(CHAR_DATA * ch, int mod);
void expend_mana(CHAR_DATA * ch, int mana);
void	switch_position args( ( CHAR_DATA *ch, int pos ) );
int     get_mval        args( ( CHAR_DATA *ch, char *argument ) );
bool    mob_remembers   args( ( CHAR_DATA *mob, char *argument ) );
bool 	stone_check	args((CHAR_DATA *ch, int sn));
AD  	*affect_find args( (AFFECT_DATA *paf, int sn));
void	affect_check	args( (CHAR_DATA *ch, int where, int vector) );
int	count_users	args( (OBJ_DATA *obj) );
void 	deduct_cost	args( (CHAR_DATA *ch, float cost) );
void	affect_enchant	args( (OBJ_DATA *obj) );
RID     *get_default_hometown args( ( CHAR_DATA * ch ) );
int     get_resist      args( (CHAR_DATA *ch, int res_type) );
int	liq_lookup	args( ( const char *name) );
int	weapon_lookup	args( ( const char *name) );
int	weapon_type	args( ( const char *name) );
char 	*weapon_name	args( ( int weapon_Type) );
int	item_ref	args( ( int item_type) );
int	item_lookup	args( ( const char *name) );
char	*item_name	args( ( int item_type) ); 
int	god_lookup	args( ( const char *name) );
int	season_lookup	args( ( const char *name) );
int	attack_lookup	args( ( const char *name) );
int	race_lookup	args( ( const char *name) );
long	wiznet_lookup	args( ( const char *name) );
int	ethos_lookup	args( ( const char *name) );
int	profession_lookup	args( (const char *name) );
int	sphere_lookup	args( ( const char *name) );
int	path_lookup(const char *name);
int	class_lookup	args( ( const char *name) );
int     arrow_lookup    args( ( const char *name) );
int	get_current_month args (());
int	get_month	args( ( int day ) );
bool	is_clan		args( (CHAR_DATA *ch) );
bool	is_same_clan	args( (CHAR_DATA *ch, CHAR_DATA *victim));
bool	is_old_mob	args ( (CHAR_DATA *ch) );
int	get_skill	args( ( CHAR_DATA *ch, int sn, bool displayOnly = false ) );
int	get_weapon_sn	args( ( CHAR_DATA *ch ) );
int	get_dual_sn	args( ( CHAR_DATA *ch ) );
int	get_weapon_sn_from_obj args ((OBJ_DATA *item));
bool	is_weapon_skill args( ( int sn ) );
int	get_weapon_skill args(( CHAR_DATA *ch, int sn, bool primary ) );
int	get_weapon_skill_weapon args((CHAR_DATA *, OBJ_DATA *obj, bool displayOnly = false));
int     get_age         args( ( CHAR_DATA *ch ) );
int	get_save	args( ( CHAR_DATA *ch ) );
void	reset_char	args( ( CHAR_DATA *ch )  );
int	get_trust	args( ( CHAR_DATA *ch ) );
int	get_curr_stat	args( (const CHAR_DATA *ch, int stat ) );
int	get_max_stat	args( (const CHAR_DATA *ch, int stat ) );
void	set_perm_stat	args( ( CHAR_DATA *ch, int stat, int value ) );
void	set_mod_stat	args( ( CHAR_DATA *ch, int stat, int value ) );
void	set_max_stat	args( ( CHAR_DATA *ch, int stat, int value ) );
int	get_hp_bonus	args( ( CHAR_DATA *ch ) );
int	get_mana_bonus	args( ( CHAR_DATA *ch ) );
int	get_move_bonus	args( ( CHAR_DATA *ch ) );
int 	get_max_train	args( ( CHAR_DATA *ch, int stat ) );
int	get_max_hunger	args( ( CHAR_DATA *ch, PC_DATA * pcdata = NULL ) );
bool	is_name		(const char *str, const char *namelist);
bool	is_name_nopunc	args( ( char *str, char *namelist ) );
char *	is_name_prefix	args( ( char *str, char *namelist ) );
bool	is_exact_name	(const char *str, const char *namelist);
void	affect_to_char	args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void	affect_to_obj	args( ( OBJ_DATA *obj, AFFECT_DATA *paf ) );
void	affect_to_area	args( ( AREA_DATA *area, AFFECT_DATA *paf) );
void	affect_to_room	args( ( ROOM_INDEX_DATA *room, AFFECT_DATA *paf) );
void	affect_remove	args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void	affect_remove_obj args( (OBJ_DATA *obj, AFFECT_DATA *paf ) );
void	affect_remove_area args( ( AREA_DATA *area, AFFECT_DATA *paf) );
void	affect_remove_room args( ( ROOM_INDEX_DATA *room, AFFECT_DATA *paf) );
void	affect_strip	args( ( CHAR_DATA *ch, int sn ) );
void	naffect_strip	args( ( CHAR_DATA *ch, int sn ) );
void	object_affect_strip args( ( OBJ_DATA *obj, int sn ) );
void	room_affect_strip args( ( ROOM_INDEX_DATA *room, int sn ) );
void	area_affect_strip args( ( AREA_DATA *area, int sn ) );
bool is_affected(const CHAR_DATA *ch, int sn);
bool	is_loc_affected	args( ( CHAR_DATA *ch, int sn, int loc ) );
bool	is_loc_not_affected	args( ( CHAR_DATA *ch, int sn, int loc ) );
int 	get_modifier	args( ( AFFECT_DATA *paf, int sn ) );
int 	get_aff_level	args( ( AFFECT_DATA *paf, int sn ) );
void *  get_aff_point	args( ( AFFECT_DATA *paf, int sn ) );
void 	up_modifier	args( ( AFFECT_DATA *paf, int sn ) );
int 	get_high_modifier	args( ( AFFECT_DATA *paf, int sn ) );
bool	obj_is_affected	args( ( OBJ_DATA *obj, int sn ) );
int     count_obj_affects(const OBJ_DATA * obj, int sn);
OBJ_DATA *get_foci  args( ( CHAR_DATA *ch, bool world ) );
OBJ_DATA *get_obj_potioncontainer args( ( CHAR_DATA *ch) );
AFFECT_DATA * get_obj_affect(const OBJ_DATA *obj, int sn, const AFFECT_DATA * lastAffect = NULL);
AFFECT_DATA * get_affect(const CHAR_DATA *ch, int sn, const AFFECT_DATA * lastAffect = NULL);
AFFECT_DATA * get_area_affect args( (AREA_DATA * area, int sn, AFFECT_DATA * lastAffect = NULL) );
AFFECT_DATA * get_room_affect args( (ROOM_INDEX_DATA * room, int sn, AFFECT_DATA * lastAffect = NULL) );
AFFECT_DATA * get_room_affect_with_modifier(ROOM_INDEX_DATA * room, int sn, int modifier);
bool	area_is_affected args( ( AREA_DATA *area, int sn) );
bool	room_is_affected args( ( ROOM_INDEX_DATA *room, int sn) );
void	affect_join	args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void	obj_affect_join	args( ( OBJ_DATA *obj, AFFECT_DATA *paf ) );
void	char_from_room	args( ( CHAR_DATA *ch ) );
void	char_to_room	args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex ) );
void	obj_to_char	args( ( OBJ_DATA *obj, CHAR_DATA *ch ) );
void	obj_from_char	args( ( OBJ_DATA *obj ) );
int	apply_ac	args( ( OBJ_DATA *obj, int iWear, int type ) );
OD *	get_eq_char	args( (const CHAR_DATA *ch, int iWear ) );
void	equip_char	args( ( CHAR_DATA *ch, OBJ_DATA *obj, int iWear ) );
void	unequip_char	args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
int	count_obj_list	args( ( OBJ_INDEX_DATA *obj, OBJ_DATA *list ) );
void	obj_from_room	args( ( OBJ_DATA *obj ) );
void	obj_to_room	args( ( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex ) );
void	obj_to_obj	args( ( OBJ_DATA *obj, OBJ_DATA *obj_to ) );
void	obj_from_obj	args( ( OBJ_DATA *obj ) );
void	extract_obj	args( ( OBJ_DATA *obj ) );
void	extract_char(CHAR_DATA *ch, bool fPull, ROOM_INDEX_DATA * room = NULL);
CD *	room_get_char_room args((ROOM_INDEX_DATA *room, char *argument ) );
CD *	room_get_char_world args((ROOM_INDEX_DATA *room, char *argument));
CD *	obj_get_char_room args (( OBJ_DATA *obj, char *argument ) );
CD *    obj_get_char_world args (( OBJ_DATA *obj, char *argument ) );
bool    verify_pc_world(CHAR_DATA * ch);
bool    verify_char_room(CHAR_DATA * ch, ROOM_INDEX_DATA * room);
bool    verify_char_world(CHAR_DATA * ch);
CD *	get_char_room	( CHAR_DATA *ch, const char *argument );
CHAR_DATA *get_char_room(CHAR_DATA *ch, ROOM_INDEX_DATA * room, const char *argument);
CD *	get_char_world	(CHAR_DATA *ch, const char *argument);
CD *	get_char_area	args( ( CHAR_DATA *ch, char *argument ) );
CD *	get_char_by_id  args( ( long id ) );
CHAR_DATA * get_char_by_id_any(long id);
CHAR_DATA * get_char_by_id_any_room(long id, const ROOM_INDEX_DATA & room);
OD *	get_obj_type	args( ( OBJ_INDEX_DATA *pObjIndexData ) );
OD *	get_obj_list	args( ( CHAR_DATA *ch, char *argument, OBJ_DATA *list ) );
OD *	get_obj_carry	args( ( CHAR_DATA *ch, char *argument, CHAR_DATA *viewer ) );
OD *	get_obj_wear	args( ( CHAR_DATA *ch, char *argument ) );
OD *	get_obj_wear_loot args( (CHAR_DATA *ch, char *argument ) );
OD *	get_obj_here	args( ( CHAR_DATA *ch, char *argument ) );
OD *    get_obj_room    args( (CHAR_DATA * ch, char *argument ) );
ROOM_INDEX_DATA * get_room_for_obj(const OBJ_DATA & obj);
bool    verify_obj_world(OBJ_DATA * obj);
OD *	get_obj_world	args( ( CHAR_DATA *ch, char *argument ) );
OD *    room_get_obj_world(ROOM_INDEX_DATA * room, char * argument);
OD *	obj_get_obj_world args((OBJ_DATA *ch, char *argument));
OD *	obj_get_obj_here args(( OBJ_DATA *ch, char *argument));
OD *    room_get_obj_here(ROOM_INDEX_DATA * room, char * argument);
OD *	create_money		args( ( long amount, int ctype ) );
OD *	create_money_concealed	args( ( long amount, int ctype ) );
OD *	create_money_stashed	args( ( long amount, int ctype ) );
int	get_obj_number	args( ( OBJ_DATA *obj ) );
int get_weight_contents(const OBJ_DATA & obj);
int	get_obj_weight(const OBJ_DATA *obj);
int	get_true_weight(const OBJ_DATA *obj);
bool	room_is_dark	args( ( ROOM_INDEX_DATA *pRoomIndex ) );
bool	is_room_owner	args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *room) );
bool	room_is_private	args( ( ROOM_INDEX_DATA *pRoomIndex ) );
char *	str_linecopy	args( ( char *buf_to, char *buf_from ) );
bool	can_see		args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool	can_see_comm	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool	can_see_obj	args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
bool	can_see_room	args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex) );
bool	can_drop_obj	args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
bool	can_see_in_room args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex) );
char *	affect_loc_name	args( ( int location ) );
const char *	affect_bit_name	args( ( AFFECT_DATA *paf ) );
const char *	extra_bit_name	(unsigned long extra_flags, int field, bool hideImmOnlyBits = false);
const char * 	wear_bit_name	args( ( int wear_flags ) );
const char *	act_bit_name	args( ( int act_flags ) );
const char *	off_bit_name	args( ( int off_flags ) );
const char *  imm_bit_name	args( ( int imm_flags ) );
char *  cdata_bit_name	args( ( int cdata_flags ) );
const char * 	form_bit_name	args( ( int form_flags ) );
const char *	part_bit_name	args( ( int part_flags ) );
const char *	weapon_bit_name	args( ( int weapon_flags ) );
const char *  comm_bit_name	args( ( int comm_flags ) );
const char *	cont_bit_name	args( ( int cont_flags) );


/* interp.c */
void	interpret	args( ( CHAR_DATA *ch, char *argument ) );
void	proginterpret	args( ( CHAR_DATA *ch, char *argument ) );
void    ointerpret      args( ( OBJ_DATA *ch, char *argument ) );
void	rinterpret	args( ( ROOM_INDEX_DATA *ch, char *argument ) );
bool	is_number	args( (const char *arg ) );
int	mult_argument	args( ( char *argument, char *arg) );

bool 	can_be_affected args( ( CHAR_DATA *ch, int sn) );

/* recycle.c */
long    get_pc_id       args( ( void ) );

/* lookup.c */
int	size_lookup	args( ( const char *name ) );

/* magic.c */
void perform_spellcasting(CHAR_DATA * ch, char * argument, bool adamantine);
bool	check_dispel	(int dis_level, CHAR_DATA *victim, int sn);
int	find_spell( CHAR_DATA *ch, const char *name, SPELL_FUN funType);
int 	mana_cost 	(CHAR_DATA *ch, int min_mana, int level);
int	skill_lookup	args( ( const char *name ) );
int	skill_lookup_full	args( ( const char *name ) );
int	material_lookup	args( ( const char *name ) );
int	slot_lookup	args( ( int slot ) );
bool	saves_spell	args( ( int level, CHAR_DATA * ch, CHAR_DATA *victim, int dam_type ) );
void	obj_cast_spell	args( ( int sn, int level, CHAR_DATA *ch,
				    CHAR_DATA *victim, OBJ_DATA *obj ) );


void    destroy_icyprison args( ( ROOM_INDEX_DATA *pRoomIndex,
				  ROOM_INDEX_DATA *tRoom ) );

/* mob_prog.c */
#ifdef DUNNO_STRSTR
char *  strstr                  args ( (const char *s1, const char *s2 ));
#endif

void	mprog_do_trans		args( ( const char *ch, PROG_RUNDATA *prog, char *t) );

PROG_VARDATA *mprog_find_pvar	args(( const char *txt, PROG_RUNDATA *prog, bool create));

bool check_verbs(CHAR_DATA * ch, const char * command, const char * subbed_command, char * text, const char * subbed_text);

void    mprog_wordlist_check    args ( ( char * arg, CHAR_DATA *mob,
                                        CHAR_DATA* actor, OBJ_DATA* object,
                                        void* vo, int type ) );
void    mprog_percent_check     args ( ( CHAR_DATA *mob, CHAR_DATA* actor,
                                        OBJ_DATA* object, void* vo,
                                        int type ) );
void	rprog_percent_check	args ( ( ROOM_INDEX_DATA *room, CHAR_DATA *actor,
					OBJ_DATA *obj, void *vo, int type ) );

void    mprog_time_trigger	args ( ( CHAR_DATA* mob ) );
void	mprog_load_trigger	args ( ( CHAR_DATA* mob) );
void    mprog_demon_trigger	args ( ( CHAR_DATA* demon ) );
void    mprog_act_trigger       args ( ( char* buf, CHAR_DATA* mob,
                                        CHAR_DATA* ch, OBJ_DATA* obj,
                                        void* vo ) );
void    mprog_bribe_trigger     args ( ( CHAR_DATA* mob, CHAR_DATA* ch,
                                        int amount ) );
void    mprog_entry_trigger     args ( ( CHAR_DATA* mob ) );
void    mprog_exit_trigger      args ( ( CHAR_DATA* mob ));
void    oprog_exit_trigger      args ( ( OBJ_DATA* mob ));
void	oprog_wear_trigger	args ( ( OBJ_DATA* obj ));
void	oprog_remove_trigger    args ( ( OBJ_DATA* obj ));
void    oprog_time_trigger	args ( ( OBJ_DATA* obj ) );
void	oprog_load_trigger	args ( ( OBJ_DATA* obj ) );
void	rprog_time_trigger	args ( ( ROOM_INDEX_DATA *room ) );
void	oprog_death_trigger	args ( ( OBJ_DATA* obj, CHAR_DATA *victim ) );
bool    mprog_give_trigger      args ( ( CHAR_DATA* mob, CHAR_DATA* ch,
                                        OBJ_DATA* obj ) );
void	mprog_hail_trigger	args ( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void    mprog_greet_trigger     args ( ( CHAR_DATA* mob ) );
void	rprog_greet_trigger	args ( ( ROOM_INDEX_DATA *room, CHAR_DATA *ch ) );
void    mprog_fight_trigger     args ( ( CHAR_DATA* mob, CHAR_DATA* ch ));
int	mprog_hit_trigger	args ( ( CHAR_DATA* mob, CHAR_DATA* ch, int dam ));
void    mprog_hitprcnt_trigger  args ( ( CHAR_DATA* mob, CHAR_DATA* ch ));
void    mprog_death_trigger     args ( ( CHAR_DATA* mob, CHAR_DATA *killer ) );
void	mprog_attack_trigger	args ( ( CHAR_DATA* mob, CHAR_DATA *killer, CHAR_DATA *victim ) );
void	rprog_attack_trigger	args ( ( ROOM_INDEX_DATA *room, CHAR_DATA *killer, CHAR_DATA *victim ) );
void	oprog_all_death_trigger args ( ( OBJ_DATA *obj, CHAR_DATA *killer, CHAR_DATA *victim ) );
void	rprog_all_death_trigger args ( ( ROOM_INDEX_DATA *room, CHAR_DATA *killer, CHAR_DATA *victim ));
void    mprog_all_death_trigger args ( ( CHAR_DATA* mob, CHAR_DATA *killer, CHAR_DATA *victim ) );

void	rprog_tick_trigger(ROOM_INDEX_DATA * room);
void	mprog_tick_trigger(CHAR_DATA * mob);
void	oprog_tick_trigger(OBJ_DATA * obj);

void    mprog_random_trigger    args ( ( CHAR_DATA* mob ) );
void	rprog_random_trigger	args ( ( ROOM_INDEX_DATA *room ) );

void    oprog_wordlist_check    args ( ( char * arg, OBJ_DATA *mob,
                			CHAR_DATA* actor, OBJ_DATA* object,
					void* vo, int type ) );
void    oprog_percent_check     args ( ( OBJ_DATA *mob, CHAR_DATA* actor,
					OBJ_DATA* object, void* vo,
					int type ) );
void    oprog_act_trigger       args ( ( char* buf, OBJ_DATA* mob,
		                        CHAR_DATA* ch, OBJ_DATA* obj,
					void* vo ) );
void    oprog_entry_trigger     args ( ( OBJ_DATA* mob ) );
void	mprog_take_trigger	args ( ( CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj ) );
void    oprog_take_trigger	args ( ( OBJ_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj ) );
void    oprog_give_trigger      args ( ( OBJ_DATA* mob, CHAR_DATA* ch,
                		        OBJ_DATA* obj ) );
void    oprog_greet_trigger     args ( ( OBJ_DATA* mob, CHAR_DATA * target ) );
int	oprog_hit_trigger	args ( ( OBJ_DATA* obj, CHAR_DATA* ch, int dam ));
void    oprog_fight_trigger     args ( ( OBJ_DATA* mob, CHAR_DATA* ch ));
void    oprog_hitprcnt_trigger  args ( ( OBJ_DATA* mob, CHAR_DATA* ch ));
void    oprog_random_trigger    args ( ( OBJ_DATA* mob ) );
void    oprog_speech_trigger    args ( ( char* txt, OBJ_DATA* mob ) );
void	oprog_sac_trigger	args ( ( OBJ_DATA *obj, CHAR_DATA *ch ));
void	rprog_speech_trigger	args ( ( char *txt, ROOM_INDEX_DATA *room, CHAR_DATA *ch ));
void	oprog_eat_trigger	args ( ( CHAR_DATA *mob, OBJ_DATA* obj));
void	oprog_drink_trigger	args ( ( CHAR_DATA* mob, OBJ_DATA* obj ) );

/* mount.c */
void	unmount			args ( ( CHAR_DATA *ch ) );

/* note.c */
void	agent_note		args( ( CHAR_DATA *ch, int room, CHAR_DATA *victim ) );
void	inquire_note		args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void 	investigate_note 	args( ( CHAR_DATA *ch, OBJ_DATA *obj, int type ) );

/* save.c */
void	save_char_obj	args( ( CHAR_DATA *ch ) );
void    add_save_room(int vnum);
void	save_room_obj	args( ( ROOM_INDEX_DATA *room ) );
void	clear_room_obj  args( ( ROOM_INDEX_DATA *room ) );
bool	load_char_obj	args( ( char *name, CHAR_DATA *ch ) );
void 	fread_obj_room	args(( ROOM_INDEX_DATA *room, FILE *fp ) );

/* skills.c */
unsigned int total_skill_spheres(int sn);
bool    is_skill_sphere_any(int sn, int sphere);
bool    is_skill_sphere_only(int sn, int sphere);
bool 	parse_gen_groups args(( CHAR_DATA *ch,char *argument ) );
void 	list_group_costs args(( CHAR_DATA *ch ) );
void    list_group_known args(( CHAR_DATA *ch ) );
int 	exp_per_level	 args(( CHAR_DATA *ch, int points ) );
int     exp_on_level    args( ( CHAR_DATA *ch, int level ) );
void 	check_improve(CHAR_DATA *ch, CHAR_DATA *victim, int sn, bool success, int multiplier);
bool    group_contains(CHAR_DATA * ch, int skillNum);
bool    group_contains(int gn, int skillNum);
int 	group_lookup	args( ( const char *name ) );
void	gn_add		args( ( CHAR_DATA *ch, int gn) );
void 	gn_remove	args( ( CHAR_DATA *ch, int gn) );
void 	group_add	args( ( CHAR_DATA *ch, const char *name, 
				bool deduct) );
void	group_remove	args( ( CHAR_DATA *ch, const char *name) );
void	do_wizgroupadd  args( ( CHAR_DATA *ch, char *argument));

/* skills_rogue.c */
bool check_detectstealth(CHAR_DATA *ch, CHAR_DATA *victim);

/* songs.c */
void	add_room_song	args( ( ROOM_INDEX_DATA *pRoom, AFFECT_DATA *song ) );
void	add_room_songs	args( ( ROOM_INDEX_DATA *pRoom, CHAR_DATA *ch ) );
void	remove_room_song args(( ROOM_INDEX_DATA *pRoom, AFFECT_DATA *song ) );
void	remove_room_songs args((ROOM_INDEX_DATA *pRoom, CHAR_DATA *ch ) );
void	stop_playing_song args((CHAR_DATA *ch, AFFECT_DATA *song));
void	stop_playing_sn	args( (CHAR_DATA *ch, int sn ) );

AD *	get_group_song	args( ( CHAR_DATA *ch, int sn ) );	
AD *	get_room_song	args( ( ROOM_INDEX_DATA *pRoom, int sn ) );


/* special.c */
SF *	spec_lookup	args( ( const char *name ) );
char *	spec_name	args( ( SPEC_FUN *function ) );

/* teleport.c */
RID *	room_by_name	args( ( char *target, int level, bool error) );

/* update.c */
enum AggressionCause {Aggression_None, Aggression_UncleanSpirit, Aggression_FactionEnemy, Aggression_Tracking, Aggression_Normal};
AggressionCause should_aggro(CHAR_DATA * ch, CHAR_DATA * victim);
void send_to_area(const char * message, AREA_DATA & area);
void	advance_level	args( ( CHAR_DATA *ch, bool hide ) );
void	gain_exp	args( ( CHAR_DATA *ch, int gain ) );
void	gain_condition	args( ( CHAR_DATA *ch, int iCond, int value ) );
void	update_handler	args( ( void ) );

void    add_event	args( ( void *vo, int timer, EVENT_FUN event ) );

void	do_dark_insight args( ( CHAR_DATA *ch, char *argument) );
void	do_demonic_might args( ( CHAR_DATA *ch, char *argument) );
void    do_demonic_focus args( ( CHAR_DATA *ch, char *argument) );

#undef	CD
#undef	MID
#undef	OD
#undef	OID
#undef	RID
#undef	SF
#undef AD

/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

/*
 * Object defined in limbo.are
 * Used in save.c to load objects that don't exist.
 */
#define OBJ_VNUM_DUMMY	30

/*
 * Area flags.
 */
#define         AREA_NONE       0
#define         AREA_CHANGED	(A)	/* Area has been modified. */
#define         AREA_ADDED      (B)	/* Area has been added to. */
#define         AREA_LOADING    (C)	/* Used for counting in db.c */
#define		AREA_UNCOMPLETE (D)

#define MAX_DIR	6
#define NO_FLAG -99	/* Must not be used in flags or stats. */

/*
 * Global Constants
 */
extern	char *	const	dir_name        [];
extern	const	int	rev_dir         [];          /* int - ROM OLC */
extern	const	struct	spec_type	spec_table	[];
extern  char *  const   lname_stat      [];
extern  char *  const   sname_stat     [];

/*
 * Global variables
 */
extern          AREA_DATA *             area_first;
extern          AREA_DATA *             area_last;
extern  	SHOP_DATA *             shop_last;
extern		ALINK_DATA *		alink_first;

extern		ROOM_INDEX_DATA *	room_rand_first;
extern		OBJ_DATA *		obj_rand_first;

extern          int                     top_affect;
extern          int                     top_area;
extern		int			top_alink;
extern          int                     top_ed;
extern          int                     top_exit;
extern          int                     top_help;
extern          int                     top_mob_index;
extern          int                     top_obj_index;
extern          int                     top_reset;
extern          int                     top_room;
extern          int                     top_shop;

extern          int                     top_vnum_mob;
extern          int                     top_vnum_obj;
extern          int                     top_vnum_room;

extern		int			g_num_rumors;
extern		int			g_num_tracks;
extern		int			g_vnum_blocks;
extern		int			g_gm_count;
extern		int			g_num_progs;
extern		int			g_num_dicts;
extern		int			g_num_accts;
extern		int			g_num_notes;
extern		int			g_num_bans;
extern		int			g_num_headers;
extern		int			g_num_descriptors;
extern		int			object_count;
extern		int			g_num_char_data;
extern		int			g_num_pcdata;
extern		int			g_num_buffer;
extern		int			g_num_poker;
extern		int			g_num_hostdata;
extern		int			g_extra_strings;
extern		int			g_num_coin_array;
extern		int			g_num_helps;
extern		int			g_num_mobmemory;
extern		int			g_num_paths;
extern		int			g_size_bufstrings;
extern		int			g_outbuf_size;
extern		int			g_num_progvars;
extern		int			g_num_loopdata;
extern		int			g_num_bitptr;
extern		int			g_num_travelptr;
extern		int			g_size_pagebuf;


extern          char                    str_empty       [1];

extern  MOB_INDEX_DATA *        mob_index_hash  [MAX_KEY_HASH];
extern  OBJ_INDEX_DATA *        obj_index_hash  [MAX_KEY_HASH];
extern  ROOM_INDEX_DATA *       room_index_hash [MAX_KEY_HASH];

bool	check_spirit_of_freedom args ((CHAR_DATA *ch) );

bool 	can_charm_another	args( (CHAR_DATA *ch) );
/* act_wiz.c */
/*
ROOM_INDEX_DATA *find_location( CHAR_DATA *ch, char *arg );
*/
/* db.c */
void	reset_area      args( ( AREA_DATA * pArea ) );
void	reset_room	args( ( ROOM_INDEX_DATA *pRoom, bool ignorePlayerPresence ) );

/* string.c */
void	string_edit	args( ( CHAR_DATA *ch, char **pString ) );
void    string_append   args( ( CHAR_DATA *ch, char **pString ) );
char *	string_replace	args( ( char * orig, char * old, char * new_str ) );
char * replace_str	args( ( char *str, char *orig, char *rep));
void    string_add      args( ( CHAR_DATA *ch, char *argument ) );
char *  format_string   args( ( char *oldstring /*, bool fSpace */ ) );
char *  first_arg       args( ( char *argument, char *arg_first, bool fCase ) );
char *	string_unpad	args( ( char * argument ) );
char *	string_proper	args( ( char * argument ) );

/* olc.c */
bool	run_olc_editor	args( ( DESCRIPTOR_DATA *d ) );
char	*olc_ed_name	args( ( CHAR_DATA *ch ) );
char	*olc_ed_vnum	args( ( CHAR_DATA *ch ) );

char *confuse_text  args (( char *str ));
char *fix_crap	args(( const char *str ));

bool	focus		args( (	int sn, int level, CHAR_DATA *ch, void *vo, 
				int target) );
void    unfocus		args( ( CHAR_DATA *ch, int slot, bool out_mess) );

/* bit.c */
extern const struct flag_type 	area_flags[];
extern const struct flag_type	sex_flags[];
extern const struct flag_type	exit_flags[];
extern const struct flag_type	door_resets[];
extern const struct flag_type	room_flags[];
extern const struct flag_type	sector_flags[];
extern const struct flag_type	type_flags[];
// extern const struct flag_type	extra_flags[][32];
extern const struct flag_type	wear_flags[];
extern const struct flag_type	act_flags[];
extern const struct flag_type	affect_flags[];
extern const struct flag_type	naffect_flags[];
extern const struct flag_type	oaffect_flags[];
extern const struct flag_type	paffect_flags[];
extern const struct flag_type	apply_flags[];
extern const struct flag_type	wear_loc_strings[];
extern const struct flag_type	wear_loc_flags[];
extern const struct flag_type	container_flags[];
extern const struct flag_type   light_flags[];

struct flag_type
{
    char *name;
    int bit;
    bool settable;
};

/* mem.c  -- most mem.c definitions found in olc.h */
ROOM_INDEX_DATA *new_room_area          args ( ( AREA_DATA *pArea ) );
void            free_room_area          args ( ( ROOM_INDEX_DATA *pRoom ) );
ACCOUNT_DATA 	*new_acct();


/* ROM OLC: */

extern const struct flag_type   form_flags[];
extern const struct flag_type   part_flags[];
extern const struct flag_type   ac_type[];
extern const struct flag_type   size_flags[];
extern const struct flag_type   off_flags[];
extern const struct flag_type   imm_flags[];
extern const struct flag_type   res_flags[];
extern const struct flag_type   vuln_flags[];
extern const struct flag_type   position_flags[];
extern const struct flag_type   weapon_class[];
extern const struct flag_type   weapon_type2[];
extern const struct flag_type   furniture_flags[];

extern const char * proflist[MAX_CLASS ];
extern const char * moon_info[8];
extern const char * confused_words[MAX_CONFUSED_WORDS];

const char *flag_string		args ( ( const struct flag_type *flag_table, int bits ) );

//FIXME: make these preprocessor macros for efficiency? -- Seb
void wrathkyana_combat_effect(CHAR_DATA *ch, CHAR_DATA *victim);

void event_warpath		(void *ch);
void event_berserk		(void *ch);
void event_stampede		(void *ch);
void event_setambush		(void *ch);
void event_plague_madness	(void *ch);
void event_moonray		(void *ch);
void event_demonpos		(void *ch);
void event_ashurmadness		(void *ch);

const char * GetBuildVersion();
const char * GetBuildTime();
const char * GetBuildPath();

#endif
