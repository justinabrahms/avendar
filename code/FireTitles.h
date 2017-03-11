#ifndef FIRETITLES_H
#define FIRETITLES_H

#include "merc.h"

class FireTitles
{
    static const char * FallbackTitle;

    public:
        static const char * LookupTitle(const CHAR_DATA & ch);

    private:
        FireTitles();
        static const char * LookupDefault(int level, bool female);
        static const char * LookupFlameheart(int level, bool female);
        static const char * LookupRagingInferno(int level, bool female);
        static const char * LookupGoldenFlames(int level, bool female);
        static const char * LookupWaterMinor(int level, bool female);
        static const char * LookupEarthMinor(int level, bool female);
        static const char * LookupVoidMinor(int level, bool female);
        static const char * LookupSpiritMinor(int level, bool female);
        static const char * LookupAirMinor(int level, bool female);
};

#endif
