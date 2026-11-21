// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include <string>

namespace aspen
{
//FIXME: needs graph processing methods
struct Pipeline
{
  Pipeline(std::string _n="",std::string _g="",uint32_t _l=0)
    : name(_n),
      graph(_g),
      length(_l)
  {}

 ~Pipeline() {}

  std::string name;
  std::string graph;
  uint32_t length;
};

}
