// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include <vector>
#include <string>
#ifndef Q_MOC_RUN
#include <boost/algorithm/string.hpp>
#endif
namespace aspen
{
struct Pcode
{
  Pcode()
    : lines()
  {}

  Pcode(std::vector<std::string> &_v)
    : lines(_v)
  {}

  //FIXME: do this for now
  std::string plainText()
  {
    
    return boost::join(lines,"\n");
  }
 
  std::vector<std::string> lines;
};

}
