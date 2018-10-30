//
// Mathmatical Expression Evaluator
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#pragma once

#include <leap/util.h>
#include <leap/stringview.h>
#include <stdexcept>
#include <cstdlib>
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
   *     double result = lml::eval(scope, "15 + input * (8/9)");
   *   }
   * \endcode
  **/

  class basic_expression
  {
    public:

      static constexpr size_t StackSize = 64;

    public:

      basic_expression() = default;
      basic_expression(leap::string_view str);

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
        mod, div, mul, abs, min, max, floor, ceil, round, trunc, clamp, sin, cos, tan, asin, acos, atan, atan2, pow, sqrt, log, exp, log2, exp2, cond, plus, minus, leq, geq, le, ge, eq, neq, bnot, band, bor, open, close, comma
      };

      struct Operator
      {
        OpCode code;
        size_t order;
        size_t precedence;
      };

      struct Node
      {
        enum
        {
          Op,
          Number,
          Identifier

        } type;

        union
        {
          struct // Op
          {
            OpCode opcode;
            size_t oporder;
          };

          struct // Number
          {
            double value;
          };

          struct // Identifier
          {
            size_t beg, len;
          };
        };

        static Node op(OpCode opcode, size_t oporder) { Node node; node.type = Op; node.opcode = opcode; node.oporder = oporder; return node; }
        static Node number(double value) { Node node; node.type = Number; node.value = value; return node; }
        static Node identifier(size_t beg, size_t len) { Node node; node.type = Identifier; node.beg = beg; node.len = len; return node; }
      };

      template<typename T>
      class SimpleStack
      {
        public:

          bool empty() const
          {
            return (m_head == 0);
          }

          T peek() const
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

      std::vector<Node> ast;
  };

  class Expression : public basic_expression
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

     Expression() = default;
     explicit Expression(std::string str);

     std::string const &str() const { return m_str; }

   private:

     std::string m_str;
  };


  //|////////////////////// eval_operator ///////////////////////////////////
  inline double eval_unary_operator(Expression::OpCode op, Expression::SimpleStack<double> &operandstack)
  {
    auto first = operandstack.pop();

    switch (op)
    {
      case Expression::OpCode::plus:
        return first;

      case Expression::OpCode::minus:
        return -first;

      case Expression::OpCode::abs:
        return std::abs(first);

      case Expression::OpCode::floor:
        return std::floor(first);

      case Expression::OpCode::ceil:
        return std::ceil(first);

      case Expression::OpCode::round:
        return std::round(first);

      case Expression::OpCode::trunc:
        return std::trunc(first);

      case Expression::OpCode::sin:
        return std::sin(first);

      case Expression::OpCode::cos:
        return std::cos(first);

      case Expression::OpCode::tan:
        return std::tan(first);

      case Expression::OpCode::asin:
        return std::asin(first);

      case Expression::OpCode::acos:
        return std::acos(first);

      case Expression::OpCode::atan:
        return std::atan(first);

      case Expression::OpCode::sqrt:
        return std::sqrt(first);

      case Expression::OpCode::log:
        return std::log(first);

      case Expression::OpCode::exp:
        return std::exp(first);

      case Expression::OpCode::log2:
        return std::log2(first);

      case Expression::OpCode::exp2:
        return std::exp2(first);

      case Expression::OpCode::bnot:
        return double(fcmp(first, 0.0));

      default:
        throw Expression::eval_error("invalid operator");
    }
  }


  //|////////////////////// eval_operator ///////////////////////////////////
  inline double eval_binary_operator(Expression::OpCode op, Expression::SimpleStack<double> &operandstack)
  {
    auto second = operandstack.pop();
    auto first = operandstack.pop();

    switch (op)
    {
      case Expression::OpCode::mod:
        return std::fmod(first, second);

      case Expression::OpCode::div:
        return first / second;

      case Expression::OpCode::mul:
        return first * second;

      case Expression::OpCode::plus:
        return first + second;

      case Expression::OpCode::minus:
        return first - second;

      case Expression::OpCode::leq:
        return double(first <= second);

      case Expression::OpCode::geq:
        return double(first >= second);

      case Expression::OpCode::le:
        return double(first < second);

      case Expression::OpCode::ge:
        return double(first > second);

      case Expression::OpCode::eq:
        return double(fcmp(first, second));

      case Expression::OpCode::neq:
        return double(!fcmp(first, second));

      case Expression::OpCode::band:
        return double(!fcmp(first, 0.0) && !fcmp(second, 0.0));

      case Expression::OpCode::bor:
        return double(!fcmp(first, 0.0) || !fcmp(second, 0.0));

      case Expression::OpCode::min:
        return std::min(first, second);

      case Expression::OpCode::max:
        return std::max(first, second);

      case Expression::OpCode::atan2:
        return std::atan2(first, second);

      case Expression::OpCode::pow:
        return std::pow(first, second);

      default:
        throw Expression::eval_error("invalid operator");
    }
  }


  //|////////////////////// eval_operator ///////////////////////////////////
  inline double eval_ternary_operator(Expression::OpCode op, Expression::SimpleStack<double> &operandstack)
  {
    auto third = operandstack.pop();
    auto second = operandstack.pop();
    auto first = operandstack.pop();

    switch (op)
    {
      case Expression::OpCode::clamp:
        return clamp(first, second, third);

      case Expression::OpCode::cond:
        return fcmp(first, 0.0) ? third : second;

      default:
        throw Expression::eval_error("invalid operator");
    }
  }


  template<typename Scope>
  double eval(Scope const &scope, basic_expression const &expression, leap::string_view text)
  {
    auto operands = basic_expression::SimpleStack<double>{};

    for(auto &node : expression.ast)
    {
      switch(node.type)
      {
        case Expression::Node::Op:

          switch(node.oporder)
          {
            case 1:
              operands.push(eval_unary_operator(node.opcode, operands));
              break;

            case 2:
              operands.push(eval_binary_operator(node.opcode, operands));
              break;

            case 3:
              operands.push(eval_ternary_operator(node.opcode, operands));
              break;
          }

          break;

        case Expression::Node::Number:
          operands.push(node.value);
          break;

        case Expression::Node::Identifier:
          operands.push(scope.lookup(text.substr(node.beg, node.len)));
          break;
      }
    }

    return operands.pop();
  }

  template<typename Scope>
  double eval(Scope const &scope, leap::string_view str)
  {
    return eval(scope, basic_expression(str), str);
  }

  template<typename Scope>
  double eval(Scope const &scope, Expression const &expression)
  {
    return eval(scope, expression, expression.str());
  }

} } //namespace
