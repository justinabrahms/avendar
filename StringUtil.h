#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <vector>
#include <string>
#include <utility>

#define MAX_STRING_LENGTH	 20000 
#define LOWER(c)		((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))
#define UPPER(c)		((c) >= 'a' && (c) <= 'z' ? (c)+'A'-'a' : (c))

void copy_string(char *& dest, const char * src);
char * one_argument(char *argument, char *arg_first, bool keepcase = false);
const char * one_argument(const char *argument, char *arg_first, bool keepcase = false);
int number_argument(const char *argument, char *arg);
std::pair<int, std::string> number_argument(const char * argument);
std::vector<std::string> splitArguments(const char * values, bool keepcase = false);
bool str_cmp(const char * astr, const char * bstr);
bool str_prefix(const char * astr, const char * bstr);
const char * indefiniteArticleFor(char nextLetter);
std::string makeString(int number);
std::vector<std::string> split_into_lines(const char * text, bool preserveNewLines = true);

bool is_whitespace(char letter);
const char * trimFront(const char * line);
std::string trimBack(const char * line, unsigned int length);
std::string trim(const char * line, unsigned int length);
std::string trim(const std::string & line);
std::string stripPunctuation(const std::string & text);

#endif
