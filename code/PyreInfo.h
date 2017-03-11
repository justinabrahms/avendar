#ifndef PYREINFO_H
#define PYREINFO_H

#include "merc.h"
#include <vector>
#include <string>
#include <utility>

class PyreInfo
{
    // Constants
    static const unsigned int Req_None = static_cast<unsigned int>(-1);

    static const unsigned int Bit_None                      = 0;
    static const unsigned int Bit_NoMoreHP                  = 248;
    static const unsigned int Bit_NoMoreMana                = 249;
    static const unsigned int Bit_MergedWithInferno         = 250;
    static const unsigned int Bit_EatenHeartOfKzroth        = 251;
    static const unsigned int Bit_SacrificedHeartOfInferno  = 252;
    static const unsigned int Bit_StudiedWhipRune           = 253;
    static const unsigned int Bit_DefeatedDedicant          = 254;
    static const unsigned int Bit_ReadGaaldScroll           = 255;
    static const unsigned int Bit_XiganathAchievement       = 256;
    static const unsigned int Bit_ReadEarendamScroll        = 257;
    static const unsigned int Bit_HeldPhoenixFeather        = 258;
    static const unsigned int Bit_EatenPhoenixEgg           = 259;
    static const unsigned int Bit_StudiedLizardSkull        = 260;
    static const unsigned int Bit_ReadGogothScroll          = 261;
    static const unsigned int Bit_SprinkledConsumePowder    = 262;
    static const unsigned int Bit_StaredGiantRuby           = 263;
    static const unsigned int Bit_OajmaQuestCompleted       = 264;
    static const unsigned int Bit_ForbiddenBooksCompleted   = 265;
    static const unsigned int Bit_SsesarykCompleted         = 266;

    // Types
    typedef void (PyreInfo::*ContinueFun)(CHAR_DATA*, OBJ_DATA*, bool&);
    enum Color {Orange = 0, Red = 1, Blue = 2, White = 3};
    enum PersistentSpellState {Spell_None, Spell_Building, Spell_Active};

    public:
        static const unsigned int Heatlash_Modifier_Earth = 0;
        static const unsigned int Heatlash_Modifier_Fire = 1;

        enum Effect
        {
            Effect_None = 0,                Effect_EssenceOfTheInferno,
            Effect_WreathOfFlame,           Effect_BrimstoneConduit,
            Effect_BayyalsRealm,            Effect_Heatlash,
            Effect_FuryOfAdaChemtaBoghor,   Effect_Max
        };

        explicit PyreInfo(OBJ_DATA * pyre);

        bool incinerateObject(CHAR_DATA * ch, OBJ_DATA * obj);
        void checkCooldownTime(CHAR_DATA * ch);
        Effect effect() const;
        bool isEffectGreater() const;
        unsigned int effectModifier() const;
        void setEffectModifier(unsigned int modifier);
        OBJ_DATA * pyre() const;

        static std::string listKnownEffects(CHAR_DATA * ch);
        static const char * effectName(Effect effectValue, bool capital = false);
        static void checkHeartSacrifice(CHAR_DATA * ch, OBJ_DATA * obj, AFFECT_DATA * heartAff);

    private:
        // Members
        OBJ_DATA * m_pyre;
        Color m_color;
        ContinueFun m_continueFun;
        unsigned int m_step;
        time_t m_lastIncineration;
        bool m_currSpellGreater;
        std::vector<std::string> m_lastOwners;

        Effect m_activePersistentSpell;
        bool m_activePersistentSpellGreater;
        unsigned int m_effectModifier;

        // Implementation
        void handleOrange(CHAR_DATA * ch, OBJ_DATA * obj);
        void handleRed(CHAR_DATA * ch, OBJ_DATA * obj);
        void handleBlue(CHAR_DATA * ch, OBJ_DATA * obj);
        void handleWhite(CHAR_DATA * ch, OBJ_DATA * obj);
        void handleCorrect(CHAR_DATA * ch, OBJ_DATA * obj);
        void handleIncorrect(CHAR_DATA * ch, OBJ_DATA * obj);
        void handleSetColor(CHAR_DATA * ch, OBJ_DATA * obj, Color color, const char * colorName);
        void rawSetColor(CHAR_DATA * ch, Color color, const char * colorName);
        void reset();
        bool checkStart(CHAR_DATA * ch, OBJ_DATA * obj, bool persistent, unsigned int lesserPrereqBit, unsigned int greaterPrereqBit, 
                        ContinueFun continueFun, const char * materialName, 
                        unsigned int itemType, unsigned int vnum, unsigned int minLevel);
        bool checkContinue(CHAR_DATA * ch, OBJ_DATA * obj, const char * materialName, unsigned int itemType, unsigned int vnum, unsigned int minLevel);
        bool checkFinishPersistent(CHAR_DATA * ch, OBJ_DATA * obj, Effect effect, const char * materialName, unsigned int itemType, unsigned int vnum, unsigned int minLevel);

        static void addEffectIfKnown(std::ostringstream & result, CHAR_DATA * ch, unsigned int lesserBit, unsigned int greaterBit, const char * name);
        static void sendToArea(CHAR_DATA * ch, const char * message, bool excludeRoom = false);
        static bool checkMatch(OBJ_DATA * obj, const char * materialName, unsigned int itemType, unsigned int vnum, unsigned int minLevel);
        static bool testBit(CHAR_DATA * ch, unsigned int num);
        static void setBit(CHAR_DATA * ch, unsigned int num);
        static bool canTransport(ROOM_INDEX_DATA * room);

        // Persistent spells
        void Proc_EssenceOfTheInferno(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_WreathOfFlames(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_FuryOfAdaChemtaBoghor(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_BrimstoneConduit(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_BayyalsRealm(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_Heatlash(CHAR_DATA * ch, OBJ_DATA * obj, bool &);

        // Instantaneous spells
        void Proc_HungerOfTheFlame(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_Fireflies(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_InfernosReach(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_SummonSalamander(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_TrailOfEmbers(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_Steelscald(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_FlickerFyre(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_Heatmine(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_Hearthglow(CHAR_DATA * ch, OBJ_DATA * obj, bool & destroyObject);
        void Proc_Flamekiss(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
        void Proc_Flamecall(CHAR_DATA * ch, OBJ_DATA * obj, bool &);
};

inline OBJ_DATA * PyreInfo::pyre() const {return m_pyre;}
inline PyreInfo::Effect PyreInfo::effect() const {return m_activePersistentSpell;}
inline bool PyreInfo::isEffectGreater() const {return m_activePersistentSpellGreater;}
inline unsigned int PyreInfo::effectModifier() const {return m_effectModifier;}
inline void PyreInfo::setEffectModifier(unsigned int modifier) {m_effectModifier = modifier;}

#endif
