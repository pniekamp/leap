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

using namespace std;
using namespace leap;
using namespace leap::lml;

static constexpr const char *Operators[8] = { "% / * abs min max floor ceil round trunc clamp sin cos tan asin acos atan atan2 pow sqrt log exp log2 exp2 if ", "+ - ", "<= >= < > ", "== != ", "! && || ", "( ) ", ", ", "" };

namespace
{
  //|////////////////////// is_literal //////////////////////////////////////
  size_t is_literal(leap::string_view str)
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


  //|////////////////////// is_identifier ///////////////////////////////////
  size_t is_identifier(leap::string_view str)
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

  //|////////////////////// is_operator /////////////////////////////////////
  size_t is_operator(leap::string_view str, Expression::OpCode *code, size_t *order, size_t *precedence, Expression::OpType optype)
  {
    *code = static_cast<Expression::OpCode>(0);

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

          if (optype == Expression::OpType::InfixOp)
          {
            *order = 2;
          }

          if (optype == Expression::OpType::PrefixOp)
          {
            switch(*code)
            {
              case Expression::OpCode::plus:
              case Expression::OpCode::minus:
              case Expression::OpCode::abs:
              case Expression::OpCode::floor:
              case Expression::OpCode::ceil:
              case Expression::OpCode::round:
              case Expression::OpCode::trunc:
              case Expression::OpCode::sin:
              case Expression::OpCode::cos:
              case Expression::OpCode::tan:
              case Expression::OpCode::asin:
              case Expression::OpCode::acos:
              case Expression::OpCode::atan:
              case Expression::OpCode::sqrt:
              case Expression::OpCode::log:
              case Expression::OpCode::exp:
              case Expression::OpCode::log2:
              case Expression::OpCode::exp2:
              case Expression::OpCode::bnot:
                *order = 1;
                break;

              case Expression::OpCode::min:
              case Expression::OpCode::max:
              case Expression::OpCode::atan2:
              case Expression::OpCode::pow:
                *order = 2;
                break;

              case Expression::OpCode::clamp:
              case Expression::OpCode::cond:
                *order = 3;
                break;

              default:
                break;
            }
          }

          return k;
        }

        j += k;

        *code = static_cast<Expression::OpCode>(static_cast<int>(*code) + 1);
      }
    }

    return 0;
  }
}

namespace leap { namespace lml
{
  //|--------------------- Expression -----------------------------------------
  //|--------------------------------------------------------------------------

  //|////////////////////// Constructor ///////////////////////////////////////
  basic_expression::basic_expression(leap::string_view str)
  {
    int operandcheck = 0;
    SimpleStack<Operator> operatorstack;

    size_t pos = 0;
    OpType nextop = OpType::PrefixOp;

    while (pos < str.size())
    {
      while (pos < str.size() && str[pos] <= ' ')
        ++pos;

      Operator tkop = {};
      TokenType tktype = TokenType::NoToken;

      size_t licnt = is_literal(str.substr(pos));
      size_t idcnt = is_identifier(str.substr(pos));
      size_t opcnt = is_operator(str.substr(pos), &tkop.code, &tkop.order, &tkop.precedence, nextop);

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

            if (operatorstack.empty() || operatorstack.peek().precedence > tkop.precedence)
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

            auto op = operatorstack.pop();

            ast.push_back(Node::op(op.code, op.order));

            if ((operandcheck -= op.order - 1) < 1)
              throw Expression::eval_error("invalid unwind");
          }

          if (tkop.code != OpCode::close)
            nextop = OpType::PrefixOp;

          break;

        case TokenType::Literal:

          // TODO: implement with from_chars
          ast.push_back(Node::number(strtod(&str[pos], nullptr)));

          operandcheck += 1;

          nextop = OpType::InfixOp;

          break;

        case TokenType::Identifier:

          ast.push_back(Node::identifier(pos, tklen));

          operandcheck += 1;

          nextop = OpType::InfixOp;

          break;

        case TokenType::NoToken:

          break;
      }

      pos += tklen;
    }

    while (!operatorstack.empty())
    {
      auto op = operatorstack.pop();

      ast.push_back(Node::op(op.code, op.order));

      if ((operandcheck -= op.order - 1) < 1)
        throw Expression::eval_error("invalid unwind");
    }

    if (operandcheck != 1)
      throw Expression::eval_error("invalid unwind");
  }


  //|////////////////////// Constructor ///////////////////////////////////////
  Expression::Expression(std::string str)
    : basic_expression(str)
  {
    m_str = std::move(str);
  }

} }
