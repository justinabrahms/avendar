#include "RomanNumerals.h"
#include <sstream>

std::string RomanNumerals::convertTo(unsigned int value)
{
    std::ostringstream result;

    value = writeModResult(result, value, "M", 1000);
    value = writeModResult(result, value, "CM", 900);
    value = writeModResult(result, value, "D", 500);
    value = writeModResult(result, value, "CD", 400);
    value = writeModResult(result, value, "C", 100);
    value = writeModResult(result, value, "XC", 90);
    value = writeModResult(result, value, "L", 50);
    value = writeModResult(result, value, "XL", 40);
    value = writeModResult(result, value, "X", 10);
    value = writeModResult(result, value, "IX", 9);
    value = writeModResult(result, value, "V", 5);
    value = writeModResult(result, value, "IV", 4);
    value = writeModResult(result, value, "I", 1);

    return result.str();
}

unsigned int RomanNumerals::writeModResult(std::ostream & out, unsigned int value, const char * str, unsigned int strValue)
{
    while (value >= strValue)
    {
        out << str;
        value -= strValue;
    }

    return value;
}
