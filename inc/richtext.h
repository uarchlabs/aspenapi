// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include <string>

namespace aspen
{

struct RichText
{
  RichText(std::string _s)
    : richText(_s)
  {}

  RichText() 
    : richText("")
  {} 

 ~RichText() {}

  //FIXME: for now
  std::string rich2PlainText() { return richText; }
  std::string richText;
};

}
