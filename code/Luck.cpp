#include "Luck.h"
#include "Oaths.h"

Luck::Result Luck::Check(int modifier)
{
    int roll(number_range(0, 1000) + modifier);
    if (roll <= 20) return Unlucky;
    if (roll >= 980) return Lucky;
    return Normal;
}

Luck::Result Luck::Check(const CHAR_DATA & ch)
{
    return Check(LuckFor(ch));
}

Luck::Result Luck::CheckOpposed(const CHAR_DATA & ch, const CHAR_DATA & opponent)
{
    return Check(LuckFor(ch) - LuckFor(opponent));
}

int Luck::LuckFor(const CHAR_DATA & ch)
{
    // Get base luck, modify by charisma
    int result(ch.luck);
    result += (get_curr_stat(&ch, STAT_CHR) - 18);
    
    // Modify for oaths
    if (Oaths::IsOathBreaker(ch)) 
        result -= 25;
    else
    {
        CHAR_DATA * oathHolder(Oaths::OathHolderFor(ch));
        if (oathHolder != NULL && oathHolder->in_room == ch.in_room)
            ++result;
    }

    return result;
}
