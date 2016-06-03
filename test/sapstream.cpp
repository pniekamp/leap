//
// sapstream.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <leap/sapstream.h>

using namespace std;
using namespace leap;

void SAPStreamTest();

//|//////////////////// SAPStreamTest ///////////////////////////////////////
void SAPStreamTest()
{
  cout << "SAP Stream Test\n\n";

  string a =
    "\
      #define NAME Tester\n\
      #define FULLNAME Joe ${NAME}\n\
      Entry EntryID\n\
      {\n\
        name = Attribute ${NAME} Value\n\
        fullname = Attribute ${FULLNAME} Value\n\
        environ = Environment ${USERNAME} Value\n\
        num = 12.3\n\
        path = C:\\\n\
        \
        SubEntry SubID\n\
        {\n\
          SubName = ${NAME}\n\
        }\n\
        \n\
        NextSubEntry NextSubID\n\
        {\n\
          SubName = SubValue\n\
        }\n\
        \
        PostAttrib = After the Sub Entry\n\
      }\n\
      \n\
      NextEntry NextID\n\
      {\n\
        NextName = Next ${NAME} Value\n\
      }\n\
    ";

  issapstream sap(a);

  sapentry entry;

  while (sap >> entry)
  {
    cout << "  " << entry["entrytype"] << " " << entry["entryid"] << "\n";
    cout << "  {\n";

    for(sapentry::const_iterator i = entry.begin(); i != entry.end(); ++i)
    {
      cout << "    " << i->name << " = " << i->value << "\n";
    }

    cout << "\n";

    sapentry subentry;
    while (entry.substream() >> subentry)
    {
      cout << "    " << subentry["entrytype"] << " " << subentry["entryid"] << "\n";
      cout << "    {\n";

      for(sapentry::const_iterator i = subentry.begin(); i != subentry.end(); ++i)
      {
        cout << "      " << i->name << " = " << i->value << "\n";
      }

      cout << "    }\n";
      cout << "\n";
    }

    cout << "  }\n";
    cout << "\n";
    cout << "  Number is " << ato<double>(entry["num"], 5.0) << "\n";
    cout << "\n";
  }

  wstring b = L"Entry EntryID\n{\nNAME = Wide Test\n}\n";

  iwssapstream wsap(b);

  wsapentry wentry;

  wsap >> wentry;

  cout << endl;
}
