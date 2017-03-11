#include "Oaths.h"
#include "Faction.h"
#include <sstream>
#include <fstream>

/// Global
void do_oath(CHAR_DATA * ch, char * argument)
{
    Oaths::DoOath(*ch, argument);
}

/// CharInfo
Oaths::CharInfo::CharInfo(const CHAR_DATA & ch) : m_id(ch.id), m_name(ch.name) {}

Oaths::CharInfo::CharInfo(std::istream & in)
{
    in >> m_id;
    while (in.peek() == ' ')
        in.get();

    std::getline(in, m_name, '~');
}

CHAR_DATA * Oaths::CharInfo::lookup() const {return get_char_by_id(m_id);}
bool Oaths::CharInfo::isMatch(const CHAR_DATA & ch) const {return (m_id == ch.id);}
const std::string & Oaths::CharInfo::name() const {return m_name;}
void Oaths::CharInfo::serialize(std::ostream & out) const {out << m_id << ' ' << m_name << '~';}
Oaths::CharInfo Oaths::CharInfo::deserialize(std::istream & in) {return CharInfo(in);}

/// Oath
Oaths::Oath::Oath(const CHAR_DATA & oathGiver, const CHAR_DATA & oathHolder, time_t expiration) :
    m_expiration(expiration), m_state(Normal), m_oathGiver(oathGiver), m_oathHolder(oathHolder)
{}

Oaths::Oath::Oath(std::istream & in) : 
    m_oathGiver(CharInfo::deserialize(in)), 
    m_oathHolder(CharInfo::deserialize(in))
{
    in >> m_expiration;

    unsigned int stateValue;
    in >> stateValue;
    if (stateValue >= Max) m_state = Normal;
    else m_state = static_cast<State>(stateValue);
}

Oaths::Oath::State Oaths::Oath::state() const {return m_state;}
bool Oaths::Oath::isExpired() const {return (m_expiration != Lifetime && current_time >= m_expiration);}
bool Oaths::Oath::isGiver(const std::string & giverName) const {return (!str_prefix(giverName.c_str(), m_oathGiver.name().c_str()));}
bool Oaths::Oath::isGiver(const CHAR_DATA & ch) const {return m_oathGiver.isMatch(ch);}
bool Oaths::Oath::isHolder(const CHAR_DATA & ch) const {return m_oathHolder.isMatch(ch);}
CHAR_DATA * Oaths::Oath::lookupGiver() const {return m_oathGiver.lookup();}
CHAR_DATA * Oaths::Oath::lookupHolder() const {return m_oathHolder.lookup();}
const std::string & Oaths::Oath::giverName() const {return m_oathGiver.name();}
const std::string & Oaths::Oath::holderName() const {return m_oathHolder.name();}
void Oaths::Oath::setState(State state) {m_state = state;}

void Oaths::Oath::setDaysLeft(int daysLeft)
{
    if (daysLeft == Lifetime) m_expiration = Lifetime;
    else m_expiration = current_time + (daysLeft * NUM_SECONDS_GAME_DAY);
}

void Oaths::Oath::shrinkDurationToMinimum()
{
    time_t newExpiration(current_time + (MinimumDuration * NUM_SECONDS_GAME_DAY * NUM_DAYS_YEAR));
    if (m_expiration == Lifetime) m_expiration = newExpiration;
    else m_expiration = UMIN(m_expiration, newExpiration);
}

int Oaths::Oath::daysLeft() const
{
    if (m_expiration == Lifetime) return Lifetime;
    return ((m_expiration - current_time) / NUM_SECONDS_GAME_DAY);
}

void Oaths::Oath::serialize(std::ostream & out) const
{
    m_oathGiver.serialize(out);
    m_oathHolder.serialize(out);
    out << ' ' << m_expiration;
    out << ' ' << m_state;
}

Oaths::Oath Oaths::Oath::deserialize(std::istream & in) {return Oath(in);}

/// OathLister
Oaths::OathLister::OathLister() :
    m_stateBox("Status", "------"),
    m_giverBox("Oathgiver", "---------"),
    m_holderBox("Oathholder", "----------"),
    m_daysBox("Days", "----")
{}

void Oaths::OathLister::add(const Oath & oath)
{
    // Fill out the state
    switch (oath.state())
    {
        default:
        case Oath::Normal:   m_stateBox.AddLine("{WPledged{x"); break;
        case Oath::Broken:   m_stateBox.AddLine("{RBroken{x"); break;
        case Oath::Released: m_stateBox.AddLine("{GReleased{x"); break;
    }

    // Fill out name and days left
    m_giverBox.AddLine("{W" + oath.giverName() + "{x");
    m_holderBox.AddLine("{W" + oath.holderName() + "{x");

    int daysLeft(oath.daysLeft());
    if (daysLeft == Lifetime) m_daysBox.AddLine("{WLifetime{x");
    else m_daysBox.AddLine("{W" + makeString(oath.daysLeft()) + "{x");
}

std::string Oaths::OathLister::render() const
{
    // Compose and render the list
    DisplayPanel::HorizontalSplit mainPanel(DisplayPanel::Options(DisplayPanel::Style_None));
    mainPanel.Add(m_stateBox);
    mainPanel.Add(m_giverBox);
    mainPanel.Add(m_holderBox);
    mainPanel.Add(m_daysBox);
    return DisplayPanel::Render(mainPanel);
}

/// Oaths
const time_t Oaths::Lifetime;
std::vector<Oaths::Oath> Oaths::s_oaths;

void Oaths::DoOath(CHAR_DATA & ch, const char * argument)
{
    // Verify PC ch
    if (IS_NPC(&ch))
    {
        send_to_char("Only adventurers may deal in oaths.\n", &ch);
        return;
    }

    // Get the arguments
    std::vector<std::string> arguments(splitArguments(argument));
    if (arguments.empty())
    {
        // No argument supplied, so show the syntax
        ListSyntax(ch);
        return;
    }

    // Dispatch according to the first argument
    if (!str_prefix(arguments[0].c_str(), "list")) ListOaths(ch, arguments);
    else if (!str_prefix(arguments[0].c_str(), "give")) GiveOath(ch, arguments);
    else if (!str_prefix(arguments[0].c_str(), "release")) ReleaseOath(ch, arguments);
    else if (!str_prefix(arguments[0].c_str(), "broken")) BrokenOath(ch, arguments);
    else if (IS_IMMORTAL(&ch) && !str_prefix(arguments[0].c_str(), "set")) SetOath(ch, arguments);
    else ListSyntax(ch);
}

void Oaths::ListSyntax(CHAR_DATA & ch)
{
    send_to_char("Syntax:\n", &ch);
    send_to_char("{Woath list{x: lists all active oaths\n", &ch);
    send_to_char("{Woath give <name> [<duration>|lifetime]{x: makes an oath to the specified player\n", &ch);
    send_to_char("{Woath release <name>{x: releases an oathgiver from the oath without penalty\n", &ch);
    send_to_char("{Woath broken <name>{x: declares an oath to be broken, penalizing the oathgiver\n", &ch);

    if (IS_IMMORTAL(&ch))
    {
        send_to_char("\nImmortal-only:\n", &ch);
        send_to_char("{Woath list <name>{x: used like oath list but displays for the specified online PC\n", &ch);
        send_to_char("{Woath list all{x: displays all active oaths\n", &ch);
        send_to_char("{Woath set <giver_name> <field> <value>{x: sets oath values; use 'oath set' for syntax\n", &ch);
    }
}

void Oaths::ListSetSyntax(CHAR_DATA & ch)
{
    send_to_char("Syntax:\n", &ch);
    send_to_char("{Woath set <giver_name> state <pledged|broken|released>{x: sets the oath's state\n", &ch);
    send_to_char("{Woath set <giver_name> state none{x: removes the oath altogether\n", &ch);
    send_to_char("{Woath set <giver_name> days <#>{x: sets the oath's duration in game days\n", &ch);
    send_to_char("{Woath set <giver_name> days minimum{x: sets the oath's duration to the normal minimum\n", &ch);
    send_to_char("{Woath set <giver_name> days lifetime{x: sets the oath's duration to lifetime\n", &ch);
}

void Oaths::ListAllOaths(CHAR_DATA & ch)
{
    // Check for empty
    if (s_oaths.empty())
    {
        send_to_char("There are no oaths in existence.\n", &ch);
        return;
    }

    // Build and render the oath list
    OathLister lister;
    for (size_t i(0); i < s_oaths.size(); ++i)
        lister.add(s_oaths[i]);

    send_to_char(lister.render().c_str(), &ch);
}

void Oaths::ListOaths(CHAR_DATA & ch, const std::vector<std::string> & arguments)
{
    // Resolve the target
    CHAR_DATA * victim(&ch);
    if (arguments.size() > 1 && IS_IMMORTAL(&ch))
    {
        // Check for 'all'
        if (!str_prefix(arguments[1].c_str(), "all"))
        {
            ListAllOaths(ch);
            return;
        }

        // Imm supplied different target, so look it up
        victim = get_char_world(&ch, arguments[1].c_str());
        if (victim == NULL)
        {
            send_to_char("You see no one by that name in the world.\n", &ch);
            return;
        }

        // Verify PC
        if (IS_NPC(victim))
        {
            send_to_char("Only PCs may hold oaths.\n", &ch);
            return;
        }
    }

    // Lookup the oaths
    Oath * oath(OathFrom(*victim));
    std::vector<Oath*> oaths(OathsTo(*victim));

    // Target is resolved, so check for oaths
    if (oath == NULL && oaths.empty())
    {
        if (victim == &ch) send_to_char("You have no active oaths to list.\n", &ch);
        else act("$N has no active oaths to list.", &ch, NULL, victim, TO_CHAR);
        return;
    }

    // Build and render the oath list
    OathLister lister;
    if (oath != NULL) 
        lister.add(*oath);

    for (size_t i(0); i < oaths.size(); ++i)
        lister.add(*oaths[i]);

    send_to_char(lister.render().c_str(), &ch);
}

void Oaths::SetOath(CHAR_DATA & ch, const std::vector<std::string> & arguments)
{
    // Check for sufficient arguments
    if (arguments.size() < 4)
    {
        ListSetSyntax(ch);
        return;
    }

    // Get the oath in question
    size_t index;
    Oath * oath(OathFrom(arguments[1], index));
    if (oath == NULL)
    {
       send_to_char("There is no active oath given by such a person.\n", &ch);
       return;
    }

    // Dispatch according to field
    if (!str_prefix(arguments[2].c_str(), "state")) SetOathState(ch, *oath, index, arguments[3]);
    else if (!str_prefix(arguments[2].c_str(), "days")) SetOathDays(ch, *oath, arguments[3]);
    else ListSetSyntax(ch);
}

void Oaths::SetOathState(CHAR_DATA & ch, Oath & oath, size_t index, const std::string & value)
{
    struct StateSetter
    {
        static bool CheckSet(CHAR_DATA & ch, Oath & oath, const std::string & value, const char * stateName, Oath::State state)
        {
            if (!str_prefix(value.c_str(), stateName))
            {
                oath.setState(state);
                std::ostringstream mess;
                mess << "Oath state set to " << stateName << ".\n";
                send_to_char(mess.str().c_str(), &ch);
                SaveOaths();
                return true;
            }
            return false;
        }
    };

    // Check for basic states
    if (StateSetter::CheckSet(ch, oath, value, "pledged", Oath::Normal)) return;
    if (StateSetter::CheckSet(ch, oath, value, "released", Oath::Released)) return;
    if (StateSetter::CheckSet(ch, oath, value, "broken", Oath::Broken)) return;

    // Check for removal
    if (!str_prefix(value.c_str(), "none"))
    {
        RemoveOath(index);
        SaveOaths();
        send_to_char("Oath removed.\n", &ch);
        return;
    }
    
    // Unrecognized state type, so show syntax
    ListSetSyntax(ch);
}

void Oaths::SetOathDays(CHAR_DATA & ch, Oath & oath, const std::string & value)
{
    // Check for lifetime
    if (!str_prefix(value.c_str(), "lifetime"))
    {
        oath.setDaysLeft(Lifetime);
        send_to_char("Oath duration set to lifetime.\n", &ch);
        SaveOaths();
        return;
    }

    // Check for minimum
    int duration;
    if (!str_prefix(value.c_str(), "minimum"))
        duration = MinimumDuration * NUM_DAYS_YEAR;
    else
    {
        // Read a number
        duration = atoi(value.c_str());
        if (duration < 0)
        {
            send_to_char("Oath duration cannot be negative.\n", &ch);
            return;
        }
    }

    // Set the duration
    oath.setDaysLeft(duration);
    SaveOaths();

    std::ostringstream mess;
    mess << "Oath duration set to " << duration << " day" << (duration == 1 ? "" : "s") << ".\n";
    send_to_char(mess.str().c_str(), &ch);
}

void Oaths::GiveOath(CHAR_DATA & ch, const std::vector<std::string> & arguments)
{
    // Verify the arguments
    if (arguments.size() < 2)
    {
        ListSyntax(ch);
        return;
    }

    // Make sure there is no outstanding oath from this character
    Oath * oath(OathFrom(ch));
    if (oath != NULL)
    {
        int daysLeft(oath->daysLeft());

        std::ostringstream mess;
        mess << "You have " << ((oath->state() == Oath::Broken) ? "broken" : "pledged") << " your";
        mess << ((daysLeft == Lifetime) ? " life-long" : "") << " oath to " << oath->holderName();
        mess << ", and cannot pledge again";
        if (daysLeft != Lifetime)
            mess << " for " << daysLeft << " more day" << ((daysLeft == 1) ? "" : "s");

        mess << ".\n";
        send_to_char(mess.str().c_str(), &ch);
        return;
    }

    // Lookup the target
    CHAR_DATA * victim(get_char_room(&ch, arguments[1].c_str()));
    if (victim == NULL)
    {
        send_to_char("You see no one by that name here.\n", &ch);
        return;
    }

    if (IS_NPC(victim))
    {
        send_to_char("You may only give your oath to another adventurer.\n", &ch);
        return;
    }

    if (!IS_SET(victim->nact, PLR_AUTOOATH))
    {
        act("$N is not accepting oaths at this time.", &ch, NULL, victim, TO_CHAR);
        act("$n offers you $s oath of fealthy, but you decline.", &ch, NULL, victim, TO_VICT);
        return;
    }

    // Target acquired, determine duration
    int duration(MinimumDuration);
    if (arguments.size() >= 3)
    {
        // Check for lifetime
        if (!str_prefix(arguments[2].c_str(), "lifetime")) 
            duration = Lifetime;
        else
        {
            // Obtain and validate the numerical duration
            duration = atoi(arguments[2].c_str());
            if (duration < MinimumDuration)
            {
                // Duration not acceptable
                std::ostringstream mess;
                mess << "The minimum duration of such an oath is " << MinimumDuration << " years.\n";
                send_to_char(mess.str().c_str(), &ch);
                return;
            }
        }
    }

    // Determine the expiration from the duration
    time_t expiration;
    if (duration == Lifetime) expiration = Lifetime;
    else expiration = current_time + (duration * NUM_SECONDS_GAME_DAY * NUM_DAYS_YEAR);

    // Make the oath
    act("You pledge yourself to $N, granting $M your oath of fealty!", &ch, NULL, victim, TO_CHAR);
    act("$n pledges $mself to you, granting you $s oath of fealty!", &ch, NULL, victim, TO_VICT);
    act("$n pledges $mself to $N, granting $M $s oath of fealty!", &ch, NULL, victim, TO_NOTVICT);
    s_oaths.push_back(Oath(ch, *victim, expiration));
    SaveOaths();
}

void Oaths::ReleaseOath(CHAR_DATA & ch, const std::vector<std::string> & arguments)
{
    // Verify the arguments
    if (arguments.size() < 2)
    {
        ListSyntax(ch);
        return;
    }

    // Look up the target
    CHAR_DATA * victim(get_char_world(&ch, arguments[1].c_str()));
    if (victim == NULL)
    {
        send_to_char("You sense no one by that name currently in the world.\n", &ch);
        return;
    }

    // Look up the oath
    size_t index;
    Oath * oath(OathFrom(*victim, index));
    if (oath == NULL || !oath->isHolder(ch))
    {
        act("$N is not beholden to you.", &ch, NULL, victim, TO_CHAR);
        return;
    }

    // Check the oath state
    switch (oath->state())
    {
        case Oath::Broken: act("$N broke $S oath to you, and cannot be released from the consequences.", &ch, NULL, victim, TO_CHAR); return;
        case Oath::Released: act("You have already released $N from $S oath to you.", &ch, NULL, victim, TO_CHAR); return;
        default: break;
    }

    // Release the oath
    act("You release $N from $S oath to you.", &ch, NULL, victim, TO_CHAR);
    act("$n releases you from your oath to $m.", &ch, NULL, victim, TO_VICT);
    oath->shrinkDurationToMinimum();
    oath->setState(Oath::Released);
    SaveOaths();
}

void Oaths::BrokenOath(CHAR_DATA & ch, const std::vector<std::string> & arguments)
{
    // Verify the arguments
    if (arguments.size() < 2)
    {
        ListSyntax(ch);
        return;
    }

    // Look up the target
    CHAR_DATA * victim(get_char_world(&ch, arguments[1].c_str()));
    if (victim == NULL)
    {
        send_to_char("You sense no one by that name currently in the world.\n", &ch);
        return;
    }

    // Look up the oath
    size_t index;
    Oath * oath(OathFrom(*victim, index));
    if (oath == NULL || !oath->isHolder(ch))
    {
        act("$N is not beholden to you.", &ch, NULL, victim, TO_CHAR);
        return;
    }

    // Check the oath state
    switch (oath->state())
    {
        case Oath::Broken: act("$N is already suffering the consequences of $S broken oath.", &ch, NULL, victim, TO_CHAR); return;
        case Oath::Released: act("You have already released $N from $S oath to you.", &ch, NULL, victim, TO_CHAR); return;
        default: break;
    }

    // Release the oath
    act("You declare $N an oathbreaker, decrying $M to the gods themselves!", &ch, NULL, victim, TO_CHAR);
    act("$n declares you an oathbreaker, decrying you to the gods themselves!", &ch, NULL, victim, TO_VICT);
    oath->setState(Oath::Broken);
    SaveOaths();

    // Hurt the oathgiver's faction standings for any faction allied to the oathholder
    for (size_t i(0); i < FactionTable::Instance().Count(); ++i)
    {
        if (FactionTable::CurrentStanding(ch, i) == Rating_Friend)
            victim->pcdata->faction_standing->Change(*victim, i, -1000, 0, 0, true);
    }
}

template <typename Type> Oaths::Oath * Oaths::OathFrom(const Type & ch)
{
    size_t index;
    return OathFrom(ch, index);
}

template <typename Type> Oaths::Oath * Oaths::OathFrom(const Type & ch, size_t & index)
{
    for (index = 0; index < s_oaths.size(); ++index)
    {
        if (s_oaths[index].isGiver(ch))
            return &s_oaths[index];
    }

    return NULL;
}

std::vector<Oaths::Oath*> Oaths::OathsTo(const CHAR_DATA & ch)
{
    std::vector<Oath*> result;
    for (size_t i(0); i < s_oaths.size(); ++i)
    {
        if (s_oaths[i].isHolder(ch))
            result.push_back(&s_oaths[i]);
    }
    return result;
}

void Oaths::SaveOaths()
{
    // Open the output file
    std::ofstream fout(OATHS_FILE);
    if (!fout.is_open())
    {
        bug("Unable to open oaths file for writing", 0);
        return;
    }

    // Write the oaths
    for (size_t i(0); i < s_oaths.size(); ++i)
    {
        if (i != 0) fout << '\n';
        s_oaths[i].serialize(fout);
    }
}

bool Oaths::LoadOaths()
{
    // Open the input file
    std::ifstream fin(OATHS_FILE);
    if (!fin.is_open())
    {
        bug("Unable to open oaths file for reading", 0);
        return true;
    }

    // Read the oaths
    while (!fin.eof())
    {
        s_oaths.push_back(Oath::deserialize(fin));
        if (fin.fail())
        {
            bug("Error while reading oaths file", 0);
            return false;
        }
    }

    return true;
}

void Oaths::RemoveOath(size_t index)
{
    s_oaths[index] = s_oaths[s_oaths.size() - 1];
    s_oaths.pop_back();
}

void Oaths::UpdateOaths()
{
    // Iterate the oaths, looking for oaths to clear
    bool changed(false);
    size_t i(0);
    while (i < s_oaths.size())
    {
        // Check for expired oaths
        if (!s_oaths[i].isExpired())
        {
            // Not expired, just continue
            ++i;
            continue;
        }

        // Oath is expired
        CHAR_DATA * oathGiver(s_oaths[i].lookupGiver());
        if (oathGiver != NULL)
        {
            std::ostringstream mess;
            mess << "Your oath to " << s_oaths[i].holderName() << " has run its course, binding you no longer.\n";
            send_to_char(mess.str().c_str(), oathGiver);
        }

        CHAR_DATA * oathHolder(s_oaths[i].lookupHolder());
        if (oathHolder != NULL)
        {
            std::ostringstream mess;
            mess << s_oaths[i].giverName() << "'s oath to you has run its course, binding no longer.\n";
            send_to_char(mess.str().c_str(), oathHolder);
        }

        // Remove the oath
        RemoveOath(i);
        changed = true;
    }

    // Save off the oaths if changed
    if (changed)
        SaveOaths();
}

bool Oaths::IsOathBreaker(const CHAR_DATA & ch)
{
    // This check is not strictly necessary, but is included for performance
    if (IS_NPC(&ch))
        return false;

    // Lookup any oaths from this character
    Oath * oath(OathFrom(ch));
    return (oath != NULL && oath->state() == Oath::Broken);
}

CHAR_DATA * Oaths::OathHolderFor(const CHAR_DATA & ch)
{
    // This check is not strictly necessary, but is included for performance
    if (IS_NPC(&ch))
        return NULL;

    // Lookup any oaths from this character
    Oath * oath(OathFrom(ch));
    if (oath == NULL || oath->state() != Oath::Normal)
        return NULL;

    return (oath->lookupHolder());
}
