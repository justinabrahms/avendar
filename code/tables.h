/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
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

/* game tables */
extern	const	struct	clan_type	clan_table[MAX_CLAN];
extern  const	struct  clanskill_type	clanskill_table[MAX_CLAN];
extern	const	struct	position_type	position_table[];
extern  const   struct  arrow_type      arrow_table[];
extern	const	struct	sex_type	sex_table[];
extern	const	struct	size_type	size_table[];
extern  const   struct  resist_type     resist_table[];
extern	const	struct	season_type	season_table[];
extern  const   struct  geo_type	geo_table[];
extern	const	struct	herb_type	herb_table[];
extern	const	struct	month_type	month_table[];
extern	const	struct	home_type	home_table[];
extern	const	struct	channel_type	channel_table[];
extern	const	struct	inscribe_type	inscribe_table[];
extern	const	struct	coin_type	coin_table[MAX_COIN];
extern	const	struct	god_type	god_table[MAX_GODS];
extern	const	struct	trait_group_type trait_groups[];
extern	const	struct	trait_type	trait_table[];

/* flag tables */
extern	const	struct	flag_type	act_flags[];
extern	const	struct	flag_type	plr_flags[];
extern	const	struct	flag_type	nact_flags[];
extern	const	struct	flag_type	nplr_flags[];
extern	const	struct	flag_type	affect_flags[];
extern	const	struct	flag_type	off_flags[];
extern	const	struct	flag_type	imm_flags[];
extern	const	struct	flag_type	lang_flags[];
extern	const	struct	flag_type	form_flags[];
extern	const	struct	flag_type	part_flags[];
extern	const	struct	flag_type	comm_flags[];
extern	const	struct	flag_type	extra_flags[][33];
extern	const	struct	flag_type	wear_flags[];
extern	const	struct	flag_type	weapon_flags[];
extern	const	struct	flag_type	container_flags[];
extern	const	struct	flag_type	portal_flags[];
extern	const	struct	flag_type	room_flags[];
extern	const	struct	flag_type	exit_flags[];
extern  const   struct  flag_type	cdata_flags[];
extern	const	struct	flag_type	spec_obj_types[];
extern	const	struct	flag_type	acct_flags[];
extern	const	struct	flag_type	instrument_table[];

extern	const	struct	flag_type	damtype_table[];

extern	const	struct	flag_type	precip_flags[];
extern	const	struct	flag_type	temp_flags[];
extern	const	struct	flag_type	wind_mag_flags[];
extern	const	struct	flag_type	geo_flags[];

extern	const	struct	flag_type	ainfo_flags[];

/*  Moved to merc.h to fix a bug I don't understand
struct flag_type
{
    char *name;
    int bit;
    bool settable;
};
*/

struct poison_type
{
    char *name;
    int *sn;
    POISON_FUN *spell_fun;
};

struct herb_type
{
    char 	*name;
    int		bit; 
    int 	rarity;
    int	min_level;
    int	duration;
    bool	sector_type[SECT_MAX]; 
    int * 	spell[4];
};

struct channel_type
{
    int		channel;
    int		ctarg;
    bool	use_lang;
    char *	to_self;
    char *	to_self_lang;
    char *	to_rest;
    char *	to_rest_lang;
    char *	to_vict;
    char *	to_vict_lang;
};

struct month_type
{
    char	*name;
    int		num_days;
    bool    warmest_day;
};

struct season_type
{
    char 	*name;
    int	first_day;    	/* Day of year season starts */
    int	temp_mod;     	/* Temperature modifier for season */
    int	sun_up;	
    int	sun_down;
    int	wind_mod;     	/* Wind speed modifier for season */
    int	snow_chance;
    int	storm_mod;    	/* Modifier to storm strength */
    int	lightning_mod;
    int	tornado_mod;
};

struct geo_type
{
    char	*name;
    int		bit;
    int	wind_mod;
    int	precip_mod;
    int	cloud_mod;
    int	storm_mod;
    int	sstr_mod;
    int	storm_move;
};    

struct clan_type
{
    char 	*name;
    char 	*who_name;
    char 	*display_name;
    int 	hall;
    int	altar;
    bool	independent; /* true for loners */
};

struct resist_type
{
    char *name;
};

struct home_type
{
    char *name;
    int   vnum;
    int align;
    int ethos;
};

struct clanskill_type
{
	char	*name;
	char	*skill1;
	char	*skill2;
	char	*skill3;
	char	*skill4;
	char	*skill5;
};

struct position_type
{
    char *name;
    char *short_name;
};

struct arrow_type
{
    char *name;
    int carve_delay;
};

struct sex_type
{
    char *name;
};

struct size_type
{
    char *name;
};

struct inscribe_type
{
    int bit;
    char *name;
};

typedef void IMM_FUNC	args((CHAR_DATA *ch, OBJ_DATA *obj));

extern	IMM_FUNC	immfunc_khanval;
extern	IMM_FUNC	immfunc_jalassa;
extern	IMM_FUNC	immfunc_jolinn;
extern	IMM_FUNC	immfunc_iandir;
extern	IMM_FUNC	immfunc_ashur;
extern	IMM_FUNC	immfunc_serachel;
extern	IMM_FUNC	immfunc_rveyelhi;
extern	IMM_FUNC	immfunc_aeolis;
extern	IMM_FUNC	immfunc_lilune;
extern	IMM_FUNC	immfunc_rystaia;
extern	IMM_FUNC	immfunc_tzajai;
extern	IMM_FUNC	immfunc_calaera;
extern	IMM_FUNC	immfunc_sythrak;
extern	IMM_FUNC	immfunc_alajial;
extern	IMM_FUNC	immfunc_arkhural;
extern	IMM_FUNC	immfunc_bayyal;
extern	IMM_FUNC	immfunc_girikha;
extern	IMM_FUNC	immfunc_dolgrael;
extern	IMM_FUNC	immfunc_chadraln;
extern 	IMM_FUNC	immfunc_elar;
extern	IMM_FUNC	immfunc_alil;
extern	IMM_FUNC	immfunc_vaialos;
extern	IMM_FUNC	immfunc_nariel;
extern	IMM_FUNC	immfunc_sitheus;
extern	IMM_FUNC	immfunc_lielqan;
extern  IMM_FUNC    	immfunc_fenthira;
extern  IMM_FUNC    	immfunc_ayaunj;
extern  IMM_FUNC    	immfunc_arikanja;
extern  IMM_FUNC	immfunc_sitheus;
extern 	IMM_FUNC	immfunc_enirra;

struct god_type
{
    char *	name;
    IMM_FUNC *	func;
    bool	can_follow;
    int	align;
    char *	marking;
    int 	apply;
    char *      godaffname;
};

struct trait_type
{
    char *	name;
    int		bit;
    int	trait_group;
    int	trains;
    int	acct_pts;
    char *	score_desc;
    char *	desc;
};


// REMEMBER: Increase the size of pc_data.traits if needbe.
// Currently allocated: unsigned char[5] (40 bits)

struct trait_group_type
{
    char *	name;
    int	group_num;
};

#define	tgBG		    (CON_TRAIT_GROUP1)
#define tgMagic		    (CON_TRAIT_GROUP2)
#define tgPhys		    (CON_TRAIT_GROUP3)
#define tgGen		    (CON_TRAIT_GROUP4)

struct coin_type
{
    char *name;
    char *abbr;
    float value;
};

#define C_COPPER	3
#define C_SILVER	2
#define C_GOLD		1
#define C_PLATINUM	0
