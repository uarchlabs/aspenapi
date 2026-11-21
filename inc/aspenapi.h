#pragma once
#include "str.h"
#include "amsg.h"
#include "json/json.h"
//#include "system.h"
#include "instruction.h"
#include "machine.h"
#include <fstream>
#include <utility>
#include <iostream>
#include <sys/stat.h>
#ifndef Q_MOC_RUN
#include "strutil.h"
#endif
// ------------------------------------------------------------------
#define TR_EN 0
#if TR_EN == 1
#define TR(s) std::cout<<s<<std::endl;
#else
#define TR(s)
#endif
// ------------------------------------------------------------------
// FIXME: There are sorting functions in various places, (atm Builder and 
// Docs). Look at defining a common set of sort functions here. Also then
// use these where possible to avoid explict sorts of maps for example
// ------------------------------------------------------------------
namespace aspen
{
struct System;
struct Register;
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct InsDisInfo
{
  aspen::Instruction ins;
  std::vector<std::string> names;
  std::vector<std::string> values;
  std::vector<std::string> erules;
};
// --------------------------------------------------------------
// reverse ordering
// --------------------------------------------------------------
struct less_is_more
{
  inline bool operator() (const uint32_t &s1, const uint32_t &s2) const
  {
    return (s1 > s2);
  }
};
// --------------------------------------------------------------
// FIXME: presumes [A-Za-z]+[0-9]+
// --------------------------------------------------------------
struct sort_as_mnemonic
{
  inline bool operator() (const std::string &s1, const std::string &s2) const
  {

    std::string s1a = s1;
    std::string s2a = s2;

    size_t pos1 = s1a.find_first_of("0123456789");
    size_t pos2 = s2a.find_first_of("0123456789");

    s1a.erase(0,pos1);
    s2a.erase(0,pos2);

    uint32_t s1i,s2i;

    try {
      s1i = aspen::toU32(s1a);
      s2i = aspen::toU32(s2a);
      return (s1i < s2i);
    } catch(std::exception&) {
      return (s1 < s2);
    }
  }
};
// --------------------------------------------------------------
// FIXME: presumes [A-Za-z][0-9]+
// --------------------------------------------------------------
struct sort_register_names
{
  inline bool operator() (const aspen::Register &s1, 
                          const aspen::Register &s2) const
  {
    std::string s1a = s1.name;
    std::string s2a = s2.name;
    s1a.erase(0,1);
    s2a.erase(0,1);

    uint32_t s1i,s2i;

    try {
      s1i = aspen::toU32(s1a);
      s2i = aspen::toU32(s2a);
      return (s1i < s2i);
    } catch(std::exception&) {
      return (s1.name < s2.name);
    }
  }
};
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct AspenApi
{
  AspenApi()
    : instructionId(0),
      silent(false),
      showbanner(true),
      currentMachine()
 {
   amsg.setWho("AspenApi");
 }

 ~AspenApi() {}

  // -----------------------------------------------------------------
  // -----------------------------------------------------------------
  enum ApiClass { ALL,INSTRUCTIONS,OPCODES,OPCNAMES_SHDESC };
  // -----------------------------------------------------------------
  // -----------------------------------------------------------------
  bool expandSyntax(Machine&,Instruction&);
  bool resolveInstructionSet(System&,Machine&);
  bool updateOpcodeValue(Machine&,Instruction&);
  bool updateInstructionFormats(Machine&);
  bool updateEncoding(System&,Machine&,Instruction&);
  bool updateFieldMasks(Machine&,Instruction&);
  bool updateNamedBits(Machine&);
  bool updateRegisterFiles(Machine&);
  bool updateRegisters(Machine&);
  bool updateEncodeRules(Machine&);
  bool mdlCleanUp(Machine&);

  std::map<std::string,Machine>::iterator findMachine(System&,std::string&);
  bool findMachine(System&,std::string&,Machine*);
  // -----------------------------------------------------------------
  bool goodBinString(std::string&);
  std::pair<uint32_t,bool>    getWidthFromBinString(std::string&);
  std::pair<uint64_t,bool>    getValueFromBinString(std::string&);
  std::pair<std::string,bool> getValueStrFromBinString(std::string&);
  std::string getHexStrFromBinStr(std::string&,bool);
  bool        getMaskedString(std::string&,std::string&,std::string&);
  std::string formSyntaxString(std::vector<std::string>&,bool);
  // -----------------------------------------------------------------
  std::pair<std::string,bool>
       getRegFileNameFromErule(Machine&,std::string,bool);
  // -----------------------------------------------------------------
  bool disassembleStr(Machine&,std::string&,InsDisInfo&);
  bool getDisassemblyValues(Machine&,Instruction&,std::string&,InsDisInfo&);
  bool disassembleIword(Machine&,const std::string&,
                        std::vector<InsDisInfo>&);
  std::string formatInsDisInfo(std::vector<InsDisInfo>&);
  std::string formatInsDisInfo(std::vector<InsDisInfo>&,bool);
  // -----------------------------------------------------------------
  bool readMdl(System&,std::string);
  bool rdMdlFileFormat(System&);
  bool rdMdlChangeLog(System&);
  bool rdMdlUserData(System&);

  bool rdMdlSystemInfo(System&);
  bool rdMdlSystemDefines(System&);
  bool rdMdlSystemFiles(System&);
  bool rdMdlSystemHierarchy(System&);
  bool rdMdlSystemPins(System&);
  bool rdMdlSystemMemory(System&);
  bool rdMdlSystemDirectives(System &m);
  bool rdMdlSystemRegisters(System &m);
  bool rdMdlSystemInstances(System &m);

  bool rdMdlMachineInfo(Machine&,Json::Value::iterator);
  bool rdMdlMachineDefines(Machine&,Json::Value::iterator);
  bool rdMdlMachineActions(Machine&,Json::Value::iterator);
  bool rdMdlMachineEncodeRules(Machine&,Json::Value::iterator);
  bool rdMdlMachineInstructions(Machine&,Json::Value::iterator);
  bool rdMdlMachineInstructionWords(Machine&,Json::Value::iterator);
  bool rdMdlMachineMemory(Machine&,Json::Value::iterator);
  bool rdMdlMachineOpcodes(Machine&,Json::Value::iterator);
  bool rdMdlMachinePins(Machine&,Json::Value::iterator);
  bool rdMdlMachineSyntax(Machine&,Json::Value::iterator);
  bool rdMdlMachinePipelines(Machine&,Json::Value::iterator);
  bool rdMdlMachineProperties(Machine&,Json::Value::iterator);
  bool rdMdlMachineRegisterFiles(Machine&,Json::Value::iterator);
  bool rdMdlMachineNamedBits(Machine&,Json::Value::iterator);

//  bool rdMdlMachineCompSites(Machine&,Json::Value::iterator);
//  bool rdMdlMachineFunits(Machine&,Json::Value::iterator);
//  bool rdMdlMachineHierarchy(Machine&,Json::Value::iterator);
//  bool rdMdlMachineResources(Machine&,Json::Value::iterator);
  // -----------------------------------------------------------------
  bool writeMdl(System&,std::string);
  void wrMdlFileFormat(System&);
  void wrMdlChangelog(System&);
  void wrMdlUserData(System&);

  void wrMdlSystemInfo(System&);
  void wrMdlSystemDefines(System&);
  void wrMdlSystemFiles(System&);
  void wrMdlSystemHierarchy(System&);
  void wrMdlSystemPins(System&);
  void wrMdlSystemMemory(System&);
  void wrMdlSystemDirectives(System&);
  void wrMdlSystemRegisters(System&);
  void wrMdlSystemInstances(System&);

  void wrMdlMachineInfo(Machine&);
  void wrMdlMachineActions(Machine&);
  void wrMdlMachineDefines(Machine&);
  void wrMdlMachineEncodeRules(Machine&);
  void wrMdlMachineInstructions(Machine&);
  void wrMdlMachineInstructionWords(Machine&);
  void wrMdlMachineMemory(Machine&);
  void wrMdlMachineOpcodes(Machine&);
  void wrMdlMachinePins(Machine&);
  void wrMdlMachineSyntax(Machine&);
  void wrMdlMachinePipelines(Machine&);
  void wrMdlMachineProperties(Machine&);
  void wrMdlMachineRegisterFiles(Machine&);
  void wrMdlMachineNamedBits(Machine&);

//  void wrMdlMachineCompSites(Machine&);
//  void wrMdlMachineFunits(Machine&);
//  void wrMdlMachineHierarchy(Machine&);
//  void wrMdlMachineResources(Machine&);
  bool testJsObj(Json::Value::iterator&,std::string);
  // -----------------------------------------------------------------
  std::string apiVersion() { return API_VERSION; }
  void reportVersion();
  // -----------------------------------------------------------------
  bool        tryStoi(std::string&,std::string&,uint32_t);
  uint32_t    numBitsToEncode(uint32_t);
  std::string to_caps(std::string);
  std::string padstr(std::string,uint32_t,std::string);
  std::string itostr(uint32_t);
  void sortByInsWidth(std::map<std::string,Instruction>&,
                      std::map<uint32_t,std::vector<Instruction> >&);
  uint32_t getMaxInstructionWidth(Machine &);
  // -----------------------------------------------------------------
  void info(ApiClass,Machine&);
  void standardizeOperandNames(Instruction&);
  // -----------------------------------------------------------------
  // placeholder
  // -----------------------------------------------------------------
  void clear() {}
  // -----------------------------------------------------------------
  static std::string createTimeStamp()
  {
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [80];
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer,80,"%d-%m-%Y %I:%M:%S",timeinfo);
  
    std::string timeStamp = buffer;
    return timeStamp;
  }
  // -----------------------------------------------------------------
  // save a little typing
  // -----------------------------------------------------------------
  void dmsg(std::string m) { if(!silent) amsg.dmsg(m); }
  void emsg(std::string m) { amsg.emsg(m); }
  void imsg(std::string m) { if(!silent) amsg.imsg(m); }
  void wmsg(std::string m) { if(!silent) amsg.wmsg(m); }
  // -----------------------------------------------------------------
  // -----------------------------------------------------------------
  AMsg amsg;

  uint32_t instructionId;
  bool silent;
  bool showbanner;
  Machine  currentMachine;

  const static std::string API_VERSION;
};

}
