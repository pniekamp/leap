//
// regex.cpp implementation of Regular Expressions
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

#include "leap/regex.h"
#include "leap/util.h"
#include <cstring>

using namespace std;
using namespace leap;
using namespace leap::regex;

namespace leap { namespace regex
{

  namespace RegExImpl
  {

    //|--------------------- RegExContext ---------------------------------------
    //|--------------------------------------------------------------------------

    //|///////////////////// RegExContext::Constructor ////////////////////////////
    RegExContext::RegExContext()
    {
    }



    //|--------------------- RegExState -----------------------------------------
    //|--------------------------------------------------------------------------

    //|///////////////////// RegExState::Constructor ////////////////////////////
    RegExState::RegExState(RegExContext *context)
      : context(context)
    {
      beg = end = nullptr;

      count = 0;

      capture = false;
    }


    //|///////////////////// RegExState::accept /////////////////////////////////
    void RegExState::accept(RegExStateVisitor &visitor)
    {
      visitor.visit(*this);

      for(size_t i = 0; i < substate.size(); ++i)
      {
        substate[i].accept(visitor);
      }
    }



    //|--------------------- RegExCommon ----------------------------------------
    //|--------------------------------------------------------------------------

    //|///////////////////// RegExCommon::Constructor ///////////////////////////
    RegExCommon::RegExCommon()
    {
      m_repeat = RepeatType::Once;
    }


    //|///////////////////// RegExCommon::Destructor ////////////////////////////
    RegExCommon::~RegExCommon()
    {
    }


    //|///////////////////// RegExCommon::set_repeat ////////////////////////////
    void RegExCommon::set_repeat(RepeatType const &repeat)
    {
      m_repeat = repeat;
    }


    //|///////////////////// RegExCommon::consider_first ////////////////////////
    bool RegExCommon::consider_first(const char *&pos, RegExState &state) const
    {
      state.count = 0;
      state.beg = state.end = pos;

      if (m_repeat == RepeatType::ZeroOrOnce || m_repeat == RepeatType::ZeroOrMore)
      {
        // Match Zero Times
        return true;
      }

      return consider_one(pos, state);
    }


    //|///////////////////// RegExCommon::consider_next /////////////////////////
    bool RegExCommon::consider_next(const char *&pos, RegExState &state) const
    {
      if (m_repeat == RepeatType::Once)
        return false;

      if (m_repeat == RepeatType::ZeroOrOnce && state.count == 1)
        return false;

      return consider_one(pos, state);
    }



    //|--------------------- RegExFilter ----------------------------------------
    //|--------------------------------------------------------------------------


    //|///////////////////// RegExFilter::Constructor ///////////////////////////
    RegExFilter::RegExFilter(string_view filter)
    {
      m_filter.reset();

      bool compliment = false;

      if (filter[0] == '^')
      {
        compliment = true;

        filter.remove_prefix(1);
      }

      while (!filter.empty())
      {
        if (filter[0] == '.') // wildcard filter
        {
          m_filter.set();

          filter.remove_prefix(1);
        }

        else if (strncmp(filter.data(), "[:alnum:]", 9) == 0)
        {
          for(char c = 'a'; c <= 'z'; ++c)
            m_filter.set(c);

          for(char c = 'A'; c <= 'Z'; ++c)
            m_filter.set(c);

          for(char c = '0'; c <= '9'; ++c)
            m_filter.set(c);

          filter.remove_prefix(9);
        }

        else if (strncmp(filter.data(), "[:alpha:]", 9) == 0)
        {
          for(char c = 'a'; c <= 'z'; ++c)
            m_filter.set(c);

          for(char c = 'A'; c <= 'Z'; ++c)
            m_filter.set(c);

          filter.remove_prefix(9);
        }

        else if (strncmp(filter.data(), "[:blank:]", 9) == 0)
        {
          m_filter.set(' ');
          m_filter.set('\t');

          filter.remove_prefix(9);
        }

        else if (strncmp(filter.data(), "[:cntrl:]", 9) == 0)
        {
          for(char c = 1; c <= 31; ++c)
            m_filter.set(c);

          filter.remove_prefix(9);
        }

        else if (strncmp(filter.data(), "[:digit:]", 9) == 0)
        {
          for(char c = '0'; c <= '9'; ++c)
            m_filter.set(c);

          filter.remove_prefix(9);
        }

        else if (strncmp(filter.data(), "[:lower:]", 9) == 0)
        {
          for(char c = 'a'; c <= 'z'; ++c)
            m_filter.set(c);

          filter.remove_prefix(9);
        }

        else if (strncmp(filter.data(), "[:space:]", 9) == 0)
        {
          m_filter.set(' ');
          m_filter.set('\t');
          m_filter.set(0xA);
          m_filter.set(0xD);

          filter.remove_prefix(9);
        }

        else if (strncmp(filter.data(), "[:upper:]", 9) == 0)
        {
          for(char c = 'A'; c <= 'Z'; ++c)
            m_filter.set(c);

          filter.remove_prefix(9);
        }

        else if (strncmp(filter.data(), "[:xdigit:]", 10) == 0)
        {
          for(char c = 'a'; c <= 'f'; ++c)
            m_filter.set(c);

          for(char c = 'A'; c <= 'F'; ++c)
            m_filter.set(c);

          for(char c = '0'; c <= '9'; ++c)
            m_filter.set(c);

          filter.remove_prefix(10);
        }

        else if (strncmp(filter.data(), "[:word:]", 8) == 0)
        {
          for(char c = 'a'; c <= 'z'; ++c)
            m_filter.set(c);

          for(char c = 'A'; c <= 'Z'; ++c)
            m_filter.set(c);

          for(char c = '0'; c <= '9'; ++c)
            m_filter.set(c);

          m_filter.set('_');

          filter.remove_prefix(8);
        }

        else
        {
          if (filter[0] == '\\')
            filter.remove_prefix(1);

          m_filter.set(filter[0]);

          filter.remove_prefix(1);
        }
      }

      if (compliment)
      {
        m_filter.flip();
      }

      // Never match null
      m_filter.reset(0);
    }


    //|///////////////////// RegExFilter::Destructor ////////////////////////////
    RegExFilter::~RegExFilter()
    {
    }


    //|///////////////////// RegExFilter::consider_one //////////////////////////
    bool RegExFilter::consider_one(const char *&pos, RegExState &state) const
    {
      if (pos == state.context->str.end())
        return false;

      if (m_filter.test(pos[0]))
      {
        ++pos;
        ++state.count;
        state.end = pos;

        return true;
      }

      return false;
    }


    //|--------------------- RegExStartOfLine -----------------------------------
    //|--------------------------------------------------------------------------


    //|///////////////////// RegExStartOfLine::Constructor //////////////////////
    RegExStartOfLine::RegExStartOfLine()
    {
    }


    //|///////////////////// RegExStartOfLine::Destructor ///////////////////////
    RegExStartOfLine::~RegExStartOfLine()
    {
    }


    //|///////////////////// RegExStartOfLine::consider_one //////////////////////
    bool RegExStartOfLine::consider_one(const char *&pos, RegExState &state) const
    {
      return (pos == state.context->str.begin());
    }



    //|--------------------- RegExEndOfLine -------------------------------------
    //|--------------------------------------------------------------------------


    //|///////////////////// RegExEndOfLine::Constructor ////////////////////////
    RegExEndOfLine::RegExEndOfLine()
    {
    }


    //|///////////////////// RegExEndOfLine::Destructor /////////////////////////
    RegExEndOfLine::~RegExEndOfLine()
    {
    }


    //|///////////////////// RegExEndOfLine::consider_one ////////////////////////
    bool RegExEndOfLine::consider_one(const char *&pos, RegExState &state) const
    {
      return (pos == state.context->str.end() || pos[0] == '\r' || pos[0] == '\n');
    }



    //|--------------------- RegExAlternative -----------------------------------
    //|--------------------------------------------------------------------------


    //|///////////////////// RegExAlternative::Constructor //////////////////////
    RegExAlternative::RegExAlternative(unique_ptr<RegExBase> &&left, unique_ptr<RegExBase> &&right)
    {
      m_left = std::move(left);
      m_right = std::move(right);
    }


    //|///////////////////// RegExAlternative::Destructor ///////////////////////
    RegExAlternative::~RegExAlternative()
    {
    }

    //|///////////////////// RegExAlternative::set_repeat ///////////////////////
    void RegExAlternative::set_repeat(RepeatType const &repeat)
    {
      // Can't Repeat an alternative (only a group containing alternatives)
    }


    //|///////////////////// RegExAlternative::consider_first ///////////////////
    bool RegExAlternative::consider_first(const char *&pos, RegExState &state) const
    {
      state.substate.resize(2, RegExState(state.context));

      state.count = 0;
      state.beg = state.end = pos;

      // Try left first
      if (m_left->consider_first(pos, state.substate[0]))
      {
        state.end = pos;

        return true;
      }

      // Then right
      if (m_right->consider_first(pos, state.substate[1]))
      {
        state.end = pos;

        return true;
      }

      return false;
    }


    //|///////////////////// RegExAlternative::consider_next ////////////////////
    bool RegExAlternative::consider_next(const char *&pos, RegExState &state) const
    {
      if (state.substate[0].count != 0)
      {
        if (m_left->consider_next(pos, state.substate[0]))
        {
          state.end = pos;

          return true;
        }

        state.substate[0].count = 0;

        // Done with the left, try the right
        if (m_right->consider_first(pos, state.substate[1]))
        {
          state.end = pos;

          return true;
        }
      }

      if (state.substate[1].count != 0)
      {
        if (m_right->consider_next(pos, state.substate[1]))
        {
          state.end = pos;

          return true;
        }
      }

      return false;
    }


    //|--------------------- RegExGroup -----------------------------------------
    //|--------------------------------------------------------------------------


    //|///////////////////// RegExGroup::Constructor ////////////////////////////
    RegExGroup::RegExGroup(string_view group)
    {
      m_capture = true;

      if (group[0] == '?' && group[1] == ':')
      {
        m_capture = false;

        group.remove_prefix(2);
      }

      define(group);
    }


    //|///////////////////// RegExGroup::Destructor /////////////////////////////
    RegExGroup::~RegExGroup()
    {
    }


    //|///////////////////// RegExGroup::consider_first /////////////////////////
    bool RegExGroup::consider_first(const char *&pos, RegExState &state) const
    {
      state.capture = m_capture;

      return RegExCore::consider_first(pos, state);
    }


    //|--------------------- RegExCore ------------------------------------------
    //|--------------------------------------------------------------------------


    //|///////////////////// RegExCore::Constructor /////////////////////////////
    RegExCore::RegExCore()
    {
    }


    //|///////////////////// RegExCore::Destructor //////////////////////////////
    RegExCore::~RegExCore()
    {
    }


    //|///////////////////// RegExCore::define //////////////////////////////////
    void RegExCore::define(string_view str)
    {
      m_conditions.clear();

      //
      // Parse the regular expression into condition objects
      //

      while (!str.empty())
      {
        if (str[0] == '.') // wildcard filter object
        {
          m_conditions.push_back(make_unique<RegExFilter>("."));

          str.remove_prefix(1);
        }

        else if (str[0] == '?' || str[0] == '*' || str[0] == '+')
        {
          if (m_conditions.size() == 0)
            return;

          if (str[0] == '?') // repeat last zero or once
          {
            m_conditions.back()->set_repeat(RepeatType::ZeroOrOnce);
          }

          if (str[0] == '*') // repeat last zero or more
          {
            m_conditions.back()->set_repeat(RepeatType::ZeroOrMore);
          }

          if (str[0] == '+') // repeat last one or more
          {
            m_conditions.back()->set_repeat(RepeatType::OneOrMore);
          }

          str.remove_prefix(1);
        }

        else if (str[0] == '|') // Alternative
        {
          auto left = make_unique<RegExCore>();
          auto right = make_unique<RegExCore>();

          // Move all our conditions into the left
          left->m_conditions = std::move(m_conditions);

          // All the rest goes on the right
          right->define(str.substr(1));

          m_conditions.clear();
          m_conditions.push_back(make_unique<RegExAlternative>(std::move(left), std::move(right)));

          str.remove_prefix(str.size());
        }

        else if (str[0] == '(') // Group
        {
          unsigned int count = 1;
          unsigned int indent = 1;

          for( ; count < str.size() && indent > 0; ++count)
          {
            if (str[count] == '(')
              ++indent;

            if (str[count] == ')')
              --indent;
          }

          m_conditions.push_back(make_unique<RegExGroup>(str.substr(1, count-2)));

          str.remove_prefix(count);
        }

        else if (str[0] == '[') // set
        {
          unsigned int count = 1;
          unsigned int indent = 1;

          for( ; count < str.size() && indent > 0; ++count)
          {
            if (str[count] == '[')
              ++indent;

            if (str[count] == ']')
              --indent;
          }

          m_conditions.push_back(make_unique<RegExFilter>(str.substr(1, count-2)));

          str.remove_prefix(count);
        }

        else if (str[0] == '^') // Start Of Line Placeholder
        {
          m_conditions.push_back(make_unique<RegExStartOfLine>());

          str.remove_prefix(1);
        }

        else if (str[0] == '$') // End Of Line Placeholder
        {
          m_conditions.push_back(make_unique<RegExEndOfLine>());

          str.remove_prefix(1);
        }

        else if (str[0] == '\\') // Escaped Character
        {
          m_conditions.push_back(make_unique<RegExFilter>(str.substr(0, 2)));

          str.remove_prefix(2);
        }

        else
        {
          // Any Other Character goes as is
          m_conditions.push_back(make_unique<RegExFilter>(str.substr(0, 1)));

          str.remove_prefix(1);
        }
      }
    }


    //|///////////////////// RegExCore::consider_one ////////////////////////////
    bool RegExCore::consider_one(const char *&pos, RegExState &state) const
    {
      if (m_conditions.size() == 0)
        return true;

      const char *cursor = pos;

      size_t c = 0;
      size_t n = m_conditions.size();

      state.substate.resize(n, RegExState(state.context));

      // Jitter until all match or fail.
      do
      {
        if (m_conditions[c]->consider_first(cursor, state.substate[c]))
        {
          // Match, move on to next condition
          ++c;
        }
        else
        {
          // No Match, move back and expand
          while (c > 0)
          {
            cursor = state.substate[--c].end;

            if (m_conditions[c]->consider_next(cursor, state.substate[c]))
            {
              // Successful expand, try next condition again
              ++c;

              break;
            }
          }
        }

      } while ((c % n) != 0);

      if (c != 0 && (c % n) == 0)
      {
        pos = cursor;
        ++state.count;
        state.end = pos;

        return true;
      }

      return false;
    }


    //|///////////////////// RegExCore::consider_next ///////////////////////////
    bool RegExCore::consider_next(const char *&pos, RegExState &state) const
    {
      if (m_conditions.size() == 0)
        return false;

      const char *cursor = pos;

      size_t c = state.count * m_conditions.size();
      size_t n = m_conditions.size();

      // Repeat not allowed or last instance used up no characters (don't repeat nulls infinetly)
      if (m_repeat != RepeatType::Once && (m_repeat != RepeatType::ZeroOrOnce || state.count == 0)
          && (state.count == 0 || state.substate[state.substate.size()-n].beg <= state.substate.back().end))
      {
        state.substate.resize((state.count+1)*m_conditions.size(), RegExState(state.context));
      }

      // Jitter until all match or fail.
      do
      {
        if (c < state.substate.size() && m_conditions[c % n]->consider_first(cursor, state.substate[c]))
        {
          // Match, move on to next condition
          ++c;
        }
        else
        {
          // No Match, move back and expand
          while (c > 0)
          {
            cursor = state.substate[--c].end;

            if (m_conditions[c % n]->consider_next(cursor, state.substate[c]))
            {
              // Successful expand, try next condition again
              ++c;

              break;
            }
          }

        }

      } while ((c % n) != 0);

      if (c != 0 && (c % n) == 0)
      {
        pos = cursor;
        state.count = c / n;
        state.end = pos;

        return true;
      }

      return false;
    }


  } // namespace RegExImpl




  //|--------------------- RegEx ----------------------------------------------
  //|--------------------------------------------------------------------------

  //|///////////////////// RegEx::Constructor /////////////////////////////////
  RegEx::RegEx()
  {
  }


  //|///////////////////// RegEx::Constructor /////////////////////////////////
  RegEx::RegEx(const char *str)
  {
    prepare(str);
  }


  //|///////////////////// RegEx::Constructor /////////////////////////////////
  RegEx::RegEx(string const &str)
  {
    prepare(str);
  }


  //|///////////////////// RegEx::Constructor /////////////////////////////////
  RegEx::RegEx(string_view str)
  {
    prepare(str);
  }


  //|///////////////////// RegEx::prepare /////////////////////////////////////
  void RegEx::prepare(string_view str)
  {
    m_regex.define(str);
  }



  //|--------------------- CaptureVisitor -------------------------------------
  //|--------------------------------------------------------------------------

  class CaptureVisitor : public RegExImpl::RegExStateVisitor
  {
    public:
      CaptureVisitor(vector<string_view> *groups)
      {
        m_groups = groups;
      }

      void visit(RegExImpl::RegExState &state)
      {
        if (state.capture == true && state.count != 0)
        {
          m_groups->push_back(string_view(state.beg, state.end - state.beg));
        }
      }

      vector<string_view> *m_groups;
  };



  //|--------------------- match ----------------------------------------------
  //|--------------------------------------------------------------------------

  bool match(RegEx const &rex, string_view str, vector<string_view> *groups)
  {
    RegExImpl::RegExContext context;

    context.str = str;

    RegExImpl::RegExState state(&context);

    auto pos = str.begin();

    if (rex.m_regex.consider_first(pos, state))
    {
      if (groups)
      {
        // Transfer the marked groups into a vector

        CaptureVisitor capturevisitor(groups);

        state.accept(capturevisitor);
      }

      return true;
    }

    return false;
  }


  //|--------------------- search ---------------------------------------------
  //|--------------------------------------------------------------------------

  bool search(RegEx const &rex, string_view str, vector<string_view> *groups)
  {
    RegExImpl::RegExContext context;

    context.str = str;

    for(auto pos = str.begin(); pos != str.end(); ++pos)
    {
      RegExImpl::RegExState state(&context);

      if (rex.m_regex.consider_first(pos, state))
      {
        if (groups)
        {
          // Transfer the marked groups into a vector

          CaptureVisitor capturevisitor(groups);

          state.accept(capturevisitor);
        }

        return true;
      }
    }

    return false;
  }


} } // namespace regex
