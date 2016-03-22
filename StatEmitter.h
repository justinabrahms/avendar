#ifndef STATEMITTER_H
#define STATEMITTER_H

#include <fstream>
#include "merc.h"

class StatEmitter
{
    static const char * StatFile;

    public:
        static void EmitStats();

    private:
        static void EmitHeaderRow(std::ostream & out);
        static void EmitStatsFor(std::ostream & out, const CHAR_DATA & ch, bool online);
};

#endif
