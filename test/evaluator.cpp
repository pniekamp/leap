//
// evaluator.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <leap/lml/evaluator.h>
#include <leap/util.h>

using namespace std;
using namespace leap;
using namespace leap::lml;

void EvaluatorTest();

void check(Evaluator const &evaluator, const char *expression, double result)
{
  if (!fcmp(evaluator.evaluate(expression), result, 1e-6))
  {
    cout << "** Eval Error " << expression << " == " << evaluator.evaluate(expression) << " != " << result << "\n";
  }
}

double eval_hook(Evaluator const &evaluator, const char *name, size_t len)
{
  if (name[0] == 'c' && name[1] == '[')
  {
    return evaluator.evaluate(name+2);
  }

  throw runtime_error("unknown variable");
}

//|//////////////////// EvaluatorTest ///////////////////////////////////////
void EvaluatorTest()
{
  cout << "Evaluator Test\n";

  Evaluator evaluator;

  evaluator.add_variable("true", 1);
  evaluator.add_variable("false", 0);
  evaluator.add_variable("pi", 3.141592);

  double x = 2;

  evaluator.add_variable("x", x);
  evaluator.add_variable("a.x", 5);
  evaluator.add_variable("a.y", 6);

  check(evaluator, "false", false);
  check(evaluator, "true", true);
  check(evaluator, "-2147483648", -2147483648);
  check(evaluator, "2147483647", 2147483647);
  check(evaluator, "-0", 0);
  check(evaluator, "+0", 0);
  check(evaluator, "-0.1234", -0.1234);
  check(evaluator, "+0.1234", 0.1234);
  check(evaluator, "-0.1234e2", -12.34);
  check(evaluator, "+0.1234e-3", 0.0001234);

  check(evaluator, "-1.78", -1.78);
  check(evaluator, "---1.78", -1.78);
  check(evaluator, "-(1 + 0.78)", -1.78);

  check(evaluator, "1+2*3", 7);
  check(evaluator, "1+2*3", 7);
  check(evaluator, "(1+2)*3", 9);
  check(evaluator, "(1+2)*(-3)", -9);
  check(evaluator, "2/4", 0.5);

  check(evaluator, "2*x*5", 2*x*5);
  check(evaluator, "2+x*3+x", 2+x*3+x);
  check(evaluator, "x+2+x*3", x+2+x*3);
  check(evaluator, "(2*x+1)*4", (2*x+1)*4);
  check(evaluator, "4*(2*x+1)", 4*(2*x+1));
  check(evaluator, "a.x + a.y", 11);
  check(evaluator, "pi", 3.141592);
  check(evaluator, "2*pi/2", 3.141592);

  check(evaluator, "(1*(2*(3*(4*(5*(6*(1+x)))))))", 2160);
  check(evaluator, "(1*(2*(3*(4*(5*(6*(7*(1+x))))))))", 15120);
  check(evaluator, "1+2-3*4/5*(2*(1-5+(3*7)*(4+6*7-3)))+12", -4300.2);
  check(evaluator, "(1/((((x+(((2.7*(((((pi*((((3.45*((pi+1)+pi))+x)+x)*1))+0.68)+2.7)+1)/1))+1)+x))+x)*1)-pi))", 0.00380592544);

  check(evaluator, "x == 1", 0);
  check(evaluator, "x == 2", 1);
  check(evaluator, "x < 5", 1);
  check(evaluator, "x > 0", 1);
  check(evaluator, "x != 2", 0);
  check(evaluator, "x >= 1 && x <= 3", 1);
  check(evaluator, "!(x >= 1 && x <= 3)", 0);
  check(evaluator, "(true && x >= 1)", 1);
  check(evaluator, "(x >= 1 && false)", 0);

  check(evaluator, "abs(9)", 9);
  check(evaluator, "abs(-9)", 9);

  check(evaluator, "sin(0.1)", sin(0.1));
  check(evaluator, "cos(0.1)", cos(0.1));
  check(evaluator, "tan(0.1)", tan(0.1));
  check(evaluator, "asin(0.1)", asin(0.1));
  check(evaluator, "acos(0.1)", acos(0.1));
  check(evaluator, "atan(0.1)", atan(0.1));

  check(evaluator, "pow(x, 2)", pow(x, 2));
  check(evaluator, "sqrt(4)", sqrt(4));

  check(evaluator, "if(true, 1, 0)", 1);
  check(evaluator, "if(false, 1, 0)", 0);

  evaluator.define_evalhook(&eval_hook);

  check(evaluator, "c[0]", 0);
  check(evaluator, "c[7]", 7);
  check(evaluator, "c[2*x+1]", 5);

  cout << "  pi = " << evaluator.evaluate("pi") << "\n";
  cout << "  2*pi = " << evaluator.evaluate("2*pi") << "\n";

  cout << endl;
}
