#include "Shades.h"
#include "merc.h"

Shades::ShadeGenInfo::ShadeGenInfo(unsigned int level, unsigned int hitDamRoll, unsigned int damDiceCount, 
                                   unsigned int damDiceSize, unsigned int damDiceBonus, unsigned int hp, unsigned int mana) :
    Level(level),
    HitDamRoll(hitDamRoll),
    DamDiceCount(damDiceCount),
    DamDiceSize(damDiceSize),
    DamDiceBonus(damDiceBonus),
    HP(hp),
    Mana(mana)
{}

const std::vector<Shades::DensityInfo> & Shades::BuildDensities()
{
    static std::vector<DensityInfo> result;
    result.push_back(DensityInfo(Teeming, "teeming",        100, 8));
    result.push_back(DensityInfo(Crowded, "crowded",         75, 6));
    result.push_back(DensityInfo(Normal, "normal",           50, 4));
    result.push_back(DensityInfo(Uncrowded, "uncrowded",     25, 2));
    result.push_back(DensityInfo(Empty, "empty",              0, 0));
    result.push_back(DensityInfo(DefaultDensity, "default",  50, 4));
    return result;
}

const std::vector<Shades::PowerInfo> & Shades::BuildPowers()
{
    static std::vector<PowerInfo> result;
    result.push_back(PowerInfo(Powerful, "powerful",    ShadeGenInfo(55, 30, 4, 50, 5, 900, 1000)));
    result.push_back(PowerInfo(Strong, "strong",        ShadeGenInfo(46, 24, 3, 50, 5, 750, 800)));
    result.push_back(PowerInfo(Average, "average",      ShadeGenInfo(37, 18, 3, 40, 5, 600, 600)));
    result.push_back(PowerInfo(Weak, "weak",            ShadeGenInfo(25, 12, 3, 30, 5, 450, 400)));
    result.push_back(PowerInfo(Powerless, "powerless",  ShadeGenInfo(16,  6, 2, 30, 5, 300, 200)));
    result.push_back(PowerInfo(DefaultPower, "default", ShadeGenInfo(37, 18, 3, 40, 5, 600, 600)));
    return result;
}

const std::vector<Shades::DensityInfo> & Shades::Densities()
{
    static const std::vector<DensityInfo> & result(BuildDensities());
    return result;
}

const std::vector<Shades::PowerInfo> & Shades::Powers()
{
    static const std::vector<PowerInfo> & result(BuildPowers());
    return result;
}

Shades::Density Shades::DensityFor(const char * name)
{
    for (size_t i(0); i < Densities().size() && i < MaxDensity; ++i)
    {
        if (!str_prefix(name, Densities()[i].name))
            return static_cast<Density>(i);
    }

    return DefaultDensity;
}

Shades::Power Shades::PowerFor(const char * name)
{
    for (size_t i(0); i < Powers().size() && i < MaxPower; ++i)
    {
        if (!str_prefix(name, Powers()[i].name))
            return static_cast<Power>(i);
    }

    return DefaultPower;
}

