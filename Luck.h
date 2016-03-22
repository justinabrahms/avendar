#ifndef LUCK_H
#define LUCK_H

#include "merc.h"

class Luck
{
    public:
        enum Result {Lucky, Normal, Unlucky};
        static Result Check(const CHAR_DATA & ch);
        static Result CheckOpposed(const CHAR_DATA & ch, const CHAR_DATA & opponent);
        static int LuckFor(const CHAR_DATA & ch);

    private:
        static Result Check(int modifier);
};

#endif
