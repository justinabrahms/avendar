#include "StatEmitter.h"
#include <sstream>

extern CHAR_DATA *load_offline_char(char *name);

const char * StatEmitter::StatFile("../gen/players.dat");

void StatEmitter::EmitStats()
{
    // Open the file
    std::ofstream fout(StatFile);
    if (!fout.is_open())
    {
        std::ostringstream mess;
        mess << "Unable to open stat file '" << StatFile << "'";
        bug(mess.str().c_str(), 0);
        return;
    }

    // Write the header
    EmitHeaderRow(fout);

    // Load every character in turn
    for (HEADER_DATA * header(g_active_headers); header != NULL; header = header->next)
    {
        // Check the online characters first
        bool foundOnline(false);
        for (DESCRIPTOR_DATA * d(descriptor_list); d != NULL; d = d->next)
        {
            CHAR_DATA * ch(d->original);
            if (ch == NULL)
            {
                ch = d->character;
                if (ch == NULL)
                    continue;
            }

            if (!str_cmp(ch->name, header->name))
            {
                EmitStatsFor(fout, *ch, foundOnline);
                foundOnline = true;
                break;
            }
        }

        if (foundOnline)
            continue;
        
        // Load the character
        CHAR_DATA * ch(load_offline_char(header->name));
        if (ch == NULL)
        {
            std::ostringstream mess;
            mess << "Unable to load character '" << header->name << "' for stats";
            bug(mess.str().c_str(), 0);
            continue;
        }

        // Emit the stats for the character
        EmitStatsFor(fout, *ch, foundOnline);
        extract_char(ch, true);
    }
}

void StatEmitter::EmitHeaderRow(std::ostream & out)
{
    out << "Name, Race, Class, Level, Align, Ethos, Hours Played, Hours Since Last Played\n";
}

void StatEmitter::EmitStatsFor(std::ostream & out, const CHAR_DATA & ch, bool online)
{
    out << ch.name << ", " << race_table[ch.race].name << ", " << class_table[ch.class_num].name << ", " << ch.level << ", ";
    
    if (IS_GOOD(const_cast<CHAR_DATA*>(&ch))) out << "good";
    else if (IS_EVIL(const_cast<CHAR_DATA*>(&ch))) out << "evil";
    else out << "neutral";
    
    out << ", ";
    if (IS_LAWFUL(const_cast<CHAR_DATA*>(&ch))) out << "lawful";
    else if (IS_CHAOTIC(const_cast<CHAR_DATA*>(&ch))) out << "chaotic";
    else out << "balanced";

    out << ", ";
    if (online) out << (ch.played / 3600) << ", 0\n";
    else out << ((ch.played + current_time - ch.logon) / 3600) << ", " << ((current_time - ch.laston) / 3600) << '\n';
}
