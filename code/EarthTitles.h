#ifndef EARTHTITLES_H
#define EARTHTITLES_H

#include "merc.h"

class EarthTitles
{
    static const char * FallbackTitle;

    public:
        static const char * LookupTitle(const CHAR_DATA & ch);

    private:
        EarthTitles();
        static const char * LookupDefault(int level, bool female);
        static const char * LookupStoneshaper(int level, bool female);
        static const char * LookupGeomancer(int level, bool female);
        static const char * LookupWakenedStone(int level, bool female);
        static const char * LookupAirMinor(int level, bool female);
        static const char * LookupVoidMinor(int level, bool female);
        static const char * LookupSpiritMinor(int level, bool female);
        static const char * LookupFireMinor(int level, bool female);
        static const char * LookupWaterMinor(int level, bool female);
};

#endif
