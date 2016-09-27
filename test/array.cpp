//
// array.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <sstream>
#include <leap/lml/array.h>

using namespace std;
using namespace leap;
using namespace leap::lml;


void ArrayTest();


//|//////////////////// ArrayBasicTest //////////////////////////////////////
static void ArrayBasicTest()
{
  cout << "Simple Array Test Set\n";

  Array<double, 2> A(10u, 20u);

  cout << "  Size: " << A.size() << "\n";

  if (A.size() != 200)
    cout << "** Error on Array Size\n";

  for(size_t j = 0; j < A.shape()[0]; ++j)
    for(size_t i = 0; i < A.shape()[1]; ++i)
      A[j][i] = j*100.0 + i;

  if (A[4][1] != 401)
    cout << "** Error on Array Index\n";

  {
    A = Array<double, 2>(25u, 15u);
  }

  cout << "  Size: " << A.size() << "\n";

  if (A.size() != 375)
    cout << "** Error on Array Size\n";

  auto B = A;

  if (B.size() != 375)
    cout << "** Error on Array Size\n";

  B = A;

  if (B.size() != 375)
    cout << "** Error on Array Size\n";

  cout << endl;
}


//|//////////////////// ArrayTest ///////////////////////////////////////////
void ArrayTest()
{
  ArrayBasicTest();

  cout << endl;
}

