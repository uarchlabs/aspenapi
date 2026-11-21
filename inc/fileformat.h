// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include <stdint.h>
#include <vector>
#include <string>

namespace aspen
{
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct FileFormat
{
  // ----------------------------------------------------------------
  FileFormat()
    : format(""),
      version(""),
      writtenBy("")
  {}
  // ----------------------------------------------------------------
  FileFormat(std::string _n,std::string _v,std::string _wb)
    : format(_n),
      version(_v),
      writtenBy(_wb)
  {}
  // ----------------------------------------------------------------
 ~FileFormat() {}

  // ----------------------------------------------------------------
  void clear()
  {
    format = "";
    version = "";
    writtenBy = "";
  }
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  std::string format;
  std::string version;
  std::string writtenBy;
};

}
