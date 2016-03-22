#ifndef PROG_CONDITIONALS_H
#define PROG_CONDITIONALS_H

#include <map>
#include <string>
#include "merc.h"

class ProgConditionals
{
    public:
        /// Types
        struct Context
        {
            PROG_RUNDATA * prog;
            CHAR_DATA * target;
            CHAR_DATA * vict;
            CHAR_DATA * rhstarg;
            char * txt;
            char * val;
            char * opr;
            char * buf;
            char * arg;
            char * proctxt;
            char * checktxt;
        };

        /// Returns:     0: false
        ///             >0: true
        ///             -1: error
        typedef int (*ConditionalFun)(Context & context);

        static ConditionalFun Lookup(const char * condition);

    private:
        struct StringComparator
        {
            bool operator()(const char * lhs, const char * rhs) const
            {
                unsigned int i(0);
                while (true)
                {
                    // Note that there is no need to check for rhs NULL terminator, it is handled implicitly
                    if (lhs[i] != rhs[i]) return lhs[i] < rhs[i];
                    if (lhs[i] == '\0') return false;
                    ++i;
                }
            }
        };

        typedef std::map<const char *, ProgConditionals::ConditionalFun, StringComparator> MapType;
        static const MapType & BuildConditions();
};

#endif
