//
// pathstring.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <leap/pathstring.h>

using namespace std;
using namespace leap;

void PathStringTest();

//|//////////////////// PathStringTest ////////////////////////////////////////////
void PathStringTest()
{
  cout << "PathString Test\n\n";

  pathstring a;

  if (a.base() != "")
    cout << "** Null path base error\n";

  if (a.name() != "")
    cout << "** Null path name error\n";

  if (a.basename() != "")
    cout << "** Null path name error\n";

  if (a.extension() != "")
    cout << "** Null path name error\n";

  pathstring b = "config.cfg";

  cout << "  " << b << " (" << b.base() << " :: " << b.name() << " .. " << b.extension() << ")" << "\n";

  pathstring c = "/config.cfg";

  cout << "  " << c << " (" << c.base() << " :: " << c.name() << " .. " << c.extension() << ")" << "\n";

  pathstring d = "c:/";

  cout << "  " << d << " (" << d.base() << " :: " << d.name() << " .. " << d.extension() << ")" << "\n";

  pathstring e = "c:/windows/config";

  cout << "  " << e << " (" << e.base() << " :: " << e.name() << " .. " << e.extension() << ")" << "\n";

  cout << endl;
  cout << endl;
}
