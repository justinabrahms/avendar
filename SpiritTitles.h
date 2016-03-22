#ifndef SPIRITTITLES_H
#define SPIRITTITLES_H

#include "merc.h"

class SpiritTitles
{
    static const char * FallbackTitle;

    public:
        static const char * LookupTitle(const CHAR_DATA & ch);

    private:
        SpiritTitles();
        static const char * LookupDefault(int level, bool female);
        static const char * LookupWeavedancer(int level, bool female);
        static const char * LookupSilverLight(int level, bool female);
        static const char * LookupEternalDawn(int level, bool female);
        static const char * LookupWaterMinor(int level, bool female);
        static const char * LookupEarthMinor(int level, bool female);
        static const char * LookupVoidMinor(int level, bool female);
        static const char * LookupFireMinor(int level, bool female);
        static const char * LookupAirMinor(int level, bool female);
};

#endif
