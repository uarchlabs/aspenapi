#pragma once
#include <map>
#include <string>
#include <iostream>

namespace aspen
{
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct EncodeRule
{
  enum eERULE_TYPE { ERULE_NONE,ERULE_UIMM,ERULE_SIMM,ERULE_REG };

  // ---------------------------------------------------------------
  // ---------------------------------------------------------------
  EncodeRule()
    : name(""),
      type(ERULE_NONE),
      width(0),
      range(),
      mapping(),
      imapping(),
      regFileName("")
  {}
  // ---------------------------------------------------------------
  // ---------------------------------------------------------------
  EncodeRule(std::string _n,eERULE_TYPE _t,uint32_t _w,
             std::pair<uint32_t,uint32_t> &_r,
             std::map<std::string,std::string> &_m,
             std::string _rfname)
    : name(_n),
      type(_t),
      width(_w),
      range(_r),
      mapping(_m),
      imapping(),
      regFileName(_rfname)
  { }
  // ---------------------------------------------------------------
  // ---------------------------------------------------------------
  ~EncodeRule() {}
  // ---------------------------------------------------------------
  std::string typeToString() {
    switch(type) {
      case ERULE_NONE: return "NONE";
      case ERULE_UIMM: return "UIMM";
      case ERULE_SIMM: return "SIMM";
      case ERULE_REG:  return "REG"; 
    }
    return "UNKNOWN";
  }
  // ---------------------------------------------------------------
  // ---------------------------------------------------------------
  std::string name;
  eERULE_TYPE type;
  uint32_t    width;
  std::pair<uint32_t,uint32_t> range;
  std::map<std::string,std::string> mapping;
  std::map<uint32_t,std::string> imapping;
  std::string regFileName;
};

}
