#include "StringUtil.h"
#include "gtest/gtest.h"

// Defined extern'd functions with nonsensical things.
char * str_dup(const char *str) { return ""; };
void free_string(char *&pstr) {};

TEST(MacroTests, Upper) {
  EXPECT_EQ('U', UPPER('u'));
  EXPECT_EQ('U', UPPER('U'));
  EXPECT_EQ('>', UPPER('>'));
}
