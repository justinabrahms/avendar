typedef struct  dictionary_data               DICTIONARY_DATA;

extern	    DICTIONARY_DATA *	    g_char_names;
extern	    DICTIONARY_DATA *	    g_denied_names;

struct dictionary_data
{
    DICTIONARY_DATA * next_letter[26];
    void *info;
};

extern void * dict_lookup args((DICTIONARY_DATA *dictionary, const char *string));
extern void   dict_insert args((DICTIONARY_DATA *dictionary, char *string, void *info));
extern DICTIONARY_DATA * new_dict args( ( void ) );

