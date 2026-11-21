#pragma once
#include "field.h"
#include "register.h"
#include <vector>

namespace aspen
{

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
struct CompSiteProperties
{
  enum csProps {
    CSPROP_NULL     = 1,
    CSPROP_INTEGER  = 2,
    CSPROP_ADDITION = 4,
    CSPROP_SUBTRACT = 8,
    CSPROP_LAST     = 16 
  };

  CompSiteProperties() : val(csProps::CSPROP_NULL) {}
  CompSiteProperties(const CompSiteProperties &rhs) : val(rhs.val) {}
  ~CompSiteProperties() = default;

  CompSiteProperties& operator=(const CompSiteProperties &rhs) {
    if (this != &rhs) {
      val = rhs.val;
    }
    return *this;
  }

  CompSiteProperties operator|(const CompSiteProperties &rhs)
  {
    this->val = static_cast<csProps>(static_cast<int>(this->val)
              | static_cast<int>(rhs.val));
    return *this;
  }

  CompSiteProperties& operator|=(const CompSiteProperties &rhs)
  {
    this->val = static_cast<csProps>(static_cast<int>(this->val)
              | static_cast<int>(rhs.val));
    return *this;
  }

  uint32_t val;

  // OLD
  // ---------------------------------------------------------------------
//  CompSiteProperties operator|(const CompSiteProperties &rhs)
//    { CompSiteProperties a(*this); a.val |= rhs.val; return a; }
//
//  CompSiteProperties& operator|=(const CompSiteProperties &rhs)
//    { val = val|rhs.val; return *this; }
//
//  CompSiteProperties& operator=(const CompSiteProperties &rhs)
//    { if(this != &rhs) { val = rhs.val; } return *this; }
//
//  CompSiteProperties operator=(const CompSiteProperties &rhs)
//    { CompSiteProperties a(*this); a.val = rhs.val; return a; }
//
//  CompSiteProperties operator=(const int &rhs)
//    { CompSiteProperties a(*this); a.val = rhs; return a; }
//
//  // ---------------------------------------------------------------------
//  // ---------------------------------------------------------------------
//  uint32_t val;
};

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
struct InsProperties
{
  enum eProps {
    PROP_NOP,                 //  00
    PROP_HALT,                //  01
    PROP_BR,                  //  02
    PROP_LD,                  //  03
    PROP_ST,                  //  04
    PROP_BYTE,                //  05
    PROP_HALF,                //  06
    PROP_WORD,                //  07
    PROP_DWORD,               //  08
    PROP_RELBR,               //  09
    PROP_ABSBR,               //  10
    PROP_ENABLED,             //  11
    PROP_PREDICATED,          //  12
    PROP_POSITIVEOFFSET,      //  13
    PROP_NEGATIVEOFFSET,      //  14
    PROP_POSITIVEPREDICATE,   //  15
    PROP_NEGATIVEPREDICATE,   //  16
    PROP_PREINCREMENT,        //  17
    PROP_POSTINCREMENT,       //  18
    PROP_PREDECREMENT,        //  19
    PROP_POSTDECREMENT,       //  20
    PROP_PREPOSITIVESTRIDE,   //  21
    PROP_PRENEGATIVESTRIDE,   //  22
    PROP_POSTPOSITIVESTRIDE,  //  23
    PROP_POSTNEGATIVESTRIDE,  //  24
    PROP_LEGALBRANCHSHADOW,   //  25
    PROP_UNDEFINED,           //  26

    PROP_LAST
  };

  // ----------------------------------------------------------------------
  InsProperties() : value(0) {}
 ~InsProperties() = default;
  // ----------------------------------------------------------------------
  // ----------------------------------------------------------------------
  void set(uint32_t pos) {
    value |= 1 << pos;
  }

  void clear(uint32_t pos) {
    value &= ~(1 << pos);
  }

  bool isSet(uint32_t pos) {
    return (((value >> pos) & 1) == 1);
  }

  // ----------------------------------------------------------------------
//NEW  eProps value;
  uint32_t value;
};

}

