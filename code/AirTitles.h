#ifndef AIRTITLES_H
#define AIRTITLES_H

#include "merc.h"

class AirTitles
{
    static const char * FallbackTitle;

    public:
        static const char * LookupTitle(const CHAR_DATA & ch);

    private:
        AirTitles();
        static const char * LookupDefault(int level, bool female);
        static const char * LookupEndlessFacade(int level, bool female);
        static const char * LookupFlickeringSkies(int level, bool female);
        static const char * LookupWindrider(int level, bool female);
        static const char * LookupEarthMinor(int level, bool female);
        static const char * LookupVoidMinor(int level, bool female);
        static const char * LookupSpiritMinor(int level, bool female);
        static const char * LookupFireMinor(int level, bool female);
        static const char * LookupWaterMinor(int level, bool female);
};

#endif
