#define LANG_COMMON	0
#define LANG_ALATHARYA	1
#define LANG_SRRYN	2
#define LANG_KANKORAN	3
#define LANG_CHTAREN	4
#define LANG_AELIN	5
#define LANG_SHUDDENI	6
#define LANG_NEFORTU	7
#define LANG_CALADARAN	8
#define LANG_ETHRON	9
#define	LANG_ARCANE	10

#define MAX_LDICT	20
#define MAX_LANGUAGE 	11
#define MAX_CONSTRAINT	8
#define MAX_NUMPERCONS	55
#define FREQ_TOTAL	100
#define MAX_LPAIRS	200
#define MAX_LTRIPS	200

#define IS_UPPER(c)	((c >= 65) && (c <= 90))
#define IS_VOWEL(c)     ((UPPER(c) == 'A') || (UPPER(c) == 'E') || (UPPER(c) == 'I') || (UPPER(c) == 'O') || (UPPER(c) == 'U'))

typedef	struct	lang_struct		LANG_STRING;
typedef	struct	lang_dict_data		LDICT_DATA;

void    mprog_speech_trigger    args ( ( LANG_STRING* txt, CHAR_DATA* mob ) );

extern	const	struct	language_type	lang_data[MAX_LANGUAGE];

struct	lang_dict_data
{
    char *	word;
    char *	replace;
    bool	coll;
};


struct language_type
{
    char *	name;
    int * 	sn;
    bool	rough;
    bool	long_words;
    char *	cons[MAX_CONSTRAINT][MAX_NUMPERCONS];
    struct {
	char * letters;
	int freq;
    }		pairs[MAX_LPAIRS];
    struct {
	char * letters;
	int freq;
    }		triplets[MAX_LTRIPS];
    int	vowel_freq[6];
    int	consonant_freq[21];
    LDICT_DATA	dict_data[MAX_LDICT];
};

// Note: MAX_INPUT_LENGTH * 2 is an arbitrary value.
struct lang_struct
{
    char *	orig_string;
    char	full_trans[MAX_INPUT_LENGTH*2];
    struct {
	int	start;
	int	len;
	int	lang;
	LDICT_DATA * coll;
    }		word[MAX_INPUT_LENGTH];
    int	ptrans;
};

extern	void	language_preproc	args( ( ) );
extern	char *	translate_out_new	args( ( CHAR_DATA *ch, LANG_STRING *lang_in ) );
extern	LANG_STRING* translate_spoken_new args((CHAR_DATA *ch, char *instr ) );

