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

#include <vector>

/**
 * \namespace leap::lml
 * \brief Leap Math Library containing mathmatical routines
 *
**/

namespace leap { namespace lml
{

  /////////// EvaluatorHook ///////////
  class EvaluatorHook
  {
    EvaluatorHook() { }
    virtual ~EvaluatorHook() { }

    public:
      virtual double eval_variable(const char *variable) = 0;
  };


  //|-------------------- Evaluator -----------------------------------------
  //|------------------------------------------------------------------------
  /**
   * \brief Mathmatical Expression Evaluator
   *
   * A class to evaluate mathmatical expressions
   *
   * Supports unary +, unary -, +, -, *, /, order of precedence, ( )
   * and variables (\@V)
   *
   * Variable support in the evaluator class takes two basic forms. An internal
   * list of all defined variables is maintained. Variables can be added to
   * this list via the AddVariable function. When a variable (\@n) is encountered
   * within an expression, the list is accessed to determine a value for n. If
   * no value is defined, the Evaluator Hook is called (if specified).
   *
   * The Evaluator Hook is defined by passing a pointer to a class that derives
   * from EvaluatorHook to the DefineEvalHook function. This class may override
   * the EvalVariable method to return the value of variables.
   *
   * Typical Use :
   * \code
   *   {
   *     Evaluator evaluator;
   *
   *     evaluator.AddVariable("input", inputvalue);
   *
   *     double result = evaluator.Evaluate("15 + @input * (8/9)");
   *   }
   * \endcode
  **/

  class Evaluator
  {
    public:
      Evaluator();

      void add_variable(const char *name, double value);
      void remove_all_variables();

      void define_evalhook(EvaluatorHook *hook);

      double evaluate(const char *expression) const;

    private:

      double eval_argument(const char *arg) const;
      double eval_expression(double left, const char *op, double right) const;

    private:

      struct VariableData
      {
        char name[32];
        double value;
      };

      std::vector<VariableData> m_variables;

      EvaluatorHook *m_hook;
  };

} } //namespace

#endif // LMLEVALUATOR_HH
