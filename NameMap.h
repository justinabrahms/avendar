#include <map>
#include <set>
#include <string>
#include "StringUtil.h"

template <typename Type> class NameMap
{
    typedef std::multimap<std::string, Type*> MapType;

    public:
        /// Types
        class Filter
        {
            public:
                virtual bool shouldCount(Type * value) = 0;

            protected:
                ~Filter() {}
        };

        void Add(const char * name, Type * value);
        void Remove(const char * name, Type * value);
        Type * Lookup(const char * name, bool exactOnly, Filter * filter) const;

    private:
        // Members
        MapType m_map;
};

template <typename Type> void NameMap<Type>::Add(const char * name, Type * value)
{
    // Sanity-checks
    if (name == 0 || name[0] == '\0')
        return;

    // Break down the name into component parts and add each one to the map
    std::vector<std::string> names(splitArguments(name));
    for (size_t i(0); i < names.size(); ++i)
    {
        // Lookup the name
        typename MapType::iterator lower(m_map.lower_bound(names[i]));
        typename MapType::iterator upper(m_map.upper_bound(names[i]));
        
        // Iterate the range
        bool found(false);
        for (typename MapType::iterator iter(lower); iter != upper && iter != m_map.end(); ++iter)
        {
            if (!str_cmp(names[i].c_str(), iter->first.c_str()) && iter->second == value)
            {
                // Element already exists
                found = true;
                break;
            }
        }

        // Don't add the element if it already exists
        if (!found)
            m_map.insert(lower, std::make_pair(names[i], value));
    }
}

template <typename Type> void NameMap<Type>::Remove(const char * name, Type * value)
{
    // Sanity-checks
    if (name == 0 || name[0] == '\0')
        return;

    // Break down the name into component parts and remove each one from the map
    std::vector<std::string> names(splitArguments(name));
    for (size_t i(0); i < names.size(); ++i)
    {
        // Lookup the names
        typename MapType::iterator iter(m_map.lower_bound(names[i]));
        typename MapType::iterator upper(m_map.upper_bound(names[i]));
       
        // Iterate the range 
        while (iter != upper && iter != m_map.end())
        {
            // Advance the iterator
            typename MapType::iterator prev(iter);
            ++iter;

            // If the previous (pre-advancement) value was a match, erase it
            if (prev->second == value)
                m_map.erase(prev);
        }
    }
}

template <typename Type> 
Type * NameMap<Type>::Lookup(const char * name, bool exactOnly, Filter * filter) const
{
    // Break down any numerical prefix and sanity-check it 
    std::pair<int, std::string> values(number_argument(name));
    if (values.first <= 0)
        return 0;

    // Break down the name into component parts
    std::vector<std::string> names(splitArguments(values.second.c_str()));
    if (names.empty())
        return 0;

    // Prepare candidate sets
    std::set<Type *> candidates0;
    std::set<Type *> candidates1;
    std::set<Type *> * currCandidates(&candidates0);
    std::set<Type *> * nextCandidates(&candidates1);

    // Perform name-based lookup
    for (size_t i(0); i < names.size(); ++i)
    {
        // Iterate the range
        typename MapType::const_iterator upper(m_map.upper_bound(names[i]));
        for (typename MapType::const_iterator iter(m_map.lower_bound(names[i])); iter != upper; ++iter)
        {
            // Verify that this is a candidate; ignore those which aren't
            if (i != 0 && currCandidates->find(iter->second) == currCandidates->end())
                continue;

            if ((exactOnly && !str_cmp(names[i].c_str(), iter->first.c_str()))
            || !str_prefix(names[i].c_str(), iter->first.c_str()))
            {
                // Found a match, so verify the filter
                if (filter != NULL && !filter->shouldCount(iter->second))
                    continue;

                // This is valid, so add it to the next candidate list
                nextCandidates->insert(iter->second);
           }
        }

        // Swap out the candidate sets for the next run
        std::swap(currCandidates, nextCandidates);
        nextCandidates->clear();
    }

    // Now play out the counter
    int counter(0);
    for (typename std::set<Type *>::iterator iter(currCandidates->begin()); iter != currCandidates->end() && values.first > 0; ++iter)
    {
        ++counter;
        if (counter == values.first)
            return *iter;
    }
 
    // Found insufficient matches
    return 0;
}
