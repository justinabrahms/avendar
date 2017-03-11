#ifndef SHADES_H
#define SHADES_H

#include <vector>

class Shades
{
    public:
        struct ShadeGenInfo
        {
            ShadeGenInfo(unsigned int level, unsigned int hitDamRoll, unsigned int damDiceCount, 
                         unsigned int damDiceSize, unsigned int damDiceBonus, unsigned int hp, unsigned int mana);

            unsigned int Level;
            unsigned int HitDamRoll;
            unsigned int DamDiceCount;
            unsigned int DamDiceSize;
            unsigned int DamDiceBonus;
            unsigned int HP;
            unsigned int Mana;
        };

        enum Density {Teeming, Crowded, Normal, Uncrowded, Empty, DefaultDensity, MaxDensity};
        enum Power {Powerful = 0, Strong = 1, Average = 2, Weak = 3, Powerless = 4, DefaultPower = 5, MaxPower = 6};
        static const Power HighestPower = Powerful;
        static const Power LowestPower = Powerless;

        static Density DensityFor(const char * name);
        static Power PowerFor(const char * name);
        static const char * NameFor(Density density);
        static const char * NameFor(Power power);
        static unsigned int RoomChanceFor(Density density);
        static unsigned int CountPer100RoomsFor(Density density);
        static const ShadeGenInfo & InfoFor(Power power);

    private:
        struct DensityInfo
        {
            DensityInfo(Density densityIn, const char * nameIn, unsigned int roomChanceIn, unsigned int countPer100RoomsIn);

            Density density;
            const char * name;
            unsigned int roomChance;
            unsigned int countPer100Rooms;
        };

        struct PowerInfo
        {
            PowerInfo(Power powerIn, const char * nameIn, const ShadeGenInfo & infoIn);

            Power power;
            const char * name;
            ShadeGenInfo info;
        };

        static const std::vector<DensityInfo> & BuildDensities();
        static const std::vector<DensityInfo> & Densities();
        static const std::vector<PowerInfo> & BuildPowers();
        static const std::vector<PowerInfo> & Powers();
};

inline Shades::DensityInfo::DensityInfo(Density densityIn, const char * nameIn, unsigned int roomChanceIn, unsigned int countPer100RoomsIn) : 
    density(densityIn), name(nameIn), roomChance(roomChanceIn), countPer100Rooms(countPer100RoomsIn) {}
inline Shades::PowerInfo::PowerInfo(Power powerIn, const char * nameIn, const ShadeGenInfo & infoIn) : power(powerIn), name(nameIn), info(infoIn) {}

inline const char * Shades::NameFor(Density density) {return Densities()[density].name;}
inline const char * Shades::NameFor(Power power) {return Powers()[power].name;}
inline unsigned int Shades::RoomChanceFor(Density density) {return Densities()[density].roomChance;}
inline unsigned int Shades::CountPer100RoomsFor(Density density) {return Densities()[density].countPer100Rooms;}
inline const Shades::ShadeGenInfo & Shades::InfoFor(Power power) {return Powers()[power].info;}

#endif
