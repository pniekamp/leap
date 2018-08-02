//
// bound.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <leap/lml/vector.h>
#include <leap/lml/bound.h>
#include <leap/lml/io.h>
#include <fstream>

using namespace std;
using namespace leap;
using namespace leap::lml;

//|//////////////////// TestBound ///////////////////////////////////////////
void TestBound()
{
  Bound2d A = make_bound(Vector2(0.0, 0.0), 0.0);

  if (volume(A) != 0.0)
    cout << "** Should Be Zero Area\n";

  if (!contains(A, Vector2(0.0, 0.0)))
    cout << "** Should Contain Zero\n";

  Bound2d B = make_bound(Vector2(0.0, 0.0), 10.0);

  if (!contains(B, Vector2(0.0, 0.0)))
    cout << "** Should Contain Zero\n";

  if (contains(B, Vector2(-50.0, 10.0)))
    cout << "** Shouldn't Contain Coincident Top Point\n";

  if (contains(B, Vector2(-50.0, -10.0)))
    cout << "** Shouldn't Contain Coincident Bottom Point\n";

  if (contains(B, Vector2(-50.0, 0.0)))
    cout << "** Shouldn't Contain Coincident Middle Point\n";

  if (contains(B, Vector2(50.0, 10.0)))
    cout << "** Shouldn't Contain Coincident Top Point\n";

  if (contains(B, Vector2(50.0, -10.0)))
    cout << "** Shouldn't Contain Coincident Bottom Point\n";

  if (contains(B, Vector2(50.0, 0.0)))
    cout << "** Shouldn't Contain Coincident Middle Point\n";

  if (!contains(B, Vector2(-10.0, 0.0)))
    cout << "** Should Contain left\n";

  if (!contains(B, Vector2(10.0, 0.0)))
    cout << "** Should Contain right\n";

  if (!contains(B, Vector2(0.0, -10.0)))
    cout << "** Should Contain bottom\n";

  if (!contains(B, Vector2(0.0, 10.0)))
    cout << "** Should Contain top\n";

  if (!contains(B, B))
    cout << "** Should Contain itself\n";

  if (!intersects(B, B))
    cout << "** Should Overlap itself\n";

  if (!contains(B, make_bound(Vector2(-10.0, 0.0), 0.0)))
    cout << "** Should Contain left\n";

  if (!contains(B, make_bound(Vector2(10.0, 0.0), 0.0)))
    cout << "** Should Contain right\n";

  if (!contains(B, make_bound(Vector2(0.0, -10.0), 0.0)))
    cout << "** Should Contain bottom\n";

  if (!contains(B, make_bound(Vector2(0.0, 10.0), 0.0)))
    cout << "** Should Contain top\n";

  if (!intersects(B, make_bound(Vector2(-10.0, 0.0), 0.0)))
    cout << "** Should Overlap left\n";

  if (!intersects(B, make_bound(Vector2(10.0, 0.0), 0.0)))
    cout << "** Should Overlap right\n";

  if (!intersects(B, make_bound(Vector2(0.0, -10.0), 0.0)))
    cout << "** Should Overlap bottom\n";

  if (!intersects(B, make_bound(Vector2(0.0, 10.0), 0.0)))
    cout << "** Should Overlap top\n";

  if (!intersects(B, make_bound(Vector2(-20.0, 0.0), 10.0)))
    cout << "** Should Overlap left\n";

  if (!intersects(B, make_bound(Vector2(20.0, 0.0), 10.0)))
    cout << "** Should Overlap right\n";

  if (!intersects(B, make_bound(Vector2(0.0, -20.0), 10.0)))
    cout << "** Should Overlap bottom\n";

  if (!intersects(B, make_bound(Vector2(0.0, 20.0), 10.0)))
    cout << "** Should Overlap top\n";

  if (translate(B, Vector2(5.0, 7.0)) != Bound2d({ -5.0, -3.0 }, { 15.0, 17.0 }))
    cout << "** Wrong Bound translate\n";

  if (scale(B, 2) != Bound2d({ -20.0, -20.0 }, { 20.0, 20.0 }))
    cout << "** Wrong Bound scalar scale\n";

  if (scale(B, Vector2(2.0, 3.0)) != Bound2d({ -20.0, -30.0 }, { 20.0, 30.0 }))
    cout << "** Wrong Bound vector scale\n";

  if (grow(B, 2) != Bound2d({ -12.0, -12.0 }, { 12.0, 12.0 }))
    cout << "** Wrong Bound scalar grow\n";

  if (grow(B, Vector2(2.0, 3.0)) != Bound2d({ -12.0, -13.0 }, { 12.0, 13.0 }))
    cout << "** Wrong Bound vector grow\n";
}


//|//////////////////// TestBoundRay ////////////////////////////////////////
void TestBoundRay()
{
  Bound3f A = make_bound(Vector3(-2.0f, -2.0f, -2.0f), Vector3(2.0f, 2.0f, 2.0f));

  cout << "  " << A << endl;

  if (intersection(A, Vector3(-6.0f, 4.0f, 3.0f), Vector3(6.0f, 4.0f, 3.0f)).ray())
    cout << "** Should not intersect\n";

  if (intersection(A, Vector3(-6.0f, 4.0f, 3.0f), Vector3(6.0f, 4.0f, 3.0f)).seg())
    cout << "** Should not intersect\n";

  if (!intersection(A, Vector3(-6.0f, 1.0f, 1.5f), Vector3(2.0f, 1.0f, 1.5f)).ray())
    cout << "** Should intersect\n";

  if (!intersection(A, Vector3(4.0f, 1.0f, 1.5f), Vector3(3.0f, 1.2f, 0.8f)).ray())
    cout << "** Should intersect\n";

  if (intersection(A, Vector3(4.0f, 1.0f, 1.5f), Vector3(3.0f, 1.2f, 0.8f)).seg())
    cout << "** Should not intersect\n";

  if (!intersection(A, Vector3(1.0f, 0.2f, 1.3f), Vector3(10.0f, 11.0f, 12.0f)).ray())
    cout << "** Should intersect\n";
}


//|//////////////////// BoundTest ///////////////////////////////////////////
void BoundTest()
{
  cout << "Test Bound\n";

  TestBound();

  TestBoundRay();

  cout << endl;
}
