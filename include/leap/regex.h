//
// Regular Expressions
//
//   Peter Niekamp, January 2006
//

//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef REGEX_HH
#define REGEX_HH

#include <vector>
#include <bitset>
#include <memory>

/**
 * \namespace leap::regex
 * \brief Regular Expression Parser
 *
**/

/**
 * \defgroup leapdata Data Containors
 * \brief Data Containors
 *
**/

namespace leap { namespace regex
{

  namespace RegExImpl
  {
    class RegExState;

    /////////////////// RegExStateVisitor ////////////////////////
    class RegExStateVisitor
    {
      public:
        RegExStateVisitor() { }
        virtual ~RegExStateVisitor() { }

        virtual void visit(RegExState &) { }
    };


    /////////////////// RegExContext /////////////////////////////
    class RegExContext
    {
      public:
        RegExContext();

        const char *startofline;
    };


    /////////////////// RegExState ///////////////////////////////
    class RegExState
    {
      public:
        RegExState(RegExContext *context);

        void accept(RegExStateVisitor &visitor);

        const char *first;
        const char *last;

        unsigned int count;

        bool grouped;

        std::vector<RegExState> substate;

        RegExContext *context;
    };


    /////////////////// RegExBase ////////////////////////////////
    class RegExBase
    {
      public:

        enum class RepeatType
        {
          Once,
          ZeroOrOnce,
          ZeroOrMore,
          OneOrMore
        };

      public:
        RegExBase() { }
        virtual ~RegExBase() { }

        virtual void set_repeat(RepeatType const &repeat) = 0;

        virtual bool consider_first(const char *str, RegExState &state) const = 0;
        virtual bool consider_next(RegExState &state) const = 0;
    };


    /////////////////// RegExCommon //////////////////////////////
    class RegExCommon : public RegExBase
    {
      public:
        RegExCommon();
        virtual ~RegExCommon();

        virtual void set_repeat(RepeatType const &repeat);

        virtual bool consider_first(const char *str, RegExState &state) const;
        virtual bool consider_next(RegExState &state) const;

        virtual bool consider_one(const char *str, RegExState &state) const = 0;

      protected:

        RepeatType m_repeat;
    };


    /////////////////// RegExCore ////////////////////////////////
    class RegExCore : public RegExCommon
    {
      public:
        RegExCore();
        virtual ~RegExCore();

        void define(const char *rex);
        void define(std::vector<std::shared_ptr<RegExBase>> const &rex);

      public:

        virtual bool consider_one(const char *str, RegExState &state) const;
        virtual bool consider_next(RegExState &state) const;

      private:

        std::vector<std::shared_ptr<RegExBase>> m_conditions;
    };


    /////////////////// RegExGroup ///////////////////////////////
    class RegExGroup : public RegExCore
    {
      public:
        RegExGroup(const char *group);
        virtual ~RegExGroup();

        virtual bool consider_first(const char *str, RegExState &state) const;

      private:

        bool m_grouping;
    };


    /////////////////// RegExFilter //////////////////////////////
    class RegExFilter : public RegExCommon
    {
      public:
        RegExFilter(const char *filter);
        virtual ~RegExFilter();

      public:

        virtual bool consider_one(const char *str, RegExState &state) const;

      private:

        std::bitset<256> m_filter;
    };


    /////////////////// RegExAlternative /////////////////////////
    class RegExAlternative : public RegExBase
    {
      public:
        RegExAlternative(RegExCore const &left, RegExCore const &right);
        virtual ~RegExAlternative();

      public:

        virtual void set_repeat(RepeatType const &repeat);

        virtual bool consider_first(const char *str, RegExState &state) const;
        virtual bool consider_next(RegExState &state) const;

      private:

        RegExCore m_left;
        RegExCore m_right;
    };


    /////////////////// RegExStartOfLine /////////////////////////
    class RegExStartOfLine : public RegExCommon
    {
      public:
        RegExStartOfLine();
        virtual ~RegExStartOfLine();

      public:

        virtual bool consider_one(const char *str, RegExState &state) const;
    };


    /////////////////// RegExEndOfLine ///////////////////////////
    class RegExEndOfLine : public RegExCommon
    {
      public:
        RegExEndOfLine();
        virtual ~RegExEndOfLine();

      public:

        virtual bool consider_one(const char *str, RegExState &state) const;
    };

  } // namespace RegExImpl


  //|///////////////////////// RegEx ////////////////////////////////////////
  /**
   * \ingroup leapdata
   *
   * \brief Regular Expression Container
   *
   * A Regular Expression string is parsed into an array of condition objects.
   *
   * Typical use :
   *
   *  \code
   *  {
   *     bool result = Match(".*", somestring);
   *  }
   *  \endcode
   *
  **/
  class RegEx
  {
    public:
      RegEx();
      RegEx(const char *str);

      void prepare(const char *str);

    private:

      friend bool match(RegEx const &rex, const char *str, std::vector<std::string> *groups);
      friend bool search(RegEx const &rex, const char *str, std::vector<std::string> *groups);

      RegExImpl::RegExCore m_regex;
  };


  //|///////////////////////// match ////////////////////////////////////////
  ///
  /// Returns true if characters at beginning of str are matched by the regular expresion
  ///
  /// \param[in] rex The regular expression to match against
  /// \param[in] str The string to match
  /// \param[out] groups Optionally return any groups
  ///
  bool match(RegEx const &rex, const char *str, std::vector<std::string> *groups = NULL);


  //|///////////////////////// search ///////////////////////////////////////
  ///
  /// Returns true if any part of str is matched by the regular expresion
  ///
  /// \param[in] rex The regular expression to match against
  /// \param[in] str The string to match
  /// \param[out] groups Optionally return any groups
  ///
  bool search(RegEx const &rex, const char *str, std::vector<std::string> *groups = NULL);

} } // namespace regex

#endif // REGEX_HH
