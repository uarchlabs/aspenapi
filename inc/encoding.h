// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include "field.h"
#include <string>
#include <vector>
#include <map>

namespace aspen
{

struct Encoding
{
  // ---------------------------------------------------------------------
  Encoding();
  Encoding(uint32_t,std::string);
  Encoding(uint32_t,std::vector<Field>&);
  // ---------------------------------------------------------------------
 ~Encoding() {}
  // ---------------------------------------------------------------------
  void fieldsFromStr(std::string s);
  void verify();
  void info(std::string i) 
  {
    std::cout<<i<<"Field string  "<<fieldString<<std::endl;
    std::cout<<i<<"Padded opcode "<<paddedOpcode<<std::endl;
    std::cout<<i<<"Fields"<<std::endl;
    for(Field &f : fields) f.info(i+" ");
    std::cout<<i<<"Masks"<<std::endl;
    for(std::pair<std::string,std::string> p : masks) {
      std::cout<<i<<" "<<p.first<<" "<<p.second<<std::endl;
    }
  }
  // ---------------------------------------------------------------------
  // ---------------------------------------------------------------------
  uint32_t width;
  bool     valid;
  std::string fieldString;
  std::string paddedOpcode;
  std::string htmlTable;
  std::vector<Field> fields;
  std::map<std::string,std::string> masks;
};

}
