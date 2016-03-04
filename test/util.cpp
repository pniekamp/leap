//
// util.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <leap/util.h>

using namespace std;
using namespace leap;

void UtilTest();

//|//////////////////// UtilTest ////////////////////////////////////////////
void UtilTest()
{
  cout << "Util Test\n\n";

  vector<double> a;
  a.push_back(1.0);
  a.push_back(2.0);
  a.push_back(3.12345);

  cout << "  " << vtoa(a, 4) << "\n";

  vector<double> b = atov<double>("1.0, 2.0, 3.1235");

  cout << "  ";
  copy(b.begin(), b.end(), std::ostream_iterator<double>(cout, " "));
  cout << "\n";
/*
TODO
  vector<string> c;
  c.push_back("a");
  c.push_back("b");
  c.push_back("c d");

  cout << "  " << vto(c) << "\n";

  vector<string> d = tov<string>("a, b, c");

  cout << "  ";
  copy(d.begin(), d.end(), std::ostream_iterator<string>(cout, " "));
  cout << "\n";
*/
  cout << endl;

  cout << "  " << '"' << trim("abcd") << '"' << endl;
  cout << "  " << '"' << trim("abcd  ") << '"' << endl;
  cout << "  " << '"' << trim("  abcd") << '"' << endl;
  cout << "  " << '"' << trim("  abcd  ") << '"' << endl;
  cout << "  " << '"' << trim("") << '"' << endl;
  cout << "  " << '"' << trim("  ") << '"' << endl;

  cout << endl;
}
