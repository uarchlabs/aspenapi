// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include "registerfile.h"
#include "encoderule.h"
#include <utility>
#include <iostream>

namespace aspen
{

struct Operand
{
  // ---------------------------------------------------------
  // ---------------------------------------------------------
  Operand( std::string _n="",std::string _er="",bool _u="", bool _d="")
    : name(_n),
      encodeRuleName(_er),
      use(_u),
      def(_d)
  {}

  // ---------------------------------------------------------
  // ---------------------------------------------------------
 ~Operand() {}

  void info(std::string i)
  {
    std::cout<<i<<"Operand: "<<name<<"("<<encodeRuleName<<")"<<std::endl;
  }
  // ---------------------------------------------------------
  // ---------------------------------------------------------
  std::string name;
  std::string encodeRuleName;
  bool use;
  bool def;
};

}
