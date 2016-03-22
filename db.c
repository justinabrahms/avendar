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
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <mysql/mysql.h>
// #include <unistd.h>
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
// #include <sys/resource.h>

#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "lookup.h"
#include "tables.h"
#include "interp.h"
#include "magic.h"
#include "languages.h"
#include "dictionary.h"
#include "RoomPath.h"
#include "Weave.h"
#include "Shades.h"
#include "ShadeControl.h"
#include "weather.h"
#include "olc.h"
#include "fight.h"
#include "StatEmitter.h"
#include "Faction.h"
#include "NameMaps.h"
#include "Oaths.h"

#define HUGEASS_WORDSTRING 1000000

char	write_str[MAX_STRING_LENGTH+1];
extern	CHAR_DATA *	mcur_running;

void log_obj_type(int type);
//void log_altars();

#if !defined(macintosh)
extern	int	_filbuf		args( (FILE *) );
#endif

#if !defined(OLD_RAND)
/* long random();*/
#if !defined(MSDOS)
#if defined(linux)
void srandom(unsigned int);
#else
#ifndef WIN32
void srandom(unsigned long);
#else
#define srandom(r) srand(r)
#endif
#endif
#endif
int getpid();
time_t time(time_t *tloc);
#endif

bool skip_eoo(FILE *fp);
char *fread_store_eol(char *buf, FILE *fp);
void fwrite_eoo(FILE *pfile, FILE *newfile);
bool post_boot = FALSE;
bool MOBtrigger = false;
void purge_old_pfiles();
extern	void	fread_rumors();
/* externals for counting purposes */
extern	OBJ_DATA	*obj_free;
extern	CHAR_DATA	*char_free;
extern  DESCRIPTOR_DATA *descriptor_free;
extern	PC_DATA		*pcdata_free;
extern  AFFECT_DATA	*affect_free;
extern	RUMORS		*rumor_free;

extern	void	load_factions();

/*
 * Globals.
 */
int	global_int_class_waterscholar;
int	global_int_class_earthscholar;
int	global_int_class_voidscholar;
int	global_int_class_spiritscholar;
int	global_int_class_airscholar;
int 	global_int_class_firescholar;
int	global_int_class_watertemplar;
int	global_int_class_earthtemplar;
int	global_int_class_voidtemplar;
int	global_int_class_spirittemplar;
int	global_int_class_airtemplar;
int	global_int_class_firetemplar;
int	global_int_class_thief;
int	global_int_class_watcher;
int 	global_int_class_assassin;
int	global_int_class_bandit;
int	global_int_class_rogue5;
int	global_int_class_rogue6;
int	global_int_class_fighter;
int	global_int_class_swordmaster;
int	global_int_class_barbarian;
int	global_int_class_gladiator;
int	global_int_class_warrior;
int     global_int_class_ranger;
int	global_int_class_gleeman;
int	global_int_class_bard;
int	global_int_class_entertainer;
int	global_int_class_alchemist;
int	global_int_class_psionicist;
int	global_int_class_druid;

int     global_int_race_human;
int     global_int_race_alatharya;
int     global_int_race_srryn;
int	global_int_race_kankoran;
int	global_int_race_chtaren;
int	global_int_race_aelin;
int	global_int_race_shuddeni;
int	global_int_race_nefortu;
int	global_int_race_caladaran;
int	global_int_race_ethron;
int	global_int_race_chaja;
int 	global_int_race_seraph;

std::vector<DamageInfo> global_showdam;
bool			global_bool_final_valid;
bool			global_bool_full_damage;
bool			global_bool_check_avoid; /* parry, shield block, etc */
bool			global_bool_ranged_attack; /* parry, shield block, etc */
char *	        	global_damage_from;
bool			global_bool_sending_brief = FALSE;

bool			global_linked_move = FALSE;

CHAR_DATA *		global_mimic;
OBJ_DATA *		global_obj_target;

bool			global_option_nomysql = FALSE;

int			silver_state;

int			g_num_rumors = 0;
MYSQL			mysql;

/* global variables for equipment crumbling */
bool			crumble_done;
bool			crumble_process;
bool			rip_process = FALSE;
bool			crumble_test;
bool			rip_test;

bool			loading_char;

// HELP_DATA *		help_first;
// HELP_DATA *		help_last;

SHOP_DATA *		shop_first = 0;
SHOP_DATA *		shop_last = 0;
NOTE_DATA *		note_free = 0;

/* some stat names */
char *  const   lname_stat        []              =
{
	    "strength", "intelligence", "wisdom", "dexterity", "constitution", "charisma"
};
char *  const   sname_stat        []              =
{
	    "str", "int", "wis", "dex", "con", "chr"
};


extern int ep_table[MAX_LEVEL];

long			last_mob_id;
char			bug_buf		[2*MAX_INPUT_LENGTH];
RUMORS *		rumor_list;
CHAR_DATA *		char_list;
CHAR_DATA *		ol_char;
// char *			help_greeting;
char			log_buf		[2*MAX_INPUT_LENGTH];
KILL_DATA		kill_table	[MAX_LEVEL];
OBJ_DATA *		object_list;
TIME_INFO_DATA		time_info;
CHAIN_DATA *		event_list;

ADDRESS_LIST *		cleanup_list = NULL;
/* WEATHER_DATA		weather_info;  */

int			gsn_shamanicward;
int			gsn_seedofgamaloth;
int			gsn_naturalarmor;
int			gsn_beastform;
int			gsn_drawblood;
int			gsn_snakebite;
int			gsn_wither;
int			gsn_oaklance;
int			gsn_eyesoftheforest;
int			gsn_moonsight;
int			gsn_shuran_gaze;
int			gsn_arrowcraft;
int			gsn_sidestep;
int			gsn_maul;
int			gsn_aggression;
int			gsn_rend;
int			gsn_fortitude;
int			gsn_ferocity;
int			gsn_lastword;
int			gsn_awaken;
int			gsn_cure_all_serious;
int			gsn_cure_blindness;
int			gsn_purify;
int			gsn_refresh;
int			gsn_remove_curse;
int			gsn_sacrifice;
int			gsn_armor;
int			gsn_immolation;
int			gsn_calm;
int			gsn_detect_invis;
int			gsn_infravision;
int			gsn_shield;
int			gsn_change_sex;
int			gsn_regeneration;
int			gsn_weaken;
int			gsn_flamestrike;
int			gsn_earthquake;
int			gsn_demoncall;
int			gsn_normal_demon_summon;
int			gsn_unseen_servant;
int			gsn_rite_of_nathli;
int			gsn_exorcism;
int			gsn_ray_of_light;
int			gsn_siphon_mana;
int			gsn_phase_door;
int			gsn_blast;
int			gsn_firebolt;
int			gsn_mirror_image;
int			gsn_wind_blast;
int			gsn_magnetic_grasp;
int			gsn_ranged_healing;
int			gsn_icebolt;
int			gsn_life_shield;
int			gsn_cone_of_cold;
int			gsn_icy_shield;
int			gsn_warp_wood;
int			gsn_thornspray;
int			gsn_lightning_breath;
int			gsn_gas_breath;
int			gsn_frost_breath;
int			gsn_fire_breath;
int			gsn_acid_breath;
int			gsn_word_of_recall;
int			gsn_stonefist;
int			gsn_spiritwrack;
int			gsn_soulburn;
int			gsn_scorch;
int          gsn_sandstorm_old;
int			gsn_scare;
int			gsn_sandstorm;
int			gsn_sandspray;
int			gsn_pyrotechnics;
int			gsn_firestorm;
int			gsn_harm;
int			gsn_drain;
int			gsn_colour_spray;
int			gsn_coughing_dust;
int			gsn_chain_lightning;
int			gsn_cause_critical;
int			gsn_cause_light;
int			gsn_cause_serious;
int			gsn_call_lightning;
int			gsn_retainercall;
int			gsn_rush;
int			gsn_opportunism;
int			gsn_defensiveroll;
int			gsn_pilfer;
int			gsn_bargain;
int			gsn_dash;
int			gsn_dissection;
int			gsn_showmanship;
int			gsn_daggertrap;
int			gsn_detectstealth;
int			gsn_pursuit;
int			gsn_forgery;
int			gsn_distract;
int			gsn_swift_staff;
int			gsn_bounty;
int			gsn_exhaustionpoison;
int			gsn_erosivepoison;
int			gsn_concealremains;
int			gsn_snapkick;
int			gsn_acuity;
int			gsn_celerity;
int			gsn_shadowfist;
int			gsn_invocationofconveru;
int			gsn_removehex;
int			gsn_continual_light;
int			gsn_lesserspiritshield;
int			gsn_seraphicwings;
int			gsn_glowinggaze;
int			gsn_froststrike;
int			gsn_healingward;
int			gsn_douse;
int			gsn_coronalglow;
int			gsn_fortify;
int			gsn_giantstrength;
int			gsn_disruption;
int			gsn_petrify;
int			gsn_runeofembers;
int			gsn_blazingspear;
int			gsn_passdoor;
int			gsn_rob;
int			gsn_headbutt;
int			gsn_shieldslam;
int			gsn_jab;
int			gsn_brawlingblock;
int			gsn_trueshot;
int			gsn_ready;
int			gsn_cook;
int			gsn_predatoryattack;
int			gsn_pounce;
int			gsn_huntersense;
int			gsn_fletch;
int			gsn_applypoison;
int			gsn_applybarbs;
int			gsn_forage;
int			gsn_farsight;
int			gsn_enervatingray;
int			gsn_mireofoame;
int			gsn_dim;
int			gsn_shadowstrike;
int			gsn_desecrateweapon;
int			gsn_pestilence;
int			gsn_shielddisarm;
int			gsn_retaliation;
int			gsn_whipmastery;
int			gsn_taunt;
int			gsn_endure;
int			gsn_bravado;
int			gsn_enthusiasm;
int			gsn_mantle_of_earth;
int			gsn_durability;
int			gsn_augmented_strength;
int			gsn_momentum;
int			gsn_charge;
int			gsn_whirl;
int			gsn_reversal;
int			gsn_grace;
int			gsn_medstrike;
int			gsn_vengeance;
int			gsn_riteofkaagn;
int			gsn_teleport;
int			gsn_thunderclap;
int			gsn_fireball;
int			gsn_jawsofthemountain;
int			gsn_sweep;
int			gsn_reach_mastery;
int			gsn_gash;
int			gsn_reaver_bind;
int			gsn_aura_of_corruption;
int			gsn_demonic_focus;
int			gsn_compact_of_Logor;
int			gsn_demonic_might;
int			gsn_dark_insight;
int			gsn_shadow_ward;
int			gsn_strength_of_Aramril;
int 			gsn_windsurge;
int			gsn_backstab;
int			gsn_dodge;
int                  gsn_pugilism;
int			gsn_evade;
int			gsn_envenom;
int			gsn_hide;
int                  gsn_morph;
int			gsn_peek;
int			gsn_pick_lock;
int			gsn_sneak;
int			gsn_steal;
int			gsn_disarm;
int			gsn_enhanced_damage;
int			gsn_kick;
int			gsn_parry;
int			gsn_rescue;
int			gsn_second_attack;
int			gsn_third_attack;
int			gsn_fourth_attack;
int			gsn_cursebarkja;
int			gsn_qwablith;
int			gsn_doppelganger;
int			gsn_bindingtentacle;
int			gsn_desecration;
int			gsn_succubuskiss;
int			gsn_veilsight;
int			gsn_darkfuture;
int			gsn_uncleanspirit;
int			gsn_devouringspirit;
int			gsn_cursekhamurn;
int			gsn_demoniccontrol;
int			gsn_blindness;
int                  gsn_blight;
int			gsn_charm_person;
int			gsn_curse;
int                  gsn_cursekijjasku;
int			gsn_invis;
int			gsn_mass_invis;
int			gsn_icyprison;
int			gsn_fieldmedicine;
int			gsn_poison;
int			gsn_poisonspirit;
int			gsn_tremblingpoison;
int			gsn_delusionpoison;
int			gsn_sleeppoison;
int			gsn_painpoison;
int			gsn_deathpoison;
int			gsn_taint;
int			gsn_cower;
int			gsn_plague;
int			gsn_plague_madness;
int			gsn_sleep;
int			gsn_demon_bind;
int			gsn_caressofpricina;
int			gsn_hungerofgrmlarloth;
int			gsn_blade_of_vershak;
int			gsn_jawsofidcizon;
int			gsn_defilement;
int			gsn_leechrune;
int			gsn_sanctuary;
int			gsn_flood;
int			gsn_cure_poison;
int			gsn_cure_light;
int			gsn_cure_serious;
int			gsn_cure_critical;
int			gsn_slow_cure;
int			gsn_slow_cure_ser;
int			gsn_slow_cure_crit;
int			gsn_slow_heal;
int			gsn_slow_cure_poison;
int			gsn_slow_cure_disease;
int			gsn_protectionfromcold;
int			gsn_resist_poison;
int			gsn_heal;
int			gsn_haste;
int			gsn_cure_disease;
int			gsn_fly;
int          gsn_massflying;

int			player_levels;

int			gsn_stringdata;
int			gsn_skill;

/* new gsns */

int			gsn_mimic;
int			gsn_tumbling;
int			gsn_harmony;
int			gsn_soulflare;
int			gsn_block_vision;
int			gsn_mindshield;
int			gsn_enhance_reflexes;
int			gsn_paranoia;
int			gsn_forget;
int			gsn_enhance_pain;
int			gsn_reduce_pain;
int			gsn_esp;
int  		gsn_slow_reflexes;
int  		gsn_symbiont;
int  		gsn_leech;
int  		gsn_deflection;
int  		gsn_levitation;
int  		gsn_read_thoughts;
int  		gsn_ignore_pain;
int  		gsn_detect_life;
int  		gsn_psychic_block;
int  		gsn_mind_thrust;
int  		gsn_psionic_blast;
int  		gsn_suggestion;
int  		gsn_sensory_vision;
int			gsn_vertigo;
int			gsn_shove;
int			gsn_foresight;
int			gsn_sense_danger;
int			gsn_dominance;
int			gsn_confusion;
int			gsn_mindlink;
int			gsn_backlash;
int                  gsn_sortie;
int                  gsn_flay;
int			gsn_prowess;
int			gsn_adrenaline_rush;
int			gsn_overwhelm;
int			gsn_conceal_thoughts;
int			gsn_ordered_mind;
int			gsn_accelerated_healing;
int			gsn_borrow_knowledge;
int			gsn_ball_lightning;
int			gsn_lightning_bolt;
int			gsn_shocking_grasp;
int  		gsn_blur;
int  		gsn_bladebarrier;
int  		gsn_blink;
int  		gsn_missileattraction;
int  		gsn_screamingarrow;
int  		gsn_arrowgeas;
int  		gsn_disjunction;
int  		gsn_precision;
int  		gsn_mirrorimage;
int  		gsn_lightningbrand;
int  		gsn_absorbelectricity;
int  		gsn_wallofair;
int  		gsn_dive;
int			gsn_rearrange;
int			gsn_thunderstrike;
int			gsn_reflectiveaura;
int			gsn_iceshard;
int			gsn_iceblast;
int  		gsn_quicksand;
int  		gsn_bindweapon;
int  		gsn_shockwave;
int  		gsn_earthmaw;
int  		gsn_protnormalmissiles;
int  		gsn_inertial_strike;
int  		gsn_devotion;
int  		gsn_anchor;
int  		gsn_stonephalanx;
int			gsn_grease;
int			gsn_slow;
int          gsn_trackless_step;
int			gsn_camoublind;
int          gsn_setsnare;
int			gsn_terrainlore;
int			gsn_aquamove;
int			gsn_encamp;
int			gsn_riteofthesun;
int			gsn_impale;
int			gsn_obscurealign;
int			gsn_pugil;
int			gsn_sixthsense;
int			gsn_windbind;
int			gsn_livingflame;
int			gsn_hew;
int			gsn_runicmessage;
int			gsn_submissionhold;
int			gsn_agility;
int			gsn_pursue;
int			gsn_bolo;
int			gsn_findcover;
int			gsn_flashpowder;
int			gsn_detecttheft;
int			gsn_tripwire;
int			gsn_deflect;
int			gsn_twirl;
int			gsn_inquire;
int			gsn_record;
int			gsn_sentry;
int			gsn_wariness;
int			gsn_identifyowner;
int			gsn_covertracks;
int			gsn_lightsleep;
int			gsn_vitalstrike;
int			gsn_shadowmastery;
int			gsn_plant;
int			gsn_plantentangle;
int			gsn_giantgrowth;
int			gsn_forestwalk;
int			gsn_shrink;
int			gsn_commune_nature;
int			gsn_wallofvines;
int			gsn_plantgrowth;
int			gsn_blindfighting;
int			gsn_dart;
int			gsn_prepare;
int			gsn_circleofstones;
int			gsn_wallofwater;
int			gsn_cutoff;
int			gsn_craftdart;
int			gsn_nerve;
int			gsn_disguise;
int			gsn_muffle;
int			gsn_caltraps;
int			gsn_garrote;
int			gsn_surpriseattack;
int			gsn_faerie_fog;
int			gsn_etherealblaze;
int			gsn_starglamour;
int			gsn_tangletrail;
int			gsn_toggleoffaff;
int			gsn_toggleonaff;
int			gsn_relock;
int			gsn_sleeve;
int			gsn_thievescant;
int			gsn_stab;
int			gsn_slice;
int			gsn_barkskin;
int			gsn_elementalprotection;
int                  gsn_archery;
int			gsn_arcshot;
int			gsn_commandweather;
int			gsn_cutpurse;
int			gsn_curseofthevoid;
int			gsn_wrathofthevoid;
int			gsn_finalstrike;
int			gsn_circle;
int			gsn_favoredblade;
int			gsn_offhanddisarm;
int			gsn_offhandparry;
int			gsn_barbs;
int			gsn_waspstrike;
int			gsn_viperstrike;
int                  gsn_aspstrike;
int			gsn_winterwind;
int			gsn_versatility;
int			gsn_ram;
int			gsn_fury;
int			gsn_rage;
int			gsn_warcry;
int			gsn_bloodlust;
int			gsn_fetish;
int			gsn_boneshatter;
int			gsn_roomdeathwalk;
int			gsn_bloodsigil;
int			gsn_findherbs;
int			gsn_mushroomcircle;
int			gsn_animaltongues;
int			gsn_insectswarm;
int			gsn_naturesbounty;
int			gsn_counterattack;
int			gsn_lunge;
int			gsn_scouting;
int			gsn_forcedmarch;
int			gsn_counter;
int			gsn_rally;
int			gsn_cover;
int			gsn_phalanx;
int			gsn_retreat;
int			gsn_nothing;
int			gsn_bandage;
int			gsn_readiness;
int			gsn_drive;
int			gsn_guard;
int			gsn_strip;
int			gsn_corner;
int			gsn_entangle;
int			gsn_grip;
int			gsn_cross;
int			gsn_feint;
int			gsn_lash;
int			gsn_dispersion;
int			gsn_choke;
int			gsn_cleave;
int			gsn_batter;
int			gsn_wildcraft;
int			gsn_divineshield;
int			gsn_spirit_of_freedom;
int			gsn_bless;
int			gsn_createfoci;
int                  gsn_astralform;
int			gsn_astralprojection;
int			gsn_affinity;
int			gsn_masspeace;
int			gsn_focusmind;
int			gsn_avatar;
int			gsn_spirit_sanctuary;
int			gsn_spiritblock;
int			gsn_spiritbond;
int			gsn_aura;
int			gsn_spiritstone;
int                  gsn_truesight;
int                  gsn_lightsword;
int			gsn_lightspear;
int			gsn_manaconduit;
int                  gsn_consecrate;
int			gsn_clarity;
int                  gsn_lucid;
int			gsn_lifebolt;
int			gsn_dreamspeak;
int			gsn_visions;
int                  gsn_sanctify;
int                  gsn_poschan;
int                  gsn_radiance;
int			gsn_runeofspirit;
int			gsn_resonance;
int			gsn_unshackle;
int			gsn_zeal;
int			gsn_spiritshield;
int			gsn_improveddetectevil;
int			gsn_thanatopsis;
int                  gsn_wrathkyana;
int			gsn_avengingseraph;
int			gsn_speakwiththedead;
int			gsn_mindshell;
int			gsn_implosion;
int			gsn_heat_metal;
int			gsn_parch;
int			gsn_delayedblastfireball;
int			gsn_beamoffire;
int			gsn_wingsofflame;
int			gsn_infernofury;
int			gsn_burnout;
int			gsn_consuming_rage;
int			gsn_frenzy;
int                  gsn_kneeshatter;
int			gsn_smolder;
int			gsn_phoenixfire;
int			gsn_flametongue;
int			gsn_blazinginferno;
int			gsn_blaze;
int			gsn_fireblast;
int			gsn_shiyulsfury;
int			gsn_consume;
int			gsn_ignite;
int			gsn_enflamed;
int			gsn_rainoffire;
int			gsn_incineration;
int			gsn_flare;
int			gsn_baptismoffire;
int			gsn_walloffire;
int			gsn_runeoffire;
int			gsn_wardoffire;
int			gsn_aggravatewounds;
int			gsn_ringoffire;
int			gsn_nova;
int			gsn_flameshield;
int			gsn_lambentaura;
int			gsn_weariness;
int			gsn_detect_magic;
int			gsn_crystalizemagic;
int			gsn_slip;
int			gsn_runeofearth;
int			gsn_meteorstrike;
int			gsn_brittleform;
int			gsn_strengthen;
int			gsn_metaltostone;
int			gsn_meldwithstone;
int			gsn_shapearmor;
int			gsn_shapeweapon;
int			gsn_gravitywell;
int			gsn_density;
int			gsn_smoothterrain;
int			gsn_voiceoftheearth;
int			gsn_stonetomud;
int			gsn_calluponearth;
int			gsn_dispelillusions;
int			gsn_earthelementalsummon;
int			gsn_stabilize;
int			gsn_knock;
int			gsn_encase;
int			gsn_diamondskin;
int			gsn_fleshtostone;
int			gsn_stoneshell;
int			gsn_wallofstone;
int			gsn_lightning_storm;
int			gsn_phantasmalcreation;
int			gsn_diversions;
int			gsn_dancingsword;
int			gsn_protectionfromlightning;
int			gsn_illusionaryobject;
int			gsn_conjureairefreet;
int			gsn_airrune;
int			gsn_gaseousform;
int			gsn_tempest;
int			gsn_groupteleport;
int			gsn_windwall;
int			gsn_airbubble;
int			gsn_spectralfist;
int			gsn_obfuscation;
int			gsn_greaterventriloquate;
int			gsn_delusions;
int			gsn_alterself;
int			gsn_scatter;
int			gsn_windwalk;
int			gsn_greaterinvis;
int			gsn_gust;
int			gsn_suction;
int			gsn_cloudkill;
int			gsn_meldwithwater;
int			gsn_heatsink;
int			gsn_frostbite;
int			gsn_icestorm;
int			gsn_wildmove;
int			gsn_waterwalk;
int			gsn_holywater;
int			gsn_scry;
int			gsn_resurrection;
int			gsn_massheal;
int			gsn_revitalize;
int                  gsn_hooded;
int			gsn_whirlpool;
int			gsn_protectionfrompoison;
int			gsn_currents;
int			gsn_cleansefood;
int                  gsn_stoneskin;
int			gsn_protectionfromfire;
int			gsn_waterelementalsummon;
int			gsn_mendwounds;
int			gsn_creaturelore;
int			gsn_runeoflife;
int			gsn_freeze;
int			gsn_frostbrand;
int			gsn_firebrand;
int			gsn_beltoflooters;
int			gsn_bind;
int			gsn_escape;
int			gsn_birdofprey;
int			gsn_pillage;
int			gsn_dedication;
int			gsn_intimidate;
int			gsn_warpath;
int			gsn_matrix;
int			gsn_division;
int			gsn_banish;
int			gsn_inscribe;
int			gsn_greaterdemonsummon;
int			gsn_lesserdemonsummon;
int			gsn_findfamiliar;
int			gsn_suppress;
int			gsn_commandword;
int			gsn_cloakofthevoid;
int			gsn_tentacles;
int			gsn_agony;
int			gsn_deathwalk;
int                  gsn_demonpos;
int                  gsn_demonichowl;
int			gsn_gate;
int                  gsn_globedarkness;
int			gsn_possession;
int			gsn_voidwalk;
int			gsn_stickstosnakes;
int			gsn_request;
int			gsn_holyavenger;
int			gsn_reveal;
int			gsn_brotherhood;
int			gsn_scourgeofdarkness;
int			gsn_soulblade;
int			gsn_mantleoffear;
int			gsn_enslave;
int			gsn_coverofdarkness;
int			gsn_runeofeyes;
int			gsn_soulreaver;
int			gsn_coven;
int			gsn_consumption;
int			gsn_demonsummon;
int			gsn_vaultaffect;
int			gsn_stoneofpower;
int			gsn_perception;
int			gsn_aegisoflaw;
int			gsn_callguards;
int			gsn_locatecriminal;
int			gsn_herbalmedicine;
int			gsn_hawkform;
int			gsn_bearform;
int			gsn_wolfform;
int			gsn_skycall;
int			gsn_naturegate;
int			gsn_creepingcurse;
int			gsn_speakwithplants;
int			gsn_animaleyes;
int			gsn_stampede;
int			gsn_calmanimals;
int			gsn_animalswarm;
int			gsn_moonray;
int			gsn_trance;
int			gsn_chameleon;
int			gsn_lunarinfluence;
int			gsn_befriend;
int			gsn_forestsense;
int			gsn_hunt;
int			gsn_callanimal;
int			gsn_tame;
int			gsn_findwater;
int			gsn_endurance;
int			gsn_tan;
int			gsn_swim;
int			gsn_poultice;
int			gsn_moonbornspeed;
int			gsn_vanish;
int			gsn_blindingcloud;
int			gsn_blindingdust;
int			gsn_cloak;
int			gsn_investigate;
int			gsn_obscure_evidence;
int			gsn_stash;
int			gsn_camouflage;
int			gsn_sharp_vision;
int			gsn_plunder;
int                  gsn_setambush;
int			gsn_ambush;
int			gsn_loot;
int			gsn_kidneyshot;
int			gsn_dualbackstab;
int			gsn_conceal;
int			gsn_trap;
int			gsn_track;
int			gsn_swordrepair;
int			gsn_pommel;
int			gsn_rout;
int			gsn_pummel;
int			gsn_uppercut;
int                  gsn_drag;
int			gsn_contact_agents;
int			gsn_ghost;
int			gsn_pox;
int			gsn_fever;
int			gsn_powerwordfear;
int			gsn_nightfears;
int			gsn_nightterrors;
int			gsn_abominablerune;
int			gsn_smoke;
int			gsn_earthbind;
int			gsn_shatter;
int			gsn_sustenance;
int			gsn_manabarbs;
int			gsn_rebirth;
int			gsn_paradise;
int			gsn_subdue;
int			gsn_disintegration;
int			gsn_coughingdust;
int			gsn_clumsiness;
int			gsn_brainwash;
int                  gsn_embraceisetaton;
int			gsn_enfeeblement;
int			gsn_protective_shield;
int			gsn_tail;
int			gsn_bite;
int			gsn_bludgeon;
int			gsn_legsweep;
int			gsn_withdraw;
int			gsn_savagery;
int			gsn_hamstring;
int			gsn_grapple;
int			gsn_decapitate;
int			gsn_nightvision;
int			gsn_detecthidden;
int			gsn_ensnare;
int			gsn_gouge;
int			gsn_brutal_damage;
int			gsn_flank;
int			gsn_throw;
int			gsn_catch_throw;
int			gsn_dual_wield;
int			gsn_fend;
int			gsn_listen;
int			gsn_gag;
int                  gsn_vortex;
int			gsn_waylay;
int			gsn_waterbreathing;
int			gsn_shieldcover;

/* ALCHEMISTS */
int			gsn_mix;
int			gsn_brew;
int			gsn_make;
int			gsn_distill;
int			gsn_pulverize;
int			gsn_reforge;
int			gsn_boil;
int			gsn_dissolve;
int			gsn_dilute;
int			gsn_delayreaction;
int			gsn_caution;
int			gsn_animategolem;
int			gsn_dye;
int			gsn_discernmagic;
int			gsn_finditems;
int			gsn_harrudimtalc;
int			gsn_smokescreen;
int			gsn_rynathsbrew;
int			gsn_concentrate;
int			gsn_alchemicstone;
int			gsn_subtlepoison;
int			gsn_transmogrification;
int		      	gsn_lesserhandwarp;
int      		gsn_handwarp;
int      		gsn_greaterhandwarp;
int      		gsn_lessertentaclegrowth;
int      		gsn_greatertentaclegrowth;
int      		gsn_lessercarapace;
int      		gsn_greatercarapace;
int      		gsn_lycanthropy;
int      		gsn_thirdeye;
int      		gsn_cateyes;
int      		gsn_eagleeyes;
int			gsn_gills;
int			gsn_drunkenness;
int			gsn_lovepotion;
int			gsn_susceptibility;
int			gsn_age;
int			gsn_polyglot;
int			gsn_resistance;
int			gsn_heroism;
int			gsn_divinesight;
int			gsn_youth;
int			gsn_invulnerability;
int			gsn_perfection;
int			gsn_improvement;
int			gsn_applyoil;
int			gsn_createwand;
int			gsn_teleportcurse;
int			gsn_aviancurse;
int			gsn_prismaticray;
int			gsn_prismaticspray;
int			gsn_maze;
int			gsn_createhomonculus;
int			gsn_transmutation;
int                  gsn_diagnose;

int			gsn_tendrilgrowth;
int			gsn_cauterize;
int			gsn_whipstitch;
int			gsn_anesthetize;
int			gsn_infectiousaura;
int			gsn_nettles;
int			gsn_deathempowerment;
int			gsn_shurangaze;
int			gsn_animate_dead;


/* HOUSES */
int			gsn_criminal;

/* LANGUAGES */
int			gsn_language_common;
int			gsn_language_aelin;
int			gsn_language_alatharya;
int			gsn_language_nefortu;
int			gsn_language_srryn;
int			gsn_language_shuddeni;
int			gsn_language_ethron;
int			gsn_language_caladaran;
int			gsn_language_chtaren;
int			gsn_language_kankoran;
int			gsn_language_arcane;

/* SONGS */
int			gsn_noteofshattering;
int			gsn_eleventhhour;
int			gsn_marchofwar;
int			gsn_sonicwave;
int			gsn_soundbubble;
int			gsn_wallsofjericho;
int			gsn_cloakofshadows;
int			gsn_auraoflight;
int			gsn_psalmofhealing;
int			gsn_serenadeoflife;
int			gsn_baneofasjia;
int			gsn_invokesympathy;
int			gsn_marchingtune;
int			gsn_aegisofmusic;
int			gsn_noteofstriking;
int			gsn_echoesoffear;
int			gsn_discord;
int			gsn_songofsoothing;
int			gsn_manasong;
int			gsn_stuttering;
int			gsn_hymntotourach;
int			gsn_generic;
int			gsn_heirloom;

/* FORMS */
int			gsn_form_of_the_bull;
int			gsn_form_of_the_whirlwind;
int			gsn_form_of_the_bear;
int			gsn_form_of_the_dragon;
int			gsn_form_of_the_boar;
int			gsn_form_of_the_mongoose;
int			gsn_form_of_the_eagle;
int			gsn_form_of_the_crab;
int			gsn_form_of_the_cat;
int			gsn_form_of_the_serpent;
int			gsn_form_of_the_spider;
int			gsn_form_of_the_panther;
int			gsn_form_of_the_mockingbird;
int			gsn_form_of_the_reed;
int			gsn_form_of_the_rose;
int			gsn_form_of_the_hawk;
int			gsn_form_of_the_monkey;
int			gsn_form_of_the_viper;
int			gsn_form_of_the_wasp;
int			gsn_form_of_the_living_seas;
int			gsn_form_of_the_winter_wind;
int			gsn_form_of_the_cyclone;
int			gsn_form_of_the_phantasm;
int			gsn_form_of_the_zephyr;
int                  gsn_form_of_the_wraith;
int                  gsn_form_of_the_asp;

/* SONGS! */
int			gsn_painful_thoughts;
int			gsn_courage;
int			gsn_dispel_illusion;
int			gsn_drowning;
int  		gsn_axe;
int  		gsn_dagger;
int  		gsn_flail;
int  		gsn_mace;
int  		gsn_polearm;
int			gsn_shield_block;
int  		gsn_spear;
int  		gsn_sword;
int  		gsn_whip;
int			gsn_staff;
int			gsn_knife; 
int			gsn_shieldbash;
int  		gsn_bash;
int  		gsn_berserk;
int  		gsn_dirt;
int  		gsn_hand_to_hand;
int  		gsn_trip;
int  		gsn_fast_healing;
int  		gsn_haggle;
int  		gsn_lore;
int  		gsn_meditation;
int			gsn_appraise;
int  		gsn_scrolls;
int  		gsn_staves;
int  		gsn_wands;
int  		gsn_recall;
int			gsn_eyes_hunter;
int			gsn_communion;

int          gsn_warmth;
int          gsn_firekin;
int          gsn_burningmind;
int          gsn_flameheart;
int          gsn_firedancer;
int          gsn_flamesight;
int          gsn_temper;
int          gsn_sealofthegoldenflames;
int          gsn_bloodpyre;
int          gsn_heartoftheinferno;
int          gsn_heartfire;
int          gsn_heatmine;
int          gsn_wrathofanakarta;
int          gsn_flameunity;
int          gsn_conflagration;
int          gsn_aspectoftheinferno;
int          gsn_phoenixdirge;
int          gsn_pyrotechnicartistry;

int  gsn_geothermics;
int  gsn_moltenshield;
int  gsn_summonlavaelemental;
int  gsn_harrudimfire;

int  gsn_chillfireshield;
int  gsn_nightflamelash;
int  gsn_ashesoflogor;

int  gsn_burningwisp;
int  gsn_heatwave;
int  gsn_pyrokineticmirror;

int  gsn_thermalmastery;
int  gsn_sauna;
int  gsn_flamesofthemartyr;
int  gsn_martyrsfire;
int  gsn_boilblood;

int  gsn_soulfireshield;
int  gsn_oriflamme;
int  gsn_holyflame;
int  gsn_soulbrand;
int  gsn_manaburn;

int  gsn_weavesense;
int  gsn_shroudsight;
int  gsn_aetherealcommunion;
int  gsn_unfettermana;
int  gsn_bondofsouls;
int  gsn_aegisofgrace;
int  gsn_wardofgrace;
int  gsn_riteofablution;
int  gsn_undyingradiance;
int  gsn_discerniniquity;
int  gsn_searinglight;
int  gsn_phaseshift;
int  gsn_leapoffaith;
int  gsn_unweave;
int  gsn_crystallizeaether;
int  gsn_weavecraft;
int  gsn_pacify;
int  gsn_absolvespirit;
int  gsn_dreamshape;
int  gsn_weavetap;
int  gsn_decorporealize;
int  gsn_countermagic;
int  gsn_reclaimessence;
int  gsn_lethebane;
int  gsn_chantlitany;
int  gsn_triumphantshout;
int  gsn_celestialtactician;
int  gsn_cohortsvengeance;
int  gsn_radiateaura;
int  gsn_nourishspirit;
int  gsn_diakinesis;
int  gsn_spectrallantern;
int  gsn_shadeswarm;
int  gsn_fugue;
int  gsn_etherealbrethren;
int  gsn_attunefount;
int  gsn_singularity;
int  gsn_leypulse;
int  gsn_quintessencerush;
int  gsn_manifestweave;
int  gsn_sunderweave;
int  gsn_etherealsplendor;
int  gsn_rebukeofinvesi;
int  gsn_wardoftheshieldbearer;
int  gsn_callhost;
int  gsn_bindessence;
int  gsn_workessence;
int  gsn_forgepostern;
int  gsn_sealpostern;
int  gsn_roaroftheexalted;
int  gsn_canticleofthelightbringer;
int  gsn_requiemofthemartyr;
int  gsn_avatarofthelodestar;
int  gsn_avataroftheprotector;
int  gsn_avataroftheannointed;

int  gsn_balmofthespirit;
int  gsn_martyrsshield;
int  gsn_distillmagic;
int  gsn_annointedone;

int  gsn_holyground;
int  gsn_crystalsoul;
int  gsn_stoneloupe;
int  gsn_earthenvessel;

int  gsn_dreammastery;
int  gsn_drowse;
int  gsn_dreamstalk;
int  gsn_parasiticbond;

int  gsn_bilocation;
int  gsn_aurora;
int  gsn_diffraction;

int  gsn_detoxify;
int  gsn_restorevigor;
int  gsn_frostkin;
int  gsn_treatinfection;
int  gsn_glyphofulyon;
int  gsn_solaceoftheseas;
int  gsn_healerstouch;
int  gsn_somaticarts;
int  gsn_deluge;
int  gsn_waveborne;
int  gsn_rimeshard;
int  gsn_wintertide;
int  gsn_maleficinsight;
int  gsn_boonoftheleech;
int  gsn_clarifymind;
int  gsn_stormmastery;
int  gsn_refinepotion;
int  gsn_wellspring;
int  gsn_frostblast;
int  gsn_markofthekaceajka;
int  gsn_draughtoftheseas;
int  gsn_physikersinstinct;
int  gsn_oceanswell;
int  gsn_maelstrom;
int  gsn_drown;
int  gsn_arcticchill;
int  gsn_breathofelanthemir;
int  gsn_glaciersedge;
int  gsn_wintersstronghold;
int  gsn_wardoffrost;
int  gsn_hoarfrost;
int  gsn_ordainsanctum;
int  gsn_corrosion;
int  gsn_contaminate;
int  gsn_darkchillburst;
int  gsn_stoneofsalyra;
int  gsn_fogelementalsummon;
int  gsn_monsoon;
int  gsn_brume;
int  gsn_steam;
int  gsn_boilseas;

int  gsn_subvocalize;
int  gsn_controlledflight;
int  gsn_disillusionment;
int  gsn_overcharge;
int  gsn_clingingfog;
int  gsn_gralcianfunnel;
int  gsn_illusion;
int  gsn_borrowluck;
int  gsn_shockcraft;
int  gsn_conjureairefreeti;
int  gsn_mistralward;
int  gsn_fatesdoor;
int  gsn_mirage;
int  gsn_sparkingcloud;
int  gsn_endlessfacade;
int  gsn_runeofair;
int  gsn_unrealincursion;
int  gsn_figmentscage;
int  gsn_phantasmalmirror;
int  gsn_empowerphantasm;
int  gsn_arcshield;
int  gsn_conduitoftheskies;
int  gsn_sonicboom;
int  gsn_electricalstorm;
int  gsn_ionize;
int  gsn_skystrike;
int  gsn_joltward;
int  gsn_displacement;
int  gsn_windrider;
int  gsn_typhoon;
int  gsn_unleashtwisters;
int  gsn_breezestep;
int  gsn_curseofeverchange;
int  gsn_beckonwindfall;
int  gsn_calluponwind;
int  gsn_mistsofarcing;
int  gsn_breathoflife;
int  gsn_mantleofrain;
int  gsn_channelwind;
int  gsn_chargestone;
int  gsn_hoveringshield;
int  gsn_miasmaofwaning;
int  gsn_drainbolt;
int  gsn_soulofthewind;
int  gsn_mirrorofsouls;
int  gsn_bewilderment;
int  gsn_chaoscast;
int  gsn_incendiaryspark;
int  gsn_feigndemise;
int  gsn_englamour;
int  gsn_floatingdisc;

int  gsn_adamantineinvocation;
int  gsn_clayshield;
int  gsn_mudfootcurse;
int  gsn_reforgemagic;
int  gsn_tuningstone;
int  gsn_reinforce;
int  gsn_honeweapon;
int  gsn_stonecraft;
int  gsn_shellofstone;
int  gsn_quake;
int  gsn_rocktomud;
int  gsn_shapematter;
int  gsn_crush;
int  gsn_stoneshape;
int  gsn_runecraft;
int  gsn_quarry;
int  gsn_gravenmind;
int  gsn_markofloam;
int  gsn_glyphofentombment;
int  gsn_geomancy;
int  gsn_latticeofstone;
int  gsn_saltoftheearth;
int  gsn_wakenedstone;
int  gsn_bedrockroots;
int  gsn_cryofthebrokenlands;
int  gsn_shakestride;
int  gsn_stonehaven;
int  gsn_conduitofstonesong;
int  gsn_clayshard;
int  gsn_constructwaterwheel;
int  gsn_causticblast;
int  gsn_clockworksoul;
int  gsn_clockworkgolem;
int  gsn_kaagnsplaguestone;
int  gsn_heartofstone;
int  gsn_durablemagics;
int  gsn_abodeofthespirit;
int  gsn_mindofsteel;
int  gsn_pillarofsparks;
int  gsn_dispatchlodestone;
int  gsn_scorchedearth;
int  gsn_forgemaster;
int  gsn_forgeweapon;
int  gsn_lavaforge;
int  gsn_tremor;
int  gsn_calcify;
int  gsn_erosion;
int  gsn_empowerment;

int gsn_gloomward;
int gsn_fellpurpose;
int gsn_corpsesense;
int gsn_revenant;
int gsn_fadeshroud;
int gsn_scionofnight;
int gsn_kissoftheblackenedtears;
int gsn_touchofthedesecrator;
int gsn_cowlofnight;
int gsn_hatefire;
int gsn_gravebeat;
int gsn_darktallow;
int gsn_wreathoffear;
int gsn_scritofkaagn;
int gsn_witherpox;
int gsn_callbat;
int gsn_callraven;
int gsn_callcat;
int gsn_callfox;
int gsn_calltoad;
int gsn_callserpent;
int gsn_harvestofsouls;
int gsn_unholymight;
int gsn_deathlyvisage;
int gsn_blackamulet;
int gsn_barrowmist;
int gsn_imbuephylactery;
int gsn_reaping;
int gsn_deathswarm;
int gsn_direfeast;
int gsn_duskfall;
int gsn_eyeblighttouch;
int gsn_dreadwave;
int gsn_shadowfiend;
int gsn_nightstalk;
int gsn_webofoame;
int gsn_gaveloflogor;
int gsn_fetiddivination;
int gsn_grimseep;
int gsn_bloodofthevizier;
int gsn_seedofmadness;
int gsn_bierofunmaking;
int gsn_stasisrift;
int gsn_scriptmastery;
int gsn_theembraceofthedeeps;
int gsn_theilalslastsailing;
int gsn_hemoplague;
int gsn_cryptkin;
int gsn_baneblade;
int gsn_gravemaw;
int gsn_reshackle;
int gsn_devouressence;
int gsn_fatebones;
int gsn_feverwinds;
int gsn_masquerade;
int gsn_painchannel;
int gsn_barbsofalthajji;
int gsn_focusfury;

/*
 * Locals.
 */
MOB_INDEX_DATA *	mob_index_hash		[MAX_KEY_HASH];
OBJ_INDEX_DATA *	obj_index_hash		[MAX_KEY_HASH];
ROOM_INDEX_DATA *	room_index_hash		[MAX_KEY_HASH];
char *			string_hash		[MAX_KEY_HASH];

AREA_DATA *		area_first;
AREA_DATA *		area_last;
ALINK_DATA *		alink_first;

ROOM_INDEX_DATA *	room_rand_first = NULL;
OBJ_DATA *		obj_rand_first = NULL;

char *			string_space;
char *			top_string;
char			str_empty	[1];

/* Global Variables for tracking memory usage */

int			top_affect;
int			top_area;
int			top_alink;
int			top_ed;
int			top_exit;
int			top_help;
int			top_mob_index;
int			top_obj_index;
int			top_reset;
int			top_room;
int			top_shop;
int                     top_vnum_room;  /* OLC */
int                     top_vnum_mob;   /* OLC */
int                     top_vnum_obj;   /* OLC */
int 			mobile_count = 0;
int			newmobs = 0;
int			newobjs = 0;

int			g_num_tracks	= 0;
int			g_vnum_blocks	= 0;
int			g_gm_count	= 0;
int			g_num_progs	= 0;
int			g_num_dicts	= 0;
int			g_num_accts	= 0;
int			g_num_notes	= 0;
int			g_num_bans	= 0;
int			g_num_headers	= 0;
int			g_num_descriptors = 0;
int			object_count	= 0;
int			g_num_char_data	= 0;
int			g_num_pcdata	= 0;
int			g_num_buffer	= 0;
int			g_num_poker	= 0;
int			g_num_hostdata	= 0;
int			g_extra_strings = 0;
int			g_num_coin_array = 0;
int			g_num_helps	= 0;
int			g_num_mobmemory = 0;
int			g_num_paths	= 0;
int			g_size_bufstrings = 0;
int			g_outbuf_size	= 0;
int			g_num_progvars	= 0;
int			g_num_loopdata	= 0;
int			g_num_bitptr	= 0;
int			g_num_travelptr	= 0;
int			g_size_pagebuf	= 0;


/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */
#define			MAX_STRING	33554432
#define			MAX_PERM_BLOCK	131072
#define			MAX_MEM_LIST	11

void *			rgFreeList	[MAX_MEM_LIST];
const int		rgSizeList	[MAX_MEM_LIST]	=
{
    16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768-64
};

int			nAllocString;
int			sAllocString;
int			nAllocPerm;
int			sAllocPerm;



/*
 * Semi-locals.
 */
bool			fBootDb;
FILE *			fpArea;
char			strArea[MAX_INPUT_LENGTH];



/*
 * Local booting procedures.
*/
void    load_gms	args(( FILE *fp ));
void    init_mm         args( ( void ) );
void	load_area	args( ( FILE *fp ) );
void    new_load_area   args( ( FILE *fp ) );   /* OLC */
// void	load_helps	args( ( FILE *fp ) );
void	load_old_mob	args( ( FILE *fp ) );
void 	load_mobiles	args( ( FILE *fp ) );
void	load_old_obj	args( ( FILE *fp ) );
void 	load_objects	args( ( FILE *fp ) );
void	load_resets	args( ( FILE *fp ) );
void	load_rooms	args( ( FILE *fp ) );
void	load_shops	args( ( FILE *fp ) );
void 	load_socials	args( ( FILE *fp ) );
void	load_specials	args( ( FILE *fp ) );
void	load_notes	args( ( void ) );
void	load_bans	args( ( void ) );
void	load_words	args( ( void ) );

void	fix_exits	args( ( void ) );
void    make_alinks     args( ( void ) );
void    assign_herbs	args( ( void ) );

void	reset_area	args( ( AREA_DATA * pArea ) );

void	pfile_maint	args( ( bool loading ) );

void	mob_obj_refresh	args( ( ROOM_INDEX_DATA *pRoom, CHAR_DATA *target ) );
void    get_player_levels	args( ( void) );
void	calc_limit_values	args( (void) );
int	limit_calc	args( (int limit_factor) );
void	rip_obj_pfiles	args( ( void) );
void    load_obj_rooms  args( ( void ) );
void	read_limits 	args( ( void ) );
void	load_char_names	args( ( void ) );

void	init_weather	args( ( void ) );
void	weather_update	args( ( void ) );

bool crumble_check(OBJ_DATA *obj, CHAR_DATA *ol_char);
char *dictwords;

DICTIONARY_DATA *g_char_names = NULL;
HEADER_DATA	*g_active_headers = NULL;
HEADER_DATA	*g_denied_headers = NULL;
DICTIONARY_DATA	*g_denied_names = NULL;

char *fread_name(FILE *fp, char *buffer, const unsigned long max_length);


void load_char_names()
{
    FILE *fp;
    HEADER_DATA *hdp;
    char buf[MAX_STRING_LENGTH];

    g_char_names = new_dict();

    sprintf(buf, "%s%s", PLAYER_DIR, PLAYERLIST);
    if ((fp = fopen(buf, "r")) == NULL)
    {
	log_string("Could not open playerlist.");
	return;
    }

    while (!feof(fp))
    {
	hdp = new_header();

	hdp->name = str_dup(fread_name(fp, buf, MAX_STRING_LENGTH));

	dict_insert(g_char_names, hdp->name, hdp);

	if (!g_active_headers)
	    g_active_headers = hdp;
	else
	{
	    hdp->next = g_active_headers;
	    g_active_headers = hdp;
	}
    }

    fclose(fp);

    log_string("Loaded character names into dictionary.");
    return;
}

void load_denied_names()
{
    FILE *fp;
    HEADER_DATA *hdp;
    char buf[MAX_STRING_LENGTH];

    g_denied_names = new_dict();

    sprintf(buf, "%s%s", PLAYER_DIR, DENIEDNAME_FILE);
    if ((fp = fopen(buf, "r")) == NULL)
    {
	log_string("Could not open deniedname_file.");
	return;
    }

    while (!feof(fp))
    {
	hdp = new_header();

	hdp->name = str_dup(fread_name(fp, buf, MAX_STRING_LENGTH));

	dict_insert(g_denied_names, hdp->name, hdp);

	if (!g_denied_headers)
	    g_denied_headers = hdp;
	else
	{
	    hdp->next = g_denied_headers;
	    g_denied_headers = hdp;
	}
    }

    fclose(fp);

    log_string("Loaded denied names into dictionary.");
    return;
}
   
bool connect_to_mysql()
{
	if (!mysql_real_connect(&mysql, "localhost", "pantheon", "BuFF3L1ve5!", "pantheon", 0, NULL, 0))
	{
		std::ostringstream mess;
		mess << "Failed to connect to MySQL: Error: " << mysql_error(&mysql);
		log_string(mess.str().c_str());
		return false;
	}

	log_string("Connected to mySQL database.");
	return true;
}

/*
 * Big mama top level function.
 */
void boot_db( void )
{
// pthread_t words_pid;
int total_ep;
int iHash;
unsigned int x;
ROOM_INDEX_DATA *pRoomIndex;

    /*
     * Init some data space stuff.
     */
    {
	if ( ( string_space = (char*)calloc( 1, MAX_STRING ) ) == NULL )
	{
	    bug( "Boot_db: can't alloc %d string space.", MAX_STRING );
	    exit( 1 );
	}
	top_string	= string_space;
	fBootDb		= TRUE;
    }

    /*
     * Init random number generator.
     */
    {
        init_mm( );
    }

    /*
     * Connect to mySQL database.
     */
    if (!global_option_nomysql)
    {
		if (!mysql_init(&mysql))
        {
	    	log_string("mysql_init: Warning: insufficient memory.");
		    exit(1);
		}

		log_string("mySQL database connection initialized.");

		if (!connect_to_mysql())
			exit(1);
	}

    /*
     * Set time and weather.
     */
    {
	int day = (current_time / ((PULSE_TICK * 2) / PULSE_PER_SECOND * NUM_HOURS_DAY) % NUM_DAYS_YEAR);
	int mnum, total = 0, x;

	time_info.hour	= (current_time / ((PULSE_TICK * 2) / PULSE_PER_SECOND) % NUM_HOURS_DAY);

	time_info.day_year = (day % NUM_DAYS_YEAR) + 1;
	
	time_info.season = -1;

	for (x = 0; season_table[x].name; x++)
	    if (time_info.day_year >= season_table[x].first_day)
	        time_info.season = x;

	if (time_info.season == -1)
	    time_info.season = 3;	/* keep the MUD running, dammit */

	for (mnum = 0;month_table[mnum].name; mnum++)
	{
	    total += month_table[mnum].num_days;

	    if (time_info.day_year <= total)
	        break;
   	}

	time_info.week = (day % 7);
	time_info.month = mnum;
	time_info.day = (time_info.day_year - (total - month_table[mnum].num_days));
        time_info.phase_lunus = (int)trunc(time_info.day_year/4);
	time_info.phase_lunus %= 8;
        time_info.phase_rhos = (int)trunc(time_info.day_year/12);
	time_info.phase_rhos %= 8;
	int sr = time_info.day_year % 95;
	if (sr < 6)     sr = 2;
	else if (sr<18) sr = 1;
	else if (sr<46) sr = 0;
	else if (sr<58) sr = 1;
	else if (sr<68) sr = 2;
	else if (sr<71) sr = 3;
	else if (sr<76) sr = 4;
	else if (sr<83) sr = 3;
	else if (sr<88) sr = 4;
	else if (sr<92) sr = 3;
	else            sr = 2;
	
	time_info.size_rhos = sr;

        init_weather();
	weather_update();

    }

    /*
     * Assign gsn's for skills which have them.
     */
    {
	int sn;

	for ( sn = 0; sn < MAX_SKILL; sn++ )
	{
	    if ( skill_table[sn].pgsn != NULL )
		*skill_table[sn].pgsn = sn;
	}
    }

    /*
     * Read in all the area files.
     */
    {
	FILE *fpList;

	if ( ( fpList = fopen( AREA_LIST, "r" ) ) == NULL )
	{
	    perror( AREA_LIST );
	    exit( 1 );
	}

	for ( ; ; )
	{
	    strcpy( strArea, fread_word( fpList ) );

	    if ( strArea[0] == '$' )
		break;

	    if ( strArea[0] == '-' )
	    {
		fpArea = stdin;
	    }
	    else
	    {
		if ( ( fpArea = fopen( strArea, "r" ) ) == NULL )
		{
		    perror( strArea );
		    exit( 1 );
		}
	    }

	    for ( ; ; )
	    {
		char *word;

		if ( fread_letter( fpArea ) != '#' )
		{
		    bug( "Boot_db: # not found.", 0 );
		    exit( 1 );
		}

		word = fread_word( fpArea );

		     if ( word[0] == '$'               )                 break;
		else if ( !str_cmp( word, "AREA"     ) ) load_area    (fpArea);
  /* OLC */     else if ( !str_cmp( word, "AREADATA" ) ) new_load_area(fpArea);
//		else if ( !str_cmp( word, "HELPS"    ) ) load_helps   (fpArea);
		else if ( !str_cmp( word, "MOBOLD"   ) ) load_old_mob (fpArea);
		else if ( !str_cmp( word, "MOBILES"  ) ) load_mobiles (fpArea);
                else if ( !str_cmp( word, "MOBPROGS" ) ) load_mobprogs(fpArea);
        	else if ( !str_cmp( word, "OBJOLD" ) ) load_old_obj   (fpArea);
	  	else if ( !str_cmp( word, "OBJECTS"  ) ) load_objects (fpArea);
		else if ( !str_cmp( word, "RESETS"   ) ) load_resets  (fpArea);
		else if ( !str_cmp( word, "ROOMS"    ) ) load_rooms   (fpArea);
		else if ( !str_cmp( word, "SHOPS"    ) ) load_shops   (fpArea);
		else if ( !str_cmp( word, "GUILD"    ) ) load_gms     (fpArea);
		else if ( !str_cmp( word, "SOCIALS"  ) ) load_socials (fpArea);
		else if ( !str_cmp( word, "SPECIALS" ) ) load_specials(fpArea);
		else
		{
		    bug( "Boot_db: bad section name.", 0 );
		    exit( 1 );
		}
	    }

	    if ( fpArea != stdin )
		fclose( fpArea );
	    fpArea = NULL;
	}
	fclose( fpList );
    }

    fBootDb = FALSE;
    loading_char = FALSE;

    FactionTable::Instance();
    load_char_names();
    load_denied_names();

// Made some functions unix-only for now, so the MUD can (maybe) load under DOS.
    pfile_maint(TRUE);
    load_obj_rooms();
    log_string("loaded vault objects");
// #endif

    fBootDb = TRUE;
    post_boot = TRUE;

    /*
     * Fix up exits.
     * Declare db booting over.
     * Reset all areas once.
     * Load up the songs, notes and ban files.
     */
    {
        fix_exits( );
        log_string("exits fixed");
        make_alinks( );
        log_string("alinks created");
        assign_herbs( );
        log_string("herbs assigned");
            convert_objects( );           /* ROM OLC */
        log_string("objects converted");
        area_update( );
        log_string("areas updated");
        fBootDb = FALSE;
        load_notes( );
        log_string("notes loaded");
        load_bans();
        log_string("bans loaded");
        language_preproc();
        log_string("language preprocessing completed");
        rangermenu_init();
        fread_rumors();
        log_string("rumors exaggerated");

        if (Weave::LoadWeave())
            log_string("weave loaded");
        else
        {
            Weave::Regenerate();
            log_string("weave failed to load, regenerated");
        }

        if (Oaths::LoadOaths())
            log_string("oaths loaded");
        else
        {
            log_string("oaths failed to load, bailing to avoid corrupting them");
            exit(1);
        }
        MOBtrigger = TRUE;
    }
    log_obj_type(0);
//    log_altars();
 
    global_int_class_waterscholar = class_lookup("water scholar");
    global_int_class_earthscholar = class_lookup("earth scholar");
    global_int_class_voidscholar = class_lookup("void scholar");
    global_int_class_spiritscholar = class_lookup("spirit scholar");
    global_int_class_airscholar = class_lookup("air scholar");
    global_int_class_firescholar = class_lookup("fire scholar");
    global_int_class_watertemplar = class_lookup("water templar");
    global_int_class_earthtemplar = class_lookup("earth templar");
    global_int_class_voidtemplar = class_lookup("void templar");
    global_int_class_spirittemplar = class_lookup("spirit templar");
    global_int_class_airtemplar = class_lookup("air templar");
    global_int_class_firetemplar = class_lookup("fire templar");
    global_int_class_thief = class_lookup("thief");
    global_int_class_watcher = class_lookup("watcher");
    global_int_class_assassin = class_lookup("assassin");
    global_int_class_bandit = class_lookup("bandit");
    global_int_class_rogue5 = class_lookup("rogue5");
    global_int_class_rogue6 = class_lookup("rogue6");
    global_int_class_fighter = class_lookup("fighter");
    global_int_class_swordmaster = class_lookup("swordmaster");
    global_int_class_barbarian = class_lookup("barbarian");
    global_int_class_gladiator = class_lookup("gladiator");
    global_int_class_warrior = class_lookup("warrior");
    global_int_class_ranger = class_lookup("ranger");
    global_int_class_gleeman = class_lookup("gleeman");
    global_int_class_bard = class_lookup("bard");
    global_int_class_entertainer = class_lookup("entertainer");
    global_int_class_alchemist = class_lookup("alchemist");
    global_int_class_psionicist = class_lookup("psionicist");
    global_int_class_druid = class_lookup("druid");

    global_int_race_human = race_lookup("human");
    global_int_race_alatharya = race_lookup("alatharya");
    global_int_race_srryn = race_lookup("srryn");
    global_int_race_kankoran = race_lookup("kankoran");
    global_int_race_chtaren  = race_lookup("ch'taren");
    global_int_race_aelin = race_lookup("aelin");
    global_int_race_shuddeni = race_lookup("shuddeni");
    global_int_race_nefortu = race_lookup("nefortu");
    global_int_race_caladaran = race_lookup("caladaran");
    global_int_race_ethron  = race_lookup("ethron");
    global_int_race_chaja = race_lookup("chaja");
    global_int_race_seraph = race_lookup("seraph");
    
    global_bool_final_valid = FALSE;
    global_bool_full_damage = FALSE;
    global_bool_check_avoid = TRUE; /* parry, shield block, etc */
    global_bool_ranged_attack = FALSE; /* bolo, dart, etc */
    global_damage_from = NULL;

    silver_state = 0;
    last_mob_id = 0;

    crumble_done = FALSE;
    crumble_process = FALSE;
    crumble_test = FALSE;

    total_ep = 0;
    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
	for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
	    total_ep += URANGE(0, UMAX(pRoomIndex->danger, pRoomIndex->area->danger), 10);
    }

    bug("Total ep: %d", total_ep);

    for (x = 1;x < MAX_LEVEL;x++)
    {
      double y, z;
      int xx;
      y = (double)x;
      z = pow(2.7183, (log(total_ep/2)/51)*x);
      xx = UMIN(EP_MIN_TO_HERO, (int)z);
      ep_table[x] = xx;
    }
  
    return;
}

/*void log_altars()
{

    ROOM_INDEX_DATA *room;
    FILE *fp;

    int vnum;
    int i;

    if ((fp = fopen("../log/altars.txt","w")) == NULL)
    {
	log_string("Failed to open altars.txt for writing.");
	return;
    } 
    for (vnum = 0;vnum<32768;vnum++)
    {
	if ((room = get_room_index(vnum)) == NULL)
	    continue;
	if (room->gods_altar != 0)
	{
	    for (i=0;i<MAX_GODS;i++)
		if (IS_SET(room->gods_altar,	    
	    fprintf(fp,"%s|%i|%s\n",
	      room->name,
	      room->vnum,
	
	);    
	}
    }
    fclose(fp);
}
*/
void log_obj_type(int type)
{

    OBJ_INDEX_DATA *obj;
    FILE *fp;

    int vnum;

    if ((fp = fopen("../log/objects.txt","w")) == NULL)
    {
	log_string("Failed to open objects.txt for writing.");
	return;
    } 
/*    switch (type)
    {
	case ITEM_WEAPON: 
	    fprintf(fp,"name|vnum|weight|type|average|level|material|2h\n"); 
	    break;
	case ITEM_WAND:
	case ITEM_STAFF: 
	    fprintf(fp,"name|vnum|area|limit|spell level|total charges|starting charges|spell\n");
	    break;
	case ITEM_SCROLL:
	case ITEM_PILL:
	case ITEM_POTION: 
	    fprintf(fp,"name|vnum|area|limit|spell level|spell1|spell2|spell3|spell4\n"); 
	    break;
    }
*/
    fprintf(fp,"name|vnum|limit_factor|limit\n");
    for (vnum = 0;vnum<32768;vnum++)
    {
	if ((obj = get_obj_index(vnum)) == NULL)
	    continue;
	if (obj->limit_factor > 0)
	    fprintf(fp,"%s|%i|%i|%i\n",obj->short_descr,obj->vnum,
		obj->limit_factor,obj->limit);
/*
	if (obj->item_type != type)
	    continue;
	if (obj->item_type == ITEM_WEAPON)
//short descr, vnum, weight, type, average, level, material, 2h y/n
	      fprintf(fp,"%s|%i|%i|%s|%i|%i|%s|%c\n",
	      	obj->short_descr,
		obj->vnum,
	      	obj->weight,
	      	weapon_class[obj->value[0]].name,
		obj->value[1] * (obj->value[2]+1) / 2,
		obj->level,
		material_table[obj->material].name,
		IS_WEAPON_STAT(obj,WEAPON_TWO_HANDS) ? 'y' : 'n'); 
	else if (obj->item_type == ITEM_WAND
	  || obj->item_type == ITEM_STAFF)
	
// short descr, vnum, area, limit, spell level, total charges, starting charges, spell
	    fprintf(fp,"%s|%i|%s|%i|%i|%i|%i|%s\n",
	      obj->short_descr,
	      obj->vnum,
	      obj->area->name,
	      obj->limit,
	      obj->value[0],
	      obj->value[1],
	      obj->value[2],
	      obj->value[3] != -1 ? skill_table[obj->value[3]].name : "");
	else if (obj->item_type == ITEM_POTION 
	  || obj->item_type == ITEM_PILL 
	  || obj->item_type == ITEM_SCROLL) 
// short descr, vnum, area, limit, spell level, spell1, spell2, spell3, spell4
	      fprintf(fp,"%s|%i|%s|%i|%i|%s|%s|%s|%s\n",
		obj->short_descr,
		obj->vnum,
		obj->area->name,
		obj->limit,
		obj->value[0],
		obj->value[1] != -1 ? skill_table[obj->value[1]].name : "",
		obj->value[2] != -1 ? skill_table[obj->value[2]].name : "",
		obj->value[3] != -1 ? skill_table[obj->value[3]].name : "",
		obj->value[4] != -1 ? skill_table[obj->value[4]].name : "");
*/
    }

    fclose(fp);
}

/*
 * Snarf an 'area' header line.
 */
void load_area( FILE *fp )
{
    AREA_DATA *pArea;

    pArea		= (AREA_DATA*)alloc_perm( sizeof(*pArea) );
    pArea->file_name	= fread_string(fp);

    pArea->area_flags   = AREA_LOADING;         /* OLC */
    pArea->security     = 9;                    /* OLC */ /* 9 -- Hugin */
    pArea->builders     = str_dup( "None" );    /* OLC */
    pArea->vnum         = top_area;             /* OLC */

    pArea->affected	= NULL;
    pArea->name		= fread_string( fp );
    pArea->credits	= fread_string( fp );
    pArea->age		= 15;
    pArea->nplayer	= 0;
    pArea->danger	= 1;

    pArea->fount_frequency = Fount::Default;
    pArea->fount_order_bias = 0;
    pArea->fount_positive_bias = 0;

    pArea->shade_density = Shades::DefaultDensity;
    pArea->shade_power = Shades::DefaultPower;

    pArea->stone_type = -1;

    pArea->empty	= FALSE;

    if ( !area_first )
	area_first = pArea;
    if ( area_last )
    {
	area_last->next = pArea;
        REMOVE_BIT(area_last->area_flags, AREA_LOADING);        /* OLC */
    }
    area_last	= pArea;
    pArea->next	= NULL;

    top_area++;
}

/*
 * OLC
 * Use these macros to load any new area formats that you choose to
 * support on your MUD.  See the new_load_area format below for
 * a short example.
 */
#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )                \
                if ( !str_cmp( word, literal ) )    \
                {                                   \
                    field  = value;                 \
                    fMatch = TRUE;                  \
                    break;                          \
                                }

#define SKEY( string, field )                       \
                if ( !str_cmp( word, string ) )     \
                {                                   \
                    free_string( field );           \
                    field = fread_string( fp );     \
                    fMatch = TRUE;                  \
                    break;                          \
                                }



/* OLC
 * Snarf an 'area' header line.   Check this format.  MUCH better.  Add fields
 * too.
 *
 * #AREAFILE
 * Name   { All } Locke    Newbie School~
 * Repop  A teacher pops in the room and says, 'Repop coming!'~
 * Recall 3001
 * End
 */
void new_load_area( FILE *fp )
{
    AREA_DATA *pArea;
    VNUM_RANGE *vnums;
    const char      *word(0);
    bool      fMatch;

    pArea               = (AREA_DATA*)alloc_perm( sizeof(*pArea) );
    pArea->age          = 15;
    pArea->nplayer      = 0;
    pArea->file_name     = str_dup( strArea );
    pArea->vnum         = top_area;
    pArea->name         = str_dup( "New Area" );
    pArea->builders     = str_dup( "" );
    pArea->security     = 9;                    /* 9 -- Hugin */
    pArea->area_flags   = 0;
    pArea->danger	= 1;
    pArea->ainfo_flags	= 0;
    pArea->herbs	= 0;

    pArea->fount_frequency = Fount::Default;
    pArea->fount_order_bias = 0;
    pArea->fount_positive_bias = 0;

    pArea->shade_density = Shades::DefaultDensity;
    pArea->shade_power = Shades::DefaultPower;

    pArea->stone_type = -1;

    pArea->base_precip  = 2;
    pArea->base_temp	= 2;
    pArea->base_wind_mag = 2;
    pArea->base_wind_dir = 1;
    pArea->geography	= 0;

    for ( ; ; )
    {
       word   = feof( fp ) ? "End" : fread_word( fp );
       fMatch = FALSE;

       switch ( UPPER(word[0]) )
       {
           case 'W':
	            if ( !str_cmp( word, "Weather" ) )
        	    {
		            pArea->base_precip 	= fread_number( fp );
            		pArea->base_temp   	= fread_number( fp );
            		pArea->base_wind_mag	= fread_number( fp );
            		pArea->base_wind_dir	= fread_number( fp );
            		pArea->geography	= fread_number( fp );
	            }
                else if (!str_cmp(word, "Weave"))
                {
                    // Read weave values
                    int freq = fread_number(fp);
                    int order_bias = fread_number(fp);
                    int positive_bias = fread_number(fp);
                    
                    // Sanity-check
                    if (freq < 0 || freq >= Fount::Max 
                    || order_bias < Fount::PowerMin || order_bias > Fount::PowerMax
                    || positive_bias < Fount::PowerMin || positive_bias > Fount::PowerMax)
                        bug("Invalid fount value(s) in new_load_area", 0);
                    
                    // Assign values, forcing them into valid ranges
                    pArea->fount_frequency = static_cast<Fount::Frequency>(URANGE(0, freq, Fount::Max - 1));
                    pArea->fount_order_bias = URANGE(Fount::PowerMin, order_bias, Fount::PowerMax);
                    pArea->fount_positive_bias = URANGE(Fount::PowerMin, positive_bias, Fount::PowerMax);
                }
        	    break;
	   case 'A':
	     KEY( "Areainfo", pArea->ainfo_flags, fread_number( fp ) );
	    break;
	   case 'H':
	     KEY( "Herbs", pArea->herbs, fread_number( fp ) );
	     break;
           case 'N':
            SKEY( "Name", pArea->name );
            break;
           case 'D':
             KEY( "Danger", pArea->danger, fread_number( fp ) );
            break;
           case 'S':
            if (!str_cmp(word, "Shades"))
            {
                int density(fread_number(fp));
                int power(fread_number(fp));
                pArea->shade_density = static_cast<Shades::Density>(URANGE(0, density, Shades::MaxDensity - 1));
                pArea->shade_power = static_cast<Shades::Power>(URANGE(0, power, Shades::MaxPower - 1));
            }
            KEY( "Stone", pArea->stone_type, fread_number(fp));
            KEY( "Security", pArea->security, fread_number( fp ) );
            break;
           case 'V':
            if ( !str_cmp( word, "VNUMs" ) )
            {
		vnums = (VNUM_RANGE*)alloc_perm(sizeof(*vnums));
		g_vnum_blocks++;
                vnums->min_vnum = fread_number( fp );
                vnums->max_vnum = fread_number( fp );
		vnums->next = pArea->vnums;
		pArea->vnums = vnums;
            }
            break;
           case 'E':
             if ( !str_cmp( word, "End" ) )
             {
                 fMatch = TRUE;
                 if ( area_first == NULL )
                    area_first = pArea;
                 if ( area_last  != NULL )
                    area_last->next = pArea;
                 area_last   = pArea;
                 pArea->next = NULL;
                 top_area++;
                 return;
            }
            break;
           case 'B':
            SKEY( "Builders", pArea->builders );
            break;
	   case 'C':
	    SKEY( "Credits", pArea->credits );
	    break;
        }
    }
}

// Snarf a mob section.  old style 
void load_old_mob( FILE *fp )
{
    MOB_INDEX_DATA *pMobIndex;
    /* for race updating */
    int race, i;
    char *ctemp;
    char name[MAX_STRING_LENGTH];

    if ( !area_last )   /* OLC */
    {
        bug( "Load_mobiles: no #AREA seen yet.", 0 );
        exit( 1 );
    }

    for ( ; ; )
    {
	int vnum;
	char letter;
	int iHash;

	letter				= fread_letter( fp );
	if ( letter != '#' )
	{
	    bug( "Load_mobiles: # not found.", 0 );
	    exit( 1 );
	}

	vnum				= fread_number( fp );
	if ( vnum == 0 )
	    break;

	fBootDb = FALSE;
	if ( get_mob_index( vnum ) != NULL )
	{
	    bug( "Load_mobiles: vnum %d duplicated.", vnum );
	    exit( 1 );
	}
	fBootDb = TRUE;

	pMobIndex			= (MOB_INDEX_DATA*)alloc_perm( sizeof(*pMobIndex) );
	pMobIndex->vnum			= vnum;
        pMobIndex->area                 = area_last;               /* OLC */
	pMobIndex->new_format		= FALSE;
	pMobIndex->player_name		= fread_string( fp );
	pMobIndex->short_descr		= fread_string( fp );
	pMobIndex->long_descr		= fread_string( fp );
	pMobIndex->description		= fread_string( fp );

	pMobIndex->long_descr[0]	= UPPER(pMobIndex->long_descr[0]);
	pMobIndex->description[0]	= UPPER(pMobIndex->description[0]);

	pMobIndex->act			= fread_flag( fp ) | ACT_IS_NPC;
	pMobIndex->affected_by		= fread_flag( fp );
	pMobIndex->pShop		= NULL;
	
	ctemp	= fread_word( fp );

	if (is_number(ctemp))  /* Support for old format */
	{
	    i = atoi(ctemp);
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

	letter				= fread_letter( fp );
	pMobIndex->level		= fread_number( fp );

	/*
	 * The unused stuff is for imps who want to use the old-style
	 * stats-in-files method.
	 */
					  fread_number( fp );	/* Unused */
					  fread_number( fp );	/* Unused */
					  fread_number( fp );	/* Unused */
	/* 'd'		*/		  fread_letter( fp );	/* Unused */
					  fread_number( fp );	/* Unused */
	/* '+'		*/		  fread_letter( fp );	/* Unused */
					  fread_number( fp );	/* Unused */
					  fread_number( fp );	/* Unused */
	/* 'd'		*/		  fread_letter( fp );	/* Unused */
					  fread_number( fp );	/* Unused */
	/* '+'		*/		  fread_letter( fp );	/* Unused */
					  fread_number( fp );	/* Unused */
        pMobIndex->wealth               = fread_number( fp )/20;	
	/* xp can't be used! */		  fread_number( fp );	/* Unused */
	pMobIndex->start_pos		= fread_number( fp );	/* Unused */
	pMobIndex->default_pos		= fread_number( fp );	/* Unused */

  	if (pMobIndex->start_pos < POS_SLEEPING)
	    pMobIndex->start_pos = POS_STANDING;
	if (pMobIndex->default_pos < POS_SLEEPING)
	    pMobIndex->default_pos = POS_STANDING;

	/*
	 * Back to meaningful values.
	 */
	pMobIndex->sex			= fread_number( fp );

    	/* compute the race BS */
   	one_argument(pMobIndex->player_name,name);
 
   	if (name[0] == '\0' || (race =  race_lookup(name)) == 0)
   	{
            /* fill in with blanks */
            pMobIndex->race = race_lookup("human");
            pMobIndex->off_flags = OFF_DODGE|OFF_DISARM|OFF_TRIP|ASSIST_VNUM;
            pMobIndex->imm_flags = 0;
            pMobIndex->res_flags = 0;
            pMobIndex->vuln_flags = 0;
            pMobIndex->form = FORM_EDIBLE|FORM_SENTIENT|FORM_BIPED|FORM_MAMMAL;
            pMobIndex->parts = PART_HEAD|PART_ARMS|PART_LEGS|PART_HEART|
                               PART_BRAINS|PART_GUTS;
    	}
    	else
    	{
            pMobIndex->race = race;
            pMobIndex->off_flags = OFF_DODGE|OFF_DISARM|OFF_TRIP|ASSIST_RACE|
                                   race_table[race].off;
            pMobIndex->imm_flags = race_table[race].imm;
            pMobIndex->res_flags = race_table[race].res;
            pMobIndex->vuln_flags = race_table[race].vuln;
            pMobIndex->form = race_table[race].form;
            pMobIndex->parts = race_table[race].parts;
    	}

	if ( letter != 'S' )
	{
	    bug( "Load_mobiles: vnum %d non-S.", vnum );
	    exit( 1 );
	}

	convert_mobile( pMobIndex );                /* ROM OLC */

	iHash			= vnum % MAX_KEY_HASH;
	pMobIndex->next		= mob_index_hash[iHash];
	mob_index_hash[iHash]	= pMobIndex;
	top_mob_index++;
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;  /* OLC */
//      assign_area_vnum( vnum );                                  /* OLC */
	kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL-1)].number++;
    }

    return;
}

/*
 * Snarf an obj section.  old style 
 */
void load_old_obj( FILE *fp )
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
	char letter;
	int iHash;

	letter				= fread_letter( fp );
	if ( letter != '#' )
	{
	    bug( "Load_objects: # not found.", 0 );
	    exit( 1 );
	}

	vnum				= fread_number( fp );
	if ( vnum == 0 )
	    break;

	fBootDb = FALSE;
	if ( get_obj_index( vnum ) != NULL )
	{
	    bug( "Load_objects: vnum %d duplicated.", vnum );
	    exit( 1 );
	}
	fBootDb = TRUE;

	pObjIndex			= (OBJ_INDEX_DATA*)alloc_perm( sizeof(*pObjIndex) );
	pObjIndex->vnum			= vnum;
        pObjIndex->area                 = area_last;            /* OLC */
	pObjIndex->new_format		= FALSE;
	pObjIndex->reset_num	 	= 0;
	pObjIndex->name			= fread_string( fp );
	pObjIndex->short_descr		= fread_string( fp );
	pObjIndex->description		= fread_string( fp );
	/* Action description */	  fread_string( fp );

	pObjIndex->short_descr[0]	= LOWER(pObjIndex->short_descr[0]);
	pObjIndex->description[0]	= UPPER(pObjIndex->description[0]);
	pObjIndex->material		= MATERIAL_UNKNOWN;

	pObjIndex->item_type		= fread_number( fp );
	pObjIndex->extra_flags[0]	= fread_flag( fp );
	pObjIndex->wear_flags		= fread_flag( fp );
	pObjIndex->value[0]		= fread_number( fp );
	pObjIndex->value[1]		= fread_number( fp );
	pObjIndex->value[2]		= fread_number( fp );
	pObjIndex->value[3]		= fread_number( fp );
	pObjIndex->value[4]		= 0;
	pObjIndex->level		= 0;
	pObjIndex->condition 		= 100;
	pObjIndex->weight		= fread_number( fp );
	pObjIndex->cost			= fread_number( fp );	/* Unused */
	/* Cost per day */		  fread_number( fp );


	if (pObjIndex->item_type == ITEM_WEAPON)
	{
	    if (is_name("two",pObjIndex->name) 
	    ||  is_name("two-handed",pObjIndex->name) 
	    ||  is_name("claymore",pObjIndex->name))
		SET_BIT(pObjIndex->value[4],WEAPON_TWO_HANDS);
	}

	for ( ; ; )
	{
	    char letter;

	    letter = fread_letter( fp );

	    if ( letter == 'A' )
	    {
		AFFECT_DATA *paf;

		paf			= (AFFECT_DATA*)alloc_perm( sizeof(*paf) );
		paf->where		= TO_OBJECT;
		paf->type		= -1;
		paf->level		= 20; /* RT temp fix */
		paf->duration		= -1;
		paf->location		= fread_number( fp );
		paf->modifier		= fread_number( fp );
		paf->bitvector		= 0;
		paf->next		= pObjIndex->affected;
		pObjIndex->affected	= paf;
		top_affect++;
	    }

	    else if ( letter == 'E' )
	    {
		EXTRA_DESCR_DATA *ed(0);

		ed			= (EXTRA_DESCR_DATA*)alloc_perm( sizeof(*ed) );
		ed->keyword		= fread_string( fp );
		ed->description		= fread_string( fp );
		ed->next		= pObjIndex->extra_descr;
		pObjIndex->extra_descr	= ed;
		top_ed++;
	    }

	    else
	    {
		ungetc( letter, fp );
		break;
	    }
	}

        /* fix armors */
        if (pObjIndex->item_type == ITEM_ARMOR)
        {
            pObjIndex->value[1] = pObjIndex->value[0];
            pObjIndex->value[2] = pObjIndex->value[1];
        }

	/*
	 * Translate spell "slot numbers" to internal "skill numbers."
	 */
	switch ( pObjIndex->item_type )
	{
	case ITEM_PILL:
	case ITEM_POTION:
	case ITEM_SCROLL:
	case ITEM_OIL:
	    pObjIndex->value[1] = slot_lookup( pObjIndex->value[1] );
	    pObjIndex->value[2] = slot_lookup( pObjIndex->value[2] );
	    pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
	    pObjIndex->value[4] = slot_lookup( pObjIndex->value[4] );
	    break;

	case ITEM_STAFF:
	case ITEM_WAND:
	    pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
	    break;
	}

        letter = fread_letter( fp );
        if ( letter == '>' )
        {
          ungetc( letter, fp );
          oprog_read_programs( fp, pObjIndex );
        }
        else ungetc( letter,fp );

	iHash			= vnum % MAX_KEY_HASH;
	pObjIndex->next		= obj_index_hash[iHash];
	obj_index_hash[iHash]	= pObjIndex;
	top_obj_index++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;   /* OLC */
//        assign_area_vnum( vnum );                                   /* OLC */
    }

    return;
}

/*
 * Adds a reset to a room.  OLC
 * Similar to add_reset in olc.c
 */
void new_reset( ROOM_INDEX_DATA *pR, RESET_DATA *pReset )
{
    RESET_DATA *pr;

    if ( !pR )
       return;

    pr = pR->reset_last;

    if ( !pr )
    {
        pR->reset_first = pReset;
        pR->reset_last  = pReset;
    }
    else
    {
        pR->reset_last->next = pReset;
        pR->reset_last       = pReset;
        pR->reset_last->next = NULL;
    }

    top_reset++;
    return;
}


void load_words( void )
{
char buf[HUGEASS_WORDSTRING];
char *word;
FILE *fp;
int words;

dictwords = NULL;

  if ((fp = fopen("/usr/dict/words", "r")) == NULL)
  {
    dictwords = NULL; /* we give up trying rather than dying */
    return;
  }

  buf[0] = (char)0; 

  words = 0;
  for (word = get_word(fp); word != NULL; )
  {
    words++;
    strcat(buf, word);
    strcat(buf, " ");
    word = get_word(fp);
  }

  dictwords = (char*)malloc(sizeof(char) * strlen(buf) + 1);
  strcpy(dictwords, buf);

  sprintf(buf, "Read %d words from /usr/dict/words, string length %ld", words, (long) strlen(dictwords));
  fclose(fp);
  log_string(buf);
}

/*
 * Snarf a reset section.
 */
void load_resets( FILE *fp )
{
    RESET_DATA *pReset;
    int         iLastRoom = 0;
    int         iLastObj  = 0;

    if ( !area_last )
    {
	bug( "Load_resets: no #AREA seen yet.", 0 );
	exit( 1 );
    }

    for ( ; ; )
    {
	ROOM_INDEX_DATA *pRoomIndex;
	EXIT_DATA *pexit;
	char letter;
/*	OBJ_INDEX_DATA *temp_index;
	int temp; */

	if ( ( letter = fread_letter( fp ) ) == 'S' )
	    break;

	if ( letter == '*' )
	{
	    fread_to_eol( fp );
	    continue;
	}

	pReset		= (RESET_DATA*)alloc_perm( sizeof(*pReset) );
	pReset->command	= letter;
	/* if_flag */	  fread_number( fp );
	pReset->arg1	= fread_number( fp );
	pReset->arg2	= fread_number( fp );
	pReset->arg3	= (letter == 'G' || letter == 'R')
			    ? 0 : fread_number( fp );
	pReset->arg4	= (letter == 'P' || letter == 'M')
			    ? fread_number(fp) : 0;
			  fread_to_eol( fp );

	/*
	 * Validate parameters.
	 * We're calling the index functions for the side effect.
	 */
	switch ( letter )
	{
	default:
	    bug( "Load_resets: bad command '%c'.", letter );
	    exit( 1 );
	    break;

	case 'M':
	    get_mob_index  ( pReset->arg1 );
            if ( ( pRoomIndex = get_room_index ( pReset->arg3 ) ) )
            {
                new_reset( pRoomIndex, pReset );
                iLastRoom = pReset->arg3;
            }
	    break;

	case 'O':
	    get_obj_index ( pReset->arg1 );
            if ( ( pRoomIndex = get_room_index ( pReset->arg3 ) ) )
            {
                new_reset( pRoomIndex, pReset );
                iLastObj = pReset->arg3;
            }
	    break;

	case 'P':
	    get_obj_index  ( pReset->arg1 );
            if ( ( pRoomIndex = get_room_index ( iLastObj ) ) )
            {
                new_reset( pRoomIndex, pReset );
            }
	    break;

	case 'G':
	case 'E':
	    get_obj_index (pReset->arg1);
            if ( ( pRoomIndex = get_room_index ( iLastRoom ) ) )
            {
                new_reset( pRoomIndex, pReset );
                iLastObj = iLastRoom;
            }
	    break;

	case 'D':
	    pRoomIndex = get_room_index( pReset->arg1 );

	    if ( pReset->arg2 < 0
	    ||   pReset->arg2 > 5
            || !pRoomIndex
	    || !( pexit = pRoomIndex->exit[pReset->arg2] ) )
	    {
		bug( "Load_resets: 'D': exit %d not door.", pReset->arg2 );
		exit( 1 );
	    }

		pexit->exit_info &= pReset->arg3;
	    break;

	case 'R':
	    pRoomIndex		= get_room_index( pReset->arg1 );

	    if ( pReset->arg2 < 0 || pReset->arg2 > 6 )
	    {
		bug( "Load_resets: 'R': bad exit %d.", pReset->arg2 );
		exit( 1 );
	    }

            if ( pRoomIndex )
                new_reset( pRoomIndex, pReset );

	    break;
	}
    }

    return;
}

/*
 * Snarf a room section.
 */
void load_rooms( FILE *fp )
{
    char wlist[MAX_STRING_LENGTH], wlist_arg[MAX_STRING_LENGTH];
    char *wlistptr, *wlist_argptr;
    ROOM_INDEX_DATA *pRoomIndex;
    TRACK_DATA	*tracks;


    if ( area_last == NULL )
    {
	bug( "Load_resets: no #AREA seen yet.", 0 );
	exit( 1 );
    }

    for ( ; ; )
    {
	int vnum;
	char letter;
	int door;
	int iHash;

    	wlistptr = &wlist[0];
    	wlist_argptr = &wlist_arg[0]; /* yes, another cheap hack */

	letter				= fread_letter( fp );
	if ( letter != '#' )
	{
	    bug( "Load_rooms: # not found.", 0 );
	    exit( 1 );
	}

	vnum				= fread_number( fp );
	if ( vnum == 0 )
	    break;

	fBootDb = FALSE;
	if ( get_room_index( vnum ) != NULL )
	{
	    bug( "Load_rooms: vnum %d duplicated.", vnum );
	    exit( 1 );
	}
	fBootDb = TRUE;

	pRoomIndex			= (ROOM_INDEX_DATA*)alloc_perm( sizeof(*pRoomIndex) );
	pRoomIndex->owner		= str_dup("");
	pRoomIndex->people		= NULL;
	pRoomIndex->contents		= NULL;
	pRoomIndex->extra_descr		= NULL;
	pRoomIndex->area		= area_last;
	pRoomIndex->vnum		= vnum;
	pRoomIndex->danger		= 1;
	pRoomIndex->gods_altar	= 0;
	pRoomIndex->name		= fread_string( fp );
	pRoomIndex->description		= fread_string( fp );
    memset(pRoomIndex->roomvalue, 0, sizeof(pRoomIndex->roomvalue));
    memset(pRoomIndex->stringvalue, 0, sizeof(pRoomIndex->stringvalue));
 
	if (IS_SET(pRoomIndex->area->ainfo_flags, AINFO_HASOWNER))
	    pRoomIndex->owner		= fread_string( fp );

	pRoomIndex->danger		= fread_number( fp );

        if (!IS_SET(pRoomIndex->area->ainfo_flags, AINFO_NOWORDLIST))
	    fread_string(fp);

	/* Area number */		  fread_number( fp );
	pRoomIndex->room_flags		= fread_flag( fp );
	pRoomIndex->sector_type		= fread_number( fp );
	pRoomIndex->light		= 0;
	for ( door = 0; door <= 5; door++ )
	    pRoomIndex->exit[door] = NULL;

	/* defaults */
	pRoomIndex->heal_rate = 100;
	pRoomIndex->move_rate = 100;
	pRoomIndex->mana_rate = 100;

    pRoomIndex->ley_group = NULL;
    pRoomIndex->fount_frequency = Fount::Default;
    pRoomIndex->fount_order_power = 0;
    pRoomIndex->fount_positive_power = 0;

    pRoomIndex->shade_density = Shades::DefaultDensity;
    pRoomIndex->shade_power = Shades::DefaultPower;

    pRoomIndex->stone_type = -1;

	for ( ; ; )
	{
	    letter = fread_letter( fp );

	    if ( letter == 'S' )
	    	break;

	    if ( letter == 'H') /* healing room */
    		pRoomIndex->heal_rate = fread_number(fp);

		else if (letter == 'A')
			pRoomIndex->gods_altar = fread_number(fp);
		
	    else if ( letter == 'V' )
	    	pRoomIndex->move_rate = fread_number(fp);
	
	    else if ( letter == 'M') /* mana room */
    		pRoomIndex->mana_rate = fread_number(fp);

        else if (letter == 'T') // stone Type
            pRoomIndex->stone_type = fread_number(fp);

	   else if ( letter == 'C') /* clan */
	   {
		if (pRoomIndex->clan)
	  	{
		    bug("Load_rooms: duplicate clan fields.",0);
		    exit(1);
		}
		pRoomIndex->clan = clan_lookup(fread_string(fp));
	    }
	

	    else if ( letter == 'D' )
	    {
		EXIT_DATA *pexit;

		door = fread_number( fp );
		if ( door < 0 || door > 5 )
		{
		    bug( "Fread_rooms: vnum %d has bad door number.", vnum );
		    exit( 1 );
		}

		pexit			= (EXIT_DATA*)alloc_perm( sizeof(*pexit) );
		pexit->description	= fread_string( fp );
		pexit->keyword		= fread_string( fp );
		pexit->exit_info	= 0;
                pexit->rs_flags         = 0;                    /* OLC */
		pexit->rs_flags		= fread_number( fp );
		pexit->exit_info	= pexit->rs_flags;
		pexit->key		= fread_number( fp );
		pexit->u1.vnum		= fread_number( fp );
		pexit->orig_door	= door;			/* OLC */

		pRoomIndex->exit[door]	= pexit;
		pRoomIndex->old_exit[door] = pexit;
		top_exit++;
	    }
	    else if ( letter == 'E' )
	    {
		EXTRA_DESCR_DATA *ed(0);

		ed			= (EXTRA_DESCR_DATA*)alloc_perm( sizeof(*ed) );
		ed->keyword		= fread_string( fp );
		ed->description		= fread_string( fp );
		ed->next		= pRoomIndex->extra_descr;
		pRoomIndex->extra_descr	= ed;
		top_ed++;
	    }

	    else if (letter == 'O')
	    {
		if (pRoomIndex->owner[0] != '\0')
		{
		    bug("Load_rooms: duplicate owner.",0);
		    exit(1);
		}

		pRoomIndex->owner = fread_string(fp);
	    }

	    else if (letter == 'P')
	    {
		rprog_read_programs( fp, pRoomIndex );
	    }

        else if (letter == 'W')
        {
            // Read weave values
            int freq = fread_number(fp);
            int order_power = fread_number(fp);
            int positive_power = fread_number(fp);

            // Sanity-check
            if (freq < 0 || freq >= Fount::Max
            || order_power < Fount::PowerMin || order_power > Fount::PowerMax
            || positive_power < Fount::PowerMin || positive_power > Fount::PowerMax)
                bug("Invalid fount value(s) in load_room", 0);
            
            // Assign values, forcing them into valid ranges
            pRoomIndex->fount_frequency = static_cast<Fount::Frequency>(URANGE(0, freq, Fount::Max - 1));
            pRoomIndex->fount_order_power = URANGE(Fount::PowerMin, order_power, Fount::PowerMax);
            pRoomIndex->fount_positive_power = URANGE(Fount::PowerMin, positive_power, Fount::PowerMax);
        }
        else if (letter == 'Z')
        {
            // Read shade values
            int density(fread_number(fp));
            int power(fread_number(fp));

            // Sanity-check
            if (density < 0 || power < 0 || density >= Shades::MaxDensity || power >= Shades::MaxPower)
                bug("Invalid shade value(s) in load_room", 0);

            // Assign values, forcing them into valid ranges
            pRoomIndex->shade_density = static_cast<Shades::Density>(URANGE(0, density, Shades::MaxDensity - 1));
            pRoomIndex->shade_power = static_cast<Shades::Power>(URANGE(0, power, Shades::MaxPower - 1));
        }

	    else
	    {
		bug( "Load_rooms: vnum %d has flag not 'DES'.", vnum );
		exit( 1 );
	    }
	}

	iHash			= vnum % MAX_KEY_HASH;
	pRoomIndex->next	= room_index_hash[iHash];
	room_index_hash[iHash]	= pRoomIndex;
	top_room++;
        top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room; /* OLC */
	tracks = (TRACK_DATA*)alloc_perm(sizeof(*tracks));
	g_num_tracks++;
	pRoomIndex->tracks = tracks;
	pRoomIndex->tracks->next = NULL;
	pRoomIndex->tracks->ch = NULL;
	pRoomIndex->tracks->direction = 0;

	if (pRoomIndex->progtypes & RAND_PROG)
	{
	    if (room_rand_first)
		pRoomIndex->next_rand = room_rand_first;
	    room_rand_first = pRoomIndex;
	}

    }
}

/*
 * Snarf a guildmaster section.
 */
void load_gms( FILE *fp )
{
//    GM_DATA *pGm;
    MOB_INDEX_DATA *pMobIndex;
    char x;
    char *sk;
    int sn;
    int vnum;

	pMobIndex = NULL;

    for ( ; ; )
    {
	GM_DATA 	*newGm;

	x 			= fread_letter(fp);
	switch(x)
	{
	case '#':
		vnum		= fread_number( fp );
		if (vnum == 0)
			{
			fread_to_eol(fp);
			}
		else
			{
			pMobIndex 	= get_mob_index(vnum);
			pMobIndex->gm	= NULL;
			}
		break;
	case 'S':
		fread_to_eol(fp);
		return;
	default:
		ungetc(x, fp);
		newGm		= (GM_DATA*)alloc_perm( sizeof(*newGm) );
		g_gm_count++;
		sk 		= fread_word( fp );
		sn   		= skill_lookup_full( sk );
		newGm->sn	= sn;
		if (pMobIndex->gm != NULL)
			newGm->next	= pMobIndex->gm;
		else
			newGm->next	= NULL;
		pMobIndex->gm	= newGm;
		break;
	}
    }

    return;
}

/*
 * Snarf a shop section.
 */
void load_shops( FILE *fp )
{
    SHOP_DATA *pShop;
    char c;

    for ( ; ; )
    {
	MOB_INDEX_DATA *pMobIndex;
	int iTrade;

	pShop			= (SHOP_DATA*)alloc_perm( sizeof(*pShop) );
	pShop->keeper		= fread_number( fp );
	if ( pShop->keeper == 0 )
	    break;
	for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
	    pShop->buy_type[iTrade]	= fread_number( fp );
	pShop->profit_buy	= fread_number( fp );
	pShop->profit_sell	= fread_number( fp );
	pShop->open_hour	= fread_number( fp );
	pShop->close_hour	= fread_number( fp );

	/* Hack for compatibility with older shop files without max_buy */
	c = getc(fp);
	if (isdigit(c))
	{
	    ungetc(c, fp);
	    pShop->max_buy = fread_number(fp);
	}
	else
	    ungetc(c, fp);

	fread_to_eol( fp );
	pMobIndex		= get_mob_index( pShop->keeper );
	pMobIndex->pShop	= pShop;

	if ( shop_first == NULL )
	    shop_first = pShop;
	if ( shop_last  != NULL )
	    shop_last->next = pShop;

	shop_last	= pShop;
	pShop->next	= NULL;
	top_shop++;
    }

    return;
}


/*
 * Snarf spec proc declarations.
 */
void load_specials( FILE *fp )
{
    for ( ; ; )
    {
	MOB_INDEX_DATA *pMobIndex;
	char letter;

	switch ( letter = fread_letter( fp ) )
	{
	default:
	    bug( "Load_specials: letter '%c' not *MS.", letter );
	    exit( 1 );

	case 'S':
	    return;

	case '*':
	    break;

	case 'M':
	    pMobIndex		= get_mob_index	( fread_number ( fp ) );
	    pMobIndex->spec_fun	= spec_lookup	( fread_word   ( fp ) );
	    if ( pMobIndex->spec_fun == 0 )
	    {
		bug( "Load_specials: 'M': vnum %d.", pMobIndex->vnum );
		exit( 1 );
	    }
	    break;
	}

	fread_to_eol( fp );
    }
}

void assign_herbs( void )
{
    ROOM_INDEX_DATA *pRoomIndex = NULL;
    AREA_DATA *pArea;
    int herb_rare[SECT_MAX];
    int i, j, rnum;

    for (pArea = area_first; pArea; pArea = pArea->next)
    {
        for (i = 0; i < SECT_MAX; i++)
	    herb_rare[i] = 0;

	for (i = 0; herb_table[i].name; i++)
	    if (IS_SET(pArea->herbs, herb_table[i].bit))
		for (j = 0; j < SECT_MAX; j++)
		    if (herb_table[i].sector_type[j])
			herb_rare[j] += herb_table[i].rarity;

	for (i = 0; i < MAX_KEY_HASH; i++)
	    for ( pRoomIndex = room_index_hash[i];
		  pRoomIndex;
		  pRoomIndex = pRoomIndex->next )
	    if (pRoomIndex->area == pArea)
	    {
		pRoomIndex->herb_type = -1;    /* just to make sure */

		if ((pRoomIndex->sector_type >= 0) && (pRoomIndex->sector_type < SECT_MAX) && herb_rare[pRoomIndex->sector_type] > 0)
		{
		    rnum = number_range(1, herb_rare[pRoomIndex->sector_type]);

		    for (j = 0; herb_table[j].name; j++)
		    {
		        if (herb_table[j].sector_type[pRoomIndex->sector_type])
			    rnum -= herb_table[j].rarity;
			if (rnum <= 0)
			{
			    pRoomIndex->herb_type = j;
			    break;
			}
		    } 
		}
	    }
    }
}


void make_alinks( void )
{
    ROOM_INDEX_DATA *pRoomIndex = NULL;
    ALINK_DATA *aLink = NULL, *pLink = NULL;
    int iHash, door = 0;
    bool fLink = FALSE;
    extern const int rev_dir [];


    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
	for ( pRoomIndex  = room_index_hash[iHash];
	      pRoomIndex != NULL;
	      pRoomIndex  = pRoomIndex->next )
        {
	    for (door = 0; door <= 5; door++)
	    {
		if (pRoomIndex->exit[door] && !pRoomIndex->exit[door]->u1.to_room)
		{
		    bug("make_alinks: room %d bad exit", pRoomIndex->vnum);
		    exit(1);
		}

		if (pRoomIndex->exit[door]
		 && (pRoomIndex->exit[door]->u1.to_room->area != pRoomIndex->area)
		 && pRoomIndex->exit[door]->u1.to_room->exit[rev_dir[door]]
		 && (pRoomIndex->exit[door]->u1.to_room->exit[rev_dir[door]]->u1.to_room->area == pRoomIndex->area))
		{
		    if (alink_first)
		    {
			pLink = alink_first;
			fLink = FALSE;
			while (pLink)
			{
			    if (((pLink->a1 == pRoomIndex->area)
                              || ((pLink->a1 == pRoomIndex->exit[door]->u1.to_room->area) && (pLink->dir1 == door)))
			     && ((pLink->a2 == pRoomIndex->area)
			      || ((pLink->a2 == pRoomIndex->exit[door]->u1.to_room->area) && (pLink->dir2 == door))))
			    {
				fLink = TRUE;
				break;
			    }
			    pLink = pLink->next;
			}
		
			if (!fLink)
			{
			    aLink->next = (ALINK_DATA*)alloc_perm(sizeof(*pLink));
			    top_alink++;
			    aLink = aLink->next;
			    aLink->a1 = pRoomIndex->area;
			    aLink->a2 = pRoomIndex->exit[door]->u1.to_room->area;
			    aLink->dir1 = rev_dir[door];
			    aLink->dir2 = door;
		        }
		    }
		    else
		    {
			alink_first = (ALINK_DATA*)alloc_perm(sizeof(*aLink));
			top_alink++;
			aLink = alink_first;
			aLink->a1 = pRoomIndex->area;
			aLink->a2 = pRoomIndex->exit[door]->u1.to_room->area;
			aLink->dir1 = rev_dir[door];
			aLink->dir2 = door;
		    }
		}
	    }
	}
    }
}

/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits( void )
{
    extern const int rev_dir [];
    ROOM_INDEX_DATA *pRoomIndex;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    EXIT_DATA *pexit_rev;
    int iHash;
    int door;

    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
	for ( pRoomIndex  = room_index_hash[iHash];
	      pRoomIndex != NULL;
	      pRoomIndex  = pRoomIndex->next )
	{
	    bool fexit;

	    fexit = FALSE;
	    for ( door = 0; door <= 5; door++ )
	    {
		if ( ( pexit = pRoomIndex->exit[door] ) != NULL )
		{
		    if ( (pexit->u1.vnum <= 0) 
		    || (get_room_index(pexit->u1.vnum) == NULL))
			pexit->u1.to_room = NULL;
		    else
		    {
		   	fexit = TRUE; 
			pexit->u1.to_room = get_room_index( pexit->u1.vnum );
		    }
		}
	    }
	    if (!fexit)
		SET_BIT(pRoomIndex->room_flags,ROOM_NO_MOB);
	}
    }

    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
	for ( pRoomIndex  = room_index_hash[iHash];
	      pRoomIndex != NULL;
	      pRoomIndex  = pRoomIndex->next )
	{
	    for ( door = 0; door <= 5; door++ )
	    {
		if ( ( pexit     = pRoomIndex->exit[door]       ) != NULL
		&&   ( to_room   = pexit->u1.to_room            ) != NULL
		&&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
		&&   pexit_rev->u1.to_room != pRoomIndex 
		&&   (pRoomIndex->vnum < 1200 || pRoomIndex->vnum > 1299))
		{
		    /* sprintf( buf, "Fix_exits: %d:%d -> %d:%d -> %d.",
			pRoomIndex->vnum, door,
			to_room->vnum,    rev_dir[door],
			(pexit_rev->u1.to_room == NULL)
			    ? 0 : pexit_rev->u1.to_room->vnum );
		    bug( buf, 0 );*/
		}
	    }
	}
    }

    return;
}


void pillage_update( void )
{
    CHAR_DATA *raider;
    AREA_DATA *pArea;
    ROOM_INDEX_DATA *pRoom;
    AFFECT_DATA *paf;
    int level;
    char buf[MAX_STRING_LENGTH];

    pRoom = NULL;
    for ( pArea = area_first; pArea != NULL; pArea = pArea->next )
    {
	for (paf = pArea->affected;paf;paf = paf->next)
	{
	    if (paf->type != gsn_pillage)
	        continue;
	    
	    if (paf->duration < 5)
	        continue;
  
  	    while (!pRoom || pRoom->area != pArea)
	    {
	        pRoom = get_random_room_no_char();
	
	        if (pRoom && pRoom->area == pArea)
	        {
		    raider = create_mobile(get_mob_index(number_range(MOB_VNUM_PILLAGE_BANDIT_MIN, MOB_VNUM_PILLAGE_BANDIT_MAX+1)));
		    char_to_room(raider, pRoom);

		    if (raider->pIndexData->vnum == MOB_VNUM_PILLAGE_BANDIT_MAX+1)
		        act("A brand of fire is thrown from out of sight, and a blaze erupts!", raider, NULL, NULL, TO_ROOM);
		    else
		        act("With a howl of fury, $n comes out of nowhere, ready for blood and war!", raider, NULL, NULL, TO_ROOM);
		
		    if (raider->pIndexData->vnum == MOB_VNUM_PILLAGE_BANDIT_MAX)
		    {
		        free_string(raider->description);
		        sprintf(buf,raider->pIndexData->description,race_table[paf->modifier].name);
		        raider->description = strdup(buf);
		    }

		    level = number_range(10, 35);
		    raider->level = level;
		    raider->damroll = (level*2)/3;
		    raider->hitroll = (level*2)/3;
		    raider->damage[0] = level/5;
		    raider->damage[1] = 5;
		    raider->damage[2] = level/10;
		    raider->hit	  = level * 20;
		    raider->max_hit   = level * 20;
	        }
	    }
	}
    }
}


/*
 * Repopulate areas periodically.
 */
void area_update( void )
{
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];
    int silver_area = top_area;

    for ( pArea = area_first; pArea != NULL; pArea = pArea->next )
    {
        if ((silver_state == SILVER_FINAL) && (silver_area > 0))
        {
            if (number_range(1, silver_area) == 1)
            {
            CHAR_DATA *ch;
            int ptype;

            for (ch = char_list; ch; ch = ch->next)
                if (ch->in_room && (ch->in_room->area == pArea))
                {
                    send_to_char("A wave of sickness passes through the area.\n\r", ch);
                    ptype = number_range(1,3);
                    switch(ptype)
                    {
                        case 1:
                        spell_plague(gsn_plague, ch->level, ch, (void *) ch, TARGET_CHAR);
                        break;
                        case 2:
                        spell_pox(gsn_pox, ch->level, ch, (void *) ch, TARGET_CHAR);
                        break;
                        case 3:
                        spell_plague_madness(gsn_plague_madness, ch->level, ch, (void *) ch, TARGET_CHAR);
                        break;
                    }
                }

            silver_area = 1;
            }

            silver_area--;
        }

        if ( ++pArea->age < 3 )
            continue;

        /*
         * Check age and reset.
         * Note: Mud School resets every 3 minutes (not 15).
         */
        if ( (!pArea->empty && (pArea->nplayer == 0 || pArea->age >= 15))
        ||    pArea->age >= 31)
        {
            ROOM_INDEX_DATA *pRoomIndex;

            reset_area( pArea );
            sprintf(buf,"%s has just been reset.",pArea->name);
            wiznet(buf,NULL,NULL,WIZ_RESETS,0,0);
        
            pArea->age = number_range( 0, 3 );
            pRoomIndex = get_room_index( 8500 ); /* vnum in tower of testing */
            if ( pRoomIndex != NULL && pArea == pRoomIndex->area )
            pArea->age = 15 - 2;
            else if (pArea->nplayer == 0) 
            pArea->empty = TRUE;
        }
    }
}

/* OLC
 * Reset one room.  Called by reset_area and olc.
 */
void reset_room(ROOM_INDEX_DATA *pRoom, bool ignorePlayerPresence)
{
    RESET_DATA  *pReset;
    CHAR_DATA   *pMob;
    CHAR_DATA	*mob;
    OBJ_DATA    *pObj;
    CHAR_DATA   *LastMob = NULL;
    OBJ_DATA    *LastObj = NULL;
    int iExit;
    int level = 0;
    bool last;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    const unsigned long ex_reset = EX_ISDOOR|EX_CLOSED|EX_LOCKED;

    if ( !pRoom )
        return;

    pMob        = NULL;
    last        = FALSE;
    
    for ( iExit = 0;  iExit < MAX_DIR;  iExit++ )
    {
        EXIT_DATA *pExit;
        if ((pExit = pRoom->exit[iExit])
	 && !IS_SET(pExit->exit_info, EX_NOREFRESH))
        {
	    pExit->exit_info ^= ((pExit->exit_info & ex_reset)^(pExit->rs_flags & ex_reset));
            //pExit->exit_info = pExit->rs_flags;

            if ( ( pExit->u1.to_room != NULL )
              && ( ( pExit = pExit->u1.to_room->exit[rev_dir[iExit]] ) ) )
            {
		pExit->exit_info ^= ((pExit->exit_info & ex_reset)^(pExit->rs_flags & ex_reset));
		//pExit->exit_info = pExit->rs_flags;
            }
        }
    }

    for ( pReset = pRoom->reset_first; pReset != NULL; pReset = pReset->next )
    {
        MOB_INDEX_DATA  *pMobIndex;
        OBJ_INDEX_DATA  *pObjIndex;
        OBJ_INDEX_DATA  *pObjToIndex;
        ROOM_INDEX_DATA *pRoomIndex;
	char buf[MAX_STRING_LENGTH];
	int count,limit=0;

        switch ( pReset->command )
        {
        default:
                bug( "Reset_room: bad command %c.", pReset->command );
                break;

        case 'M':
            if ( !( pMobIndex = get_mob_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'M': bad vnum %d.", pReset->arg1 );
                continue;
            }

	    if ( ( pRoomIndex = get_room_index( pReset->arg3 ) ) == NULL )
	    {
		bug( "Reset_area: 'R': bad vnum %d.", pReset->arg3 );
		continue;
	    }
	    if (pMobIndex->count < 0)
		pMobIndex->count = 0;
            if ( pMobIndex->count >= pReset->arg2 )
            {
                last = FALSE;
                break;
            }

	    count = 0;
	    for (mob = pRoomIndex->people; mob != NULL; mob = mob->next_in_room)
		if (mob->pIndexData == pMobIndex)
		{
		    count++;
		    if (count >= pReset->arg4)
		    {
		    	last = FALSE;
		    	break;
		    }
		}

	    if (count >= pReset->arg4)
		break;

            pMob = create_mobile( pMobIndex );

            /*
             * Some more hard coding.
             */
            if ( room_is_dark( pRoom ) )
                SET_BIT(pMob->affected_by, AFF_INFRARED);

	    if (IS_SET(pMob->affected_by, AFF_HIDE) && ((pRoom->sector_type == SECT_FOREST) || (pRoom->sector_type == SECT_HILLS) || (pRoom->sector_type == SECT_SWAMP) || (pRoom->sector_type == SECT_MOUNTAIN)))
	    {
	 	REMOVE_BIT(pMob->affected_by, AFF_HIDE);
		af.where     = TO_AFFECTS;
		af.type	     = gsn_camouflage;
		af.level     = pMob->level;
		af.location  = 0;
		af.duration  = -1;
		af.modifier  = 0;
		af.bitvector = 0;
		affect_to_char(pMob, &af);
	    }

            /*
             * Pet shop mobiles get ACT_PET set.
             */
            {
                ROOM_INDEX_DATA *pRoomIndexPrev;

                pRoomIndexPrev = get_room_index( pRoom->vnum - 1 );
                if ( pRoomIndexPrev
                    && IS_SET( pRoomIndexPrev->room_flags, ROOM_PET_SHOP ) )
                    SET_BIT( pMob->act, ACT_PET);
            }

            char_to_room( pMob, pRoom );
	    pMob->clan = pMob->in_room->clan;
	    pMob->reset = pReset;
	    mprog_load_trigger(pMob);

            LastMob = pMob;
            level  = URANGE( 0, pMob->level - 2, LEVEL_HERO - 1 ); /* -1 ROM */
            last = TRUE;
            break;

        case 'O':
            if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'O' 1 : bad vnum %d", pReset->arg1 );
                sprintf (buf,"%d %d %d %d",pReset->arg1, pReset->arg2, pReset->arg3,
                pReset->arg4 );
		bug(buf,1);
                continue;
            }

            if ( !( pRoomIndex = get_room_index( pReset->arg3 ) ) )
            {
                bug( "Reset_room: 'O' 2 : bad vnum %d.", pReset->arg3 );
                sprintf (buf,"%d %d %d %d",pReset->arg1, pReset->arg2, pReset->arg3,
                pReset->arg4 );
		bug(buf,1);
                continue;
            }

            if ((!ignorePlayerPresence && pRoom->area->nplayer > 0)
              || count_obj_list( pObjIndex, pRoom->contents ) > 0 )
	    {
		last = FALSE;
		break;
	    }

		/* Don't load an = or overlimit object */
	    if ((pObjIndex->limit_factor > 0) 
             && (pObjIndex->current >= pObjIndex->limit))
		break;

            pObj = create_object( pObjIndex,              /* UMIN - ROM OLC */
				  UMIN(number_fuzzy( level ), LEVEL_HERO -1) );
            pObj->cost = 0;
            obj_to_room( pObj, pRoom );
	    oprog_load_trigger(pObj);
	    last = TRUE;
            break;

        case 'P':
            if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'P': bad vnum %d.", pReset->arg1 );
                continue;
            }

            if ( !( pObjToIndex = get_obj_index( pReset->arg3 ) ) )
            {
                bug( "Reset_room: 'P': bad vnum %d.", pReset->arg3 );
                continue;
            }
		

	    /* Don't load an = or overlimit object */
	    if ((pObjIndex->limit > 0) 
             && (pObjIndex->current >= pObjIndex->limit))
		break;
/*
	    if ( (pObjIndex->current >= pObjIndex->limit) && (pObjIndex->limit > 0) )
            {
	    break;
	    }
*/

            if (pReset->arg2 > 50) /* old format */
                limit = pReset->arg2;
            else if (pReset->arg2 == -1) /* no limit */
                limit = 999;
            else
                limit = pReset->arg2;

            if ((!ignorePlayerPresence && pRoom->area->nplayer > 0)
              || ( LastObj = get_obj_type( pObjToIndex ) ) == NULL
              || ( LastObj->in_room == NULL && !last)
              || ( pObjIndex->count >= limit /* && number_range(0,4) != 0 */ )
              || ( count = count_obj_list( pObjIndex, LastObj->contains ) ) > pReset->arg4  )
	    {
		last = FALSE;
		break;
	    }
				                /* lastObj->level  -  ROM */

	    while (count < pReset->arg4)
	    {
            pObj = create_object( pObjIndex, number_fuzzy( LastObj->level ) );
            obj_to_obj( pObj, LastObj );
		count++;
		if (pObjIndex->count >= limit)
		    break;
	    }

	    /* fix object lock state! */
	    LastObj->value[1] = LastObj->pIndexData->value[1];
	    last = TRUE;
            break;

        case 'G':
        case 'E':
            if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'E' or 'G': bad vnum %d.", pReset->arg1 );
                continue;
            }

            if ( !last )
                break;

            if ( !LastMob )
            {
                bug( "Reset_room: 'E' or 'G': null mob for vnum %d.",
                    pReset->arg1 );
                last = FALSE;
                break;
            }
		
	    /* Don't load an = or overlimit object */
	    if ((pObjIndex->limit > 0) 
             && ( /* fBootDb || */ (pObjIndex->current >= pObjIndex->limit)))
		break;
/*
	    if ( (pObjIndex->current >= pObjIndex->limit) && (pObjIndex->limit > 0) )
            {
	    break;
	    }
*/
            if ( LastMob->pIndexData->pShop )   /* Shop-keeper? */
            {
                int olevel=0,i,j;

		if (!pObjIndex->new_format)
                 switch ( pObjIndex->item_type )
                {
                default:                olevel = 0;                      break;
                case ITEM_PILL:
                case ITEM_POTION:
                case ITEM_SCROLL:
		case ITEM_OIL:
 		    olevel = 53;
		    for (i = 1; i < 5; i++)
		    {
			if (pObjIndex->value[i] > 0)
			{
		    	    for (j = 0; j < MAX_CLASS; j++)
			    {
				olevel = UMIN(olevel,
				         skill_table[pObjIndex->value[i]].
						     skill_level[j]);
			    }
			}
		    }
		   
		    olevel = UMAX(0,(olevel * 3 / 4) - 2);
		    break;
		    
                case ITEM_WAND:         olevel = number_range( 10, 20 ); break;
                case ITEM_STAFF:        olevel = number_range( 15, 25 ); break;
                case ITEM_ARMOR:        olevel = number_range(  5, 15 ); break;
                /* ROM patch weapon, treasure */
		case ITEM_WEAPON:       olevel = number_range(  5, 15 ); break;
		case ITEM_TREASURE:     olevel = number_range( 10, 20 ); break;

#if 0 /* envy version */
                case ITEM_WEAPON:       if ( pReset->command == 'G' )
                                            olevel = number_range( 5, 15 );
                                        else
                                            olevel = number_fuzzy( level );
#endif /* envy version */

                  break;
                }

                pObj = create_object( pObjIndex, olevel );
		SET_BIT( pObj->extra_flags[0], ITEM_INVENTORY );  /* ROM OLC */

#if 0 /* envy version */
                if ( pReset->command == 'G' )
                    SET_BIT( pObj->extra_flags[0], ITEM_INVENTORY );
#endif /* envy version */

            }
	    else   /* ROM OLC else version */
	    {
		int limit;
		if (pReset->arg2 > 50 )  /* old format */
		    limit = pReset->arg2;
		else if ( pReset->arg2 == -1 || pReset->arg2 == 0 )  /* no limit */
		    limit = 999;
		else
		    limit = pReset->arg2;

		if ( pObjIndex->count < limit || number_range(0,4) == 0 )
		{
		    pObj = create_object( pObjIndex, 
			   UMIN( number_fuzzy( level ), LEVEL_HERO - 1 ) );
		    /* error message if it is too high */
/*		    if (pObj->level > LastMob->level + 3
		    ||  (pObj->item_type == ITEM_WEAPON 
		    &&   pReset->command == 'E' 
		    &&   pObj->level < LastMob->level -5 && pObj->level < 45))
			fprintf(stderr,
			    "Err: obj %s (%d) -- %d, mob %s (%d) -- %d\n",
			    pObj->short_descr,pObj->pIndexData->vnum,pObj->level,
			    LastMob->short_descr,LastMob->pIndexData->vnum,LastMob->level);
*/
		}
		else
		    break;
	    }
									 
#if 0 /* envy else version */
            else
            {
                pObj = create_object( pObjIndex, number_fuzzy( level ) );
            }
#endif /* envy else version */

            obj_to_char( pObj, LastMob );

	    if (pReset->command == 'E')
		wear_obj(LastMob, pObj, TRUE, TRUE, FALSE);

            last = TRUE;
            break;

        case 'D':
            break;

        case 'R':
            if ( !( pRoomIndex = get_room_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'R': bad vnum %d.", pReset->arg1 );
                continue;
            }

            {
                EXIT_DATA *pExit;
                int d0;
                int d1;

                for ( d0 = 0; d0 < pReset->arg2 - 1; d0++ )
                {
                    d1                   = number_range( d0, pReset->arg2-1 );
                    pExit                = pRoomIndex->exit[d0];
                    pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
                    pRoomIndex->exit[d1] = pExit;
                }
            }
            break;
        }
    }

    return;
}

/* OLC
 * Reset one area.
 */
void reset_area( AREA_DATA *pArea )
{
    ROOM_INDEX_DATA *pRoom;
    int vnum;
    VNUM_RANGE *vrange;

    for (vrange = pArea->vnums; vrange; vrange = vrange->next)
        for ( vnum = vrange->min_vnum; vnum <= vrange->max_vnum; vnum++ )
        {
            if ( ( pRoom = get_room_index(vnum) ) )
	    {
                reset_room(pRoom, false);
	        if (!rip_process)
	            mob_obj_refresh(pRoom, NULL);
	    }
        }

    return;
}

void mob_obj_refresh( ROOM_INDEX_DATA *pRoom, CHAR_DATA *target )
{
    CHAR_DATA *vch;
    RESET_DATA *pReset;
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *pObj, *cObj;
    bool oFound;

    for (vch = pRoom->people; vch; vch = vch->next_in_room )
    {
	if (!IS_NPC(vch) || !vch->reset || !vch->reset->next || (target && (vch != target)))
	    continue;

	if (number_bits(3) != 0)
	    continue;

	for (pReset = vch->reset->next; pReset; pReset = pReset->next)
	{
	    if ((pReset->command == 'E') || (pReset->command == 'G'))
	    {
		if (!(pObjIndex = get_obj_index(pReset->arg1)))
		    continue;

		if ((pObjIndex->limit > 0)
		 && (pObjIndex->current >= pObjIndex->limit))
		    continue;

		oFound = FALSE;
		for (cObj = vch->carrying; cObj; cObj = cObj->next_content)
		    if (cObj->pIndexData == pObjIndex)
			oFound = TRUE;

		if (!oFound)
		{	
		    if ((pReset->arg2 < 1) 
		     || (pObjIndex->count < pReset->arg2)
		     || (number_range(0, 4) == 0))
		        pObj = create_object(pObjIndex, 
				UMIN(number_fuzzy(URANGE(0, vch->level - 2, LEVEL_HERO - 1)), LEVEL_HERO - 1));
		    else
		        continue;

		    obj_to_char(pObj, vch);
		    if (pReset->command == 'E')
			wear_obj(vch, pObj, TRUE, TRUE, FALSE);
		}

		continue;
	    }

	    if (pReset->command == 'M')
		break;
	}
    }
}		    

/*
 * Create an instance of a mobile.
 */
CHAR_DATA *create_mobile( MOB_INDEX_DATA *pMobIndex )
{
    CHAR_DATA *mob;
    int i;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_INPUT_LENGTH];
 
    mobile_count++;

    if ( pMobIndex == NULL )
    {
	bug( "Create_mobile: NULL pMobIndex.", 0 );
	return NULL;
    }

    mob = new_char();

    mob->pIndexData	= pMobIndex;

    mob->skilled	= FALSE;
    setName(*mob, pMobIndex->player_name );    /* OLC */
    mob->short_descr	= str_dup( pMobIndex->short_descr );    /* OLC */
    mob->long_descr	= str_dup( pMobIndex->long_descr  );    /* OLC */
    mob->description	= str_dup( pMobIndex->description );    /* OLC */
    setFakeName(*mob, pMobIndex->player_name);
    mob->dam_verb	= str_dup( pMobIndex->dam_verb    );
 
    one_argument(pMobIndex->player_name, buf);
    sprintf(buf, "%s_%d_%d", buf, pMobIndex->vnum, pMobIndex->total_count + 1);
    setUniqueName(*mob, buf);

    mob->spec_fun	= pMobIndex->spec_fun;
    mob->familiar	= 0;
    mob->id		= ++last_mob_id;
    mob->prompt		= NULL;
    mob->mobvalue[0]	= 0;
    mob->mobvalue[1]	= 0;
    mob->mobvalue[2]	= 0;
    mob->mobvalue[3]	= 0;
    mob->mobvalue[4]	= 0;
    mob->mobvalue[5]	= 0;
    mob->mobvalue[6]	= 0;
    mob->mobvalue[7]	= 0;
    mob->mobvalue[8]	= 0;
    mob->mobvalue[9]	= 0;
    mob->pose = NULL;
    for (i = 0; i < MAX_MOBFOCUS; i++)
	mob->memfocus[i]	= NULL;

    mob->lang_flags	= pMobIndex->lang_flags;
    mob->speaking	= race_table[pMobIndex->race].native_tongue;

    if (pMobIndex->wealth == 0)
    {
	for (i = 0; i < MAX_COIN; i++)
	    mob->coins[i] = 0;
    }
    else
    {
	float wealth;

	wealth = (float) (pMobIndex->wealth * FACTOR_Y * pow(2, pMobIndex->level / 8.3) / 100);
	wealth = wealth * number_range(8,12) / 10;

	inc_player_coins(mob, value_to_coins(wealth, TRUE));
    } 

    if (pMobIndex->new_format)
    /* load in new style */
    {
	/* read from prototype */
 	mob->group		= pMobIndex->group;
	mob->act 		= pMobIndex->act;
	mob->nact		= pMobIndex->nact;
	mob->comm		= 0;
	mob->affected_by	= pMobIndex->affected_by;
	switch (pMobIndex->alignment)
	{
	    case ALIGN_GOOD:	mob->alignment	= 750;	break;
	    case ALIGN_NEUTRAL:	mob->alignment	= 0;	break;
	    case ALIGN_EVIL:	mob->alignment	= -750;	break;
	    case ALIGN_RANDOM:	
            switch (number_range(0, 2))
            {
                case 0: mob->alignment = 750; break;
                case 1: mob->alignment = -750; break;
                default: mob->alignment = 0; break;
            }
            break;
	    default:
		bug ("Bad alignment value, mob index %d.", pMobIndex->vnum);
		mob->alignment = 0;
		break;
	}
	mob->level		= pMobIndex->level;
	mob->hitroll		= pMobIndex->hitroll;
	mob->damroll		= pMobIndex->damage[DICE_BONUS];
	mob->max_hit		= dice(pMobIndex->hit[DICE_NUMBER],
				       pMobIndex->hit[DICE_TYPE])
				  + pMobIndex->hit[DICE_BONUS];
	mob->hit		= mob->max_hit;
	mob->max_mana		= dice(pMobIndex->mana[DICE_NUMBER],
				       pMobIndex->mana[DICE_TYPE])
				  + pMobIndex->mana[DICE_BONUS];
	mob->mana		= mob->max_mana;
	mob->damage[DICE_NUMBER]= pMobIndex->damage[DICE_NUMBER];
	mob->damage[DICE_TYPE]	= pMobIndex->damage[DICE_TYPE];
	mob->dam_type		= pMobIndex->dam_type;
        if (mob->dam_type == 0)
    	    switch(number_range(1,3))
            {
                case (1): mob->dam_type = 3;        break;  /* slash */
                case (2): mob->dam_type = 7;        break;  /* pound */
                case (3): mob->dam_type = 11;       break;  /* pierce */
            }
	for (i = 0; i < 4; i++)
	    mob->armor[i]	= pMobIndex->ac[i]; 
	mob->off_flags		= pMobIndex->off_flags;
	mob->imm_flags		= pMobIndex->imm_flags;
	mob->res_flags		= pMobIndex->res_flags;
	mob->vuln_flags		= pMobIndex->vuln_flags;
	mob->start_pos		= pMobIndex->start_pos;
	mob->default_pos	= pMobIndex->default_pos;
	mob->sex		= pMobIndex->sex;
        if (mob->sex == 3) /* random sex */
            mob->sex = number_range(1,2);
	mob->race		= pMobIndex->race;
	mob->class_num		= pMobIndex->class_num;
	mob->form		= pMobIndex->form;
	mob->parts		= pMobIndex->parts;
	mob->size		= pMobIndex->size;
	mob->material		= MATERIAL_FLESH;

	/* computed on the spot */

    	for (i = 0; i < MAX_STATS; i ++)
            mob->perm_stat[i] = UMIN(25,11 + mob->level/4);
            
        if (IS_SET(mob->act,ACT_WARRIOR))
        {
            mob->perm_stat[STAT_STR] += 3;
            mob->perm_stat[STAT_INT] -= 1;
            mob->perm_stat[STAT_CON] += 2;
        }
        
        if (IS_SET(mob->act,ACT_THIEF))
        {
            mob->perm_stat[STAT_DEX] += 3;
            mob->perm_stat[STAT_INT] += 1;
            mob->perm_stat[STAT_WIS] -= 1;
        }
        
        if (IS_SET(mob->off_flags,OFF_FAST))
            mob->perm_stat[STAT_DEX] += 2;
            
        mob->perm_stat[STAT_STR] += mob->size - SIZE_MEDIUM;
        mob->perm_stat[STAT_CON] += (mob->size - SIZE_MEDIUM) / 2;

	/* let's get some spell action */
	if (IS_AFFECTED(mob,AFF_SANCTUARY))
	{
	    af.where	 = TO_AFFECTS;
	    af.type      = skill_lookup("sanctuary");
	    af.level     = mob->level;
	    af.duration  = -1;
	    af.location  = APPLY_NONE;
	    af.modifier  = 0;
	    af.bitvector = AFF_SANCTUARY;
	    affect_to_char( mob, &af );
	}

	if (IS_AFFECTED(mob,AFF_HASTE))
	{
	    af.where	 = TO_AFFECTS;
	    af.type      = skill_lookup("haste");
    	    af.level     = mob->level;
      	    af.duration  = -1;
    	    af.location  = APPLY_DEX;
    	    af.modifier  = 1 + (mob->level >= 18) + (mob->level >= 25) + 
			   (mob->level >= 32);
    	    af.bitvector = AFF_HASTE;
    	    affect_to_char( mob, &af );
	}

	if (IS_AFFECTED(mob,AFF_PROTECT_EVIL))
	{
	    af.where	 = TO_AFFECTS;
	    af.type	 = skill_lookup("protection evil");
	    af.level	 = mob->level;
	    af.duration	 = -1;
	    af.location	 = APPLY_SAVES;
	    af.modifier	 = -1;
	    af.bitvector = AFF_PROTECT_EVIL;
	    affect_to_char(mob,&af);
	}

        if (IS_AFFECTED(mob,AFF_PROTECT_GOOD))
        {
	    af.where	 = TO_AFFECTS;
            af.type      = skill_lookup("protection good");
            af.level     = mob->level;
            af.duration  = -1;
            af.location  = APPLY_SAVES;
            af.modifier  = -1;
            af.bitvector = AFF_PROTECT_GOOD;
            affect_to_char(mob,&af);
        }
	
	/*demon shit*/
	mob->demonstate = 0;
	mob->demontrack = NULL;
	if (IS_AFFECTED(mob,AFF_WIZI))
	    mob->invis_level = mob->level;
    }
    else /* read in old format and convert */
    {
	mob->act		= pMobIndex->act;
	mob->affected_by	= pMobIndex->affected_by;
	switch (pMobIndex->alignment)
	{
	    case ALIGN_GOOD:	mob->alignment	= 750;	break;
	    case ALIGN_NEUTRAL:	mob->alignment	= 0;	break;
	    case ALIGN_EVIL:	mob->alignment	= -750;	break;
        case ALIGN_RANDOM:	
            switch (number_range(0, 2))
            {
                case 0: mob->alignment = 750; break;
                case 1: mob->alignment = -750; break;
                default: mob->alignment = 0; break;
            }
            break;

	    default:
		bug ("Bad alignment value, mob index %d.", pMobIndex->vnum);
		mob->alignment = 0;
		break;
	}
	mob->level		= pMobIndex->level;
	mob->hitroll		= pMobIndex->hitroll;
	mob->damroll		= 0;
	mob->max_hit		= mob->level * 8 + number_range(
					mob->level * mob->level/4,
					mob->level * mob->level);
	mob->max_hit = mob->max_hit * 9 / 10;
	mob->hit		= mob->max_hit;
	mob->max_mana		= 100 + dice(mob->level,10);
	mob->mana		= mob->max_mana;
	switch(number_range(1,3))
	{
	    case (1): mob->dam_type = 3; 	break;  /* slash */
	    case (2): mob->dam_type = 7;	break;  /* pound */
	    case (3): mob->dam_type = 11;	break;  /* pierce */
	}
	for (i = 0; i < 3; i++)
	    mob->armor[i]	= interpolate(mob->level,100,-100);
	mob->armor[3]		= interpolate(mob->level,100,0);
	mob->race		= pMobIndex->race;
	mob->off_flags		= pMobIndex->off_flags;
	mob->imm_flags		= pMobIndex->imm_flags;
	mob->res_flags		= pMobIndex->res_flags;
	mob->vuln_flags		= pMobIndex->vuln_flags;
	mob->start_pos		= pMobIndex->start_pos;
	mob->default_pos	= pMobIndex->default_pos;
	mob->sex		= pMobIndex->sex;
	mob->form		= pMobIndex->form;
	mob->parts		= pMobIndex->parts;
	mob->size		= SIZE_MEDIUM;
	mob->material		= MATERIAL_FLESH;
	mob->tracking		= NULL;

        for (i = 0; i < MAX_STATS; i ++)
            mob->perm_stat[i] = 11 + mob->level/4;
    }

    for (i = 0; i < MAX_RESIST; i++)
        mob->resist[i] = pMobIndex->resist[i];

    mob->position = mob->start_pos;

    SET_BIT(mob->affected_by, AFF_INFRARED);

    /* link the mob to the world list */
   mob->next		= char_list;
   char_list		= mob;

   pMobIndex->count++;
   pMobIndex->total_count++;
   return mob;
}

/* duplicate a mobile exactly -- except inventory */
void clone_mobile(CHAR_DATA *parent, CHAR_DATA *clone)
{
    int i;
    AFFECT_DATA *paf;

    if ( parent == NULL || clone == NULL || !IS_NPC(parent))
	return;
    
    /* start fixing values */ 
    setName(*clone, parent->name);
    clone->version	= parent->version;
    clone->short_descr	= str_dup(parent->short_descr);
    clone->long_descr	= str_dup(parent->long_descr);
    clone->description	= str_dup(parent->description);
    clone->group	= parent->group;
    clone->sex		= parent->sex;
    clone->class_num	= parent->class_num;
    clone->race		= parent->race;
    clone->level	= parent->level;
    clone->trust	= 0;
    clone->timer	= parent->timer;
    clone->wait		= parent->wait;
    clone->hit		= parent->hit;
    clone->max_hit	= parent->max_hit;
    clone->mana		= parent->mana;
    clone->max_mana	= parent->max_mana;
    clone->move		= parent->move;
    clone->max_move	= parent->max_move;
    *clone->coins	= *parent->coins;
    clone->exp		= parent->exp;
    clone->ep		= parent->ep;
    clone->act		= parent->act;
    clone->comm		= parent->comm;
    clone->imm_flags	= parent->imm_flags;
    clone->res_flags	= parent->res_flags;
    clone->vuln_flags	= parent->vuln_flags;
    clone->invis_level	= parent->invis_level;
    clone->affected_by	= parent->affected_by;
    clone->position	= parent->position;
    clone->practice	= parent->practice;
    clone->train	= parent->train;
    clone->saving_throw	= parent->saving_throw;
    clone->luck         = parent->luck;
    clone->alignment	= parent->alignment;
    clone->hitroll	= parent->hitroll;
    clone->damroll	= parent->damroll;
    clone->wimpy	= parent->wimpy;
    clone->form		= parent->form;
    clone->parts	= parent->parts;
    clone->size		= parent->size;
    clone->material	= parent->material;
    clone->off_flags	= parent->off_flags;
    clone->dam_type	= parent->dam_type;
    clone->start_pos	= parent->start_pos;
    clone->default_pos	= parent->default_pos;
    clone->spec_fun	= parent->spec_fun;
   
    for (i = 0; i < MAX_RESIST; i++)
	clone->resist[i] = parent->resist[i];
 
    for (i = 0; i < 4; i++)
    	clone->armor[i]	= parent->armor[i];

    for (i = 0; i < MAX_STATS; i++)
    {
	clone->perm_stat[i]	= parent->perm_stat[i];
	clone->mod_stat[i]	= parent->mod_stat[i];
    }

    for (i = 0; i < 3; i++)
	clone->damage[i]	= parent->damage[i];

    /* now add the affects */
    for (paf = parent->affected; paf != NULL; paf = paf->next)
        affect_to_char(clone,paf);

}

/*
 * Create an instance of an object.
 */
OBJ_DATA *create_object(OBJ_INDEX_DATA *pObjIndex, int level)
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;
    int i;

	/* Increment ye olde limit count -mw */
    if (!loading_char)
    	pObjIndex->current++;

    obj = new_obj();

    obj->pIndexData	= pObjIndex;
    obj->in_room	= NULL;
    obj->enchanted	= FALSE;

    if (pObjIndex->new_format)
	obj->level = pObjIndex->level;
    else
	obj->level		= UMAX(0,level);

//    obj->wear_loc	= -1;
    obj->worn_on	= 0;

    setName(*obj, pObjIndex->name );           /* OLC */
    obj->short_descr	= str_dup( pObjIndex->short_descr );    /* OLC */
    obj->description	= str_dup( pObjIndex->description );    /* OLC */
    obj->lore		= str_dup( pObjIndex->lore );
    obj->obj_str	= str_dup( pObjIndex->obj_str );
    obj->material	= pObjIndex->material;
    obj->item_type	= pObjIndex->item_type;
    obj->extra_flags[0]	= pObjIndex->extra_flags[0];
    obj->extra_flags[1] = pObjIndex->extra_flags[1];
	obj->extra_flags[2] = pObjIndex->extra_flags[2];
    obj->wear_flags	= pObjIndex->wear_flags;
    obj->value[0]	= pObjIndex->value[0];
    obj->value[1]	= pObjIndex->value[1];
    obj->value[2]	= pObjIndex->value[2];
    obj->value[3]	= pObjIndex->value[3];
    obj->value[4]	= pObjIndex->value[4];
    obj->weight		= pObjIndex->weight;
    obj->size		= pObjIndex->size;
    obj->condition	= pObjIndex->condition;
    obj->objvalue[0]	= 0;
    obj->objvalue[1]	= 0;
    obj->objvalue[2]	= 0;
    obj->objvalue[3]	= 0;
    obj->objvalue[4]	= 0;
    obj->objvalue[5]	= 0;
    obj->objvalue[6]	= 0;
    obj->objvalue[7]	= 0;
    obj->objvalue[8]	= 0;
    obj->objvalue[9]	= 0;
   
    for (i = 0; i < MAX_LASTOWNER; i++)
	obj->lastowner[i] = str_dup("");

    obj->timestamp	= current_time;

    if (level == -1 || pObjIndex->new_format)
	obj->cost	= pObjIndex->cost;
    else
    	obj->cost	= number_fuzzy( 10 )
			* number_fuzzy( level ) * number_fuzzy( level );
    
	/*
     * Mess with object properties.
     */
    switch ( obj->item_type )
    {
    default:
	bug( "create_object: vnum %d bad type.", pObjIndex->vnum );
	break;

    case ITEM_LIGHT:
	if (obj->value[2] == 999)
		obj->value[2] = -1;
	break;

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
    case ITEM_PORTAL:
	if (!pObjIndex->new_format)
	    obj->cost /= 5;
	break;

    case ITEM_TREASURE:
    case ITEM_POTIONCONTAINER:
    case ITEM_WARP_STONE:
    case ITEM_ROOM_KEY:
    case ITEM_GEM:
    case ITEM_JEWELRY:
    case ITEM_INSTRUMENT:
    case ITEM_ARROW:
    case ITEM_BOW:
    case ITEM_NET:
    case ITEM_WRITING:
	break;

    case ITEM_JUKEBOX:
	for (i = 0; i < 5; i++)
	   obj->value[i] = -1;
	break;

    case ITEM_SCROLL:
	if (level != -1 && !pObjIndex->new_format)
	    obj->value[0]	= number_fuzzy( obj->value[0] );
	break;

    case ITEM_WAND:
    case ITEM_STAFF:
	if (level != -1 && !pObjIndex->new_format)
	{
	    obj->value[0]	= number_fuzzy( obj->value[0] );
	    obj->value[1]	= number_fuzzy( obj->value[1] );
	    obj->value[2]	= obj->value[1];
	}
	if (!pObjIndex->new_format)
	    obj->cost *= 2;
	break;

    case ITEM_WEAPON:
	if (level != -1 && !pObjIndex->new_format)
	{
	    obj->value[1] = number_fuzzy( number_fuzzy( 1 * level / 4 + 2 ) );
	    obj->value[2] = number_fuzzy( number_fuzzy( 3 * level / 4 + 6 ) );
	}
	break;

    case ITEM_ARMOR:
	if (level != -1 && !pObjIndex->new_format)
	{
	    obj->value[0]	= number_fuzzy( level / 5 + 3 );
	    obj->value[1]	= number_fuzzy( level / 5 + 3 );
	    obj->value[2]	= number_fuzzy( level / 5 + 3 );
	}
	break;

    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_OIL:
	if (level != -1 && !pObjIndex->new_format)
	    obj->value[0] = number_fuzzy( number_fuzzy( obj->value[0] ) );
	break;

    case ITEM_MONEY:
	if (!pObjIndex->new_format)
	    obj->value[0]	= obj->cost;
	break;
   
    case ITEM_SPECIAL:
	if (obj->value[0] == SOBJ_CHESS)
	{
	    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

	    af.where	 = TO_OBJECT;
	    af.type      = gsn_stringdata;
	    af.level     = obj->level;
	    af.location  = APPLY_NONE;
	    af.point	 = (void *) str_dup("RNBKQBNRPPPPPPPP                                pppppppprnbkqbnr");
	    af.duration  = -1;
	    af.modifier  = 0;
	    af.bitvector = 0;
	    affect_to_obj(obj, &af);
	}

	if (obj->value[0] == SOBJ_CARDS)
	{
	    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

	    af.where	 = TO_OBJECT;
	    af.type      = gsn_stringdata;
	    af.level     = obj->level;
	    af.location  = APPLY_NONE;
	    af.point	 = (void *) str_dup("                                                    ");
	    af.duration  = -1;
	    af.modifier  = 0;
	    af.bitvector = 0;
	    affect_to_obj(obj, &af);
	}

	break;
    }
  
    for (paf = pObjIndex->affected; paf != NULL; paf = paf->next) 
	if ( paf->location == APPLY_SPELL_AFFECT )
	    affect_to_obj(obj,paf);
  
    obj->next		= object_list;
    object_list		= obj;
    pObjIndex->count++;

    if (IS_SET(pObjIndex->progtypes, RAND_PROG))
    {
        obj->next_rand = obj_rand_first;
        obj_rand_first = obj;
    }

    return obj;
}

/* duplicate an object exactly -- except contents */
void clone_object(OBJ_DATA *parent, OBJ_DATA *clone)
{
    int i;
    AFFECT_DATA *paf;
    EXTRA_DESCR_DATA *ed,*ed_new;

    if (parent == NULL || clone == NULL)
	return;

    /* start fixing the object */
    setName(*clone, parent->name);
    copy_string(clone->short_descr, parent->short_descr);
    copy_string(clone->description, parent->description);
    copy_string(clone->lore, parent->lore);
    copy_string(clone->killed_by, parent->killed_by);
    copy_string(clone->obj_str, parent->obj_str);

    clone->pIndexData     = parent->pIndexData;
    clone->item_type	  = parent->item_type;
    clone->extra_flags[0] = parent->extra_flags[0];
    clone->extra_flags[1] = parent->extra_flags[1];
    clone->extra_flags[2] = parent->extra_flags[2];
    clone->wear_flags	  = parent->wear_flags;
    clone->weight	  = parent->weight;
    clone->size       = parent->size;
    clone->cost		  = parent->cost;
    clone->level	  = parent->level;
    clone->condition	  = parent->condition;
    clone->material	  = parent->material;
    clone->timer	  = parent->timer;

    for (i = 0;  i < 5; i ++)
        clone->value[i]	= parent->value[i];

    /* affects */
    clone->enchanted	= parent->enchanted;
    for (paf = parent->affected; paf != NULL; paf = paf->next) 
        affect_to_obj(clone,paf);

    /* extended desc */
    for (ed = parent->extra_descr; ed != NULL; ed = ed->next)
    {
        ed_new                  = new_extra_descr();
        ed_new->keyword    	= str_dup( ed->keyword);
        ed_new->description     = str_dup( ed->description );
        ed_new->next           	= clone->extra_descr;
        clone->extra_descr  	= ed_new;
    }

    // Last owners
    for (i = 0; i < MAX_LASTOWNER; ++i)
        copy_string(clone->lastowner[i], parent->lastowner[i]);
}

/*
 * Clear a new character.
 */
void clear_char( CHAR_DATA *ch )
{
    static CHAR_DATA ch_zero;
    int i;

    NameMaps::Remove(*ch);

    *ch				= ch_zero;
    ch->name			= &str_empty[0];
    ch->short_descr		= &str_empty[0];
    ch->long_descr		= &str_empty[0];
    ch->description		= &str_empty[0];
    ch->prompt                  = &str_empty[0];
    ch->battlecry		= &str_empty[0];
    ch->logon			= current_time;
    ch->lasthour		= current_time;
    ch->lines			= PAGELEN;
    for (i = 0; i < 4; i++)
    	ch->armor[i]		= 100;
    ch->position		= POS_STANDING;
    ch->hit			= 20;
    ch->max_hit			= 20;
    ch->mana			= 100;
    ch->max_mana		= 100;
    ch->move			= 100;
    ch->max_move		= 100;
    ch->on			= NULL;
    for (i = 0; i < MAX_STATS; i ++)
    {
        ch->perm_stat[i] = 13; 
        ch->mod_stat[i] = 0;
    }
}

/*
 * Get an extra description from a list.
 */
char *get_extra_descr( const char *name, EXTRA_DESCR_DATA *ed )
{
    for ( ; ed != NULL; ed = ed->next )
    {
	if ( is_name( (char *) name, ed->keyword ) )
	    return ed->description;
    }
    return NULL;
}



/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA *get_mob_index( int vnum )
{
    MOB_INDEX_DATA *pMobIndex;
    if (vnum < 0)
	return NULL;

    for ( pMobIndex  = mob_index_hash[vnum % MAX_KEY_HASH];
	  pMobIndex != NULL;
	  pMobIndex  = pMobIndex->next )
    {
	if ( pMobIndex->vnum == vnum )
	    return pMobIndex;
    }

    if (fBootDb)
    {
	bug("Error finding mob during db boot: %d", vnum);
	exit( 1 );
    }

    return NULL;
}



/*
 * Translates mob virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA *get_obj_index( int vnum )
{
    OBJ_INDEX_DATA *pObjIndex;

    for ( pObjIndex  = obj_index_hash[vnum % MAX_KEY_HASH];
	  pObjIndex != NULL;
	  pObjIndex  = pObjIndex->next )
    {
	if ( pObjIndex->vnum == vnum )
	    return pObjIndex;
    }

    if ( fBootDb )
    {
	bug( "Get_obj_index: bad vnum %d.", vnum );
	exit( 1 );
    }

    return NULL;
}

/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA *get_room_index( int vnum )
{
    ROOM_INDEX_DATA *pRoomIndex;

    for ( pRoomIndex  = room_index_hash[vnum % MAX_KEY_HASH];
	  pRoomIndex != NULL;
	  pRoomIndex  = pRoomIndex->next )
    {
	if ( pRoomIndex->vnum == vnum )
	    return pRoomIndex;
    }

    return NULL;
}



/*
 * Read a letter from a file.
 */
char fread_letter( FILE *fp )
{
    char c;

    do
    {
	c = getc( fp );
    }
    while ( isspace(c) );

    return c;
}



/*
 * Read a number from a file.
 */
int fread_number( FILE *fp )
{
    int number;
    bool sign;
    char c;

    do
    {
	c = getc( fp );
    }
    while ( isspace(c) );

    number = 0;

    sign   = FALSE;
    if ( c == '+' )
    {
	c = getc( fp );
    }
    else if ( c == '-' )
    {
	sign = TRUE;
	c = getc( fp );
    }

    if ( !isdigit(c) )
    {
	char buf[MAX_STRING_LENGTH];
	sprintf(buf, "Fread_number: bad format: %c", c);
	bug(buf, 0);
	    bug( "Fread_number: bad format.", 0 );
	exit( 1 );
    }

    while ( isdigit(c) )
    {
	number = number * 10 + c - '0';
	c      = getc( fp );
    }

    if ( sign )
	number = 0 - number;

    if ( c == '|' )
	number += fread_number( fp );
    else if ( c != ' ' )
	ungetc( c, fp );

    return number;
}

long fread_flag( FILE *fp)
{
    int number;
    char c;
    bool negative = FALSE;

    do
    {
	c = getc(fp);
    }
    while ( isspace(c));

    if (c == '-')
    {
	negative = TRUE;
	c = getc(fp);
    }

    number = 0;

    if (!isdigit(c))
    {
	while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
	{
	    number += flag_convert(c);
	    c = getc(fp);
	}
    }

    while (isdigit(c))
    {
	number = number * 10 + c - '0';
	c = getc(fp);
    }

    if (c == '|')
	number += fread_flag(fp);

    else if  ( c != ' ')
	ungetc(c,fp);

    if (negative)
	return -1 * number;

    return number;
}

long flag_convert(char letter )
{
    long bitsum = 0;
    char i;

    if ('A' <= letter && letter <= 'Z') 
    {
	bitsum = 1;
	for (i = letter; i > 'A'; i--)
	    bitsum *= 2;
    }
    else if ('a' <= letter && letter <= 'z')
    {
	bitsum = 67108864; /* 2^26 */
	for (i = letter; i > 'a'; i --)
	    bitsum *= 2;
    }

    return bitsum;
}


char *fwrite_format( char *str )
{
    char *wlast;
    unsigned short char_count = 0;

    memset((void *) write_str, 0, MAX_STRING_LENGTH);

    if ( !str )
	return write_str;

    if (strlen(str) > MAX_STRING_LENGTH)
    {
        bug("fwrite_format: attempt to write string length %d", strlen(str));
	strncpy(write_str, str, MAX_STRING_LENGTH);
	bug(write_str, 0);
	return write_str;
    }

    wlast = write_str;

    while ( *str != '\0' )
    {
	switch (*str)
	{
	    case '\'':
		*wlast++ = '\\';
		*wlast++ = '@';
		char_count += 2;
		break;
	    case '"':
		*wlast++ = '\\';
		*wlast++ = '#';
		char_count += 2;
		break;
	    case '\\':
		*wlast++ = '\\';
		*wlast++ = '\\';
		char_count += 2;
		break;
	    case '~':
		*wlast++ = '\\';
		*wlast++ = '$';
		char_count += 2;
		break;
	    default:
		*wlast++ = *str;
		char_count++;
		break;
	}
	str++;

	if (char_count >= MAX_STRING_LENGTH)
	{
	    write_str[MAX_STRING_LENGTH-1] = 0;
	    sprintf(log_buf, "fwrite_format: translation too long.  Context %20s", write_str);
	    bug(log_buf, 0);
	    return write_str;
	}
    }
	    
    return write_str;
}

/*
 * Read and allocate space for a string from a file.
 * These strings are read-only and shared.
 * Strings are hashed:
 *   each string prepended with hash pointer to prev string,
 *   hash code is simply the string length.
 *   this function takes 40% to 50% of boot-up time.
 */
char *fread_string( FILE *fp )
{
    char *plast;
    char c, d;

    plast = top_string + sizeof(char *);
    if ( plast > &string_space[MAX_STRING - MAX_STRING_LENGTH] )
    {
	bug( "Fread_string: MAX_STRING %d exceeded.", MAX_STRING );
	exit( 1 );
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
	c = getc( fp );
    }
    while ( isspace(c) );

    if ( ( *plast = c ) == '~' )
	return &str_empty[0];

    if (c == '\\')
    {
	d = getc(fp);
	switch (d)
	{
	    case '@':
	        *plast = '\'';
	        break;
	    case '#':
	        *plast = '"';
	        break;
	    case '$':
	        *plast = '~';
	        break;
	    case '\\':
		*plast = '\\';
	        break;
	    default:
	        plast++;
	        *plast = d;		    
	    break;
	}
    }

    plast++; 

    for ( ;; )
    {
        /*
         * Back off the char type lookup,
         *   it was too dirty for portability.
         *   -- Furey
         */

	switch ( *plast = getc(fp) )
	{
        default:
            plast++;
            break;
 
        case EOF:
	/* temp fix */
            bug( "Fread_string: EOF", 0 );
	    return NULL;
            /* exit( 1 ); */
            break;
 
        case '\n':
            plast++;
            *plast++ = '\r';
            break;
 
        case '\r':
            break;

	case '\\':
	    d = getc(fp);
	    switch (d)
	    {
		case '@':
		    *plast++ = '\'';
		    break;
		case '#':
		    *plast++ = '"';
		    break;
		case '$':
		    *plast++ = '~';
		    break;
		case '\\':
		    *plast++ = '\\';
		    break;
		default:
		    plast++;
		    *plast++ = d;		    
		    break;
	    }
	    break;
 
        case '~':
            plast++;
	    {
		union
		{
		    char *	pc;
		    char	rgc[sizeof(char *)];
		} u1;
		size_t ic;
		int iHash;
		char *pHash;
		char *pHashPrev;
		char *pString;

		plast[-1] = '\0';
		iHash     = UMIN( MAX_KEY_HASH - 1, plast - 1 - top_string );
		for ( pHash = string_hash[iHash]; pHash; pHash = pHashPrev )
		{
		    for ( ic = 0; ic < sizeof(char *); ic++ )
				u1.rgc[ic] = pHash[ic];
		    pHashPrev = u1.pc;
		    pHash    += sizeof(char *);

		    if ( top_string[sizeof(char *)] == pHash[0]
		    &&   !strcmp( top_string+sizeof(char *)+1, pHash+1 ) )
			return pHash;
		}

		if ( fBootDb )
		{
		    pString		= top_string;
		    top_string		= plast;
		    u1.pc		= string_hash[iHash];
		    for ( ic = 0; ic < sizeof(char *); ic++ )
			pString[ic] = u1.rgc[ic];
		    string_hash[iHash]	= pString;

		    nAllocString += 1;
		    sAllocString += top_string - pString;
		    return pString + sizeof(char *);
		}
		else
		{
		    return str_dup( top_string + sizeof(char *) );
		}
	    }
	}
    }
}

char *fread_string_eol( FILE *fp )
{
    static bool char_special[256-EOF];
    char *plast;
    char c;
 
    if ( char_special[EOF-EOF] != TRUE )
    {
        char_special[EOF -  EOF] = TRUE;
        char_special['\n' - EOF] = TRUE;
        char_special['\r' - EOF] = TRUE;
    }
 
    plast = top_string + sizeof(char *);
    if ( plast > &string_space[MAX_STRING - MAX_STRING_LENGTH] )
    {
        bug( "Fread_string: MAX_STRING %d exceeded.", MAX_STRING );
        exit( 1 );
    }
 
    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
        c = getc( fp );
    }
    while ( isspace(c) );
 
    if ( ( *plast++ = c ) == '\n')
        return &str_empty[0];
 
    for ( ;; )
    {
        if ( !char_special[ ( *plast++ = getc( fp ) ) - EOF ] )
            continue;
 
        switch ( plast[-1] )
        {
        default:
            break;
 
        case EOF:
            bug( "Fread_string_eol  EOF", 0 );
            exit( 1 );
            break;
 
        case '\n':  case '\r':
            {
                union
                {
                    char *      pc;
                    char        rgc[sizeof(char *)];
                } u1;
                size_t ic;
                int iHash;
                char *pHash;
                char *pHashPrev;
                char *pString;
 
                plast[-1] = '\0';
                iHash     = UMIN( MAX_KEY_HASH - 1, plast - 1 - top_string );
                for ( pHash = string_hash[iHash]; pHash; pHash = pHashPrev )
                {
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        u1.rgc[ic] = pHash[ic];
                    pHashPrev = u1.pc;
                    pHash    += sizeof(char *);
 
                    if ( top_string[sizeof(char *)] == pHash[0]
                    &&   !strcmp( top_string+sizeof(char *)+1, pHash+1 ) )
                        return pHash;
                }
 
                if ( fBootDb )
                {
                    pString             = top_string;
                    top_string          = plast;
                    u1.pc               = string_hash[iHash];
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        pString[ic] = u1.rgc[ic];
                    string_hash[iHash]  = pString;
 
                    nAllocString += 1;
                    sAllocString += top_string - pString;
                    return pString + sizeof(char *);
                }
                else
                {
                    return str_dup( top_string + sizeof(char *) );
                }
            }
        }
    }
}



/*
 * Read to end of line (for comments).
 */
void fread_to_eol( FILE *fp )
{
    char c;

    do
    {
	c = getc( fp );
    }
    while ( c != '\n' && c != '\r' );

    do
    {
	c = getc( fp );
    }
    while ( c == '\n' || c == '\r' );

    ungetc( c, fp );
    return;
}


// I wrote this because I seriously didn't understand the
// usefulness of ungetc in fread_word.
char *fread_name(FILE *fp, char *buffer, const unsigned long max_length)
{
    char *pword;
    char cEnd;

    do
    {
	cEnd = getc( fp );
    }
    while ( isspace( cEnd ) );

    if ( cEnd == '\'' || cEnd == '"' )
    {
	pword   = buffer;
    }
    else
    {
	buffer[0] = cEnd;
	pword   = buffer+1;
	cEnd    = ' ';
    }

    for ( ; pword < buffer + max_length; pword++ )
    {
	*pword = getc( fp );
	if ( cEnd == ' ' ? isspace(*pword) : *pword == cEnd )
	{
	    *pword = '\0';

	    do
	    {
		cEnd = getc(fp);
	    }
	    while (isspace(cEnd));

	    if (!feof(fp))
		ungetc(cEnd, fp);

	    return buffer;
	}
    }

    bug ("fread_word: could not find end of name", 0);   

    exit( 1 );
    return NULL;
}



/*
 * Read one word (into static buffer).
 */
char *fread_word( FILE *fp )
{
    static char word[MAX_INPUT_LENGTH];
    char *pword;
    char cEnd, d, c;
    bool escaped = FALSE;
    
    do
    {
	cEnd = getc( fp );
    }
    while ( isspace( cEnd ) );

    if ( cEnd == '\'' || cEnd == '"' )
    {
	pword   = word;
    }
    else 
    {
	word[0] = cEnd;
	pword   = word+1;
	cEnd    = ' ';
    }

    for ( ; pword < word + MAX_INPUT_LENGTH; pword++ )
    {
	c = getc( fp );
	escaped = FALSE;
	if (c == '\\')
	{
		escaped = TRUE;
		d = getc(fp);
		switch (d)
		{
			case '@':
				c = '\'';
				break;
			case '#':
				c = '"';
				break;
			case '$':
				c = '~';
				break;
			case '\\':
				c = '\\';
				break;
			default:
				c = d;
				break;                                                                                                       }
	}
	*pword = c;
	if ( !escaped && (cEnd == ' ' ? isspace(*pword) : *pword == cEnd) )
	{
	    if ( cEnd == ' ' )
		ungetc( *pword, fp );
	    *pword = '\0';
	    return word;
	}
    }

    sprintf(log_buf, "pfile error: %s", strArea);
    bug (log_buf, 0);   

    sprintf(log_buf, "fread_word: word too long.  context = \"%20s...\"", word);
    bug(log_buf, 0);
    exit( 1 );
    return NULL;
}


/* fread word is the devil. Granted, I'm sure no one was
 * thinking of using threading when they wrote it with
 * a static buffer...but really, people.
 */
char *fread_dyn( FILE *fp, char *word, const unsigned long max_chars )
{
    char *start;
    char *pword;
    char cEnd;

    start = word;

    do
    {
	cEnd = getc( fp );
    }
    while ( isspace( cEnd ) );


    if ( cEnd == '\'' || cEnd == '"' )
    {
	pword   = word;
    }
    else
    {
	*start  = cEnd;
	pword   = word+1;
	cEnd    = ' ';
    }

    for ( ; pword < word + max_chars; pword++ )
    {
	if ((*pword = getc( fp )) == EOF)
	{
	    *pword = '\0';
	    return start;
	}

	if ( cEnd == ' ' ? isspace(*pword) : *pword == cEnd )
	{
	    if ( cEnd == ' ' )
		ungetc( *pword, fp );
	    *pword = '\0';
	    return start;
	}
    }

    start[max_chars - 1] = '\0';

    sprintf(log_buf, "fread_dyn: word too long (max = %lu).  context = \"%20s...\"", max_chars, start);
    bug(log_buf, 0);
//    exit( 1 );
    return NULL;
}


void add_cleanup(void *addr)
{
    ADDRESS_LIST *cleanup_ptr = (ADDRESS_LIST*) malloc(sizeof(struct address_list));

	if (cleanup_ptr == 0)
	{
		bug("Unable to allocate ADDRESS_LIST* in add_cleanup", 0);
	}

    cleanup_ptr->addr = addr;
    cleanup_ptr->next = cleanup_list;
    cleanup_list = cleanup_ptr;
}

void cleanup()
{
    ADDRESS_LIST *al, *al_next;
    
    printf("in function cleanup\n");

    free(string_space);

    for (al = cleanup_list; al; al = al_next)
    {
	al_next = al->next;
	free(al->addr);
	free(al);
    }

    return;
}

/*
 * Allocate some permanent memory.
 * Permanent memory is never freed,
 *   pointers into it may be copied safely.
 */
void *alloc_perm( int sMem )
{
    static char *pMemPerm(0);
    static int iMemPerm(0);
    void *pMem(0);
    
	// Align on a long
    while ( sMem % sizeof(long) != 0 )
		sMem++;

	// Make sure the memory is not too big
    if (sMem > MAX_PERM_BLOCK)
    {
		bug("Alloc_perm: %d too large.", sMem);
		exit(1);
    }

    if (pMemPerm == NULL || iMemPerm + sMem > MAX_PERM_BLOCK)
    {
		iMemPerm = 0;
		if ((pMemPerm = (char*)calloc(1, MAX_PERM_BLOCK)) == NULL)
		{
	    	perror("Alloc_perm");
		    exit(1);
		}

		add_cleanup(pMemPerm);
    }

    pMem        = pMemPerm + iMemPerm;
    iMemPerm   += sMem;
    nAllocPerm += 1;
    sAllocPerm += sMem;

    if (mcur_running)
    {
		char buf[MAX_INPUT_LENGTH];

		sprintf(buf, "%d bytes allocated, mob %d running prog.", sMem, mcur_running->pIndexData->vnum);
		wiznet(buf, NULL, NULL, WIZ_DEBUG, 0, 0);
    }

    return pMem;
}

/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
char *str_dup( const char *str )
{
  char *str_new = NULL;

  if (str)
  {
    if ( str[0] == '\0' )
	return &str_empty[0];

    if ( str >= string_space && str < top_string )
	return (char *) str;

	size_t len(strlen(str));
    str_new = (char*)malloc(len + 1 );
    g_extra_strings += (len + 1);
    strcpy( str_new, str );
  }
  return str_new;
}



/*
 * Free a string.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */
void free_string(char *&pstr)
{
    if (pstr != NULL && pstr != &str_empty[0] && (pstr < string_space || pstr >= top_string))
	{
	    g_extra_strings -= (strlen((const char *)pstr) + 1);
    	free(pstr);
        pstr = NULL;
	}
}

void do_areas(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_INPUT_LENGTH];
    AREA_DATA *pArea = area_first;
    int pval = 0;

    while (pArea)
    {
	if (!IS_SET(pArea->ainfo_flags, AINFO_INVIS))
	{
	    if (pval++ == 2)
	    {
	        sprintf(buf, "\n\r%-39s", pArea->credits);
	        pval = 1;
	    }
	    else
	        sprintf(buf, "%-39s", pArea->credits);

	    send_to_char_bw(buf, ch);
	}

	pArea = pArea->next;
    }

    send_to_char("\n\r", ch);

    return;
}


extern int top_chain;
extern int top_event;
extern int chain_count;
extern int event_count;

void do_memory( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    sprintf( buf, "Affects %6d\n\r", top_affect    ); send_to_char( buf, ch );
    sprintf( buf, "Alinks  %6d\n\r", top_alink	   ); send_to_char( buf, ch );
    sprintf( buf, "Areas   %6d\n\r", top_area      ); send_to_char( buf, ch );
    sprintf( buf, "Chains  %6d\n\r", top_chain	   ); send_to_char( buf, ch );
    sprintf( buf, "(in use)%6d\n\r", chain_count   ); send_to_char( buf, ch );
    sprintf( buf, "Events  %6d\n\r", top_event	   ); send_to_char( buf, ch );
    sprintf( buf, "(in use)%6d\n\r", event_count   ); send_to_char( buf, ch );
    sprintf( buf, "ExDes   %6d\n\r", top_ed        ); send_to_char( buf, ch );
    sprintf( buf, "Exits   %6d\n\r", top_exit      ); send_to_char( buf, ch );
    sprintf( buf, "Helps   %6d\n\r", top_help      ); send_to_char( buf, ch );
    sprintf( buf, "Socials %6d\n\r", social_count  ); send_to_char( buf, ch );
    sprintf( buf, "Mobs    %6d(%d new format)\n\r", top_mob_index,newmobs ); 
    send_to_char( buf, ch );
    sprintf( buf, "(in use)%6d\n\r", mobile_count  ); send_to_char( buf, ch );
    sprintf( buf, "Objs    %6d(%d new format)\n\r", top_obj_index,newobjs ); 
    send_to_char( buf, ch );
    sprintf( buf, "Resets  %6d\n\r", top_reset     ); send_to_char( buf, ch );
    sprintf( buf, "Rooms   %6d\n\r", top_room      ); send_to_char( buf, ch );
    sprintf( buf, "Shops   %6d\n\r", top_shop      ); send_to_char( buf, ch );

    sprintf( buf, "Strings %6d strings of %7d bytes (max %d).\n\r",
	nAllocString, sAllocString, MAX_STRING );
    send_to_char( buf, ch );

    sprintf( buf, "Perms   %6d blocks  of %7d bytes.\n\r",
	nAllocPerm, sAllocPerm );
    send_to_char( buf, ch );

    sprintf( buf, "Segvs   %d\n\r", segv_counter );
    send_to_char( buf, ch );

    return;
}

void do_debug_memory(CHAR_DATA *ch, const std::vector<std::string> &)
{
    char buf[MAX_STRING_LENGTH];
    long total_size = 0;

    send_to_char("Data Type         Alloc        SizeOf         Usage\n\r", ch);
    send_to_char("===================================================\n\r", ch);

    sprintf(buf, "Accounts         %6d %13d %13d\n\r", g_num_accts, sizeof(struct account_data), g_num_accts * sizeof(struct account_data));
    total_size += (g_num_accts * sizeof(struct account_data));
    send_to_char(buf, ch);

    sprintf(buf, "Affects          %6d %13d %13d\n\r", top_affect, sizeof(struct affect_data), top_affect * sizeof(struct affect_data));
    total_size += (top_affect * sizeof(struct affect_data));
    send_to_char(buf, ch);

    sprintf(buf, "Area Links       %6d %13d %13d\n\r", top_alink, sizeof(struct area_link_data), top_alink * sizeof(struct area_link_data));
    total_size += (top_alink * sizeof(struct area_link_data));
    send_to_char(buf, ch);

    sprintf(buf, "Areas            %6d %13d %13d\n\r", top_area, sizeof(struct area_data), top_area * sizeof(struct area_data));
    total_size += (top_area * sizeof(struct area_data));
    send_to_char(buf, ch);

    sprintf(buf, "Bans             %6d %13d %13d\n\r", g_num_bans, sizeof(struct ban_data), g_num_bans * sizeof(struct ban_data));
    total_size += (g_num_bans * sizeof(struct ban_data));
    send_to_char(buf, ch);

    sprintf(buf, "Buffers          %6d %13d %13d\n\r", g_num_buffer, sizeof(struct buf_type), g_num_buffer * sizeof(struct buf_type));
    total_size += (g_num_buffer * sizeof(struct buf_type));
    send_to_char(buf, ch);

    sprintf(buf, "(strings)                             %13d\n\r", g_size_bufstrings);
    total_size += g_size_bufstrings;
    send_to_char(buf, ch);

    sprintf(buf, "Chains           %6d %13d %13d\n\r", top_chain, sizeof(struct chain_data), top_chain * sizeof(struct chain_data));
    total_size += (top_chain * sizeof(struct chain_data));
    send_to_char(buf, ch);

    sprintf(buf, "Characters       %6d %13d %13d\n\r", g_num_char_data, sizeof(struct char_data), g_num_char_data * sizeof(struct char_data));
    total_size += (g_num_char_data * sizeof(struct char_data));
    send_to_char(buf, ch);

    sprintf(buf, "Coin Arrays      %6d %13d %13d\n\r", g_num_coin_array, MAX_COIN * sizeof(long), g_num_coin_array * MAX_COIN * sizeof(long));
    total_size += (g_num_coin_array * MAX_COIN * sizeof(long));
    send_to_char(buf, ch);

    sprintf(buf, "Descriptors      %6d %13d %13d\n\r", g_num_descriptors, sizeof(struct descriptor_data), g_num_descriptors * sizeof(struct descriptor_data));
    total_size += (g_num_descriptors * sizeof(struct descriptor_data));
    send_to_char(buf, ch);

    sprintf(buf, "(outbufs)                             %13d\n\r", g_outbuf_size);
    total_size += g_outbuf_size;
    send_to_char(buf, ch);

    sprintf(buf, "(pagebufs)                            %13d\n\r", g_size_pagebuf);
    total_size += g_size_pagebuf;
    send_to_char(buf, ch);

    sprintf(buf, "Dictionaries     %6d %13d %13d\n\r", g_num_dicts, sizeof(struct dictionary_data), g_num_dicts * sizeof(struct dictionary_data));
    total_size += (g_num_dicts * sizeof(struct dictionary_data));
    send_to_char(buf, ch);

    sprintf(buf, "Events           %6d %13d %13d\n\r", top_event, sizeof(struct event_data), top_event * sizeof(struct event_data));
    total_size += (top_event * sizeof(struct event_data));
    send_to_char(buf, ch);

    sprintf(buf, "ExDescs          %6d %13d %13d\n\r", top_ed, sizeof(struct extra_descr_data), top_ed * sizeof(struct extra_descr_data));
    total_size += (top_ed * sizeof(struct extra_descr_data));
    send_to_char(buf, ch);

    sprintf(buf, "Exits            %6d %13d %13d\n\r", top_exit, sizeof(struct exit_data), top_exit * sizeof(struct exit_data));
    total_size += (top_exit * sizeof(struct exit_data));
    send_to_char(buf, ch);

    sprintf(buf, "Guildmasters     %6d %13d %13d\n\r", g_gm_count, sizeof(struct gm_data), g_gm_count * sizeof(struct gm_data));
    total_size += (g_gm_count * sizeof(struct gm_data));
    send_to_char(buf, ch);

    sprintf(buf, "Headers          %6d %13d %13d\n\r", g_num_headers, sizeof(struct header_data), g_num_headers * sizeof(struct header_data));
    total_size += (g_num_headers * sizeof(struct header_data));
    send_to_char(buf, ch);

    sprintf(buf, "Helps            %6d %13d %13d\n\r", g_num_helps, sizeof(struct help_data), g_num_helps * sizeof(struct help_data));
    total_size += (g_num_helps * sizeof(struct help_data));
    send_to_char(buf, ch);

    sprintf(buf, "Host Data        %6d %13d %13d\n\r", g_num_hostdata, sizeof(struct host_data), g_num_hostdata * sizeof(struct host_data));
    total_size += (g_num_hostdata * sizeof(struct host_data));
    send_to_char(buf, ch);

    sprintf(buf, "Loop Data        %6d %13d %13d\n\r", g_num_loopdata, sizeof(struct loop_data), g_num_loopdata * sizeof(struct loop_data));
    total_size += (g_num_loopdata * sizeof(struct loop_data));
    send_to_char(buf, ch);

    sprintf(buf, "Mobile Indices   %6d %13d %13d\n\r", top_mob_index, sizeof(struct mob_index_data), top_mob_index * sizeof(struct mob_index_data));
    total_size += (top_mob_index * sizeof(struct mob_index_data));
    send_to_char(buf, ch);

    sprintf(buf, "Mob Memory       %6d %13d %13d\n\r", g_num_mobmemory, sizeof(struct memory_data), g_num_mobmemory * sizeof(struct memory_data));
    total_size += (g_num_mobmemory * sizeof(struct memory_data));
    send_to_char(buf, ch);

    sprintf(buf, "Notes            %6d %13d %13d\n\r", g_num_notes, sizeof(struct note_data), g_num_notes * sizeof(struct note_data));
    total_size += (g_num_notes * sizeof(struct note_data));
    send_to_char(buf, ch);

    sprintf(buf, "Object Indicies  %6d %13d %13d\n\r", top_obj_index, sizeof(struct obj_index_data), top_obj_index * sizeof(struct obj_index_data));
    total_size += (top_obj_index * sizeof(struct obj_index_data));
    send_to_char(buf, ch);

    sprintf(buf, "Objects          %6d %13d %13d\n\r", object_count, sizeof(struct obj_data), object_count * sizeof(struct obj_data));
    total_size += (object_count * sizeof(struct obj_data));
    send_to_char(buf, ch);

    sprintf(buf, "PC Data          %6d %13d %13d\n\r", g_num_pcdata, sizeof(struct pc_data), g_num_pcdata * sizeof(struct pc_data));
    total_size += (g_num_pcdata * sizeof(struct pc_data));
    send_to_char(buf, ch);

    sprintf(buf, "Progs            %6d %13d %13d\n\r", g_num_progs, sizeof(struct mob_prog_data), g_num_progs * sizeof(struct mob_prog_data));
    total_size += (g_num_progs * sizeof(struct mob_prog_data));
    send_to_char(buf, ch);

    sprintf(buf, "Prog Vars        %6d %13d %13d\n\r", g_num_progvars, sizeof(struct prog_vardata), g_num_progvars * sizeof(struct prog_vardata));
    total_size += (g_num_progvars * sizeof(struct prog_vardata));
    send_to_char(buf, ch);

    sprintf(buf, "Resets           %6d %13d %13d\n\r", top_reset, sizeof(struct reset_data), top_reset * sizeof(struct reset_data));
    total_size += (top_reset * sizeof(struct reset_data));
    send_to_char(buf, ch);

    sprintf(buf, "Rooms            %6d %13d %13d\n\r", top_room, sizeof(struct room_index_data), top_room * sizeof(struct room_index_data));
    total_size += (top_room * sizeof(struct room_index_data));
    send_to_char(buf, ch);

    sprintf(buf, "Shops            %6d %13d %13d\n\r", top_shop, sizeof(struct shop_data), top_shop * sizeof(struct shop_data));
    total_size += (top_shop * sizeof(struct shop_data));
    send_to_char(buf, ch);

    sprintf(buf, "Tracks           %6d %13d %13d\n\r", g_num_tracks, sizeof(struct track_data), g_num_tracks * sizeof(struct track_data));
    total_size += (g_num_tracks * sizeof(struct track_data));
    send_to_char(buf, ch);

    sprintf(buf, "Vnum Blocks      %6d %13d %13d\n\r", g_vnum_blocks, sizeof(struct vnum_range), g_vnum_blocks * sizeof(struct vnum_range));
    total_size += (g_vnum_blocks * sizeof(struct vnum_range));
    send_to_char(buf, ch);

    send_to_char("                                      =============\n\r", ch);
    sprintf(buf, "                           Sub-Total: %13ld\n\r", total_size);
    send_to_char(buf, ch);
    sprintf(buf, "              Allocated String Space: %13d\n\r", MAX_STRING);
    send_to_char(buf, ch);
    sprintf(buf, "             Other Allocated Strings: %13d\n\r", g_extra_strings);
    send_to_char(buf, ch);

    send_to_char("                                      =============\n\r", ch);
    sprintf(buf, "                               Total: %13ld\n\r", total_size + g_extra_strings + MAX_STRING);
    send_to_char(buf, ch);

    return;
}

char door_letter(int door)
{
    static const char dir_name[MAX_DOOR + 1] = {'N', 'E', 'S', 'W', 'U', 'D', '?'};
    return dir_name[UMIN(static_cast<unsigned int>(door), MAX_DOOR)];
}

static void do_debug_drakes(CHAR_DATA * ch, const std::vector<std::string> & args)
{
    if (args.size() < 1)
    {
        send_to_char("Choose a drake to nearly age.\n", ch);
        return;
    }

    // Lookup the drake
    CHAR_DATA * drake(get_char_room(ch, args[0].c_str()));
    if (drake == NULL || !IS_NPC(drake) || drake->pIndexData->vnum != MOB_VNUM_DRAKE)
    {
        send_to_char("There is no drake here by that name.\n", ch);
        return;
    }

    // Lookup the effect
    AFFECT_DATA * paf(get_affect(drake, gsn_wakenedstone));
    if (paf == NULL)
    {
        act("$N lacks the proper effect!", ch, NULL, drake, TO_CHAR);
        return;
    }

    // Set the duration to nearly complete
    paf->duration = 0;
    send_to_char("Duration set to 0.\n", ch);
}

static void do_debug_path(CHAR_DATA * ch, const std::vector<std::string> & args)
{
    // Check for no args
    if (args.size() < 4)
    {
        send_to_char("Options for debug path:\n", ch);
        send_to_char("<actor> <target room vnum> <max steps|-1> <ignoreClosed: true|false> [brief]: routes a path to the target vnum from the current room; brief suppresses most output\n", ch);
        return;
    }

    // Get the actor
    CHAR_DATA * actor(get_char_room(ch, const_cast<char*>(args[0].c_str())));
    if (actor == NULL)
    {
        std::ostringstream mess;
        mess << "Did not find an actor here with name '" << args[0] << "'\n";
        send_to_char(mess.str().c_str(), ch);
        return;
    }

    // Get the target room
    if (!is_number(args[1].c_str()))
    {
        std::ostringstream mess;
        mess << "Unable to parse room vnum from argument '" << args[1] << "'\n";
        send_to_char(mess.str().c_str(), ch);
        return;
    }

    ROOM_INDEX_DATA * endRoom(get_room_index(atoi(args[1].c_str())));
    if (endRoom == NULL)
    {
        send_to_char("Target room does not exist.\n", ch);
        return;
    }

    ROOM_INDEX_DATA * startRoom(ch->in_room);
    if (startRoom == NULL)
    {
        send_to_char("Source room does not exist.\n", ch);
        return;
    }

    // Get the max number of steps
    if (!is_number(args[2].c_str()))
    {
        std::ostringstream mess;
        mess << "Unable to parse max step count from argument '" << args[2] << "'\n";
        send_to_char(mess.str().c_str(), ch);
        return;
    }

    // Treat negative values as infinite
    int maxStepsRaw(atoi(args[2].c_str()));
    unsigned int maxSteps;
    if (maxStepsRaw < 0) maxSteps = RoomPath::Infinite;
    else maxSteps = static_cast<unsigned int>(maxStepsRaw);

    // Check for include closed or not
    bool includeClosed;
    if (!str_prefix(args[3].c_str(), "true")) includeClosed = true;
    else if (!str_prefix(args[3].c_str(), "false")) includeClosed = false;
    else
    {
        std::ostringstream mess;
        mess << "Unable to parse true/false from argument '" << args[3] << "'\n";
        send_to_char(mess.str().c_str(), ch);
        return;
    }

    // Check for brief/verbose
    bool verbose(true);
    if (args.size() >= 5)
    {
        if (!str_prefix(args[4].c_str(), "brief")) verbose = false;
        else
        {
            std::ostringstream mess;
            mess << "Did not understand argument '" << args[4] << "'; did you mean 'brief'?\n";
            send_to_char(mess.str().c_str(), ch);
            return;
        }
    }

    // Generate the path
    RoomPath path(*startRoom, *endRoom, actor, maxSteps, includeClosed);
    if (!path.Exists())
    {
        std::ostringstream mess;
        mess << "No path found from room " << startRoom->vnum << " to room " << endRoom->vnum << "\n";
        send_to_char(mess.str().c_str(), ch);
        return;
    }

    // Path exists; send the path if verbose
    if (verbose)
    {
        const unsigned int rowLength = 20;
        std::ostringstream mess;
        for (unsigned int i(0); i < path.StepCount(); ++i)
        {
            mess << door_letter(path.StepDirection(i));
            if (i % rowLength == (rowLength - 1)) mess << '\n';
            else mess << ' ';
        }

        if (path.StepCount() % rowLength != (rowLength - 1))
            mess << '\n';

        send_to_char(mess.str().c_str(), ch);
    }

    // Send the path length
    std::ostringstream mess;
    mess << "Path found from room " << startRoom->vnum << " to room " << endRoom->vnum << " in " << path.StepCount() << " steps\n";
    send_to_char(mess.str().c_str(), ch);
}

void do_debug_weather(CHAR_DATA * ch, const std::vector<std::string> & args)
{
    if (args.size() > 0)
    {
        if (!str_prefix(args[0].c_str(), "area_temperatures"))
        {
            int value = NO_FLAG;
            if (args.size() > 1)
            {
                value = flag_value(temp_flags, const_cast<char*>(args[1].c_str()));
                if (value == NO_FLAG)
                    send_to_char("Did not understand temperature name, listing all areas\n", ch);
            }

            for (AREA_DATA * pArea = area_first; pArea != NULL; pArea = pArea->next)
            {
                std::ostringstream mess;
                if (pArea->base_temp == value || value == NO_FLAG)
                    mess << pArea->name << ": " << flag_string(temp_flags, pArea->base_temp) << "\n";
                send_to_char(mess.str().c_str(), ch);
            }
            return;
        }

        send_to_char("Options for debug weather:\n", ch);
        send_to_char("[no argument]: prints a list of the current area's statistics, focusing on temperature\n", ch);
        send_to_char("area_temperatures [temp type]: prints a list of all areas with temperature indices\n", ch);
        return;
    }

    // Sanity check
    if (ch->in_room == NULL || ch->in_room->area == NULL)
    {
        send_to_char("Cannot report on weather; missing room or area\n", ch);
        return;
    }

    // Print out the current weather data
    const AREA_DATA & area(*(ch->in_room->area));
    std::ostringstream mess;
    mess << "Current weather in " << area.name << "\n";
    mess << "Temperature: " << area.w_cur.temperature << "\n";
    mess << "Cloud Cover: " << area.w_cur.cloud_cover << "\n";
    mess << "Wind Direction: " << area.w_cur.wind_dir << "(" << door_letter(area.w_cur.wind_dir) << ")\n";
    mess << "Wind Magnitude: " << area.w_cur.wind_mag << "\n";
    mess << "Storm Strength: " << area.w_cur.storm_str << "\n";
    mess << "Precipitation Index: " << area.w_cur.precip_index << "\n";
    mess << "Precipitation Type: " << area.w_cur.precip_type << "\n";
    mess << "Lightning Strength: " << area.w_cur.lightning_str << "\n";
    mess << "Tornado: " << (area.w_cur.tornado ? "Yes" : "No") << "\n";
    send_to_char(mess.str().c_str(), ch);

    // Print out the area's base stats
    mess.str(""); mess << "\n";
    mess << "Base Temperature: " << calculate_base_temperature(area.base_temp) << "\n";
    mess << "Base Precipitation: " << area.base_precip << "\n";
    mess << "Base Wind Magnitude: " << area.base_wind_mag << "\n";
    mess << "Base Wind Direction: " << area.base_wind_dir << "(" << door_letter(area.base_wind_dir) << ")\n";
    mess << "Geography: " << area.geography << "\n";
    send_to_char(mess.str().c_str(), ch);

    // Print out any weather-affecting area affects
    int commandTemp(0);
    int commandWet(0);
    int commandWind(0);
    unsigned int commandUnknown(0);
    unsigned int totalCommand(0);
    for (const AFFECT_DATA * paf(area.affected); paf != NULL; paf = paf->next)
    {
        if (paf->type == gsn_commandweather)
        {
            ++totalCommand;
            switch (paf->modifier)
            {
                case WCOM_DRIER: --commandWet; break;
                case WCOM_WETTER: ++commandWet; break;
                case WCOM_CALMER: --commandWind; break;
                case WCOM_WINDIER: ++commandWind; break;
                case WCOM_COOLER: --commandTemp; break;
                case WCOM_WARMER: ++commandTemp; break;
                default: ++commandUnknown; break;
            }
        }
    }

    mess.str(""); mess << "\n";
    mess << "Command Weather Effects: " << totalCommand << "\n";
    if (commandTemp != 0) mess << "Temperature Modifier: " << calculate_base_temperature(area.base_temp + commandTemp) << "\n";
    if (commandWet != 0) mess << "Humidity Modifier: " << commandWet << "\n";
    if (commandWind != 0) mess << "Wind Modifier: " << commandWind << "\n";
    if (commandUnknown > 0) mess << "Unknown Modifiers: " << commandUnknown << "\n";
    send_to_char(mess.str().c_str(), ch);

    // Print out the seasonal modifiers
    const season_type & season(season_table[time_info.season]);
    mess.str(""); mess << "\n";
    mess << "Seasonal modifiers for " << season.name << "\n";
    mess << "Temperature Modifier: " << calculate_seasonal_temperature_modifier() << "\n";
    mess << "Wind Modifier: " << season.wind_mod << "\n";
    mess << "Snow Chance: " << season.snow_chance << "\n";
    mess << "Storm Modifier: " << season.storm_mod << "\n";
    mess << "Lightning Modifier: " << season.lightning_mod << "\n";
    mess << "Tornado Modifier: " << season.tornado_mod << "\n";
    mess << "Daylight Modifier: " << calculate_daily_temperature_modifier(area.w_cur.cloud_cover) << "\n";
    send_to_char(mess.str().c_str(), ch);

    // Print out the connected areas
    int connectedMod(0);
    mess.str(""); mess << "\n";
    mess << "Connected Areas Blowing This Way\n";
    for (ALINK_DATA * alink(alink_first); alink != NULL; alink = alink->next)
    {
        AREA_DATA * other;
        if (alink->a1 == &area && alink->a2->w_cur.wind_dir == alink->dir1) other = alink->a2;
        else if (alink->a2 == &area && alink->a1->w_cur.wind_dir == alink->dir2) other = alink->a1;
        else continue;

        mess << other->name << " (Wind: " << other->w_cur.wind_mag << ") (Temp: " << other->w_cur.temperature << ")\n";
        connectedMod += ((other->w_cur.temperature - area.w_cur.temperature) * other->w_cur.wind_mag);
    }

    mess << "Connected Temperature Modifier: " << (connectedMod / 100) << "\n";
    send_to_char(mess.str().c_str(), ch);
}

static void do_debug_weave_founts(CHAR_DATA * ch)
{
    // Iterate all rooms in the world
    std::ostringstream mess;
    mess << std::setfill(' ');
    mess << std::left;

    unsigned int col(0);
    for (unsigned int i(0); i < MAX_KEY_HASH; ++i)
    {
        for (const ROOM_INDEX_DATA * roomIter(room_index_hash[i]); roomIter != NULL; roomIter = roomIter->next)
        {
            // Check whether this room has a fount
            if (roomIter->ley_group != NULL && roomIter->ley_group->HasFount())
            {
                mess << '[' << std::setw(6) << roomIter->vnum << "] " << std::setw(40) << roomIter->name;
                ++col;
                if (col % 2 == 0)
                    mess << '\n';
            }
        }
    }

    mess << "\nTotal: " << col << "\n";
    page_to_char(mess.str().c_str(), ch);
}

static void do_debug_weave_area(CHAR_DATA * ch)
{
    // Sanity check
    if (ch->in_room == NULL || ch->in_room->area == NULL)
        return;

    std::ostringstream mess;
    mess << "Weave in " << ch->in_room->area->name << "\n";
    mess << "Fount Locations ([Vnum] [Order/Power] Location):\n";
    mess << std::setfill(' ') << std::left;

    unsigned int roomCount(0);
    unsigned int leyRooms(0);
    unsigned int fountCount(0);
    for (const VNUM_RANGE * range(ch->in_room->area->vnums); range != NULL; range = range->next)
    {
        for (int i(range->min_vnum); i <= range->max_vnum; ++i)
        {
            // Get this room
            ROOM_INDEX_DATA * room(get_room_index(i));
            if (room == NULL)
                continue;

            ++roomCount;
            if (room->ley_group == NULL)
                continue;

            if (room->ley_group->HasFount())
            {
                ++fountCount;
                ++leyRooms;
                mess << "[" << std::setw(6) << room->vnum << "] [";
                mess << std::setw(2) << room->ley_group->FountOrderPower() << " / " << std::setw(2) << room->ley_group->FountPositivePower() << "] ";
                mess << room->name << "\n";
            }
            else if (room->ley_group->LineCount() > 0)
                ++leyRooms;
        }
    }

    mess << "\nTotal Rooms: " << roomCount << "\n";
    mess << "Ley Rooms: " << leyRooms << "\n";
    mess << "Founts: " << fountCount << "\n";
    send_to_char(mess.str().c_str(), ch);
}

static void do_debug_weave_totals(CHAR_DATA * ch)
{
    // Iterate all rooms in the world
    std::ostringstream mess;

    unsigned int total(0);
    unsigned int leyRooms(0);
    unsigned int fountCount(0);
    std::map<int, std::map<int, unsigned int> > founts;
    for (unsigned int i(0); i < MAX_KEY_HASH; ++i)
    {
        for (const ROOM_INDEX_DATA * roomIter(room_index_hash[i]); roomIter != NULL; roomIter = roomIter->next)
        {
            ++total;

            // Check whether this room has any ley presence
            if (roomIter->ley_group == NULL)
                continue;

            // Check for a fount
            if (roomIter->ley_group->HasFount())
            {
                ++leyRooms;
                ++fountCount;
                std::map<int, std::map<int, unsigned int> >::iterator iter(founts.find(roomIter->ley_group->FountOrderPower()));
                if (iter == founts.end())
                    iter = founts.insert(std::make_pair(roomIter->ley_group->FountOrderPower(), std::map<int, unsigned int>())).first;

                std::map<int, unsigned int>::iterator otherIter(iter->second.find(roomIter->ley_group->FountPositivePower()));
                if (otherIter == iter->second.end()) iter->second.insert(std::make_pair(roomIter->ley_group->FountPositivePower(), 0));
                else ++otherIter->second;
            }
            else if (roomIter->ley_group->LineCount() > 0)
                ++leyRooms;
        }
    }

    mess << "Total Rooms: " << total << "\n";
    mess << "Ley Rooms: " << leyRooms << "\n";
    mess << "Founts: " << fountCount << "\n";
    mess << "Founts Totals [Order / Positive]\n";
    for (std::map<int, std::map<int, unsigned int> >::const_iterator iter(founts.begin()); iter != founts.end(); ++iter)
    {
        for (std::map<int, unsigned int>::const_iterator otherIter(iter->second.begin()); otherIter != iter->second.end(); ++otherIter)
            mess << "[" << iter->first << " / " << otherIter->first << " ]: " << otherIter->second << "\n";
    }
    send_to_char(mess.str().c_str(), ch);
}

static void do_debug_weave(CHAR_DATA * ch, const std::vector<std::string> & args)
{
    // Test for any arguments
    if (args.empty())
    {
        send_to_char("Usage:\n", ch);
        send_to_char("debug weave founts: lists fount locations\n", ch);
        send_to_char("debug weave reset: rebuilds the entire weave\n", ch);
        send_to_char("debug weave totals: gives statistics on the weave\n", ch);
        send_to_char("debug weave area: gives statistics and fount locations on the weave in your current area\n", ch);
        return;
    }

    if (!str_prefix(args[0].c_str(), "founts")) do_debug_weave_founts(ch);
    else if (!str_prefix(args[0].c_str(), "reset")) Weave::Regenerate();
    else if (!str_prefix(args[0].c_str(), "totals")) do_debug_weave_totals(ch);
    else if (!str_prefix(args[0].c_str(), "area")) do_debug_weave_area(ch);
    else send_to_char("Invalid weave debug command; use no arguments to get a list of valid commands.\n", ch);
}

static void do_debug_shades_totals(CHAR_DATA * ch)
{
    std::map<int, unsigned int> shadeLevels;
    std::map<std::string, unsigned int> shadeAreas;
    unsigned int total(0);

    for (unsigned int i(0); i < MAX_KEY_HASH; ++i)
    {
        for (const ROOM_INDEX_DATA * room(room_index_hash[i]); room != NULL; room = room->next)
        {
            for (const CHAR_DATA * ch(room->people); ch != NULL; ch = ch->next_in_room)
            {
                // Check for shade
                if (!IS_NPC(ch) || !IS_SET(ch->nact, ACT_SHADE))
                    continue;

                // Found a shade; increment the total count
                ++total;
                
                // Add in the level count
                std::map<int, unsigned int>::iterator levelIter(shadeLevels.find(ch->level));
                if (levelIter == shadeLevels.end())
                    levelIter = shadeLevels.insert(std::make_pair(ch->level, 0)).first;
                ++levelIter->second;

                // Add in the area count
                std::string areaName(room->area->name);
                std::map<std::string, unsigned int>::iterator areaIter(shadeAreas.find(areaName));
                if (areaIter == shadeAreas.end())
                    areaIter = shadeAreas.insert(std::make_pair(areaName, 0)).first;
                ++areaIter->second;
            }
        }
    }

    std::ostringstream mess;
    mess << "Total: " << total << "\n\nShades by Level\n";
    for (std::map<int, unsigned int>::const_iterator iter(shadeLevels.begin()); iter != shadeLevels.end(); ++iter)
        mess << "Level " << iter->first << ": " << iter->second << "\n";
    mess << "\nShades by Area\n";
    for (std::map<std::string, unsigned int>::const_iterator iter(shadeAreas.begin()); iter != shadeAreas.end(); ++iter)
        mess << iter->first << ": " << iter->second << "\n";

    mess << "\n";
    page_to_char(mess.str().c_str(), ch);
}

static void do_debug_shades_area(CHAR_DATA * ch)
{
    // Sanity-check
    if (ch->in_room == NULL || ch->in_room->area == NULL)
        return;

    // Iterate all rooms in this area
    unsigned int total(0);
    std::ostringstream mess;
    mess << std::setfill(' ') << std::left;
    mess << "Shades in " << ch->in_room->area->name << "\n";
    mess << "[ROOM VNUM]: COUNT\n";
    for (const VNUM_RANGE * range(ch->in_room->area->vnums); range != NULL; range = range->next)
    {
        for (int i(range->min_vnum); i <= range->max_vnum; ++i)
        {
            // Get this room
            ROOM_INDEX_DATA * room(get_room_index(i));
            if (room == NULL)
                continue;

            // Check for shades
            unsigned int totalInRoom(0);
            for (const CHAR_DATA * ch(room->people); ch != NULL; ch = ch->next_in_room)
            {
                // Check for shade
                if (!IS_NPC(ch) || !IS_SET(ch->nact, ACT_SHADE))
                    continue;

                // Found a shade
                ++total;
                ++totalInRoom;
            }
            
            if (totalInRoom > 0)
                mess << "[" << std::setw(6) << room->vnum << "]: " << totalInRoom << "\n";            
        }
    }

    mess << "\nTotal: " << total << "\n";
    send_to_char(mess.str().c_str(), ch);
}

static void do_debug_shades(CHAR_DATA * ch, const std::vector<std::string> & args)
{
    // Test for any arguments
    if (args.empty())
    {
        send_to_char("Usage:\n", ch);
        send_to_char("debug shades totals: gives statistics on shades\n", ch);
        send_to_char("debug shades update: performs a shade update\n", ch);
        send_to_char("debug shades area: gives statistics and locations of shades in your current area\n", ch);
        return;
    }

    if (!str_prefix(args[0].c_str(), "totals")) do_debug_shades_totals(ch);
    else if (!str_prefix(args[0].c_str(), "update")) ShadeControl::Update();
    else if (!str_prefix(args[0].c_str(), "area")) do_debug_shades_area(ch);
    else send_to_char("Invalid shade debug command; use no arguments to get a list of valid commands.\n", ch);
}

static void do_debug_windfall_list(CHAR_DATA * ch, const std::vector<std::string> & args)
{
    bool showMarked(true);
    if (args.size() >= 2)
    {
        if (!str_prefix(args[1].c_str(), "unmarked")) 
            showMarked = false;
        else
        {
            send_to_char("Unrecognized arguments; use debug windfall with no arguments for syntax.\n", ch);
            return;
        }
    }

    std::ostringstream mess;
    mess << std::left;
    unsigned int count(0);
    for (int i(0); i < MAX_KEY_HASH; ++i)
    {
        for (OBJ_INDEX_DATA * obj(obj_index_hash[i]); obj != NULL; obj = obj->next)
        {
            if (IS_OBJ_STAT_EXTRA(obj, ITEM_WINDFALL) == showMarked)
            {
                if (count != 0 && count % 3 == 0) 
                    mess << '\n';

                std::string name(obj->name);
                if (name.size() > 40)
                    name = name.substr(0, 40);

                mess << '[' << std::setw(5) << obj->vnum << "] " << std::setw(40) << name;
                ++count;
            }
        }
    }
    mess << '\n';
    mess << "Total: " << count << "\n";
    page_to_char_bw(mess.str().c_str(), ch);
}

static bool hasReset(OBJ_INDEX_DATA * obj)
{
    for (int i(0); i < MAX_KEY_HASH; ++i)
    {
        for (ROOM_INDEX_DATA * room(room_index_hash[i]); room != NULL; room = room->next)
        {
            for (RESET_DATA * reset(room->reset_first); reset != NULL; reset = reset->next)
            {
                if (reset->command != 'O' && reset->command != 'P' && reset->command != 'G' && reset->command != 'E')
                    continue;

                if (obj->vnum == reset->arg1)
                    return true;
            }
        }
    }

    return false;
}

static void do_debug_windfall_mark(CHAR_DATA * ch, const std::vector<std::string> & args)
{
    bool performMark(false);
    if (args.size() >= 2)
    {
        if (!str_prefix(args[1].c_str(), "run")) 
        {
            performMark = true;
            send_to_char("Auto-marking disabled for now.\n", ch);
            return;
        }
        else
        {
            send_to_char("Unrecognized arguments; use debug windfall with no arguments for syntax.\n", ch);
            return;
        }
    }

    std::ostringstream mess;
    mess << std::left;
    unsigned int count(0);
    for (int i(0); i < MAX_KEY_HASH; ++i)
    {
        for (OBJ_INDEX_DATA * obj(obj_index_hash[i]); obj != NULL; obj = obj->next)
        {
            // Filter out things which are too valuable
            if (obj->level > 51 || (obj->limit_factor <= 3 && obj->limit_factor > 0))
                continue;

            // Filter out items by type
            if (obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_POTIONCONTAINER)
                continue;

            // Filter out items by flag
            if (IS_OBJ_STAT(obj, ITEM_NOPURGE) || IS_OBJ_STAT(obj, ITEM_VIS_DEATH) || IS_OBJ_STAT(obj, ITEM_AFFINITY) 
            || IS_OBJ_STAT(obj, ITEM_QUEST) || IS_OBJ_STAT(obj, ITEM_HIDDEN) || IS_OBJ_STAT(obj, ITEM_STASHED)
            || IS_OBJ_STAT(obj, ITEM_WIZI) || IS_OBJ_STAT(obj, ITEM_NOLONG) || IS_OBJ_STAT_EXTRA(obj, ITEM_NOMORTAL))
                continue;

            // Filter out items by wear loc
            if (!CAN_WEAR(obj, ITEM_TAKE) || CAN_WEAR(obj, ITEM_WEAR_SIGIL) || CAN_WEAR(obj, ITEM_PROG))
                continue;

            // Filter out any object with a % in the string fields
            if (strchr(obj->name, '%') != NULL || strchr(obj->short_descr, '%') != NULL || strchr(obj->description, '%') != NULL)
                continue;
            
            // Filter out any object without a reset
            if (!hasReset(obj))
                continue;

            if (performMark)
            {
                SET_BIT(obj->extra_flags[2], ITEM_WINDFALL);
                SET_BIT(obj->area->area_flags, AREA_CHANGED);
            }

            if (count != 0 && count % 3 == 0) 
                mess << '\n';

            std::string name(obj->name);
            if (name.size() > 40)
                name = name.substr(0, 40);

            mess << '[' << std::setw(5) << obj->vnum << "] " << std::setw(40) << name;
            ++count;
        }
    }
    mess << '\n';
    mess << "Total: " << count << "\n";
    page_to_char_bw(mess.str().c_str(), ch);
}

static void do_debug_windfall(CHAR_DATA * ch, const std::vector<std::string> & args)
{
    // Test for any arguments
    if (args.empty())
    {
        send_to_char("Usage:\n", ch);
        send_to_char("debug windfall list: shows all items marked windfall\n", ch);
        send_to_char("debug windfall list unmarked: shows all items not marked windfall\n", ch);
        send_to_char("debug windfall mark: shows all items which will be marked by the 'mark run' command\n", ch);
        send_to_char("debug windfall mark run: marks items as windfall according to parameters\n", ch);
        return;
    }

    if (!str_prefix(args[0].c_str(), "list")) do_debug_windfall_list(ch, args);
    else if (!str_prefix(args[0].c_str(), "mark")) do_debug_windfall_mark(ch, args);
    else send_to_char("Invalid windfall debug command; use no arguments to get a list of valid command.\n", ch);
}

static void do_debug_faction(CHAR_DATA * ch, const std::vector<std::string> & args)
{
    for (int i = 0; i < MAX_KEY_HASH; i++)
    {
        for (MOB_INDEX_DATA * mob = mob_index_hash[i]; mob; mob = mob->next)
        {
            if (mob->factionNumber == 0)
            {
                mob->factionNumber = Faction::None;
                SET_BIT(mob->area->area_flags, AREA_CHANGED);
            }
        }
    }
}

void do_debug(CHAR_DATA *ch, char *argument)
{
    // Get the first argument
    char arg[MAX_STRING_LENGTH];
    argument = one_argument(argument, arg);

    // Test for any arguments
    if (arg[0] == '\0')
    {
        send_to_char("Options (individual commands may require more arguments):\n", ch);
        send_to_char("debug <no argument>: pulls up this list\n", ch);
        send_to_char("debug memory: shows memory allocations\n", ch);
        send_to_char("debug path: for path testing\n", ch);
        send_to_char("debug drakes: for drakes testing\n", ch);
        send_to_char("debug weather: for weather testing\n", ch);
        send_to_char("debug weave: for weave testing\n", ch);
        send_to_char("debug shades: for shade testing\n", ch);
        send_to_char("debug windfall: for windfall testing\n", ch);
        send_to_char("debug faction: maps all mobs of faction 0 to faction none\n", ch);
        return;
    }

    // Get all the arguments
    std::string command(arg);
    std::vector<std::string> args;
    while (argument[0] != '\0')
    {
        argument = one_argument(argument, arg);
        args.push_back(std::string(arg));
    }

    // Establish a timer before dispatching to the appropriate debug command
    clock_t startTime(clock());
    if (!str_prefix(command.c_str(), "memory")) do_debug_memory(ch, args);
    else if (!str_prefix(command.c_str(), "path")) do_debug_path(ch, args);
    else if (!str_prefix(command.c_str(), "drakes")) do_debug_drakes(ch, args);
    else if (!str_prefix(command.c_str(), "weather")) do_debug_weather(ch, args);
    else if (!str_prefix(command.c_str(), "weave")) do_debug_weave(ch, args);
    else if (!str_prefix(command.c_str(), "shades")) do_debug_shades(ch, args);
    else if (!str_prefix(command.c_str(), "windfall")) do_debug_windfall(ch, args);
    else if (!str_prefix(command.c_str(), "faction")) do_debug_faction(ch, args);
    else
    {
        send_to_char("Invalid debug command; use 'debug' with no command to see a list of valid options.\n", ch);
        return;
    }

    clock_t endTime(clock());
    float seconds((static_cast<float>(endTime) - static_cast<float>(startTime)) / static_cast<float>(CLOCKS_PER_SEC));
    std::ostringstream mess;
    mess << "Run time: " << seconds << " seconds\n";
    send_to_char(mess.str().c_str(), ch);
}

void do_dump( CHAR_DATA *ch, char *argument )
{
    int count,count2,num_pcs,aff_count;
    CHAR_DATA *fch;
    MOB_INDEX_DATA *pMobIndex;
    PC_DATA *pc;
    OBJ_DATA *obj;
    OBJ_INDEX_DATA *pObjIndex;
//    ROOM_INDEX_DATA *room;
//    EXIT_DATA *exit;
    DESCRIPTOR_DATA *d;
    AFFECT_DATA *af;
    FILE *fp;
    int vnum,nMatch = 0;

    /* open file */
    fclose(fpReserve);
    fp = fopen("mem.dmp","w");

    /* report use of data structures */
    
    num_pcs = 0;
    aff_count = 0;

    /* mobile prototypes */
    fprintf(fp,"MobProt	%4d (%8ld bytes)\n",
	top_mob_index, (long) top_mob_index * (sizeof(*pMobIndex))); 

    /* mobs */
    count = 0;  count2 = 0;
    for (fch = char_list; fch != NULL; fch = fch->next)
    {
	count++;
	if (fch->pcdata != NULL)
	    num_pcs++;
	for (af = fch->affected; af != NULL; af = af->next)
	    aff_count++;
    }
    for (fch = char_free; fch != NULL; fch = fch->next)
	count2++;

    fprintf(fp,"Mobs	%4d (%8ld bytes), %2d free (%ld bytes)\n",
	count, (long) count * (sizeof(*fch)), count2, (long) count2 * (sizeof(*fch)));

    /* pcdata */
    count = 0;
    for (pc = pcdata_free; pc != NULL; pc = pc->next)
	count++; 

    fprintf(fp,"Pcdata	%4d (%8ld bytes), %2d free (%ld bytes)\n",
	num_pcs, (long) num_pcs * (sizeof(*pc)), count, (long) count * (sizeof(*pc)));

    /* descriptors */
    count = 0; count2 = 0;
    for (d = descriptor_list; d != NULL; d = d->next)
	count++;
    for (d= descriptor_free; d != NULL; d = d->next)
	count2++;

    fprintf(fp, "Descs	%4d (%8ld bytes), %2d free (%ld bytes)\n",
	count, (long) count * (sizeof(*d)), count2, (long) count2 * (sizeof(*d)));

    /* object prototypes */
    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
        if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
        {
	    for (af = pObjIndex->affected; af != NULL; af = af->next)
		aff_count++;
            nMatch++;
        }

    fprintf(fp,"ObjProt	%4d (%8ld bytes)\n",
	top_obj_index, (long) top_obj_index * (sizeof(*pObjIndex)));


    /* objects */
    count = 0;  count2 = 0;
    for (obj = object_list; obj != NULL; obj = obj->next)
    {
	count++;
	for (af = obj->affected; af != NULL; af = af->next)
	    aff_count++;
    }
    for (obj = obj_free; obj != NULL; obj = obj->next)
	count2++;

    fprintf(fp,"Objs	%4d (%8ld bytes), %2d free (%ld bytes)\n",
	count, (long) count * (sizeof(*obj)), count2, (long) count2 * (sizeof(*obj)));

    /* affects */
    count = 0;
    for (af = affect_free; af != NULL; af = af->next)
	count++;

    fprintf(fp,"Affects	%4d (%8ld bytes), %2d free (%ld bytes)\n",
	aff_count, (long) aff_count * (sizeof(*af)), count, (long) count * (sizeof(*af)));

    /* rooms */
    fprintf(fp,"Rooms	%4d (%8ld bytes)\n",
	top_room, (long) top_room * (sizeof(ROOM_INDEX_DATA)));

     /* exits */
    fprintf(fp,"Exits	%4d (%8ld bytes)\n",
	top_exit, (long) top_exit * (sizeof(EXIT_DATA)));

    fclose(fp);

    /* start printing out mobile data */
    fp = fopen("mob.dmp","w");

    fprintf(fp,"\nMobile Analysis\n");
    fprintf(fp,  "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_mob_index; vnum++)
	if ((pMobIndex = get_mob_index(vnum)) != NULL)
	{
	    nMatch++;
	    fprintf(fp,"#%-4d %3d active %3d killed     %s\n",
		pMobIndex->vnum,pMobIndex->count,
		pMobIndex->killed,pMobIndex->short_descr);
	}
    fclose(fp);

    /* start printing out object data */
    fp = fopen("obj.dmp","w");

    fprintf(fp,"\nObject Analysis\n");
    fprintf(fp,  "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_obj_index; vnum++)
	if ((pObjIndex = get_obj_index(vnum)) != NULL)
	{
	    nMatch++;
	    fprintf(fp,"#%-4d %3d active %3d reset      %s\n",
		pObjIndex->vnum,pObjIndex->count,
		pObjIndex->reset_num,pObjIndex->short_descr);
	}

    /* close file */
    fclose(fp);
    fpReserve = fopen( NULL_FILE, "r" );
}



/*
 * Stick a little fuzz on a number.
 */
int number_fuzzy( int number )
{
    switch ( number_bits( 2 ) )
    {
    case 0:  number -= 1; break;
    case 3:  number += 1; break;
    }

    return UMAX( 1, number );
}



/*
 * Generate a random number.
 */
int number_range( int from, int to )
{
    int power;
    int number;

    if (from == 0 && to == 0)
	return 0;

    if ( ( to = to - from + 1 ) <= 1 )
	return from;

    for ( power = 2; power < to; power <<= 1 )
	;

    while ( ( number = number_mm() & (power -1 ) ) >= to )
	;

    return from + number;
}



/*
 * Generate a percentile roll.
 */
int number_percent( void )
{
    int percent;

    while ( (percent = number_mm() & (128-1) ) > 99 )
	;

    return 1 + percent;
}



/*
 * Generate a random door.
 */
int number_door( void )
{
    int door;

    while ( ( door = number_mm() & (8-1) ) > 5)
	;

    return door;
}

int number_bits( int width )
{
    return number_mm( ) & ( ( 1 << width ) - 1 );
}




/*
 * I've gotten too many bad reports on OS-supplied random number generators.
 * This is the Mitchell-Moore algorithm from Knuth Volume II.
 * Best to leave the constants alone unless you've read Knuth.
 * -- Furey
 */

/* I noticed streaking with this random number generator, so I switched
   back to the system srandom call.  If this doesn't work for you, 
   define OLD_RAND to use the old system -- Alander */

#if defined (OLD_RAND)
static  int     rgiState[2+55];
#endif
 
void init_mm( )
{
#if defined (OLD_RAND)
    int *piState;
    int iState;
 
    piState     = &rgiState[2];
 
    piState[-2] = 55 - 55;
    piState[-1] = 55 - 24;
 
    piState[0]  = ((int) current_time) & ((1 << 30) - 1);
    piState[1]  = 1;
    for ( iState = 2; iState < 55; iState++ )
    {
        piState[iState] = (piState[iState-1] + piState[iState-2])
                        & ((1 << 30) - 1);
    }
#else
    srandom(time(NULL)^getpid());
#endif
    return;
}
 
 
 
long number_mm( void )
{
#if defined (OLD_RAND)
    int *piState;
    int iState1;
    int iState2;
    int iRand;
 
    piState             = &rgiState[2];
    iState1             = piState[-2];
    iState2             = piState[-1];
    iRand               = (piState[iState1] + piState[iState2])
                        & ((1 << 30) - 1);
    piState[iState1]    = iRand;
    if ( ++iState1 == 55 )
        iState1 = 0;
    if ( ++iState2 == 55 )
        iState2 = 0;
    piState[-2]         = iState1;
    piState[-1]         = iState2;
    return iRand >> 6;
#else
#if defined(WIN32)
    return rand() >> 6;
#else
    return random() >> 6;
#endif
#endif
}


/*
 * Roll some dice.
 */
int dice( int number, int size )
{
    int idice;
    int sum;

    switch ( size )
    {
    case 0: return 0;
    case 1: return number;
    }

    for ( idice = 0, sum = 0; idice < number; idice++ )
	sum += number_range( 1, size );

    return sum;
}



/*
 * Simple linear interpolation.
 */
int interpolate( int level, int value_00, int value_32 )
{
    return value_00 + level * (value_32 - value_00) / 32;
}


/* Removes $ from strings.
 * These are not needed and crash in orders
 */
void smash_dollar(char *str)
{
    if (!str || *str == '\0')
	return;
    for ( ; *str != '\0'; str++ )
    {
	if (*str == '$')
	    *str = '4';
    }
    return;
}

/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde( char *str )
{
/*
    if (!str || *str == '\0')
	return;

    for ( ; *str != '\0'; str++ )
    {
	if (*str == '~')
	    *str = '-';
    }
*/
    return;
}

void strip_punc( char *str, char *nopunc )
{
    if (!str || !nopunc)
	return;

    while (str[0] != '\0')
    {
        if (*str != ',' && *str != ';' && *str != ':' && *str != '!'
        && *str != '.' && *str != '?')
	{
	    *nopunc = *str;
	    nopunc++;
	}
	str++;
    }

    *nopunc = '\0';
    return;
}
 
/* nazi checking on input alphanumerics, and spaces*/
void smash_punc( char *str )
{

    if (!str || *str == '\0')
	return;

    for ( ; *str != '\0'; str++ )
    {
        if (!isspace(*str) && *str != ' ' && !isalnum(*str) &&
	*str != ' ' && *str != ',' && *str != ';' && *str != ':'
	&& *str != '!' && *str != '.' && *str != '&' && *str != '%'
	&& *str != '	')
	    *str = '-';
    }

    return;
}

/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns TRUE if astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix( const char *astr, const char *bstr )
{
    int sstr1;
    int sstr2;
    int ichar;
    char c0;

    if ( ( c0 = LOWER(astr[0]) ) == '\0' )
	return FALSE;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    for ( ichar = 0; ichar <= sstr2 - sstr1; ichar++ )
    {
	if ( c0 == LOWER(bstr[ichar]) && !str_prefix( astr, bstr + ichar ) )
	    return FALSE;
    }

    return TRUE;
}



/*
 * Compare strings, case insensitive, for suffix matching.
 * Return TRUE if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix( const char *astr, const char *bstr )
{
    int sstr1;
    int sstr2;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);
    if ( sstr1 <= sstr2 && !str_cmp( astr, bstr + sstr2 - sstr1 ) )
	return FALSE;
    else
	return TRUE;
}



/*
 * Returns an initial-capped string.
 */
char *capitalize( const char *str )
{
    static char strcap[MAX_STRING_LENGTH];
    int i;

    for ( i = 0; str[i] != '\0'; i++ )
	strcap[i] = LOWER(str[i]);
    strcap[i] = '\0';
    strcap[0] = UPPER(strcap[0]);
    return strcap;
}

/*
 * Should capitalize the first non-whitespace of any string -- Pugster
 */
char *front_capitalize( char *str )
{
    *str = UPPER(*str);
    return str;
}


/*
 * Append a string to a file.
 */
void append_file( CHAR_DATA *ch, char *file, char *str )
{
    FILE *fp;

    if ( IS_NPC(ch) || str[0] == '\0' )
	return;

    fclose( fpReserve );
    if ( ( fp = fopen( file, "a" ) ) == NULL )
    {
	perror( file );
	send_to_char( "Could not open the file!\n\r", ch );
    }
    else
    {
	fprintf( fp, "[%5d] %s: %s\n",
	    ch->in_room ? ch->in_room->vnum : 0, ch->name, str );
	fclose( fp );
    }

    fpReserve = fopen( NULL_FILE, "r" );
    return;
}



/*
 * Reports a bug.
 */
void bug( const char *str, int param )
{
    char buf[MAX_STRING_LENGTH];

    if ( fpArea != NULL )
    {
	int iLine;
	int iChar;

	if ( fpArea == stdin )
	{
	    iLine = 0;
	}
	else
	{
	    iChar = ftell( fpArea );
	    fseek( fpArea, 0, 0 );
	    for ( iLine = 0; ftell( fpArea ) < iChar; iLine++ )
	    {
		while ( getc( fpArea ) != '\n' )
		    ;
	    }
	    fseek( fpArea, iChar, 0 );
	}

	sprintf( buf, "[*****] FILE: %s LINE: %d", strArea, iLine );
	log_string( buf );
/* RT removed because we don't want bugs shutting the mud 
	if ( ( fp = fopen( "shutdown.txt", "a" ) ) != NULL )
	{
	    fprintf( fp, "[*****] %s\n", buf );
	    fclose( fp );
	}
*/
    }

    strcpy( buf, "[*****] BUG: " );
    sprintf( buf + strlen(buf), str, param );
    log_string( buf );
    wiznet(buf, NULL, NULL, WIZ_BUGS, 0, 0);
/* RT removed due to bug-file spamming 
    fclose( fpReserve );
    if ( ( fp = fopen( BUG_FILE, "a" ) ) != NULL )
    {
	fprintf( fp, "%s\n", buf );
	fclose( fp );
    }
    fpReserve = fopen( NULL_FILE, "r" );
*/

    return;
}

void write_newbielog( char *str )
{
    char fbuf[MAX_STRING_LENGTH];
    FILE *fp;
  
    sprintf(fbuf, "%s%s", PLAYLOG_DIR, NEWBIE_FILE);

    if ((fp = fopen(fbuf, "a")) == NULL
     && (fp = fopen(fbuf, "w")) == NULL)
	return;
  
    fprintf(fp, "%s\n", str);

    fclose(fp);
    return;
}

void write_slog( CHAR_DATA *ch, char *str )
{
    char fbuf[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char *point;
    FILE *fp;
  
    sprintf(fbuf, "%s%s%s", PLAYLOG_DIR, capitalize(ch->name), ".log");

    if (!(fp = fopen(fbuf, "a")))
	if (!(fp = fopen(fbuf, "w")))
	{
	    sprintf(buf, "Error creating %s.", fbuf);
	    bug(buf, 0);
	    REMOVE_BIT(ch->act, PLR_SLOG);
	    return;
	}
  
    point = buf;

    while ( str[0] != '\0' )
    {
	if ((str[0] != '\r') && (str[0] != '\n'))
	   *point++ = *str;
	str++;
    }

    *point   = '\0';

    fprintf(fp, "%s\n", buf);

    fclose(fp);
    return;
}

/*
 * Writes a string to the log.
 */
void log_string( const char *str )
{
    char *strtime;

    strtime                    = ctime( &current_time );
    strtime[strlen(strtime)-1] = '\0';
    fprintf( stderr, "%s :: %s\n", strtime, str );
    return;
}

/* Snarf a MOBprogram section from the area file.
 */
void load_mobprogs( FILE *fp )
{
  MOB_INDEX_DATA *iMob;
  MPROG_DATA     *original;
  MPROG_DATA     *working;
  char            letter;
  int             value;

  for ( ; ; )
    switch ( letter = fread_letter( fp ) )
    {
    default:
      bug( "Load_mobprogs: bad command '%c'.",letter);
      exit(1);
      break;
    case 'S':
    case 's':
      fread_to_eol( fp );
      return;
    case '*':
      fread_to_eol( fp );
      break;
    case 'M':
    case 'm':
      value = fread_number( fp );
      if ( ( iMob = get_mob_index( value ) ) == NULL )
      {
        bug( "Load_mobprogs: vnum %d doesnt exist", value );
        exit( 1 );
      }

      /* Go to the end of the prog command list if other commands
         exist */

      if ( ( original = iMob->mobprogs ) )
        for ( ; original->next != NULL; original = original->next );

      working = (MPROG_DATA *)alloc_perm( sizeof( MPROG_DATA ) );
	g_num_progs++;
      if ( original )
        original->next = working;
      else
        iMob->mobprogs = working;
      working       = mprog_file_read( fread_word( fp ), working, iMob );
      working->next = NULL;
      fread_to_eol( fp );
      break;
    }

  return;

}

void calc_limit_values()
{
    OBJ_INDEX_DATA *pObjIndex;
    int vnum;
    int nMatch=0;

    log_string("Recalculating limit values.");

    for (vnum = 0; nMatch < top_obj_index; vnum++)
    {
	if (pObjIndex = get_obj_index(vnum))
	{
	    nMatch++;
	    pObjIndex->limit = limit_calc(pObjIndex->limit_factor);
	}
    }
}


int limit_calc(int limit_factor)
{
	return limit_factor;
   /* double temp;
    double t;
    double lf;
    double rounder;

    if (limit_factor == 0)
	return 0;
    if (limit_factor > 0 && limit_factor < 11)
	return limit_factor;

    t = player_levels;
    lf = limit_factor;
    temp = t/50000;
    rounder = lf*temp;

    return (UMAX(1, (int) rounder))+2;*/
}

extern CHAR_DATA *load_offline_char(char *name);

// Combine the function of purge_old_pfiles, get_player_levels,
// calc_limit_values, and rip_obj_pfiles into one.

void pfile_maint(bool loading)
{
    HEADER_DATA *hdp, *hdp_next;
    bool delete_file, crumbled;
    OBJ_DATA *obj, *obj_next, *obj_in;
    OBJ_DATA *cobj,*cobj_next;

    if (loading)
    	log_string("Beginning pfile maintenance (initial loading)...");
    else
	    log_string("Beginning pfile maintenance (daily update)...");

    log_string("Beginning stat emission");
    StatEmitter::EmitStats();
    log_string("Stat emission complete");

    if (loading)
	    player_levels = 0;

    for (hdp = g_active_headers; hdp; hdp = hdp_next)
    {
        hdp_next = hdp->next;

        if (!load_offline_char(hdp->name))
            continue;

        if (ol_char->pcdata->dailyxp == 1)
            ol_char->pcdata->dailyxp = 0;
        // Step 1: Add the player levels (done before purging check since
        //         delete/deny now removes player levels.

        if (loading)
            player_levels += ol_char->level;

        // Step 1: Check if the pfiles needs to be deleted.
        delete_file = FALSE;

        // If delete has been requested...
        if (ol_char->pcdata != NULL && ol_char->pcdata->delete_requested != 0)
        {
            // ...then check for whether a week has passed
            if ((ol_char->pcdata->delete_requested + (NUM_SECONDS_DAY * 7)) < current_time)
                delete_file = TRUE;
        }

        if (IS_PERM_DEAD(ol_char))					/* 2 days */
        {
            if ((ol_char->laston + (NUM_SECONDS_DAY * 2)) < current_time) 
                delete_file = TRUE;
        }
        else if ((ol_char->level >= 1) && (ol_char->level <= 20))	/* 1 month */
        {
            if ((ol_char->laston + (NUM_SECONDS_DAY * 31)) < current_time)
                delete_file = TRUE;
        }
        else if ((ol_char->level >= 21) && (ol_char->level < LEVEL_IMMORTAL)) /* 2m */
        {
            if ((ol_char->laston + (NUM_SECONDS_DAY * 61)) < current_time)
                delete_file = TRUE;
        }

        if (delete_file)
        {
            sprintf(log_buf, "Purging %s due to %s.", ol_char->name, IS_PERM_DEAD(ol_char) ? "death" : "inactivity");
            log_string(log_buf);
            if (ol_char->level < 25)
                delete_char(ol_char);
            else
                deny_char(ol_char);

            continue;
        }

        if (loading)
        {
            for (obj = ol_char->carrying; obj; obj = obj->next_content)
            {
               obj->pIndexData->current += 1;
                for (obj_in = obj->contains; obj_in; obj_in = obj_in->next_content)
                    obj_in->pIndexData->current += 1;
            }
        }

        if (!loading && !IS_IMMORTAL(ol_char) && ol_char->laston > 0 && (current_time - ol_char->lasthour) > (NUM_SECONDS_DAY * 7))
        {
            crumbled = FALSE;
            for (obj = ol_char->carrying; obj; obj = obj_next)
            {
                obj_next = obj->next_content;
                
                if (obj->pIndexData->limit_factor == 0 && obj->item_type != ITEM_CONTAINER || (obj->item_type == ITEM_CONTAINER && obj->contains == NULL))
                    continue;

                if (obj->item_type == ITEM_CONTAINER)
                {
                    for (cobj = obj->contains;cobj;cobj = cobj_next)
                    {
                        cobj_next = cobj->next_content;
                        if (cobj->pIndexData->limit_factor != 0 && crumble_check(cobj,ol_char) && number_bits(1))
                            break;
                    }
                }
                else if (crumble_check(obj,ol_char) && number_bits(1))
                    break;
            }
        }
        save_char_obj(ol_char);
        extract_char(ol_char, TRUE);
        ol_char = NULL;
    }

    calc_limit_values();
    log_string("... maintenance complete.");
}

bool crumble_check(OBJ_DATA *obj, CHAR_DATA *ol_char)
{
    if (!ol_char || !obj)
	return FALSE;
	
    if (number_percent() < ((obj->pIndexData->limit_factor <= 10) ? (20 - obj->pIndexData->limit_factor) : 3))
    {
	sprintf(log_buf, "RECYCLE: %s lost %s", ol_char->name, obj->short_descr);
	log_string(log_buf);

	strcpy(log_buf, ol_char->pcdata->recycle_log);
	strcat(log_buf, obj->short_descr);
	strcat(log_buf, "\n\r");

	free_string(ol_char->pcdata->recycle_log);
	ol_char->pcdata->recycle_log = str_dup(log_buf);

	if (obj->worn_on)
	{
	    unequip_char(ol_char, obj);
	    oprog_remove_trigger(obj);
	}

	extract_obj(obj);
	return TRUE;
    }
    return FALSE;
}	

void purge_old_pfiles()
{
    long ct = (long) current_time;
    FILE *fpList;
    FILE *pfile;
    char pname[MAX_INPUT_LENGTH];
    const char *word(0);
    char charname[13];
    int plevel;
    long ptime;
    bool fMatch;

    if ((fpList = fopen(PLAYERLIST, "r")) == NULL)
    {
	sprintf(log_buf, "purge_old_pfiles: couldn't open %s, aborting.", PLAYERLIST);
	bug(log_buf, 0);
	exit(1);
    }

    for ( ; ; )
    {
	strcpy(charname, fread_word(fpList));

	if (charname[0] == '_')
	{
	    fread_to_eol(fpList);
	    fclose(fpList);
	    return;
	}

	sprintf(pname, "%s%s", PLAYER_DIR, charname);

	if ((pfile = fopen(pname, "r")) == NULL)
	{
	    sprintf(log_buf, "purge_old_pfiles: failed to open %s, continuing.", pname);
	    bug(log_buf, 0);
	    continue;
	}
/*
        sprintf(log_buf, "Opening: %s", charname);
	log_string(log_buf);
*/	
	fMatch = FALSE;
	plevel = 0;
	ptime = 0;
	for ( ; !fMatch ; )
	{
	    word = feof( pfile ) ? "End" : fread_word(pfile);

	    if (feof(pfile))
	    {
		fclose(pfile);
		fMatch = TRUE;
	        break;
	    }

	    if (!word)
	    {
		fclose(pfile);
		sprintf(log_buf, "Error in pfile: %s", strArea);
		bug(log_buf, 0);
		fMatch = TRUE;
		break;
	    }

	    if (!str_cmp(word, "LogO"))
		ptime = fread_number(pfile);
	    else if (!str_cmp(word, "Levl"))
		plevel = fread_number(pfile);
	    else
		fread_to_eol(pfile);

	    if (ptime && plevel)
	    {
		bool delete_file = FALSE;

		if ((plevel >= 1) && (plevel <= 5))		/* 1 week */
		{
		    if ((ptime + (NUM_SECONDS_DAY * 7)) < ct)
			delete_file = TRUE;
		}
		else if ((plevel >= 6) && (plevel <= 10))	/* 2 weeks */
		{
		    if ((ptime + (NUM_SECONDS_DAY * 15)) < ct)
			delete_file = TRUE;
		}
		else if ((plevel >= 11) && (plevel <= 20))	/* 1 month */
		{
		    if ((ptime + (NUM_SECONDS_DAY * 31)) < ct)
			delete_file = TRUE;
		}
		else if ((plevel >= 21) && (plevel < LEVEL_IMMORTAL)) /* 2m */
		{
		    if ((ptime + (NUM_SECONDS_DAY * 61)) < ct)
			delete_file = TRUE;
		}

		if (delete_file)
		{
		    sprintf(log_buf, "Purging %s due to inactivity.", charname);
		    log_string(log_buf);
		    fclose(pfile);
		    remove(pname);
		}
		else
		    fclose(pfile);

		fMatch = TRUE;
		break;

	    }
	}
    }

    return;
}
	    

void get_player_levels()
{
FILE *fpList;
FILE *pfile;
bool fMatch;
int level;
const char *word(0);
char bword[MAX_INPUT_LENGTH];
char strsave[MAX_INPUT_LENGTH];
char strArea[MAX_STRING_LENGTH];

	player_levels = 0;

	word = &bword[0];

        if ( ( fpList = fopen( PLAYERLIST, "r" ) ) == NULL )
        {
	    bug("Error opening player list", 0);
            exit( 1 );
        }

        for ( ; ; )
        {
	    if (!fread_dyn(fpList, &bword[0], MAX_INPUT_LENGTH))
		bug("get_player_levels: error reading player list", 0);

            strcpy( strArea, bword);

            if ( strArea[0] == '_' )
		{
		fread_to_eol(fpList);
		fclose(fpList);
		return;
		}

	sprintf(strsave, "%s%s", PLAYER_DIR, strArea);
	if ( ( pfile = fopen( strsave, "r" ) ) == NULL )
		{
		sprintf(log_buf, "Failed to open pfile: %s", strArea);
		log_string(log_buf);
		continue;
		}

//	sprintf(log_buf, "Processing: %s", strArea);
//	log_string(log_buf);

	fMatch = FALSE;
	for ( ; fMatch == FALSE ; )
		{
		word   = feof( pfile ) ? "End" : fread_dyn( pfile, &bword[0], MAX_INPUT_LENGTH );

		if (!word)
		{
		    sprintf(log_buf, "Error in pfile: %s", strArea);
		    bug(log_buf, 0);
		    break;
		}

		if (!str_cmp( word, "Levl" ))
			{
			level = fread_number(pfile);
			player_levels += level;
			fclose(pfile);
			fMatch=TRUE;
			break;
			}
		else
			{
			fread_to_eol(pfile);
			}
		}
	}

return;
}

void load_obj_rooms()
{
    FILE *fpList;
    FILE *pfile;
    bool fMatch;
    char *word;
    char bword[MAX_INPUT_LENGTH];
    char strsave[MAX_INPUT_LENGTH];
    char strArea[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *room = NULL;
    int rnum;


    word = &bword[0];

    if ( ( fpList = fopen( ROOMSAVELIST, "r" ) ) == NULL )
    {
	bug("Error opening room savelist", 0);
        exit( 1 );
    }

    for ( ; ; )
    {
	if (!fread_dyn(fpList, bword, MAX_INPUT_LENGTH))
	    bug("load_obj_rooms: error reading room savelist.", 0);

        strcpy( strArea, bword);

        if ( strArea[0] == '_' )
	{
	    fread_to_eol(fpList);
	    fclose(fpList);
	    return;
	}

	int num = atoi(strArea);
	sprintf(strsave, "%s%s", ROOMSAVE_DIR, strArea);
	if ((pfile = fopen(strsave, "r")) == NULL)
	{
	    bug("Failed to open roomfile, %d", num);
	    return;
	}
    add_save_room(num);

	fMatch = FALSE;
	for ( ; fMatch == FALSE ; )
	{
	    if (!(word   = fread_dyn( pfile, &bword[0], MAX_INPUT_LENGTH )))
	    {
		sprintf(log_buf, "Error in roomfile, %d", atoi(strArea));
		bug(log_buf, 0);
		break; 
	    }

	    if ((word[0] == '#') && (word[1] == 'O'))
	    {
		fread_to_eol(pfile);
		if ((room = get_room_index(atoi(strArea))) == NULL)
		{
		    bug ("atoi strarea failed", 0);
		    break;
		}
		
		fread_obj_room(room, pfile);
		/*
			word = fread_dyn(pfile, &bword[0], MAX_INPUT_LENGTH);
			vnum = fread_number(pfile);
			fread_to_eol(pfile);
			if ((pObjIndex = get_obj_index(vnum)))
			    pObjIndex->current++;
		*/
	    }
	    else
	    {
		if (!str_cmp(word, "Rnum"))
		{
		    rnum = fread_number(pfile);
		    if ((room = get_room_index(rnum)) == NULL)
		    {
			bug("load_obj_rooms: room 'rnum %d' not found.", rnum);
			break;
		    }
		}
		else if (!str_cmp(word, "Desc") && room)
		{
		    free_string(room->description);
		    room->description = fread_string(pfile);
		}
		else if (!str_cmp(word, "ExDe") && room)
		{
		    EXTRA_DESCR_DATA *ed = (EXTRA_DESCR_DATA*)alloc_perm( sizeof(*ed) );
		    ed->keyword		= fread_string( pfile );
		    ed->description	= fread_string( pfile );
		    ed->next		= room->extra_descr;
		    room->extra_descr	= ed;
		    top_ed++;
		}
		else if (!str_cmp(word, "#DONE"))
		{
		    room = NULL;
		    fread_to_eol(pfile);
		    fclose(pfile);
		    pfile=NULL;
		    fMatch=TRUE;
		}
	    }
	}
        if(pfile)
	    fclose(pfile);
    }
    return;
}

void read_limits()
{
FILE *fp;
OBJ_INDEX_DATA *pObjIndex;
int vnum, limit;
char *word;
char bword[MAX_INPUT_LENGTH];
char buf[1024];

word = &bword[0];

	if ((fp = fopen(LIMITFILE, "r")) == NULL)
	{
	  log_string("Unable to open limit file for data.");
	  return;
	}

	while (1)
	{
		if (!(word   = fread_dyn( fp, &bword[0], MAX_INPUT_LENGTH )))
		{
		    bug("Error in limits.dat file.", 0);
		    exit (1);
		}

		if (word[0] == '#')
		{
		  fclose(fp);
		  return;
		}

		if (!str_cmp(word, "Item"))
		{
		  vnum = fread_number(fp);
		  limit = fread_number(fp);
		  fread_to_eol(fp);
		  if ((pObjIndex = get_obj_index(vnum)) == NULL)
		  {
		    sprintf(buf, "Failed to get index data for vnum %d. Discarded.", vnum);
		    log_string(buf);
		  }
		  else
		    pObjIndex->current = limit;
		}
		else if (!str_cmp(word, "PLevels"))
		{
		  vnum = fread_number(fp);
		  player_levels = vnum;
		  fread_to_eol(fp);
		}
		else
		{
		  bug("limit data in bad format. Dying.", 0);
		  exit(-1);
		}
	}
}


void rip_obj_pfiles()
{
DESCRIPTOR_DATA *d;
OBJ_INDEX_DATA *dObjIndex, *pObjIndex;
OBJ_DATA *pObj;
FILE *fpList;
FILE *pfile;
bool fMatch;
int vnum, nMatch = 0;
char *word;
char bword[MAX_INPUT_LENGTH];
char strArea[MAX_STRING_LENGTH];
char strsave[MAX_INPUT_LENGTH];
char people_on[MAX_STRING_LENGTH];

    log_string("Running object count correction.");

    /* Generate a list of people who are online so items are not  */
    /* double counted. 	- Erinos				  */
    for (d = descriptor_list; d; d = d->next)
        if (d->connected == CON_PLAYING)
        {
            strcat(people_on, d->original ? d->original->name : d->character->name);
	    strcat(people_on, " ");
        }

        for ( vnum = 0; nMatch < top_obj_index; vnum++ )
        {   
            if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
            {   
              nMatch++;
              pObjIndex->current = 0;
            }
        }

	/* count items still IN the world */
	if (post_boot)
	  for ( pObj = object_list; pObj != NULL; pObj = pObj->next)
	    if (pObj->pIndexData->limit_factor > 0)
	      pObj->pIndexData->current++;

        if ( ( fpList = fopen( PLAYERLIST, "r" ) ) == NULL )
        {
	    bug("Error in rip_obj_pfiles opening playerlist", 0);
            exit( 1 );
        }

        for ( ; ; )
        {
	    if (!fread_dyn(fpList, bword, MAX_INPUT_LENGTH))
		bug("rip_obj_pfiles: error reading playerlist.", 0);

            strcpy( strArea, bword);

            if ( strArea[0] == '_' )
		{
		fread_to_eol(fpList);
		fclose(fpList);
		return;
		}

	if (is_name(strArea, people_on))
	{
	  sprintf(log_buf, "%s is on, skipping...", strArea);
	  log_string(log_buf);
	  continue;
	}

	sprintf(strsave, "%s%s", PLAYER_DIR, strArea);
	if ( ( pfile = fopen( strsave, "r" ) ) == NULL )
		{
		sprintf(log_buf, "Failed to open pfile: %s", strArea);
		log_string(log_buf);
		continue;
		}

	fMatch = FALSE;
	for ( ; fMatch == FALSE ; )
		{
		if (!(word   = fread_dyn( pfile, &bword[0], MAX_INPUT_LENGTH )))
		{
		    sprintf(log_buf, "Error in pfile: %s", strArea);
		    bug(log_buf, 0);
		    break;
		}	

		if ((word[0] == '#') && (word[1] == 'O'))
			{
			fread_to_eol(pfile);
			if ((word = fread_dyn(pfile, &bword[0], MAX_INPUT_LENGTH)) == NULL)
			{
			    sprintf(log_buf, "rip_obj_pfiles: error reading pfile '%s'.", strArea);
			    bug(log_buf, 0);
			}
			vnum = fread_number(pfile);
			fread_to_eol(pfile);
			if ((dObjIndex = get_obj_index(vnum)))
			    dObjIndex->current++;
			}
		else
			{
			if (!str_cmp(word, "#END"))
				{
				fread_to_eol(pfile);
				fclose(pfile);
				fMatch=TRUE;
				}
			}
		}


	}
return;
}


/* The new equipment crumbling. MO: Scan through each pfile in the list,
 * and for each, determine the time they've been on. If its recent, just
 * skip. If not, create a list of item vnums that get to go away.
 * If there's deletions going on, go through the objects file by reading
 * up to #O, checking the vnum, and if its ok to pass, writing to end of
 * object. If it needs to be purged, don't write the #O or anything
 * else, just skip down to the next #O. Exception: don't purge containers.
 * I'm not going to code this piece of unholy shit so it takes items that
 * were in a purged container and sets their nest to 0, so we'll
 * skip them until the read code can correct nest 1+ objects following
 * containers automatically.
 *
 * This code makes object reading dependant on vnums being after #O, like
 * rip_obj_pfiles().
 */
void char_equip_crumbling()
{
    OBJ_INDEX_DATA *dObjIndex;
    DESCRIPTOR_DATA *d, *d_next;
    FILE *fpList;
    FILE *pfile;
    FILE *newfile;
    bool fMatch;
    int vnum;
    long laston;
    char *word;
    char *fstring;
    char bword[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char people_on[MAX_STRING_LENGTH];
    char strsave[MAX_INPUT_LENGTH];
    char item_list[MAX_STRING_LENGTH];

  /* blast half-connecteds, to prevent half-loadeds.
   * TODO: fine tune so rolling characters not bombed off
   */
    for (d = descriptor_list; d != NULL; d = d_next)
    {
        d_next = d->next;
        if (d->connected == CON_PLAYING)
        {
            strcat(people_on, " ");
            strcat(people_on, d->original ? d->original->name : d->character->name);
        }
        else
            close_socket(d);
    }

    if ((fpList = fopen(PLAYERLIST, "r")) == NULL)
    {
	bug("Failed to open playerlist.", 0);
	crumble_process = FALSE;
        exit( 1 );
    }

    for ( ; ; )
    {
	if (!fread_dyn(fpList, bword, MAX_INPUT_LENGTH))
	    bug("char_equip_crumbling: error reading playerlist.", 0);

        strcpy( strArea, bword);

        if ( strArea[0] == '_' )
	{
	    fread_to_eol(fpList);
	    fclose(fpList);
	    crumble_process = FALSE;
	    return;
	}

	if (is_name(strArea, people_on))
	{
	    sprintf(log_buf, "%s is on, skipping...", strArea);
	    log_string(log_buf);
	    continue;
	}

        laston = 0;
	sprintf(strsave, "%s%s", PLAYER_DIR, strArea);
	if ((pfile = fopen(strsave, "r")) == NULL)
	{
	    sprintf(log_buf, "Failed to open pfile: %s", strArea);
	    bug(log_buf, 0);
	    continue;
	}

	fMatch = FALSE;
	for ( ; fMatch == FALSE ; )
	{
	    if (!(word   = fread_dyn( pfile, &bword[0], MAX_INPUT_LENGTH )))
	    {
	        sprintf(log_buf, "char_equip_crumbling: error in pfile '%s'.", strArea);
	        bug(log_buf, 0);
	        break;
	    }

	    if ((word[0] == 'L') && (!strncmp(word, "LogO", 4)))
	    {
		laston = fread_number(pfile);
		fread_to_eol(pfile);
		if (laston == 0 || current_time - laston < NUM_SECONDS_DAY * 7)
		{
		    fclose(pfile);
		    break;
		}
	    }

	    if ((word[0] == '#') && (word[1] == 'O'))
	    {
		if (!laston)
		{
		    fclose(pfile);
		    break;
		}

		fread_to_eol(pfile);
		if ((word = fread_dyn(pfile, &bword[0], MAX_INPUT_LENGTH)) == NULL)
		{
		    sprintf(log_buf, "char_equip_crumbling: error reading pfile '%s'.", strArea);
		    bug(log_buf, 0);
		}

		vnum = fread_number(pfile);
		fread_to_eol(pfile);

		if ((dObjIndex = get_obj_index(vnum)) == NULL)
		    continue;

		if (dObjIndex->item_type == ITEM_CONTAINER || dObjIndex->limit_factor == 0)
		    continue; /* do not purge continaers */
		else
		{
		    strcat(item_list, " ");
		    sprintf(buf, " %d", dObjIndex->vnum);
		    strcat(item_list, buf);
		}
	    }
	    else
	    {
		if (!str_cmp(word, "#END"))
		{
		    fread_to_eol(pfile);
		    fclose(pfile);
		    fMatch=TRUE;
		}
	    }
	}

        if (laston == 0 || current_time - laston < NUM_SECONDS_DAY * 7)
	{
	    sprintf(buf, "%s has been on, skipping...", strArea);
	    log_string(buf);
	    continue;
	}
	 
		/* we have now assembled a list of items which
		   are limited and should be considered for removal.
		   I won't count, but you get a chance to lose
		   each copy of a limited item. We'll keep the odds
		   low, since I'm not going to break after one. Yes,
		   kids, you can lose ALL your items in one night if
		   you're sufficiently unlucky. */

        sprintf(buf, "Doing eq purge on %s...", strArea);
	    log_string(buf);
	    if ( ( pfile = fopen( strsave, "r" ) ) == NULL )
		{
		    sprintf(log_buf, "Failed to open pfile: %s", strArea);
		    bug(log_buf, 0);
		    continue;
		}

        if ( ( newfile = fopen( PURGEFILE, "w" ) ) == NULL )
		{
		    sprintf(log_buf, "Failed to open pfile: %s", strArea);
	    	bug(log_buf, 4);
		    continue;
		}

	    fMatch = FALSE;
        for ( ; fMatch == FALSE ; )
	    {
		    if (!(word   = fread_dyn( pfile, &bword[0], MAX_INPUT_LENGTH )))
	    	{
		        sprintf(log_buf, "Error in pfile: %s", strArea);
		        bug(log_buf, 0);
		        break;
	      	}

		    if ((word[0] == '#') && (word[1] == 'O'))
			{
			    fread_to_eol(pfile);
			    if ((word = fread_dyn(pfile, &bword[0], MAX_INPUT_LENGTH)) == NULL)
			    {
				sprintf(log_buf, "char_equip_crumbling: error reading pfile '%s'.", strArea);
				bug(log_buf, 0);
			    }
			    vnum = fread_number(pfile);
			    fread_to_eol(pfile);
			    if ((dObjIndex = get_obj_index(vnum)) == NULL)
			    {
			        if (!skip_eoo(pfile))
				{
				    sprintf(log_buf, "char_equip_crumbling: skip_eoo: error vnum %d, pfile '%s'.", vnum, strArea);
				    bug(log_buf, 0);
				}
			        continue;
			    }
			    sprintf(buf, "%d", dObjIndex->vnum);
			    if (!is_name(buf, item_list))
			    {
			        fprintf(newfile, "#O\nVnum %d\n", dObjIndex->vnum);
			        fwrite_eoo(pfile, newfile);
			        continue;
			    }

			    if (dObjIndex->limit_factor <= 10 && number_percent() < 25)
			    {
			        sprintf(buf, "CRUMBLE: %s lost %s", strArea, dObjIndex->short_descr);
			        log_string(buf);
			        if (!skip_eoo(pfile))
				{
				    sprintf(log_buf, "char_equip_crumbling: skip_eoo: error vnum %d, pfile '%s'.", vnum, strArea);
				    bug(log_buf, 0);
				}
			        continue;
			    }
		  	    else if (number_percent() < 4)
			    {
			        sprintf(buf, "CRUMBLE: %s lost %s", strArea, dObjIndex->short_descr);
			        log_string(buf);
			        if (!skip_eoo(pfile))
				{
				    sprintf(log_buf, "char_equip_crumbling: skip_eoo: error vnum %d, pfile '%s'.", vnum, strArea);
				    bug(log_buf, 0);
				}
			        continue;
			    }
			    else
			    {
			        fprintf(newfile, "#O\nVnum %d\n", dObjIndex->vnum);
			        fwrite_eoo(pfile, newfile);
			        continue;
			    }
			}
			else
			{
			    if (word == '\0')
			        fprintf(newfile, "\n");
			    else
		 	        fprintf(newfile, "%s", word);

			    if (!strncmp(word, "#END", 4))
			    {
			        fprintf(newfile, "%s", fread_store_eol(&buf[0], pfile));
			        break;
			    }
			    else if (!str_cmp(word, "Desc") || !str_cmp(word, "Back"))
		        {
                    // Awful, awful hack to prevent this function from getting rid of linebreaks in
                    // descriptions due to fread_dyn skipping over \n\r with isspace().

				    fstring = fread_string(pfile);
					fprintf(newfile, " %s~\n", fwrite_format(fstring));
				    free_string(fstring);
				}
			    else
			        fprintf(newfile, "%s", fread_store_eol(&buf[0], pfile));
            }
	    }

	    fclose(pfile);
	    fclose(newfile);
	    rename(PURGEFILE, strsave);
	}

    crumble_process = FALSE;
    return;
}

/* skip to end of object (eoo) */
bool skip_eoo(FILE *fp)
{
    char *buf;
    char bword[MAX_INPUT_LENGTH];

    while (1)
    {
	if ((buf = fread_dyn(fp, &bword[0], MAX_INPUT_LENGTH)) == NULL)
	    return FALSE;

	fread_to_eol(fp);

	if (!strncmp(buf, "End", 3))
	    break;
    } 

    return TRUE;
}

/* print the rest of the line into buf 
 * and return a char * to the front */
char *fread_store_eol(char *buf, FILE *fp)
{
    char *start;
    char c;

    start = buf;
    do
    {
        c = fgetc(fp);
        *buf++ = c;
    }
    while (!feof(fp) && c != '\n');

    *buf = '\0';
    return start;
}

/* you're midobject in pfile. Finish writing until #END is written out */
void fwrite_eoo(FILE *pfile, FILE *newfile) {
 char buf[1024];
 char *moo;

 moo = buf;

 while (1) {
   moo = fread_store_eol(moo, pfile);
   fprintf(newfile, "%s", moo);
   if (!strncmp(moo, "End", 3))
   {
     fprintf(newfile, "\n");
     return;
   }
   else if (feof(pfile))
   {
	bug("EOF in fwrite_eoo!", 0);
	fclose(pfile);
   }

 } /*while*/

}
