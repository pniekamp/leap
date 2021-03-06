//
// Simple Attribute Pair Stream Reader
//
//   Peter Niekamp, August, 2005
//

//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#pragma once

#include <leap/util.h>
#include <sstream>
#include <fstream>

/**
 * \namespace leap
 * \brief Leap Library containing common helper routines
 *
**/

/**
 * \defgroup leapdata Data Containors
 * \brief Data Containors
 *
**/

namespace leap
{
  template<typename T, class traits> class basic_sapentry;
  template<typename T, class traits> class basic_sapstream;
  template<typename T, class traits> class basic_issapstream;
  template<typename T, class traits> class basic_ifsapstream;


  //|------------------------- sapstream ------------------------------------
  //|------------------------------------------------------------------------
  /**
   * \ingroup leapdata
   *
   * \brief Simple Attribute Pair Stream Reader
   *
   * The Simple Attribute Pair Stream is a text stream of the form :-
   *
   * \code
   *   <EntryType> <EntryId>
   *   {
   *     <attribute> [ = <value> ]
   *     <attribute> [ = <value> ]
   *   }
   * \endcode
   *
   * Entries may also be nested. Use of #defines is supported.
   *
  **/

  template<typename T, class traits = std::char_traits<T>>
  class basic_sapstream
  {
    public:

      using entry_type = basic_sapentry<T, traits>;
      using string_type = std::basic_string<T, traits>;
      using streambuf_type = std::basic_streambuf<T, traits>;
      using string_view_type= leap::basic_string_view<T, traits>;

      enum ParseFlags
      {
        FullParse        = 0x00,
        NoControlChars   = 0x01,
        NoEnvironment    = 0x02,
        NoNameSeparation = 0x04,
      };

    public:
      basic_sapstream();

      void set_parse_options(long flags);

      void define(string_type name, string_type value);

      explicit operator bool() const { return (m_state == std::ios_base::goodbit); }

      void set_state(std::ios_base::iostate state) { m_state = state; }

      basic_sapstream &operator >>(entry_type &entry);

    public:

      streambuf_type *rdbuf() const { m_sb->pubseekpos(m_sbpos, std::ios_base::in); return m_sb; }
      streambuf_type *rdbuf(streambuf_type *sb);

      void rewind();

    private:

      bool is_eol(T ch) const { return (ch == 0x0A || ch == 0x0D || ch == 0); }
      bool is_white(T ch) const { return (ch == ' ' || ch == '\t' || ch == 0x0A || ch == 0x0D); }

      bool getline(T *buffer, unsigned int n);

      void preparse(T const *src, T *dest, unsigned int n);
      void parse_hashdefine(T *buffer);
      void parse_headerline(T *buffer1, entry_type *entry);
      void parse_entryline(T *buffer, entry_type *entry);

      string_type expand(string_view_type src);

    private:

      struct Variable
      {
        string_type name;
        string_type value;
      };

      std::vector<Variable> m_variables;

    private:

      long m_flags;

      std::ios_base::iostate m_state;

      std::streampos m_sbpos;

      streambuf_type *m_sb;
  };


  //|///////////////////// sapstream::Constructor ///////////////////////////
  template<typename T, class traits>
  basic_sapstream<T, traits>::basic_sapstream()
    : m_sbpos(0)
  {
    m_flags = FullParse;

    m_sb = NULL;
    m_state = std::ios_base::eofbit;
  }


  //|///////////////////// sapstream::set_parse_options /////////////////////
  template<typename T, class traits>
  void basic_sapstream<T, traits>::set_parse_options(long flags)
  {
    m_flags = flags;
  }


  //|///////////////////// sapstream::define ////////////////////////////////
  template<typename T, class traits>
  void basic_sapstream<T, traits>::define(string_type name, string_type value)
  {
    m_variables.push_back({ std::move(name), std::move(value) });
  }


  //|///////////////////// sapstream::rdbuf /////////////////////////////////
  template<typename T, class traits>
  std::basic_streambuf<T,traits> *basic_sapstream<T, traits>::rdbuf(std::basic_streambuf<T,traits> *sb)
  {
    streambuf_type *old = m_sb;

    m_sb = sb;

    if (m_sb != NULL)
      m_sbpos = m_sb->pubseekoff(0, std::ios_base::cur, std::ios_base::in);

    m_state = (m_sb != NULL) ? std::ios_base::goodbit : std::ios_base::eofbit;

    return old;
  }


  //|///////////////////// sapstream::rewind ////////////////////////////////
  template<typename T, class traits>
  void basic_sapstream<T, traits>::rewind()
  {
    m_sbpos = 0;

    m_state = (m_sb != NULL) ? std::ios_base::goodbit : std::ios_base::eofbit;
  }


  //|///////////////////// sapstream::getline ///////////////////////////////
  template<typename T, class traits>
  bool basic_sapstream<T, traits>::getline(T *buffer, unsigned int n)
  {
    m_sb->pubseekpos(m_sbpos, std::ios_base::in);

    if (m_sb->sgetc() == traits::eof())
      return false;

    // Retreive characters until eol
    while (--n > 0 && m_sb->sgetc() != traits::eof() && !is_eol(m_sb->sgetc()))
      *buffer++ = m_sb->sbumpc();

    *buffer = 0;

    // Skip over eol character(s)
    while (is_eol(m_sb->sgetc()))
      m_sb->sbumpc();

    m_sbpos = m_sb->pubseekoff(0, std::ios_base::cur, std::ios_base::in);

    return true;
  }


  //|///////////////////// sapstream::pre_parse /////////////////////////////
  template<typename T, class traits>
  void basic_sapstream<T, traits>::preparse(T const *src, T *dest, unsigned int n)
  {
    // Check for whole line comments
    if (*src == '#' || *src == '/' || *src == '!' || *src == '{' || *src == '}')
    {
      *dest = 0;
      return;
    }

    //
    // Transfer src to dest
    //

    T *ch = dest;
    while (--n > 0 && !is_eol(*src))
      *ch++ = *src++;

    *ch = 0;
    while (ch >= dest && (*ch == 0 || is_white(*ch)))
      *ch-- = 0;
  }


  //|///////////////////// sapstream::parse_setvariable /////////////////////
  template<typename T, class traits>
  void basic_sapstream<T, traits>::parse_hashdefine(T *buffer)
  {
    // skip "#define"
    buffer += 7;

    // skip whitespaces
    while (is_white(*buffer))
      ++buffer;

    //
    // Extract Name
    //

    T const *name = buffer;

    while (*buffer != 0 && !is_white(*buffer))
      ++buffer;

    if (*buffer != 0)
    {
      *buffer++ = 0;
    }

    // skip whitespace
    while (is_white(*buffer))
      ++buffer;

    //
    // Extract Value
    //

    T const *value = buffer;

    //
    // Define Variable
    //

    define(name, value);
  }


  //|///////////////////// sapstream::parse_headerline //////////////////////
  template<typename T, class traits>
  void basic_sapstream<T, traits>::parse_headerline(T *buffer, basic_sapentry<T, traits> *entry)
  {
    static T const entrytype_literal[] = { 'E', 'n', 't', 'r', 'y', 'T', 'y', 'p', 'e', 0 };
    static T const entryid_literal[] = { 'E', 'n', 't', 'r', 'y', 'I', 'd', 0 };

    entry->clear();
    entry->push_substream(*this);

    //
    // Extract EntryType
    //

    T const *type = buffer;

    while (*buffer != 0 && !is_white(*buffer))
      ++buffer;

    if (*buffer != 0)
    {
      *buffer++ = 0;
    }

    // Skip Whitespace
    while (is_white(*buffer))
      ++buffer;

    entry->add(entrytype_literal, type);

    //
    // Extract EntryId
    //

    T const *id = buffer;

    entry->add(entryid_literal, id);
  }


  //|///////////////////// sapstream::parse_entryline ///////////////////////
  template<typename T, class traits>
  void basic_sapstream<T, traits>::parse_entryline(T *buffer, basic_sapentry<T, traits> *entry)
  {
    if (*buffer == 0)
      return;

    string_type name;
    string_type value;

    if (!(m_flags & NoNameSeparation))
    {
      //
      // Extract Name
      //

      T const *beg = buffer;

      while (*buffer != 0 && *buffer != '=' && *buffer != '@' && *buffer != ':')
        ++buffer;

      T const *end = buffer - 1;

      if (*buffer != 0)
      {
        while (is_white(*end))
          --end;

        ++buffer;
      }

      name = string_type(beg, end + 1);

      // Skip Whitespace
      while (is_white(*buffer))
        ++buffer;
    }

    //
    // Extract Value
    //

    value = expand(buffer);

    if (!(m_flags & NoEnvironment))
      value = strvpnd(value);

    if (!(m_flags & NoControlChars))
      value = strxpnd(value);

    //
    // Add Entry
    //

    entry->add(name, value);
  }


  //|///////////////////// sapstream::expand ////////////////////////////////
  template<typename T, class traits>
  std::basic_string<T, traits> basic_sapstream<T, traits>::expand(string_view_type src)
  {
    string_type result;

    size_t pos = 0;
    size_t next = 0;

    while ((next = src.find('$', next)) != string_view_type::npos)
    {
      size_t beg = src.find('{', next+1);
      size_t end = src.find('}', next+1);

      if (beg == next+1 && end != string_view_type::npos)
      {
        // Extract Variable Name
        auto varname = src.substr(beg+1, end-beg-1);

        string_type varvalue;
        for(size_t j = 0; j < m_variables.size(); ++j)
        {
          if (strcmp(m_variables[j].name, varname) == 0)
          {
            varvalue = expand(m_variables[j].value);
            break;
          }
        }

        if (!varvalue.empty())
        {
          // Copy in preceeding text
          result.append(src.begin() + pos, src.begin() + next);

          // Copy in variable text
          result.append(varvalue.begin(), varvalue.end());

          pos = end+1;
          next = end;
        }
      }

      ++next;
    }

    result.append(src.begin() + pos, src.end());

    return result;
  }


  //|///////////////////// sapstream::operator >> ///////////////////////////
  ///
  /// Retreive an entry from the stream
  ///
  /// \param[out] entry the entry retreived (if no error)
  /// \return stream
  ///
  template<typename T, class traits>
  basic_sapstream<T, traits> &basic_sapstream<T, traits>::operator >>(basic_sapentry<T, traits> &entry)
  {
    static T const hashdefine_literal[] = { '#', 'd', 'e', 'f', 'i', 'n', 'e' };

    int level = 0;
    T buffer1[512];
    T buffer2[512];

    buffer1[0] = buffer2[0] = 0;

    //
    // Start moving through the entries
    // (note that buffer2 is one line ahead of buffer1... buffer1 is what counts)
    //
    while (getline(buffer2, sizeof(buffer2)))
    {
      size_t pos = 0;

      // Skip Whitespaces
      while (is_white(buffer2[pos]))
        ++pos;

      if (traits::compare(&buffer2[pos], hashdefine_literal, extentof(hashdefine_literal)) == 0)
        parse_hashdefine(&buffer2[pos]);

      // Check if we are nesting
      if (buffer2[pos] == '{')
        ++level;

      // if we are the correct level (not nested)
      if (level == 1)
      {
        if (buffer2[pos] == '{')
        {
          parse_headerline(buffer1, &entry);
        }
        else
        {
          parse_entryline(buffer1, &entry);
        }
      }

      // Check if we are un-nesting
      if (buffer2[pos] == '}')
      {
        --level;

        // unnesting before we even begun ?
        if (level < 0)
          m_state = std::ios_base::failbit;

        if (level <= 0)
          return *this;
      }

      if (level <= 1)
      {
        preparse(&buffer2[pos], buffer1, sizeof(buffer1));
      }
    }

    // End of file without a valid entry

    m_state = std::ios_base::eofbit;

    return *this;
  }




  //|--------------------- sapentry -----------------------------------------
  //|------------------------------------------------------------------------
  /**
   * \ingroup leapdata
   *
   * \brief Simple Data Definition Entry Object
   *
   *
  **/

  template<typename T, class traits = std::char_traits<T>>
  class basic_sapentry
  {
    public:

      using string_type = std::basic_string<T, traits>;
      using string_view_type = leap::basic_string_view<T, traits>;

      struct Attribute
      {
        string_type name;
        string_type value;
      };

      using const_iterator = typename std::vector<Attribute>::const_iterator;

    public:

      bool defined(string_view_type name) const;

      string_type lookup(string_view_type name, string_type defval = {}) const;

      string_type const &operator [](string_view_type name) const;

    public:

      size_t size() const { return m_attributes.size(); }

      Attribute const &operator [](size_t i) const { return m_attributes[i]; }

      const_iterator begin() const { return m_attributes.begin(); }
      const_iterator end() const { return m_attributes.end(); }

    public:

      void clear();

      void add(string_type name, string_type value);
      void insert(size_t index, string_type name, string_type value);

      void push_substream(basic_sapstream<T, traits> const &stream);

      basic_sapstream<T, traits> &substream();
      basic_sapstream<T, traits> const &substream() const;

    private:

      basic_sapstream<T, traits> m_substream;

    private:

      std::vector<Attribute> m_attributes;
  };

  using sapentry = basic_sapentry<char>;
  using wsapentry = basic_sapentry<wchar_t>;


  //|///////////////////// sapentry::clear //////////////////////////////////
  template<typename T, class traits>
  void basic_sapentry<T, traits>::clear()
  {
    m_attributes.clear();

    m_substream.rdbuf(NULL);
  }


  //|///////////////////// sapentry::add ////////////////////////////////////
  template<typename T, class traits>
  void basic_sapentry<T, traits>::add(string_type name, string_type value)
  {
    m_attributes.push_back({ std::move(name), std::move(value) });
  }


  //|///////////////////// sapentry::insert /////////////////////////////////
  template<typename T, class traits>
  void basic_sapentry<T, traits>::insert(size_t index, string_type name, string_type value)
  {
    m_attributes.insert(m_attributes.begin() + index, { std::move(name), std::move(value) });
  }


  //|///////////////////// sapentry::push_substream /////////////////////////
  template<typename T, class traits>
  void basic_sapentry<T, traits>::push_substream(basic_sapstream<T, traits> const &stream)
  {
    m_substream = stream;
  }


  //|///////////////////// sapentry::substream //////////////////////////////
  template<typename T, class traits>
  basic_sapstream<T, traits> &basic_sapentry<T, traits>::substream()
  {
    return m_substream;
  }


  //|///////////////////// sapentry::substream //////////////////////////////
  template<typename T, class traits>
  basic_sapstream<T, traits> const &basic_sapentry<T, traits>::substream() const
  {
    return m_substream;
  }


  //|///////////////////// sapentry::defined ////////////////////////////////
  template<typename T, class traits>
  bool basic_sapentry<T, traits>::defined(string_view_type name) const
  {
    for(auto &attribute : m_attributes)
    {
      if (stricmp(attribute.name, name) == 0)
        return true;
    }

    return false;
  }


  //|///////////////////// sapentry::lookup /////////////////////////////////
  template<typename T, class traits>
  std::basic_string<T, traits> basic_sapentry<T, traits>::lookup(string_view_type name, string_type defval) const
  {
    for(auto &attribute : m_attributes)
    {
      if (stricmp(attribute.name, name) == 0)
        return attribute.value;
    }

    return std::move(defval);
  }


  //|///////////////////// sapentry::operator[] /////////////////////////////
  template<typename T, class traits>
  std::basic_string<T, traits> const &basic_sapentry<T, traits>::operator[](string_view_type name) const
  {
    static std::basic_string<T, traits> nullstr;

    for(auto &attribute : m_attributes)
    {
      if (stricmp(attribute.name, name) == 0)
        return attribute.value;
    }

    return nullstr;
  }



  //|--------------------- basic_issapstream --------------------------------
  //|------------------------------------------------------------------------

  template<typename T, class traits = std::char_traits<T>>
  class basic_issapstream : public basic_sapstream<T, traits>
  {
    public:
      basic_issapstream(std::basic_string<T, traits> const &str);
      ~basic_issapstream();
  };

  using issapstream = basic_issapstream<char>;
  using iwssapstream = basic_issapstream<wchar_t>;


  //|///////////////////// issapstream::Constructor /////////////////////////
  template<typename T, class traits>
  basic_issapstream<T, traits>::basic_issapstream(std::basic_string<T, traits> const &str)
  {
    this->rdbuf(new std::basic_stringbuf<T, traits>(str));
  }


  //|///////////////////// issapstream::Destructor //////////////////////////
  template<typename T, class traits>
  basic_issapstream<T, traits>::~basic_issapstream()
  {
    delete basic_sapstream<T, traits>::rdbuf();
  }



  //|--------------------- basic_ifsapstream --------------------------------
  //|------------------------------------------------------------------------

  template<typename T, class traits = std::char_traits<T>>
  class basic_ifsapstream : public basic_sapstream<T, traits>
  {
    public:
      basic_ifsapstream(T const *filename, std::ios::openmode mode = std::ios::in);
      ~basic_ifsapstream();
  };

  using ifsapstream = basic_ifsapstream<char>;
  using wifsapstream = basic_ifsapstream<wchar_t>;


  //|///////////////////// ifsapstream::Constructor /////////////////////////
  template<typename T, class traits>
  basic_ifsapstream<T, traits>::basic_ifsapstream(T const *filename, std::ios::openmode mode)
  {
    auto filebuf = new std::basic_filebuf<T, traits>;

    filebuf->open(filename, mode | std::ios::binary);

    basic_sapstream<T, traits>::rdbuf(filebuf);

    if (!filebuf->is_open())
      basic_sapstream<T, traits>::set_state(std::ios_base::badbit);
  }


  //|///////////////////// ifsapstream::Destructor //////////////////////////
  template<typename T, class traits>
  basic_ifsapstream<T, traits>::~basic_ifsapstream()
  {
    delete basic_sapstream<T, traits>::rdbuf();
  }

} // namespace
