//
// regex.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <algorithm>
#include <leap/util.h>
#include <leap/regex.h>

using namespace std;
using namespace leap;
using namespace regex;

void RegExTest();


////////// print_w ///////////
template<typename T> struct print_w
{
  void operator()(leap::string_view w)
  {
    cout << "'" << ato<T>(w) << "' ";
  }
};


//|//////////////////// SimpleRegExTests ////////////////////////////////////
static void SimpleRegExTests()
{
  cout << "RegExTest Set\n";

  if (!match("", "Test"))
    cout << "** Null RegEx Match Failed\n";

  match(".", "");

  if (!match(".", "T"))
    cout << "** Wildcard Match Failed\n";

  if (!match(".", "abcd"))
    cout << "** Wildcard Trailing Match Failed\n";

  if (!match("\\.\\\\", ".\\"))
    cout << "** Literal Match Failed\n";

  if (!search("", "Test"))
    cout << "** Null RegEx Search Failed\n";

  if (!search("a.", "aT"))
    cout << "** Wildcard Search Failed\n";

  cout << "  Grouping...\n";

  if (!match("a(bc)d", "abcd"))
    cout << "** Simple Group Match Failed\n";

  if (!match("a(bc())g", "abcg"))
    cout << "** Empty Group Match Failed\n";

  if (!match("a(bc(de)f)g", "abcdefg"))
    cout << "** Nested Group Match Failed\n";

  cout << "  Repeats...\n";

  if (match("abc", "ac") || !match("abc", "abc") || match("abc", "abbbc"))
    cout << "** Direct Match Failed\n";

  if (!match("ab*c", "ac") || !match("ab*c", "abc") || !match("ab*c", "abbbc"))
    cout << "** Zero or More Repeat Match Failed\n";

  if (!match("ab?c", "ac") || !match("ab?c", "abc") || match("ab?c", "abbbc"))
    cout << "** Zero or One Repeat Match Failed\n";

  if (match("ab+c", "ac") || !match("ab+c", "abc") || !match("ab+c", "abbbc"))
    cout << "** One or More Repeat Match Failed\n";

  if (!match("a(bc)*d", "ad") || !match("a(bc)*d", "abcd") || !match("a(bc)*d", "abcbcbcd"))
    cout << "** Group Repeat Match Failed\n";

  if (!match("a(bc(de)+)*f", "af") || match("a(bc(de)+)*f", "abcf") || !match("a(bc(de)+)*f", "abcdebcdedef"))
    cout << "** Nested Group Repeat Match Failed\n";

  cout << "  Alternatives...\n";

  if (!match("def|ghi", "def") || !match("def|ghi", "ghi") || match("def|ghi", "abc"))
    cout << "** Single Alternative Match Failed\n";

  if (!match("def|ghi|jkl", "def") || !match("def|ghi|jkl", "ghi") || !match("def|ghi|jkl", "jkl"))
    cout << "** Double Alternative Match Failed\n";

  if (!match("(def|ghi+)+t", "defghiit"))
    cout << "** Grouped Alternative Match Failed\n";

  cout << "  Placeholders...\n";

  if (!match("^abc", "abc") || match("^abc", "aabc"))
    cout << "** Start Of Line Match Failed\n";

  if (!match(".*bcd$", "abcd") || match(".*bcd$", "abcde"))
    cout << "** End Of Line Match Failed\n";

  if (!match("^.*bcd.*$", "abcde"))
    cout << "** Full Line Match Failed\n";

  cout << "  Sets...\n";

  if (!match("[[:alnum:]]*[[:space:]][[:upper:]][[:lower:]]$", "abcDEF012 Aa"))
    cout << "** Set Match Failed\n";

  if (!match("^[+-[:digit:]\\.Ee]+$", "-12.4E+02"))
    cout << "** Number Set Match Failed\n";

  cout << "  Group Marking...\n";

  vector<leap::string_view> split;

  if (!match("^(?:(.+)(?:[[:space:]]+|$))*$", "This is a test", &split))
    cout << "** Split Set Match Failed\n";

  cout << "    ";
  for_each(split.begin(), split.end(), print_w<string>());
  cout << "\n";

  vector<leap::string_view> numbers;

  if (!match("^[[:space:]]*(?:([+-[:digit:]\\.Ee]+)(?:[[:space:]]+|$))*$", " 1 2 6.4 -12E04\t+0.11", &numbers))
    cout << "** Number Set Match Failed\n";

  cout << "    ";
  for_each(numbers.begin(), numbers.end(), print_w<double>());
  cout << "\n";

  vector<leap::string_view> words;
  
  if (!match("^[[:space:]]*(?:((?:\"[^\"]+\"|[^[:space:]]+))(?:[[:space:]]+|$))*$", "  dogs, cats and \"other things\" (mice?)", &words))
    cout << "** Word Set Match Failed\n";

  cout << "    ";
  for_each(words.begin(), words.end(), print_w<string>());
  cout << "\n";

  cout << "\n";
}


//|//////////////////// RegExTest ///////////////////////////////////////////
void RegExTest()
{
  SimpleRegExTests();

  cout << endl;
}
