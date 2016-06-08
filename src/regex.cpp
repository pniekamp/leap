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
      startofline = NULL;
    }



    //|--------------------- RegExState -----------------------------------------
    //|--------------------------------------------------------------------------

    //|///////////////////// RegExState::Constructor ////////////////////////////
    RegExState::RegExState(RegExContext *context)
      : context(context)
    {
      first = last = NULL;

      count = 0;

      grouped = false;
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
    bool RegExCommon::consider_first(const char *str, RegExState &state) const
    {
      state.first = str;
      state.last = str-1;
      state.count = 0;

      if (str == NULL)
        return false;

      if (m_repeat == RepeatType::ZeroOrOnce || m_repeat == RepeatType::ZeroOrMore)
      {
        // Match Zero Times
        return true;
      }

      return consider_one(str, state);
    }


    //|///////////////////// RegExCommon::consider_next /////////////////////////
    bool RegExCommon::consider_next(RegExState &state) const
    {
      if (m_repeat == RepeatType::Once)
        return false;

      if (m_repeat == RepeatType::ZeroOrOnce && state.count == 1)
        return false;

      return consider_one(state.last+1, state);
    }



    //|--------------------- RegExFilter ----------------------------------------
    //|--------------------------------------------------------------------------


    //|///////////////////// RegExFilter::Constructor ///////////////////////////
    RegExFilter::RegExFilter(const char *filter)
    {
      m_filter.reset();

      if (filter[0] == '.') // wildcard filter
      {
        m_filter.set();
      }

      else if (filter[0] == '[') // set
      {
        ++filter;

        bool compliment = false;

        if (*filter == '^')
        {
          compliment = true;
          ++filter;
        }

        while (*filter != 0)
        {
          if (strncmp(filter, "[:alnum:]", 9) == 0)
          {
            for(char c = 'a'; c <= 'z'; ++c)
              m_filter.set(c);

            for(char c = 'A'; c <= 'Z'; ++c)
              m_filter.set(c);

            for(char c = '0'; c <= '9'; ++c)
              m_filter.set(c);

            filter += 9;
          }

          else if (strncmp(filter, "[:alpha:]", 9) == 0)
          {
            for(char c = 'a'; c <= 'z'; ++c)
              m_filter.set(c);

            for(char c = 'A'; c <= 'Z'; ++c)
              m_filter.set(c);

            filter += 9;
          }

          else if (strncmp(filter, "[:blank:]", 9) == 0)
          {
            m_filter.set(' ');
            m_filter.set('\t');

            filter += 9;
          }

          else if (strncmp(filter, "[:cntrl:]", 9) == 0)
          {
            for(char c = 1; c <= 31; ++c)
              m_filter.set(c);

            filter += 9;
          }

          else if (strncmp(filter, "[:digit:]", 9) == 0)
          {
            for(char c = '0'; c <= '9'; ++c)
              m_filter.set(c);

            filter += 9;
          }

          else if (strncmp(filter, "[:lower:]", 9) == 0)
          {
            for(char c = 'a'; c <= 'z'; ++c)
              m_filter.set(c);

            filter += 9;
          }

          else if (strncmp(filter, "[:space:]", 9) == 0)
          {
            m_filter.set(' ');
            m_filter.set('\t');
            m_filter.set(0xA);
            m_filter.set(0xD);

            filter += 9;
          }

          else if (strncmp(filter, "[:upper:]", 9) == 0)
          {
            for(char c = 'A'; c <= 'Z'; ++c)
              m_filter.set(c);

            filter += 9;
          }

          else if (strncmp(filter, "[:xdigit:]", 10) == 0)
          {
            for(char c = 'a'; c <= 'f'; ++c)
              m_filter.set(c);

            for(char c = 'A'; c <= 'F'; ++c)
              m_filter.set(c);

            for(char c = '0'; c <= '9'; ++c)
              m_filter.set(c);

            filter += 10;
          }

          else if (strncmp(filter, "[:word:]", 8) == 0)
          {
            for(char c = 'a'; c <= 'z'; ++c)
              m_filter.set(c);

            for(char c = 'A'; c <= 'Z'; ++c)
              m_filter.set(c);

            for(char c = '0'; c <= '9'; ++c)
              m_filter.set(c);

            m_filter.set('_');

            filter += 8;
          }

          else
          {
            if (*filter == '\\')
              ++filter;

            m_filter.set(*filter++);
          }
        }

        if (compliment)
        {
          m_filter.flip();
        }
      }

      else
      {
        for( ; *filter != 0; ++filter)
        {
          if (*filter == '\\')
            ++filter;

          m_filter.set(*filter);
        }
      }

      // Never match null
      m_filter.reset(0);
    }


    //|///////////////////// RegExFilter::Destructor ////////////////////////////
    RegExFilter::~RegExFilter()
    {
    }


    //|///////////////////// RegExFilter::consider_one //////////////////////////
    bool RegExFilter::consider_one(const char *str, RegExState &state) const
    {
      if (m_filter.test(*str))
      {
        state.last = str;
        ++state.count;

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
    bool RegExStartOfLine::consider_one(const char *str, RegExState &state) const
    {
      return (str == state.context->startofline);
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
    bool RegExEndOfLine::consider_one(const char *str, RegExState &state) const
    {
      return (*str == '\0' || *str == '\r' || *str == '\n');
    }



    //|--------------------- RegExAlternative -----------------------------------
    //|--------------------------------------------------------------------------


    //|///////////////////// RegExAlternative::Constructor //////////////////////
    RegExAlternative::RegExAlternative(RegExCore const &left, RegExCore const &right)
    {
      m_left = left;
      m_right = right;
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
    bool RegExAlternative::consider_first(const char *str, RegExState &state) const
    {
      while (state.substate.size() != 2)
        state.substate.push_back(RegExState(state.context));

      state.first = str;

      // Try left first
      if (m_left.consider_first(str, state.substate[0]))
      {
        state.last = state.substate[0].last;
        return true;
      }

      // Then right
      if (m_right.consider_first(str, state.substate[1]))
      {
        state.last = state.substate[1].last;
        return true;
      }

      return false;
    }


    //|///////////////////// RegExAlternative::consider_next ////////////////////
    bool RegExAlternative::consider_next(RegExState &state) const
    {
      if (state.substate[0].count != 0)
      {
        if (m_left.consider_next(state.substate[0]))
        {
          state.last = state.substate[0].last;
          return true;
        }

        state.substate[0].count = 0;

        // Done with the left, try the right
        if (m_right.consider_first(state.first, state.substate[1]))
        {
          state.last = state.substate[1].last;
          return true;
        }
      }

      if (state.substate[1].count != 0)
      {
        if (m_right.consider_next(state.substate[1]))
        {
          state.last = state.substate[1].last;
          return true;
        }
      }

      return false;
    }


    //|--------------------- RegExGroup -----------------------------------------
    //|--------------------------------------------------------------------------


    //|///////////////////// RegExGroup::Constructor ////////////////////////////
    RegExGroup::RegExGroup(const char *group)
    {
      m_grouping = true;

      if (group[0] == '?' && group[1] == ':')
      {
        group += 2;
        m_grouping = false;
      }

      define(group);
    }


    //|///////////////////// RegExGroup::Destructor /////////////////////////////
    RegExGroup::~RegExGroup()
    {
    }


    //|///////////////////// RegExGroup::consider_first /////////////////////////
    bool RegExGroup::consider_first(const char *str, RegExState &state) const
    {
      state.grouped = m_grouping;

      return RegExCore::consider_first(str, state);
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
    void RegExCore::define(const char *str)
    {
      m_conditions.clear();

      //
      // Parse the regular expression into condition objects
      //
      while (*str != 0)
      {
        if (*str == '.') // wildcard filter object
        {
          m_conditions.push_back(make_unique<RegExFilter>("."));
        }

        else if (*str == '?' || *str == '*' || *str == '+')
        {
          if (m_conditions.size() == 0)
            return;

          if (*str == '?') // repeat last zero or once
          {
            m_conditions.back()->set_repeat(RepeatType::ZeroOrOnce);
          }

          if (*str == '*') // repeat last zero or more
          {
            m_conditions.back()->set_repeat(RepeatType::ZeroOrMore);
          }

          if (*str == '+') // repeat last one or more
          {
            m_conditions.back()->set_repeat(RepeatType::OneOrMore);
          }
        }

        else if (*str == '|') // Alternative
        {
          RegExCore left, right;

          // Move all our conditions into the left
          left.define(m_conditions);

          // All the rest goes on the right
          right.define(str+1);

          m_conditions.clear();
          m_conditions.push_back(make_unique<RegExAlternative>(left, right));

          break;
        }

        else if (*str == '(') // Group
        {
          string group;
          unsigned int indent = 1;

          for( ; str[1] != 0 && indent > 0; ++str)
          {
            if (str[1] == '(')
              ++indent;

            if (str[1] == ')')
              --indent;

            group += *str;
          }

          m_conditions.push_back(make_unique<RegExGroup>(group.c_str()+1));
        }

        else if (*str == '[') // set
        {
          string set;
          unsigned int indent = 1;

          for( ; str[1] != 0 && indent > 0; ++str)
          {
            if (str[1] == '[')
              ++indent;

            if (str[1] == ']')
              --indent;

            set += *str;
          }

          m_conditions.push_back(make_unique<RegExFilter>(set.c_str()));
        }


        else if (*str == '^') // Start Of Line Placeholder
        {
          m_conditions.push_back(make_unique<RegExStartOfLine>());
        }

        else if (*str == '$') // End Of Line Placeholder
        {
          m_conditions.push_back(make_unique<RegExEndOfLine>());
        }

        else if (*str == '\\') // Escaped Character
        {
          char ch[3] = { *str, *++str, 0 };
          m_conditions.push_back(make_unique<RegExFilter>(ch));
        }

        else
        {
          // Any Other Character goes as is
          char ch[2] = { *str, 0 };
          m_conditions.push_back(make_unique<RegExFilter>(ch));
        }

        ++str;
      }
    }


    //|///////////////////// RegExCore::define //////////////////////////////////
    void RegExCore::define(std::vector<shared_ptr<RegExBase>> const &rex)
    {
      m_conditions = rex;
    }


    //|///////////////////// RegExCore::consider_one ////////////////////////////
    bool RegExCore::consider_one(const char *str, RegExState &state) const
    {
      if (m_conditions.size() == 0)
        return true;

      const char *cs = str;
      size_t c = 0;
      size_t n = m_conditions.size();

      while (state.substate.size() != n)
        state.substate.push_back(RegExState(state.context));

      // Jitter until all match or fail.
      do
      {
        if (m_conditions[c]->consider_first(cs, state.substate[c]))
        {
          // Match, move on to next condition
          cs = state.substate[c].last + 1;
          ++c;
        }
        else
        {
          // No Match, move back and expand
          while (c > 0)
          {
            --c;
            if (m_conditions[c]->consider_next(state.substate[c]))
            {
              // Successful expand, try next condition again
              cs = state.substate[c].last + 1;
              ++c;

              break;
            }
          }
        }

      } while ((c % n) != 0);

      if (c != 0 && (c % n) == 0)
      {
        state.last = state.substate[c-1].last;
        ++state.count;

        return true;
      }

      return false;
    }


    //|///////////////////// RegExCore::consider_next ///////////////////////////
    bool RegExCore::consider_next(RegExState &state) const
    {
      if (m_conditions.size() == 0)
        return false;

      const char *cs = state.last+1;
      size_t c = state.count * m_conditions.size();
      size_t n = m_conditions.size();

      // Repeat not allowed or last instance used up no characters (don't repeat nulls infinetly)
      if (m_repeat != RepeatType::Once && (m_repeat != RepeatType::ZeroOrOnce || state.count == 0)
          && (state.count == 0 || state.substate[state.substate.size()-n].first <= state.substate.back().last))
      {
        while (state.substate.size() < (state.count+1)*m_conditions.size())
          state.substate.push_back(RegExState(state.context));
      }

      // Jitter until all match or fail.
      do
      {
        if (c < state.substate.size() && m_conditions[c % n]->consider_first(cs, state.substate[c]))
        {
          // Match, move on to next condition
          cs = state.substate[c].last + 1;
          ++c;
        }
        else
        {
          // No Match, move back and expand
          while (c > 0)
          {
            --c;
            if (m_conditions[c % n]->consider_next(state.substate[c]))
            {
              // Successful expand, try next condition again
              cs = state.substate[c].last + 1;
              ++c;

              break;
            }
          }

        }

      } while ((c % n) != 0);

      if (c != 0 && (c % n) == 0)
      {
        state.last = state.substate[c-1].last;
        state.count = c / n;

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


  //|///////////////////// RegEx::prepare /////////////////////////////////////
  void RegEx::prepare(const char *str)
  {
    m_regex.define(str);
  }



  //|--------------------- GroupVisitor ---------------------------------------
  //|--------------------------------------------------------------------------
  class GroupVisitor : public RegExImpl::RegExStateVisitor
  {
    public:
      GroupVisitor(std::vector<std::string> *groups)
      {
        m_groups = groups;
      }

      virtual ~GroupVisitor()
      {
      }

      void visit(RegExImpl::RegExState &state)
      {
        // only matching matched groups
        if (state.grouped == false || state.count == 0)
          return;

        m_groups->push_back(std::string(state.first, state.last+1));
      }

      std::vector<std::string> *m_groups;
  };



  //|--------------------- match ----------------------------------------------
  //|--------------------------------------------------------------------------
  bool match(RegEx const &rex, const char *str, std::vector<std::string> *groups)
  {
    RegExImpl::RegExContext context;

    context.startofline = str;

    RegExImpl::RegExState state(&context);

    //
    // test for match
    //
    if (rex.m_regex.consider_first(str, state))
    {
      if (groups != NULL)
      {
        //
        // Transfer the marked groups into a vector
        //
        GroupVisitor groupvisitor(groups);

        state.accept(groupvisitor);
      }

      return true;
    }

    return false;
  }


  //|--------------------- search ---------------------------------------------
  //|--------------------------------------------------------------------------
  bool search(RegEx const &rex, const char *str, std::vector<std::string> *groups)
  {
    RegExImpl::RegExContext context;

    context.startofline = str;

    //
    // Iterate through looking for a match
    //
    while (*str != 0)
    {
      RegExImpl::RegExState state(&context);

      if (rex.m_regex.consider_first(str, state))
      {
        if (groups != NULL)
        {
          //
          // Transfer the marked groups into a vector
          //
          GroupVisitor groupvisitor(groups);

          state.accept(groupvisitor);
        }

        return true;
      }

      ++str;
    }

    return false;
  }


} } // namespace regex
