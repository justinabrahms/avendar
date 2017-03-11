#include "NameMaps.h"
#include "NameMap.h"

static NameMap<OBJ_DATA> s_objects;
static NameMap<CHAR_DATA> s_chars;

class CharObserverFilter : public NameMap<CHAR_DATA>::Filter
{
    public:
        explicit CharObserverFilter(CHAR_DATA & observer) : m_observer(&observer) {}
        virtual bool shouldCount(CHAR_DATA * observed) 
        {
            return (can_see(m_observer, observed) && observed->valid);
        }

    private:
        CHAR_DATA * m_observer;
};

class ObjObserverFilter : public NameMap<OBJ_DATA>::Filter
{
    public:
        explicit ObjObserverFilter(CHAR_DATA & observer) : m_observer(&observer) {}
        virtual bool shouldCount(OBJ_DATA * observed) 
        {
            return (can_see_obj(m_observer, observed) && observed->valid);
        }

    private:
        CHAR_DATA * m_observer;
};


void NameMaps::Add(OBJ_DATA & obj) {s_objects.Add(obj.name, &obj);}
void NameMaps::Remove(OBJ_DATA & obj) {s_objects.Remove(obj.name, &obj);}

void NameMaps::Add(CHAR_DATA & ch) 
{
    if (ch.name != NULL) s_chars.Add(ch.name, &ch);
    if (ch.fake_name != NULL) s_chars.Add(ch.fake_name, &ch);
    if (ch.unique_name != NULL) s_chars.Add(ch.unique_name, &ch);
}

void NameMaps::Remove(CHAR_DATA & ch) 
{
    if (ch.name != NULL) s_chars.Remove(ch.name, &ch);
    if (ch.fake_name != NULL) s_chars.Remove(ch.fake_name, &ch);
    if (ch.unique_name != NULL) s_chars.Remove(ch.unique_name, &ch);
}

OBJ_DATA * NameMaps::LookupObject(CHAR_DATA * ch, const char * name, bool exactOnly) 
{
    if (ch == NULL) return s_objects.Lookup(name, exactOnly, 0);
    ObjObserverFilter filter(*ch);
    return s_objects.Lookup(name, exactOnly, &filter);
}

CHAR_DATA * NameMaps::LookupChar(CHAR_DATA * ch, const char * name, bool exactOnly) 
{
    // Perform basic lookup
    CHAR_DATA * result;
    if (ch == NULL) 
        result = s_chars.Lookup(name, exactOnly, 0);
    else
    {
        CharObserverFilter filter(*ch);
        result = s_chars.Lookup(name, exactOnly, &filter);
    }

    if (result != NULL || exactOnly)
        return result;

    // Search the descriptors for a prefix; since we're only looking at PCs, numerical and multi-word arguments make no sense
    for (DESCRIPTOR_DATA * d = descriptor_list; d != NULL; d = d->next)
    {
        // Filter out certain characters
        if (d->connected != CON_PLAYING) continue;
        if (str_prefix(name, d->character->name) && str_prefix(name, d->character->fake_name)) continue;
        if (ch != NULL && !can_see(ch, d->character)) continue;
        return d->character;
    }

    return 0;
}

template <typename Type> static void setNameString(Type & value, char *& originalName, const char * newName)
{
    NameMaps::Remove(value);
    free_string(originalName);
    
    //if (newName == str_empty || originalName != NULL) originalName = str_empty;
    //else 
    originalName = str_dup(newName);
    NameMaps::Add(value);
}

void setName(CHAR_DATA & ch, const char * name)
{
    setNameString(ch, ch.name, name);
}

void setFakeName(CHAR_DATA & ch, const char * name)
{
    setNameString(ch, ch.fake_name, name);
}

void setUniqueName(CHAR_DATA & ch, const char * name)
{
    setNameString(ch, ch.unique_name, name);
}

void setName(OBJ_DATA & obj, const char * name)
{
    setNameString(obj, obj.name, name);
}
