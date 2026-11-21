#pragma once
#include <stdint.h>
#include <vector>
#include <string>

namespace aspen
{
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct FileFormat
{
  // ----------------------------------------------------------------
  FileFormat()
    : format(""),
      version(""),
      writtenBy("")
  {}
  // ----------------------------------------------------------------
  FileFormat(std::string _n,std::string _v,std::string _wb)
    : format(_n),
      version(_v),
      writtenBy(_wb)
  {}
  // ----------------------------------------------------------------
 ~FileFormat() {}

  // ----------------------------------------------------------------
  void clear()
  {
    format = "";
    version = "";
    writtenBy = "";
  }
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  std::string format;
  std::string version;
  std::string writtenBy;
};

}
