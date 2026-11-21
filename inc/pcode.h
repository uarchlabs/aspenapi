#pragma once
#include <vector>
#include <string>
#include "strutil.h"
namespace aspen
{
struct Pcode
{
  Pcode()
    : lines()
  {}

  Pcode(std::vector<std::string> &_v)
    : lines(_v)
  {}

  //FIXME: do this for now
  std::string plainText()
  {
    
    return joinStrings(lines,"\n");
  }
 
  std::vector<std::string> lines;
};

}
