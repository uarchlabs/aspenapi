#pragma once
#include "field.h"
#include "encoding.h"
#include "functionalunit.h"
#include "mnemonic.h"
#include "nvx.h"
#include "opcode.h"
#include "operand.h"
#include "pcode.h"
#include "properties.h"
#include "richtext.h"
#include "state.h"
#include "syntax.h"
#include "timing.h"
#include <vector>
#include <string>
#include <iostream>

namespace aspen
{
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct InstructionWord
{
  InstructionWord() {}
 ~InstructionWord() {}

  std::string name;
  std::vector<Field> fields;
};
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct Instruction
{
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  Instruction()
    : id(0),
      formatName(""),
      mnemonic(),
      action(),
      opcode(),
      syntax(),
      pcode(),
      desc(),
      shortDesc(""),
      funcUnits(),
      props(),
      encoding(),
      timing(),
      operands(),
      notations(),
      state()
  {}
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  Instruction(std::string _m,std::string _opc,uint32_t _w,
              std::string _syn,std::string _shrt,
              std::vector<Operand> _opr,
              std::string _fs)
    : id(0),
      formatName(""),
      mnemonic(_m),
      action(),
      opcode(Opcode(_opc)),
      syntax(Syntax(_syn)),
      pcode(),
      desc(),
      shortDesc(_shrt),
      funcUnits(),
      props(),
      encoding(Encoding(_w,_fs)),
      timing(),
      operands(_opr),
      notations(),
      state()
  {
    opcode.name = _opc;
  }

  // -----------------------------------------------------------------
 ~Instruction() {}
  // -----------------------------------------------------------------
  void setDescription(std::string t) { desc.richText = t; }
  // -----------------------------------------------------------------
  void info(std::string i)
  {

    std::cout<<i<<"Instruction "<<opcode.name<<"("<<id<<")"<<std::endl;
    std::cout<<i<<"  Syntax "<<syntax.key<<std::endl;

    for(Operand &o : operands) o.info(i+"  ");
    encoding.info(i+" ");

  }
  // -----------------------------------------------------------------
  void setDesc(std::string s) { desc.richText = s; }
  // -----------------------------------------------------------------
  uint32_t id;
  std::string formatName;
  Mnemonic mnemonic;
  Nvt action;
  Opcode opcode;

  Syntax syntax;
  Pcode pcode;
  RichText desc;
  std::string shortDesc;
  std::vector<std::string> funcUnits; //unit names
  InsProperties props;

  Encoding encoding;
  InsTiming timing;

  std::vector<Operand> operands;
  std::vector<Nvt> notations;
  State state;
};

}
