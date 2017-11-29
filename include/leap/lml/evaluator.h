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

#include <leap/stringview.h>
#include <vector>
#include <functional>

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
   * Variable support in the evaluator class takes two basic forms. An internal
   * list of all defined variables is maintained. Variables can be added to
   * this list via the add_variable function. When a variable is encountered
   * within an expression, the list is accessed to determine a value for n. If
   * no value is defined, the Evaluator Hook is called (if specified).
   *
   * Typical Use :
   * \code
   *   {
   *     Evaluator evaluator;
   *
   *     evaluator.add_variable("input", inputvalue);
   *
   *     double result = evaluator.evaluate("15 + input * (8/9)");
   *   }
   * \endcode
  **/

  class Evaluator
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

    public:
      Evaluator();

      void add_variable(string_view name, double value);
      void remove_all_variables();

      void define_evalhook(double(*hook)(Evaluator const &, const char *, size_t));

      double evaluate(const char *expression) const;

    private:

      struct Operator
      {
        int code;
        int order;
        int precedence;
      };

      struct Operand
      {
        double value;
      };

      Operand eval_argument(const char *arg, size_t len) const;
      Operand eval_expression(Operator op, Operand first) const;
      Operand eval_expression(Operator op, Operand second, Operand first) const;
      Operand eval_expression(Operator op, Operand third, Operand second, Operand first) const;

    private:

      struct Variable
      {
        size_t len;
        char name[32];
        Operand value;
      };

      std::vector<Variable> m_variables;

      std::function<double(Evaluator const &, const char *, size_t)> m_hook;
  };

} } //namespace

#endif // LMLEVALUATOR_HH
