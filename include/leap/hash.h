//
// hash.h
//

#pragma once

#include <memory>
#include <string>

/**
 * \namespace leap::crypto
 * \brief Crypto Routines
 *
**/

namespace leap { namespace crypto
{
  //-------------------------- MD5 --------------------------------------------
  //---------------------------------------------------------------------------

  struct md5
  {
    md5();
    md5(md5 &&) noexcept;
    md5(md5 const &) = delete;
    ~md5();

    std::string hex() const;

    uint8_t const *data() const;

    static constexpr size_t size() { return 16; }

    void *state;
  };

  void md5_init(md5 *context);
  void md5_update(md5 *context, const void *data, size_t size);
  void md5_finalise(md5 *context);

  md5 md5digest(const void *data, size_t size);


  //-------------------------- SHA1 -------------------------------------------
  //---------------------------------------------------------------------------

  struct sha1
  {
    sha1();
    sha1(sha1 &&) noexcept;
    sha1(sha1 const &) = delete;
    ~sha1();

    std::string hex() const;

    uint8_t const *data() const;

    static constexpr size_t size() { return 20; }

    void *state;
  };

  void sha1_init(sha1 *context);
  void sha1_update(sha1 *context, const void *data, size_t size);
  void sha1_finalise(sha1 *context);

  sha1 sha1digest(const void *data, size_t size);

} } // namespace crypto
