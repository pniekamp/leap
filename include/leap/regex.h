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

#pragma once

#include <leap/stringview.h>
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

    /////////////////// RegExVisitor /////////////////////////////
    class RegExVisitor
    {
      public:
        RegExVisitor() = default;
        virtual ~RegExVisitor() = default;

        virtual void visit(RegExState &) = 0;
    };


    /////////////////// RegExContext /////////////////////////////
    struct RegExContext
    {
      string_view str;
    };


    /////////////////// RegExState ///////////////////////////////
    class RegExState
    {
      public:
        RegExState(RegExContext *context);

        void accept(RegExVisitor &visitor);

        const char *beg, *end;

        size_t count;

        bool capture;

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
        RegExBase() = default;
        virtual ~RegExBase() = default;

        virtual void set_repeat(RepeatType const &repeat) = 0;

        virtual bool consider_first(const char *&pos, RegExState &state) const = 0;
        virtual bool consider_next(const char *&pos, RegExState &state) const = 0;
    };


    /////////////////// RegExCommon //////////////////////////////
    class RegExCommon : public RegExBase
    {
      public:
        RegExCommon();

        void set_repeat(RepeatType const &repeat) override;

        bool consider_first(const char *&pos, RegExState &state) const override;
        bool consider_next(const char *&pos, RegExState &state) const override;

        virtual bool consider_one(const char *&pos, RegExState &state) const = 0;

      protected:

        RepeatType m_repeat;
    };


    /////////////////// RegExCore ////////////////////////////////
    class RegExCore : public RegExCommon
    {
      public:
        RegExCore();

        void define(string_view str);

      public:

        bool consider_one(const char *&pos, RegExState &state) const override;
        bool consider_next(const char *&pos, RegExState &state) const override;

      private:

        std::vector<std::unique_ptr<RegExBase>> m_conditions;
    };


    /////////////////// RegExGroup ///////////////////////////////
    class RegExGroup : public RegExCore
    {
      public:
        RegExGroup(string_view group);

        bool consider_first(const char *&pos, RegExState &state) const override;

      private:

        bool m_capture;
    };


    /////////////////// RegExFilter //////////////////////////////
    class RegExFilter : public RegExCommon
    {
      public:
        RegExFilter(string_view filter);

      public:

        bool consider_one(const char *&pos, RegExState &state) const override;

      private:

        std::bitset<256> m_filter;
    };


    /////////////////// RegExAlternative /////////////////////////
    class RegExAlternative : public RegExBase
    {
      public:
        RegExAlternative(std::unique_ptr<RegExBase> &&left, std::unique_ptr<RegExBase> &&right);

      public:

        void set_repeat(RepeatType const &repeat) override;

        bool consider_first(const char *&pos, RegExState &state) const override;
        bool consider_next(const char *&pos, RegExState &state) const override;

      private:

        std::unique_ptr<RegExBase> m_left;
        std::unique_ptr<RegExBase> m_right;
    };


    /////////////////// RegExStartOfLine /////////////////////////
    class RegExStartOfLine : public RegExCommon
    {
      public:
        RegExStartOfLine();

      public:

        bool consider_one(const char *&pos, RegExState &state) const override;
    };


    /////////////////// RegExEndOfLine ///////////////////////////
    class RegExEndOfLine : public RegExCommon
    {
      public:
        RegExEndOfLine();

      public:

        bool consider_one(const char *&pos, RegExState &state) const override;
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
      RegEx(std::string const &str);
      RegEx(string_view str);

      void prepare(string_view str);

    private:

      friend bool match(RegEx const &rex, string_view str, std::vector<string_view> *groups);
      friend bool search(RegEx const &rex, string_view str, std::vector<string_view> *groups);

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
  bool match(RegEx const &rex, string_view str, std::vector<string_view> *groups = nullptr);

  //|///////////////////////// search ///////////////////////////////////////
  ///
  /// Returns true if any part of str is matched by the regular expresion
  ///
  /// \param[in] rex The regular expression to match against
  /// \param[in] str The string to match
  /// \param[out] groups Optionally return any groups
  ///
  bool search(RegEx const &rex, string_view str, std::vector<string_view> *groups = nullptr);

} } // namespace regex
