#ifndef ENCUMBRANCE_H
#define ENCUMBRANCE_H

#include "merc.h"

class Encumbrance
{
    /// Constants
    static const unsigned int MaxCarryCount = 10000;;
    static const unsigned int MaxCarryWeight = 10000000;
    static const unsigned int SingleItemMax = 25;

    public:
        enum Level {None = 0, Light, Medium, Heavy, Max};

        class ChangeNotifier
        {
            public:
                explicit ChangeNotifier(CHAR_DATA & ch);
                ~ChangeNotifier();

            private:
                ChangeNotifier(const ChangeNotifier &);
                void operator=(const ChangeNotifier &);

                CHAR_DATA * m_ch;
                long m_id;
                Level m_level;
        };

        static const char * DescriptorFor(Level level);
        static Level LevelFor(const CHAR_DATA & ch);
        static unsigned int CarryCountCapacity(const CHAR_DATA & ch);
        static unsigned int CarryWeightCapacity(const CHAR_DATA & ch);

    private:
        static unsigned int CarryWeightCapacity(const CHAR_DATA & ch, int currStrength, unsigned int runeCount);
        static unsigned int CarryWeightThresholdFor(const CHAR_DATA & ch, Level level, unsigned int maxWeight, unsigned int runeCount);
        static bool WornArmorEncumbers(const CHAR_DATA & ch, int currStrength);
        static unsigned int WornWeight(const CHAR_DATA & ch, int wearSlot);
};

#endif
