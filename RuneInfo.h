#ifndef RUNEINFO_H
#define RUNEINFO_H

#include <string>

class Rune
{
    public:
        enum Type
        {
            Still = 0,  Stone,      Ground,
            Bind,       Strength,   Truth,
            Sunder,     Order,      Fossil,
            Tremor,     Calcify,    Gravitas,
            Erosion,    Pulverize,  Adamantine,
            Empowerment, Max
        };

        Rune(Type typeIn, unsigned int learnedBitIn, unsigned int syllableCountIn, const std::string & nameIn);

        Type type;
        unsigned int learnedBit;
        unsigned int syllableCount;
        std::string name;
};

class RuneTable
{
    public:
        static const Rune * Lookup(const char * name);
        static const Rune * Lookup(Rune::Type type);
};

#endif
