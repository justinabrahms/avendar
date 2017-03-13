#include "StringUtil.h"
#include <cassert>
#include <sstream>
#include <ctype.h>
#include <cstring>
#include <cstdlib>

// Externs which need to be moved elsewhere at some point
extern char * str_dup(const char *str);
extern void free_string(char *&pstr);

static char s_arg_first_default[MAX_STRING_LENGTH];

bool is_whitespace(char letter)
{
    switch (letter)
    {
        case ' ':
        case '\n':
        case '\r':
        case '\t':
            return true;
    }

    return false;
}

const char * trimFront(const char * line)
{
    while (is_whitespace(*line))
        ++line;

    return line;
}

std::string trimBack(const char * line, unsigned int length)
{
    while (length > 0 && is_whitespace(line[length - 1]))
        --length;

    return std::string(line, length);
}

std::string trim(const char * line, unsigned int length)
{
    const char * begin(trimFront(line));
    return trimBack(begin, length - (begin - line));
}

std::string trim(const std::string & line)
{
    return trim(line.c_str(), line.size());
}

void copy_string(char *& dest, const char * src)
{
    free_string(dest);
    dest = (src == NULL ? NULL : str_dup(src));
}

// Picks off one argument from a string and return the rest; understands quotes
char *one_argument(char *argument, char *arg_first, bool keepcase)
{
    if (arg_first == 0)
        arg_first = s_arg_first_default;

    char cEnd;

    while ( isspace(*argument) )
        argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;

    while ( *argument != '\0' && *argument != '\n' && *argument != '\r')
    {
        if ( *argument == cEnd )
        {
            argument++;
            break;
        }
        *arg_first = (keepcase ? *argument : LOWER(*argument));
        arg_first++;
        argument++;
    }

    *arg_first = '\0';
    while (isspace(*argument))
        argument++;

    return argument;
}

const char *one_argument(const char *argument, char *arg_first, bool keepcase)
{
    return one_argument(const_cast<char*>(argument), arg_first, keepcase);
}

// Case insensitive; returns true if strings don't match, false if they do
bool str_cmp(const char *astr, const char *bstr)
{
    if (astr == NULL || bstr == NULL)
        return true;

    for (; *astr || *bstr; astr++, bstr++)
    {
        if (LOWER(*astr) != LOWER(*bstr))
            return true;
    }

    return false;
}

// Case insensitive; return true if astr not a prefix of bstr, false if it is
// Also returns true (to indicate not a prefix) if astr is empty, unless bstr is also empty
bool str_prefix(const char *astr, const char *bstr)
{
    if (astr == NULL || bstr == NULL)
        return true;

    if (astr[0] == '\0')
        return (bstr[0] != '\0');

    for (; *astr; astr++, bstr++)
    {
        if (LOWER(*astr) != LOWER(*bstr))
            return true;
    }

    return false;
}

int number_argument(const char *argument, char *arg)
{
    char * argIter(arg);
    *argIter = '\0';
    for (const char * iter(argument); *iter != '\0'; ++iter)
    {
        if (*iter == '.')
        {
            // Found a dot, so interpret as numerical prefix
            *argIter = '\0';
            int number(atoi(arg));
            strcpy(arg, iter + 1);
            return number;
        }

        // Copy the value
        *argIter = *iter;
        ++argIter;
    }

    *argIter = '\0';
    return 1;
}

std::string makeString(int number)
{
    std::ostringstream mess;
    mess << number;
    return mess.str();
}

std::pair<int, std::string> number_argument(const char * argument)
{
    char buffer[MAX_STRING_LENGTH];
    int number(number_argument(argument, buffer));
    return std::make_pair(number, std::string(buffer));
}

std::vector<std::string> splitArguments(const char * values, bool keepcase)
{
    char buffer[MAX_STRING_LENGTH];
    std::vector<std::string> result;
    while (values[0] != '\0')
    {
        values = one_argument(values, buffer, keepcase);
        result.push_back(buffer);
    }
    return result;
}

const char * indefiniteArticleFor(char nextLetter)
{
    switch (LOWER(nextLetter))
    {
        case 'a':
        case 'e':
        case 'i':
        case 'o':
        case 'u':
            return "an";
    }

    return "a";
}

// Splits a string into lines.  This function is relatively slow, so only
// use it for human-time or rare events (like string editing)
std::vector<std::string> split_into_lines(const char * text, bool preserveNewLines)
{
	assert(text != 0);

	std::vector<std::string> result;
	size_t lineStart(0);
	size_t i(0);
	while (text[i] != '\0')
	{
		// Look for an end of line character
        size_t lineEnd(i);
		if (text[i] == '\r' || text[i] == '\n')
		{
			// Character found; check the next one for another; if found, skip it
			if ((text[i + 1] == '\r' || text[i + 1] == '\n') && text[i + 1] != text[i])
				++i;
			
			// Push the string onto the vector and update lineStart
            if (preserveNewLines)
    			result.push_back(std::string(text + lineStart, i + 1 - lineStart));
            else
                result.push_back(std::string(text + lineStart, lineEnd - lineStart));

			lineStart = i + 1;
		}

		// Increment the walking var
		++i;
	}

	// Don't forget any remnant
	if (lineStart != i)
		result.push_back(std::string(text + lineStart, i - lineStart));

	// Return the vector of strings
	return result;
}

std::string stripPunctuation(const std::string & text)
{
    std::string result;
    result.reserve(text.size());
    for (size_t i(0); i < text.size(); ++i)
    {
        if (text[i] >= '!' && text[i] <= '/')
            continue;

        if (text[i] >= ':' && text[i] <= '@')
            continue;
        
        if (text[i] >= '[' && text[i] <= '`')
            continue;

        if (text[i] >= '{')
            continue;

        result += text[i];
    }

    return result;
}
