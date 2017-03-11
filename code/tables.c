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
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "magic.h"

const struct poison_type poison_table[] = 
{
    { "sleep", &gsn_sleeppoison, &sleeppoison },
    { "trembling", &gsn_tremblingpoison, &tremblingpoison },
    { "pain", &gsn_painpoison, &painpoison },
    { "delusion", &gsn_delusionpoison, &delusionpoison },
    { "erosive", &gsn_erosivepoison, &erosivepoison },
    { "exhaustion", &gsn_exhaustionpoison, &exhaustionpoison },
    { "death", &gsn_deathpoison, &deathpoison },
    { "", 0, NULL }
};

const struct trait_group_type trait_groups[] =
{
    { "background",	tgBG	},
    { "general",	tgGen	},
    { "magical",	tgMagic	},
    { "physical",	tgPhys	},
    { NULL,		0	}
};

const struct trait_type trait_table[] =
{
    // Done
    { "ambidextrous",	    TRAIT_AMBIDEXTROUS,	tgPhys,	2,  3,
      "You are ambidextrous.",
      "You are good at balancing your weapons, when fighting with two."	},
    // Done
    { "aquatic nature",	    TRAIT_AQUATIC,	tgPhys,	1,  2,
      "You have an aquatic nature.",
      "You are a surprisingly good swimmer."	},
    // Done
    { "aristocrat",	    TRAIT_ARISTOCRAT,	tgBG,	2,  3,
      "You are an aristocrat.",
      "When you have matured some, you will be granted an inheritance from your estate."   },
    // Done
    { "arcane touch",	    TRAIT_ARCANE_TOUCH,	tgMagic,3,  4,
      "You have an affinity for magical devices.",
      "You have a natural affinity for using staves and wands." },
    // Done
    { "blacksmith",	    TRAIT_BLACKSMITH,	tgBG,	1,  2,
      "You are a blacksmith.",
      "Your weapons and armor require less repair (metal only)."},
    // Done
    { "brave",		    TRAIT_BRAVE,	tgGen,	2,  3,
      "You are brave.",
      "You are braver than most in the face of danger."	},
    // Done
    { "charming",	    TRAIT_CHARMING,	tgPhys,	2,  3,
      "You are charming.",
      "You have the potential to be extraordinarily charismatic."},
    // Done
    { "cold-natured",	    TRAIT_COLDNATURE,	tgPhys,	1,  2,
      "You are cold-natured.",
      "You don't really notice cold weather."	},
    { "cowardly",	    TRAIT_COWARD,	tgGen,	1,  2,
      "You are a coward.",
      "You are a coward, adept at escaping danger."},
    // Done					    
    { "cynic",		    TRAIT_CYNIC,	tgGen,	2,  3,
      "You are a cynic.",
      "You are difficult to fool."    },
    // Done
    { "eagle-eyed",	    TRAIT_EAGLE_EYED,	tgPhys,	2,  3,
      "You are eagle-eyed.",
      "You have sharp vision, allowing you to see further."},
    // Done
    { "endurance",	    TRAIT_ENDURANCE,	tgPhys,	2,  3,
      "You have a strong endurance.",
      "You can go longer without drink and nourishment."    },
    // Done				    
    { "fleet",		    TRAIT_FLEET,	tgPhys,	2,  3,
      "You are fleet.",
      "You are able to grab fallen weapons in combat."	},
    // Done
    { "frugal",		    TRAIT_FRUGAL,	tgGen,	1,  2,
      "You are frugal.",
      "You are quite persuasive when it comes to bartering with merchants."  },
    // Done
    { "gifted",		    TRAIT_GIFTED,	tgMagic,1,  2,
      "You are magically gifted.",
      "You may attempt to cast spells you do not have the energy for."  },
    { "heirloom",	    TRAIT_HEIRLOOM,	tgBG,	3,  4,
      "You own a family heirloom.",
      "You are the owner of a family heirloom." },
    // Done
    { "hollow leg",	    TRAIT_HOLLOWLEG,	tgPhys,	1,  2,
      "You have a hollow leg.",
      "You can eat more than most before you get full."	},
    // Done
    { "iron stomach",	    TRAIT_IRONSTOMACH,	tgPhys,	1,  2,
      "You have an iron stomach.",
      "You can eat almost anything without becoming ill."},
    // Done
    { "linguistic",	    TRAIT_LINGUIST,	tgGen,	1,  2,
      "You are a linguist.",
      "You can more easily hear the subtle variations in different languages, and learn them at a faster rate than most people."    },
    // Done
    { "long-lived",	    TRAIT_LONGLIFE,	tgPhys, 1,  2,
      "You will be longer lived than others of your race.",
      "You're likely to live longer than most of your race."},
    // Done
    { "loyal retainer",	    TRAIT_RETAINER,	tgBG,	3,  4,
      "You have a loyal retainer.",
      "At level 10, you gain the ability to call a loyal retainer."},
    // Done
    { "magic aptitude",	    TRAIT_MAGAPT,	tgMagic,1,  2,
      "You have a magical aptitude.",
      "The initial magical energy required for your spells is less."},
    // Done
    { "armored caster",		    TRAIT_NIMBLE,	tgPhys,	1,  3,
      "You are versed in armored casting.",
      "You are versed in casting spells with armor on, and will never be lightly-encumbered." },
    // Done
    { "obscure",	    TRAIT_OBSCURE,	tgMagic,1,  2,
      "You can obscure your spellcasting.",
      "No one can make out the arcane words used in casting your spells."},
    // Done
    { "pack horse",	    TRAIT_PACK_HORSE,	tgPhys,	2,  3,
      "You are a pack horse.",
      "You can carry more than someone of your size and strength usually could."	    },
    // Done
    { "pious",		    TRAIT_PIOUS,	tgMagic,1,  2,
      "You are pious.",
      "Recall until level 25. After level 25, a greater chance of divine notice to your sacrifices."	},
    // Done
    { "streetwise",	    TRAIT_STREETWISE,	tgBG,	2,  3,
      "You are streetwise.",
      "A life spent growing up on the streets has left you wise to many common tricks."	    },
    // Done
    { "survivor",	    TRAIT_SURVIVOR,	tgPhys,	1,  2,
      "You are a survivor.",
      "You have a firm grasp on the material world, and will resist your final death longer than others."   },
    
    { "marked",		    TRAIT_MARKED,	-1,	0,  5,
      "You have been marked by your god.",
      ""    },

    { "timeless",	    TRAIT_TIMELESS,	-1,	0,  5,
      "You resist the effects of advanced age.",
      ""    },

    { "poison resistant",   TRAIT_POISONRES,	-1,	0,  5,
      "You are immune to poisons created with catalyst.",
      ""    },

    { "sword mastery", 	    TRAIT_SWORDMASTERY,	-1,	0,  5,
      "You can dual wield any two swords.",
      ""    },

    { "exotic mastery",     TRAIT_EXOTICMASTERY,	-1,	0,  5,
      "You are skilled with exotic weapons.",
      ""    },

    { "critical eye", 	    TRAIT_CRITICAL,	-1,	0,  5,
      "You can quickly tell when you make mistakes.",
      ""    },

    { "light sleeper",	    TRAIT_LIGHTSLEEPER,	tgPhys,	1,  2,
      "You are a light sleeper.",
      "You can sense nearby activity while sleeping."	},

    { "thieves cant",	    TRAIT_THIEVESCANT, tgBG, 1, 2,
      "You are skilled in thieves cant.",
      "You know the silent hand signal language of thieves."  },

    { "mortician",	    TRAIT_MORTICIAN, tgBG, 1, 2,
      "You are a skilled in dissection.",
      "You are quite skilled at dismembering and dissecting corpses."  },
 
    { NULL }	
};

// Update do_checkfollow (god_buf) if we put in a name longer than 8
// IF YOU ARE ADDING TO THIS GOD TABLE:
// 		1. Put your new god-trait information in the table.  The
// 		   TRUE/FALSE bit describes if this offers a godboon or not.
// 		2. Put a void immfunc [godname] in act_wiz.c.
// 		3. Put a declarator in tables.h.
const struct god_type god_table[] =
{
    { "Aeolis",		&immfunc_aeolis,    TRUE,   ALIGN_GOOD,
      "You notice $E has a pinkish rose birthmark.", APPLY_SAVES,
      "angelic grace" },
    { "Alajial",	&immfunc_alajial,   TRUE,   ALIGN_GOOD,
      "You sense a serene air about $M.", APPLY_RESIST_HOLY,
      "peaceful solace"	},
    { "Arkhural",	&immfunc_arkhural,  TRUE,   ALIGN_EVIL,
      "You notice $S teeth are unusually sharp, and stained with blood.",
      APPLY_DAMROLL, "force of havoc"	},
    { "Ashur",		&immfunc_ashur,	    TRUE,  ALIGN_EVIL,
      "", APPLY_NONE, "" },
    { "Bayyal",		&immfunc_bayyal,    TRUE,   ALIGN_EVIL,
      "You notice $E has a terrible burn scar.", APPLY_RESIST_FIRE,
      "blessing of the flames"	},
    { "Calaera",	&immfunc_calaera,   TRUE,	ALIGN_GOOD,
      "You sense an honest air about $M.", APPLY_RESIST_NEGATIVE,
      "shield of judgement" },
    { "Chadraln",	&immfunc_chadraln,  TRUE,   ALIGN_NEUTRAL,
      "You feel a scholarly air about $M.", APPLY_MANA,
      "insight of the seer"	},
    { "Dolgrael",	&immfunc_dolgrael,  TRUE,   ALIGN_NEUTRAL,
      "You notice $E has a hammer-shaped birthmark.", APPLY_HIT,
      "strength of the forge"	},
    { "Lilune",		&immfunc_lilune,    TRUE,   ALIGN_EVIL,
      "You notice $E has a black orchid birthmark.", APPLY_MANA,
      "moonlight inspiration" },
    { "Girikha",	&immfunc_girikha,   TRUE,   ALIGN_NEUTRAL,
      "You notice $E has sharp, pronounced canines.", APPLY_MOVE,
      "stamina of the hunter"	},
    { "Iandir",		&immfunc_iandir,    TRUE,   ALIGN_NEUTRAL,
      "You notice $E has an unusually steely gaze.", APPLY_RESIST_WEAPON,
      "protection of the crown" },
    { "Jolinn",		&immfunc_jolinn,    TRUE,   ALIGN_GOOD,
      "You notice $E has bright blue eyes.", APPLY_RESIST_FIRE,
      "ward of the father"	},
    { "Rveyelhi",	&immfunc_rveyelhi,  TRUE,   ALIGN_EVIL,
      "You notice $S skin is unnaturally pale, aged beyond its years.", 
      APPLY_RESIST_LIGHT, "ward of the oppressor"	},
    { "Rystaia",	&immfunc_rystaia,   TRUE,   ALIGN_GOOD,
      "You notice $E has sparkling golden eyes.", APPLY_SAVES,
      "will of the lightbringer"	},
    { "Serachel",	&immfunc_serachel,  TRUE,   ALIGN_EVIL,
      "", APPLY_SAVES, "angelic will" },
    { "Sythrak",	&immfunc_sythrak,   TRUE,  ALIGN_EVIL,
      "You notice a swath of brilliant crimson scales about $S shoulders.",
      APPLY_DAMROLL, "saurian strength" },
    { "Tzajai",		&immfunc_tzajai,    TRUE,  ALIGN_NEUTRAL,
      "You sense a decidedly capricious air about $M.", -1,
      "fortune" },
    { "Elar",		&immfunc_elar,	    TRUE,	ALIGN_NEUTRAL,
      "You notice $E seems particularly vivacious.", APPLY_HIT,
      "touch of nature"	},
    { "Lielqan",	&immfunc_lielqan,   FALSE,	ALIGN_NEUTRAL,
      "", APPLY_SAVES, "unbroken path" },
    { "Alil",		&immfunc_alil,	    TRUE,	ALIGN_NEUTRAL,
      "You notice $E has vibrant crimson eyes.", APPLY_CHR,
      "angelic allure"	},
    { "Vaialos",	&immfunc_vaialos,   TRUE,	ALIGN_GOOD,
      "", APPLY_NONE, "" },
    { "Sitheus",	&immfunc_sitheus,   TRUE,	ALIGN_EVIL,
      "You notice $E's body is tinged with a deathly pallor.", 
      APPLY_RESIST_HOLY, "ward of spite"},
    { "Khanval",	&immfunc_khanval,   FALSE,	ALIGN_EVIL,
      "", APPLY_DAMROLL, "barbarous strength" },
    { "Arikanja",	&immfunc_arikanja,  FALSE,	ALIGN_GOOD,
      "", APPLY_DAMROLL, "desert fury" },
    { "Ayaunj",		&immfunc_ayaunj,    FALSE,	ALIGN_NEUTRAL,
      "", -1, "prosperity" },
    { "Fenthira",	&immfunc_fenthira,  FALSE,	ALIGN_NEUTRAL,
      "", APPLY_HIT, "spirit of survival" },
    { "Nariel",		&immfunc_nariel,    FALSE,  ALIGN_GOOD,
      "", APPLY_HITROLL, "seeking arrow"},
    { "Enirra",		&immfunc_enirra,    TRUE,  ALIGN_NEUTRAL,
      "You notice $E has triangle-shaped pupils in $S eyes.", 
      APPLY_RESIST_MENTAL, "will of balance"},
    { "Jalassa",	&immfunc_jalassa,   TRUE,  ALIGN_NEUTRAL,
      "You sense a focused air around $M.", APPLY_MANA,
      "wisdom of the arbiter" }
};

const struct coin_type coin_table[MAX_COIN] =
{
    { "platinum",	"p",	1000	},
    { "gold",		"g",	100	},
    { "silver",		"s",	10	},
    { "copper",		"c",	1	}
};


/* for clans */
const struct clan_type clan_table[MAX_CLAN] =
{
    /* independent should be FALSE if is a real clan */

    {	"",		"", 	"",	ROOM_VNUM_ALTAR,   0,	TRUE	},
    {	"the Guardians of the Law",		"GUARDIAN",
        "[GUARDIAN] ",	ROOM_VAULT_GUARDIAN,	12007,		FALSE	},
    {	"the Coven of the Shunned",		"SHUNNED",
        "[SHUNNED] ",	ROOM_VAULT_SHUNNED,	0,		FALSE	},
    {	"the Wanderers of the Outland",		"WANDERER",
        "[WANDERER] ",	ROOM_VNUM_ALTAR,	0,		FALSE	},
    {	"the Champions of the Light",		"CHAMPION",
	"[CHAMPION] ",	ROOM_VAULT_CHAMPION,	12062,		FALSE	},
    {	"the Raiders of Twilight",		"RAIDER",
	"[RAIDER] ",	ROOM_VAULT_RAIDER,	12114,		FALSE	},
    {	"the Seekers of Knowledge",		"SEEKER",
	"[SEEKER] ",	ROOM_VNUM_ALTAR,	0,		FALSE	},
    {	"the Merchants of Avendar",		"MERCHANT",
	"[MERCHANT] ",	ROOM_VNUM_ALTAR,	0,		FALSE	},
    {	"the Lords of Conquest",		"CONQUEST",
	"[CONQUEST] ",	ROOM_VAULT_CONQUEST,	0,		FALSE	},
    {	"the Knights of Balance",		"KNIGHT",
	"[KNIGHT] ",	ROOM_VNUM_ALTAR,	0,		FALSE	},
};

const struct clanskill_type clanskill_table[MAX_CLAN] =
{
/* name, skill1, skill2, skill3, skill4, skill5 */
    {   "",	"", 	"", 	"", 	"", 	"" },
    {   "GUARDIAN",	"criminal","perception","aegis of law",
    			"call guards","locate criminal" },
    {   "SHUNNED",	"shadow ward","demonic might","soul reaver",
    			"aura of corruption", "mantle of fear" },
    {   "WANDERER",	"", 	"", 	"", 	"", 	"" },
    {   "CHAMPION",	"strength of Aramril","holy avenger","reveal",
    			"scourge of darkness","soulblade" },
    {   "RAIDER",	"escape","belt of looters","hooded rogue",
    			"pillage","bird of prey" },
    {   "SEEKER",	"", 	"", 	"", 	"", 	"" },
    {   "MERCHANT",	"", 	"", 	"", 	"", 	"" },
    {   "CONQUEST",	"dedication","intimidate", 
    			"matrix","division" },
    {	"KNIGHT",	"",	"",	"",	"",	"" },
};

/* for position */
const struct position_type position_table[] =
{
    {	"dead",			"dead"	},
    {	"mortally wounded",	"mort"	},
    {	"incapacitated",	"incap"	},
    {	"stunned",		"stun"	},
    {	"sleeping",		"sleep"	},
    {	"resting",		"rest"	},
    {   "sitting",		"sit"   },
    {	"fighting",		"fight"	},
    {	"standing",		"stand"	},
    {	NULL,			NULL	}
};

const struct channel_type channel_table[] =
{
    { 	CHAN_SAY, CTO_ROOM,	TRUE,
	"{YYou say, '$t'{x",		"{YYou say, in %s, '$t'{x",
	"{Y$N says, '$t'{x",		"{Y$N says, in %s, '$t'{x",
	NULL,				NULL
    },
    {	CHAN_OOC, CTO_VICT,	TRUE,
	"{rOOC to $N: $t{x",		"{rOOC, in %s, to $N: $t{x",
	NULL,				NULL,
	"{r[OOC] $N: $t{x",		"{r[OOC (%s)] $N: $t{x"
    },
    {	CHAN_YELL, CTO_AREA,	TRUE,
	"{WYou yell, {c'$t'{x",		"{WYou yell, in %s, {c'$t'{x",
	"{W$N yells, {c'$t'{x",		"{W$N yells, in %s, {c'$t'{x",
	"{W$N yells sharply, {c'$t'{x",  "{W$N yells sharply, in %s, {c'$t'{x"
    },
    {   CHAN_CYELL, CTO_AREA,	TRUE,
	"{WYou yell, {c'$t'{x",		"{WYou yell, in %s, {c'$t'{x",
	"{W$N yells, {c'$t'{x",		"{W$N yells, in %s, {c'$t'{x",
	NULL,				NULL
    },
    {	CHAN_MUSIC, CTO_AREA,	TRUE,
	"{cYou music, '{C$t{c'{x",	"{cYou music, in %s, '{C$t{c'{x",
	"{c$N musics, '{C$t{c'{x",	"{c$N musics, in %s, '{C$t{c'{x",
	NULL,				NULL
    },
    {	CHAN_DRUID, CTO_CLASS,	FALSE,
	"{g[{GDruid - $N{g]: {y$t{x",	"{g[{GDruid - $N{g]: {y$t{x",
	"{g[{GDruid - $N{g]: {y$t{x",	"{g[{GDruid - $N{g]: {y$t{x",
	NULL,				NULL
    },
    {   CHAN_HOUSE, CTO_HOUSE,	FALSE,
	"{w[{W%s{w] $N: $t{x",		"{w[{W%s{w] $N: $t{x",
	"{w[{W%s{w] $N: $t{x",		"{w[{W%s{w] $N: $t{x",
	NULL,				NULL
    },
    {	CHAN_AUCTION, CTO_AREA,	TRUE,
	"{yYou auction, '$t'{x",		"{yYou auction, in %s, '$t'{x",
	"{y$N auctions, '$t'{x",		"{y$N auctions, in %s, '$t'{x",
	NULL,				NULL
    },
    {	CHAN_QUESTION, CTO_AREA,TRUE,
	"{yYou question, '$t'{x",	"{yYou question, in %s, '$t'{x",
	"{y$N questions, '$t'{x",	"{y$N questions, in %s, '$t'{x",
	NULL,				NULL
    },
    {	CHAN_ANSWER, CTO_AREA,	TRUE,
	"{yYou answer, '$t'{x",		"{yYou answer, in %s, '$t'{x",
	"{y$N answers, '$t'{x",		"{y$N answers, in %s, '$t'{x",
	NULL,				NULL
    },
    {	CHAN_SING, CTO_ROOM,	TRUE,
	"{YYou sing, '$t'{x",		"{YYou sing, in %s, '$t'{x",
	"{Y$N sings, '$t'{x",		"{Y$N sings, in %s, '$t'{x",
	NULL,				NULL
    },
    {	CHAN_THINK, CTO_OTHER,	FALSE,
	"{mYou think, '$t'{x",		"{mYou think, '$t'{x",
	"Roleplay: $N thinks, '$t'",	"Roleplay: $N thinks, '$t'",
	NULL,				NULL
    },
    {	CHAN_GTELL, CTO_OTHER,	TRUE,
	"{mYou tell the group, '{M$t{m'{x",	"{mYou tell the group, in %s, '{M$t{m'{x",
	"{m$N tells the group, '{M$t{m'{x",	"{m$N tells the group, in %s, '{M$t{m'{x",
	NULL
    },
    {	CHAN_TELL, CTO_VICT,	TRUE,
	"{gYou tell $N, '$t'{x",		"{gYou tell $N, in %s, '$t'{x",
	NULL,				NULL,
	"{g$N tells you, '$t'{x",	"{g$N tells you, in %s, '$t'{x"
    },
    {	CHAN_ESAY, CTO_ROOM,	TRUE,
	"{Y$N %s, '$t'{x",		"{Y$N %s, in %s, '$t'{x",
	"{Y$N %s, '$t'{x",		"{Y$N %s, in %s, '$t'{x",
	NULL,				NULL
    },
    {	-1, -1, FALSE, NULL, NULL, NULL, NULL, NULL, NULL }
};

const struct herb_type herb_table[] =
{
    {	"fela mushroom",	A,	100,	1,	24,
	{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ &gsn_slow_cure, 	NULL,	NULL, 	NULL }
    },
    {	"tuva bark",		B,	80,	1,	24,
	{ 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_slow_cure_ser,	NULL,	NULL,	NULL	}
    },
    {	"goka tuber",		C,	75,	1,	16,
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
      	{ &gsn_slow_cure_crit,	NULL,	NULL,	NULL	}
    },
    {	"qorith leaves",	D,	80,	1,	16,
	{ 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_cure_light,	NULL,	NULL,	NULL	}
    },
    {	"nkran blossom",	E,	75,	1,	12,
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
      	{ &gsn_cure_serious,	NULL,	NULL,	NULL	}
    },
    {	"krasa root",		F,	50,	1,	32,
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
      	{ &gsn_slow_cure_poison,NULL,	NULL,	NULL	}
    },
    {	"suspirha flower",	G,	90,	30,	32,
	{ 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_cure_light,	NULL,	NULL,	NULL	}
    },
    {	"acujira petal",	H,	85,	30,	32,
	{ 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_cure_serious,	NULL,	NULL,	NULL	}
    },
    {	"prickly arylaria leaf",I,	50,	30,	24,
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
      	{ &gsn_cure_critical,	NULL,	NULL,	NULL	}
    },
    {	"ssirth vine",		J,	30,	30,	48,
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
      	{ &gsn_cure_poison,	NULL,	NULL,	NULL	}
    },
    {	"ilkra toadstools",	K,	15,	30,	24,
	{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_slow_cure_disease,	NULL,	NULL,	NULL	}
    },
    {	"flower of Laeren",	L,	5,	30,	16,
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_slow_heal,	NULL,	NULL,	NULL	}
    },
    {	"llaen root",		M,	100,	51,	100,
	{ 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_cure_light,	NULL,	NULL,	NULL	}
    },
    {	"vin blossom",		N,	95,	51,	72,
	{ 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_cure_serious,	NULL,	NULL,	NULL	}
    },
    {	"xel petals",		O,	80,	51,	48,
	{ 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_cure_critical,	NULL,	NULL,	NULL	}
    },
    {	"black belidiss blossom",P,	50,	51,	96,
	{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_cure_poison,	NULL,	NULL,	NULL	}
    },
    {	"soala leaves",		Q,	30,	51,	72,
	{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_cure_disease,	NULL,	NULL,	NULL	}
    },
    {	"yala blossom",		R,	5,	51,	24,
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_heal,	NULL,	NULL,	NULL	}
    },
    {	"flower of Caelyra",	S,	1,	51,	16,
	{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_haste,	NULL,	NULL,	NULL	}
    },
    {	"violet belidiss blossom",T,	7,	51,	32,
	{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_resist_poison,	NULL,	NULL,	NULL	}
    },
    {	"scarlet belidiss blossom",U,	1,	51,	32,
	{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_protectionfrompoison,	NULL,	NULL,	NULL	}
    },
    {	"nru tuber",		V,	3,	51,	48,
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_protectionfromcold,	NULL,	NULL,	NULL	}
    },
    {	"rkala blossom",	W,	3,	51,	48,
	{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{ &gsn_protectionfromfire,	NULL,	NULL,	NULL	}
    },
    {	NULL,		0,	0,	0,	0,
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      	{	NULL,	NULL,	NULL,	NULL	}
    }
 
};

/* seasonal data */
const struct season_type season_table[] =
{
    {	"summer",	
	81,	    12,	6,	20,	0,
	-30,	0,	5,	0	},
    {	"autumn",	
	177,	0,	6,	20,	15,
	30,	10,	20,	1	},
    {	"winter",	
	270,	-12,	6,	20,	5,
	50,	0,	-5,	-10	},
    {	"spring",	
	369,	0,	6,	20,	15,
	-15,	10,	20,	1	},
    {   NULL,		
	0,	0,	0,	0,	0,
	0,	0,	0,	0	}
};

/* Was originally just going to store the start day for each month, 	*/
/* but ended up just putting days per month for easier expansion. -E	*/
const struct month_type month_table[] =
{
    {	"The Dawning"		,32	, false},
    {	"The Day of Quickening"	,1	, false},
    {	"Nelennamir"		,32	, false},
    {	"Tyrilis"		,30	, false},
    {	"The Burning"		,32	, false},
    {	"The Dragons' Feast"	,1	, true},
    {	"Converumir"		,32	, false},
    {	"Rystaiamir"		,15	, false},
    {	"The Reaping"		,15	, false},
    {	"The Setting"		,32	, false},
    {	"The Day of Tears"	,1	, false},
    {	"Kyanamir"		,32	, false},
    {	"Valectis"		,30	, false},
    {   "The Sundering"		,32	, false},
    {	"The Hearth Feast"	,1	, false},
    {	"Elanthemir"		,32	, false},
    {	"Lyceonis"		,15	, false},
    {	"Year's End"		,15	, false},
    {   "The Day of Year's End"	,1	, false},
    {	NULL			,0	, false}
};

const struct geo_type geo_table[] =
{
    {	"ocean",	
	A,	10,	2,	20,	15,	10,	5	},
    {	"coastal",	
	B,	5,	1,	15,	10,	5,	5	},
    {	"mountains",	
	C,	10,	1,	10,	5,	10,	-10	},
    {   "plains",	
	D,	-5,	0,	0,	-10,	0,	0	},
    {   "lowlands",	
	E,	-5,	1,	5,	5,	0,	5	},
    {	"desert",	
	F,	15,	-2,	-50,	-50,	0,	-50	},
    {   "river",	
	G,	0,	1,	0,	5,	0,	0	},
    {	NULL,		
	0,	0,	0,	0,	0,	0,	0	}
};

/* for arrows */
const struct arrow_type arrow_table[] =
{
   { "short",   0    },
   { "medium",  0    },
   { "long",    0    },
   { "barbed",  1    },
   { NULL,      -1 }
};

/* for sex */
const struct sex_type sex_table[] =
{
   {	"none"		},
   {	"male"		},
   {	"female"	},
   {	"either"	},
   {	NULL		}
};

/* for sizes */
const struct size_type size_table[] =
{ 
    {	"tiny"		},
    {	"small" 	},
    {	"medium"	},
    {	"large"		},
    {	"huge", 	},
    {	"giant" 	},
    {	NULL		}
};

const struct home_type home_table[MAX_HOMETOWN] =
{
    {   "Var Bandor",		3499,	0,	0	},  /* Default */
    {   "Var Bandor",		3491,   1,	7	},  /* Good Default */
    {   "Var Bandor",		3492,   2,	7	},  /* Neutral Def. */ 
    {   "Var Bandor",		3500,	4,	7	},  /* Evil Default */
    {   "Earendam",		7099,	1,	7	},
    {   "Earendam",		7369,   2,	7	},
    {   "Earendam",		7161,   4,	7	},
    {   "Krilin",		1587,	3,	7	},
    {   "Nendor",	       19657,	7,	7	},
    {	"Qilarn",		1462,	7,	7	},
    {	"Hakurah Monastery",   10642,	7,	3	},
    {   "Kor Thrandir",	       17218,	1,	7	},		
};

/* various flag tables */
const struct flag_type nact_flags[] =
{
    {   "bank",			A,	TRUE	},
    {	"omnilingual",		B,	TRUE	},
    {	"psycho",		C,	TRUE	},
    {   "dealer",		D,	FALSE	},
    {   "pacific",		E,	TRUE	},
    {   "charmrand",		F,	TRUE	},
    {	"classed",		G,	TRUE	},
    {   "norescue",		H,	TRUE	},
    {   "mount",		I,	TRUE	},
    {   "smith",		J,	TRUE	},
    {   "nodisarm",		K,	TRUE	},
    {   "track_fast",		L,	TRUE	},
    {   "track_open",		M,	TRUE	},
    {   "track_ram",		N,	TRUE	},
    {	"keepgold",		O,	TRUE	},
    {	"faction_leader",	P,	TRUE	},
    {	"faction_elite",	Q,	TRUE	},
    {   "shade",        R,  TRUE    },
    {	NULL,			0,      FALSE	}
};

const struct inscribe_type inscribe_table[] =
{
    {	A,	"Trigon of Binding"		},
    {   B,	"Pentagram of Summoning"	},
    {	C,	"Circle of Protection"		},
    {	D,	"Maze of Isetaton"		},
    {   E,	"Blasphemous Sigil of Nygothua"	},
    {   F,	"Spiral of Bahhaoth"		},
    {   G,	"Logorin Star"			},
    {   H,	"Angles of Selb-Kar"		},
    {   I,	"Seal of the Dragon"		},
    {   J,	"Eye of Xthjich"		},
    {	K,	"Mark of the Damned"		},
    {   L,	"Tetragon of the Dead"		},
    {   M,	"Vakalic Sign"			},
    {   N,	"Lost Cipher of Pnakur"		},
    {   O,	"Vkoren Configuration"		},
    {   P,	"Sigil of Logor"		},
    {	Q,	"Scar of Gagaroth"		},
    {	R,	"Tear of Pricina"		},
    {	S,	"Mad Etchings of Kyalee"	},
    {	T,	"Cracks of Xixzyr"		},
    {   U,	"Crest of Chaigidon"		},
    {	V,	"Aklaju Hieroglyph"		},
    {   W,	"Crest of Khamurn"		},
    {	0,	NULL				}
};


const struct flag_type act_flags[] =
{
    {	"npc",			A,	FALSE	},
    {	"sentinel",		B,	TRUE	},
    {	"scavenger",		C,	TRUE	},
    {   "notrack",		D,	TRUE	},
    {   "track_gate",		E,	TRUE	},
    {	"aggressive",		F,	TRUE	},
    {	"stay_area",		G,	TRUE	},
    {	"wimpy",		H,	TRUE	},
    {	"pet",			I,	TRUE	},
    {	"train",		J,	TRUE	},
    {	"practice",		K,	TRUE	},
    {	"nowander",		L,	TRUE	},
    { 	"guildguard",		M,	TRUE	},
    {   "animal",		N,	TRUE	},
    {	"undead",		O,	TRUE	},
    {   "badass",		P,	TRUE	},
    {	"modified",		Q,	TRUE	},
    {	"illusion",		R,	TRUE	},
    {	"thief",		S,	TRUE	},
    {	"warrior",		T,	TRUE	},
    {	"noalign",		U,	TRUE	},
    {	"nopurge",		V,	TRUE	},
    {	"outdoors",		W,	TRUE	},
    {	"nosubdue",		X,	TRUE	},
    {	"indoors",		Y,	TRUE	},
    {   "guardian",		Z,	TRUE	},
    {	"healer",		aa,	TRUE	},
    {	"gain",			bb,	TRUE	},
    {	"update_always",	cc,	TRUE	},
    {	"changer",		dd,	TRUE	},
    {   "familiar",		ee,	TRUE	},
    {	NULL,			0,	FALSE	}
};

const struct flag_type plr_flags[] =
{
    {	"npc",			PLR_IS_NPC,	FALSE	},
    {	"autoattack",		PLR_AUTOATTACK,	FALSE	},
    {	"autodefend",		PLR_AUTODEFEND, FALSE	},
    {	"autoassist",		PLR_AUTOASSIST,	FALSE	},
    {	"autoexit",		PLR_AUTOEXIT,	FALSE	},
    {	"autoloot",		PLR_AUTOLOOT,	FALSE	},
    {	"autodestroy",		PLR_AUTODES,	FALSE	},
    {	"autogold",		PLR_AUTOGOLD,	FALSE	},
    {	"autosplit",		PLR_AUTOSPLIT,	FALSE	},
    {   "learn",		PLR_LEARN,	FALSE	},
    {   "epnote",		PLR_EPNOTE,	TRUE	},
    {   "autodate",		PLR_AUTODATE,	TRUE	},
    {	"holylight",		PLR_HOLYLIGHT,	FALSE	},
    {   "account",		PLR_ACCOUNT,	FALSE	},
    {	"can_loot",		PLR_CANLOOT,	FALSE	},
    {	"nosummon",		PLR_NOSUMMON,	FALSE	},
    {	"nofollow",		PLR_NOFOLLOW,	TRUE	},
    {	"colour",		PLR_COLOUR,	FALSE	},
    {	"permit",		PLR_PERMIT,	TRUE	},
    {   "slog",			PLR_SLOG,	TRUE   },
    {	"log",			PLR_LOG,	FALSE	},
    {	"deny",			PLR_DENY,	FALSE	},
    {	"freeze",		PLR_FREEZE,	TRUE	},
    {	"nopk",			PLR_NOPK,	FALSE	},
    {	"reward",		PLR_REWARD,	FALSE	},
    {   "induct",		PLR_INDUCT,	TRUE	},
    {   "sound",		PLR_SOUND,	FALSE   },
    {   "display",		PLR_DISPLAY,	TRUE    },
    {   "showdam",		PLR_SHOWDAM,	TRUE    },
	{   "showlines",    PLR_SHOWLINES,  FALSE   },
    {	NULL,			0,		0	}
};

const struct flag_type nplr_flags[] =
{
    {   "mercy_death",		PLR_MERCY_DEATH,	TRUE	},
    {   "mercy_beg",		PLR_MERCY_BEG,		TRUE	},
    {	"mercy_force",		PLR_MERCY_FORCE,	TRUE	},
    {	"noweather",		PLR_NOWEATHER,		TRUE	},
    {   "noteify",		PLR_NOTEIFY,		TRUE	},
    {	"showdata",		PLR_SHOWDATA,		TRUE	},
    {	"noyell",		PLR_NOYELL,		TRUE	},
    {	"hardcore",		PLR_HARDCORE,		FALSE	},
    {	"noaccept",		PLR_NOACCEPT,		TRUE	},
    {	NULL,			0,			0	},
};

const struct flag_type affect_flags[] =
{
    {	"blind",		A,	TRUE	},
    {	"invisible",		B,	TRUE	},
    {	"detect_evil",		C,	TRUE	},
    {	"detect_invis",		D,	TRUE	},
    {	"fly_natural",		E,	TRUE	},
    {	"detect_hidden",	F,	TRUE	},
    {	"detect_good",		G,	TRUE	},
    {	"sanctuary",		H,	TRUE	},
    {	"coronal_glow",		I,	TRUE	},
    {	"infrared",		J,	TRUE	},
    {	"curse",		K,	TRUE	},
    {   "wizi",			L, 	TRUE	},
    {	"poison",		M,	TRUE	},
    {	"protect_evil",		N,	TRUE	},
    {	"protect_good",		O,	TRUE	},
    {	"sneak",		P,	TRUE	},
    {	"hide",			Q,	TRUE	},
    {	"sleep",		R,	TRUE	},
    {	"charm",		S,	TRUE	},
    {	"flying",		T,	TRUE	},
    {	"pass_door",		U,	TRUE	},
    {	"haste",		V,	TRUE	},
    {	"calm",			W,	TRUE	},
    {	"plague",		X,	TRUE	},
    {	"weaken",		Y,	TRUE	},
    {	"dark_vision",		Z,	TRUE	},
    {	"berserk",		aa,	TRUE	},
    {	"swim",			bb,	TRUE	},
    {	"regeneration",		cc,	TRUE	},
    {	"slow",			dd,	TRUE	},
    {   "detect_wizi",		ee,	TRUE	},
    {   "freeagent",    ff, TRUE},
    {	NULL,			0,	0	}
};

const struct flag_type naffect_flags[] =
{
    {	"avatar",		A,	TRUE	},
    {	"flesh to stone",	B,	TRUE	},
    {	"guarding",		C,	TRUE	},
    {	"cloudkill",		D,	TRUE	},
    {	"rally",		E,	TRUE	},
    {	"forced march",		F,	TRUE	},
    {	"scouting",		G,	TRUE	},
    {	"larva",		H,	TRUE	},
    {	"armshatter",		I,	TRUE	},
    {	"legshatter",		J,	TRUE	},
    {	"grapple",		K,	TRUE	},
    {   "rage",			L, 	TRUE	},
    {	"fury",			M,	TRUE	},
    {	"om nom nom",		N,	TRUE	},
    {	"garrotee",		O,	TRUE	},
    {	"garroter",		P,	TRUE	},
    {	"garrote le tired",	Q,	TRUE	},
    {	"muffle",		R,	TRUE	},
    {	"tremble",		S,	TRUE	},
    {	"delusion",		T,	TRUE	},
    {	"light sleep",		U,	TRUE	},
    {	"wariness",		V,	TRUE	},
    {	"flashpowder",		W,	TRUE	},
    {	"bolo arm",		X,	TRUE	},
    {	"bolo leg",		Y,	TRUE	},
    {	"bolo",			Z,	TRUE	},
    {	"pursue",		aa,	TRUE	},
    {	"agility",		bb,	TRUE	},
    {	"submissioner",		cc,	TRUE	},
    {	"submissionee",		dd,	TRUE	},
    {   "subdue",		ee,	TRUE	},
    {	NULL,			0,	0	}
};

const struct flag_type oaffect_flags[] =
{
    {	"encamp",		A,	TRUE	},
    {	"shadow mastery",	B,	TRUE	},
    {	"encase",		C,	TRUE	},
    {	"rite of dawn",		D,	TRUE	},
    {	"inspire",		E,	TRUE	},
    {	"petrify",		F,	TRUE	},
    {	"beastform",		G,	TRUE	},
    {	"mantle of fear",	H,	TRUE	},
    {	"eyefocus",		I,	TRUE	},
    {	"blink",		J,	TRUE	},
    {	"disguise",		K,	TRUE	},
    {   "paranoia",		L, 	TRUE	},
    {	"symbiont",		M,	TRUE	},
    {	"read thoughts",	N,	TRUE	},
    {	"symbiontee",		O,	TRUE	},
    {	"coven",		P,	TRUE	},
    {	"bleeding",		Q,	TRUE	},
    {	"burnout",		R,	TRUE	},
    {	"nova charge",		S,	TRUE	},
    {	"consumption",		T,	TRUE	},
    {	"demon possession",	U,	TRUE	},
    {	"positive channel",	V,	TRUE	},
    {	"radiance",		W,	TRUE	},
    {	"inscribe",		X,	TRUE	},
    {	"deafen",		Y,	TRUE	},
    {	"ghost",		Z,	TRUE	},
    {	"unclean spirit",	aa,	TRUE	},
    {	"dark future",		bb,	TRUE	},
    {	"one-handed",		cc,	TRUE	},
    {	"nightfears",		dd,	TRUE	},
    {   "doppel",		ee,	TRUE	},
    {	NULL,			0,	0	}
};

const struct flag_type paffect_flags[] =
{
    {	"airless",		A,	TRUE	},
    {	"sharp vision",		B,	TRUE	},
    {	"sensory vision",	C,	TRUE	},
    {	"voidwalk",		D,	TRUE	},
    {	"crystalize magic",	E,	TRUE	},
    {	"obscure evidence",	F,	TRUE	},
    {	"mute",			G,	TRUE	},
    {	"aura of corruption",	H,	TRUE	},
    {	"shroud of nyogthua",	I,	TRUE	},
    {	"endure",		J,	TRUE	},
    {	"dash",			K,	TRUE	},
    {   "twohand",		L, 	TRUE	},
    {	"trueshot",		M,	TRUE	},
    {	"unused",		N,	TRUE	},
    {	"maul",			O,	TRUE	},
    {	"unused",		P,	TRUE	},
    {	"unused",		Q,	TRUE	},
    {	"unused",		R,	TRUE	},
    {	"unused",		S,	TRUE	},
    {	"unused",		T,	TRUE	},
    {	"unused",		U,	TRUE	},
    {	"unused",		V,	TRUE	},
    {	"unused",		W,	TRUE	},
    {	"unused",		X,	TRUE	},
    {	"unused",		Y,	TRUE	},
    {	"unused",		Z,	TRUE	},
    {	"unused",		aa,	TRUE	},
    {	"unused",		bb,	TRUE	},
    {	"unused",		cc,	TRUE	},
    {	"unused",		dd,	TRUE	},
    {	"unused",		ee,	TRUE	},
    {	NULL,			0,	0	}
};

const struct flag_type off_flags[] =
{
    {	"area_attack",		A,	TRUE	},
    {	"backstab",		B,	TRUE	},
    {	"bash",			C,	TRUE	},
    {	"berserk",		D,	TRUE	},
    {	"disarm",		E,	TRUE	},
    {	"dodge",		F,	TRUE	},
    {	"fade",			G,	TRUE	},
    {	"fast",			H,	TRUE	},
    {	"kick",			I,	TRUE	},
    {	"dirt_kick",		J,	TRUE	},
    {	"parry",		K,	TRUE	},
    {	"rescue",		L,	TRUE	},
    {	"tail",			M,	TRUE	},
    {	"trip",			N,	TRUE	},
    {	"crush",		O,	TRUE	},
    {	"assist_all",		P,	TRUE	},
    {	"assist_align",		Q,	TRUE	},
    {	"assist_race",		R,	TRUE	},
    {	"assist_players",	S,	TRUE	},
    {	"assist_guard",		T,	TRUE	},
    {	"assist_vnum",		U,	TRUE	},
    {   "bite",			V,	TRUE	},
    {	"assist_bouncer",	W,	TRUE	},
    {	"assist_npcrace",	aa,	TRUE	},
    {	NULL,			0,	0	}
};

const struct flag_type lang_flags[] =
{
    {	"common",		A,	TRUE	},
    {	"alatharya",		B,	TRUE	},
    {	"srryn",		C,	TRUE	},
    {	"kankoran",		D,	TRUE	},
    {	"ch'taren",		E,	TRUE	},
    {	"aelin",		F,	TRUE	},
    {	"shuddeni",		G,	TRUE	},
    {	"nefortu",		H,	TRUE	},
    {	"caladaran",		I,	TRUE	},
    {	"ethron",		J,	TRUE	},
    {	"arcane",		K,	TRUE	},
    {	NULL,			0,	0	}
};

const struct flag_type imm_flags[] =
{
    {	"summon",		A,	TRUE	},
    {	"charm",		B,	TRUE	},
    {	"magic",		C,	TRUE	},
    {	"weapon",		D,	TRUE	},
    {	"bash",			E,	TRUE	},
    {	"pierce",		F,	TRUE	},
    {	"slash",		G,	TRUE	},
    {	"fire",			H,	TRUE	},
    {	"cold",			I,	TRUE	},
    {	"lightning",		J,	TRUE	},
    {	"acid",			K,	TRUE	},
    {	"poison",		L,	TRUE	},
    {	"negative",		M,	TRUE	},
    {	"holy",			N,	TRUE	},
    {	"energy",		O,	TRUE	},
    {	"mental",		P,	TRUE	},
    {	"disease",		Q,	TRUE	},
    {	"drowning",		R,	TRUE	},
    {	"light",		S,	TRUE	},
    {	"sound",		T,	TRUE	},
    {   "illusion",		U,	TRUE	},
    {   "defilement",	V,	TRUE	},
    {	"fear",			W,	TRUE	},
    {	"wood",			X,	TRUE	},
    {	"silver",		Y,	TRUE	},
    {	"iron",			Z,	TRUE	},
    {	"tame",			aa,	TRUE	},
    {	"blind",		bb,	TRUE	},
    {	NULL,			0,	0	}
};

const struct flag_type damtype_table[]	=
{
    {	"none",			DAM_NONE,	TRUE 	},
    {	"bash",			DAM_BASH,	TRUE	},
    {	"pierce",		DAM_PIERCE,	TRUE	},
    {	"slash",		DAM_SLASH,	TRUE	},
    {	"fire",			DAM_FIRE,	TRUE	},
    {	"cold",			DAM_COLD,	TRUE	},
    {	"lightning",		DAM_LIGHTNING,	TRUE	},
    {	"acid",			DAM_ACID,	TRUE	},
    {	"poison",		DAM_POISON,	TRUE	},
    {	"negative",		DAM_NEGATIVE,	TRUE	},
    {	"holy",			DAM_HOLY,	TRUE	},
    {	"energy",		DAM_ENERGY,	TRUE	},
    {	"mental",		DAM_MENTAL,	TRUE	},
    {	"disease",		DAM_DISEASE,	TRUE	},
    {	"drowning",		DAM_DROWNING,	TRUE	},
    {	"light",		DAM_LIGHT,	TRUE	},
    {	"other",		DAM_OTHER,	TRUE	},
    {	"fear",			DAM_FEAR,	TRUE	},
    {	"charm",		DAM_CHARM,	TRUE	},
    {	"sound",		DAM_SOUND,	TRUE	},
    {	"illusion",		DAM_ILLUSION,	TRUE	},
    {	"defilement",		DAM_DEFILEMENT,	TRUE	},
    {	NULL,			0,		0	}
};

const struct flag_type form_flags[] =
{
    {	"edible",		FORM_EDIBLE,		TRUE	},
    {	"poison",		FORM_POISON,		TRUE	},
    {	"magical",		FORM_MAGICAL,		TRUE	},
    {	"instant_decay",	FORM_INSTANT_DECAY,	TRUE	},
    {	"other",		FORM_OTHER,		TRUE	},
    {	"animal",		FORM_ANIMAL,		TRUE	},
    {	"sentient",		FORM_SENTIENT,		TRUE	},
    {	"undead",		FORM_UNDEAD,		TRUE	},
    {	"construct",		FORM_CONSTRUCT,		TRUE	},
    {	"mist",			FORM_MIST,		TRUE	},
    {	"intangible",		FORM_INTANGIBLE,	TRUE	},
    {	"biped",		FORM_BIPED,		TRUE	},
    {	"centaur",		FORM_CENTAUR,		TRUE	},
    {	"insect",		FORM_INSECT,		TRUE	},
    {	"spider",		FORM_SPIDER,		TRUE	},
    {	"crustacean",		FORM_CRUSTACEAN,	TRUE	},
    {	"worm",			FORM_WORM,		TRUE	},
    {	"blob",			FORM_BLOB,		TRUE	},
    {	"mammal",		FORM_MAMMAL,		TRUE	},
    {	"bird",			FORM_BIRD,		TRUE	},
    {	"reptile",		FORM_REPTILE,		TRUE	},
    {	"snake",		FORM_SNAKE,		TRUE	},
    {	"dragon",		FORM_DRAGON,		TRUE	},
    {	"amphibian",		FORM_AMPHIBIAN,		TRUE	},
    {	"fish",			FORM_FISH ,		TRUE	},
    {	"cold_blood",		FORM_COLD_BLOOD,	TRUE	},
    {   "quadruped",		FORM_QUADRUPED,		TRUE	},
    {	NULL,			0,			0	}
};

const struct flag_type part_flags[] =
{
    {	"head",			PART_HEAD,		TRUE	},
    {	"arms",			PART_ARMS,		TRUE	},
    {	"legs",			PART_LEGS,		TRUE	},
    {	"heart",		PART_HEART,		TRUE	},
    {	"brains",		PART_BRAINS,		TRUE	},
    {	"guts",			PART_GUTS,		TRUE	},
    {	"hands",		PART_HANDS,		TRUE	},
    {	"feet",			PART_FEET,		TRUE	},
    {	"fingers",		PART_FINGERS,		TRUE	},
    {	"ear",			PART_EAR,		TRUE	},
    {	"eye",			PART_EYE,		TRUE	},
    {	"long_tongue",		PART_LONG_TONGUE,	TRUE	},
    {	"eyestalks",		PART_EYESTALKS,		TRUE	},
    {	"tentacles",		PART_TENTACLES,		TRUE	},
    {	"fins",			PART_FINS,		TRUE	},
    {	"wings",		PART_WINGS,		TRUE	},
    {	"tail",			PART_TAIL,		TRUE	},
    {	"claws",		PART_CLAWS,		TRUE	},
    {	"fangs",		PART_FANGS,		TRUE	},
    {	"horns",		PART_HORNS,		TRUE	},
    {	"scales",		PART_SCALES,		TRUE	},
    {	"tusks",		PART_TUSKS,		TRUE	},
    {	NULL,			0,			0	}
};

const struct flag_type comm_flags[] =
{
    {	"quiet",		COMM_QUIET,		TRUE	},
    {   "deaf",			COMM_DEAF,		TRUE	},
    {   "nowiz",		COMM_NOWIZ,		TRUE	},
    {   "noclangossip",		COMM_NOAUCTION,		TRUE	},
    {   "nogossip",		COMM_NOGOSSIP,		TRUE	},
    {   "noquestion",		COMM_NOQUESTION,	TRUE	},
    {   "nomusic",		COMM_NOMUSIC,		TRUE	},
    {   "noclan",		COMM_NOCLAN,		TRUE	},
    {   "noquote",		COMM_NOQUOTE,		TRUE	},
    {   "shoutsoff",		COMM_SHOUTSOFF,		TRUE	},
    {   "compact",		COMM_COMPACT,		TRUE	},
    {   "brief",		COMM_BRIEF,		TRUE	},
    {   "prompt",		COMM_PROMPT,		TRUE	},
    {   "combine",		COMM_COMBINE,		TRUE	},
    {   "telnet_ga",		COMM_TELNET_GA,		TRUE	},
    {   "show_affects",		COMM_SHOW_AFFECTS,	TRUE	},
    {   "nograts",		COMM_NOGRATS,		TRUE	},
    {   "noemote",		COMM_NOEMOTE,		FALSE	},
    {   "noshout",		COMM_NOSHOUT,		FALSE	},
    {   "notell",		COMM_NOTELL,		FALSE	},
    {   "nochannels",		COMM_NOCHANNELS,	FALSE	},
    {   "snoop_proof",		COMM_SNOOP_PROOF,	FALSE	},
    {   "afk",			COMM_AFK,		TRUE	},
    {   "nonewbie",		COMM_NONEWBIE,		TRUE	},
    {	NULL,			0,			0	}
};

const struct flag_type area_flags[] =
{
    {	"none",			AREA_NONE,		FALSE	},
    {	"changed",		AREA_CHANGED,		TRUE	},
    {	"added",		AREA_ADDED,		TRUE	},
    {	"loading",		AREA_LOADING,		FALSE	},
    {   "uncomplete",		AREA_UNCOMPLETE,	TRUE	},
    {	NULL,			0,			0	}
};

const struct flag_type ainfo_flags[] =
{
    {   "newres",		AINFO_NEWRES,		FALSE	},
    {   "avnums",		AINFO_AVNUMS,		FALSE	},
    {   "invis",		AINFO_INVIS,		TRUE	},
    {   "mobclass",		AINFO_MOBCLASS,		FALSE	},
    {   "noworldlist",		AINFO_NOWORDLIST,	FALSE	},
    {	NULL,			0,			0	}
};

const struct flag_type sex_flags[] =
{
    {	"male",			SEX_MALE,		TRUE	},
    {	"female",		SEX_FEMALE,		TRUE	},
    {	"neutral",		SEX_NEUTRAL,		TRUE	},
    {   "random",               3,                      TRUE    },   /* ROM */
    {	"none",			SEX_NEUTRAL,		TRUE	},
    {	NULL,			0,			0	}
};


const struct flag_type acct_flags[] =
{
    {   "no_ban",		ACCT_NO_BAN,		TRUE    },
    {	"denied",		ACCT_DENY,		TRUE	},
    {	"mentor",		ACCT_MENTOR,		TRUE	},
    {	NULL,			0,			0	}
};


const struct flag_type exit_flags[] =
{
    {   "door",			EX_ISDOOR,		TRUE    },
    {	"closed",		EX_CLOSED,		TRUE	},
    {	"locked",		EX_LOCKED,		TRUE	},
    {	"secret",		EX_SECRET,		TRUE	},
    {	"pickproof",		EX_PICKPROOF,		TRUE	},
    {   "nopass",		EX_NOPASS,		TRUE	},
    {   "easy",			EX_EASY,		TRUE	},
    {   "hard",			EX_HARD,		TRUE	},
    {	"infuriating",		EX_INFURIATING,		TRUE	},
    {	"noclose",		EX_NOCLOSE,		TRUE	},
    {	"nolock",		EX_NOLOCK,		TRUE	},
    {   "illusionary", 		EX_ILLUSION,		TRUE	},
    {   "walled", 		EX_WALLED,		TRUE	},
    {	"runed",		EX_RUNEOFEARTH,		TRUE	},
    {   "walloffire", 		EX_WALLOFFIRE,		TRUE	},
    {   "noram", 		EX_NORAM,		TRUE	},
    {   "fake", 		EX_FAKE,		TRUE	},
    {	"norefresh",		EX_NOREFRESH,		TRUE	},
    {	"noflee",		EX_NOFLEE,		TRUE	},
    {	NULL,			0,			0	}
};



const struct flag_type door_resets[] =
{
    {	"open and unlocked",	0,		TRUE	},
    {	"closed and unlocked",	1,		TRUE	},
    {	"closed and locked",	2,		TRUE	},
    {	NULL,			0,		0	}
};



const struct flag_type room_flags[] =
{
    {	"dark",			ROOM_DARK,		TRUE	},
    {	"nogate",		ROOM_NOGATE,		TRUE	},
    {	"no_mob",		ROOM_NO_MOB,		TRUE	},
    {	"indoors",		ROOM_INDOORS,		TRUE	},
    {	"nosum_to",		ROOM_NOSUM_TO,		TRUE	},
    {	"nosum_from",		ROOM_NOSUM_FROM,	TRUE	},
    {	"noneforyou", 		ROOM_NONEFORYOU,	TRUE	},
    {	"vault", 		ROOM_VAULT,		TRUE	},
    {	"laboratory",		ROOM_LABORATORY,	TRUE	},
    {   "noyell", 		ROOM_NOYELL,		TRUE	},
    {	"private",		ROOM_PRIVATE,		TRUE    },
    {	"safe",			ROOM_SAFE,		TRUE	},
    {	"solitary",		ROOM_SOLITARY,		TRUE	},
    {	"pet_shop",		ROOM_PET_SHOP,		TRUE	},
    {	"no_recall",		ROOM_NO_RECALL,		TRUE	},
    {	"imp_only",		ROOM_IMP_ONLY,		TRUE    },
    {	"gods_only",	        ROOM_GODS_ONLY,		TRUE    },
    {	"heroes_only",		ROOM_HEROES_ONLY,	TRUE	},
    {	"newbies_only",		ROOM_NEWBIES_ONLY,	TRUE	},
    {	"law",			ROOM_LAW,		TRUE	},
    {   "nowhere",		ROOM_NOWHERE,		TRUE	},
    {   "nomagic", 		ROOM_NOMAGIC,		TRUE	},
    {	"guild",		ROOM_GUILD, 		TRUE	},
    {	"noweather",		ROOM_NOWEATHER,		TRUE	},
    {	"uberdark",		ROOM_UBERDARK,		TRUE	},
    {   "power_nexus",		ROOM_POWER_NEXUS,	TRUE	},
    {	"rough",		ROOM_ROUGH,		TRUE	},
    {   "has_water",		ROOM_HAS_WATER,		TRUE	},
    {   "save",			ROOM_SAVE,		TRUE	},
    {   "arena",		ROOM_ARENA,		TRUE 	},
    {	NULL,			0,			0	}
};



const struct flag_type sector_flags[] =
{
    {	"inside",	SECT_INSIDE,		TRUE	},
    {	"city",		SECT_CITY,		TRUE	},
    {	"field",	SECT_FIELD,		TRUE	},
    {	"forest",	SECT_FOREST,		TRUE	},
    {	"hills",	SECT_HILLS,		TRUE	},
    {	"mountain",	SECT_MOUNTAIN,		TRUE	},
    {	"swim",		SECT_WATER_SWIM,	TRUE	},
    {	"noswim",	SECT_WATER_NOSWIM,	TRUE	},
    {   "unused",	SECT_UNUSED,		TRUE	},
    {	"air",		SECT_AIR,		TRUE	},
    {	"desert",	SECT_DESERT,		TRUE	},
    {   "underwater",	SECT_UNDERWATER,	TRUE	},
    {   "underground",	SECT_UNDERGROUND,	TRUE	},
    {   "road",		SECT_ROAD,		TRUE	},
    {   "swamp",	SECT_SWAMP,		TRUE	},
    {	NULL,		0,			0	}
};


const struct flag_type precip_flags[] =
{
    {   "barren",		PRECIP_BARREN,		TRUE	},
    {   "arid",			PRECIP_ARID,		TRUE    },
    {   "average",		PRECIP_AVERAGE,		TRUE	},
    {   "wet",			PRECIP_WET,		TRUE    },
    {   "monsoon",		PRECIP_MONSOON,		TRUE	},
    {   NULL,			0,			0	},
};

const struct flag_type temp_flags[] =
{
    {   "blistering",		TEMP_BLISTERING,	TRUE    },
    {   "hot",			TEMP_HOT,		TRUE    },
    {   "temperate",		TEMP_TEMPERATE,		TRUE    },
    {   "cool",			TEMP_COOL,		TRUE    },
    {   "frigid",		TEMP_FRIGID,		TRUE    },
    {   NULL,			0,			0	}
};

const struct flag_type wind_mag_flags[] =
{
    {   "gale",			WMAG_GALE,		TRUE    },
    {   "windy",		WMAG_WINDY,		TRUE    },
    {   "normal",		WMAG_NORMAL,		TRUE    },
    {   "still",		WMAG_STILL,		TRUE    },
    {   "doldrums",		WMAG_DOLDRUMS,		TRUE    },
    {   NULL,			0,			0	}
};

const struct flag_type geo_flags[] =
{
    {   "ocean",		GEO_OCEAN,		TRUE    },
    {   "coastal",		GEO_COASTAL,		TRUE    },
    {   "mountainous",		GEO_MOUNTAINOUS,	TRUE    },
    {   "plains",		GEO_PLAINS,		TRUE    },
    {   "lowlands",		GEO_LOWLANDS,		TRUE    },
    {   "desert",		GEO_DESERT,		TRUE    },
    {   "river",		GEO_RIVER,		TRUE	},
    {   NULL,			0,			0	}
};

const struct flag_type instrument_table[] =
{
    {   "percussion",		INST_TYPE_PERC,		TRUE	},
    {   "string",		INST_TYPE_STRING,	TRUE	},
    {   "woodwind",		INST_TYPE_WOODWIND,	TRUE	},
    {   "brass",		INST_TYPE_BRASS,	TRUE	},
    {	NULL,			0,			0	}
};

const struct flag_type spec_obj_types[] =
{
    {	"chessboard",		SOBJ_CHESS,		TRUE	},
    {   "cards",		SOBJ_CARDS,		TRUE	},
    {   NULL,			0,			0	}
};

const struct flag_type type_flags[] =
{
    {	"light",		ITEM_LIGHT,		TRUE	},
    {	"scroll",		ITEM_SCROLL,		TRUE	},
    {	"wand",			ITEM_WAND,		TRUE	},
    {	"staff",		ITEM_STAFF,		TRUE	},
    {	"weapon",		ITEM_WEAPON,		TRUE	},
    {	"treasure",		ITEM_TREASURE,		TRUE	},
    {	"armor",		ITEM_ARMOR,		TRUE	},
    {	"potion",		ITEM_POTION,		TRUE	},
    {   "oil",             	ITEM_OIL,           	TRUE    },
    {	"furniture",		ITEM_FURNITURE,		TRUE	},
    {	"trash",		ITEM_TRASH,		TRUE	},
    {	"container",		ITEM_CONTAINER,		TRUE	},
    {	"drinkcontainer",	ITEM_DRINK_CON,		TRUE	},
    {	"key",			ITEM_KEY,		TRUE	},
    {	"food",			ITEM_FOOD,		TRUE	},
    {	"money",		ITEM_MONEY,		TRUE	},
    {	"boat",			ITEM_BOAT,		TRUE	},
    {	"npccorpse",		ITEM_CORPSE_NPC,	TRUE	},
    {	"pc corpse",		ITEM_CORPSE_PC,		FALSE	},
    {	"fountain",		ITEM_FOUNTAIN,		TRUE	},
    {	"pill",			ITEM_PILL,		TRUE	},
    {	"protect",		ITEM_PROTECT,		TRUE	},
    {	"map",			ITEM_MAP,		TRUE	},
    {   "portal",		ITEM_PORTAL,		TRUE	},
    {   "warpstone",		ITEM_WARP_STONE,	TRUE	},
    {	"roomkey",		ITEM_ROOM_KEY,		TRUE	},
    { 	"gem",			ITEM_GEM,		TRUE	},
    {	"jewelry",		ITEM_JEWELRY,		TRUE	},
    {	"jukebox",		ITEM_JUKEBOX,		TRUE	},
    {	"instrument",		ITEM_INSTRUMENT,	TRUE	},
    {	"net",  		ITEM_NET,		TRUE	},
    {   "arrow",                ITEM_ARROW,             TRUE    },
    {   "bow",                  ITEM_BOW,               TRUE    },
    {   "special",		ITEM_SPECIAL,		TRUE	},
    {   "writing",		ITEM_WRITING,		TRUE	},
    {	"potioncontainer",	ITEM_POTIONCONTAINER,	TRUE	},
    {	NULL,			0,			0	}
};


const struct flag_type extra_flags[][33] =
{ {
    {	"glow",			ITEM_GLOW,		TRUE	},
    {	"hum",			ITEM_HUM,		TRUE	},
    {	"dark",			ITEM_DARK,		TRUE	},
    {	"warm",			ITEM_WARM,		TRUE	},
    {	"evil",			ITEM_EVIL,		TRUE	},
    {	"invis",		ITEM_INVIS,		TRUE	},
    {	"magic",		ITEM_MAGIC,		TRUE	},
    {	"nodrop",		ITEM_NODROP,		TRUE	},

    {	"bless",		ITEM_BLESS,		TRUE	},
    {	"antigood",		ITEM_ANTI_GOOD,		TRUE	},
    {	"antievil",		ITEM_ANTI_EVIL,		TRUE	},
    {	"antineutral",		ITEM_ANTI_NEUTRAL,	TRUE	},
    {	"noremove",		ITEM_NOREMOVE,		TRUE	},
    {	"inventory",		ITEM_INVENTORY,		TRUE	},
    {	"nopurge",		ITEM_NOPURGE,		TRUE	},
    {	"rotdeath",		ITEM_ROT_DEATH,		TRUE	},

    {	"visdeath",		ITEM_VIS_DEATH,		TRUE	},
    {   "affinity",		ITEM_AFFINITY,		TRUE	},
    {   "nonmetal",		ITEM_NONMETAL,		TRUE	},
    {	"nolocate",		ITEM_NOLOCATE,		TRUE	},
    {	"meltdrop",		ITEM_MELT_DROP,		TRUE	},
    {	"hadtimer",		ITEM_HAD_TIMER,		TRUE	},
    {	"sellextract",		ITEM_SELL_EXTRACT,	TRUE	},
    {   "quest",		ITEM_QUEST,		TRUE	},

    {	"burnproof",		ITEM_BURN_PROOF,	TRUE	},
    {	"nouncurse",		ITEM_NOUNCURSE,		TRUE	},
    {	"nodestroy",		ITEM_NODESTROY,		TRUE 	},
    {	"concealed",		ITEM_HIDDEN,		TRUE	},
    {	"stashed",		ITEM_STASHED,		TRUE 	},
    {	"wizi",			ITEM_WIZI,		TRUE 	},
    {   "nolong",               ITEM_NOLONG,            TRUE    },
    {   "nodisarm",		ITEM_NODISARM,		TRUE	},
    {	NULL,			0,			0	}
   },
   {
//    {   "nodisarm",		ITEM_NODISARM,		TRUE	},
    {   NULL,			0,			0	}
   },
   {
    {	"fire",		ITEM_FIRE,		TRUE	},
    {	"nomortal",	ITEM_NOMORTAL,	TRUE	},
    {   "quintessence", ITEM_QUINTESSENCE, TRUE },
    {   "incense",  ITEM_INCENSE,   TRUE    },
    {   "annointingoil", ITEM_ANNOINTINGOIL, TRUE},
    {   "windfall", ITEM_WINDFALL,  TRUE    },
    {   "nomirror", ITEM_NOMIRROR,  TRUE    },
    {   NULL,			0,			0	}
   }
};



const struct flag_type wear_flags[] =
{
    {	"take",			ITEM_TAKE,		TRUE	},
    {	"finger",		ITEM_WEAR_FINGER,	TRUE	},
    {	"neck",			ITEM_WEAR_NECK,		TRUE	},
    {	"body",			ITEM_WEAR_BODY,		TRUE	},
    {	"head",			ITEM_WEAR_HEAD,		TRUE	},
    {	"legs",			ITEM_WEAR_LEGS,		TRUE	},
    {	"feet",			ITEM_WEAR_FEET,		TRUE	},
    {	"hands",		ITEM_WEAR_HANDS,	TRUE	},
    {	"arms",			ITEM_WEAR_ARMS,		TRUE	},
    {	"shield",		ITEM_WEAR_SHIELD,	TRUE	},
    {	"about",		ITEM_WEAR_ABOUT,	TRUE	},
    {	"waist",		ITEM_WEAR_WAIST,	TRUE	},
    {	"wrist",		ITEM_WEAR_WRIST,	TRUE	},
    {	"wield",		ITEM_WIELD,		TRUE	},
    {	"hold",			ITEM_HOLD,		TRUE	},
    {   "nosac",		ITEM_NO_SAC,		TRUE	},
    {	"wearfloat",		ITEM_WEAR_FLOAT,	TRUE	},
    {	"sigil",		ITEM_WEAR_SIGIL,	TRUE	},
    {   "prog",			ITEM_PROG,		TRUE	},
    {   "familiar",		ITEM_WEAR_FAMILIAR,		TRUE	},
    {	NULL,			0,			0	}
};

/*
 * Used when adding an affect to tell where it goes.
 * See addaffect and delaffect in act_olc.c
 */
const struct flag_type apply_flags[] =
{
    {	"none",			APPLY_NONE,		TRUE	},
    {	"strength",		APPLY_STR,		TRUE	},
    {	"dexterity",		APPLY_DEX,		TRUE	},
    {	"intelligence",		APPLY_INT,		TRUE	},
    {	"wisdom",		APPLY_WIS,		TRUE	},
    {	"constitution",		APPLY_CON,		TRUE	},
    {   "charisma",		APPLY_CHR,		TRUE    },
    {	"sex",			APPLY_SEX,		TRUE	},
    {	"class",		APPLY_CLASS,		TRUE	},
    {	"level",		APPLY_LEVEL,		TRUE	},
    {	"age",			APPLY_AGE,		TRUE	},
    {	"height",		APPLY_HEIGHT,		TRUE	},
    {	"weight",		APPLY_WEIGHT,		TRUE	},
    {	"mana",			APPLY_MANA,		TRUE	},
    {	"hp",			APPLY_HIT,		TRUE	},
    {	"move",			APPLY_MOVE,		TRUE	},
    {	"gold",			APPLY_GOLD,		TRUE	},
    {	"experience",		APPLY_EXP,		TRUE	},
    {	"ac",			APPLY_AC,		TRUE	},
    {	"hitroll",		APPLY_HITROLL,		TRUE	},
    {	"damroll",		APPLY_DAMROLL,		TRUE	},
    {	"saves",		APPLY_SAVES,		TRUE	},
    {	"savingpara",		APPLY_SAVING_PARA,	TRUE	},
    {	"savingrod",		APPLY_SAVING_ROD,	TRUE	},
    {	"savingpetri",		APPLY_SAVING_PETRI,	TRUE	},
    {	"savingbreath",		APPLY_SAVING_BREATH,	TRUE	},
    {	"savingspell",		APPLY_SAVING_SPELL,	TRUE	},
    {   "applyhide", 		APPLY_HIDE, 	        TRUE	},
    {   "range",                APPLY_RANGE,            TRUE    },
    {   "resistsummon",         APPLY_RESIST_SUMMON,    TRUE    },
    {   "resistcharm",          APPLY_RESIST_CHARM,     TRUE    },
    {   "resistmagic",          APPLY_RESIST_MAGIC,     TRUE    },
    {   "resistweapon",         APPLY_RESIST_WEAPON,    TRUE    },
    {   "resistbash",           APPLY_RESIST_BASH,      TRUE    },
    {   "resistpierce",         APPLY_RESIST_PIERCE,    TRUE    },
    {   "resistslash",          APPLY_RESIST_SLASH,     TRUE    },
    {   "resistfire",           APPLY_RESIST_FIRE,      TRUE    },
    {   "resistcold",           APPLY_RESIST_COLD,      TRUE    },
    {   "resistlightning",      APPLY_RESIST_LIGHTNING, TRUE    },
    {   "resistacid",           APPLY_RESIST_ACID,      TRUE    },
    {   "resistpoison",         APPLY_RESIST_POISON,    TRUE    },
    {   "resistnegative",       APPLY_RESIST_NEGATIVE,  TRUE    },
    {   "resistholy",           APPLY_RESIST_HOLY,      TRUE    },
    {   "resistenergy",         APPLY_RESIST_ENERGY,    TRUE    },
    {   "resistmental",         APPLY_RESIST_MENTAL,    TRUE    },
    {   "resistdisease",        APPLY_RESIST_DISEASE,   TRUE    },
    {   "resistdrowning",       APPLY_RESIST_DROWNING,  TRUE    },
    {   "resistlight",          APPLY_RESIST_LIGHT,     TRUE    },
    {   "resistsound",          APPLY_RESIST_SOUND,     TRUE    },
    {   "resistillusion",       APPLY_RESIST_ILLUSION,  TRUE    },
    {   "resistdefilement",     APPLY_RESIST_DEFILEMENT,      TRUE    },
    {   "resistfear",         APPLY_RESIST_FEAR,    TRUE    },
    {   "resistiron",           APPLY_RESIST_IRON,      TRUE    },
    {   "size", 		APPLY_SIZE,		TRUE    },
    {	"spellaffect",		APPLY_SPELL_AFFECT,	FALSE	},
    {   "maxstrength",		APPLY_MAXSTR,		TRUE	},
    {	"maxdexterity",		APPLY_MAXDEX,		TRUE	},
    {	"maxintelligence",	APPLY_MAXINT,		TRUE	},
    {	"maxwisdom",		APPLY_MAXWIS,		TRUE	},
    {	"maxconstitution",	APPLY_MAXCON,		TRUE	},
    {   "maxcharisma",		APPLY_MAXCHR,		TRUE    },
    {   "scanrange",        APPLY_SCANRANGE,    TRUE    },
    {   "luck",             APPLY_LUCK,         TRUE    },
    {	NULL,			0,			0	}
};



/*
 * What is seen.
 */
const struct flag_type wear_loc_strings[] =
{
    {	"in the inventory",	WEAR_NONE,	TRUE	},
    {	"as a light",		WEAR_LIGHT,	TRUE	},
    {	"on the left finger",	WEAR_FINGER_L,	TRUE	},
    {	"on the right finger",	WEAR_FINGER_R,	TRUE	},
    {	"around the neck (1)",	WEAR_NECK_1,	TRUE	},
    {	"around the neck (2)",	WEAR_NECK_2,	TRUE	},
    {	"on the body",		WEAR_BODY,	TRUE	},
    {	"over the head",	WEAR_HEAD,	TRUE	},
    {	"on the legs",		WEAR_LEGS,	TRUE	},
    {	"on the feet",		WEAR_FEET,	TRUE	},
    {	"on the hands",		WEAR_HANDS,	TRUE	},
    {	"on the arms",		WEAR_ARMS,	TRUE	},
    {	"as a shield",		WEAR_SHIELD,	TRUE	},
    {	"about the shoulders",	WEAR_ABOUT,	TRUE	},
    {	"around the waist",	WEAR_WAIST,	TRUE	},
    {	"on the left wrist",	WEAR_WRIST_L,	TRUE	},
    {	"on the right wrist",	WEAR_WRIST_R,	TRUE	},
    {	"wielded",		WEAR_WIELD,	TRUE	},
    {   "duel wielded",         WEAR_WIELD,     TRUE    },
    {	"held in the hands",	WEAR_HOLD,	TRUE	},
    {	"floating nearby",	WEAR_FLOAT,	TRUE	},
    {	"branded",		WEAR_SIGIL,	TRUE	},
    {   "as a prog",		WEAR_PROGSLOT,	TRUE	},
    {   "as familiar",		WEAR_FAMILIAR,	TRUE	},
    {	NULL,			0	      , 0	}
};


const struct flag_type wear_loc_flags[] =
{
    {	"none",		WEAR_NONE,	TRUE	},
    {	"light",	WEAR_LIGHT,	TRUE	},
    {	"lfinger",	WEAR_FINGER_L,	TRUE	},
    {	"rfinger",	WEAR_FINGER_R,	TRUE	},
    {	"neck1",	WEAR_NECK_1,	TRUE	},
    {	"neck2",	WEAR_NECK_2,	TRUE	},
    {	"body",		WEAR_BODY,	TRUE	},
    {	"head",		WEAR_HEAD,	TRUE	},
    {	"legs",		WEAR_LEGS,	TRUE	},
    {	"feet",		WEAR_FEET,	TRUE	},
    {	"hands",	WEAR_HANDS,	TRUE	},
    {	"arms",		WEAR_ARMS,	TRUE	},
    {	"shield",	WEAR_SHIELD,	TRUE	},
    {	"about",	WEAR_ABOUT,	TRUE	},
    {	"waist",	WEAR_WAIST,	TRUE	},
    {	"lwrist",	WEAR_WRIST_L,	TRUE	},
    {	"rwrist",	WEAR_WRIST_R,	TRUE	},
    {	"wielded",	WEAR_WIELD,	TRUE	},
    {	"hold",		WEAR_HOLD,	TRUE	},
    {	"floating",	WEAR_FLOAT,	TRUE	},
    {	"branded",	WEAR_SIGIL,	TRUE	},
    {	"dual",		WEAR_DUAL_WIELD,TRUE	},
    {   "prog",		WEAR_PROGSLOT,	TRUE	},
    {   "familiar",	WEAR_FAMILIAR,	TRUE	},
    {	NULL,		0,		0	}
};

const struct flag_type container_flags[] =
{
    {	"closeable",		1,		TRUE	},
    {	"pickproof",		2,		TRUE	},
    {	"closed",		4,		TRUE	},
    {	"locked",		8,		TRUE	},
    {	"puton",		16,		TRUE	},
    {	NULL,			0,		0	}
};

/*****************************************************************************
                      ROM - specific tables:
 ****************************************************************************/




const struct flag_type ac_type[] =
{
    {   "pierce",        AC_PIERCE,            TRUE    },
    {   "bash",          AC_BASH,              TRUE    },
    {   "slash",         AC_SLASH,             TRUE    },
    {   "exotic",        AC_EXOTIC,            TRUE    },
    {   NULL,              0,                    0       }
};


const struct flag_type size_flags[] =
{
    {   "tiny",          SIZE_TINY,            TRUE    },
    {   "small",         SIZE_SMALL,           TRUE    },
    {   "medium",        SIZE_MEDIUM,          TRUE    },
    {   "large",         SIZE_LARGE,           TRUE    },
    {   "huge",          SIZE_HUGE,            TRUE    },
    {   "giant",         SIZE_GIANT,           TRUE    },
    {   NULL,              0,                    0       },
};


const struct flag_type weapon_class[] =
{
    {   "exotic",	WEAPON_EXOTIC,		TRUE    },
    {   "sword",	WEAPON_SWORD,		TRUE    },
    {   "dagger",	WEAPON_DAGGER,		TRUE    },
    {   "spear",	WEAPON_SPEAR,		TRUE    },
    {   "mace",		WEAPON_MACE,		TRUE    },
    {   "axe",		WEAPON_AXE,		TRUE    },
    {   "flail",	WEAPON_FLAIL,		TRUE    },
    {   "whip",		WEAPON_WHIP,		TRUE    },
    {   "polearm",	WEAPON_POLEARM,		TRUE    },
    {	"staff",	WEAPON_STAFF,		TRUE	},
    {	"knife",	WEAPON_KNIFE,		TRUE 	},
    {   NULL,		0,			0       }
};


const struct flag_type weapon_type2[] =
{
    {   "flaming",       WEAPON_FLAMING,       TRUE    },
    {   "frost",         WEAPON_FROST,         TRUE    },
    {   "vampiric",      WEAPON_VAMPIRIC,      TRUE    },
    {   "sharp",         WEAPON_SHARP,         TRUE    },
    {   "vorpal",        WEAPON_VORPAL,        TRUE    },
    {   "twohands",     WEAPON_TWO_HANDS,     TRUE    },
    {	"shocking",	 WEAPON_SHOCKING,      TRUE    },
    {	"poison",	WEAPON_POISON,		TRUE	},
    {   NULL,              0,                    0       }
};

const struct resist_type resist_table[] =
{
    {	"summon"    },
    {   "charm"     },
    {   "magic"     },
    {   "weapon"    },
    {   "bash"      },
    {   "pierce"    },
    {   "slash"     },
    {   "fire"      },
    {   "cold"      },
    {   "lightning" },
    {   "acid"      },
    {   "poison"    },
    {   "negative"  },
    {   "holy"      },
    {   "energy"    },
    {   "mental"    },
    {   "disease"   },
    {   "drowning"  },
    {   "light"     },
    {	"sound"     },
    {   "illusion"  },
    {	"wood"      },
    {	"silver"    },
    {	"iron"      },
    {   NULL        }
};


const struct flag_type res_flags[] =
{
    {	"summon",	 RES_SUMMON,		TRUE	},
    {   "charm",         RES_CHARM,            TRUE    },
    {   "magic",         RES_MAGIC,            TRUE    },
    {   "weapon",        RES_WEAPON,           TRUE    },
    {   "bash",          RES_BASH,             TRUE    },
    {   "pierce",        RES_PIERCE,           TRUE    },
    {   "slash",         RES_SLASH,            TRUE    },
    {   "fire",          RES_FIRE,             TRUE    },
    {   "cold",          RES_COLD,             TRUE    },
    {   "lightning",     RES_LIGHTNING,        TRUE    },
    {   "acid",          RES_ACID,             TRUE    },
    {   "poison",        RES_POISON,           TRUE    },
    {   "negative",      RES_NEGATIVE,         TRUE    },
    {   "holy",          RES_HOLY,             TRUE    },
    {   "energy",        RES_ENERGY,           TRUE    },
    {   "mental",        RES_MENTAL,           TRUE    },
    {   "disease",       RES_DISEASE,          TRUE    },
    {   "drowning",      RES_DROWNING,         TRUE    },
    {   "light",         RES_LIGHT,            TRUE    },
    {	"sound",	RES_SOUND,		TRUE	},
    {   "illusion",	RES_ILLUSION,		TRUE	},
    //{   "none",         RES_NONE,               TRUE    },
    //{   "none",         RES_NONE2,              TRUE    },
    {	"defilement",	RES_DEFILEMENT,		TRUE	},
    {	"fear",	        RES_FEAR,		TRUE	},
    {	"iron",		RES_IRON,		TRUE	},
    {   NULL,          0,            0    }
};


const struct flag_type vuln_flags[] =
{
    {	"summon",	 VULN_SUMMON,		TRUE	},
    {	"charm",	VULN_CHARM,		TRUE	},
    {   "magic",         VULN_MAGIC,           TRUE    },
    {   "weapon",        VULN_WEAPON,          TRUE    },
    {   "bash",          VULN_BASH,            TRUE    },
    {   "pierce",        VULN_PIERCE,          TRUE    },
    {   "slash",         VULN_SLASH,           TRUE    },
    {   "fire",          VULN_FIRE,            TRUE    },
    {   "cold",          VULN_COLD,            TRUE    },
    {   "lightning",     VULN_LIGHTNING,       TRUE    },
    {   "acid",          VULN_ACID,            TRUE    },
    {   "poison",        VULN_POISON,          TRUE    },
    {   "negative",      VULN_NEGATIVE,        TRUE    },
    {   "holy",          VULN_HOLY,            TRUE    },
    {   "energy",        VULN_ENERGY,          TRUE    },
    {   "mental",        VULN_MENTAL,          TRUE    },
    {   "disease",       VULN_DISEASE,         TRUE    },
    {   "drowning",      VULN_DROWNING,        TRUE    },
    {   "light",         VULN_LIGHT,           TRUE    },
    {	"sound",	 VULN_SOUND,		TRUE	},
    {	"illusion",	 VULN_ILLUSION,		TRUE	},
    {   "defilement",          VULN_DEFILEMENT,            TRUE    },
    {   "fear",        VULN_FEAR,          TRUE    },
    {   "iron",          VULN_IRON,            TRUE    },
    {   NULL,              0,                    0       }
};

const struct flag_type position_flags[] =
{
    {   "dead",           POS_DEAD,            FALSE   },
    {   "mortal",         POS_MORTAL,          FALSE   },
    {   "incap",          POS_INCAP,           FALSE   },
    {   "stunned",        POS_STUNNED,         FALSE   },
    {   "sleeping",       POS_SLEEPING,        TRUE    },
    {   "resting",        POS_RESTING,         TRUE    },
    {   "sitting",        POS_SITTING,         TRUE    },
    {   "fighting",       POS_FIGHTING,        FALSE   },
    {   "standing",       POS_STANDING,        TRUE    },
    {   NULL,              0,                    0       }
};

const struct flag_type portal_flags[]=
{
    {   "normal_exit",	  GATE_NORMAL_EXIT,	TRUE	},
    {	"no_curse",	  GATE_NOCURSE,		TRUE	},
    {   "go_with",	  GATE_GOWITH,		TRUE	},
    {   "buggy",	  GATE_BUGGY,		TRUE	},
    {	"random",	  GATE_RANDOM,		TRUE	},
    {   NULL,		  0,			0	}
};

const struct flag_type light_flags[] =
{
    {   "always_burn",  LIGHT_ALWAYS_BURN,  TRUE},
    {   NULL,           0,                  0}
};

const struct flag_type furniture_flags[]=
{
    {   "stand_at",	  STAND_AT,		TRUE	},
    {	"stand_on",	  STAND_ON,		TRUE	},
    {	"stand_in",	  STAND_IN,		TRUE	},
    {	"sit_at",	  SIT_AT,		TRUE	},
    {	"sit_on",	  SIT_ON,		TRUE	},
    {	"sit_in",	  SIT_IN,		TRUE	},
    {	"rest_at",	  REST_AT,		TRUE	},
    {	"rest_on",	  REST_ON,		TRUE	},
    {	"rest_in",	  REST_IN,		TRUE	},
    {	"sleep_at",	  SLEEP_AT,		TRUE	},
    {	"sleep_on",	  SLEEP_ON,		TRUE	},
    {	"sleep_in",	  SLEEP_IN,		TRUE	},
    {	"put_at",	  PUT_AT,		TRUE	},
    {	"put_on",	  PUT_ON,		TRUE	},
    {	"put_in",	  PUT_IN,		TRUE	},
    {	"put_inside",	  PUT_INSIDE,		TRUE	},
    {	NULL,		  0,			0	}
};


