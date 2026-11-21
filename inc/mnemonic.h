#pragma once
#include <string>

namespace aspen
{
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct Mnemonic
{
  // ----------------------------------------------------------------
  Mnemonic(std::string _m,std::string _p="",std::string _s="")
    : name(_m),
      prefix(_p),
      suffix(_s)
  {}
  // ----------------------------------------------------------------
  Mnemonic()
    : name(""),
      prefix(""),
      suffix("")
  {}

 ~Mnemonic() {}

  std::string name;
  std::string prefix;
  std::string suffix;
};

}
