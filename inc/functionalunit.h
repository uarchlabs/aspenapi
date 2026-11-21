#pragma once
#include "computationsite.h"
#include <vector>
#include <string>

namespace aspen
{
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct FunctionalUnit
{
  FunctionalUnit(std::string _n,std::vector<ComputationSite> &_c)
    : name(_n),
      compsites(_c)
  {}

  FunctionalUnit()
    : name(""),
      compsites()
  {}

 ~FunctionalUnit() {}

  std::string name;
  std::vector<ComputationSite> compsites;

};

}
