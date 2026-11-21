// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include "field.h"
#include <vector>
#include <string>

namespace aspen
{
struct Field;
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct Register
{
  // ------------------------------------------------------------------------
  Register()
    : name(""),
      desc(""),
      width(0),
      address(0),
      bits()
  {}

  // ------------------------------------------------------------------------
  Register(std::string _n)
    : name(_n),
      desc(""),
      width(0),
      address(0),
      bits()
  {}

  // ------------------------------------------------------------------------
  Register(std::string _n,std::vector<Field> &_b)
    : name(_n),
      desc(""),
      width(0),
      address(0),
      bits(_b)
  {}

  // ------------------------------------------------------------------------
  Register(std::string _n,std::string _d,uint32_t _w,
           uint64_t _a,std::vector<Field> &_b)
    : name(_n),
      desc(_d),
      width(_w),
      address(_a),
      bits(_b)
  {}
  // ------------------------------------------------------------------------
  ~Register() {}
  // ------------------------------------------------------------------------
  // ------------------------------------------------------------------------
  std::string name;
  std::string desc;
  uint32_t    width;
  uint64_t    address;
  std::vector<Field> bits;
};

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
struct InsPredicate
{
  InsPredicate()
    : predicated(false)
  {}
 ~InsPredicate() {}

  bool predicated;
  std::vector<Register> predRegisters;
};


}
