#include "RuneInfo.h"
#include "StringUtil.h"
#include <vector>

Rune::Rune(Type typeIn, unsigned int learnedBitIn, unsigned int syllableCountIn, const std::string & nameIn) :
    type(typeIn), learnedBit(learnedBitIn), syllableCount(syllableCountIn), name(nameIn) {}

static const std::vector<Rune> & BuildRunes()
{
    static const unsigned int FirstBit(281);
    static std::vector<Rune> runes;
    unsigned int currBit(FirstBit);

    // This order must match the enum order, or else Lookup may behave incorrectly
    #define ADD_RUNE(name, syllables) runes.push_back(Rune((Rune::name), (currBit++), (syllables), (#name)))
    ADD_RUNE(Still, 1);
    ADD_RUNE(Stone, 1);
    ADD_RUNE(Ground, 1);
    ADD_RUNE(Bind, 1);
    ADD_RUNE(Strength, 1);
    ADD_RUNE(Truth, 1);
    ADD_RUNE(Sunder, 2);
    ADD_RUNE(Order, 2);
    ADD_RUNE(Fossil, 2);
    ADD_RUNE(Tremor, 2);
    ADD_RUNE(Calcify, 3);
    ADD_RUNE(Gravitas, 3);
    ADD_RUNE(Erosion, 3);
    ADD_RUNE(Pulverize, 3);
    ADD_RUNE(Adamantine, 4);
    ADD_RUNE(Empowerment, 4);
    #undef ADD_RUNE

    return runes;
}

static const std::vector<Rune> & GetRunes()
{
    static const std::vector<Rune> & runes(BuildRunes());
    return runes;
}

const Rune * RuneTable::Lookup(const char * name)
{
    const std::vector<Rune> & runes(GetRunes());
    for (size_t i(0); i < runes.size(); ++i)
    {
        if (!str_prefix(name, runes[i].name.c_str()))
            return &runes[i];
    }

    return 0;
}

const Rune * RuneTable::Lookup(Rune::Type type)
{
    if (static_cast<unsigned int>(type) >= static_cast<unsigned int>(Rune::Max))
        return NULL;

    return &GetRunes()[type];
}
