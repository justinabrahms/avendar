#define MAX_ITEMS_IN_ANY		9
#define MIN_ITEMS_IN_ANY		3
#define MAX_ALCHEMICAL_SKILL_TYPES	3  //Only refers to mix/make/brew
#define MAX_POSSIBLE_PLATONIC_BIASES	6
#define MAX_ALCHEMY_COLORS		8
#define MAX_ALCHEMY_ADJECTIVES		46
#define MAX_WAND_ADJECTIVES		16
#define START_NICE_ADJECTIVES		13
#define VNUM_ALCHEMY_PRODUCT		165
#define MOB_VNUM_HOMONCULUS		174
#define VNUM_GOLEM			165
#define VNUM_MIX			783
#define VNUM_BREW			784
#define VNUM_MAKE			785
#define BIAS_NULL_INDEX			-1
#define BIAS_SPIRIT_INDEX		1
#define BIAS_WATER_INDEX		2
#define BIAS_FIRE_INDEX			3
#define BIAS_EARTH_INDEX		4
#define BIAS_AIR_INDEX			5
#define BIAS_NATURE_INDEX		6
#define BIAS_ANCIENT_INDEX		7
#define BIAS_TRANSMOGRIFY_INDEX		8
#define BIAS_VOID_INDEX			9
#define BIAS_DAMAGE_INDEX              	1
#define BIAS_MALEDICTION_INDEX		2
#define BIAS_PROTECTION_INDEX           3
#define BIAS_ORDER_INDEX                4
#define BIAS_ENHANCEMENT_INDEX          5
#define BIAS_CHAOS_INDEX               	6
#define BIAS_OBSCUREMENT_INDEX         	7
#define BIAS_HEALING_INDEX              8
#define BIAS_KNOWLEDGE_INDEX            9
#define BIAS_PLANAR_INDEX        	10
#define BIAS_ALTERATION_INDEX           11

struct alchemy_type
{
	int		gsn;		//gsn of the skill referring to this type
	int		min_items;	//min ingredients
	int		max_items;	//max ingredients
	char *		action;		//i.e. 'mix', 'brew', etc.
	char *		product;	//i.e. 'oil', 'potion', etc.
	int		item_type;
	int		weight;
};

struct alchemy_skills_special
{
	int		local_gsn;
	char *		short_descr;
	char *		extra_name;
	bool		metal_only; 
	bool		solid_only;
	bool		liquid_only;
};

struct alchemy_skills
{
	char *		skill_name;
	int		alchemy_object;	//1: oil  2: potion  4: pill
	int		plat_bias[MAX_POSSIBLE_PLATONIC_BIASES];
	int		power;
};

struct alchemy_colors
{
	char * 		color_spirit;
	char * 		color_water;
	char * 		color_air;
	char * 		color_earth;
	char *		color_fire;
	char * 		color_void;
	char *		color_nature;
	char *		color_ancient;
	char *		color_transmogrify;
};

struct alchemy_adjectives
{
	char *		adjective;
};
