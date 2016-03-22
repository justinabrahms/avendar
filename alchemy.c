#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sstream>
#include "merc.h"
#include "alchemy.h"
#include "recycle.h"
#include "NameMaps.h"

const struct alchemy_type alchemy_table[] =
{
	/* {gsn, min_items, max_items, action, product, type, weight} */
	
	{VNUM_MIX, 	3, 9, 	"mix", 	"oil",		ITEM_OIL,	3},
	{VNUM_BREW, 	3, 9,	"brew",	"potion",	ITEM_POTION,	5},
	{VNUM_MAKE,	3, 9,	"make",	"pill",		ITEM_PILL,	1}
};

const struct alchemy_skills_special alchemy_skills_table[] =
{
	/* {local_gsn, short_descr, extra_name, metal_only, solid_only, liquid_only} */
	{0, "a distillate of", "", FALSE, FALSE, TRUE},
	{1, "a bag of fine powder, ground from", "powder", FALSE, TRUE, FALSE},
	{2, "a mass of metal, reforged from", "metal", TRUE, FALSE, FALSE},
	{3, "a concentrate of", "", FALSE, FALSE, TRUE},
	{4, "a solution of", "", FALSE, TRUE, FALSE},
	{5, "some diluted", "", FALSE, FALSE, TRUE}
};

const struct alchemy_adjectives alchemy_adjective_table[] =
{
	{"shimmering"}, {"clotted"}, {"coagulated"}, {"pungent"}, {"hazy"}, {"misty"},
	{"viscous"}, {"warm"}, {"steaming"}, {"thin"}, {"glutinous"}, {"milky"},
	{"aromatic"}, {"caustic"}, {"cold"}, {"fragrant"}, {"foaming"}, {"metallic"},
	{"oily"}, {"pearlescent"}, {"sparkling"}, {"sticky"}, {"swirling"},
	{"strange-smelling"}, {"frozen"}, {"frothy"}, {"foul-smelling"}, {"foul"}, {"flaming"}, {"filmy"}, {"dense"},
	{"fermenting"}, {"smoking"}, {"odd"}, {"odorless"}, {"oozing"}, {"opaque"}, {"syrupy"}, {"tasteless"},
	{"bland"}, {"acrid-smelling"}, {"cloudy"}, {"bubbling"}, {"incandescent"}, {"murky"}, {"greasy"}
};

const struct alchemy_adjectives wand_adjective_table[] =
{
	{"long"}, {"thin"}, {"short"}, {"thick"}, {"stubby"}, {"fat"}, {"elongated"},
	{"stilted"}, {"pitted"}, {"straight"}, {"smooth"}, {"rough"},
	
	//Crit success adjs
	{"beautiful"}, {"ornate"}, {"well-crafted"}, {"elegant"}
};

const struct alchemy_colors alchemy_color_table[] =
{
	/* golden(S), blue(W), white(Ai), brown(E), red(F), black(V), green(N), prismatic/purple(An), silver(T)
	 * Gets darker as you progress down */
	{"gold-flecked",	"blue-tinged",	"clear",	"dark yellow",		"pink", 
	 "grey", 		"green-tinged", "lavender", 	"silver-flecked"},
	{"gold-tinged", 	"pale blue", 	"faint", 	"faint brown", 		"pale red", 
	 "dark grey", 		"pale green", 	"lilac", 	"silver-tinged"},
	{"pale gold", 		"light blue", 	"pale", 	"light brown", 		"light red", 
	 "light black",		"light green", 	"mauve", 	"pale silver"},
	{"sunny", 		"sky blue", 	"faint white", 	"copper-colored",	"rosy",
	 "dusty black",		"lime green", 	"light purple",	"sterling"},
	
	{"golden", 		"blue", 	"white", 	"brown", 		"red",
	 "black", 		"green", 	"purple", 	"silver"},
		
	{"aureate", 		"dark blue", 	"bright white", "russet", 		"vermillion",
	 "ebony", 		"pine green", 	"amethyst", 	"argentate"},
	{"bright gold", 	"deep blue", 	"bleached", 	"chestnut", 		"sanguine",
	 "onyx", 		"olive green", 	"violet", 	"bright silver"},
	{"aurulent", 		"royal blue", 	"bone-white", 	"umber", 		"maroon",
	 "coal-black", 		"deep green", 	"deep purple", 	"shining silver"},
	{"dazzlingly golden", 	"navy blue", 	"chalk-white", 	"dark brown", 		"scarlet",
	 "midnight-black", 	"dark green", 	"dark purple", 	"dazzling silver"},
	{"sunfire", 		"midnight blue","alabaster", 	"deep brown", 		"crimson",
	 "pitch-black", 	"forest green", "royal purple", "moonlight-silver"},
	
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

const struct alchemy_skills void_skills[] =
{
	/* {skill name, object type, platonic bias[], power} */
	{"blindness", 1, {BIAS_MALEDICTION_INDEX, BIAS_OBSCUREMENT_INDEX, BIAS_NULL_INDEX}, 4},
	{"curse", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 4},
	{"nightfears", 1, {BIAS_OBSCUREMENT_INDEX, BIAS_MALEDICTION_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 2},
	{"teleport", 6, {BIAS_CHAOS_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"invisibility", 6, {BIAS_OBSCUREMENT_INDEX, BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 4},
	{"chill touch", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 1},
	{"plague", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 4},
	{"pass door", 6, {BIAS_ALTERATION_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 3},
	{"pox", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 7},
	{"agony", 1, {BIAS_MALEDICTION_INDEX, BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 6},
	{"drain", 1, {BIAS_DAMAGE_INDEX, BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 5},
	{"banish", 1, {BIAS_DAMAGE_INDEX, BIAS_PLANAR_INDEX, BIAS_ORDER_INDEX, BIAS_NULL_INDEX}, 3},
	{"cloak of the void", 6, {BIAS_ENHANCEMENT_INDEX, BIAS_PLANAR_INDEX, BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 8},
	{"enfeeblement", 1, {BIAS_MALEDICTION_INDEX, BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 5},
	{"cause critical", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 2},
	{"cause light", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 1},
	{"cause serious", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 1},
	{"harm", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 5},
	{"numb", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 1},
	{"sleep", 1, {BIAS_MALEDICTION_INDEX, BIAS_OBSCUREMENT_INDEX, BIAS_NULL_INDEX}, 8},
	{"power word fear", 1, {BIAS_KNOWLEDGE_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 3},
	{"weaken", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 7},
	{"word of recall", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"vortex", 1, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 9},
	{"voidwalk", 6, {BIAS_OBSCUREMENT_INDEX, BIAS_ALTERATION_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 10},
	{"possession", 1, {BIAS_PLANAR_INDEX, BIAS_OBSCUREMENT_INDEX, BIAS_NULL_INDEX}, 9},
	{"curse of kijjasku", 7, {BIAS_PROTECTION_INDEX, BIAS_ORDER_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 6},
	{"siphon mana", 1, {BIAS_MALEDICTION_INDEX, BIAS_ORDER_INDEX, BIAS_NULL_INDEX}, 4},
	{"blight of adduthala", 1, {BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 5},
	{"embrace of isetaton", 1, {BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 7},
	{"globe of darkness", 6, {BIAS_OBSCUREMENT_INDEX, BIAS_NULL_INDEX}, 7},
	{"armor", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 1},
	{"shield", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 1},
	{"detect invis", 6, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 2},
	{"protective shield", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 5},
	{NULL, 0, { BIAS_NULL_INDEX }, 0}
};

const struct alchemy_skills spirit_skills[] =
{
	{"improved detect evil", 6, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 4},
	{"spirit stone", 6, {BIAS_PLANAR_INDEX, BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 3},
	{"spirit block", 1, {BIAS_MALEDICTION_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 8},
	{"spirit bond", 3, {BIAS_ENHANCEMENT_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 5},
	{"affinity", 1, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 8},
	{"subdue", 1, {BIAS_OBSCUREMENT_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 4},
	{"spiritwrack", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 5},
	{"visions", 1, {BIAS_OBSCUREMENT_INDEX, BIAS_MALEDICTION_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 4},
	{"pass door", 7, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 2},
	{"thanatopsis", 6, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 4},
	{"spirit shield", 6, {BIAS_ENHANCEMENT_INDEX, BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 5},
	{"zeal", 7, {BIAS_ENHANCEMENT_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 5},
	{"clarity", 7, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 4},
	{"focus mind", 7, {BIAS_ENHANCEMENT_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 6},
	{"bless", 7, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 1},
	{"soulburn", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 1},
	{"calm", 6, {BIAS_PROTECTION_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 4},
	{"invisibility", 7, {BIAS_OBSCUREMENT_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 3},
	{"mana conduit", 1, {BIAS_KNOWLEDGE_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 5},
	{"teleport", 7, {BIAS_CHAOS_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"avenging seraph", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 7},
	{"avatar", 6, {BIAS_PLANAR_INDEX, BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 10},
	{"detect good", 6, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 2},
        {"ray of truth", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 5},
	{"word of recall", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"astral projection", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 8},
	{"awaken", 1, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 2},
	{"brainwash", 1, {BIAS_KNOWLEDGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 6},
	{"continual light", 7, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 1},
	{"mindshell", 7, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 6},
	{"dreamspeak", 6, {BIAS_KNOWLEDGE_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 2},
	{"life bolt", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 5},
	{"consecrate weapon", 1, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 4},
	{"soul flare", 1, {BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 7},
	{"astral form", 6, {BIAS_PLANAR_INDEX, BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 4},
	{"radiance", 6, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 2},
	{"wrath of kyana", 1, {BIAS_MALEDICTION_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 7},
	{"vengeance", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 5},
	{"spirit sanctuary", 6, {BIAS_PLANAR_INDEX, BIAS_OBSCUREMENT_INDEX, BIAS_NULL_INDEX}, 9},
	{"detect invis", 6, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 2},
	{"protective shield", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 5},
	{NULL, 0, { BIAS_NULL_INDEX }, 0}
};

const struct alchemy_skills water_skills[] =
{
	{"iceblast", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 6},
	{"communion", 7, {BIAS_PLANAR_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 4},
	{"currents", 6, {BIAS_PLANAR_INDEX, BIAS_CHAOS_INDEX, BIAS_NULL_INDEX}, 3},
	{"protection from poison", 7, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 5},
	{"protection from fire", 7, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 8},
	{"flood", 6, {BIAS_PLANAR_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 4},
	{"mend wounds", 1, {BIAS_ALTERATION_INDEX, BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 4},
	{"mass heal", 6, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 7},
	{"heal", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 5},
	{"sanctuary", 7, {BIAS_ENHANCEMENT_INDEX, BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 9},
	{"frostbite", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 6},
	{"cure light", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 1},
	{"cure serious", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 2},
	{"create spring", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 2},
	{"create food", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 1},
	{"cure poison", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 3},
	{"cure critical", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 3},
	{"cure blindness", 7, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 3},
	{"cure disease", 7, {BIAS_ALTERATION_INDEX, BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 3},
	{"cure critical", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 3},
	{"water breathing", 7, {BIAS_ALTERATION_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 3},
	{"holy water", 1, {BIAS_DAMAGE_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 5},
	{"rune of life", 1, {BIAS_ENHANCEMENT_INDEX, BIAS_PLANAR_INDEX, BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 5},
	{"refresh", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 4},
	{"rebirth", 6, {BIAS_KNOWLEDGE_INDEX, BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 8},
	{"resurrection", 1, {BIAS_HEALING_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 10},
	{"icy prison", 1, {BIAS_PLANAR_INDEX, BIAS_ORDER_INDEX, BIAS_NULL_INDEX}, 8},
	{"word of recall", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"cone of cold", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 7},
	{"wall of water", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 8},
	{"freeze", 1, {BIAS_DAMAGE_INDEX, BIAS_ORDER_INDEX, BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 6},
	{"frostbrand", 1, {BIAS_ALTERATION_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 5},
	{"aquatic movement", 7, {BIAS_ENHANCEMENT_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"encase", 1, {BIAS_DAMAGE_INDEX, BIAS_ORDER_INDEX, BIAS_NULL_INDEX}, 7},
	{"whirlpool", 1, {BIAS_MALEDICTION_INDEX, BIAS_PLANAR_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 5},
	{"revitalize", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 8},
	{"water walk", 7, {BIAS_PLANAR_INDEX, BIAS_ORDER_INDEX, BIAS_NULL_INDEX}, 1},
	{"protective shield", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 5},
	{"detect invis", 6, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 2},
	{"heatsink", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 3},
	{NULL, 0, { BIAS_NULL_INDEX }, 0}
};

const struct alchemy_skills fire_skills[] =
{
	{"wall of fire", 6, {BIAS_DAMAGE_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 5},
	{"dispersion", 6, {BIAS_DAMAGE_INDEX, BIAS_CHAOS_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 6},
	{"consume", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 6},
	{"incineration", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 5},
	{"flame tongue", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 8},
	{"fire blast", 1, {BIAS_DAMAGE_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 5},
	{"smolder", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 2},
	{"fireball", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 6},
	{"frenzy", 7, {BIAS_CHAOS_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 3},
	{"firestorm", 6, {BIAS_CHAOS_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 8},
	{"beam of fire", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 8},
	{"ethereal blaze", 1, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 4},
	{"flame shield", 7, {BIAS_PROTECTION_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 5},
	{"baptism of fire", 7, {BIAS_HEALING_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 2},
	{"wings of flame", 7, {BIAS_ALTERATION_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 4},
	{"burning hands", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 1},
	{"flamestrike", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 3},
	{"infravision", 7, {BIAS_ALTERATION_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 2},
	{"immolation", 7, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 4},
	{"scorch", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 1},
	{"teleport", 6, {BIAS_CHAOS_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"living flame", 7, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 9},
	{"disintegration", 1, {BIAS_CHAOS_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 10},
	{"combustion", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 6},
	{"coronal glow", 1, {BIAS_MALEDICTION_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 3},
	{"word of recall", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"firebrand", 1, {BIAS_ALTERATION_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 4},
	{"weariness", 1, {BIAS_MALEDICTION_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 3},
	{"fury of the inferno", 7, {BIAS_ENHANCEMENT_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 6},
	{"burnout", 7, {BIAS_MALEDICTION_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 4},
	{"melt metal", 1, {BIAS_ALTERATION_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 5},
	{"consuming rage", 7, {BIAS_KNOWLEDGE_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 6},
	{"ring of fire", 1, {BIAS_MALEDICTION_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 7},
	{"implosion", 6, {BIAS_CHAOS_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 10},
	{"heat metal", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 3},
	{"parch", 1, {BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 2},
	{"flare", 6, {BIAS_PROTECTION_INDEX, BIAS_CHAOS_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 4},
	{"detect invis", 6, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 2},
	{"delayed blast fireball", 6, {BIAS_DAMAGE_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 7},
	{NULL, 0, { BIAS_NULL_INDEX }, 0}
};

const struct alchemy_skills earth_skills[] =
{
	{"stabilize", 7, {BIAS_ORDER_INDEX, BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 5},
	{"smooth terrain", 6, {BIAS_ORDER_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 2},
	{"strengthen", 1, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 1},
	{"armor", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 1},
	{"shield", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 1},
	{"slip", 1, {BIAS_ALTERATION_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 2},
	{"giant strength", 7, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 2},
	{"sandspray", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 3},
	{"earthquake", 6, {BIAS_PLANAR_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 3},
	{"create spring", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 2},
	{"earthbind", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 4},
	{"fortify", 7, {BIAS_ENHANCEMENT_INDEX, BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 5},
	{"flesh to stone", 1, {BIAS_ORDER_INDEX, BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 8},
	{"crystalize magic", 7, {BIAS_ORDER_INDEX, BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 9},
	{"stonefist", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 9},
	{"stone shell", 7, {BIAS_PROTECTION_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 4},
	{"call upon earth", 7, {BIAS_PLANAR_INDEX, BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 5},
	{"density", 1, {BIAS_DAMAGE_INDEX, BIAS_ALTERATION_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 4},
	{"brittleform", 7, {BIAS_DAMAGE_INDEX, BIAS_ALTERATION_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 7},
	{"shape armor", 1, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 8},
	{"shape weapon", 1, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 8},
	{"metal to stone", 1, {BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 4},
	{"meteor strike", 1, {BIAS_DAMAGE_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 6},
	{"stone to mud", 6, {BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 7},
	{"dispel illusion", 6, {BIAS_DAMAGE_INDEX, BIAS_ORDER_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 4},
	{"stone skin", 7, {BIAS_ALTERATION_INDEX, BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 7},
	{"meld with stone", 6, {BIAS_ALTERATION_INDEX, BIAS_OBSCUREMENT_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 7},
	{"word of recall", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"quicksand", 1, {BIAS_DAMAGE_INDEX, BIAS_ORDER_INDEX, BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 7},
	{"petrify", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_ORDER_INDEX, BIAS_NULL_INDEX}, 7},
	{"anchor", 7, {BIAS_PROTECTION_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 5},
	{"magnetic grasp", 1, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 6},
	{"shatter", 1, {BIAS_ALTERATION_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 5},
	{"diamond skin", 7, {BIAS_ALTERATION_INDEX, BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 10},
	{"bind weapon", 6, {BIAS_ORDER_INDEX, BIAS_NULL_INDEX}, 5},
	{"detect invis", 6, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 2},
	{"wall of stone", 6, {BIAS_PROTECTION_INDEX, BIAS_ORDER_INDEX, BIAS_NULL_INDEX}, 7},
	{NULL, 0, { BIAS_NULL_INDEX }, 0}
};

const struct alchemy_skills air_skills[] =
{
	{"delusions", 1, {BIAS_OBSCUREMENT_INDEX, BIAS_MALEDICTION_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 7},
	{"windbind", 1, {BIAS_MALEDICTION_INDEX, BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 7},
	{"teleport", 6, {BIAS_CHAOS_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"fly", 7, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 3},
	{"mass flying", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 5},
	{"lightning bolt", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 3},
	{"shocking grasp", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 1},
	{"illusionary object", 1, {BIAS_PLANAR_INDEX, BIAS_OBSCUREMENT_INDEX, BIAS_NULL_INDEX}, 2},
	{"air bubble", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 3},
	{"group teleport", 6, {BIAS_CHAOS_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 5},
	{"gust", 1, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 2},
	{"invisibility", 7, {BIAS_OBSCUREMENT_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"faerie fog", 6, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 4},
	{"tempest", 6, {BIAS_CHAOS_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 7},
	{"cloudkill", 7, {BIAS_PLANAR_INDEX, BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 8},
	{"chain lightning", 1, {BIAS_CHAOS_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 8},
	{"air rune", 6, {BIAS_OBSCUREMENT_INDEX, BIAS_OBSCUREMENT_INDEX, BIAS_NULL_INDEX}, 7},
	{"scatter", 6, {BIAS_CHAOS_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 6},
	{"gaseous form", 7, {BIAS_ALTERATION_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 7},
	{"spectral fist", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 6},
	{"call lightning", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 3},
	{"word of recall", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"greater invis", 7, {BIAS_OBSCUREMENT_INDEX, BIAS_NULL_INDEX}, 9},
	{"blade barrier", 7, {BIAS_PLANAR_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 7},
	{"protection from lightning", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 8},
	{"armor", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 1},
	{"shield", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 1},
	{"ball lightning", 1, {BIAS_PLANAR_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 7},
	{"wind blast", 1, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"mirror image", 7, {BIAS_PLANAR_INDEX, BIAS_OBSCUREMENT_INDEX, BIAS_NULL_INDEX}, 6},
	{"lightning brand", 1, {BIAS_ENHANCEMENT_INDEX, BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 5},
	{"absorb electricity", 7, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 6},
	{"wall of air", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 5},
	{"thunderclap", 1, {BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 6},
	{"blur", 6, {BIAS_PROTECTION_INDEX, BIAS_CHAOS_INDEX, BIAS_OBSCUREMENT_INDEX, BIAS_NULL_INDEX}, 7},
	{"detect invis", 6, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 2},
	{"blink", 6, {BIAS_CHAOS_INDEX, BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 6},
	{"suction", 6, {BIAS_CHAOS_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"gust", 1, {BIAS_CHAOS_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"wind blast", 1, {BIAS_CHAOS_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 5},
	{"diversions", 6, {BIAS_CHAOS_INDEX, BIAS_OBSCUREMENT_INDEX, BIAS_NULL_INDEX}, 2},
	{NULL, 0, { BIAS_NULL_INDEX }, 0}
};

const struct alchemy_skills nature_skills[] =
{
	{"moonray", 1, {BIAS_CHAOS_INDEX, BIAS_HEALING_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 7},
	{"regeneration", 7, {BIAS_ENHANCEMENT_INDEX, BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 5},
	{"sticks to snakes", 1, {BIAS_PLANAR_INDEX, BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 5},
	{"sunray", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 6},
	{"insect swarm", 1, {BIAS_PLANAR_INDEX, BIAS_MALEDICTION_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 6},
	{"plant growth", 6, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 4},
	{"mushroom circle", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 7},
	{"entwine", 1, {BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 7},
	{"giant growth", 7, {BIAS_ALTERATION_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 6},
	{"shrink", 1, {BIAS_ALTERATION_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 6},
	{"warp wood", 1, {BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 3},
	{"forest walk", 6, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 3},
	{"tangle trail", 6, {BIAS_OBSCUREMENT_INDEX, BIAS_CHAOS_INDEX, BIAS_NULL_INDEX}, 5},
	{"bark skin", 7, {BIAS_ALTERATION_INDEX, BIAS_PROTECTION_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 7},
	{"elemental protection", 7, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 9},
	{"stampede", 6, {BIAS_CHAOS_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 4},
	{"calm animals", 6, {BIAS_ORDER_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 3},
	{"speak with plants", 6, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 2},
	{"bounty of nature", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 2},
	{"slow cure light", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 1},
	{"slow cure serious", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 2},
	{"slow cure critical", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 3},
	{"slow heal", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 5},
	{"resist poison", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 5},
	{"slow cure poison", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 2},
	{"slow cure disease", 7, {BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 2},
	{"water breathing", 7, {BIAS_ALTERATION_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 3},
	{"sustenance", 6, {BIAS_ENHANCEMENT_INDEX, BIAS_ORDER_INDEX, BIAS_NULL_INDEX}, 2},
	{"thorn spray", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 5},
	{"star glamour", 1, {BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 2},
	{"protection from cold", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 6},
	{"wall of thorns", 6, {BIAS_PROTECTION_INDEX, BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 5},
	{NULL, 0, { BIAS_NULL_INDEX }, 0}
};

const struct alchemy_skills ancient_skills[] =
{
	{"acid blast", 1, {BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 9},
	{"change sex", 6, {BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 2},
	{"colour spray", 1, {BIAS_CHAOS_INDEX, BIAS_DAMAGE_INDEX, BIAS_NULL_INDEX}, 2},
	{"detect magic", 7, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 4},
	{"detect invis", 7, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 2},
	{"enchant armor", 1, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 7},
	{"enchant weapon", 1, {BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 9},
	{"shield", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 1},
	{"armor", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 1},
	{"haste", 7, {BIAS_KNOWLEDGE_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 8},
	{"ehiqegieegh", 7, {BIAS_PLANAR_INDEX, BIAS_HEALING_INDEX, BIAS_NULL_INDEX}, 8},
	{"slow", 1, {BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 7},
	{"word of recall", 6, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 4},
	{"identify", 1, {BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 2},
	{"cancellation", 6, {BIAS_ORDER_INDEX, BIAS_NULL_INDEX}, 6},
	{"love potion", 2, {BIAS_KNOWLEDGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 10},
	{"charm person", 1, {BIAS_KNOWLEDGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 10},
	{"plague of madness", 1, {BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 8},
	{"protection evil", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 8},
	{"protection good", 6, {BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 8},
	{"prismatic ray", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 10},
	{"prismatic spray", 1, {BIAS_DAMAGE_INDEX, BIAS_MALEDICTION_INDEX, BIAS_NULL_INDEX}, 10},
	{"transmutation", 1, {BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 5},
	{"maze", 1, {BIAS_PLANAR_INDEX, BIAS_NULL_INDEX}, 8},
	{NULL, 0, { BIAS_NULL_INDEX }, 0}
};

const struct alchemy_skills transmogrify_skills[] =
{
	{"wolfform", 6, {BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 6},
	{"hawkform", 6, {BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 5},
	{"bearform", 6, {BIAS_ALTERATION_INDEX, BIAS_ENHANCEMENT_INDEX, BIAS_NULL_INDEX}, 7},
	{"lesser hand warp", 6, {BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 3},
	{"hand warp", 6, {BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 5},
	{"greater hand warp", 6, {BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 8},
	{"lesser tentacle growth", 6, {BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 2},
	{"greater tentacle growth", 6, {BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 6},
	{"lesser carapace", 6, {BIAS_ALTERATION_INDEX, BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 3},
	{"greater carapace", 6, {BIAS_ALTERATION_INDEX, BIAS_PROTECTION_INDEX, BIAS_NULL_INDEX}, 7},
	{"lycanthropy", 6, {BIAS_ALTERATION_INDEX, BIAS_NULL_INDEX}, 8},
	{"third eye", 6, {BIAS_ALTERATION_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 7},
	{"cat eyes", 6, {BIAS_ALTERATION_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 6},
	{"eagle eyes", 6, {BIAS_ALTERATION_INDEX, BIAS_KNOWLEDGE_INDEX, BIAS_NULL_INDEX}, 2},
	{NULL, 0, { BIAS_NULL_INDEX }, 0}
};

void explosive_alchemy( CHAR_DATA *, int);
void do_createhomonculus( CHAR_DATA *, char *);
void do_createwand( CHAR_DATA *, char *);
void do_alchemicstone( CHAR_DATA *, char *);
void do_harrudimtalc( CHAR_DATA *, char *);
void do_smokescreen( CHAR_DATA *, char *);
void do_rynathsbrew( CHAR_DATA *, char *);
void do_finditems( CHAR_DATA *, char *);
void do_dye( CHAR_DATA *, char *);
void do_colorlist( CHAR_DATA *, char *);
void do_discernmagic( CHAR_DATA *, char *);
void do_delayreaction( CHAR_DATA *, char *);
void do_animategolem( CHAR_DATA *, char *);
void do_distill( CHAR_DATA *, char *);
void do_pulverize( CHAR_DATA *, char *);
void do_reforge( CHAR_DATA *, char *);
void do_boil( CHAR_DATA *, char *);
void do_dissolve( CHAR_DATA *, char *);
void do_dilute( CHAR_DATA *, char *);
void do_alchemy_skill (CHAR_DATA *, char *, int, int);
void do_mix( CHAR_DATA *, char *);
void do_brew( CHAR_DATA *, char *);
void do_make( CHAR_DATA *, char *);
void do_alchemy( CHAR_DATA *, char *, int, int, double);
void determine_color(char *, char *, int, double);
void sum_elements(struct elemental_bias_type *, const struct elemental_bias_type *);
void sum_platonics(struct platonic_bias_type *, const struct platonic_bias_type *);
void stone_biases(struct elemental_bias_type *, struct platonic_bias_type *, int [3]); 
int determine_spell_space(int [MAX_SKILL][2], const struct alchemy_skills *, int, double, int, int);
int make_bin(int);
int elemental_lookup(char *);
int elemental_retrieval(char *, int [MAX_POSSIBLE_PLATONIC_BIASES], int *);
int strongest_element(struct elemental_bias_type *, int *);
int strongest_platonic(struct platonic_bias_type *, int *);
int close_elements(struct elemental_bias_type *, int, double);
int close_platonics(struct platonic_bias_type *, int, double);
int ladder_sum(int);
double evaluate_item(OBJ_DATA *, struct elemental_bias_type *, struct platonic_bias_type *, int *);
bool basic_alchemy_sanity_checks( CHAR_DATA *, char *, int );
bool alchemy_sanity_checks(CHAR_DATA *, char *, int);
bool duplicate_ingredients( CHAR_DATA *, OBJ_DATA * [], int );
bool exists_in_color_table( char * );
bool exists_in_adjective_table( char * );
bool exists_in_space(char *, const struct alchemy_skills *);
bool retrieve_plat(char *, const struct alchemy_skills *, int [MAX_POSSIBLE_PLATONIC_BIASES], int *);
bool valid_alchemy_location(CHAR_DATA * );
bool leading_vowel( char * );
bool describe_spell(char *, char *, char *, int);
char * find_adjective( char * );
char * read_elemental_biases(struct elemental_bias_type *);
char * read_platonic_biases(struct platonic_bias_type *);

extern void free_affect args((AFFECT_DATA *af));
extern int damtype_lookup args((char * argument));

bool describe_spell(char * spell, char * element, char * strength, int level)
{
	int power = 0;
	int temp[MAX_POSSIBLE_PLATONIC_BIASES];
	int bin = elemental_retrieval(spell, temp, &power);
	if (bin == 0)
	{
		if (level < 20) sprintf(strength, "weak");
		else if (level < 35) sprintf(strength, "moderately-powered");
		else if (level < 45) sprintf(strength,  "powerful");
		else sprintf(strength, "very powerful");
		return FALSE;
	}
	if (bin & make_bin(BIAS_VOID_INDEX)) sprintf(element, "void");
	if (bin & make_bin(BIAS_SPIRIT_INDEX)) sprintf(element, "spirit");
	if (bin & make_bin(BIAS_WATER_INDEX)) sprintf(element, "water");
	if (bin & make_bin(BIAS_AIR_INDEX)) sprintf(element, "air");
	if (bin & make_bin(BIAS_FIRE_INDEX)) sprintf(element, "fire");
	if (bin & make_bin(BIAS_EARTH_INDEX)) sprintf(element, "earth");
	if (bin & make_bin(BIAS_NATURE_INDEX)) sprintf(element, "nature");
	if (bin & make_bin(BIAS_ANCIENT_INDEX)) sprintf(element, "ancient magic");
	if (bin & make_bin(BIAS_TRANSMOGRIFY_INDEX)) sprintf(element, "transmogrification");

	if (power < 4) sprintf(strength, "weak");
	else if (power < 7) sprintf(strength, "moderately-powered");
	else if (power < 9) sprintf(strength, "powerful");
	else sprintf(strength, "very powerful");
	
	return TRUE;
}

int ladder_sum(int number)
{
	int total = 0, i;
	for (i = 1; i < number; i++) total += i;
	return total;
}

void stone_biases(struct elemental_bias_type * elem, struct platonic_bias_type * plat, int vals[3])
{
	int i;
	for (i = 0; i < 2; i++)
	{
		if (vals[i] & make_bin(BIAS_VOID_INDEX)) elem->bias_void += 1;
		if (vals[i] & make_bin(BIAS_SPIRIT_INDEX)) elem->bias_spirit += 1;
		if (vals[i] & make_bin(BIAS_WATER_INDEX)) elem->bias_water += 1;
		if (vals[i] & make_bin(BIAS_FIRE_INDEX)) elem->bias_fire += 1;
		if (vals[i] & make_bin(BIAS_AIR_INDEX)) elem->bias_air += 1;
		if (vals[i] & make_bin(BIAS_EARTH_INDEX)) elem->bias_earth += 1;
		if (vals[i] & make_bin(BIAS_NATURE_INDEX)) elem->bias_nature += 1;
		if (vals[i] & make_bin(BIAS_ANCIENT_INDEX)) elem->bias_ancient += 1;
		if (vals[i] & make_bin(BIAS_TRANSMOGRIFY_INDEX)) elem->bias_transmogrify += 1;
	}
	if (vals[2] & make_bin(BIAS_DAMAGE_INDEX)) plat->bias_damage += 1;
	if (vals[2] & make_bin(BIAS_MALEDICTION_INDEX)) plat->bias_malediction += 1;
	if (vals[2] & make_bin(BIAS_PROTECTION_INDEX)) plat->bias_protection += 1;
	if (vals[2] & make_bin(BIAS_ORDER_INDEX)) plat->bias_order += 1;
	if (vals[2] & make_bin(BIAS_ENHANCEMENT_INDEX)) plat->bias_enhancement += 1;
	if (vals[2] & make_bin(BIAS_CHAOS_INDEX)) plat->bias_chaos += 1;
	if (vals[2] & make_bin(BIAS_ALTERATION_INDEX)) plat->bias_alteration += 1;
	if (vals[2] & make_bin(BIAS_PLANAR_INDEX)) plat->bias_planar += 1;
	if (vals[2] & make_bin(BIAS_KNOWLEDGE_INDEX)) plat->bias_knowledge += 1;
	if (vals[2] & make_bin(BIAS_OBSCUREMENT_INDEX)) plat->bias_obscurement += 1;
	if (vals[2] & make_bin(BIAS_HEALING_INDEX)) plat->bias_healing += 1;
	return;
}

int elemental_retrieval(char * argument, int plist[MAX_POSSIBLE_PLATONIC_BIASES], int * power)
{
	int bin = 0;
	if (retrieve_plat(argument, &void_skills[0], plist, power)) bin = bin | make_bin(BIAS_VOID_INDEX);
	if (retrieve_plat(argument, &spirit_skills[0], plist, power)) bin = bin | make_bin(BIAS_SPIRIT_INDEX);
	if (retrieve_plat(argument, &earth_skills[0], plist, power)) bin = bin | make_bin(BIAS_EARTH_INDEX);
	if (retrieve_plat(argument, &air_skills[0], plist, power)) bin = bin | make_bin(BIAS_AIR_INDEX);
	if (retrieve_plat(argument, &water_skills[0], plist, power)) bin = bin | make_bin(BIAS_WATER_INDEX);
	if (retrieve_plat(argument, &fire_skills[0], plist, power)) bin = bin | make_bin(BIAS_FIRE_INDEX);
	if (retrieve_plat(argument, &nature_skills[0], plist, power)) bin = bin | make_bin(BIAS_NATURE_INDEX);
	if (retrieve_plat(argument, &ancient_skills[0], plist, power)) bin = bin | make_bin(BIAS_ANCIENT_INDEX);
	if (retrieve_plat(argument, &transmogrify_skills[0], plist, power)) bin = bin | make_bin(BIAS_TRANSMOGRIFY_INDEX);
	return bin;
}

int elemental_lookup(char * argument)
{
	int bin = 0;
	if (exists_in_space(argument, &void_skills[0])) bin = bin | make_bin(BIAS_VOID_INDEX);
	if (exists_in_space(argument, &spirit_skills[0])) bin = bin | make_bin(BIAS_SPIRIT_INDEX);
	if (exists_in_space(argument, &earth_skills[0])) bin = bin | make_bin(BIAS_EARTH_INDEX);
	if (exists_in_space(argument, &air_skills[0])) bin = bin | make_bin(BIAS_AIR_INDEX);
	if (exists_in_space(argument, &water_skills[0])) bin = bin | make_bin(BIAS_WATER_INDEX);
	if (exists_in_space(argument, &fire_skills[0])) bin = bin | make_bin(BIAS_FIRE_INDEX);
	if (exists_in_space(argument, &nature_skills[0])) bin = bin | make_bin(BIAS_NATURE_INDEX);
	if (exists_in_space(argument, &ancient_skills[0])) bin = bin | make_bin(BIAS_ANCIENT_INDEX);
	if (exists_in_space(argument, &transmogrify_skills[0])) bin = bin | make_bin(BIAS_TRANSMOGRIFY_INDEX);
	return bin;
}

bool leading_vowel(char * argument)
{
	switch (LOWER(argument[0]))
	{
		case 'a':
		case 'e':
		case 'i':
		case 'o':
		case 'u': return TRUE; break;
		default: return FALSE; break;
	}
	return FALSE;
}

char * read_elemental_biases(struct elemental_bias_type * elem)
{
	char buf[MAX_STRING_LENGTH];
	buf[0] = '\0';
	
	if (elem->bias_spirit != 0) sprintf(buf, "%sSpirit: %d\n", buf, elem->bias_spirit);
	if (elem->bias_water != 0) sprintf(buf, "%sWater: %d\n", buf, elem->bias_water);
	if (elem->bias_air != 0) sprintf(buf, "%sAir: %d\n", buf, elem->bias_air);
	if (elem->bias_earth != 0) sprintf(buf, "%sEarth: %d\n", buf, elem->bias_earth);
	if (elem->bias_fire != 0) sprintf(buf, "%sFire: %d\n", buf, elem->bias_fire);
	if (elem->bias_void != 0) sprintf(buf, "%sVoid: %d\n", buf, elem->bias_void);
	if (elem->bias_nature != 0) sprintf(buf, "%sNature: %d\n", buf, elem->bias_nature);
	if (elem->bias_ancient != 0) sprintf(buf, "%sAncient: %d\n", buf, elem->bias_ancient);
	if (elem->bias_transmogrify != 0) sprintf(buf, "%sTransmogrify: %d\n", buf, elem->bias_transmogrify);
	
	return str_dup(buf);
}

char * read_platonic_biases(struct platonic_bias_type * plat)
{
	char buf[MAX_STRING_LENGTH];
	buf[0] = '\0';
	
	if (plat->bias_damage != 0) sprintf(buf, "%sDamage: %d\n", buf, plat->bias_damage);
	if (plat->bias_malediction != 0) sprintf(buf, "%sMalediction: %d\n", buf, plat->bias_malediction);
	if (plat->bias_protection != 0) sprintf(buf, "%sProtection: %d\n", buf, plat->bias_protection);
	if (plat->bias_order != 0) sprintf(buf, "%sOrder: %d\n", buf, plat->bias_order);
	if (plat->bias_enhancement != 0) sprintf(buf, "%sEnhancement: %d\n", buf, plat->bias_enhancement);
	if (plat->bias_chaos != 0) sprintf(buf, "%sChaos: %d\n", buf, plat->bias_chaos);
	if (plat->bias_obscurement != 0) sprintf(buf, "%sObscurement: %d\n", buf, plat->bias_obscurement);
	if (plat->bias_healing != 0) sprintf(buf, "%sHealing: %d\n", buf, plat->bias_healing);
	if (plat->bias_knowledge != 0) sprintf(buf, "%sKnowledge: %d\n", buf, plat->bias_knowledge);
	if (plat->bias_planar != 0) sprintf(buf, "%sPlanar: %d\n", buf, plat->bias_planar);
	if (plat->bias_alteration != 0) sprintf(buf, "%sAlteration: %d\n", buf, plat->bias_alteration);
	
	return str_dup(buf);
}

void explosive_alchemy(CHAR_DATA * ch, int power)
{
	int door, dam;
	CHAR_DATA * vics;
	
	affect_strip(ch, gsn_delayreaction);
	
	act("With a resounding *BOOM*, your reaction explodes!", ch, NULL, NULL, TO_CHAR);
	act("With a resounding *BOOM*, $n's reaction explodes!", ch, NULL, NULL, TO_ROOM);
	act("Burning fluids and shards of glass fly all about you!", ch, NULL, NULL, TO_CHAR);
	act("Burning fluids and shards of glass fly all about you!", ch, NULL, NULL, TO_ROOM);
	for (vics = ch->in_room->people; vics != NULL; vics = vics->next_in_room)
	{
		if (vics == NULL) break;
		if (!is_safe_spell(ch, vics, FALSE))
		{
			dam = dice(ch->level, power) + ch->level/3;
			if (saves_spell(ch->level, NULL, vics, DAM_FIRE)) dam /= 2;
			damage_old(ch, vics, dam, gsn_delayreaction, DAM_FIRE, TRUE);
		}
	}							
	for (door = 0; door < 6; door++)
	{
		if (ch->in_room->exit[door])
		{
			act("You hear the muffled sound of an explosion nearby!", 
				ch->in_room->exit[door]->u1.to_room->people, NULL, NULL, TO_CHAR);
			act("You hear the muffled sound of an explosion nearby!", 
				ch->in_room->exit[door]->u1.to_room->people, NULL, NULL, TO_ROOM);
		}
	}    
}

//This function assigns a numerical value to a string
//Used to make a number based on a player's name for random seeding
int convert_to_number(const char * argument)
{
	char buf[MAX_STRING_LENGTH];
	int total = 0, index = 0, power = 1;

	sprintf(buf, "%s", argument);
	while (buf[index] != '\0')
	{
		total += (power * ((int)buf[index]));
		power *= 2;
		index++;
	}
	
	return total;
}

void do_createhomonculus(CHAR_DATA * ch, char * argument)
{
	AFFECT_DATA af = {0}, *paf;
    CHAR_DATA * homonculus;
	char buf[MAX_STRING_LENGTH];

	if (get_skill(ch, gsn_createhomonculus) <= 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}
	if (ch->mana < skill_table[gsn_createhomonculus].min_mana)
	{
		send_to_char("You do not have the energy to create a homonculus.\n\r", ch);
		return;
	}
	if ((paf = get_affect(ch, gsn_createhomonculus)) != NULL)
	{
		if (paf->location == 0) send_to_char("You already have a homonculus!\n\r", ch);
		else send_to_char("You have not yet recovered from the loss of your last homonculus.\n\r", ch);
		return;
	}
	if (ch->in_room->room_flags & ROOM_UBERDARK)
	{
		send_to_char("You wouldn't be able to see well enough to make a homonculus here.\n\r", ch);
		return;
	}

	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_createhomonculus].beats));
	expend_mana(ch, skill_table[gsn_createhomonculus].min_mana);
	
	if (number_percent() >= get_skill(ch, gsn_createhomonculus))
	{
		act("You perform the ancient rite, carving out a pound of your own flesh, but fail to create a homonculus.",
			ch, NULL, NULL, TO_CHAR);
		act("$n performs a ritual of some sort, carving out a pound of $s own flesh, but nothing happens.",
			ch, NULL, NULL, TO_ROOM);
		damage_old(ch, ch, number_range(ch->level * 2, ch->level * 4), gsn_maze, DAM_NONE, TRUE);
		check_improve(ch, NULL, gsn_createhomonculus, FALSE, 1);
		return;
	}
	act("You perform the ancient rite, carving out a pound of your own flesh, and a homonculus is created!",
		ch, NULL, NULL, TO_CHAR);
	act("$n performs a ritual of some sort, carving out a pound of $s own flesh, and a homonculus is created!",
		ch, NULL, NULL, TO_ROOM);
	damage_old(ch, ch, number_range(ch->level * 2, ch->level * 4), gsn_maze, DAM_NONE, TRUE);
	check_improve(ch, NULL, gsn_createhomonculus, TRUE, 1);

	homonculus = create_mobile(get_mob_index(MOB_VNUM_HOMONCULUS));
	homonculus->level = ch->level;
	homonculus->max_hit = ch->max_hit;
	homonculus->hit = homonculus->max_hit;

	sprintf(buf, "homonculus %s", ch->name);
    setName(*homonculus, buf);

	sprintf(buf, "%s's homonculus", ch->name);
	free_string(homonculus->short_descr);
	homonculus->short_descr = str_dup(buf);

	sprintf(buf, "%s is here.\n\r", homonculus->short_descr);
	buf[0] = UPPER(buf[0]);
	free_string(homonculus->long_descr);
	homonculus->long_descr = str_dup(buf);

	sprintf(buf, "This homonculus is a miniature version of:\n\r%s", ch->description);
	free_string(homonculus->description);
	homonculus->description = str_dup(buf);

	af.where     = TO_AFFECTS;
	af.type      = gsn_createhomonculus;
	af.level     = ch->level;
	af.duration  = -1;
	af.modifier  = 500;
	af.point     = (void *) homonculus;
	affect_to_char(ch, &af);
	
	af.point     = (void *) ch;
	affect_to_char(homonculus, &af);

	char_to_room(homonculus, ch->in_room);
}

void do_createwand(CHAR_DATA * ch, char * argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_STRING_LENGTH];
	OBJ_DATA * obj, * orig_obj;
	int type, adj_index;
	bool crit_success = FALSE;
	
	if (get_skill(ch, gsn_createwand) <= 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}
	if (ch->mana < skill_table[gsn_createwand].min_mana)
	{
		send_to_char("You are too tired to craft a wand or stave right now.\n\r", ch);
		return;
	}
	if (argument[0] == '\0')
	{
		send_to_char("Do you want to create a wand or a stave?\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg);
	if (LOWER(arg[0]) == 'w')
		type = ITEM_WAND;
	else if (LOWER(arg[0]) == 's')
		type = ITEM_STAFF;
	else
	{
		send_to_char("You can only create wands and staves.\n\r", ch);
		return;
	}
	if (argument[0] == '\0')
	{
		sprintf(buf, "Craft the %s out of what?\n\r", type == ITEM_WAND ? "wand" : "staff");
		send_to_char(buf, ch);
		return;
	}
	
	argument = one_argument(argument, arg);
	if ((orig_obj = get_obj_carry(ch, arg, ch)) == NULL)
	{
		sprintf(buf, "You aren't carrying any %s.\n\r", arg);
		send_to_char(buf, ch);
		return;
	}
	if (!strcmp(material_table[orig_obj->material].name, "unknown") 
	|| orig_obj->pIndexData->vnum == VNUM_ALCHEMY_PRODUCT
	|| orig_obj->item_type == ITEM_STAFF
	|| orig_obj->item_type == ITEM_WAND
	|| material_table[orig_obj->material].charges < 1)
	{
		send_to_char("You can't make anything out of that!\n\r", ch);
		return;
	}
		
	expend_mana(ch, skill_table[gsn_createwand].min_mana);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_createwand].beats));
	
	if (number_percent() < get_skill(ch, gsn_createwand))
	{
		if (number_percent() < (get_skill(ch, gsn_createwand) / 33)) crit_success = TRUE;
		obj = create_object(get_obj_index(VNUM_ALCHEMY_PRODUCT), ch->level);
		obj->value[0] = orig_obj->level + (ch->level / 17) + 
			(number_percent() < get_skill(ch, gsn_createwand) ? 2 : -2)
			+ ((material_table[orig_obj->material].charges - 4) * 3);
		obj->value[1] = material_table[orig_obj->material].charges + (crit_success ? 1 : 0);
		obj->value[2] = 0;
		obj->value[3] = -1;
		obj->item_type = type;
		obj->material = orig_obj->material;
		obj->level = UMIN(obj->value[0], ch->level);

		if (crit_success || number_percent() < 40)
		{
			adj_index = number_range(0, START_NICE_ADJECTIVES - 1);
			if (crit_success) adj_index = number_range(START_NICE_ADJECTIVES, MAX_WAND_ADJECTIVES - 1);
			sprintf(buf, "%s %s %s formed of %s",
				(leading_vowel(wand_adjective_table[adj_index].adjective) ? "an" : "a"),
				wand_adjective_table[adj_index].adjective,
				type == ITEM_WAND ? "wand" : "staff",
				material_table[obj->material].name);
		}
		else
		{
			sprintf(buf, "a %s formed of %s", type == ITEM_WAND ? "wand" : "staff", 
				material_table[obj->material].name);
		}
		free_string(obj->short_descr);
		obj->short_descr = str_dup(buf);
        setName(*obj, buf);
		sprintf(buf, "%s lies here.", obj->short_descr);
		buf[0] = UPPER(buf[0]);
		free_string(obj->description);
		obj->description = str_dup(buf);
		obj->weight = (type == ITEM_WAND ? 10 : 50);
		obj->size = (type == ITEM_WAND ? 1 : 2);
	
		sprintf(buf, "You skillfully craft %s from %s.\n\r", obj->short_descr, orig_obj->short_descr);
		send_to_char(buf, ch);
		act("$n skillfully and swiftly crafts $p.", ch, obj, NULL, TO_ROOM);
		check_improve(ch, NULL, gsn_createwand, TRUE, 1);
		obj_to_char(obj, ch);
	}
	else
	{
		sprintf(buf, "You attempt to craft a %s from %s, but only destroy it.\n\r", 
			type == ITEM_WAND ? "wand" : "staff", orig_obj->short_descr);
		send_to_char(buf, ch);
		sprintf(buf, "%s tries to make a %s, but fails.", ch->name, type == ITEM_WAND ? "wand" : "staff");
		act(buf, ch, NULL, NULL, TO_ROOM);
		check_improve(ch, NULL, gsn_createwand, FALSE, 1);
	}
	extract_obj(orig_obj);
}

void do_alchemicstone(CHAR_DATA * ch, char * argument)
{
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA * new_obj;
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
	
	if (get_skill(ch, gsn_alchemicstone) <= 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}
	if (is_affected(ch, gsn_alchemicstone))
	{
		send_to_char("You are not ready to create another alchemy stone yet.\n\r", ch);
		return;
	}
	if (ch->mana < skill_table[gsn_alchemicstone].min_mana)
	{
		send_to_char("You are too weary to make an alchemy stone.\n\r", ch);
		return;
	}
	
	expend_mana(ch, skill_table[gsn_alchemicstone].min_mana);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_alchemicstone].beats));

	if (number_percent() >= get_skill(ch, gsn_alchemicstone))
	{
		send_to_char("You attempt to create a stone for alchemical purposes, but fail.\n\r", ch);
		act("$n attempts to create an alchemist's stone, but fails.", ch, NULL, NULL, TO_ROOM);
		check_improve(ch, NULL, gsn_alchemicstone, FALSE, 1);
		return;
	}

	new_obj = create_object(get_obj_index(VNUM_ALCHEMY_PRODUCT), ch->level);
	free_string(new_obj->short_descr);
	new_obj->short_descr = str_dup("an alchemist's stone");
    setName(*new_obj, "alchemists alchemist's stone alchemy");
	sprintf(buf, "%s lies here.", new_obj->short_descr);
	buf[0] = UPPER(buf[0]);
	free_string(new_obj->description);
	new_obj->description = str_dup(buf);
	new_obj->level = ch->level;
	new_obj->item_type = ITEM_TRASH;
	new_obj->material = material_lookup("stone");

	//Seed based on name
	srand((unsigned)(convert_to_number(ch->name)));

	new_obj->objvalue[0] = ((int)rand()) % 30000;
	new_obj->objvalue[1] = ((int)rand()) % 30000;
	new_obj->objvalue[2] = ((int)rand()) % 30000;
	new_obj->objvalue[3] = ((int)rand()) % 30000;

	af.where        = TO_OBJECT;
	af.type         = gsn_alchemicstone;
	af.level        = ch->level;
	af.duration     = -1;
	af.location     = APPLY_NONE;
	af.modifier     = 0;
	af.bitvector    = 0;
	affect_to_obj(new_obj, &af);

	af.where = TO_AFFECTS;
	af.duration = 100 - (ch->level >= 51 ? 30 : 0);
	affect_to_char(ch, &af);

	check_improve(ch, NULL, gsn_alchemicstone, TRUE, 1);

	act("$n skillfully crafts a new alchemic stone!", ch, NULL, NULL, TO_ROOM);
	act("You skillfully craft a new alchemic stone!", ch, NULL, NULL, TO_CHAR);
	obj_to_char(new_obj, ch);
}

void do_harrudimtalc( CHAR_DATA * ch, char * argument)
{
	char arg[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	int i, count;

	if (get_skill(ch, gsn_harrudimtalc) <= 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}

	if (ch->in_room == NULL) return;
	
	if ((victim = ch->fighting) == NULL && argument[0] == '\0')
	{
		send_to_char( "Use the talc against whom?\n\r", ch );
		return;
	}
	if (argument[0] != '\0')
	{
		argument = one_argument(argument, arg);
		if ((victim = get_char_room(ch, arg)) == NULL)
		{
			send_to_char("They aren't here.\n\r", ch);
			return;
		}
	}
	if (ch->mana < skill_table[gsn_harrudimtalc].min_mana)
	{
		send_to_char("You are too tired to concoct some Harrudim Talc.\n\r", ch);
		return;
	}
	expend_mana(ch, skill_table[gsn_harrudimtalc].min_mana);
		
	WAIT_STATE(ch, skill_table[gsn_harrudimtalc].beats);

	if (number_percent() < get_skill(ch, gsn_harrudimtalc))
	{
		act("You throw some Harrudim Talc at $N, blasting $M with several small explosions!", 
				ch, NULL, victim, TO_CHAR);
		act("$n throws some Harrudim Talc at $N, blasting $M with several small explosions!",
				ch, NULL, victim, TO_NOTVICT);
		act("$n throws some Harrudim Talc at you, blasting you with several small explosions!",
				ch, NULL, victim, TO_VICT);
		count = 0;
		for (i = 0; i < 6 + (ch->level / 8); i++)
		{
			if (number_bits(1) == 0 || count < 3)
			{
				damage(ch, victim, 3 + number_range(ch->level / 15, ch->level / 6), 
					gsn_harrudimtalc, number_bits(1) == 0 ? DAM_FIRE : DAM_BASH, TRUE);
				count++;
			}
		}

		//Chance of lag
		if (number_percent() < (count * 2.5 + ch->level - victim->level))
		{
			act("$N is dazed by all the blasts!", ch, NULL, victim, TO_CHAR);
			act("$N is dazed by all the blasts!", ch, NULL, victim, TO_NOTVICT);
			act("You are dazed by all the blasts!", ch, NULL, victim, TO_VICT);
			WAIT_STATE(victim, UMAX(victim->wait, 2*PULSE_VIOLENCE));
		}
		check_improve(ch, NULL, gsn_harrudimtalc, TRUE, 1);
	}
	else
	{
		act("You throw some Harrudim Talc at $N, but it barely makes a pop.",
			ch, NULL, victim, TO_CHAR);
		act("$n throws some Harrudim Talc at $N, but it barely makes a pop.",
			ch, NULL, victim, TO_NOTVICT);
		act("$n throws some Harrudim Talc at you, but it barely makes a pop.",
			ch, NULL, victim, TO_VICT);
		damage(ch, victim, number_bits(1) == 0 ? 1 : 0,
			gsn_harrudimtalc, number_bits(1) == 0 ? DAM_FIRE : DAM_BASH, TRUE);
		check_improve(ch, NULL, gsn_harrudimtalc, FALSE, 1);
	}
	
	return;
}

void do_smokescreen( CHAR_DATA * ch, char * argument)
{
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
	
	if (get_skill(ch, gsn_smokescreen) <= 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}
	if (is_affected(ch, gsn_smokescreen))
	{
		send_to_char("You can't create another smokescreen just yet.\n\r", ch);
		return;
	}
        if (ch->in_room && ch->in_room->sector_type == SECT_UNDERWATER)
        {
	    send_to_char("You can't create a smokescreen underwater.\n\r",ch);
   	    return;
        }
	if (room_is_affected(ch->in_room, gsn_smokescreen))
	{
		send_to_char("There is already a screen of smoke here.\n\r", ch);
		return;
	}
	
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_smokescreen].beats));
	if (number_percent() >= get_skill(ch, gsn_smokescreen))
	{
		act("You throw up some smoke, but it quickly dissipates.", ch, NULL, NULL, TO_CHAR);
		act("$n throws up some smoke, but it quickly dissipates.", ch, NULL, NULL, TO_ROOM);
		check_improve(ch, NULL, gsn_smokescreen, FALSE, 1);
		return;
	}		
	
	act("You throw up some smoke, and it quickly pervades the room!", ch, NULL, NULL, TO_CHAR);
	act("$n throws up some smoke, and it quickly pervades the room!", ch, NULL, NULL, TO_ROOM);
	check_improve(ch, NULL, gsn_smokescreen, TRUE, 1);
	af.where     = TO_ROOM;
	af.type      = gsn_smokescreen;
	af.level     = ch->level;
	af.duration  = ch->level/5;
	af.modifier  = 0;
	af.location  = 0;
	af.bitvector = 0;
	affect_to_room(ch->in_room, &af);

	af.where = TO_AFFECTS;
	affect_to_char(ch, &af);
					
	return;
}

void do_rynathsbrew( CHAR_DATA * ch, char * argument)
{
	char arg[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

	if (get_skill(ch, gsn_rynathsbrew) <= 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}

	if (ch->in_room == NULL) return;

	if ((victim = ch->fighting) == NULL && argument[0] == '\0')
	{
		send_to_char( "Use Rynath's brew against whom?\n\r", ch );
		return;
	}
	if (argument[0] != '\0')
	{
		argument = one_argument(argument, arg);
		if ((victim = get_char_room(ch, arg)) == NULL)
		{
			send_to_char("They aren't here.\n\r", ch);
			return;
		}
	}
	
	if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE)) return;
	if (ch->mana < skill_table[gsn_rynathsbrew].min_mana)
	{
		send_to_char("You are too weary to concoct Rynath's Brew.\n\r", ch);
		return;
	}
	expend_mana(ch, skill_table[gsn_rynathsbrew].min_mana);
	
	WAIT_STATE( ch, skill_table[gsn_rynathsbrew].beats);

	if (number_percent() < get_skill(ch, gsn_rynathsbrew))
	{
		act("You fling Rynath's brew at $N, and $E screams in pain as the acid burns $M!",
			ch, NULL, victim, TO_CHAR);
		act("$n flings Rynath's brew at $N, and $E screams in pain as the acid burns $M!",
			ch, NULL, victim, TO_NOTVICT);
		act("$n flings Rynath's brew at you, and you scream in pain as the acid burns you!",
			ch, NULL, victim, TO_VICT);
		damage(ch, victim, number_fuzzy(20 + (ch->level / 3)), gsn_rynathsbrew, DAM_ACID, TRUE);
		
		//Give them -chr
		af.where        = TO_AFFECTS;
		af.type         = gsn_rynathsbrew;
		af.level        = ch->level;
		af.duration     = ch->level;
		af.modifier     = -1;
		af.location     = APPLY_CHR;
		af.bitvector    = 0;
		affect_to_char(victim, &af);

		af.duration	= ch->level / 8;
		af.modifier	= number_fuzzy(ch->level / 8);
		af.location	= APPLY_NONE;
		affect_to_char(victim, &af);
		
		check_improve(ch, NULL, gsn_rynathsbrew, TRUE, 1);
	}
	else
	{
		act("You fling Rynath's brew at $N, but $E appears largely unaffected.",
			ch, NULL, victim, TO_CHAR);
		act("$n flings Rynath's brew at $N, but $E appears largely unaffected.",
			ch, NULL, victim, TO_NOTVICT);
		act("$n flings Rynath's brew at you, but you are largely unaffected.",
			ch, NULL, victim, TO_VICT);
		damage(ch, victim, number_bits(1) == 0 ? 1 : 0,gsn_rynathsbrew, DAM_ACID, TRUE);
		check_improve(ch, NULL, gsn_rynathsbrew, FALSE, 1);
	}
	
	return;					
}

void do_finditems( CHAR_DATA * ch, char * argument)
{
    char buf[MAX_STRING_LENGTH];
    char sstr[MAX_STRING_LENGTH];
    char *stype;
    int t=0,cost=0,maxlevel=0,dur=0;

    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    char * arg_backup;

    if (get_skill(ch, gsn_finditems) <= 0)
    {
       send_to_char("Huh?\n\r", ch);
       return;
    }
	
    if (is_affected(ch, gsn_finditems))
    {
	send_to_char("You already have a contract out to find an item.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
       send_to_char("What item did you wish to find, and how thoroughly?\n\r", ch);
       return;
    }

    stype = one_argument(argument,sstr);

    if (stype[0] == '\0' || sstr[0] == '\0')
    {
        send_to_char("What did you wish to find, and how thoroughly?\n\r",ch);
        return;
    }

    if (!str_cmp(stype,"common"))
    {
        t = 1;
        cost = 500;
        maxlevel = 20;
	dur = 0;	
    }
    else if (!str_cmp(stype,"uncommon"))
    {
        t = 2;
        cost = 1000;
        maxlevel = 40;
	dur = 2;
    }
    else if (!str_cmp(stype,"rare"))
    {
	t = 3;
        cost = 2500;
        maxlevel = 51;
	dur = 5;
    }
    else
    {
	send_to_char("How thorough did you wish the search to be?\n\r",ch);
       	return;
    }

    if (!dec_player_bank(ch,value_to_coins(cost,FALSE)))
	if (!dec_player_coins(ch,value_to_coins(cost,FALSE)))
   	{ 
       	    send_to_char("You don't have enough money to pay for such services.\n\r", ch);
       	    return;
    	}

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_finditems].beats));

    if (number_percent() >= get_skill(ch, gsn_finditems))
    {
       	send_to_char("You fail to go through the proper channels, and end up wasting your money.\n\r", ch);
       	check_improve(ch, NULL, gsn_finditems, FALSE, 1);
       	return;
    }
    check_improve(ch, NULL, gsn_finditems, TRUE, 1);
    arg_backup = str_dup(sstr);

    if (ch->class_num == global_int_class_watcher)
	sprintf(buf, "You petition the Office of Records to track down %s.\n\r",sstr);
    else
	sprintf(buf, "You contact your guild to search for %s.\n\r", sstr);
    send_to_char(buf, ch);

    af.where     = TO_AFFECTS;
    af.type      = gsn_finditems;
    af.level     = maxlevel;
    af.duration  = dur;
    af.modifier  = 0;
    af.location  = APPLY_HIDE;
    af.bitvector = 0;
    af.point = (void *) arg_backup;
    affect_to_char(ch, &af);

    return;
}

void do_colorlist(CHAR_DATA * ch, char * argument)
{
	char buf[MAX_STRING_LENGTH];
	BUFFER *buffer;
	int i = 0;
	
	buffer = new_buf();
	while (alchemy_color_table[i].color_spirit != NULL)
	{
		sprintf(buf, "%-25s %-25s %-25s\n\r", alchemy_color_table[i].color_spirit,
				alchemy_color_table[i].color_water, alchemy_color_table[i].color_air);
		add_buf(buffer, buf);
		sprintf(buf, "%-25s %-25s %-25s\n\r", alchemy_color_table[i].color_earth,
				alchemy_color_table[i].color_fire, alchemy_color_table[i].color_void);
		add_buf(buffer, buf);
		sprintf(buf, "%-25s %-25s %-25s\n\r", alchemy_color_table[i].color_nature,
				alchemy_color_table[i].color_ancient, alchemy_color_table[i].color_transmogrify);
		add_buf(buffer, buf);
		i++;
	}

	page_to_char(buf_string(buffer),ch);
	free_buf(buffer);
	
	return;
}

void do_dye( CHAR_DATA * ch, char * argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_STRING_LENGTH];
	char * adj, * type_string, * new_color;
	OBJ_DATA * obj;
	
	if (ch->class_num != global_int_class_alchemist)
	{
		send_to_char("Only alchemists can dye potions.\n\r", ch);
		return;
	}
	if (ch->mana < skill_table[gsn_dye].min_mana)
	{
		send_to_char("You are too tired to dye anything right now.\n\r", ch);
		return;
	}
	if (argument[0] == '\0')
	{
		send_to_char("Dye what?\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg);
	if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
	{
		sprintf(buf, "You aren't carrying any %s.\n\r", arg);
		send_to_char(buf, ch);
		return;
	}
	if (obj->pIndexData->vnum != VNUM_ALCHEMY_PRODUCT || obj->item_type == ITEM_TRASH)
	{
		send_to_char("You can only dye the products of alchemical work.\n\r", ch);
		return;
	}
	if (obj->item_type != ITEM_POTION)
	{
		send_to_char("You can only dye potions.\n\r", ch);
		return;
	}
	if (argument[0] == '\0')
	{
		send_to_char("How do you wish to dye it?\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg);
	if (!exists_in_color_table(arg))
	{
		send_to_char("Invalid color.\n\r", ch);
		return;
	}

	expend_mana(ch, skill_table[gsn_dye].min_mana);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_dye].beats));
	
	new_color = str_dup(arg);
	adj = str_dup(find_adjective(obj->short_descr));
	switch (obj->item_type)
	{
		case ITEM_POTION: type_string = str_dup("potion"); break;
		case ITEM_PILL: type_string = str_dup("pill"); break;
		case ITEM_OIL: type_string = str_dup("oil"); break;
		default: type_string = str_dup("potion"); break;
	}

	sprintf(buf, "%s adds some dyeing agents to %s, and it turns %s.", ch->name, obj->short_descr, new_color);
	act(buf, ch, NULL, NULL, TO_ROOM);
	sprintf(buf, "You add some dyes to %s, and it turns %s.\n\r", obj->short_descr, new_color);
	send_to_char(buf, ch);
	
	if (adj[0] != '\0')
		sprintf(buf, "%s %s %s %s", leading_vowel(adj) ? "an" : "a", adj, new_color, type_string);
	else
		sprintf(buf, "%s %s %s", leading_vowel(new_color) ? "an" : "a", new_color, type_string);
	free_string(obj->short_descr);
	obj->short_descr = str_dup(buf);
    setName(*obj, buf);
	sprintf(buf, "%s lies here.\n\r", obj->short_descr);
	buf[0] = UPPER(buf[0]);
	free_string(obj->description);
	obj->description = str_dup(buf);
	free_string(new_color);
	free_string(adj);
}

char * find_adjective(char * argument)
{
	char arg[MAX_STRING_LENGTH];
	
	while (argument[0] != '\0')
	{
		argument = one_argument(argument, arg);
		if (exists_in_adjective_table(arg)) return str_dup(arg);
	}
	return str_dup("");
}

bool exists_in_color_table(char * argument)
{
	int i = 0;
	
	while (alchemy_color_table[i].color_spirit != NULL)
	{
		if (!str_cmp(argument, alchemy_color_table[i].color_spirit)) return TRUE;
		if (!str_cmp(argument, alchemy_color_table[i].color_water)) return TRUE;
		if (!str_cmp(argument, alchemy_color_table[i].color_air)) return TRUE;
		if (!str_cmp(argument, alchemy_color_table[i].color_earth)) return TRUE;
		if (!str_cmp(argument, alchemy_color_table[i].color_fire)) return TRUE;
		if (!str_cmp(argument, alchemy_color_table[i].color_void)) return TRUE;
		if (!str_cmp(argument, alchemy_color_table[i].color_nature)) return TRUE;
		if (!str_cmp(argument, alchemy_color_table[i].color_ancient)) return TRUE;
		if (!str_cmp(argument, alchemy_color_table[i].color_transmogrify)) return TRUE;
		i++;
	}
	
	return FALSE;
}

bool exists_in_adjective_table(char * argument)
{
	int i;

	for (i = 0; i < MAX_ALCHEMY_ADJECTIVES; i++)
		if (!str_cmp(argument, alchemy_adjective_table[i].adjective)) return TRUE;

	return FALSE;
}

bool valid_alchemy_location(CHAR_DATA * ch)
{
	if (IS_SET(ch->in_room->room_flags,ROOM_VAULT))
	{
		send_to_char("You can't do that here.\n\r", ch);
		return FALSE;
	}
	return TRUE;
}

void do_discernmagic( CHAR_DATA * ch, char * argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_STRING_LENGTH];
	OBJ_DATA * obj;
	int i, masked_gsn;
	AFFECT_DATA * poisoned;
	bool is_magic = TRUE;
	
	if (get_skill(ch, gsn_discernmagic) == 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}
	if (argument[0] == '\0')
	{
		send_to_char("Discern the properties of what?\n\r", ch);
		return;
	}
	if (ch->mana < skill_table[gsn_discernmagic].min_mana)
	{
		send_to_char("You are too weary to discern magical properties right now.\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg);
	if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
	{
		sprintf(buf, "You aren't carrying any %s.\n\r", arg);
		send_to_char(buf, ch);
		return;
	}
	switch (obj->item_type)
	{
		case ITEM_POTION:
		case ITEM_OIL:
		case ITEM_PILL:
		case ITEM_SCROLL:
		case ITEM_WAND:
		case ITEM_STAFF: break;
		default: is_magic = FALSE; break;
	}
	
	expend_mana(ch, skill_table[gsn_discernmagic].min_mana);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_discernmagic].beats));
	
	if (number_percent() >= get_skill(ch, gsn_discernmagic))
	{
		send_to_char("You lost your concentration.\n\r", ch);
		check_improve(ch, NULL, gsn_discernmagic, FALSE, 1);
		return;
	}

	act("$n studies $p intently for a moment.", ch, obj, NULL, TO_ROOM);
	sprintf(buf, "You study %s intently, carefully discerning its properties.\n\n\r", obj->short_descr);
	send_to_char(buf, ch);
	
	sprintf(buf, "Object: %s is of type %s\n\r", obj->short_descr, item_name(obj->item_type));
	send_to_char(buf, ch);
	
	if (is_magic)
	{
	  if (obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND)
	  {
		if (obj->value[3] > 0)
		{
			sprintf(buf, "It casts %s at level %d.\n", skill_table[obj->value[3]].name, obj->value[0]);
			send_to_char(buf, ch);
			sprintf(buf, "It seems capable of storing %d charges, and %d charges remain.\n\n", 
				obj->value[1], obj->value[2]);
			send_to_char(buf, ch);
		}
		else send_to_char("It has no magic imbued within it.\n", ch);
	  }
	  else
	  {
		sprintf(buf, "It casts the following spell(s) at level %d:\n", obj->value[0]);
		send_to_char(buf, ch);
		masked_gsn = -1;
		if ((poisoned = get_obj_affect(obj, gsn_subtlepoison)) != NULL)
			masked_gsn = poisoned->modifier;
		for (i = 1; i < 5; i++)
		{
			if (obj->value[i] > 0)
			{
										
				sprintf(buf, "  %s\n", masked_gsn > 0 ? 
					skill_table[masked_gsn].name : skill_table[obj->value[i]].name);
				send_to_char(buf, ch);
			}
		}
		send_to_char("\n\r", ch);
	  }
	}
	sprintf(buf, "It is composed of %s, and is level %d\n\r", material_table[obj->material].name, obj->level);
	send_to_char(buf, ch);
	
	check_improve(ch, NULL, gsn_discernmagic, TRUE, 1);
	
	return;
}

void do_delayreaction( CHAR_DATA * ch, char * argument)
{
	char arg[MAX_STRING_LENGTH];
	int wait, i;
	
	if (get_skill(ch, gsn_delayreaction) == 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}
	
	if (argument[0] == '\0')
	{
		send_to_char("How long do you wish to delay the reaction?\n\r", ch);
		return;
	}
	argument = one_argument(argument, arg);
	if ((wait = atoi(arg)) <= 0)
	{
		send_to_char("Delay time must be greater than zero.\n\r", ch);
		return;
	}
	if (wait > 5)
	{
		send_to_char("You cannot delay the reaction longer than five hours.\n\r", ch);
		return;
	}
	if (argument[0] == '\0')
	{
		send_to_char("Delay what type of reaction?\n\r", ch);
		return;
	}
	if (ch->mana < 2 * skill_table[gsn_delayreaction].min_mana)
	{
		send_to_char("You don't have the energy to start a delayed process right now.\n\r", ch);
		return;
	}
	argument = one_argument(argument, arg);
	for (i = 0; i < MAX_ALCHEMICAL_SKILL_TYPES; i++)
	{
		if (!str_cmp(arg, alchemy_table[i].action))
		{
			do_alchemy(ch, argument, i, wait, 0);
			expend_mana(ch, skill_table[gsn_delayreaction].min_mana);
			WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_delayreaction].beats));
			return;
		}
	}

	send_to_char("Invalid reaction type.\n\r", ch);
	return;
}

void do_animategolem( CHAR_DATA * ch, char * argument)
{
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char item_list[MAX_STRING_LENGTH];
	int total_ingredients = 0, i, elem_biased = -1, elem_biased_amount = -1, total_vnums;
	double power = 0;
	double golem_power;
	struct elemental_bias_type elemental_bias, elem_temp;
	struct platonic_bias_type platonic_bias, plat_temp;
	OBJ_DATA * ingredients[MAX_ITEMS_IN_ANY];
        CHAR_DATA *golem;
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
	bool stone_used = FALSE;

	memset(&elemental_bias, 0, sizeof(struct elemental_bias_type));
	memset(&platonic_bias, 0, sizeof(struct platonic_bias_type));
	
	int max_ingredients = MAX_ITEMS_IN_ANY;
	int min_ingredients = MIN_ITEMS_IN_ANY;
	
	if (get_skill(ch, gsn_animategolem) <= 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}
	if (!valid_alchemy_location(ch)) return;
	
	if (is_affected(ch, gsn_animategolem))
	{
		send_to_char("You don't feel you can animate another golem yet.\n\r", ch);
		return;
	}
	if (ch->pet != NULL)
	{
		send_to_char("You already have a loyal follower, and cannot bind more loyalties.\n\r", ch);
		return;
	}		
	if (ch->mana < skill_table[gsn_animategolem].min_mana)
	{
		send_to_char("You are too weary to animate a golem at present.\n\r", ch);
		return;
	}
	if (argument[0] == '\0')
	{
		send_to_char("With what ingredients do you wish to form your golem?\n\r", ch);
		return;
	}
	
	//Load up ingredients, making sure they didn't try to use too many
	while (*argument != '\0')
	{
		argument = one_argument(argument, arg);
		if (total_ingredients >= max_ingredients)
		{
		  	sprintf(buf, "You cannot use more than %d ingredients to create a golem.\n\r", max_ingredients);
		 	send_to_char(buf, ch);
			return;
		}
		if ((ingredients[total_ingredients] = get_obj_carry(ch, arg, ch)) == NULL)
		{
			sprintf(buf, "You are not carrying any %s.\n\r", arg);
			send_to_char(buf, ch);
			return;
		}
		if (obj_is_affected(ingredients[total_ingredients], gsn_alchemicstone))
		{
			stone_used = TRUE;
		}
		total_ingredients++;
	}

	//Make sure they have enough ingredients
	if (total_ingredients - (stone_used ? 1 : 0) < min_ingredients)
	{
		sprintf(buf, "You must use at least %d ingredients to create a golem.\n\r", min_ingredients);
		send_to_char(buf, ch);
		return;
	}

	//Make sure they don't use the same item twice.
	//Note that two items of the same type are allowed -- this is only
	//  checking that they don't just list the exact same one twice.
	//Also returns true if any item is no_drop
	if (duplicate_ingredients(ch, ingredients, total_ingredients))
		return;

	//Determine elemental/platonic bias of ingredients
	//Also prepares the random number seed
	for (i = 0; i < total_ingredients; i++)
	{
		power += URANGE(0, evaluate_item(ingredients[i], &elem_temp, &platonic_bias, &total_vnums), 10);
		sum_elements(&elemental_bias, &elem_temp);
		sum_platonics(&platonic_bias, &plat_temp);
	}
	
	power /= (total_ingredients - (stone_used ? 1 : 0));
	power += (ch->in_room->room_flags & ROOM_LABORATORY ? 1 : 0);
	
	//Determine winning biases
	//Also look for reinforcement power points
        elem_biased = strongest_element(&elemental_bias, &elem_biased_amount);

  	power += URANGE(0, (elem_biased_amount / total_ingredients) - 8, 2);
   	power *= ((double)(get_skill(ch, gsn_animategolem)) / 100);
	if (ch->level < 40) power *= ((double)(get_skill(ch, gsn_animategolem)) / 100); //hit 'em up for it again
	power = UMIN(power, stone_used ? 15 : 7);
	expend_mana(ch, skill_table[gsn_animategolem].min_mana);

	//Remove the used items, store their names
	item_list[0] = '\0';
	for (i = 0; i < total_ingredients; i++)
	{
		strcat(item_list, ingredients[i]->short_descr);
		if (i <= total_ingredients - 2) strcat(item_list, ", ");
		if (i == total_ingredients - 2) strcat(item_list, "and ");
		if (!obj_is_affected(ingredients[i], gsn_alchemicstone))
			extract_obj(ingredients[i]);
        }

	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_animategolem].beats));
	
	if (number_percent() >= get_skill(ch, gsn_animategolem) || power < 3)
	{
		act("$n attempts to shape some ingredients into a golem, but fails.", ch, NULL, NULL, TO_ROOM);
		sprintf(buf, "You blend %s together, but fail to animate a golem.", item_list);
		act(buf, ch, NULL, NULL, TO_CHAR);
		check_improve(ch, NULL, gsn_animategolem, FALSE, 1);
		return;
	}
	
	check_improve(ch, NULL, gsn_animategolem, TRUE, 1);

	golem = create_mobile(get_mob_index(VNUM_GOLEM - 1 + elem_biased));
	
	golem_power = number_fuzzy((ladder_sum(UMIN(10, (int)power)) + ch->level) / 2);
	golem->level = ch->level;
	golem->damroll += static_cast<int>(golem_power / 2);
	golem->hitroll += static_cast<int> (golem_power);
	golem->damage[0] = static_cast<int> (golem_power / 2);
	golem->damage[1] = 4;
	golem->damage[2] += static_cast<int> (golem_power / 4);
	golem->max_hit  += static_cast<int> (golem_power * 80);
	golem->hit      = golem->max_hit;
	char_to_room(golem, ch->in_room);
	ch->pet = golem;
	golem->master = ch;
	golem->leader = ch;
		
	act("$n blends some alchemical ingredients together, and $N animates!", ch, NULL, golem, TO_ROOM);
	sprintf(buf, "You blend %s together, animating %s!\n\r", item_list, golem->short_descr);
	send_to_char(buf, ch);
													 
	af.where     = TO_AFFECTS;
	af.type      = gsn_animategolem;
	af.level     = ch->level;
	af.duration  = 20;
	af.location  = 0;
	af.modifier  = 0;
	af.bitvector = 0;
	affect_to_char(ch, &af);		    
	
	return;
}

void do_distill( CHAR_DATA * ch, char * argument)
{
	do_alchemy_skill(ch, argument, gsn_distill, 0);
	return;
}

void do_pulverize( CHAR_DATA * ch, char * argument)
{
	do_alchemy_skill(ch, argument, gsn_pulverize, 1);
	return;
}

void do_reforge( CHAR_DATA * ch, char * argument)
{
	do_alchemy_skill(ch, argument, gsn_reforge, 2);
	return;
}

void do_boil( CHAR_DATA * ch, char * argument)
{
	do_alchemy_skill(ch, argument, gsn_boil, 3);
	return;
}

void do_dissolve( CHAR_DATA * ch, char * argument)
{
	do_alchemy_skill(ch, argument, gsn_dissolve, 4);
	return;
}

void do_dilute( CHAR_DATA * ch, char * argument)
{
	do_alchemy_skill(ch, argument, gsn_dilute, 5);
	return;
}

void do_alchemy_skill(CHAR_DATA * ch, char * argument, int skill_gsn, int local_gsn)
{
	char arg[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
	OBJ_DATA * obj;
	int i;
	
	if (!alchemy_sanity_checks(ch, argument, skill_gsn)) return;

	argument = one_argument(argument, arg);
	if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
	{
		sprintf(buf, "You aren't carrying any %s.\n\r", arg);
		send_to_char(buf, ch);
		return;
	}

	if (obj->pIndexData->vnum == VNUM_ALCHEMY_PRODUCT)
	{
		sprintf(buf, "You have already prepared %s.\n\r", obj->short_descr);
		send_to_char(buf, ch);
		return;
	}

	for (i = gsn_distill; i <= gsn_dilute; i++)
	{
		if (obj_is_affected(obj, i))
		{
			sprintf(buf, "You have already prepared %s.\n\r", obj->short_descr);
			send_to_char(buf, ch);
			return;
		}
	}

	if (alchemy_skills_table[local_gsn].metal_only && !material_table[obj->material].metal)
	{
		sprintf(buf, "You can only %s metal objects.\n\r", skill_table[skill_gsn].name);
		send_to_char(buf, ch);
		return;
	}

	if (alchemy_skills_table[local_gsn].solid_only && obj->item_type == ITEM_DRINK_CON)
	{
		sprintf(buf, "You cannot %s liquids.\n\r", skill_table[skill_gsn].name);
		send_to_char(buf, ch);
		return;
	}

	if (alchemy_skills_table[local_gsn].liquid_only)
	{
		if (obj->item_type != ITEM_DRINK_CON)
		{
			sprintf(buf, "You can only %s liquids.\n\r", skill_table[skill_gsn].name);
			send_to_char(buf, ch);
			return;
		}
		if (obj->value[1] <= 0)
		{
			sprintf(buf, "But %s is empty!\n\r", obj->short_descr);
			send_to_char(buf, ch);
			return;
		}
	}

	if (number_percent() < get_skill(ch, skill_gsn))
	{
		if (obj->item_type != ITEM_DRINK_CON)
		{
			sprintf(buf, "%s %s", alchemy_skills_table[local_gsn].short_descr, obj->short_descr);
			free_string(obj->short_descr);
			obj->short_descr = str_dup(buf);
			sprintf(buf, "%s %s", obj->short_descr, alchemy_skills_table[local_gsn].extra_name);
            setName(*obj, buf);
			sprintf(buf, "%s lies here.", obj->short_descr);
			buf[0] = UPPER(buf[0]);
			free_string(obj->description);
			obj->description = str_dup(buf);
			obj->wear_flags = ITEM_TAKE;
		}
		
		af.where        = TO_OBJECT;
		af.type         = skill_gsn;
		af.level        = ch->level;
		af.duration     = -1;
		af.location     = APPLY_NONE;
		af.modifier     = 0;
		af.bitvector    = 0;
		affect_to_obj(obj, &af);

		sprintf(buf, "%s carefully %ss %s for future alchemical work.", 
			ch->name, skill_table[skill_gsn].name, obj->short_descr);
		act(buf, ch, NULL, NULL, TO_ROOM);
		sprintf(buf, "You carefully %s %s for future alchemical work.\n\r",
			skill_table[skill_gsn].name, obj->short_descr);
		send_to_char(buf, ch);
		check_improve(ch, NULL, skill_gsn, TRUE, 1);
	}
	else
	{
		sprintf(buf, "You attempt to %s %s, but your efforts only destroy it.\n\r", 
			skill_table[skill_gsn].name, obj->short_descr);
		send_to_char(buf, ch);
		check_improve(ch, NULL, skill_gsn, FALSE, 1);
		extract_obj(obj);
	}
	expend_mana(ch, skill_table[skill_gsn].min_mana);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[skill_gsn].beats));
	return;
}


bool alchemy_sanity_checks(CHAR_DATA * ch, char * argument, int skill_gsn)
{
	char buf[MAX_STRING_LENGTH];
	//Make sure they have the skill
	if (get_skill(ch, skill_gsn) == 0)
	{
		send_to_char("Huh?\n\r", ch);
		return FALSE;
	}
	if (!valid_alchemy_location(ch)) return FALSE;
	
	//Make sure they have enough mana
	if (ch->mana < skill_table[skill_gsn].min_mana)
	{
		sprintf(buf, "You are too tired to %s anything right now.\n\r", skill_table[skill_gsn].name);
		send_to_char(buf, ch);
		return FALSE;
	}

	//Make sure they have arguments
	if (argument[0] == '\0')
	{
		sprintf(buf, "What do you wish to %s?\n\r", skill_table[skill_gsn].name);
		send_to_char(buf, ch);
		return FALSE;
	}

	return TRUE;
}

/*
 * This is for mixing oils.
 * The argument list should contain the desired ingredients.
 */
void do_mix( CHAR_DATA *ch, char *argument)
{
	do_alchemy( ch, argument, 0, 0, 0);
}

void do_brew( CHAR_DATA *ch, char *argument)
{
	do_alchemy( ch, argument, 1, 0, 0);
}

void do_make( CHAR_DATA *ch, char *argument)
{
	do_alchemy( ch, argument, 2, 0, 0);
}

//Returns the most powerful bias of the struct with bias at least amount
int strongest_element(struct elemental_bias_type * elemental_bias, int * elem_biased_amount)
{
	int elem_biased = BIAS_ANCIENT_INDEX;
	if (elemental_bias->bias_void > *elem_biased_amount)
		{elem_biased = BIAS_VOID_INDEX; *elem_biased_amount = elemental_bias->bias_void;}
	if (elemental_bias->bias_spirit > *elem_biased_amount)
		{elem_biased = BIAS_SPIRIT_INDEX; *elem_biased_amount = elemental_bias->bias_spirit;}
	if (elemental_bias->bias_water > *elem_biased_amount)
		{elem_biased = BIAS_WATER_INDEX; *elem_biased_amount = elemental_bias->bias_water;}
	if (elemental_bias->bias_fire > *elem_biased_amount)
		{elem_biased = BIAS_FIRE_INDEX; *elem_biased_amount = elemental_bias->bias_fire;}
	if (elemental_bias->bias_earth > *elem_biased_amount)
		{elem_biased = BIAS_EARTH_INDEX; *elem_biased_amount = elemental_bias->bias_earth;}
	if (elemental_bias->bias_air > *elem_biased_amount)
		{elem_biased = BIAS_AIR_INDEX; *elem_biased_amount = elemental_bias->bias_air;}
	if (elemental_bias->bias_nature > *elem_biased_amount)
		{elem_biased = BIAS_NATURE_INDEX; *elem_biased_amount = elemental_bias->bias_nature;}
	if (elemental_bias->bias_ancient > *elem_biased_amount)
		{elem_biased = BIAS_ANCIENT_INDEX; *elem_biased_amount = elemental_bias->bias_ancient;}
	if (elemental_bias->bias_transmogrify > *elem_biased_amount)
		{elem_biased = BIAS_TRANSMOGRIFY_INDEX; *elem_biased_amount = elemental_bias->bias_transmogrify;}	
	return elem_biased;
}

int close_elements(struct elemental_bias_type * elemental_bias, int lead, double perc)
{
	int elems = 0;
	if (elemental_bias->bias_void >= UMAX(lead * perc, 1)) elems = elems | make_bin(BIAS_VOID_INDEX);
	if (elemental_bias->bias_spirit >= UMAX(lead * perc, 1)) elems = elems | make_bin(BIAS_SPIRIT_INDEX);
	if (elemental_bias->bias_water >= UMAX(lead * perc, 1)) elems = elems | make_bin(BIAS_WATER_INDEX);
	if (elemental_bias->bias_air >= UMAX(lead * perc, 1)) elems = elems | make_bin(BIAS_AIR_INDEX);
	if (elemental_bias->bias_earth >= UMAX(lead * perc, 1)) elems = elems | make_bin(BIAS_EARTH_INDEX);
	if (elemental_bias->bias_fire >= UMAX(lead * perc, 1)) elems = elems | make_bin(BIAS_FIRE_INDEX);
	if (elemental_bias->bias_nature >= UMAX(lead * perc, 1)) elems = elems | make_bin(BIAS_NATURE_INDEX);
	if (elemental_bias->bias_ancient >= UMAX(lead * perc, 1)) elems = elems | make_bin(BIAS_ANCIENT_INDEX);
	if (elemental_bias->bias_transmogrify >= UMAX(lead * perc, 1)) elems = elems | make_bin(BIAS_TRANSMOGRIFY_INDEX);
	return elems;
}

int strongest_platonic(struct platonic_bias_type * platonic_bias, int * plat_biased_amount)
{
	int plat_biased = BIAS_DAMAGE_INDEX;
	if (platonic_bias->bias_damage > *plat_biased_amount)
		{plat_biased = BIAS_DAMAGE_INDEX; *plat_biased_amount = platonic_bias->bias_damage;}
	if (platonic_bias->bias_malediction > *plat_biased_amount)
		{plat_biased = BIAS_MALEDICTION_INDEX; *plat_biased_amount = platonic_bias->bias_malediction;}
	if (platonic_bias->bias_protection > *plat_biased_amount)
		{plat_biased = BIAS_PROTECTION_INDEX; *plat_biased_amount = platonic_bias->bias_protection;}
	if (platonic_bias->bias_order > *plat_biased_amount)
		{plat_biased = BIAS_ORDER_INDEX; *plat_biased_amount = platonic_bias->bias_order;}
	if (platonic_bias->bias_enhancement > *plat_biased_amount)
		{plat_biased = BIAS_ENHANCEMENT_INDEX; *plat_biased_amount = platonic_bias->bias_enhancement;}
	if (platonic_bias->bias_chaos > *plat_biased_amount)
		{plat_biased = BIAS_CHAOS_INDEX; *plat_biased_amount = platonic_bias->bias_chaos;}
	if (platonic_bias->bias_obscurement > *plat_biased_amount)
		{plat_biased = BIAS_OBSCUREMENT_INDEX; *plat_biased_amount = platonic_bias->bias_obscurement;}
	if (platonic_bias->bias_healing > *plat_biased_amount)
		{plat_biased = BIAS_HEALING_INDEX; *plat_biased_amount = platonic_bias->bias_healing;}
	if (platonic_bias->bias_knowledge > *plat_biased_amount)
		{plat_biased = BIAS_KNOWLEDGE_INDEX; *plat_biased_amount = platonic_bias->bias_knowledge;}
	if (platonic_bias->bias_planar > *plat_biased_amount)
		{plat_biased = BIAS_PLANAR_INDEX; *plat_biased_amount = platonic_bias->bias_planar;}
	if (platonic_bias->bias_alteration > *plat_biased_amount)
		{plat_biased = BIAS_ALTERATION_INDEX; *plat_biased_amount = platonic_bias->bias_alteration;}
	return plat_biased;
}

int close_platonics(struct platonic_bias_type * platonic_bias, int lead, double perc)
{
	int plats = 0;
	if (platonic_bias->bias_damage >= UMAX(lead * perc, 1)) plats = plats | make_bin(BIAS_DAMAGE_INDEX);
	if (platonic_bias->bias_malediction >= UMAX(lead * perc, 1)) plats = plats | make_bin(BIAS_MALEDICTION_INDEX);
	if (platonic_bias->bias_protection >= UMAX(lead * perc, 1)) plats = plats | make_bin(BIAS_PROTECTION_INDEX);
	if (platonic_bias->bias_order >= UMAX(lead * perc, 1)) plats = plats | make_bin(BIAS_ORDER_INDEX);
	if (platonic_bias->bias_enhancement >= UMAX(lead * perc, 1)) plats = plats | make_bin(BIAS_ENHANCEMENT_INDEX);
	if (platonic_bias->bias_chaos >= UMAX(lead * perc, 1)) plats = plats | make_bin(BIAS_CHAOS_INDEX);
	if (platonic_bias->bias_obscurement >= UMAX(lead * perc, 1)) plats = plats | make_bin(BIAS_OBSCUREMENT_INDEX);
	if (platonic_bias->bias_healing >= UMAX(lead * perc, 1)) plats = plats | make_bin(BIAS_HEALING_INDEX);
	if (platonic_bias->bias_knowledge >= UMAX(lead * perc, 1)) plats = plats | make_bin(BIAS_KNOWLEDGE_INDEX);
	if (platonic_bias->bias_planar >= UMAX(lead * perc, 1)) plats = plats | make_bin(BIAS_PLANAR_INDEX);
	if (platonic_bias->bias_alteration >= UMAX(lead * perc, 1)) plats = plats | make_bin(BIAS_ALTERATION_INDEX);
	if (platonic_bias->bias_protection >= UMAX(lead * perc, 1)) plats = plats | make_bin(BIAS_PROTECTION_INDEX);
	return plats;
}

/*
 * This is for making all of the alchemical concoctions.
 * The argument list should contain the desired ingredients.
 */
void do_alchemy( CHAR_DATA *ch, char *argument, int alc_sn, int delay, double added_power)	
{
    if (argument == NULL)
    {
        send_to_char("There was a problem completing your alchemical work. Please notify the Immortals of any details that might help resolve it.\n", ch);
        std::ostringstream mess;
        mess << "NULL argument for alchemical work; character is " << ch->name;
        bug(mess.str().c_str(), 0);
        return;
    }

	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char temp[MAX_STRING_LENGTH];
	char item_list[MAX_STRING_LENGTH];
	char wiz_list[MAX_STRING_LENGTH], wiz_start[MAX_STRING_LENGTH];
	char * full_argument(0), * temp_str(0);
	int total_ingredients = 0, i, elem_biased = -1, elem_biased_amount = -1, levelmod = 0;
	int plat_biased = -1, plat_biased_amount = -1, high_elems = 0, high_plats = 0, spell_loops = 0;
	float random1, random2;
	double power = 0;
	int stone_value = 0;
	int total_spells = 0, total_vnums = 0, chosen_spell_index = 0, chosen_spell, power_check, spells_found;
	int spell_list[MAX_SKILL][2]; //0: gsn   1: alchemist power
	struct elemental_bias_type elemental_bias, elem_temp;
	struct platonic_bias_type platonic_bias, plat_temp;
	OBJ_DATA * ingredients[MAX_ITEMS_IN_ANY];
	OBJ_DATA * final_product(0);
	OBJ_DATA * phil_stone(0), * alc_stone = NULL;
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
	bool concentrate = FALSE, stone_used = FALSE, poison = FALSE;
	
	wiz_list[0] = '\0';
	sprintf(wiz_start, "%s %s%s", ch->name, alchemy_table[alc_sn].action,
		(!str_cmp(alchemy_table[alc_sn].action, "mix") ? "es" : "s"));
	memset(&elemental_bias, 0, sizeof(struct elemental_bias_type));
	memset(&platonic_bias, 0, sizeof(struct platonic_bias_type));
	
	int max_ingredients = UMIN(MAX_ITEMS_IN_ANY, alchemy_table[alc_sn].max_items);
	int min_ingredients = UMAX(MIN_ITEMS_IN_ANY, alchemy_table[alc_sn].min_items);
	
	if (added_power >= 0 && !basic_alchemy_sanity_checks(ch, argument, alc_sn)) return;

	full_argument = argument;
	
	//Check for concentrate
	argument = one_argument(argument, arg);
	if (!str_cmp(arg, "concentrate") && get_skill(ch, gsn_concentrate) > 0) 
	{
		if (ch->mana < skill_table[gsn_concentrate].min_mana)
			send_to_char("You don't have the energy to concentrate your solution, so you forgo it.\n\r", ch);
		else if (!(ch->in_room->room_flags & ROOM_LABORATORY))
			send_to_char("Since you are not in a lab, you forgo concentrating your solution.\n\r", ch);
		else if (!strcmp(alchemy_table[alc_sn].product, "oil"))
			send_to_char("Since it is impossible to concentrate oils, you forgo that process.\n\r", ch);
		else
			concentrate = TRUE;
	}
	else 
		argument = full_argument;
	
	//Load up ingredients, making sure they didn't try to use too many
	while (*argument != '\0')
	{
		argument = one_argument(argument, arg);
		if (total_ingredients >= max_ingredients)
		{
		  	sprintf(buf, "You cannot incorporate more than %d ingredients into your %s.\n\r", 
				max_ingredients, alchemy_table[alc_sn].product);
		 	send_to_char(buf, ch);
			return;
		}
		if ((ingredients[total_ingredients] = get_obj_carry(ch, arg, ch)) == NULL)
		{
			sprintf(buf, "You are not carrying any %s.\n\r", arg);
			send_to_char(buf, ch);
			return;
		}
		sprintf(buf, "%s (%d), ", ingredients[total_ingredients]->short_descr, 
				ingredients[total_ingredients]->pIndexData->vnum);
		strcat(wiz_list, buf);
		if (obj_is_affected(ingredients[total_ingredients], gsn_alchemicstone)) 
		{
			if (stone_used)
			{
				send_to_char("You cannot use more than one alchemic stone in your concoction.\n\r", ch);
				return;
			}
			stone_used = TRUE;
			alc_stone = ingredients[total_ingredients];
			stone_value = ingredients[total_ingredients]->objvalue[0];
		}
		if (ingredients[total_ingredients]->item_type == ITEM_DRINK_CON)
		{
			if (!strcmp(alchemy_table[alc_sn].product, "pill"))
			{
				send_to_char("You cannot incorporate a liquid into a pill, it would dissolve!\n\r", ch);
				return;
			}
			if (ingredients[total_ingredients]->value[1] <= 0)
			{
				sprintf(buf, "%s is empty.\n\r", ingredients[total_ingredients]->short_descr);
				buf[0] = UPPER(buf[0]);
				send_to_char(buf, ch);
				return;
			}
		}
		if (ingredients[total_ingredients]->item_type == ITEM_CONTAINER 
		&& ingredients[total_ingredients]->contains != NULL)
		{
			sprintf(buf, "You should take everything out of %s before using it.\n\r", 
				ingredients[total_ingredients]->short_descr);
			send_to_char(buf, ch);
			return;
		}
		total_ingredients++;
	}

	//Make sure they have enough ingredients
	if (total_ingredients - (stone_used ? 1 : 0) < min_ingredients)
	{
		sprintf(buf, "You must use at least %d ingredients to %s your %s.\n\r", 
			min_ingredients, alchemy_table[alc_sn].action, alchemy_table[alc_sn].product);
		send_to_char(buf, ch);
		return;
	}

	//Make sure they don't use the same item twice.
	//Note that two items of the same type are allowed -- this is only
	//  checking that they don't just list the exact same one twice.
	//Also returns true if any item is no_drop
	if (duplicate_ingredients(ch, ingredients, total_ingredients))
		return;

	//Support for delay
	if (delay > 0)
	{
		af.where     = TO_AFFECTS;
		af.type      = gsn_delayreaction;
		af.level     = (int)(delay/2 + 1);
		af.duration  = delay;
		af.modifier  = alc_sn;
		af.location  = APPLY_NONE;
		af.bitvector = 0;
		af.point = (void *) str_dup(full_argument);
		affect_to_char(ch, &af);
						
		sprintf(buf, "You set up your ingredients to delay for %d hour%s.\n\r", delay, (delay == 1 ? "" : "s"));
		send_to_char(buf, ch);
		act("$n prepares some alchemical ingredients for a delayed reaction.", ch, NULL, NULL, TO_ROOM);
		return;
	}
	
	//Determine elemental/platonic bias of ingredients
	//Also prepares the random number seed
	for (i = 0; i < total_ingredients; i++)
	{
		power += URANGE(0, evaluate_item(ingredients[i], &elem_temp, &plat_temp, &total_vnums), 10);
		sum_elements(&elemental_bias, &elem_temp);
		sum_platonics(&platonic_bias, &plat_temp);
	}	
	
	power /= (total_ingredients - (stone_used ? 1 : 0));
	power += (ch->in_room->room_flags & ROOM_LABORATORY ? 1 : 0);
	power += (!strcmp(alchemy_table[alc_sn].product, "potion") ? 0.5 : 0);

	elem_biased = strongest_element(&elemental_bias, &elem_biased_amount);
	plat_biased = strongest_platonic(&platonic_bias, &plat_biased_amount);
	
	total_spells = 0;
	spell_loops = 0;
	while (total_spells < 2 && spell_loops < 7)
	{
		high_elems = close_elements(&elemental_bias, elem_biased_amount, 0.80);
		high_plats = close_platonics(&platonic_bias, plat_biased_amount, 0.80 - (.1 * (double)spell_loops));
	
		if (spell_loops <= 0)
		{
			power += URANGE(0, (elem_biased_amount / total_ingredients) - 8, 2);
			power += URANGE(0, (plat_biased_amount / total_ingredients) - 8, 2);
			power *= ((double)(get_skill(ch, alchemy_table[alc_sn].gsn)) / 100);
			if (ch->level < 40) power *= ((double)(get_skill(ch, alchemy_table[alc_sn].gsn)) / 100);
			power += added_power;
			power = UMIN(power, stone_used ? 15 : 7);
		}
	
		//Run through skills/spells to find the ones that match
		total_spells = 0;
		if (high_elems & make_bin(BIAS_VOID_INDEX))
		{
			total_spells = determine_spell_space(spell_list, &void_skills[0], high_plats, 
					power, alc_sn, total_spells);
		}
		if (high_elems & make_bin(BIAS_SPIRIT_INDEX))
		{
			total_spells = determine_spell_space(spell_list, &spirit_skills[0], high_plats, 
					power, alc_sn, total_spells);
		}
		if (high_elems & make_bin(BIAS_WATER_INDEX))
		{
			total_spells = determine_spell_space(spell_list, &water_skills[0], high_plats, 
					power, alc_sn, total_spells);
		}
		if (high_elems & make_bin(BIAS_FIRE_INDEX))
		{
			total_spells = determine_spell_space(spell_list, &fire_skills[0], high_plats, 
					power, alc_sn, total_spells);
		}
		if (high_elems & make_bin(BIAS_EARTH_INDEX))
		{
			total_spells = determine_spell_space(spell_list, &earth_skills[0], high_plats, 
					power, alc_sn, total_spells);
		}
		if (high_elems & make_bin(BIAS_AIR_INDEX))
		{
			total_spells = determine_spell_space(spell_list, &air_skills[0], high_plats, 
					power, alc_sn, total_spells);
		}
		if (high_elems & make_bin(BIAS_NATURE_INDEX))
		{
			total_spells = determine_spell_space(spell_list, &nature_skills[0], high_plats, 
					power, alc_sn, total_spells);
		}
		if (high_elems & make_bin(BIAS_ANCIENT_INDEX))
		{
			total_spells = determine_spell_space(spell_list, &ancient_skills[0], high_plats, 
				  power, alc_sn, total_spells);
		}
		if (high_elems & make_bin(BIAS_TRANSMOGRIFY_INDEX))
		{
			total_spells = determine_spell_space(spell_list, &transmogrify_skills[0], high_plats, 
				  power, alc_sn, total_spells);
		}
		spell_loops++;
	} 
	
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[alchemy_table[alc_sn].gsn].beats));
	expend_mana(ch, skill_table[alchemy_table[alc_sn].gsn].min_mana);

        if (total_spells <= 0)
	{
		if (number_percent() < get_skill(ch, alchemy_table[alc_sn].gsn) / 2)
		{
			high_plats = 4095;
			power = 2;
			total_spells = determine_spell_space(spell_list, &void_skills[0], high_plats, 
					power, alc_sn, total_spells);
			total_spells = determine_spell_space(spell_list, &spirit_skills[0], high_plats,
					power, alc_sn, total_spells);
			total_spells = determine_spell_space(spell_list, &fire_skills[0], high_plats,
					power, alc_sn, total_spells);
			total_spells = determine_spell_space(spell_list, &water_skills[0], high_plats,
					power, alc_sn, total_spells);
			total_spells = determine_spell_space(spell_list, &air_skills[0], high_plats,
					power, alc_sn, total_spells);
			total_spells = determine_spell_space(spell_list, &earth_skills[0], high_plats,
					power, alc_sn, total_spells);
			sprintf(buf, "%s %sbut gets nothing. Random weak spell chosen.", wiz_start, wiz_list);
			wiznet(buf, ch, NULL, WIZ_ALCHEMY_BRIEF, 0, 0);
			wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
		}
		else if (number_percent() < get_curr_stat(ch, STAT_WIS) + get_skill(ch, alchemy_table[alc_sn].gsn)/4
								+ get_skill(ch, gsn_caution)/5)
		{
			act("You realize in time to save your ingredients that they aren't producing anything, and hastily cool your reaction.", ch, NULL, NULL, TO_CHAR);
			act("$n hastily cools $s reaction.", ch, NULL, NULL, TO_ROOM);
			sprintf(buf, "%s %sbut gets nothing. Saves ingredients.", wiz_start, wiz_list);
			wiznet(buf, ch, NULL, WIZ_ALCHEMY_BRIEF, 0, 0);
			wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
			return;
		}
	}
	
	
	//Remove the used items, store their names
	item_list[0] = '\0';
	for (i = 0; i < total_ingredients; i++)
	{
		sprintf(buf, "the contents of %s", ingredients[i]->short_descr);
		strcat(item_list, (ingredients[i]->item_type == ITEM_DRINK_CON ? buf : ingredients[i]->short_descr));
		if (i <= total_ingredients - 2) strcat(item_list, ", ");
		if (i == total_ingredients - 2) strcat(item_list, "and ");
		if (!obj_is_affected(ingredients[i], gsn_alchemicstone))
		{
			if (ingredients[i]->item_type == ITEM_DRINK_CON) ingredients[i]->value[1] = 0;
			else extract_obj(ingredients[i]);
		}
	}

	if (added_power < 0)
	{
		sprintf(buf, "%s %sbut it explodes.", wiz_start, wiz_list);
		wiznet(buf, ch, NULL, WIZ_ALCHEMY_BRIEF, 0, 0);
		wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
		explosive_alchemy(ch, static_cast<int> (power));
		check_improve(ch, NULL, alchemy_table[alc_sn].gsn, FALSE, 1);
		return;
	}
	
	if (total_spells <= 0 || number_percent() >= get_skill(ch, alchemy_table[alc_sn].gsn)
							- (concentrate ? 30 : 1)
							- (!strcmp(alchemy_table[alc_sn].product, "potion") ? 5 : 0))
	{
		//Consider different failure types
		switch (total_spells <= 0 ? 0 : number_range(0, 3))
		{
			//Subtle poison
			case 1: if (!(!strcmp(alchemy_table[alc_sn].product, "oil")))
				{
					poison = TRUE;
					sprintf(buf, "%s %sbut produces a subtle poison.", wiz_start, wiz_list);
					wiznet(buf, ch, NULL, WIZ_ALCHEMY_BRIEF, 0, 0);
					wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
					break;
				} //if it's an oil, go on to explosive_alchemy
			
			//Explosive!
			case 2: sprintf(buf, "%s %sbut it explodes.", wiz_start, wiz_list);
			        wiznet(buf, ch, NULL, WIZ_ALCHEMY_BRIEF, 0, 0);
				wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
				explosive_alchemy(ch, static_cast<int>(power)); 
				check_improve(ch, NULL, alchemy_table[alc_sn].gsn, FALSE, 1);
				return;			 
				break;
			
			//Mild
			case 3: levelmod = ch->level / 2;
				sprintf(buf, "%s %sbut produces a half-power potion.", wiz_start, wiz_list);
				wiznet(buf, ch, NULL, WIZ_ALCHEMY_BRIEF, 0, 0);
				wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
				break;
			
			//Null result
			case 0:
			default:
			if (total_spells <= 0)
			{
			 sprintf(buf, "%s %sbut produces a null spell space.", wiz_start, wiz_list);
			 wiznet(buf, ch, NULL, WIZ_ALCHEMY_BRIEF, 0, 0);
			 wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
			 sprintf(buf, "You render down %s, but these ingredients fail to produce your desired %s.\n\r",
					item_list, alchemy_table[alc_sn].product);
			}
			else
			{
			 sprintf(buf, "%s %sbut fails the skill check.", wiz_start, wiz_list);
			 wiznet(buf, ch, NULL, WIZ_ALCHEMY_BRIEF, 0, 0);
			 wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
			 sprintf(buf, "You render down %s, but fumble a critical step and fail to produce anything.\n\r",
			  item_list);
			}
			 send_to_char(buf, ch);
			 act("$n mixes together some alchemical ingredients, but nothing results.", ch, NULL, NULL, TO_ROOM);
			 check_improve(ch, NULL, alchemy_table[alc_sn].gsn, FALSE, 1);
			 return;
			break;
		}
	}
	check_improve(ch, NULL, alchemy_table[alc_sn].gsn, TRUE, 1);

	temp_str = read_elemental_biases(&elemental_bias);
	sprintf(buf, temp_str);
	free_string(temp_str);
	wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
	temp_str = read_platonic_biases(&platonic_bias);
	sprintf(buf, temp_str);
	free_string(temp_str);
	wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
	sprintf(buf, "Spell space (power %d):", (int)power);
	wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
	for (i = 0; i < total_spells; i++)
	{
		int skill_vnum(spell_list[i][0]);
		if (skill_vnum > 0 && skill_vnum < MAX_SKILL)
		{
			sprintf(buf, "%s %d", skill_table[skill_vnum].name, spell_list[i][1]);
			wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
		}
	}

	//Base random seed off vnum total to ensure consistency of results
	srand((unsigned)((total_vnums+1337) * 4 + (total_spells * total_spells * 3) + stone_value));
	
	power_check = static_cast<int> (power) - 2;
	spells_found = 0;
	while (power_check > 0 && spells_found < 2)
	{
		power_check--;
		for (i = 0; i < total_spells; i++) spells_found += (spell_list[i][1] >= power_check ? 1 : 0);
	}

	do{
		chosen_spell_index = (((int)rand()) % total_spells);
	}while(spell_list[chosen_spell_index][1] < power_check);
	
	//Check for rare potions
	srand(time(NULL));
	if (power >= 10 && number_bits(11) == 0 && stone_used)
	{
		act("$n's concoction shimmers, and $s alchemist stone suddenly takes on a strange sheen!", 
				ch, NULL, NULL, TO_ROOM);
		act("Your concoction shimmers, and your alchemist stone suddenly takes on a strange sheen!",
				ch, NULL, NULL, TO_ROOM);
		extract_obj(alc_stone);
		phil_stone = create_object(get_obj_index(VNUM_ALCHEMY_PRODUCT+1), ch->level);
		obj_to_char(phil_stone, ch);
	}
	else if (power >= 10 && number_bits(8) == 0)
	{
		total_spells = 0;
		switch(number_range(0, 3))
		{		
			case 0: spell_list[chosen_spell_index][0] = gsn_youth; break;
			case 1: spell_list[chosen_spell_index][0] = gsn_invulnerability; break;
			case 2: spell_list[chosen_spell_index][0] = gsn_perfection; break;
			case 3: spell_list[chosen_spell_index][0] = gsn_improvement; break;
		}
		spell_list[chosen_spell_index][1] = 10;
	}
	else if (power >= 8 && number_bits(7) == 0)
	{
		total_spells = 0;
		switch(number_range(0, 3))
		{
			case 0: spell_list[chosen_spell_index][0] = gsn_polyglot; break;
			case 1: spell_list[chosen_spell_index][0] = gsn_resistance; break;
			case 2: spell_list[chosen_spell_index][0] = gsn_heroism; break;
			case 3: spell_list[chosen_spell_index][0] = gsn_divinesight; break;
		}
		spell_list[chosen_spell_index][1] = 8;
	}
	
	chosen_spell = spell_list[chosen_spell_index][0];
	
	//Success! Produce the item.
	final_product = create_object(get_obj_index(VNUM_ALCHEMY_PRODUCT), ch->level);
	final_product->level = 1;
	final_product->item_type = alchemy_table[alc_sn].item_type;
	final_product->value[0] = (int) (ch->level+UMIN(power - spell_list[chosen_spell_index][1],10) - levelmod);
	for (i = 2; i < 5; i++) 
		final_product->value[i] = -1;
	final_product->value[1] = chosen_spell;

	sprintf(buf, "%s %sand produces %s.", wiz_start, wiz_list, (chosen_spell >= 0 ? skill_table[chosen_spell].name : "nothing"));
	wiznet(buf, ch, NULL, WIZ_ALCHEMY_BRIEF, 0, 0);
	wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
	
	if (concentrate && !poison && total_spells > 0)
	{
		expend_mana(ch, skill_table[gsn_concentrate].min_mana);
		WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_concentrate].beats));
		if (number_percent() < get_skill(ch, gsn_concentrate) - 15)
		{
			i = 0;
			chosen_spell = -1;
			while (i < 5 && chosen_spell <= 0)
			{
				chosen_spell_index = (((int)rand()) % total_spells);
				chosen_spell = spell_list[chosen_spell_index][0];
				i++;
			}
			final_product->value[2] = chosen_spell;
			sprintf(buf, "Concentrated: %s", (chosen_spell >= 0 ? skill_table[chosen_spell].name : "nothing"));
			wiznet(buf, ch, NULL, WIZ_ALCHEMY_BRIEF, 0, 0);
			wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
			if (number_percent() < get_skill(ch, gsn_concentrate) / 33) //critical success
			{
				i = 0;
				chosen_spell = -1;
				while (i < 5 && chosen_spell <= 0)
				{
					chosen_spell_index = (((int)rand()) % total_spells);
					chosen_spell = spell_list[chosen_spell_index][0];
					i++;
				}
				final_product->value[3] = chosen_spell;
				sprintf(buf, "Super concentrated: %s", (chosen_spell >= 0 ? skill_table[chosen_spell].name : "nothing"));
				wiznet(buf, ch, NULL, WIZ_ALCHEMY_BRIEF, 0, 0);
				wiznet(buf, ch, NULL, WIZ_ALCHEMY_VERBOSE, 0, 0);
			}
			check_improve(ch, NULL, gsn_concentrate, TRUE, 1);
		}
		else if (number_percent() < 10)
		{
			explosive_alchemy(ch, static_cast<int>(power));
			check_improve(ch, NULL, alchemy_table[alc_sn].gsn, FALSE, 1);
			extract_obj(final_product);
			return;
		}
		else
		{
			check_improve(ch, NULL, gsn_concentrate, FALSE, 1);
		}
	}
	if (chosen_spell < 0)
	    return;
	final_product->material = material_lookup("glass");
	final_product->weight = alchemy_table[alc_sn].weight;
	final_product->size = 1;
	SET_BIT(final_product->wear_flags, ITEM_TAKE);
	SET_BIT(final_product->wear_flags, ITEM_HOLD);
	
	//Determines color and any other adjectives
	determine_color(temp, skill_table[final_product->value[1]].name, elem_biased, power);
	random1 = 100 * (float)(rand() / (RAND_MAX + 1.0));
	random2 = MAX_ALCHEMY_ADJECTIVES * (float)(rand() / (RAND_MAX + 1.0));
	if (random1 < 30)
	{
		sprintf(buf, "%s %s %s %s", leading_vowel(alchemy_adjective_table[(int)random2].adjective) ? "an" : "a",
			alchemy_adjective_table[(int)random2].adjective, temp, alchemy_table[alc_sn].product);
	}
	else
	{
		sprintf(buf, "%s %s %s", leading_vowel(temp) ? "an" : "a", temp, alchemy_table[alc_sn].product);
	}

    setName(*final_product, buf);
	free_string(final_product->short_descr);
	final_product->short_descr = str_dup(buf);
	sprintf(buf, "%s lies here.", final_product->short_descr);
	buf[0] = UPPER(buf[0]);
	free_string(final_product->description);
	final_product->description = str_dup(buf);
	
	//Subtle poison case
	if (poison)
	{
	        af.where     = TO_OBJECT;
		af.type      = gsn_subtlepoison;
		af.level     = ch->level;
		af.duration  = -1;
		af.modifier  = final_product->value[1];
		af.location  = APPLY_NONE;
		af.bitvector = 0;
		affect_to_obj(final_product, &af);
		
		switch(number_range(0, 13))
		{
			case 0: final_product->value[1] = gsn_poison; break;
			case 1: final_product->value[1] = gsn_plague; break;
			case 2: final_product->value[1] = gsn_pox; break;
			case 3: final_product->value[1] = gsn_curse; break;
			case 4: final_product->value[1] = gsn_lovepotion; break;
			case 5: final_product->value[1] = gsn_sleep; break;
			case 6: final_product->value[1] = gsn_agony; break;
			case 7: final_product->value[1] = gsn_delusions; break;
			case 8: final_product->value[1] = gsn_drunkenness; break;
			case 9: final_product->value[1] = gsn_susceptibility; break;
			case 10: final_product->value[1] = 
				number_percent() < 5 ? gsn_age :
				number_percent() < 15 ? skill_lookup("change sex") : gsn_poison;
				break;
			case 11: final_product->value[1] = gsn_gills; break;
			case 12: final_product->value[1] = gsn_teleportcurse; break;
			case 13: final_product->value[1] = gsn_aviancurse; break;
			default: final_product->value[1] = gsn_poison; break;
					
		}
	}
	
	obj_to_char(final_product, ch);

	sprintf(buf, "You render down %s, quickly creating %s%s\n\r", 
		item_list, final_product->short_descr, final_product->value[2] > 0 ? ", concentrated!" : "!");
	send_to_char(buf, ch);
	act("$n renders some alchemical ingredients together, quickly creating $p.", ch, final_product, NULL, TO_ROOM);

	if (poison && number_percent() < (get_skill(ch, gsn_caution) / 10 + get_curr_stat(ch, STAT_WIS) - 15))
		send_to_char("You suspect something may not be quite right with your concoction.\n\r", ch);
	
	return;
}

void sum_elements(struct elemental_bias_type * elem1, const struct elemental_bias_type * elem2)
{
	elem1->bias_void += elem2->bias_void;
	elem1->bias_spirit += elem2->bias_spirit;
	elem1->bias_water += elem2->bias_water;
	elem1->bias_fire += elem2->bias_fire;
	elem1->bias_earth += elem2->bias_earth;
	elem1->bias_air += elem2->bias_air;
	elem1->bias_nature += elem2->bias_nature;
	elem1->bias_ancient += elem2->bias_ancient;
	elem1->bias_transmogrify += elem2->bias_transmogrify;
	return;
}

void sum_platonics(struct platonic_bias_type * plat1, const struct platonic_bias_type * plat2)
{
	plat1->bias_damage += plat2->bias_damage;
	plat1->bias_malediction += plat2->bias_malediction;
	plat1->bias_protection += plat2->bias_protection;
	plat1->bias_order += plat2->bias_order;
	plat1->bias_enhancement += plat2->bias_enhancement;
	plat1->bias_chaos += plat2->bias_chaos;
	plat1->bias_obscurement += plat2->bias_obscurement;
	plat1->bias_healing += plat2->bias_healing;
	plat1->bias_knowledge += plat2->bias_knowledge;
	plat1->bias_planar += plat2->bias_planar;
	plat1->bias_alteration += plat2->bias_alteration;
	return;
}

int count_color_rows()
{
	int i = 0;
	while (alchemy_color_table[i].color_spirit != NULL) i++;
	return i;
}

void determine_color(char * buf, char * top_spell, int elem_biased, double power)
{
	bool picked = FALSE;
	int total_rows = count_color_rows();
	int darkness = URANGE(0, static_cast<int> (power) / total_rows, total_rows - 1);
	if (top_spell == NULL)
	{
	    sprintf(buf,"clear");
	    return;
	}
	int elems = elemental_lookup(top_spell);
	if (elems & make_bin(BIAS_VOID_INDEX) && (!picked || elem_biased == BIAS_VOID_INDEX))
	{
		sprintf(buf, alchemy_color_table[darkness].color_void);
		picked = TRUE;
	}
	if (elems & make_bin(BIAS_WATER_INDEX) && (!picked || elem_biased == BIAS_WATER_INDEX))
	{
		sprintf(buf, alchemy_color_table[darkness].color_water);
		picked = TRUE;
	}
	if (elems & make_bin(BIAS_EARTH_INDEX) && (!picked || elem_biased == BIAS_EARTH_INDEX))
	{
		sprintf(buf, alchemy_color_table[darkness].color_earth);
		picked = TRUE;
	}
	if (elems & make_bin(BIAS_SPIRIT_INDEX) && (!picked || elem_biased == BIAS_SPIRIT_INDEX))
	{
		sprintf(buf, alchemy_color_table[darkness].color_spirit);
		picked = TRUE;
	}
	if (elems & make_bin(BIAS_AIR_INDEX) && (!picked || elem_biased == BIAS_AIR_INDEX))
	{
		sprintf(buf, alchemy_color_table[darkness].color_air);
		picked = TRUE;
	}
	if (elems & make_bin(BIAS_FIRE_INDEX) && (!picked || elem_biased == BIAS_FIRE_INDEX))
	{
		sprintf(buf, alchemy_color_table[darkness].color_fire);
		picked = TRUE;
	}
	if (elems & make_bin(BIAS_NATURE_INDEX) && (!picked || elem_biased == BIAS_NATURE_INDEX))
	{
		sprintf(buf, alchemy_color_table[darkness].color_nature);
		picked = TRUE;
	}
	if (elems & make_bin(BIAS_ANCIENT_INDEX) && (!picked || elem_biased == BIAS_ANCIENT_INDEX))
	{
		sprintf(buf, alchemy_color_table[darkness].color_ancient);
		picked = TRUE;
	}
	if (elems & make_bin(BIAS_TRANSMOGRIFY_INDEX) && (!picked || elem_biased == BIAS_TRANSMOGRIFY_INDEX))
	{
		sprintf(buf, alchemy_color_table[darkness].color_transmogrify);
		picked = TRUE;
	}
	if (!picked) sprintf(buf, alchemy_color_table[darkness].color_void);
	return;
}

bool retrieve_plat(char * spell_name, const struct alchemy_skills * skills_ref,
			int plist[MAX_POSSIBLE_PLATONIC_BIASES], int * power)
{
	int counter = 0, i;
	char comp1[MAX_STRING_LENGTH];
	char comp2[MAX_STRING_LENGTH];
	bool found = FALSE;
	sprintf(comp1, "%s", spell_name);
	while (skills_ref[counter].skill_name != NULL)
	{
		sprintf(comp2, "%s", skills_ref[counter].skill_name);
		if (!strcmp(comp1, comp2))
		{
			for (i = 0; i < MAX_POSSIBLE_PLATONIC_BIASES; i++)
				plist[i] = skills_ref[counter].plat_bias[i];
			*power = skills_ref[counter].power;
			found = TRUE;
			break;
		}
		counter++;
	}
	return found;
}

bool exists_in_space(char * spell_name, const struct alchemy_skills * skills_ref)
{
	int counter = 0;
	char comp1[MAX_STRING_LENGTH];
	char comp2[MAX_STRING_LENGTH];
	sprintf(comp1, "%s", spell_name);
	while (skills_ref[counter].skill_name != NULL)
	{
		sprintf(comp2, "%s", skills_ref[counter].skill_name);
		if (!strcmp(comp1, comp2)) return TRUE;
		counter++;
	}
	return FALSE;
}

int determine_spell_space(int spell_list[MAX_SKILL][2], const struct alchemy_skills * skills_ref, 
			   int plat_biased, double power, int alc_sn, int total_spells)
{
	int counter = 0, i;
	int bin_product = make_bin(alc_sn);
	
	while (skills_ref[counter].skill_name != NULL)
	{
		if (skills_ref[counter].power <= power && (bin_product & skills_ref[counter].alchemy_object))
		{
			i = 0;
			while (skills_ref[counter].plat_bias[i] != BIAS_NULL_INDEX && i < MAX_POSSIBLE_PLATONIC_BIASES)
			{
				if (make_bin(skills_ref[counter].plat_bias[i]) & plat_biased)
				{
					spell_list[total_spells][0] = skill_lookup(skills_ref[counter].skill_name);
					spell_list[total_spells][1] = skills_ref[counter].power;
					total_spells++;
					break;
				}
				i++;
			}
		}
		counter++;
	}
	return total_spells;
}

int make_bin(int prebin)
{
	if (prebin < 0) return 0;
	int base = 1, i;
	for (i = 0; i < prebin; i++)
		base *= 2;
	return base;
}
		
double evaluate_item( OBJ_DATA * obj, struct elemental_bias_type * elem, struct platonic_bias_type * plat, int * vtot)
{
	double item_power = 0;
	bool bodypart = TRUE, count_material = TRUE;;
	MOB_INDEX_DATA * pMobIndex;
	int plist[MAX_POSSIBLE_PLATONIC_BIASES];
	int elist, i, count, get_power, store_power, stone_vals[3];

	memset(elem, 0, sizeof(struct elemental_bias_type));
	memset(plat, 0, sizeof(struct platonic_bias_type));
	
	if (obj_is_affected(obj, gsn_alchemicstone))
	{
		for (i = 1; i < 4; i++)	stone_vals[i-1] = obj->objvalue[i];
		stone_biases(elem, plat, stone_vals);
		return item_power;
	}
	
	if (obj->pIndexData->vnum == VNUM_ALCHEMY_PRODUCT) count_material = FALSE;
	
	*vtot += obj->pIndexData->vnum;
	
	if (obj->level > 20)
	{
		switch (obj->pIndexData->limit)
		{
			case 1: item_power += 10; break;
			case 2:
			case 3:
			case 4:
			case 5: item_power += 4; break;
			case 6:
			case 7:
			case 8:
			case 9:
			case 10: item_power += 3; break;
			default: break;
		}
	}
	if (obj->pIndexData->limit >= 11) item_power++;
	if (obj->pIndexData->limit >= 11 && obj->pIndexData->limit <= 30) item_power++;
	
	if (obj->cost > 150000) item_power++;
	if (obj->cost > 100000) item_power++;
	if (obj->cost > 50000) item_power++;
	
	switch (obj->pIndexData->vnum)
	{
		case OBJ_VNUM_SEVERED_HEAD:
		case OBJ_VNUM_TROPHY_HEAD:
		case OBJ_VNUM_BRAINS:
		case OBJ_VNUM_TROPHY_BRAINS:	plat->bias_knowledge += 10; break;
		case OBJ_VNUM_SLICED_ARM:	plat->bias_enhancement += 5; break;
		case OBJ_VNUM_SLICED_LEG:	plat->bias_planar += 10; break;
		case OBJ_VNUM_TORN_HEART:
		case OBJ_VNUM_TROPHY_HEART:	plat->bias_malediction += 10; break;
		case OBJ_VNUM_GUTS:
		case OBJ_VNUM_TROPHY_ENTRAILS:	plat->bias_malediction += 10; break;
		case OBJ_VNUM_TROPHY_HAND:	plat->bias_damage += 5; plat->bias_protection += 5; break;
		case OBJ_VNUM_TROPHY_FINGER:	plat->bias_obscurement += 5; plat->bias_enhancement += 5; break;
		case OBJ_VNUM_TROPHY_EAR:	plat->bias_knowledge += 5; plat->bias_alteration += 5; break;
		case OBJ_VNUM_TROPHY_EYE:	plat->bias_knowledge += 10; break;
		case OBJ_VNUM_TROPHY_TONGUE:	plat->bias_healing += 5; plat->bias_alteration += 5; break;
		case OBJ_VNUM_TAIL:		plat->bias_obscurement += 10; break;
		case OBJ_VNUM_STONE_POWER:	elem->bias_ancient += 10; item_power += 10; break;
		default: bodypart = FALSE; break;
	}
	if (bodypart && obj->value[0] > 0)
	{
		pMobIndex = get_mob_index(obj->value[0]);
		if (IS_SET(pMobIndex->act, ACT_NOSUBDUE)) item_power += 2;
		if (IS_SET(pMobIndex->act, ACT_BADASS)) item_power += 3;
		if (pMobIndex->race == global_int_race_chtaren) elem->bias_spirit += 10;
		else if (pMobIndex->race == global_int_race_shuddeni) elem->bias_void += 10;
		else if (pMobIndex->race == global_int_race_srryn) elem->bias_fire += 10;
		else if (pMobIndex->race == global_int_race_caladaran) plat->bias_order += 10;
		else if (pMobIndex->race == global_int_race_nefortu) {elem->bias_void += 5; elem->bias_fire += 5;}
		else if (pMobIndex->race == global_int_race_ethron) elem->bias_nature += 10;
		else if (pMobIndex->race == global_int_race_aelin) plat->bias_order += 10;
		else if (pMobIndex->race == global_int_race_alatharya) elem->bias_ancient += 10;
		else if (pMobIndex->race == race_lookup("dragon")) {item_power++; plat->bias_knowledge += 10;}
		else if (pMobIndex->race == race_lookup("demon")) {item_power++; elem->bias_void += 10;}
	}
	else if (bodypart) item_power += 2; //pc bodypart
	else
	{
		if (obj->level >= 40 && obj->level <= 50) item_power += 1;
		else if (obj->level == 51) item_power += 2;
		else if (obj->level >= 52 && obj->level <= 54) item_power += 3;
		else if (obj->level >= 55 && obj->level <= 57) item_power += 4;
		else if (obj->level >= 58 && obj->level <= 60) item_power += 5;
		else if (obj->level >= 61 && obj->level <= 64) item_power += 6;
		else if (obj->level >= 65 && obj->level <= 69) item_power += 7;
		else if (obj->level >= 70) item_power += 10;
	}
	
	if (obj->wear_flags & ITEM_WEAR_FLOAT)
	{
		elem->bias_air += 10;
		plat->bias_planar += 5;
	}
	if (obj->wear_flags & ITEM_NO_SAC) item_power++;
	if (IS_SET(obj->extra_flags[0], ITEM_DARK) || IS_SET(obj->extra_flags[0], ITEM_EVIL)) elem->bias_void += 5;
	if (IS_SET(obj->extra_flags[0], ITEM_GLOW))
	{
		item_power += 1;
		elem->bias_spirit += 5;
	}
	if (IS_SET(obj->extra_flags[0], ITEM_HUM)) item_power += 1; 
	if (IS_SET(obj->extra_flags[0], ITEM_MAGIC)) item_power += 1;
	if (IS_SET(obj->extra_flags[0], ITEM_ANTI_EVIL))
	{
		elem->bias_fire -= 5;
		elem->bias_void -= 5;
	}
	if (IS_SET(obj->extra_flags[0], ITEM_ANTI_NEUTRAL))
	{
		elem->bias_air -= 5;
		elem->bias_water -= 5;
	}
	if (IS_SET(obj->extra_flags[0], ITEM_ANTI_GOOD))
	{
		elem->bias_spirit -= 5;
		elem->bias_water -= 5;
	}
	if (IS_SET(obj->extra_flags[0], ITEM_WARM)) plat->bias_protection += 5;
	if (IS_OBJ_STAT(obj, ITEM_INVIS)) plat->bias_obscurement += 10;
	if (IS_OBJ_STAT(obj, ITEM_BLESS)) elem->bias_spirit += 5;
	if (IS_OBJ_STAT_EXTRA(obj, ITEM_FIRE)) elem->bias_fire += 10;
	if (IS_OBJ_STAT(obj, ITEM_NODESTROY)) item_power++;

	if (obj->item_type == ITEM_WEAPON)
	{
		if (IS_WEAPON_STAT(obj, WEAPON_SHOCKING)) elem->bias_air += 10;
		if (IS_WEAPON_STAT(obj, WEAPON_FLAMING)) elem->bias_fire += 10;
		if (IS_WEAPON_STAT(obj, WEAPON_VAMPIRIC)) elem->bias_void += 10;
		if (IS_WEAPON_STAT(obj, WEAPON_POISON)) {plat->bias_malediction += 5; elem->bias_void += 5;}
		if (IS_WEAPON_STAT(obj, WEAPON_FROST)) elem->bias_water += 10;
	}
	
	if (obj->item_type == ITEM_DRINK_CON)
	{
		memset(elem, 0, sizeof(struct elemental_bias_type));
	        memset(plat, 0, sizeof(struct platonic_bias_type));
		item_power = 0;
		count_material = FALSE;
		if (obj->value[2] == liq_lookup("water"))
		{
			elem->bias_water += 10; plat->bias_healing += 4;
			plat->bias_protection += 4; plat->bias_enhancement += 2;
		}
		else if (obj->value[2] == liq_lookup("ale") || obj->value[2] == liq_lookup("beer")
			 || obj->value[2] == liq_lookup("dark ale"))
		{
			elem->bias_water += 5; elem->bias_nature += 5;
			plat->bias_obscurement += 5; plat->bias_healing += 5;
		}
		else if (obj->value[2] == liq_lookup("red wine")
			 || obj->value[2] == liq_lookup("white wine")
			 || obj->value[2] == liq_lookup("rose wine")
			 || obj->value[2] == liq_lookup("benedictine wine"))
		{	
			elem->bias_water += 6; elem->bias_nature += 4;
			plat->bias_obscurement += 3; plat->bias_knowledge += 4; plat->bias_enhancement += 4;
		}
		else if (obj->value[2] == liq_lookup("whisky"))
		{
			elem->bias_water += 3; elem->bias_fire += 7;
			plat->bias_obscurement += 7; plat->bias_damage += 3;
		}
		else if (obj->value[2] == liq_lookup("lemonade"))
		{
			elem->bias_water += 7; elem->bias_nature += 3;
			plat->bias_malediction += 3; plat->bias_enhancement += 7;
		}
		else if (obj->value[2] == liq_lookup("firebreather"))
		{
			elem->bias_water += 1; elem->bias_fire += 9;
			plat->bias_damage += 5; plat->bias_knowledge += 3; plat->bias_obscurement += 2;
		}
		else if (obj->value[2] == liq_lookup("local specialty")
			 || obj->value[2] == liq_lookup("aquavit")
			 || obj->value[2] == liq_lookup("sherry")
			 || obj->value[2] == liq_lookup("framboise")
			 || obj->value[2] == liq_lookup("cordial"))
		{
			elem->bias_water += 3; elem->bias_nature += 4; elem->bias_fire += 3;
			plat->bias_obscurement += 4; plat->bias_knowledge += 3; plat->bias_chaos += 3;
		}
		else if (obj->value[2] == liq_lookup("slime mold juice"))
		{
			elem->bias_void += 5; elem->bias_nature += 5;
			plat->bias_malediction += 7; plat->bias_alteration += 3;
		}
                else if (obj->value[2] == liq_lookup("milk"))
		{
			elem->bias_water += 3; elem->bias_nature += 7;
			plat->bias_enhancement += 8; plat->bias_order += 2;
		}
        	else if (obj->value[2] == liq_lookup("tea"))
		{
			elem->bias_water += 6; elem->bias_nature += 4;
			plat->bias_knowledge += 4; 
		}
        	else if (obj->value[2] == liq_lookup("coffee"))
		{
			elem->bias_water += 7; elem->bias_nature += 3;
			plat->bias_enhancement += 7; plat->bias_chaos += 3;
		}
        	else if (obj->value[2] == liq_lookup("blood"))
		{
			elem->bias_void += 7; elem->bias_fire += 3;
			plat->bias_malediction += 6; plat->bias_damage += 4; 
		}
        	else if (obj->value[2] == liq_lookup("salt water"))
		{
			elem->bias_water += 5; elem->bias_earth += 5;
			plat->bias_malediction += 5; plat->bias_order += 5;
		}
        	else if (obj->value[2] == liq_lookup("root beer"))
		{
			elem->bias_water += 4; elem->bias_nature += 6;
			plat->bias_healing += 5; plat->bias_enhancement += 5;
		}
                else if (obj->value[2] == liq_lookup("elvish wine"))
		{
			elem->bias_ancient += 10;
			plat->bias_alteration += 3; plat->bias_enhancement += 3; plat->bias_enhancement += 4;
		}
		else if (obj->value[2] == liq_lookup("champagne"))
		{
			elem->bias_water += 2; elem->bias_air += 5; elem->bias_fire += 3;
			plat->bias_enhancement += 5; plat->bias_chaos += 5;
		}
		else if (obj->value[2] == liq_lookup("mead"))
		{
			elem->bias_water += 5; elem->bias_nature += 5;
			plat->bias_planar += 5; plat->bias_healing += 5;
		}
		else if (obj->value[2] == liq_lookup("vodka"))
		{
			elem->bias_water += 5; elem->bias_fire += 5;
			plat->bias_chaos += 5; plat->bias_obscurement += 5;
		}
		else if (obj->value[2] == liq_lookup("cranberry juice"))
		{
			elem->bias_water += 2; elem->bias_nature += 8;
			plat->bias_malediction += 10;
		}
		else if (obj->value[2] == liq_lookup("orange juice"))
		{
			elem->bias_water += 4; elem->bias_nature += 6;
			plat->bias_healing += 6; plat->bias_protection += 4;
		}
		else if (obj->value[2] == liq_lookup("absinthe"))
		{
			elem->bias_water += 1; elem->bias_nature += 3; elem->bias_fire += 6;
			plat->bias_damage += 10;
		}
		else if (obj->value[2] == liq_lookup("brandy"))
		{
			elem->bias_water += 5; elem->bias_nature += 5;
			plat->bias_obscurement += 10;
		}
		else if (obj->value[2] == liq_lookup("schnapps"))
		{
			elem->bias_water += 5; elem->bias_fire += 5;
			plat->bias_healing += 5; plat->bias_damage += 5;
		}
		else if (obj->value[2] == liq_lookup("icewine"))
		{
			elem->bias_water += 8; elem->bias_nature += 2;
			plat->bias_order += 3; plat->bias_protection += 7;
		}
		else if (obj->value[2] == liq_lookup("amontillado"))
		{
			elem->bias_water += 3; elem->bias_ancient += 7;
			plat->bias_knowledge += 10;
		}
		else if (obj->value[2] == liq_lookup("rum"))
		{
			elem->bias_water += 2; elem->bias_fire += 4; elem->bias_void += 4;
			plat->bias_damage += 5; plat->bias_malediction += 2; plat->bias_obscurement += 3;
		}
	}
	else if (obj->item_type == ITEM_POTION || obj->item_type == ITEM_OIL || obj->item_type == ITEM_PILL
		|| obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF)
	{
		count_material = (obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF);
		int spellIndex = ((obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF) ? 3 : 1);
		if (obj->value[spellIndex] >= 0)
		{
			elist = elemental_retrieval(skill_table[obj->value[spellIndex]].name, plist, &get_power);
			count = 0;
			for (i = 0; i < 30; i++) if ((elist & (1 << i)) != 0) count++;
			store_power = get_power / (count == 0 ? 1 : count); //Dilute by count, truncate.
			if (elist & make_bin(BIAS_VOID_INDEX)) elem->bias_void += store_power;
			if (elist & make_bin(BIAS_SPIRIT_INDEX)) elem->bias_spirit += store_power;
			if (elist & make_bin(BIAS_WATER_INDEX)) elem->bias_water += store_power;
			if (elist & make_bin(BIAS_FIRE_INDEX)) elem->bias_fire += store_power;
			if (elist & make_bin(BIAS_AIR_INDEX)) elem->bias_air += store_power;
			if (elist & make_bin(BIAS_EARTH_INDEX)) elem->bias_earth += store_power;
			if (elist & make_bin(BIAS_NATURE_INDEX)) elem->bias_nature += store_power;
			if (elist & make_bin(BIAS_ANCIENT_INDEX)) elem->bias_ancient += store_power;
			if (elist & make_bin(BIAS_TRANSMOGRIFY_INDEX)) elem->bias_transmogrify += store_power;
		
			//First run for count, second for specific ones. This makes me wince.
			i = 0; count = 0;
			while (count < MAX_POSSIBLE_PLATONIC_BIASES && plist[count] != BIAS_NULL_INDEX)	count++;
			store_power = get_power / (count == 0 ? 1 : count);
			while (i < MAX_POSSIBLE_PLATONIC_BIASES && plist[i] != BIAS_NULL_INDEX)
			{
				switch (plist[i])
				{
					case BIAS_DAMAGE_INDEX: plat->bias_damage += store_power; break;               
					case BIAS_MALEDICTION_INDEX: plat->bias_malediction += store_power; break;          
					case BIAS_PROTECTION_INDEX: plat->bias_protection += store_power; break;           
					case BIAS_ORDER_INDEX: plat->bias_order += store_power; break;                
					case BIAS_ENHANCEMENT_INDEX: plat->bias_enhancement += store_power; break;          
					case BIAS_CHAOS_INDEX: plat->bias_chaos += store_power; break;                
					case BIAS_OBSCUREMENT_INDEX: plat->bias_obscurement += store_power; break;          
					case BIAS_HEALING_INDEX: plat->bias_healing += store_power; break;              
					case BIAS_KNOWLEDGE_INDEX: plat->bias_knowledge += store_power; break;            
					case BIAS_PLANAR_INDEX: plat->bias_planar += store_power; break;               
					case BIAS_ALTERATION_INDEX: plat->bias_alteration += store_power; break;           
					default: break;
				}
				i++;
			}
		}
	}
	else
	{
		sum_elements(elem, &(item_table[item_ref(obj->item_type)].elem_bias));
		sum_platonics(plat, &(item_table[item_ref(obj->item_type)].plat_bias));
	}

	if (count_material)
	{
		sum_elements(elem, &(material_table[obj->material].elem_bias));
		sum_platonics(plat, &(material_table[obj->material].plat_bias));
	}

	if (obj_is_affected(obj, gsn_distill))
	{
		elem->bias_spirit += 10;
		elem->bias_void -= 10;
	}
	if (obj_is_affected(obj, gsn_pulverize))
	{
		elem->bias_air += 10;
		elem->bias_earth -= 10;
	}
	if (obj_is_affected(obj, gsn_reforge))
	{
		elem->bias_earth += 10;
		elem->bias_air -= 10;
	}
	if (obj_is_affected(obj, gsn_boil))
	{
		elem->bias_fire += 5;
		elem->bias_water -= 5;
	}
	if (obj_is_affected(obj, gsn_dissolve))
	{
		elem->bias_fire -= 10;
		elem->bias_water += 10;
	}
	if (obj_is_affected(obj, gsn_dilute))
	{
		elem->bias_void += 10;
		elem->bias_spirit -= 10;
	}
	
	return item_power;
}

/*
 * Does basic alchemical sanity checks
 */
bool basic_alchemy_sanity_checks( CHAR_DATA *ch, char *argument, int alc_sn)
{
	char buf[MAX_STRING_LENGTH];
	// Make sure they have the skill
	if (get_skill(ch, alchemy_table[alc_sn].gsn) == 0)
	{
		send_to_char("Huh?\n\r", ch);
		return FALSE;
	}
	if (!valid_alchemy_location(ch)) return FALSE;	

	//Make sure they have enough mana
	if (ch->mana < skill_table[alchemy_table[alc_sn].gsn].min_mana)
	{
		sprintf(buf, "You are too tired to %s %ss right now.\n\r",
			alchemy_table[alc_sn].action, alchemy_table[alc_sn].product);
		send_to_char(buf, ch);
		return FALSE;
	}
	
	//Make sure they have arguments
    if (argument[0] == '\0')
	{
		sprintf(buf, "With what ingredients do you wish to %s your %s?\n\r", 
			alchemy_table[alc_sn].action, alchemy_table[alc_sn].product);
		send_to_char(buf, ch);
		return FALSE;
	}

	return TRUE;
}

bool duplicate_ingredients( CHAR_DATA * ch, OBJ_DATA * ingredients[], int total_ingredients)
{
	char buf[MAX_STRING_LENGTH];
	int i, j;
	for (i = 0; i < total_ingredients; i++)
	{
		if (IS_OBJ_STAT(ingredients[i], ITEM_NODROP))
		{
			sprintf(buf, "You can't let go of %s.\n\r", ingredients[i]->short_descr);
			send_to_char(buf, ch);
			return TRUE;
		}
		if (ingredients[i]->wear_flags & ITEM_NO_SAC)
		{
			sprintf(buf, "You can't destroy %s.\n\r", ingredients[i]->short_descr);
			send_to_char(buf, ch);
			return TRUE;
		}
		for (j = i+1; j < total_ingredients; j++)
		{
			if (ingredients[i] == ingredients[j])
			{
				sprintf(buf, "You cannot use %s more than once!\n\r", ingredients[i]->short_descr);
				send_to_char(buf, ch);
				return TRUE;
			}
		}
	}
	return FALSE;
}
