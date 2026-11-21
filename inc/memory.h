// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include <string>

namespace aspen
{
//placeholder
struct Memory
{
  Memory(std::string _n="",std::string _t="",std::string _a="",
         uint32_t _w=0, uint32_t _en=0,uint32_t _sd=0, uint32_t _ld=0)
    : name(_n),
      type(_t),
      access(_a),
      width(_w),
      entries(_en),
      storeDelay(_sd),
      loadDelay(_ld)
  {}

 ~Memory() {}

  std::string name;
  std::string type;
  std::string access;
  uint32_t width;
  uint32_t entries;
  uint32_t storeDelay;
  uint32_t loadDelay;
};

}
