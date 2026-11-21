// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#include "aspenapi.h"
#include "system.h"
#include "changelog.h"
#include <sstream>
#include <string>
#include <iostream>
#include <iomanip>
using namespace std;
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool opcodeMaskTest(aspen::AspenApi&,aspen::Machine&);
bool showMasks(aspen::AspenApi&,aspen::Machine&);
bool testDisassembly(aspen::AspenApi&);
bool instructionInfo(aspen::AspenApi&);
bool dumpInstruction(aspen::AspenApi&);
void dumpDisInfo(aspen::InsDisInfo &info);
// ------------------------------------------------------------------------
string mdlFile = "test/mdl_11.json";
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
void report(aspen::AspenApi &api,bool ok,string s)
{
  if(!ok) {
    api.emsg(s+": FAIL");
    exit(1);
  } else {
    api.silent = false;
    api.imsg(s+": PASS");
    api.silent = true;
  }
}
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
int main(int ac, char** av) 
{
  ofstream log("RUN.LOG");

  if(!log.is_open()) {
    int ret = system("echo \"FAIL - OPEN FAILED\" > RUN.LOG");
    ret = 1;
    return ret;
  }
  log << "FAIL";
  log.close();

  aspen::AspenApi api;
  aspen::System sys;
  aspen::Machine m;

  api.silent     = true;
  api.showbanner = false;

  bool ok = api.readMdl(sys,"./test/vx.json");
  report(api,ok,"Test::Load test/vx.json MDL");

  ok = api.readMdl(sys,mdlFile);
  report(api,ok,"Test::Load MDL");

  ok = api.writeMdl(sys,"test/my-alt0.json");
  report(api,ok,"Test::Write MDL");

  ok = api.readMdl(sys,"test/my-alt0.json");
  report(api,ok,"Test::Re-read MDL");

  map<string,aspen::Machine>::iterator q;
  q = sys.machines.find("vx");
  ok = (q != sys.machines.end());
  report(api,ok,"Test::Find vx in system");

  m = q->second;
  ok = (m.opcodes.size() != 0);
  report(api,ok,"Test::Check vx opcodes");

  ok = opcodeMaskTest(api,m);
  report(api,ok,"Test::Opcode masks");

  ok = api.readMdl(sys,"test/atsobel.json");
  report(api,ok,"Test::Split operand support");

  ok = showMasks(api,m);
  report(api,ok,"Test::Show operand masks");

//Still needs work
if(0) {
  ok = testDisassembly(api);
  report(api,ok,"Test::Disassembly");
}

  ok = instructionInfo(api);
  report(api,ok,"Test::Instruction info dump");

  api.silent = false;
  api.imsg("AspenApi test: PASS");


  log.open("RUN.LOG",std::ofstream::out);
  if(!log.is_open()) {
    int ret = system("echo \"FAIL - OPEN FAILED\" > RUN.LOG");
    ret = 1;
    return ret;
  }
  log << "PASS";
  log.close();

  return 0;
}
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool dumpInstruction(aspen::AspenApi &api)
{
  aspen::System sys;
  aspen::Machine m;

  api.imsg("Test::Reading MDL : "+mdlFile);
  bool ok = api.readMdl(sys,mdlFile);

  if(!ok) {
    api.emsg("Test::Could not load MDL");
    return false;
  }

  map<string,aspen::Machine>::iterator q;
  q = sys.machines.find("vx");
  if(q == sys.machines.end()) {
    api.emsg("Could not find machine 'vx' in system");
    return false;
  }

  m = q->second;

//  api.resolveInstructionSet(sys,m);

  string opcodeName = "OPC_ABS_16b_0";

  map<string,aspen::Instruction>::iterator p;
  p = m.instructions.find(opcodeName);
  if(p == m.instructions.end()) {
    api.emsg("Could not find instruction "+opcodeName+" in vx");
    return false;
  }

  p->second.info("");
  return true;
}
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool opcodeMaskTest(aspen::AspenApi &api,aspen::Machine &m)
{
  struct OpcInfo {
    uint32_t insWidth;
    uint32_t opcLsb;
    uint32_t opcMsb;
    string   opcStr;
  };
  map<string,aspen::Instruction>::iterator q;
  for(q=m.instructions.begin();q!=m.instructions.end();++q) {

    map<uint32_t,OpcInfo> imap;

    for(size_t i=0;i<q->second.encoding.fields.size();++i) {
      aspen::Field field = q->second.encoding.fields[i];
      if(field.name == "opc") {
        OpcInfo opcInfo;

        opcInfo.insWidth = q->second.encoding.width;
        opcInfo.opcLsb = field.posLsb;
        opcInfo.opcMsb = field.posMsb;
        opcInfo.opcStr = field.strValue;
        imap.insert(make_pair(opcInfo.opcLsb,opcInfo));;
      }
    }

    map<uint32_t,OpcInfo>::iterator p;
    string binMask="";
    for(size_t j=0;j<q->second.encoding.width;++j) binMask += "0";
    bool atleastOne = false;
    for(p=imap.begin();p!=imap.end();++p) {
      for(size_t k=p->second.opcLsb;k<=p->second.opcMsb;++k) {
        binMask[k] = '1';
        atleastOne = true;
      }
    }

    reverse(binMask.begin(),binMask.end());

    if(!atleastOne) {
      api.emsg("Malformed encoding for opcode "+q->second.opcode.name);
      return false;
    }
  }
  return true;
}
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool showMasks(aspen::AspenApi &api,aspen::Machine &m)
{
  map<string,aspen::Instruction>::iterator q;
  for(q=m.instructions.begin();q!=m.instructions.end();++q) {

    stringstream ss;
    ss<<endl<<q->second.opcode.name<<" ";

    map<string,string>::iterator p;
    for(p=q->second.encoding.masks.begin();
        p!=q->second.encoding.masks.end();
        ++p)
    {
      ss<<" "<<p->first<<" "<<p->second;
    }
    ss<<endl;
    api.dmsg(ss.str());
  }
  return true;
}
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
void dumpDisInfo(aspen::InsDisInfo &info) {
  cout<<info.ins.opcode.name<<" :: "<<info.ins.mnemonic.name<<" ";
  for(size_t i=0;i<info.values.size();++i) {
    cout<<info.values[i]<<" ";
  } 
  cout<<endl;
}
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool testDisassembly(aspen::AspenApi &api)
{
  aspen::System sys;
  aspen::Machine mach;

  bool ok = api.readMdl(sys,mdlFile);
  report(api,ok,"Test:Disassembly load MDL");

  std::map<std::string,aspen::Machine>::iterator q;
  string name = "vx";
  q = api.findMachine(sys,name);

  ok = (q != sys.machines.end());
  report(api,ok,"Test:Disassembly find machine");

//  if(!api.resolveInstructionSet(sys,mach)) {
//    api.emsg("Could not resolve instruction set");
//    return false;
//  }

//  //OPC_SUB_16b_119  SUB R10 R12
//  //      8'b01110111
//  //  opc 1111111100000000
//  //  s1  0000000011110000
//  //  s2  0000000000001111
//  string OPC_SUB_16b_133  = "0111011110101100";
//  InsDisInfo dis = api.disassembleStr(mach,OPC_SUB_16b_133);
//  dumpDisInfo(dis);
//
//  //OPC_BEQ_16b_8  BEQ 0xEC
//  //      8'b00001000
//  //  opc 1111111100000000
//  //  s1  0000000011111111
//  string OPC_BEQ_16b_153  = "0000100011101100";
//  dis = api.disassembleStr(mach,OPC_BEQ_16b_153);
//  dumpDisInfo(dis);

  typedef boost::dynamic_bitset<> bitset_t;
  vector<bitset_t> bsv;

  bsv.push_back(bitset_t(string("0101011101110000"))); // MVKU R7,0x0
  bsv.push_back(bitset_t(string("0100000100110111"))); // LDW  R3,R7
  bsv.push_back(bitset_t(string("0000001001110100"))); // ADDU R7,0x4
  bsv.push_back(bitset_t(string("0100000101010111"))); // LDW  R5,R7
  bsv.push_back(bitset_t(string("0000001001110100"))); // ADDU R7,0x4
  bsv.push_back(bitset_t(string("0100000101100111"))); // LDW  R6,R7
  bsv.push_back(bitset_t(string("0101100111110110"))); // MV  R15,R6
  bsv.push_back(bitset_t(string("0000000100100000"))); // ADDU R2,R0
  bsv.push_back(bitset_t(string("0000000100100001"))); // ADDU R2,R1
  bsv.push_back(bitset_t(string("0101100100000001"))); // MV   R0,R1
  bsv.push_back(bitset_t(string("0101100100010010"))); // MV   R1,R2
  bsv.push_back(bitset_t(string("0111001100001111"))); // STW  R0,R15
  bsv.push_back(bitset_t(string("0000001011110100"))); // ADDU R15,0x4
  bsv.push_back(bitset_t(string("0000001001000001"))); // ADDU R4,0x1

  bsv.push_back(bitset_t(string("0001100101000011"))); // CMPU R4,R3

  bsv.push_back(bitset_t(string("0001001001010000"))); // BLT  R5,0
  bsv.push_back(bitset_t(string("0101101000000010")));  // NOP  2
  bsv.push_back(bitset_t(string("0100000100000110"))); // LDW  R0,R6
  bsv.push_back(bitset_t(string("0000001001100100"))); // ADDU R6,0x4
  bsv.push_back(bitset_t(string("0100000100010110"))); // LDW  R1,R6
  bsv.push_back(bitset_t(string("0000001001100100"))); // ADDU R6,0x4
  bsv.push_back(bitset_t(string("0100000100100110"))); // LDW  R2,R6
  bsv.push_back(bitset_t(string("0000001001100100"))); // ADDU R6,0x4
  bsv.push_back(bitset_t(string("0100000100110110"))); // LDW  R3,R6

  for(bitset_t &bs : bsv) {
//    string s;
//    boost::to_string(bs,s);
//    InsDisInfo dis = api.disassembleStr(mach,s);
//    dumpDisInfo(dis);

    vector<aspen::InsDisInfo> v;
    bool ok = api.disassembleIword(mach,bs,v);
    if(!ok) return ok;
    cout<<api.formatInsDisInfo(v,true)<<endl;
  }

  return true;
}
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool instructionInfo(aspen::AspenApi &api)
{
  aspen::System sys;
  aspen::Machine m;

  bool ok = api.readMdl(sys,mdlFile);
  report(api,ok,"Test:InstrInfo read MDL");

  map<string,aspen::Machine>::iterator p;
  p = sys.machines.find("vx");
  ok = (p != sys.machines.end());
  report(api,ok,"Test:InstrInfo find vx");

  m = p->second;

  aspen::Instruction ins = m.instructions["OPC_STW_16b_113"];
  ins.info("");

  return true;
}
