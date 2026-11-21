// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include <string>

namespace aspen
{

struct Opcode
{
  Opcode(std::string _n="",std::string _bs="",
         uint32_t _w=0,uint64_t _v=0)
    : name(_n),
      binString(_bs),
      width(_w),
      value(_v)
  {}

 ~Opcode() {}

  std::string name;
  std::string binString;
  uint32_t width;
  uint64_t value;
};

}
