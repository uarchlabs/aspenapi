// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include <iostream>

namespace aspen
{
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct Field
{
  enum FieldProperties { NONE, RW, RO, RAZ, WO, OPERAND,
                         OPCODE, IWORD, FIXME_ADD_MORE };

  // ----------------------------------------------------------------
  Field()
    : name(""),
      msb(0),
      lsb(0),
      posMsb(0),
      posLsb(0),
      value(0xdeadbeef),
      strValue(),
      props(NONE)
  {}
  // ----------------------------------------------------------------
  Field(std::string _n,uint32_t _msb,uint32_t _lsb,uint32_t _val,
        FieldProperties _p=RW)
    : name(_n),
      msb(_msb),
      lsb(_lsb),
      posMsb(0),
      posLsb(0),
      value(_val),
      strValue(),
      props(_p)
  {}
  // ----------------------------------------------------------------
 ~Field() {}

  void info(std::string i) 
  {
    std::cout<<i<<"Name      "<<name<<std::endl;
    std::cout<<i<<"MSB/LSB   "<<msb<<":"<<lsb<<std::endl;
    std::cout<<i<<"PMSB/PLSB "<<posMsb<<":"<<posLsb<<std::endl;
    std::cout<<i<<"Value     "<<value<<std::endl;
  }
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  std::string name;
  uint32_t msb;    //the range extracted from the operand and encoded
  uint32_t lsb;    
  uint32_t posMsb; //the position of this field in the iword
  uint32_t posLsb;
  uint32_t value;
  std::string strValue;
  FieldProperties props; 
};

}
