//
// matrix.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <sstream>
#include <ctime>
#include <leap/lml/io.h>
#include <leap/lml/vector.h>
#include <leap/lml/matrix.h>
#include <leap/lml/quaternion.h>
#include <leap/lml/matrixconstants.h>
#include <leap/util.h>

using namespace std;
using namespace leap;
using namespace leap::lml;

void MatrixTest();

//|//////////////////// SimpleMatrixTest ////////////////////////////////////
static void SimpleMatrixTest()
{
  cout << "Simple Matrix Test Set\n";

  Matrix<float, 3, 4> P;
  if (P.rows() != 3 || P.columns() != 4)
     cout << "** Size Error!\n";

  cout << "  Size : " << P.rows() << "," << P.columns() << "\n";

  for(size_t i = 0; i < P.rows(); ++i)
    for(size_t j = 0; j < P.columns(); ++j)
      P(i, j) = i*10.0f + j;

  if (P(0,0) != 0 || P(0,1) != 1 || P(0,2) != 2 || P(1,2) != 12 || P(2,1) != 21)
    cout << "** Set Error!\n";

  cout << "  Data P() : ";
  for(size_t i = 0; i < P.rows(); ++i)
  {
    for(size_t j = 0; j < P.columns(); ++j)
      cout << P(i, j) << " ";
  }
  cout << "\n";

  Matrix<float, 3, 4> Q = P;
  if (Q(0,0) != 0 || Q(0,1) != 1 || Q(0,2) != 2 || Q(1,2) != 12 || Q(2,1) != 21)
    cout << "** Copy Constructor Error!\n";

  Q(1,2) = 4;
  if (Q(1,2) != 4 || P(1, 2) != 12)
    cout << "** Copy Constructor Set Error!\n";

  Q = P;
  if (Q(0,0) != 0 || Q(0,1) != 1 || Q(0,2) != 2 || Q(1,2) != 12 || Q(2,1) != 21)
    cout << "** Copy Operator Error!\n";

  Q(0,0) = 4;
  Q(1,2) = 5;
  Q(2,3) = 6;
  if (Q(0,0) != 4 || Q(1,2) != 5 || Q(2,3) != 6 || P(0,0) != 0 || P(1,2) != 12 || P(2,3) != 23)
    cout << "** Copy Operator Set Error!\n";

  Q = Q;
  if (Q(0,0) != 4 || Q(1,2) != 5 || Q(2,3) != 6)
    cout << "** Self Copy Error!\n";

  Matrix<double, 3, 4> R = P;
  if (R(0,0) != 0 || R(0,1) != 1 || R(0,2) != 2 || R(1,2) != 12 || R(2,1) != 21)
    cout << "** Cross Copy Constructor Error!\n";

  R = Q;
  if (R(0,0) != 4 || R(1,2) != 5 || R(2,3) != 6)
    cout << "** Cross Copy Operator Error!\n";

  if (Q != R)
    cout << "** Equality Test Error!\n";

  swap(P, Q);
  if (P(0,0) != 4 || P(1,2) != 5 || P(2,3) != 6 || Q(0,0) != 0 || Q(1,2) != 12 || Q(2,1) != 21)
    cout << "** Swap Error!\n";

  swap(P, P);
  if (P(0,0) != 4 || P(1,2) != 5 || P(2,3) != 6)
    cout << "** Self Swap Error!\n";

  cout << endl;
}


//|//////////////////// MatrixMathTest //////////////////////////////////////
static void MatrixMathTest()
{
  cout << "Matrix Math Test Set\n";

  Matrix<double, 3, 2> A;
  Matrix<double, 3, 2> B;
  Matrix<double, 2, 3> C;

  for(int i = 0; i < 3; ++i)
  {
    for(int j = 0; j < 2; ++j)
    {
      A(i, j) = i+j;
      B(i, j) = i*2+j;
    }
  }

  for(int i = 0; i < 2; ++i)
  {
    for(int j = 0; j < 3; ++j)
    {
      C(i, j) = (i==0) ? 0.5 : i*2 + j;
    }
  }

  cout << "  " << A << " + " << B << "      = " << A+B << "\n";
  if (toa(A+B) != "[(0,2)(3,5)(6,8)]")
    cout << "** Error in Matrix addition\n";

  cout << "  " << B << " - " << A << "      = " << B-A << "\n";
  if (toa(B-A) != "[(0,0)(1,1)(2,2)]")
    cout << "** Error in Matrix subtraction\n";

  cout << "  " << A << " * " << C << " = " << A*C << "\n";
  if (toa(A*C) != "[(2,3,4)(4.5,6.5,8.5)(7,10,13)]")
    cout << "** Error in Matrix multiplication\n";

  cout << "                  5 * " << C << " = " << 5*C << "\n";
  if (5*C != 5.0*C || 5*C != C*5 || toa(5*C) != "[(2.5,2.5,2.5)(10,15,20)]")
    cout << "** Error in Matrix scaler multiplication\n";

  cout << "                           " << A << "<->" << transpose(A) << "\n";
  if (toa(transpose(A)) != "[(0,1,2)(1,2,3)]")
    cout << "** Error in Matrix transpose\n";

  Vector2d V = Vector2(1.0, 2.0);

  cout << "  " << A << " * " << V << "                  = " << A*V << "\n";
  if (A*V != Vector3(2.0, 5.0, 8.0))
    cout << "** Error in Matrix-Vector multiplication\n";

  Matrix2d F = { { 1, 2}, { 3, 4 } };

  cout << "                   " << "determinant(" << F << ")" << " = " << determinant(F) << endl;
  if (determinant(F) != -2)
    cout << "** Error in Matrix2x2 determinant\n";

  cout << "                       " << "inverse(" << F << ")" << " = " << inverse(F) << endl;
  if (inverse(F) != Matrix2d{ -2, 1, 1.5, -0.5 })
    cout << "** Error in Matrix2x2 inverse\n";

  Matrix3d G = { { 1, 3, 9 }, { 2, 5, 3 }, { 7, 4, 6 } };

  cout << "        " << "determinant(" << G << ")" << " = " << determinant(G) << endl;
  if (determinant(G) != -198)
    cout << "** Error in Matrix3x3 determinant\n";

  cout << endl;
}


//|//////////////////// MatrixConstantTest //////////////////////////////////
static void MatrixConstantTest()
{
  cout << "Matrix Constants\n";

  auto Z = ZeroMatrix<float, 5, 2>();
  cout << "  Zero Matrix : " << Z << "\n";

  auto I = IdentityMatrix<float, 3>();
  cout << "  Identity Matrix : " << I << "\n";

  auto R = RotationMatrix(Vector3(0.0f, 0.0f, 1.0f), 3.14f/2);
  cout << "  Rotation Matrix : " << R << "\n";

  auto Q = RotationMatrix(Quaternion<float>(Vector3(0.0f, 0.0f, 1.0f), 3.14f/2));
  cout << "  Rotation Matrix : " << Q << "\n";

  cout << endl;
}


//|//////////////////// lmlioTest ///////////////////////////////////////////
static void lmlioTest()
{
  cout << "lml IO Test Set\n";

  istringstream is;

  Matrix<double, 2, 3> P;
  for(int i = 0; i < 2; ++i)
    for(int j = 0; j < 3; ++j)
      P(i, j) = j / 10.0 + i;

  cout << "  Output : " << P << "\n";

  is.str("[(1,2.3,-3.12)(0, 1, 2)]");
  if (!(is >> P) || P(0,0) != 1.0 || P(0,1) != 2.3 || P(0,2) != -3.12 || P(1,0) != 0 || P(1,1) != 1 || P(1,2) != 2)
    cout << "** Read without spaces Error!\n";

  is.str(" [  (  1 , -2 , +3.911  ) (  -1.2 , +2.12 , .11  ) ] ");
  if (!(is >> P) || P(0,0) != 1.0 || P(0,1) != -2.0 || P(0,2) != 3.911 || P(1,0) != -1.2 || P(1,1) != 2.12 || P(1,2) != 0.11)
    cout << "** Read with spaces Error!\n";

  cout << "  Input  : " << P << "\n";

  cout << endl;
}


//|//////////////////// QuaternionTest //////////////////////////////////////
static void QuaternionTest()
{
  cout << "Quaternion Test Set\n";

  Quaternion<double> P(xUnit3d, 3.14);

  cout << "  " << P << " : " << norm(P) << "\n";

  if (norm(P) != 1.0)
    cout << "** Quaternion not normalised\n";

  Quaternion<double> Q = conjugate(P);

  cout << "  " << Q << " : " << norm(Q) << "\n";

  if (norm(Q) != 1.0)
    cout << "** Quaternion copy or conjugate error\n";

  Quaternion<double> R = P * Q;

  cout << "  " << R << " : " << norm(R) << "\n";

  cout << "  " << Quaternion<double>(zUnit3d, 3.14/2) * Vector3(2.0, 0.0, 0.0);
  cout << "  " << Quaternion<double>(zUnit3d, 3.14) * Vector3(2.0, 0.0, 0.0);
  cout << "  " << Quaternion<double>(zUnit3d, -3.14/2) * Vector3(2.0, 0.0, 0.0) << "\n";
  cout << "  " << Quaternion<double>(zUnit3d, 3.14/2) * Vector3(0.0, 2.0, 0.0);
  cout << "  " << Quaternion<double>(zUnit3d, 3.14) * Vector3(0.0, 2.0, 0.0);
  cout << "  " << Quaternion<double>(zUnit3d, -3.14/2) * Vector3(0.0, 2.0, 0.0) << "\n";
  cout << "  " << Quaternion<double>(zUnit3d, 3.14/2) * Vector3(0.0, 0.0, 2.0);
  cout << "  " << Quaternion<double>(zUnit3d, 3.14) * Vector3(0.0, 0.0, 2.0);
  cout << "  " << Quaternion<double>(zUnit3d, -3.14/2) * Vector3(0.0, 0.0, 2.0) << "\n";

  cout << endl;
}


//|//////////////////// MatrixTest //////////////////////////////////////////
void MatrixTest()
{
  SimpleMatrixTest();

  MatrixMathTest();

  MatrixConstantTest();

  lmlioTest();

  QuaternionTest();
}
