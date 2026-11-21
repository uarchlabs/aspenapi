#pragma once
#include "properties.h"
#include <vector>
#include <string>

namespace aspen
{
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct ComputationSite
{
  ComputationSite(std::string _n="",
                  uint32_t _t=0,
                  uint32_t _l=0)
    : name(_n),
      throughput(_t),
      latency(_l),
      props()
  {}

//  ComputationSite()
//    : name(""),
//      throughput(-1),
//      latency(-1)
//  {}

 ~ComputationSite() {}

  std::string name;
  uint32_t throughput;
  uint32_t latency;
//  uint32_t props;
  CompSiteProperties props;
};

}
