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
#include "leap/util.h"
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <cassert>

using namespace std;
using namespace leap;

const size_t StackSize = 64;

enum class TokenType
{
  NoToken,
  OpToken,
  ArgToken,
};

enum class OpType
{
  NoOp,
  PrefixOp,
  InfixOp
};

enum class OpCode
{
  mod, div, mul, abs, min, max, sin, cos, tan, asin, acos, atan, atan2, pow, sqrt, log, exp, log2, exp2, cond, plus, minus, leq, geq, le, ge, eq, neq, bnot, band, bor, open, close, comma,
};

const char *Operators[] = { "% / * abs min max sin cos tan asin acos atan atan2 pow sqrt log exp log2 exp2 if", "+ -", "<= >= < >", "== !=", "! && ||", "( )", ",", "" };

namespace
{
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
        assert(m_head < StackSize);

        if (m_head < StackSize)
          m_stack[m_head++] = obj;
      }

    protected:

      size_t m_head;
      T m_stack[StackSize];
  };


  //|///////////////////////// is_operator //////////////////////////////////
  int is_operator(const char *c, int *code, int *order, int *precedence, OpType optype)
  {
    *code = 0;

    for(int i = 0; Operators[i][0] != 0; ++i)
    {
      for(int j = 0; Operators[i][j] != 0; ++j)
      {
        int k;
        for(k = 0; Operators[i][j+k] == c[k] && Operators[i][j+k] > ' '; k++)
          ;

        if (Operators[i][j+k] <= ' ')
        {
          *order = 0;
          *precedence = i;

          if (optype == OpType::InfixOp)
          {
            *order = 2;
          }

          if (optype == OpType::PrefixOp)
          {
            switch(static_cast<OpCode>(*code))
            {
              case OpCode::plus:
              case OpCode::minus:
              case OpCode::abs:
              case OpCode::sin:
              case OpCode::cos:
              case OpCode::tan:
              case OpCode::asin:
              case OpCode::acos:
              case OpCode::atan:
              case OpCode::sqrt:
              case OpCode::log:
              case OpCode::exp:
              case OpCode::log2:
              case OpCode::exp2:
              case OpCode::bnot:
                *order = 1;
                break;

              case OpCode::min:
              case OpCode::max:
              case OpCode::atan2:
              case OpCode::pow:
                *order = 2;
                break;

              case OpCode::cond:
                *order = 3;
                break;

              default:
                break;
            }
          }

          return k;
        }

        // Skip to next op
        for( ; Operators[i][j+1] != 0 && Operators[i][j] != ' '; ++j)
          ;

        *code += 1;
      }
    }

    return 0;
  }


  //|///////////////////////// is_argument ////////////////////////////////////
  size_t is_argument(const char *c)
  {
    size_t k = 0;

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
      while (isdigit(*c) || isalpha(*c) || *c == '@' || *c == '$' || *c == '_' || *c == '.' || *c == '{' || *c == '}')
      {
        ++k;
        ++c;
      }

      if (*c == '[')
      {
        ++k;
        ++c;

        int nest = 1;

        while (*c != 0 && nest > 0)
        {
          if (*c == '[')
            ++nest;

          if (*c == ']')
            --nest;

          ++k;
          ++c;
        }
      }
    }

    return k;
  }


  //|///////////////////////// next_token ///////////////////////////////////
  TokenType next_token(size_t &pos, const char *exp, size_t *tokenpos, size_t *tokenlen, int *code, int *order, int *precedence, OpType optype)
  {
    for( ; exp[pos] != 0 && exp[pos] <= ' '; ++pos)
      ;

    size_t opcnt = is_operator(&exp[pos], code, order, precedence, optype);
    size_t agcnt = is_argument(&exp[pos]);

    *tokenpos = pos;

    pos += max(opcnt, agcnt);

    *tokenlen = pos - *tokenpos;

    if (opcnt > 0 && opcnt >= agcnt)
      return TokenType::OpToken;

    if (agcnt > 0)
      return TokenType::ArgToken;

    return TokenType::NoToken;
  }
}

namespace leap { namespace lml
{

  //|---------------------- Evaluator ---------------------------------------
  //|------------------------------------------------------------------------

  //|////////////////////// Evaluator::Constructor //////////////////////////
  Evaluator::Evaluator()
  {
  }


  //|////////////////////// Evaluator::add_variable /////////////////////////
  /// Add a variable into the evaluators namespace
  /// Variables are accessed in expressions as \@name
  ///
  /// \param[in] name Variable Name
  /// \param[in] value Variable Value
  ///
  void Evaluator::add_variable(leap::string_view name, double value)
  {
    if (name.size() >= sizeof(Variable::name))
      throw eval_error("name length overflow");

    for(size_t i = 0; i < m_variables.size(); ++i)
    {
      if (stricmp(leap::string_view(m_variables[i].name, m_variables[i].len), name) == 0)
      {
        m_variables[i].value = { value };
        return;
      }
    }

    Variable variable;
    variable.len = name.size();
    strlcpy(variable.name, name.data(), name.size()+1);
    variable.value = { value };

    m_variables.push_back(variable);
  }


  //|////////////////////// Evaluator::remove_all_variables /////////////////
  /// Removes all variables that have been defined
  ///
  void Evaluator::remove_all_variables()
  {
    m_variables.clear();
  }


  //|////////////////////// Evaluator::define_evalhook //////////////////////
  /// Define a class implementing EvaluatorHook to evaluate variables
  /// not defined through AddVariable()
  ///
  /// \param[in] hook Variable Callback Hook
  ///
  void Evaluator::define_evalhook(double(*hook)(Evaluator const &, const char *, size_t))
  {
    m_hook = hook;
  }


  //|////////////////////// Evaluator::eval_argument ////////////////////////
  Evaluator::Operand Evaluator::eval_argument(const char *arg, size_t len) const
  {
    if (isdigit(*arg) || *arg == '.')
    {
      return { atof(arg) };
    }

    for(size_t i = 0; i < m_variables.size(); ++i)
    {
      if (stricmp(leap::string_view(arg, len), leap::string_view(m_variables[i].name, m_variables[i].len)) == 0)
        return m_variables[i].value;
    }

    if (m_hook)
    {
      return { m_hook(*this, arg, len) };
    }

    throw eval_error("unknown variable");
  }


  //|////////////////////// Evaluator::eval_expression //////////////////////
  Evaluator::Operand Evaluator::eval_expression(Operator op, Operand first) const
  {
    switch(static_cast<OpCode>(op.code))
    {
      case OpCode::plus:
        return { first.value };

      case OpCode::minus:
        return { -first.value };

      case OpCode::abs:
        return { abs(first.value) };

      case OpCode::sin:
        return { sin(first.value) };

      case OpCode::cos:
        return { cos(first.value) };

      case OpCode::tan:
        return { tan(first.value) };

      case OpCode::asin:
        return { asin(first.value) };

      case OpCode::acos:
        return { acos(first.value) };

      case OpCode::atan:
        return { atan(first.value) };

      case OpCode::sqrt:
        return { sqrt(first.value) };

      case OpCode::log:
        return { log(first.value) };

      case OpCode::exp:
        return { exp(first.value) };

      case OpCode::log2:
        return { log2(first.value) };

      case OpCode::exp2:
        return { exp2(first.value) };

      case OpCode::bnot:
        return { double(fcmp(first.value, 0.0)) };

      default:
        throw eval_error("invalid operator");
    }
  }


  //|////////////////////// Evaluator::eval_expression //////////////////////
  Evaluator::Operand Evaluator::eval_expression(Operator op, Operand second, Operand first) const
  {
    switch(static_cast<OpCode>(op.code))
    {
      case OpCode::mod:
        return { fmod(first.value, second.value) };

      case OpCode::div:
        return { first.value / second.value };

      case OpCode::mul:
        return { first.value * second.value };

      case OpCode::plus:
        return { first.value + second.value };

      case OpCode::minus:
        return { first.value - second.value };

      case OpCode::leq:
        return { double(first.value <= second.value) };

      case OpCode::geq:
        return { double(first.value >= second.value) };

      case OpCode::le:
        return { double(first.value < second.value) };

      case OpCode::ge:
        return { double(first.value > second.value) };

      case OpCode::eq:
        return { double(fcmp(first.value, second.value)) };

      case OpCode::neq:
        return { double(!fcmp(first.value, second.value)) };

      case OpCode::band:
        return { double(!fcmp(first.value, 0.0) && !fcmp(second.value, 0.0)) };

      case OpCode::bor:
        return { double(!fcmp(first.value, 0.0) || !fcmp(second.value, 0.0)) };

      case OpCode::min:
        return { min(first.value, second.value) };

      case OpCode::max:
        return { max(first.value, second.value) };

      case OpCode::atan2:
        return { atan2(first.value, second.value) };

      case OpCode::pow:
        return { pow(first.value, second.value) };

      default:
        throw eval_error("invalid operator");
    }
  }


  //|////////////////////// Evaluator::eval_expression //////////////////////
  Evaluator::Operand Evaluator::eval_expression(Operator op, Operand third, Operand second, Operand first) const
  {
    switch(static_cast<OpCode>(op.code))
    {
      case OpCode::cond:
        return { fcmp(first.value, 0.0) ? third.value : second.value };

      default:
        throw eval_error("invalid operator");
    }
  }


  //|////////////////////// Evaluator::evaluate /////////////////////////////
  /// Evaluate a mathmatical expression (*, /, +, -)
  ///
  /// \param[in] expression The expression to Evaluate
  ///
  double Evaluator::evaluate(const char *expression) const
  {
    SimpleStack<Operator> operatorstack;
    SimpleStack<Operand> operandstack;

    TokenType tktype;
    size_t tkpos, tklen;
    Operator tkop;

    size_t pos = 0;
    OpType nextop = OpType::PrefixOp;

    while ((tktype = next_token(pos, expression, &tkpos, &tklen, &tkop.code, &tkop.order, &tkop.precedence, nextop)) != TokenType::NoToken)
    {
      switch(tktype)
      {
        case TokenType::OpToken:

          while (true)
          {
            if (nextop == OpType::PrefixOp)
            {
              operatorstack.push(tkop);
              break;
            }

            if (operatorstack.size() == 0)
            {
              operatorstack.push(tkop);
              break;
            }

            if (operatorstack.peek().precedence > tkop.precedence)
            {
              operatorstack.push(tkop);
              break;
            }

            if (operatorstack.peek().code == static_cast<int>(OpCode::open))
            {
              if (tkop.code != static_cast<int>(OpCode::comma))
                operatorstack.pop();
              break;
            }

            Operator op = operatorstack.pop();

            if (operandstack.size() < (size_t)op.order)
              throw eval_error("invalid unwind");

            switch(op.order)
            {
              case 1:
                operandstack.push(eval_expression(op, operandstack.pop()));
                break;

              case 2:
                //operandstack.push(eval_expression(op, operandstack.pop(), operandstack.pop())); // c++17
                {  auto a = operandstack.pop(); auto b = operandstack.pop(); operandstack.push(eval_expression(op, a, b)); }
                break;

              case 3:
                //operandstack.push(eval_expression(op, operandstack.pop(), operandstack.pop(), operandstack.pop())); // c++17
                {  auto a = operandstack.pop(); auto b = operandstack.pop(); auto c = operandstack.pop(); operandstack.push(eval_expression(op, a, b, c)); }
                break;
            }
          }

          if (expression[tkpos] != '(' && expression[tkpos] != ')')
            nextop = OpType::PrefixOp;

          break;

        case TokenType::ArgToken:

          operandstack.push({ eval_argument(&expression[tkpos], tklen) });

          nextop = OpType::InfixOp;

          break;

        case TokenType::NoToken:

          break;
      }
    }

    while (operatorstack.size() > 0)
    {
      Operator op = operatorstack.pop();

      if (operandstack.size() < (size_t)op.order)
        throw eval_error("invalid unwind");

      switch(op.order)
      {
        case 1:
          operandstack.push(eval_expression(op, operandstack.pop()));
          break;

        case 2:
//          operandstack.push(eval_expression(op, operandstack.pop(), operandstack.pop())); // c++17
          {  auto a = operandstack.pop(); auto b = operandstack.pop(); operandstack.push(eval_expression(op, a, b)); }
          break;

        case 3:
//          operandstack.push(eval_expression(op, operandstack.pop(), operandstack.pop(), operandstack.pop())); // c++17
          {  auto a = operandstack.pop(); auto b = operandstack.pop(); auto c = operandstack.pop(); operandstack.push(eval_expression(op, a, b, c)); }
          break;
      }
    }

    if (operatorstack.size() != 0 || operandstack.size() != 1)
      throw eval_error("invalid unwind");

    return operandstack.pop().value;
  }

} } // namespace lml
