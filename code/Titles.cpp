#include "Titles.h"
#include "AirTitles.h"
#include "EarthTitles.h"
#include "FireTitles.h"
#include "SpiritTitles.h"
#include "WaterTitles.h"
#include "VoidTitles.h"
#include <sstream>

const char * Titles::LookupTitle(const CHAR_DATA & ch)
{
    // Handle special classes
    if (ch.class_num == global_int_class_airscholar)    return AirTitles::LookupTitle(ch);
    if (ch.class_num == global_int_class_earthscholar)  return EarthTitles::LookupTitle(ch);
    if (ch.class_num == global_int_class_firescholar)   return FireTitles::LookupTitle(ch);
    if (ch.class_num == global_int_class_spiritscholar) return SpiritTitles::LookupTitle(ch);
    if (ch.class_num == global_int_class_waterscholar)  return WaterTitles::LookupTitle(ch);
    if (ch.class_num == global_int_class_voidscholar)   return VoidTitles::LookupTitle(ch);

    // Handle the rest of the classes
    const char * result(title_table[ch.class_num][ch.level][ch.sex == SEX_FEMALE ? 1 : 0]);
    if (result != NULL)
        return result;

    // Handle an error condition
    std::ostringstream mess;
    mess << "Null title returned for '" << ch.name << "'; level is " << ch.level << ", gender is " << ch.sex;
    bug(mess.str().c_str(), 0);
    return "the Adventurer";
}
