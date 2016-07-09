//
// test.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>

using namespace std;

extern void ArrayTest();  // array.cpp
extern void VectorTest();  // vector.cpp
extern void MatrixTest();  // matrix.cpp
extern void SwizzleTest();  // swizzle.cpp
extern void EvaluatorTest();  // evaluator.cpp
extern void InterpolatorTest();  // interpolator.cpp
extern void SigLibTest(); // siglib.cpp
extern void SAPStreamTest(); // sapstream.cpp
extern void ThreadTest(); // threadcontrol.cpp
extern void SocketTest(); // sockets.cpp
extern void RegExTest(); // regex.cpp
extern void UtilTest(); // util.cpp
extern void PathStringTest(); // pathstring.cpp
extern void HTTPTest(); // http.cpp
extern void GeometryTest(); // geometry.cpp
extern void BoundTest(); // bound.cpp
extern void RTreeTest(); // rtree.cpp


//|//////////////////// TestSet /////////////////////////////////////////////
int main(int argc, char *args[])
{
  cout << "libLeap Test Routines\n\n";

  try
  {
    ArrayTest();

    VectorTest();

    MatrixTest();

    SwizzleTest();

    EvaluatorTest();

    InterpolatorTest();

    SigLibTest();

    SAPStreamTest();

    ThreadTest();

    SocketTest();

    RegExTest();

    UtilTest();

    PathStringTest();

    HTTPTest();

    GeometryTest();

    BoundTest();

    RTreeTest();
  }
  catch(const exception &e)
  {
    cout << "** " << e.what() << endl;
  }

  cout << endl;
}
