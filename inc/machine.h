// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include "json/json.h"
#include "nvx.h"
#include "pin.h"
#include "pipeline.h"
#include "memory.h"
#include "fileformat.h"
#include "changelog.h"
#include "instruction.h"
#include <vector>
#include <map>
#include <string>

namespace aspen
{

struct Machine
{
  Machine()
    : name(""),
      version(""),
      config(""),
      fileFormat(),
      changeLog(),
      actions(),
      userData(),
      syntax(),
      defines(),
      opcodes(),
      encodeRules(),
      instructions(),
      opcodeNames(),
      instructionWords(),
      pipelines(),
      properties(),
      registerFiles(),
      functionalUnits(),
      namedBits(),
      pins(),
      memories(),
      root()
  {}

 ~Machine() {}

  void clear()
  { 
    name = "";
    version = "";
    config = "";
    fileFormat.clear();
    changeLog.clear();
    actions.clear();
    userData.clear();
    syntax.clear(),
    defines.clear();
    opcodes.clear();
    encodeRules.clear();
    instructions.clear();
    opcodeNames.clear();
    instructionWords.clear();
    pipelines.clear();
    properties.clear();
    registerFiles.clear();
    functionalUnits.clear(),
    namedBits.clear();
    pins.clear();
    memories.clear();

    root.clear();
  }

  std::string name;
  std::string version;
  std::string config;
  FileFormat fileFormat;
  ChangeLog changeLog;
  std::map<std::string,Nvp> actions;
  std::vector<Nvt> userData;
  std::map<std::string,Syntax> syntax;
  std::map<std::string,Nvt> defines;
  std::map<std::string,Opcode> opcodes;
  std::map<std::string,EncodeRule> encodeRules;
  std::map<std::string,Instruction> instructions;
  std::map<std::string,uint64_t> opcodeNames;
  std::vector<InstructionWord> instructionWords;
  std::vector<Pipeline> pipelines;
  std::vector<Nvp> properties;
  std::vector<RegisterFile> registerFiles;
  std::vector<FunctionalUnit> functionalUnits;
  std::map<std::string,std::vector<Field> > namedBits;
  std::vector<Pin> pins;
  std::vector<Memory> memories;
  std::map<uint32_t,std::map<std::string,std::string> > instructionFormats;

  Json::Value root;
};

}
