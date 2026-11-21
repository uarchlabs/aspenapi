#include "aspenapi.h"
#include "system.h"
#include "pipeline.h"
#include <string>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <typeinfo>
#include <algorithm>
#include <map>

using namespace std;
namespace aspen
{
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::writeMdl(System &s,string fileName)
{
  imsg("Creating MDL for : "+s.name);
  imsg("Location :"+fileName);

  ofstream out(fileName.c_str());
  if(!out.is_open()) {
    emsg("Could not open file : "+fileName);
    return false;
  }

  s.root.clear();

  wrMdlFileFormat(s);
  wrMdlChangelog(s);
  wrMdlUserData(s);

  wrMdlSystemInfo(s);
  wrMdlSystemDefines(s);
  wrMdlSystemFiles(s);
  wrMdlSystemHierarchy(s);
  wrMdlSystemPins(s);
  wrMdlSystemMemory(s);
  wrMdlSystemDirectives(s);
  wrMdlSystemRegisters(s);
  wrMdlSystemInstances(s);

  s.root["machines"] = Json::arrayValue;

  map<string,Machine>::iterator q; 
  for(q=s.machines.begin();q!=s.machines.end();++q) {

    wrMdlMachineInfo(q->second);
    wrMdlMachineDefines(q->second);
    wrMdlMachineActions(q->second);
    wrMdlMachineEncodeRules(q->second);
    wrMdlMachineInstructions(q->second);
    wrMdlMachineInstructionWords(q->second);
    wrMdlMachineMemory(q->second);
    wrMdlMachinePins(q->second);
    wrMdlMachineOpcodes(q->second);
    wrMdlMachineSyntax(q->second);
    wrMdlMachinePipelines(q->second);
    wrMdlMachineProperties(q->second);
    wrMdlMachineRegisterFiles(q->second);
    wrMdlMachineNamedBits(q->second);

//    wrMdlMachineCompSites(q->second);
//    wrMdlMachineFunits(q->second);
//    wrMdlMachineHierarchy(q->second);
//    wrMdlMachineResources(q->second);

    s.root["machines"].append(q->second.root);
  }

  out << s.root;
  out << endl;
  out.close();
  return true;  
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlFileFormat(System &s)
{
  s.fileFormat.format = "ASPEN_MDL";
  s.fileFormat.version = "0.0.1";
  s.fileFormat.writtenBy = "AspenApi v"+API_VERSION;
  s.root["-fileformat"]["format"] = s.fileFormat.format;
  s.root["-fileformat"]["version"] = s.fileFormat.version;
  s.root["-fileformat"]["writtenby"] = s.fileFormat.writtenBy;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlChangelog(System &s)
{
  int id = 0;
  for(auto e : s.changeLog.entries) {
    string idx = ::to_string(id)+"_entry";
    string author = e.second.author;
    s.root["-changelog"][idx]["author"]  = e.second.author;
    s.root["-changelog"][idx]["comment"] = e.second.comment;
    s.root["-changelog"][idx]["date"]    = e.second.date;
    ++id;
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlUserData(System &s)
{
  s.root["userdata"] = Json::arrayValue;
  for(size_t i=0;i<s.userData.size();++i) {
    Json::Value v;
    v["name"]  = s.userData[i].name;
    v["value"] = s.userData[i].value;
    v["type"]  = s.userData[i].type;
    s.root["userdata"].append(v);
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlSystemInfo(System &s)
{
  s.root["system"]["-info"]["-name"] = s.name;
  s.root["system"]["-info"]["-version"] = s.version;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlSystemDefines(System &s)
{
  s.root["system"]["defines"] = Json::arrayValue;
  map<string,Nvt>::iterator q;
  for(q=s.defines.begin();q!=s.defines.end();++q) {
    Json::Value v;
    v["name"]  = q->first;
    v["value"] = q->second.value;
    v["type"]  = q->second.type;
    s.root["system"]["defines"].append(v);
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlSystemHierarchy(System &s)
{
  s.root["system"]["hierarchy"] = "";
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlSystemMemory(System &s)
{
  s.root["system"]["memory"] = "";
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlSystemPins(System &s)
{
  s.root["system"]["pins"] = Json::arrayValue;
  map<string,Pin>::iterator q;
  for(q=s.pins.begin();q!=s.pins.end();++q) {
    Json::Value v;
    v["name"]  = q->first;
    v["width"] = q->second.width;
    v["direction"] = q->second.direction;
    v["trigger"]  = q->second.trigger;
    s.root["system"]["pins"].append(v);
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlSystemFiles(System &s)
{
  s.root["system"]["files"] = Json::arrayValue;
  map<string,string>::iterator q;
  for(q=s.files.begin();q!=s.files.end();++q) {
    Json::Value v;
    v["name"]  = q->first;
    v["value"] = q->second;
    s.root["system"]["files"].append(v);
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlSystemDirectives(System &s)
{
  s.root["system"]["directives"] = "";
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlSystemRegisters(System &s)
{
  s.root["system"]["registers"] = Json::arrayValue;
  map<string,Register>::iterator q;

  for(q=s.registers.begin();q!=s.registers.end();++q) {

    Json::Value v;
    v["-name"] = q->first;
    v["desc"] = q->second.desc;

    v["address"] = itostr((uint32_t) q->second.address);
    v["width"]   = itostr(q->second.width);
    v["fields"]  = Json::arrayValue;

    for(size_t i=0;i<q->second.bits.size();++i) {
      Json::Value bv;
      bv["-name"]    = q->second.bits[i].name;
      bv["msb"]      = itostr(q->second.bits[i].msb);
      bv["lsb"]      = itostr(q->second.bits[i].lsb);
      bv["posMsb"]   = itostr(q->second.bits[i].posMsb);
      bv["posLsb"]   = itostr(q->second.bits[i].posLsb);
      bv["value"]    = itostr(q->second.bits[i].value);
      bv["strvalue"] = q->second.bits[i].strValue;
      bv["props"]    = itostr(q->second.bits[i].props);
      v["fields"].append(bv);
    }

    s.root["system"]["registers"].append(v);

  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlSystemInstances(System &s)
{
//  s.root["system"]["instances"] = "";
  map<string,string>::iterator q;
  for(q=s.instances.begin();q!=s.instances.end();++q) {
    s.root["system"]["instances"][q->first] = q->second;
  }
}
// =================================================================
// =================================================================
void AspenApi::wrMdlMachineInfo(Machine &m)
{
  m.root["-name"]     = m.name;
  m.root["-version"]  = m.version;
  m.root["-config"]   = m.config;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlMachineDefines(Machine &m)
{
  map<string,Nvt>::iterator q;
  for(q=m.defines.begin();q!=m.defines.end();++q) {
    Json::Value v;
    v["name"]  = q->first;
    v["value"] = q->second.value;
    v["type"]  = q->second.type;
    m.root["defines"].append(v);
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlMachineActions(Machine &m)
{
  map<string,Nvp>::iterator q;
  for(q=m.actions.begin();q!=m.actions.end();++q)
  {
    m.root["actions"][q->second.name] = q->second.value;
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlMachineEncodeRules(Machine &m)
{
  m.root["encoderules"] = Json::arrayValue;

  map<string,EncodeRule>::iterator q;
  for(q=m.encodeRules.begin();q!=m.encodeRules.end();++q)
  {

    Json::Value v;
    v["rulename"] = q->second.name;
    v["width"] = q->second.width;

    v["msb"] = q->second.range.first;
    v["lsb"] = q->second.range.second;

    map<string,string>::iterator p;
    for(p=q->second.mapping.begin();p!=q->second.mapping.end();++p) {
      v["map"][p->first] = p->second;
    }

    v["regfile"] = q->second.regFileName;

    m.root["encoderules"].append(v);

  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlMachineInstructions(Machine &m)
{
  m.root["instructions"] = Json::arrayValue;
  Json::Value v;

  map<string,Instruction>::iterator q;
  for(q=m.instructions.begin();q!=m.instructions.end();++q) {
    v["-mnemonic"] = q->second.mnemonic.name;
    v["prefix"] = q->second.mnemonic.prefix;
    v["suffix"] = q->second.mnemonic.suffix;

    v["action"] = q->second.action.name;
    v["-opc"]   = q->second.opcode.name;

    v["encoding"]["fields"] = q->second.encoding.fieldString;
    v["encoding"]["width"]  = q->second.encoding.width;

    v["shortdesc"]   = q->second.shortDesc;
    v["description"] = q->second.desc.richText;

    v["pseudocode"]  = Json::arrayValue;
    for(size_t i=0;i<q->second.pcode.lines.size();++i) {
      v["pseudocode"].append(q->second.pcode.lines[i]);
    }

    v["syntax"] = q->second.syntax.key;

    v["state"]["use"] = Json::arrayValue;
    for(size_t i=0;i<q->second.state.use.size();++i) {
      v["state"]["use"].append(q->second.state.use[i]);
    }

    v["state"]["def"] = Json::arrayValue;
    for(size_t i=0;i<q->second.state.def.size();++i) {
      v["state"]["def"].append(q->second.state.def[i]);
    }

    v["operands"] = Json::arrayValue;
    for(size_t i=0;i<q->second.operands.size();++i) {
      Json::Value jvOpr;
      jvOpr["name"] = q->second.operands[i].name;
      jvOpr["encoderule"] = q->second.operands[i].encodeRuleName;
      v["operands"].append(jvOpr);
    }

    v["funcunits"] = Json::arrayValue;
    for(size_t i=0;i<q->second.funcUnits.size();++i) {
      v["funcunits"].append(q->second.funcUnits[i]);
    }

    v["timing"]["latency"]     = to_string(q->second.timing.latency);
    v["timing"]["throughput"]  = to_string(q->second.timing.throughput);
    v["timing"]["branchdelay"] = to_string(q->second.timing.branchDelay);
    v["timing"]["loaduse"]     = to_string(q->second.timing.loadUse);

    v["notations"] = Json::arrayValue;
    for(size_t i=0;i<q->second.notations.size();++i) {
      Json::Value jvNote;
      jvNote["name"]  = q->second.notations[i].name;
      jvNote["value"] = q->second.notations[i].value;
      jvNote["type"] = q->second.notations[i].type;
      v["notations"].append(jvNote);
    }

    m.root["instructions"].append(v);
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlMachineInstructionWords(Machine &m)
{
  m.root["instructionwords"] = Json::arrayValue;

  for(size_t i=0;i<m.instructionWords.size();++i) {

    Json::Value v;
    v["name"] = m.instructionWords[i].name;
    v["fields"] = Json::arrayValue;
    for(size_t j=0;j<m.instructionWords[i].fields.size();++j) {
      v["fields"].append(m.instructionWords[i].fields[j].strValue);
    }

    m.root["instructionwords"].append(v);
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlMachineMemory(Machine &m)
{
  m.root["memory"] = Json::arrayValue;

  for(size_t i=0;i<m.memories.size();++i) {
    Json::Value v;
    v["name"]   = m.memories[i].name;
    v["type"]   = m.memories[i].type;
    v["access"] = m.memories[i].access;
    v["width"]  = m.memories[i].width;
    v["entries"]  = m.memories[i].entries;
    v["storedelay"] = m.memories[i].storeDelay;
    v["loaddelay"]  = m.memories[i].loadDelay;
    m.root["memory"].append(v);
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlMachinePins(Machine &m)
{
  m.root["pins"] = Json::arrayValue;

  for(size_t i=0;i<m.pins.size();++i) {
    Json::Value v;
    v["name"] = m.pins[i].name;
    v["width"] = m.pins[i].width;
    v["direction"] = m.pins[i].direction;
    v["trigger"]  = m.pins[i].trigger;
    m.root["pins"].append(v);
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlMachineOpcodes(Machine &m)
{
  map<string,Opcode>::iterator q;
  for(q=m.opcodes.begin();q!=m.opcodes.end();++q)
  {
    m.root["opcodes"][q->second.name] = q->second.binString;
  }

//  m.root["opcodes"] = Json::arrayValue;
//
//  map<string,Opcode>::iterator q;
//  for(q=m.opcodes.begin();q!=m.opcodes.end();++q)
//  {
//    Json::Value v;
//    v["name"]  = q->second.name;
//    v["value"] = q->second.binString;
//    m.root["opcodes"].append(v);
//  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlMachineSyntax(Machine &m)
{
  map<string,Syntax>::iterator q;
  for(q=m.syntax.begin();q!=m.syntax.end();++q)
  {
    m.root["syntax"][q->second.key] = q->second.shortHand;
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlMachinePipelines(Machine &m)
{
  m.root["pipelines"] = Json::arrayValue;

  for(size_t i=0;i<m.pipelines.size();++i) {
    Json::Value v;
    v["name"] = m.pipelines[i].name;
    v["graph"] = m.pipelines[i].graph;
    v["length"] = m.pipelines[i].length;
    m.root["pipelines"].append(v);
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlMachineProperties(Machine &m)
{
  m.root["properties"] = Json::arrayValue;

  for(size_t i=0;i<m.properties.size();++i) {
    Json::Value v;
    v["name"] = m.properties[i].name;
    v["value"] = m.properties[i].value;
    m.root["properties"].append(v);
  }

}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlMachineRegisterFiles(Machine &m)
{
  m.root["registerfiles"] = Json::arrayValue;

  for(size_t i=0;i<m.registerFiles.size();++i) {
    Json::Value v;
    v["name"] = m.registerFiles[i].name; 
    v["width"] = m.registerFiles[i].width; 
    v["type"] = m.registerFiles[i].type;

    string joined = "";
    for(size_t j=0;j<m.registerFiles[i].regs.size();++j) {
      string comma = j == 0 ? "" : ",";
      joined += comma + m.registerFiles[i].regs[j].name;
    }

    v["list"] = joined;
    m.root["registerfiles"].append(v); 
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::wrMdlMachineNamedBits(Machine &m)
{
  map<string,vector<Field> >::iterator q;
  Json::Value r;

  for(q=m.namedBits.begin();q!=m.namedBits.end();++q)
  {
    Json::Value s;
    for(size_t i=0;i<q->second.size();++i) {
      s[::to_string(q->second[i].msb)] = q->second[i].name;
    }

    r[q->first] = s;
  }
  m.root["namedbits"] = r;
}
}
