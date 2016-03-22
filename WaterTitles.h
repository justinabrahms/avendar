#ifndef WATERTITLES_H
#define WATERTITLES_H

#include "merc.h"

class WaterTitles
{
    static const char * FallbackTitle;

    public:
        static const char * LookupTitle(const CHAR_DATA & ch);

    private:
        WaterTitles();
        static const char * LookupDefault(int level, bool female);
        static const char * LookupLivingWaters(int level, bool female);
        static const char * LookupWaveborne(int level, bool female);
        static const char * LookupWintertide(int level, bool female);
        static const char * LookupEarthMinor(int level, bool female);
        static const char * LookupVoidMinor(int level, bool female);
        static const char * LookupSpiritMinor(int level, bool female);
        static const char * LookupFireMinor(int level, bool female);
        static const char * LookupAirMinor(int level, bool female);
};

#endif
