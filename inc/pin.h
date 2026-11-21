// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include <string>

namespace aspen
{

struct Pin
{
  Pin(std::string _n="",uint32_t _w=0,
      std::string _d="",std::string _t="")
  : name(_n),
    width(_w),
    direction(_d),
    trigger(_t)
  {}

 ~Pin() {}

  std::string name;
  uint32_t width;
  std::string direction;
  std::string trigger;
};

}
