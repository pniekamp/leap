//
// vector.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <sstream>
#include <leap/lml/vector.h>
#include <leap/lml/io.h>
#include <leap/util.h>

using namespace std;
using namespace leap;
using namespace leap::lml;

void VectorTest();

constexpr Vector2f gVector1{ 1.0f, 2.0f };
constexpr Vector2f gVector2 = Vector2(3.0f, 4.0f);
constexpr float gElement = gVector1(0) * gVector2(1);
//constexpr float gVecNorm = norm(gVector1);


//|//////////////////// VectorBasicTest /////////////////////////////////////
static void VectorBasicTest()
{
  cout << "Simple Vector Test Set\n";

  Vector3f P;
  if (P.size() != 3)
     cout << "** Size Error!\n";

  cout << "  Size : " << P.size() << "\n";

  P(0) = 1;
  P(1) = 2;
  P(2) = 3;

  if (P(0) != 1 || P(1) != 2 || P(2) != 3)
    cout << "** Set Error!\n";

  cout << "  Data P() : ";
  for(size_t i = 0; i < P.size(); ++i)
    cout << P(i) << " ";
  cout << "\n";

  Vector3f Q = P;
  if (Q(0) != 1 || Q(1) != 2 || Q(2) != 3)
    cout << "** Copy Constructor Error!\n";

  Q(0) = 4;
  if (Q(0) != 4 || P(0) != 1)
    cout << "** Copy Constructor Set Error!\n";

  Q = P;
  if (Q(0) != 1 || Q(1) != 2 || Q(2) != 3)
    cout << "** Copy Operator Error!\n";

  Q(0) = 4;
  Q(1) = 5;
  Q(2) = 6;
  if (Q(0) != 4 || Q(1) != 5 || Q(2) != 6 || P(0) != 1 || P(1) != 2 || P(2) != 3)
    cout << "** Copy Operator Set Error!\n";

  Q = Q;
  if (Q(0) != 4 || Q(1) != 5 || Q(2) != 6)
    cout << "** Self Copy Error!\n";

//  Vector3d R = P;
//  if (R(0) != 1 || R(1) != 2 || R(2) != 3)
//    cout << "** Cross Copy Constructor Error!\n";

//  R = Q;
//  if (R(0) != 4 || R(1) != 5 || R(2) != 6)
//    cout << "** Cross Copy Operator Error!\n";

//  if (Q != R)
//    cout << "** Equality Test Error!\n";

  swap(P, Q);
  if (P(0) != 4 || P(1) != 5 || P(2) != 6 || Q(0) != 1 || Q(1) != 2 || Q(2) != 3)
    cout << "** Swap Error!\n";

  swap(P, P);
  if (P(0) != 4 || P(1) != 5 || P(2) != 6)
    cout << "** Self Swap Error!\n";

  if (gElement != 4)
    cout << "** Error in Vector Constants\n";

  cout << endl;
}


class Vec3 : public Vector3f
{
  public:
    Vec3(float x, float y, float z)
      : Vector3f({ x, y, z })
    {
    }
};


//|//////////////////// VectorMathTest //////////////////////////////////////
static void VectorMathTest()
{
  cout << "Vector Math Test Set\n";

  Vector3d A, B, C;

  for(int i = 0; i < 3; ++i)
  {
    A(i) = i;
    B(i) = i+1;
    C(i) = (i==0) ? 0.5 : i*2;
  }

  cout << "    " << A << " + " << B << "   = " << A+B << "\n";
  if (A+B != Vector3(1.0, 3.0, 5.0))
    cout << "** Error in Vector addition\n";

  cout << "  " << C << " - " << B << "   = " << C-B << "\n";
  if (C-B != Vector3(-0.5, 0.0, 1.0))
    cout << "** Error in Vector subtraction\n";

  cout << "          5 * " << C << " = " << 5*C << "\n";
  if (5*C != 5.0*C || 5*C != C*5 || 5*C != Vector3(2.5, 10.0, 20.0))
    cout << "** Error in Vector scaler multiplication\n";

  cout << "  " << C << " / 5         = " << C/5 << "\n";
  if (C/5 != C/5.0 || C/5 != Vector3(0.1, 0.4, 0.8))
    cout << "** Error in Vector scaler multiplication\n";

// This test gives a warning about conversion to int
//  Vector<3, int> G = Vector3(1, 2, 3);
//  cout << "        1.5 * " << G << "   = " << 1.5f*G << "\n";
//  if (1.5f*G != Vector3(1, 3, 4))
//    cout << "** Error in integer Vector scaler multiplication\n";

  cout << "  norm(" << C << ") = " << norm(C) << "\n";
  if (norm(C) != 4.5)
    cout << "** Error in Vector norm\n";

  cout << "  dot(" << A << ", " << C << ") = " << dot(A, C) << "\n";
  if (dot(A, C) != 10)
    cout << "** Error in Vector dot product\n";

  cout << "  clamp(" << A << ", 0, 1) = " << clamp(A, 0.0, 1.0) << "\n";

  cout << "  swizzle<2,0,1>(" << A << ") = " << swizzle<2,0,1>(A) << "\n";

  Vec3 pt(1, 2, 3);

  cout << "  Point (Vector derivative) : " << pt + pt << " (" << sizeof(pt) << " bytes)" << " [" << get<0>(pt) << "," << get<1>(pt) << "," << get<2>(pt) << "]\n";

  cout << endl;
}


//|//////////////////// lmlioTest ///////////////////////////////////////////
static void lmlioTest()
{
  cout << "lml IO Test Set\n";

  istringstream is;

  Vector<double, 5> P;
  for(int i = 0; i < 5; ++i)
    P(i) = i / 10.0;

  cout << "  Output : " << P << "\n";

  is.str("(1,2.3,-3.12,9e-1,10)");
  if (!(is >> P) || P(0) != 1.0 || P(1) != 2.3 || P(2) != -3.12 || P(3) != 0.9 || P(4) != 10)
    cout << "** Read without spaces Error!\n";

  is.str("  (  1 , -2 , +3.911 , 8e-2, 11 )  ");
  if (!(is >> P) || P(0) != 1.0 || P(1) != -2.0 || P(2) != 3.911 || P(3) != 0.08 || P(4) != 11)
    cout << "** Read with spaces Error!\n";

  cout << "  Input  : " << P << "\n";

  cout << endl;
}


//|//////////////////// VectorTest //////////////////////////////////////////
void VectorTest()
{
  VectorBasicTest();

  VectorMathTest();

  lmlioTest();

  cout << endl;
}
