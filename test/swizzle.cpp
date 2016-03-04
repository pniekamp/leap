//
// swizzle.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <sstream>
#include <cstddef>
#include <leap/lml/vector.h>
#include <leap/lml/bound.h>
#include <leap/lml/geometry.h>
#include <leap/lml/io.h>
#include <leap/util.h>

using namespace std;
using namespace leap;
using namespace leap::lml;

void SwizzleTest();


class Vec2 : public VectorView<Vec2, float, 0, 1>
{
  public:
    Vec2(float x, float y)
      : x(x), y(y)
    {
    }

    union
    {
      struct
      {
        float x;
        float y;
      };
    };
};

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
        float x;
        float y;
        float z;
      };

      Vec2 xy;

      VectorView<Vec2, float, 0, 0> xx;
      VectorView<Vec2, float, 0, 2> xz;
      VectorView<Vec2, float, 1, 0> yx;
      VectorView<Vec2, float, 1, 1> yy;
      VectorView<Vec2, float, 1, 2> yz;
      VectorView<Vec2, float, 2, 0> zx;
      VectorView<Vec2, float, 2, 1> zy;
      VectorView<Vec2, float, 2, 2> zz;

      VectorView<Vec3, float, 0, 0, 0> xxx;
      VectorView<Vec3, float, 1, 1, 1> yyy;
      VectorView<Vec3, float, 2, 2, 2> zzz;
    };
};

class Vec4 : public VectorView<Vec4, float, 0, 1, 2, 3>
{
  public:
    Vec4(float x, float y, float z, float w = 1.0f)
      : x(x), y(y), z(z), w(w)
    {
    }

    Vec4 &operator =(Vec4 const &that)
    {
      x = that.x;
      y = that.y;
      z = that.z;
      w = that.w;

      return *this;
    }

    union
    {
      struct
      {
        float x;
        float y;
        float z;
        float w;
      };

      Vec2 xy;
      Vec3 xyz;

      VectorView<Vec2, float, 0, 0> xx;
      VectorView<Vec2, float, 0, 2> xz;
      VectorView<Vec2, float, 1, 0> yx;
      VectorView<Vec2, float, 1, 1> yy;
      VectorView<Vec2, float, 1, 2> yz;
      VectorView<Vec2, float, 2, 0> zx;
      VectorView<Vec2, float, 2, 1> zy;
      VectorView<Vec2, float, 2, 2> zz;

      VectorView<Vec3, float, 0, 0, 0> xxx;
      VectorView<Vec3, float, 0, 2, 1> xzy;
      VectorView<Vec3, float, 1, 0, 2> yxz;
      VectorView<Vec3, float, 1, 1, 1> yyy;
      VectorView<Vec3, float, 1, 2, 0> yzx;
      VectorView<Vec3, float, 2, 0, 1> zxy;
      VectorView<Vec3, float, 2, 1, 0> zyx;
      VectorView<Vec3, float, 2, 2, 2> zzz;

      VectorView<Vec4, float, 0, 0, 0, 0> xxxx;
      VectorView<Vec4, float, 1, 1, 1, 1> yyyy;
      VectorView<Vec4, float, 2, 2, 2, 2> zzzz;
      VectorView<Vec4, float, 3, 3, 3, 3> wwww;
    };
};

class Rect2 : public BoundView<Rect2, float, 2, 0, 1>
{
  public:
    Rect2(Vec2 const &min, Vec2 const &max)
      : left(min.x), bottom(min.y), right(max.x), top(max.y)
    {
    }

    Vec2 centre() const { return (min + max)/2; }
    Vec2 halfdim() const { return (max - min)/2; }

    VectorView<Vec2, float, 0, 3> const &topleft() const { return (VectorView<Vec2, float, 0, 3>&)(*this); }
    VectorView<Vec2, float, 2, 3> const &topright() const { return (VectorView<Vec2, float, 2, 3>&)(*this); }
    VectorView<Vec2, float, 0, 1> const &bottomleft() const { return (VectorView<Vec2, float, 0, 1>&)(*this); }
    VectorView<Vec2, float, 2, 1> const &bottomright() const { return (VectorView<Vec2, float, 2, 1>&)(*this); }

    float width() const { return right - left; }
    float height() const { return top - bottom; }

  union
  {
    struct
    {
      float left;
      float bottom;
      float right;
      float top;
    };
    
    VectorView<Vec2, float, 0, 1> min;
    VectorView<Vec2, float, 2, 3> max;
  };
};


//|//////////////////// SwizzleBasicTest ////////////////////////////////////
static void SwizzleBasicTest()
{
  cout << "Swizzle Test Set\n";

  Vec4 a(1, 2, 3, 4);

  cout << "  " << "(" << a.x << "," << a.y << "," << a.z << ") " << a.x + a.y + a.z << endl;
  cout << "  " << a.xx << " " << a.yy << " " << a.zz << endl;

  Vec4 b(a);

  a.x = 9;
  a.zy = b.zy();
  a.yz = b.zy();
  a.zy *= 2.5;

  b = a;

  if (a != b)
    cout << "** Equal Error!\n";

  if (b.x != 9 || b.y != 7.5 || b.z != 5.0 || b.w != 4.0)
    cout << "** Set Error!\n";

  a.yx = b.xy;

  if (a.xz != b.yz)
    cout << "** Equal Error!\n";

  if (sizeof(Vec4) != 4*sizeof(float))
    cout << "** Size Error!\n";

  cout << endl;
}


//|//////////////////// SwizzleBoundTest ////////////////////////////////////
static void SwizzleBoundTest()
{
  Rect2 b(Vec2(-4, -2), Vec2(10, 12));

  if (b.min != Vec2(-4, -2))
    cout << "** Min Error!\n";

  if (b.max != Vec2(10, 12))
    cout << "** Min Error!\n";

  if (b.topleft() != Vec2(-4, 12))
    cout << "** Top Left Error!\n";

  if (b.topright() != Vec2(10, 12))
    cout << "** Top Right Error!\n";

  if (b.bottomleft() != Vec2(-4, -2))
    cout << "** Bottom Left Error!\n";

  if (b.bottomright() != Vec2(10, -2))
    cout << "** Bottom Right Error!\n";

  cout << "  " << b.min << b.max << b.centre() << endl;
}


//|//////////////////// VectorTest //////////////////////////////////////////
void SwizzleTest()
{
  SwizzleBasicTest();

  SwizzleBoundTest();

  cout << endl;
}
