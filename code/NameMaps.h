#ifndef NAMEMAPS_H
#define NAMEMAPS_H

#include "merc.h"

class NameMaps
{
    public:
        static void Add(OBJ_DATA & obj);
        static void Add(CHAR_DATA & ch);
        static void Remove(OBJ_DATA & obj);
        static void Remove(CHAR_DATA & ch);
        static OBJ_DATA * LookupObject(CHAR_DATA * ch, const char * name, bool exactOnly);
        static CHAR_DATA * LookupChar(CHAR_DATA * ch, const char * name, bool exactOnly);
};

void setName(CHAR_DATA & ch, const char * name);
void setFakeName(CHAR_DATA & ch, const char * name);
void setUniqueName(CHAR_DATA & ch, const char * name);
void setName(OBJ_DATA & obj, const char * name);

#endif
