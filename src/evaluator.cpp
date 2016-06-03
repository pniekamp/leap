//
// Mathmatical Expression Evaluator
//
//   Peter Niekamp, September, 2000
//

//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#include "leap/lml/evaluator.h"
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <algorithm>

using namespace std;
using namespace leap;

const unsigned int kStackSize = 64;

enum class TokenType
{
  NoToken,
  OpToken,
  ArgToken,
};

enum class UnaryMask
{
  NoUnary = -2,
  NextUnary = -1,
};

const char *UnaryOps = "+ - abs";
const char *Operators[] = { "% / * abs", "+ -", "<= >= > <", "== != =", "&& || & |", "( )", "" };


//|---------- SimpleStack ----------
//|---------------------------------
template<typename T>
class SimpleStack
{
  public:
    SimpleStack()
    {
      m_head = 0;
    }

    size_t size()
    {
      return m_head;
    }

    T peek()
    {
      return m_stack[m_head-1];
    }

    T pop()
    {
      return m_stack[--m_head];
    }

    void push(T const &obj)
    {
      if (m_head < kStackSize)
        m_stack[m_head++] = obj;
    }

  protected:

    size_t m_head;
    T m_stack[kStackSize];
};



//|///////////////////////// strinpcmp //////////////////////////////////////
//|
//| Compares two string in a case insensitive manner
//| The first string determines the compare length
//|
static bool strinpcmp(const char *str1, const char *str2)
{
  while (tolower(*str1) == tolower(*str2) && *str1 != 0)
  {
    ++str1;
    ++str2;
  }

  return (*str1 == 0);
}


//|///////////////////////// IsOperator /////////////////////////////////////
//|
//| Determine if character c is an operator
//|
static int IsOperator(const char *c)
{
  for(int i = 0; Operators[i][0] != 0; ++i)
    for(int j = 0; Operators[i][j] != 0; ++j)
    {
      // Is it this op ?
      int k;
      for(k = 0; Operators[i][j+k] == *c && *c > ' '; k++, c++)
        ;

      if (k != 0)
        return k;

      // Skip to next op
      for(++j; Operators[i][j] > ' '; ++j)
        ;
    }

  return 0;
}


//|///////////////////////// IsUnaryOp //////////////////////////////////////
//|
//| Determine if character c is a uanry operator
//|
static int IsUnaryOp(const char *c)
{
  for(int i = 0; UnaryOps[i] != 0; ++i)
  {
    // Is it this op ?
    int k;
    for(k = 0; UnaryOps[i+k] == *c && *c > ' '; k++, c++)
      ;

    if (k != 0)
      return k;

    // Skip to next op
    for(++i; UnaryOps[i] > ' '; ++i)
      ;
  }

  return 0;
}


//|///////////////////////// IsArgument /////////////////////////////////////
//|
//| Determine if character c is an argument (number or variable)
//|
static int IsArgument(const char *c)
{
  int k = 0;

  if (isdigit(*c) || *c == '.')
  {
    // number
    while (isdigit(*c) || *c == '.')
    {
      ++k;
      ++c;
    }

    if (tolower(*c) == 'e')
    {
      while (tolower(*c) == 'e' || *c == '-' || *c == '+')
      {
        ++k;
        ++c;
      }

      while (isdigit(*c))
      {
        ++k;
        ++c;
      }
    }
  }
  else
  {
    // alpha
    while (isdigit(*c) || isalpha(*c) || *c == '@' || *c == '_' || *c == '.')
    {
      ++k;
      ++c;
    }

    if (*c == '[')
    {
      ++k;
      ++c;

      while (*c != 0 && *(c-1) != ']')
      {
        ++k;
        ++c;
      }
    }
  }

  return k;
}


//|///////////////////////// GetPrecedence //////////////////////////////////
//|
//| Determine operator precedence
//|
static int GetPrecedence(const char *op)
{
  for(int i = 0; Operators[i][0] != 0; ++i)
  {
    for(int j = 0; Operators[i][j] != 0; ++j)
    {
      if (*op == Operators[i][j])
        return i;

      for( ; Operators[i][j+1] != 0 && Operators[i][j] != ' '; ++j)
        ;
    }
  }

  return -1;
}



//|///////////////////////// GetToken ///////////////////////////////////////
//|
//| Retreives the next token from an expresion
//| Tokens are defined as expressions seperated by operators
//| or operaters themselves
//|
static TokenType GetToken(int &pos, const char *exp, int *tokenpos)
{
  // Skip Whitespaces...
  for( ; exp[pos] != 0 && exp[pos] <= ' '; ++pos)
    ;

  // Get Token Size
  int opcnt = IsOperator(&exp[pos]);
  int agcnt = IsArgument(&exp[pos]);

  // set token pos
  *tokenpos = pos;

  pos += max(opcnt, agcnt);

  if (opcnt > 0)
    return TokenType::OpToken;

  if (agcnt > 0)
    return TokenType::ArgToken;

  return TokenType::NoToken;
}


namespace leap { namespace lml
{

  //|------------------------- Evaluator ------------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////////// Evaluator::Constructor ///////////////////////
  Evaluator::Evaluator()
  {
    m_hook = NULL;
  }


  //|///////////////////////// Evaluator::add_variable //////////////////////
  ///
  /// Add a variable into the evaluators namespace
  /// Variables are accessed in expressions as \@name
  ///
  /// \param[in] name Variable Name
  /// \param[in] value Variable Value
  ///
  void Evaluator::add_variable(const char *name, double value)
  {
    //
    // Check if variable already in variable list
    //
    for(size_t i = 0; i < m_variables.size(); ++i)
    {
      if (strinpcmp(m_variables[i].name, name))
      {
        //
        // It was? Just update it's value
        //
        m_variables[i].value = value;
        return;
      }
    }

    VariableData vd;
    strncpy(vd.name, name, sizeof(vd.name));
    vd.value = value;

    //
    // Otherwise, Add in a new variable
    //
    m_variables.push_back(vd);
  }


  //|///////////////////////// Evaluator::remove_all_variables //////////////
  ///
  /// Removes all variables that have been defined
  ///
  void Evaluator::remove_all_variables()
  {
    m_variables.clear();
  }


  //|///////////////////////// Evaluator::define_evalhook ///////////////////
  ///
  /// Define a class implementing EvaluatorHook to evaluate variables
  /// not defined through AddVariable()
  ///
  /// \param[in] hook Variable Callback Hook
  ///
  void Evaluator::define_evalhook(EvaluatorHook *hook)
  {
    m_hook = hook;
  }


  //|///////////////////////// Evaluator::eval_argument /////////////////////
  //|
  //| Evaluates an argument (after it's been extracted and splitted enough)
  //| Will either convert text to a double or extract variable value
  //|
  double Evaluator::eval_argument(const char *arg) const
  {
    //
    // Is it a variable
    //
    if (arg[0] == '@')
    {
      //
      // Find Variable in list and return value
      //
      for(size_t i = 0; i < m_variables.size(); ++i)
        if (strinpcmp(m_variables[i].name, &arg[1]))
          return m_variables[i].value;

      //
      // Not in list? See if the EvalHook can help us
      //
      if (m_hook != NULL)
        return m_hook->eval_variable(arg);

      return 0;
    }
    else
    {
      return atof(arg);
    }
  }


  //|///////////////////////// Evaluator::eval_expression ///////////////////
  //|
  //| Evaluates an expression by applying left operator right
  //|
  double Evaluator::eval_expression(double left, const char *op, double right) const
  {
    //
    // Apply the operator
    //
    switch(op[0])
    {
      case '+' :
        return left + right;

      case '-' :
        return left - right;

      case '*' :
        return left * right;

      case '/' :
        return left / right;

      case '%' :
        return (int)left % (int)right;

      case '&' :
        return left && right;

      case '|' :
        return left || right;

      case '=' :
        return left == right;

      case '!' :
        return left != right;

      case '<' :
        if (op[1] == '=')
          return left <= right;
        else
          return left < right;

      case '>' :
        if (op[1] == '=')
          return left >= right;
        else
          return left > right;
    }

    if (strinpcmp("abs", op))
      return abs(right);

    return 0;
  }


  //|///////////////////////// Evaluator::evaluate //////////////////////////
  ///
  /// Evaluate a mathmatical expression (*, /, +, -)
  ///
  /// \param[in] expression The expression to Evaluate
  ///
  double Evaluator::evaluate(const char *expression) const
  {
    // Two Stacks
    SimpleStack<int> OperatorStack;
    SimpleStack<double> OperandStack;

    int tkpos, pos = 0;
    TokenType tktype;
    UnaryMask unaryop = UnaryMask::NextUnary;

    //
    // For each token
    //
    while ((tktype = GetToken(pos, expression, &tkpos)) != TokenType::NoToken)
    {
      switch(tktype)
      {
        case TokenType::OpToken :

          // Attempt to directly apply unary operators inline...
          if (unaryop == UnaryMask::NextUnary && IsUnaryOp(&expression[tkpos]))
          {
            OperandStack.push(0);
            OperatorStack.push(strlen(expression));
            OperatorStack.push(tkpos);
            break;
          }

          while (true)
          {
            // Anything We Can Do ?
            if (OperatorStack.size() == 0 || expression[tkpos] == '(')
            {
              OperatorStack.push(tkpos);
              break;
            }

            // Ensure Higher Precedence Ops Done First...
            if (GetPrecedence(&expression[OperatorStack.peek()]) > GetPrecedence(&expression[tkpos]))
            {
              OperatorStack.push(tkpos);
              break;
            }

            // End of Bracketed Expression ?
            if (expression[OperatorStack.peek()] == '(')
            {
              OperatorStack.pop();
              break;
            }

            // End of Unary Expression ?
            if (expression[OperatorStack.peek()] == 0)
            {
              OperatorStack.pop();
              continue;
            }

            // Lets Actually Evaluate Something...
            if (OperandStack.size() >= 2)
            {
              int op = OperatorStack.pop();
              double right = OperandStack.pop();
              double left = OperandStack.pop();

              OperandStack.push(eval_expression(left, &expression[op], right));
            }
            else
              break;
          }

          if (expression[tkpos] != '(' && expression[tkpos] != ')')
            unaryop = UnaryMask::NextUnary;

          break;

        case TokenType::ArgToken :

          OperandStack.push(eval_argument(&expression[tkpos]));

          unaryop = UnaryMask::NoUnary;

          break;

        case TokenType::NoToken :

          break;
      }
    }

    //
    // Finally, tidy up the stacks
    //
    while (OperatorStack.size() > 0)
    {
      int op = OperatorStack.pop();

      if (expression[op] == '(' || expression[op] == 0)
        op = OperatorStack.pop();

      if (OperandStack.size() >= 2)
      {
        double right = OperandStack.pop();
        double left = OperandStack.pop();

        OperandStack.push(eval_expression(left, &expression[op], right));
      }
    }

    // Return the Result
    return (OperandStack.size() > 0) ? OperandStack.pop() : 0;
  }

} } // namespace lml
