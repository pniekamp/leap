//
// interpolator.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <array>
#include <leap/lml/interpolation.h>
#include <leap/lml/cubicspline.h>
#include <leap/lml/bezier.h>
#include <leap/lml/io.h>
#include <leap/util.h>

using namespace std;
using namespace leap;
using namespace leap::lml;


void InterpolatorTest();


//|//////////////////// SimpleInterpolatorTest //////////////////////////////
static void SimpleInterpolatorTest()
{
  cout << "Simple Interpolator Test Set\n";

  vector<double> xa;
  vector<double> ya;

  if (interpolate<linear>(xa, ya, 0.5) != 0.0)
    cout << "** Error in Empty Linear Interpolation\n";

  xa.push_back(0.1);
  ya.push_back(0.1);

  if (interpolate<linear>(xa, ya, 0.5) != 0.1)
    cout << "** Error in Single Linear Interpolation\n";

  xa.push_back(0.5);
  ya.push_back(0.5);

  if (!fcmp(interpolate<linear>(xa, ya, -0.5), -0.5) || !fcmp(interpolate<linear>(xa, ya, 0.25), 0.25) || !fcmp(interpolate<linear>(xa, ya, 0.5), 0.5) || !fcmp(interpolate<linear>(xa, ya, 1.5), 1.5))
    cout << "** Error in Double Linear Interpolation\n";

  xa.push_back(0.75);
  ya.push_back(0.75);

  if (!fcmp(interpolate<linear>(xa, ya, -0.5), -0.5) || !fcmp(interpolate<linear>(xa, ya, 0.25), 0.25) || !fcmp(interpolate<linear>(xa, ya, 0.5), 0.5) || !fcmp(interpolate<linear>(xa, ya, 1.5), 1.5))
    cout << "** Error in Triple Linear Interpolation\n";

  xa.push_back(1.0);
  ya.push_back(2.0);

  if (!fcmp(interpolate<cubic>(xa, ya, 0.0), 0.0) || !fcmp(interpolate<cubic>(xa, ya, 0.25), 0.25) || !fcmp(interpolate<cubic>(xa, ya, 0.5), 0.5) || !fcmp(interpolate<cubic>(xa, ya, 1.1), 2.388))
    cout << "** Error in Triple Cubic Interpolation\n";

  clock_t s = clock();

  double sum = 0;
  for(int i = 0; i < 500000; ++i)
    sum += interpolate<linear>(xa, ya, i/500000.0);

  cout << "  Sum: " << sum << " (" << (clock() - s)/(double)CLOCKS_PER_SEC << "s)\n";

  s = clock();

  sum = 0;
  for(int i = 0; i < 500000; ++i)
    sum += interpolate<cubic>(xa, ya, i/500000.0);

  cout << "  Sum: " << sum << " (" << (clock() - s)/(double)CLOCKS_PER_SEC << "s)\n";

  cout << endl;
}


//|//////////////////// ArrayInterpolatorTest ///////////////////////////////
static void ArrayInterpolatorTest()
{
  cout << "Array Interpolator Test Set\n";

  {
    Array<double, 1> ya(1u);

    ya[0] = 99;

    vector<double> xa[1] = { { 0 } };

    double x[1] = { 0.5 };

    if (interpolate<linear>(xa, ya, x) != 99)
      cout << "** Error on Empty Array Interpolation\n";

    if (interpolate<cubic>(xa, ya, x) != 99)
      cout << "** Error on Empty Array Interpolation\n";
  }

  {
    Array<double, 1> ya(4u);

    ya[0] = 0;
    ya[1] = 2;
    ya[2] = 4;
    ya[3] = 9;

    vector<double> xa[1] = { { 0.0, 1.0, 2.0, 3.0 } };

    double x[1] = { 1.5 };

    if (interpolate<linear>(xa, ya, x) != 3.0)
      cout << "** Error on 1d Array Interpolation\n";

    if (interpolate<cubic>(xa, ya, x) != 2.8125)
      cout << "** Error on 1d Array Interpolation\n";
  }

  {
    Array<double, 2> ya(2u, 4u);

    ya[0][0] = 0;
    ya[0][1] = 2;
    ya[0][2] = 4;
    ya[0][3] = 9;
    ya[1][0] = 1;
    ya[1][1] = 3;
    ya[1][2] = 5;
    ya[1][3] = 10;

    vector<double> xa[2] = { { 10.0, 20.0 }, { 0.0, 1.0, 2.0, 3.0 } };

    double x[2] = { 12.0, 1.5 };

    if (!fcmp(interpolate<linear>(xa, ya, x), 3.2))
      cout << "** Error on 2d Array Interpolation\n";

    if (!fcmp(interpolate<cubic>(xa, ya, x), 3.0125))
      cout << "** Error on 2d Array Interpolation\n";

  }

  {
    Array<double, 2> ya(1u, 4u);

    ya[0][0] = 0;
    ya[0][1] = 2;
    ya[0][2] = 4;
    ya[0][3] = 9;

    vector<double> xa[2] = { { 10.0 }, { 0.0, 1.0, 2.0, 3.0 } };

    if (interpolate<linear>(xa, ya, {10.0, 2.5}) != 6.5)
      cout << "** Error on 2d Array Interpolation\n";

    if (interpolate<linear>(xa, ya, {10.0, 0.0}) != 0.0)
      cout << "** Error on 2d Array Interpolation\n";

    if (interpolate<linear>(xa, ya, {0.0, 0.0}) != 0.0)
      cout << "** Error on 2d Array Interpolation\n";

    if (interpolate<linear>(xa, ya, {99.0, 4.0}) != 14.0)
      cout << "** Error on 2d Array Interpolation\n";
  }

  {
    Array<double, 3> ya(2u, 2u, 4u);

    ya[0][0][0] = 0;
    ya[0][0][1] = 2;
    ya[0][0][2] = 4;
    ya[0][0][3] = 9;
    ya[0][1][0] = 0;
    ya[0][1][1] = 4;
    ya[0][1][2] = 8;
    ya[0][1][3] = 18;
    ya[1][0][0] = 0;
    ya[1][0][1] = 3;
    ya[1][0][2] = 5;
    ya[1][0][3] = 10;
    ya[1][1][0] = 0;
    ya[1][1][1] = 6;
    ya[1][1][2] = 10;
    ya[1][1][3] = 20;

    vector<double> xa[3] = { { 0.0, 1.0 }, { 5.0, 10.0 }, { 0.0, 1.0, 2.0, 3.0 } };

    if (!fcmp(interpolate<linear>(xa, ya, {0.5, 6.0, 1.5}),4.2))
      cout << "** Error on 3d Array Interpolation\n";

    if (!fcmp(interpolate<cubic>(xa, ya, {0.5, 6.0, 1.5}),4.0125))
      cout << "** Error on 3d Array Interpolation\n";

    clock_t s = clock();

    double sum = 0;
    for(int i = 0; i < 300000; ++i)
      sum += interpolate<linear>(xa, ya, {0.5, 6.0, i/100000.0});

    cout << "  Sum: " << sum << " (" << (clock() - s)/(double)CLOCKS_PER_SEC << "s)\n";

    s = clock();

    sum = 0;
    for(int i = 0; i < 300000; ++i)
      sum += interpolate<cubic>(xa, ya, {0.5, 6.0, i/100000.0});

    cout << "  Sum: " << sum << " (" << (clock() - s)/(double)CLOCKS_PER_SEC << "s)\n";

  }

  cout << endl;
}


//|//////////////////// SplineTest //////////////////////////////////////////
static void SplineTest()
{
  CubicSpline<Vector2f> spline({ Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f), Vector2(5.0f, 15.0f) });

  if (!fcmp(spline.value(0.5f), 0.40625f))
    cout << "** Spline Value Error\n";

  if (!fcmp(spline.derivative(0.5f), 0.9375f))
    cout << "** Spline Derivative Error\n";
}


//|//////////////////// BezierTest //////////////////////////////////////////
static void BezierTest()
{
  Bezier<Vector2f> bezier2({ Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f), Vector2(5.0f, 15.0f) });

  if (!fcmp(bezier2.value(0.4f)(0), 0.44f, 1e-3f) || !fcmp(bezier2.value(0.4f)(1), -0.52f, 1e-3f))
    cout << "** Bezier Value Error\n";

  Bezier<Vector3d> bezier3({ Vector3(0.0, 0.0, 0.0), Vector3(1.0, 1.0, 1.0), Vector3(5.0, 15.0, 20.0) });

  if (!fcmp(bezier3.value(0.4f)(0), 0.44, 1e-3) || !fcmp(bezier3.value(0.4f)(1), -0.52, 1e-3) || !fcmp(bezier3.value(0.4f)(2), -1.0, 1e-3))
    cout << "** Bezier3 Value Error\n";
}


//|//////////////////// InterpolatorTest ////////////////////////////////////
void InterpolatorTest()
{
  SimpleInterpolatorTest();

  ArrayInterpolatorTest();

  SplineTest();

  BezierTest();

  cout << endl;
}
