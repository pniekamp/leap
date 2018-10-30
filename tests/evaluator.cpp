//
// evaluator.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <map>
#include <leap/lml/evaluator.h>
#include <leap/util.h>

using namespace std;
using namespace leap;
using namespace leap::lml;

void EvaluatorTest();

template<typename Scope>
void check(Scope const &scope, const char *expression, double result)
{
  if (!fcmp(eval(scope, expression), result, 1e-6))
  {
    cout << "** Eval Error " << expression << " == " << eval(scope, expression) << " != " << result << "\n";
  }
}

//|//////////////////// EvaluatorTest ///////////////////////////////////////
void EvaluatorTest()
{
  cout << "Evaluator Test\n";

  struct Scope
  {
    std::map<std::string, double, std::less<>> variables;

    double lookup(leap::string_view name) const
    {
      auto j = variables.find(name);

      if (j != variables.end())
        return j->second;

      if (name[0] == 'c' && name[1] == '[')
      {
        return eval(*this, name.substr(2, name.size()-3));
      }

      throw Expression::eval_error("Unknown Variable");
    }

  } scope;

  scope.variables["true"] = 1;
  scope.variables["false"] = 0;
  scope.variables["pi"] = 3.141592;

  double x = 2;

  scope.variables["x"] = x;
  scope.variables["a.x"] = 5;
  scope.variables["a.y"] = 6;

  check(scope, "false", false);
  check(scope, "true", true);
  check(scope, "-2147483647", -2147483647);
  check(scope, "2147483647", 2147483647);
  check(scope, "-0", 0);
  check(scope, "+0", 0);
  check(scope, "-0.1234", -0.1234);
  check(scope, "+0.1234", 0.1234);
  check(scope, "-0.1234e2", -12.34);
  check(scope, "+0.1234e-3", 0.0001234);

  check(scope, "-1.78", -1.78);
  check(scope, "---1.78", -1.78);
  check(scope, "-(1 + 0.78)", -1.78);

  check(scope, "1+2*3", 7);
  check(scope, "1+(2*3)", 7);
  check(scope, "(1+2)*3", 9);
  check(scope, "(1+2)*(-3)", -9);
  check(scope, "2/4", 0.5);

  check(scope, "2*x*5", 2*x*5);
  check(scope, "2+x*3+x", 2+x*3+x);
  check(scope, "x+2+x*3", x+2+x*3);
  check(scope, "(2*x+1)*4", (2*x+1)*4);
  check(scope, "4*(2*x+1)", 4*(2*x+1));
  check(scope, "a.x + a.y", 11);
  check(scope, "pi", 3.141592);
  check(scope, "2*pi/2", 3.141592);

  check(scope, "(1*(2*(3*(4*(5*(6*(1+x)))))))", 2160);
  check(scope, "(1*(2*(3*(4*(5*(6*(7*(1+x))))))))", 15120);
  check(scope, "1+2-3*4/5*(2*(1-5+(3*7)*(4+6*7-3)))+12", -4300.2);
  check(scope, "(1/((((x+(((2.7*(((((pi*((((3.45*((pi+1)+pi))+x)+x)*1))+0.68)+2.7)+1)/1))+1)+x))+x)*1)-pi))", 0.00380592544);

  check(scope, "x == 1", 0);
  check(scope, "x == 2", 1);
  check(scope, "x < 5", 1);
  check(scope, "x > 0", 1);
  check(scope, "x != 2", 0);
  check(scope, "x >= 1 && x <= 3", 1);
  check(scope, "!(x >= 1 && x <= 3)", 0);
  check(scope, "(true && x >= 1)", 1);
  check(scope, "(x >= 1 && false)", 0);

  check(scope, "abs(9)", 9);
  check(scope, "abs(-9)", 9);
  check(scope, "-abs(-9)", -9);

  check(scope, "sin(0.1)", sin(0.1));
  check(scope, "cos(0.1)", cos(0.1));
  check(scope, "tan(0.1)", tan(0.1));
  check(scope, "asin(0.1)", asin(0.1));
  check(scope, "acos(0.1)", acos(0.1));
  check(scope, "atan(0.1)", atan(0.1));

  check(scope, "floor(1.6)", floor(1.6));
  check(scope, "ceil(1.6)", ceil(1.6));
  check(scope, "round(1.6)", round(1.6));
  check(scope, "floor(-1.6)", floor(-1.6));
  check(scope, "trunc(-1.6)", trunc(-1.6));

  check(scope, "pow(x, 2)", pow(x, 2));
  check(scope, "sqrt(4)", sqrt(4));

  check(scope, "min(0.1 + x * 3, 0.2 * x)", 0.2 * x);
  check(scope, "max(0.1 + x * 3, 0.2 * x)", 0.1 + x * 3);
  check(scope, "clamp(x, 0.0, 1.0)", 1.0);

  check(scope, "if(true, 1, 0)", 1);
  check(scope, "if(false, 1, 0)", 0);

  check(scope, "c[0]", 0);
  check(scope, "c[7]", 7);
  check(scope, "c[2*x+1]", 5);

  cout << "  pi = " << eval(scope, "pi") << "\n";
  cout << "  2*pi = " << eval(scope, "2*pi") << "\n";

  cout << endl;
}
