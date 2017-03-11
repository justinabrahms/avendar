#ifndef VOIDTITLES_H
#define VOIDTITLES_H

#include "merc.h"

class VoidTitles
{
    static const char * FallbackTitle;

    public:
        static const char * LookupTitle(const CHAR_DATA & ch);

    private:
        VoidTitles();
        static const char * LookupDefault(int level, bool female);
        static const char * LookupNecromancer(int level, bool female);
        static const char * LookupNightfall(int level, bool female);
        static const char * LookupRivenVeil(int level, bool female);
        static const char * LookupAirMinor(int level, bool female);
        static const char * LookupEarthMinor(int level, bool female);
        static const char * LookupSpiritMinor(int level, bool female);
        static const char * LookupFireMinor(int level, bool female);
        static const char * LookupWaterMinor(int level, bool female);
};

#endif
