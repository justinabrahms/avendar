#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include "merc.h"
#include "dictionary.h"

// Local function declaration
bool	checkString	args( ( const char *string ) );
void	insert_internal	args( ( DICTIONARY_DATA *dictionary, char *string, void *info ) );
void	*lookup_internal args(( DICTIONARY_DATA *dictionary, const char *string ) );


void dict_insert(DICTIONARY_DATA *dictionary, char *string, void *info)
{
    if (!checkString(string) || !dictionary)
	return;

    insert_internal(dictionary, string, info); 
}

void insert_internal(DICTIONARY_DATA *dictionary, char *string, void *info)
{
    int chIndex;
    DICTIONARY_DATA *next_dict;

    chIndex = LOWER(string[0]) - 'a';
    if (dictionary->next_letter[chIndex] == NULL) 
	dictionary->next_letter[chIndex] = new_dict();
    next_dict = dictionary->next_letter[chIndex];
    if (!isalpha(string[1]))
    {
	next_dict->info = info;
  	return; 
    }    
    insert_internal(next_dict, string + sizeof(char), info);
}

void *dict_lookup(DICTIONARY_DATA *dictionary, const char *string)
{
    //This is really a wrapper function, that checks to see if the
    //string is legal and cleans it up.

    if (!checkString(string) || !dictionary) 
	return NULL;

    return lookup_internal(dictionary, string);
}

bool checkString(const char *string)
{
    if (!isalpha(string[0]))
	return FALSE;
    
    return TRUE;
}

void *lookup_internal(DICTIONARY_DATA *dictionary, const char *string)
{
    int chIndex;
    DICTIONARY_DATA * next_dict;

    if (string[0] == '\0')
	return NULL;

    chIndex = LOWER(string[0]) - 'a';

    next_dict = dictionary->next_letter[chIndex];

    if (next_dict == NULL)
	return NULL;

    if (!isalpha(string[1]))
	return next_dict->info;

    return lookup_internal(next_dict, string + sizeof(char));
}

DICTIONARY_DATA *new_dict()
{
    DICTIONARY_DATA *dict = (DICTIONARY_DATA*)alloc_perm(sizeof(*dict));
    int i;

    g_num_dicts++;

    for (i = 0; i < 26; i++)
	dict->next_letter[i] = NULL;
    dict->info = NULL;

    return dict;
}
