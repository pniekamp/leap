//
// Mathmatical Expression Evaluator
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef LMLEVALUATOR_HH
#define LMLEVALUATOR_HH

#include <leap/util.h>
#include <leap/stringview.h>
#include <stdexcept>
#include <cassert>
#include <cmath>

/**
 * \namespace leap::lml
 * \brief Leap Math Library containing mathmatical routines
 *
**/

namespace leap { namespace lml
{
  //|-------------------- Evaluator -----------------------------------------
  //|------------------------------------------------------------------------
  /**
   * \brief Mathmatical Expression Evaluator
   *
   * A class to evaluate mathmatical expressions
   *
   * Supports unary +, unary -, +, -, *, /, order of precedence, ( )
   * and variables
   *
   * Typical Use :
   * \code
   *   {
   *     struct Scope
   *     {
   *       double lookup(leap::string_view name) const
   *       {
   *         return variable[name];
   *       }
   *     };
   *
   *     Scope scope;
   *
   *     double result = Expr::evaluate(scope, "15 + input * (8/9)");
   *   }
   * \endcode
  **/

  class Expr
  {
    public:

      class eval_error : public std::logic_error
      {
        public:
          eval_error(std::string const &arg)
            : logic_error(arg)
          {
          }
      };

      template<typename Scope>
      static double evaluate(Scope const &scope, leap::string_view expression);

    private:

      static constexpr size_t StackSize = 64;

      enum class TokenType
      {
        NoToken,
        OpToken,
        Literal,
        Identifier
      };

      enum class OpType
      {
        NoOp,
        PrefixOp,
        InfixOp
      };

      enum class OpCode
      {
        mod, div, mul, abs, min, max, floor, ceil, round, trunc, sin, cos, tan, asin, acos, atan, atan2, pow, sqrt, log, exp, log2, exp2, cond, plus, minus, leq, geq, le, ge, eq, neq, bnot, band, bor, open, close, comma
      };

      static constexpr const char *Operators[8] = { "% / * abs min max floor ceil round trunc sin cos tan asin acos atan atan2 pow sqrt log exp log2 exp2 if ", "+ - ", "<= >= < > ", "== != ", "! && || ", "( ) ", ", ", "" };

      static constexpr size_t is_literal(leap::string_view str);
      static constexpr size_t is_identifier(leap::string_view str);
      static constexpr size_t is_operator(leap::string_view str, OpCode *code, size_t *order, size_t *precedence, OpType optype);

      struct Operator
      {
        OpCode code;
        size_t order;
        size_t precedence;
      };

      struct Operand
      {
        double value;
      };

      template<typename T>
      class SimpleStack
      {
        public:

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

          size_t m_head = 0;
          T m_stack[StackSize];
      };

      static Operand eval_unary_operator(Operator op, SimpleStack<Operand> &operandstack);
      static Operand eval_binary_operator(Operator op, SimpleStack<Operand> &operandstack);
      static Operand eval_ternary_operator(Operator op, SimpleStack<Operand> &operandstack);
  };


  //|////////////////////// Expr::is_literal //////////////////////////////////
  constexpr size_t Expr::is_literal(leap::string_view str)
  {
    auto ch = str.begin();

    while (ch != str.end() && (std::isdigit(*ch) || *ch == '.'))
      ++ch;

    if (ch != str.begin())
    {
      if (ch != str.end() && std::tolower(*ch) == 'e')
      {
        ++ch;

        if (ch != str.end() && (*ch == '-' || *ch == '+'))
          ++ch;

        while (ch != str.end() && std::isdigit(*ch))
          ++ch;
      }
    }

    return ch - str.begin();
  }


  //|////////////////////// Expr::is_identifier /////////////////////////////
  constexpr size_t Expr::is_identifier(leap::string_view str)
  {
    auto ch = str.begin();

    if (ch != str.end() && (std::isalpha(*ch) || *ch == '@' || *ch == '$' || *ch == '_' || *ch == '{' || *ch == '}'))
    {
      ++ch;

      while (ch != str.end() && (std::isdigit(*ch) || std::isalpha(*ch) || *ch == '@' || *ch == '$' || *ch == '_' || *ch == '.' || *ch == '{' || *ch == '}'))
        ++ch;

      if (ch != str.end() && *ch == '[')
      {
        ++ch;

        int nest = 1;

        while (ch != str.end() && nest > 0)
        {
          if (*ch == '[')
            ++nest;

          if (*ch == ']')
            --nest;

          ++ch;
        }
      }
    }

    return ch - str.begin();
  }

  //|////////////////////// Expr::is_operator /////////////////////////////////
  constexpr size_t Expr::is_operator(leap::string_view str, OpCode *code, size_t *order, size_t *precedence, OpType optype)
  {
    *code = static_cast<OpCode>(0);

    for(int i = 0; Operators[i][0] != 0; ++i)
    {
      for(int j = 0; Operators[i][j] != 0; ++j)
      {
        int k = 0;
        while (Operators[i][j+k] != ' ')
          ++k;

        if (str.substr(0, k) == leap::string_view(Operators[i] + j, k))
        {
          *order = 0;
          *precedence = i;

          if (optype == OpType::InfixOp)
          {
            *order = 2;
          }

          if (optype == OpType::PrefixOp)
          {
            switch(*code)
            {
              case OpCode::plus:
              case OpCode::minus:
              case OpCode::abs:
              case OpCode::floor:
              case OpCode::ceil:
              case OpCode::round:
              case OpCode::trunc:
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

        j += k;

        *code = static_cast<OpCode>(static_cast<int>(*code) + 1);
      }
    }

    return 0;
  }


  //|////////////////////// Expr::eval_operator /////////////////////////////
  inline Expr::Operand Expr::eval_unary_operator(Operator op, SimpleStack<Operand> &operandstack)
  {
    auto first = operandstack.pop();

    switch (op.code)
    {
      case OpCode::plus:
        return { first.value };

      case OpCode::minus:
        return { -first.value };

      case OpCode::abs:
        return { std::abs(first.value) };

      case OpCode::floor:
        return { std::floor(first.value) };

      case OpCode::ceil:
        return { std::ceil(first.value) };

      case OpCode::round:
        return { std::round(first.value) };

      case OpCode::trunc:
        return { std::trunc(first.value) };

      case OpCode::sin:
        return { std::sin(first.value) };

      case OpCode::cos:
        return { std::cos(first.value) };

      case OpCode::tan:
        return { std::tan(first.value) };

      case OpCode::asin:
        return { std::asin(first.value) };

      case OpCode::acos:
        return { std::acos(first.value) };

      case OpCode::atan:
        return { std::atan(first.value) };

      case OpCode::sqrt:
        return { std::sqrt(first.value) };

      case OpCode::log:
        return { std::log(first.value) };

      case OpCode::exp:
        return { exp(first.value) };

      case OpCode::log2:
        return { std::log2(first.value) };

      case OpCode::exp2:
        return { std::exp2(first.value) };

      case OpCode::bnot:
        return { double(fcmp(first.value, 0.0)) };

      default:
        throw eval_error("invalid operator");
    }
  }


  //|////////////////////// Expr::eval_operator /////////////////////////////
  inline Expr::Operand Expr::eval_binary_operator(Operator op, SimpleStack<Operand> &operandstack)
  {
    auto second = operandstack.pop();
    auto first = operandstack.pop();

    switch (op.code)
    {
      case OpCode::mod:
        return { std::fmod(first.value, second.value) };

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
        return { std::min(first.value, second.value) };

      case OpCode::max:
        return { std::max(first.value, second.value) };

      case OpCode::atan2:
        return { std::atan2(first.value, second.value) };

      case OpCode::pow:
        return { std::pow(first.value, second.value) };

      default:
        throw eval_error("invalid operator");
    }
  }


  //|////////////////////// Expr::eval_operator ///////////////////////////////
  inline Expr::Operand Expr::eval_ternary_operator(Operator op, SimpleStack<Operand> &operandstack)
  {
    auto third = operandstack.pop();
    auto second = operandstack.pop();
    auto first = operandstack.pop();

    switch (op.code)
    {
      case OpCode::cond:
        return { fcmp(first.value, 0.0) ? third.value : second.value };

      default:
        throw eval_error("invalid operator");
    }
  }

  //|////////////////////// Expr::evaluate //////////////////////////////////
  template<typename Scope>
  double Expr::evaluate(Scope const &scope, leap::string_view expression)
  {
    SimpleStack<Operator> operatorstack;
    SimpleStack<Operand> operandstack;

    size_t pos = 0;
    OpType nextop = OpType::PrefixOp;

    while (pos < expression.size())
    {
      while (pos < expression.size() && expression[pos] <= ' ')
        ++pos;

      Operator tkop;
      TokenType tktype = TokenType::NoToken;

      size_t licnt = is_literal(expression.substr(pos));
      size_t idcnt = is_identifier(expression.substr(pos));
      size_t opcnt = is_operator(expression.substr(pos), &tkop.code, &tkop.order, &tkop.precedence, nextop);

      size_t tklen = std::max({ opcnt, licnt, idcnt });

      if (opcnt > 0 && opcnt >= idcnt)
        tktype = TokenType::OpToken;

      else if (licnt > 0)
        tktype = TokenType::Literal;

      else if (idcnt > 0)
        tktype = TokenType::Identifier;

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

            if (operatorstack.size() == 0 || operatorstack.peek().precedence > tkop.precedence)
            {
              operatorstack.push(tkop);

              break;
            }

            if (operatorstack.peek().code == OpCode::open)
            {
              if (tkop.code != OpCode::comma)
                operatorstack.pop();

              break;
            }

            Operator op = operatorstack.pop();

            if (operandstack.size() < op.order)
              throw eval_error("invalid unwind");

            switch(op.order)
            {
              case 1:
                operandstack.push(eval_unary_operator(op, operandstack));
                break;

              case 2:
                operandstack.push(eval_binary_operator(op, operandstack));
                break;

              case 3:
                operandstack.push(eval_ternary_operator(op, operandstack));
                break;
            }
          }

          if (tkop.code != OpCode::close)
            nextop = OpType::PrefixOp;

          break;

        case TokenType::Literal:

          // TODO: implement with from_chars
          operandstack.push({ atof(&expression[pos]) });

          nextop = OpType::InfixOp;

          break;

        case TokenType::Identifier:

          operandstack.push({ scope.lookup(leap::string_view(&expression[pos], tklen)) });

          nextop = OpType::InfixOp;

          break;

        case TokenType::NoToken:

          break;
      }

      pos += tklen;
    }

    while (operatorstack.size() > 0)
    {
      Operator op = operatorstack.pop();

      if (operandstack.size() < op.order)
        throw eval_error("invalid unwind");

      switch(op.order)
      {
        case 1:
          operandstack.push(eval_unary_operator(op, operandstack));
          break;

        case 2:
          operandstack.push(eval_binary_operator(op, operandstack));
          break;

        case 3:
          operandstack.push(eval_ternary_operator(op, operandstack));
          break;
      }
    }

    if (operatorstack.size() != 0 || operandstack.size() != 1)
      throw eval_error("invalid unwind");

    return operandstack.pop().value;
  }

} } //namespace

#endif // LMLEVALUATOR_HH
