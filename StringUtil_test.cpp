#include "StringUtil.h"
#include "gtest/gtest.h"

// Defined extern'd functions with nonsensical things. Void cast is to silence unused param warnings.
char * str_dup(const char *str) { (void) str; return ""; };
void free_string(char *&pstr)   { (void)pstr; };

TEST(MacroTests, Upper) {
  EXPECT_EQ('U', UPPER('u'));
  EXPECT_EQ('U', UPPER('U'));
  EXPECT_EQ('>', UPPER('>'));
}

TEST(MacroTests, Lower) {
  EXPECT_EQ('u', LOWER('u'));
  EXPECT_EQ('u', LOWER('U'));
  EXPECT_EQ('>', LOWER('>'));
}

TEST(WhiteSpace, IsWhitespace) {
  EXPECT_EQ(true, is_whitespace(' '));
  EXPECT_EQ(true, is_whitespace('\n'));
  EXPECT_EQ(true, is_whitespace('	'));
  EXPECT_EQ(true, is_whitespace('\r'));
  EXPECT_EQ(false, is_whitespace('k'));
  EXPECT_EQ(false, is_whitespace('\0'));
}

TEST(Trim, TrimFront) {
  ASSERT_STREQ("foo", trimFront(" foo"));
  // touble space
  ASSERT_STREQ("foo", trimFront("  foo"));
  // doesn't trim ending
  ASSERT_STREQ("foo  ", trimFront("  foo  "));
}

TEST(Trim, Trim_CPP) {
  EXPECT_EQ(std::string("foo"), trim("   foo "));
  EXPECT_EQ(std::string("foo"), trim("foo"));
  EXPECT_EQ(std::string("foo"), trim(" foo"));
  EXPECT_EQ(std::string("foo"), trim("foo "));
}

TEST(Trim, Punctuation) {
  EXPECT_EQ(std::string("foo"), stripPunctuation("foo!"));
  EXPECT_EQ(std::string("foo"), stripPunctuation("foo."));
  EXPECT_EQ(std::string("foobar"), stripPunctuation("foo-bar"));
  EXPECT_EQ(std::string("foo bar"), stripPunctuation("foo bar"));
}
