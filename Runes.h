#ifndef RUNES_H
#define RUNES_H

#include "merc.h"
#include "RuneInfo.h"
#include <string>
#include <vector>

class Runes
{
    struct RuneInfo
    {
        RuneInfo(OBJ_DATA & objIn, AFFECT_DATA & objAffIn, AFFECT_DATA * chAffIn);

        OBJ_DATA * obj;
        AFFECT_DATA * objAff;
        AFFECT_DATA * chAff;
    };

    public:
        static std::string ListRunes(CHAR_DATA & ch);
        static void ShowObjectRunes(CHAR_DATA & ch, OBJ_DATA & obj, char * buffer);
        static unsigned int BonusCount(CHAR_DATA & ch);
        static unsigned int InvokedCount(const CHAR_DATA & ch, Rune::Type type);
        static unsigned int InvokedCountHere(const CHAR_DATA & ch, Rune::Type type, unsigned int & bonusCount);
        static bool IsInvoked(const OBJ_DATA & obj, Rune::Type type);
        static bool CarveRune(CHAR_DATA & ch, OBJ_DATA & obj, bool success, const char * argument);
        static void InvokeRune(CHAR_DATA & ch, const char * argument);
        static void RevokeRune(CHAR_DATA & ch, const char * argument);
        static void UpdateAttunementsFor(CHAR_DATA & ch, int count);
        static void UpdateManaCostsFor(CHAR_DATA & ch);

    private:
        static int AttunementCount(const CHAR_DATA & ch);
        static bool UpdateRuneManaCosts(CHAR_DATA & ch, int attunements);
        static size_t LookupRune(const std::vector<RuneInfo> & runes, const char * argument);
        static std::vector<RuneInfo> LookupInvokedRunes(const CHAR_DATA & ch);
        static std::vector<RuneInfo> LookupInvokableRunes(const CHAR_DATA & ch);
        static const CHAR_DATA * InvokerOf(const AFFECT_DATA & af);
        static const Rune * Lookup(unsigned int index);
        static int ManaCost(const Rune & rune, int attunements, bool & makeMonoFree);
        static bool KnowsRune(const CHAR_DATA & ch, const Rune & rune);
};

inline Runes::RuneInfo::RuneInfo(OBJ_DATA & objIn, AFFECT_DATA & objAffIn, AFFECT_DATA * chAffIn) :
    obj(&objIn), objAff(&objAffIn), chAff(chAffIn) {}

#endif
