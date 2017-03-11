#include "LeyGroup.h"
#include "merc.h"

const long LeyGroup::Unattuned;

const std::vector<Fount::Info> & Fount::BuildInfos()
{
    static std::vector<Info> infos;
    infos.push_back(Info(Always,        "always",       0,      4));
    infos.push_back(Info(Often,         "often",        0,      3));
    infos.push_back(Info(Occasionally,  "occasionally", 40,     2));
    infos.push_back(Info(Rarely,        "rarely",       80,     1));
    infos.push_back(Info(Never,         "never",        101,    0));
    infos.push_back(Info(Default,       "default",      40,     2));
    return infos;
}

const std::vector<Fount::Info> & Fount::Infos()
{
    static const std::vector<Fount::Info> & infos(BuildInfos());
    return infos;
}

Fount::Frequency Fount::ValueFor(const char * name)
{
    for (unsigned int i(0); i < Max && i < Infos().size(); ++i)
    {
        if (!str_prefix(name, Infos()[i].name))
            return static_cast<Frequency>(i);
    }

    return Default;
}

int LeyGroup::TotalOrderPower() const
{
    int total(m_fount.OrderPower);
    for (size_t i(0); i < m_infos.size(); ++i)
        total += m_infos[i].OrderPower;

   return total;
}

int LeyGroup::TotalPositivePower() const
{
    int total(m_fount.PositivePower);
    for (size_t i(0); i < m_infos.size(); ++i)
        total += m_infos[i].PositivePower;

    return total;
}

unsigned int LeyGroup::LineByID(const void * id) const
{
    for (size_t i(0); i < m_infos.size(); ++i)
    {
        if (m_infos[i].ID == id)
            return i;
    }

    return NotFound;
}

void LeyGroup::Remove(unsigned int index)
{
    m_infos[index] = m_infos[m_infos.size() - 1];
    m_infos.pop_back();
}
