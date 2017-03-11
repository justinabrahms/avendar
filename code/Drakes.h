#ifndef DRAKES_H
#define DRAKES_H

#include "merc.h"
#include <vector>
#include <map>

class Drakes
{
    public:
        enum Age {Hatchling = 0, Youngling, Fledgling, Wingborne, Greater, Elder, Ancient};

        enum Special 
        {   
            Diamondskin,    Bash,       Overwhelm, 
            Loyal,          Ambush,     Stonemend, 
            Charge,         Defender,   Frantic,
            Sunderer,       Ferocious,  Vicious,
            Attentive,      Sandman,    Biting
        };

        static void WakenStone(CHAR_DATA & ch, const char * argument, int skill);
        static int SpecialCount(CHAR_DATA & drake, Special special);
        static bool CheckMature(CHAR_DATA & drake, const AFFECT_DATA & paf);
        static bool IsMinimumAge(const CHAR_DATA & drake, Age age);
        static bool HasStonesong(const CHAR_DATA & drake);

    private:
        /// Types
        struct ResistInfo
        {
            ResistInfo(int typeIn, int modBaseIn, int modStepIn);

            int type;
            int modBase;
            int modStep;
        };

        struct SpecialInfo
        {
            SpecialInfo(Special typeIn, Age ageIn, int amountIn);

            Special type;
            Age age;
            int amount;
        };

        struct Info
        {
            Info(int maturationModIn, int hpModIn, int damageModIn, int damageTypeIn);

            int maturationMod;
            int hpMod;
            int damageMod;
            int damageType;
            std::vector<ResistInfo> resistances;
            std::vector<SpecialInfo> specials;
        };

        typedef std::map<int, Info> MapType;

        /// Methods
        static int AddInfo(MapType & infos, const char * name, const Info & info);
        static const MapType & BuildInfos();
        static const MapType & Infos();
        static const Info * Lookup(int stoneType);

        static void AdjustDrake(CHAR_DATA & ch, CHAR_DATA & drake, int stoneType, const Info & info, int level, Age age);
        static std::string BuildShortDesc(const char * stoneName, Age age);
        static std::string BuildDescription(CHAR_DATA & ch, const char * stoneName, Age age);
};

#endif
