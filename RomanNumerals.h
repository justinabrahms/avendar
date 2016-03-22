#ifndef ROMANNUMERALS_H
#define ROMANNUMERALS_H

#include <string>
#include <ostream>

class RomanNumerals
{
    public:
        static std::string convertTo(unsigned int value);

    private:
        static unsigned int writeModResult(std::ostream & out, unsigned int value, const char * str, unsigned int strValue);
};

#endif
