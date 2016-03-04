//
// geometry.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <leap/lml/io.h>
#include <leap/lml/geometry.h>
#include <leap/lml/geometry2d.h>
#include <leap/lml/matrixconstants.h>

using namespace std;
using namespace leap;
using namespace leap::lml;

void GeometryTest();

// Test Point Type

struct Vec2 { float x; float y; };

namespace leap { namespace lml
{
  template<>
  struct point_traits<Vec2>
  {
    static constexpr size_t dimension = 2;
  };
} }

template<size_t i>
constexpr auto get(Vec2 const &pt);

template<>
constexpr auto get<0>(Vec2 const &pt)
{
  return pt.x;
}

template<>
constexpr auto get<1>(Vec2 const &pt)
{
  return pt.y;
}

std::ostream &operator <<(std::ostream &os, Vec2 const &pt)
{
  os << '(' << pt.x << ',' << pt.y << ')';

  return os;
}

// Test Point Type

class Vec3 : public VectorView<Vec3, float, 0, 1, 2>
{
  public:
    Vec3(float x, float y, float z)
      : x(x), y(y), z(z)
    {
    }

    Vec3 &operator =(Vec3 const &that)
    {
      x = that.x;
      y = that.y;
      z = that.z;

      return *this;
    }

    union
    {
      struct
      {
        float x, y, z;
      };

      VectorView<Vector2f, float, 0, 1> xy;
    };
};


bool similar(vector<Vector2d> const &a, vector<Vector2d> const &b)
{
  if (a.size() > 0 && b.size() > 0)
  {
    size_t i = 0;
    while (i < a.size() && a[i] != b[0])
      ++i;

    for(size_t j = 0; j < b.size(); ++j)
      if (a[(i+j) % a.size()] != b[j])
        return false;
  }

  return (a.size() == b.size());
}


//|//////////////////// TestBasic ///////////////////////////////////////////
void TestBasic()
{
  cout << "Test Basic Geometry\n";

  cout << "  distance: " << dist(Vec2{0.0, 0.0}, Vec2{0.0, 10.0}) << "\n";
  cout << "  angle: " << angle(Vec2{0.0, 0.0}, Vec2{10.0, 10.0})/pi<double>()*180 << "\n";
  cout << "  area2d: " << area(Vec2{0.0, 0.0}, Vec2{10.0, 0.0}, Vec2{5.0, 5.0}) << "\n";
  cout << "  area3d: " << area(Vector3f{0.0, 0.0, 2.0}, Vector3f{10.0, 0.0, 2.0}, Vector3f{5.0, 5.0, 2.0}) << "\n";

  if (!coincident(Vec2{2.5, 7.7}, Vec2{2.5, 7.7}))
    cout << "** Should be coincident\n";

//  if (!collinear(Point{2.5, 7.7}, Point{5.0, 15.4}, Point{10.0, 30.8}))
  if (!collinear(Vec2{2.5, 1.0}, Vec2{5.0, 2.0}, Vec2{10.0, 4.0}))
    cout << "** Should be collinear 2d\n";

  if (!collinear(Vector3f{2.5, 1.0, 2.0}, Vector3f{5.0, 2.0, 2.0}, Vector3f{10.0, 4.0, 2.0}))
    cout << "** Should be collinear 3d\n";

  if (orientation(Vec2{0.0, 0.0}, Vec2{5.0, 2.0}, Vec2{3.0, 4.0}) <= 0)
    cout << "** Should be Counter-Clockwise\n";

  if (quadrant(Vec2{1.0, 1.0}) != 0 || quadrant(Vec2{-1.0, 1.0}) != 1 || quadrant(Vec2{1.0, -1.0}) != 2 || quadrant(Vec2{-1.0, -1.0}) != 3)
    cout << "** Should be Four Quadrants\n";

  cout << "  normal: " << normal(Vector3f{0.0, 0.0, 2.0}, Vector3f{5.0, 2.0, 2.0}, Vector3f{3.0, 4.0, 2.0}) << endl;

  auto a = nearest_on_segment(Vec2{0.0, 0.0}, Vec2{5.0, 2.0}, Vec2{3.0, 4.0});
  auto b = nearest_on_segment(Vector3f{0.0, 0.0, 2.0}, Vector3f{5.0, 2.0, 2.0}, Vector3f{3.0, 4.0, 2.0});

  if (a.x != b(0) || a.y != b(1))
    cout << "** Should be similar nearest points\n";

  if (dist(std::pair<double, double>(1.0, 2.0), std::pair<double, double>(2.0, 2.0)) != 1)
    cout << "** Pair as Point Error\n";

  if (dist(std::array<double, 2>{1.0, 2.0}, std::array<double, 2>{2.0, 2.0}) != 1)
    cout << "** Array as Point Error\n";

  Vector2f c = Vector2(0.0f, 0.0f);

  a = translate(a, Vector2(1.0f, 2.0f));
  c = c + Vector2(1.0f, 2.0f);
  c += Vector2(1.0f, 2.0f);

  Vec3 d = Vec3(0.0f, 0.0f, 0.0f);
  d = d + Vector3(1.0f, 2.0f, 3.0f);
  d += Vector3(1.0f, 2.0f, 3.0f);
  d.xy = d.xy + Vector2(1.0f, 2.0f);
  d.xy += Vector2(1.0f, 2.0f);

  vector<Vector2d> P;
  P.push_back(Vector2(0.0, -0.5));
  P.push_back(Vector2(0.25, -0.5));
  P.push_back(Vector2(0.0, 0.0));
  P.push_back(Vector2(0.25, 0.5));
  P.push_back(Vector2(0.0, 0.5));

  if (is_convex(P))
    cout << "** Test Convex Error\n";

  if (is_simple(P))
    cout << "** Test Simple Error\n";

  cout << endl;
}


//|//////////////////// TestIntersection ////////////////////////////////////
void TestIntersection()
{
  cout << "Test Intersection\n";

  if (intersection(Vector2(10.0, 10.0), Vector2(20.0, 10.0), Vector2(10.0, 20.0), Vector2(20.0, 20.0)))
    cout << "** Parallel Lines Intersect?\n";

  if (intersection(Vector2(10.0, 10.0), Vector2(10.0, 20.0), Vector2(20.0, 10.0), Vector2(20.0, 20.0)))
    cout << "** Parallel Lines Intersect?\n";

  auto A = intersection(Vector2(10.0, 10.0), Vector2(20.0, 30.0), Vector2(10.0, 15.0), Vector2(20.0, 15.0));
  cout << "  intersection: " << *A << "\n";
  if (*A != Vector2(12.5, 15.0))
    cout << "** Wrong Intersection\n";

  if (intersection(Vector2(10.0, 10.0), Vector2(20.0, 30.0), Vector2(30.0, 15.0), Vector2(40.0, 15.0)).segseg())
    cout << "** Out of Segment Intersect?\n";

  auto B = nearest_on_segment(Vector2(-20.0, 10.0), Vector2(20.0, 30.0), Vector2(0.0, 0.0));
  cout << "  neareset_on_segment: " << B << "\n";
  if (B != Vector2(-8.0, 16.0))
    cout << "** Wrong NearPoint\n";

  cout << endl;
}


//|//////////////////// TestPolygon /////////////////////////////////////////
void TestPolygon()
{
  cout << "Test Polygon\n";

  vector<Vector2d> O;
  O.push_back(Vector2(-10.0, 0.0));
  O.push_back(Vector2(0.0, -10.0));
  O.push_back(Vector2(10.0, 0.0));
  O.push_back(Vector2(0.0, 10.0));

  if (!contains(O, Vector2(0.0, 0.0)))
    cout << "** Should Contain Zero\n";

  if (contains(O, Vector2(-50.0, 10.0)))
    cout << "** Shouldn't Contain Coincident Top Point\n";

  if (contains(O, Vector2(-50.0, -10.0)))
    cout << "** Shouldn't Contain Coincident Bottom Point\n";

  if (contains(O, Vector2(-50.0, 0.0)))
    cout << "** Shouldn't Contain Coincident Middle Point\n";

  if (contains(O, Vector2(50.0, 10.0)))
    cout << "** Shouldn't Contain Coincident Top Point\n";

  if (contains(O, Vector2(50.0, -10.0)))
    cout << "** Shouldn't Contain Coincident Bottom Point\n";

  if (contains(O, Vector2(50.0, 0.0)))
    cout << "** Shouldn't Contain Coincident Middle Point\n";

  vector<Vector2d> P;
  P.push_back(Vector2(-10.0, -10.0));
  P.push_back(Vector2(10.0, -10.0));
  P.push_back(Vector2(10.0, 10.0));
  P.push_back(Vector2(-10.0, 10.0));

  vector<Vector2d> Q;
  Q.push_back(Vector2(5.0, 5.0));
  Q.push_back(Vector2(15.0, 5.0));
  Q.push_back(Vector2(15.0, 15.0));
  Q.push_back(Vector2(5.0, 15.0));

  vector<vector<Vector2d> > R = boolean_intersection(P, Q);

  for(size_t i = 0; i < R.size(); ++i)
  {
    cout << "  " << i << ":";
    copy(R[i].begin(), R[i].end(), ostream_iterator<Vector2d>(cout, " "));
    cout << "\n";
  }

  if (R.size() != 1 || !similar(R[0], { Vector2(10.0, 5.0), Vector2(10.0, 10.0), Vector2(5.0, 10.0), Vector2(5.0, 5.0) }))
    cout << "** Intersection Error\n";

  R = boolean_intersection(Q, P);

  for(size_t i = 0; i < R.size(); ++i)
  {
    cout << "  " << i << ":";
    copy(R[i].begin(), R[i].end(), ostream_iterator<Vector2d>(cout, " "));
    cout << "\n";
  }

  if (R.size() != 1 || !similar(R[0], { Vector2(5.0, 5.0), Vector2(10.0, 5.0), Vector2(10.0, 10.0), Vector2(5.0, 10.0) }))
    cout << "** Intersection Error\n";

  Q[0] = Vector2(10.0, 5.0);
  Q[3] = Vector2(10.0, 15.0);

  R = boolean_intersection(P, Q);

  if (R.size() != 0)
    cout << "** Null Intersection Error\n";

  Q[0] = Vector2(5.0, 10.0);
  Q[1] = Vector2(15.0, 10.0);
  Q[2] = Vector2(15.0, 15.0);
  Q[3] = Vector2(5.0, 15.0);

  R = boolean_intersection(P, Q);

  if (R.size() != 0)
    cout << "** Null Intersection Error\n";

  Q[0] = Vector2(-10.0, 0.0);
  Q[1] = Vector2(0.0, -10.0);
  Q[2] = Vector2(10.0, 0.0);
  Q[3] = Vector2(0.0, 10.0);

  R = boolean_intersection(P, Q);

  for(size_t i = 0; i < R.size(); ++i)
  {
    cout << "  " << i << ":";
    copy(R[i].begin(), R[i].end(), ostream_iterator<Vector2d>(cout, " "));
    cout << "\n";
  }

  if (R.size() != 1 || !similar(R[0], { Vector2(-10.0, 0.0), Vector2(0.0, -10.0), Vector2(10.0, 0.0), Vector2(0.0, 10.0) }))
    cout << "** Intersection Error\n";

  Q[0] = Vector2(-10.0, 20.0);
  Q[1] = Vector2(0.0, 10.0);
  Q[2] = Vector2(10.0, 20.0);
  Q[3] = Vector2(0.0, 30.0);

  R = boolean_intersection(P, Q);

  if (R.size() != 0)
    cout << "** Null Intersection Error\n";

  Q[0] = Vector2(-10.0, 20.0);
  Q[1] = Vector2(-5.0, 10.0);
  Q[2] = Vector2(5.0, 10.0);
  Q[3] = Vector2(10.0, 20.0);

  R = boolean_intersection(P, Q);

  if (R.size() != 0)
    cout << "** Null Intersection Error\n";

  Q[0] = Vector2(-10.0, 0.0);
  Q[1] = Vector2(10.0, 0.0);
  Q[2] = Vector2(5.0, 10.0);
  Q[3] = Vector2(-5.0, 10.0);

  R = boolean_intersection(P, Q);

  for(size_t i = 0; i < R.size(); ++i)
  {
    cout << "  " << i << ":";
    copy(R[i].begin(), R[i].end(), ostream_iterator<Vector2d>(cout, " "));
    cout << "\n";
  }

  if (R.size() != 1 || !similar(R[0], { Vector2(5.0, 10.0), Vector2(-5.0, 10.0), Vector2(-10.0, 0.0), Vector2(10.0, 0.0) }))
    cout << "** Intersection Error\n";

  Q[0] = Vector2(-10.0, 0.0);
  Q[1] = Vector2(20.0, 0.0);
  Q[2] = Vector2(20.0, 10.0);
  Q[3] = Vector2(-10.0, 10.0);

  R = boolean_intersection(P, Q);

  for(size_t i = 0; i < R.size(); ++i)
  {
    cout << "  " << i << ":";
    copy(R[i].begin(), R[i].end(), ostream_iterator<Vector2d>(cout, " "));
    cout << "\n";
  }

  if (R.size() != 1 || !similar(R[0], { Vector2(-10.0, 0.0), Vector2(10.0, 0.0), Vector2(10.0, 10.0), Vector2(-10.0, 10.0) }))
    cout << "** Intersection Error\n";

  R = boolean_intersection(P, P);

  for(size_t i = 0; i < R.size(); ++i)
  {
    cout << "  " << i << ":";
    copy(R[i].begin(), R[i].end(), ostream_iterator<Vector2d>(cout, " "));
    cout << "\n";
  }

  if (R.size() != 1 || !similar(R[0], { Vector2(-10.0, -10.0), Vector2(10.0, -10.0), Vector2(10.0, 10.0), Vector2(-10.0, 10.0) }))
    cout << "** Self Intersection Error\n";

  cout << endl;
}


//|//////////////////// TestPolygonDecomp ///////////////////////////////////
void TestPolygonDecomp()
{
//  cout << "Test Polygon Decompositoin\n";

//  list<Vector2d> P;
////  P.push_back(Vector2(0.0, 0.0));
////  P.push_back(Vector2(10.0, 1.0));
////  P.push_back(Vector2(10.0, 10.0));
////  P.push_back(Vector2(0.0, 11.0));
//  P.push_back(Vector2(-5.0, 5.0));
//  P.push_back(Vector2(-8.0, 10.0));
//  P.push_back(Vector2(0.0, 0.0));
//  P.push_back(Vector2(0.0, 15.0));

//  vector<list<Vector2d>> contours;
//  contours.push_back(P);

//  lmlPolygonDecompImpl::Mesh<Vector2d> mesh;

//  lmlPolygonDecompImpl::triangulate(&mesh, contours);

//  cout << endl;
}


//|//////////////////// TestDelaunay2d //////////////////////////////////////
void TestDelaunay2d()
{
  cout << "Test Delaunay\n";

  auto m = RotationMatrix<double>(-0.3);

  list<Vector2d> P;
  for(double y = 0; y < 100; y += 25)
  {
    for(double x = 0; x < 100; x += 25)
    {
      P.push_back(m * Vector2(x, y));
    }
  }

  vector<list<Vector2d>> polygons;
  polygons.push_back(P);

  Delaunay2d::Mesh<Vector2d> mesh;

  Delaunay2d::triangulate(&mesh, polygons);

  cout << endl;
}


//|//////////////////// TestVoronoi2d ///////////////////////////////////////
void TestVoronoi2d()
{
  cout << "Test Voronoi\n";

  auto m = RotationMatrix<double>(0.8);

  vector<Vector2d> P;
  for(double y = 0; y < 100; y += 25)
  {
    for(double x = 0; x < 100; x += 25)
    {
      P.push_back(m * Vector2(x, y));
    }
  }

  Voronoi2d::Voronoi<Vector2d> voronoi;

  voronoi.add_sites(P.begin(), P.end());

  voronoi.calculate();

  cout << endl;
}


//|//////////////////// GeometryTest ////////////////////////////////////////
void GeometryTest()
{
  cout << "Geometry Test\n\n";

  TestBasic();

  TestIntersection();

  TestPolygon();

  TestPolygonDecomp();

  TestDelaunay2d();

  TestVoronoi2d();

  cout << endl;
}
