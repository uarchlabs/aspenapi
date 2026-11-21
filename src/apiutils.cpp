// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#include "aspenapi.h"
#include "str.h"
#include "system.h"
#include <iostream>
#include <cstdlib>
#include <vector>
#include <utility>
#include <string>

using namespace std;
using namespace boost;
namespace aspen
{
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::testJsObj(Json::Value::iterator &p,std::string s)
{
  return (*p).isObject() && (*p).isMember(s);
}
// -----------------------------------------------------------------
// FIXME: does not actually comprehend multi-issue even though the
// ret is a vector, either mask off the iword slot before this call
// and therefore return a single value, or supply a mask and make
// multiple calls to disassembleStr() and return a vector
// -----------------------------------------------------------------
bool AspenApi::disassembleIword(Machine &m,dynamic_bitset<> &bits,
                                vector<InsDisInfo> &v)
{
  string s; to_string(bits,s);
  InsDisInfo info;
  bool ok = disassembleStr(m,s,info);
  v.push_back(info);
  return ok;
}
// -----------------------------------------------------------------
// FIXME: these should use the syntax definitions found in the MDL
// -----------------------------------------------------------------
string AspenApi::formatInsDisInfo(vector<InsDisInfo> &v)
  { return formatInsDisInfo(v,false); }
// -----------------------------------------------------------------
string AspenApi::formatInsDisInfo(vector<InsDisInfo> &v,bool showOpcName)
{
  string ret = "";
  for(InsDisInfo &inf : v) {
    ret  = showOpcName ? (inf.ins.opcode.name +" :: ") : "";
    ret += inf.ins.mnemonic.name+" ";
    for(size_t i=0;i<inf.values.size();++i) {
      string v = Str::rmvWS(inf.values[i]);
      ret += v + " ";
    }
  }
  return ret;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::disassembleStr(Machine &m,string &inputStr,InsDisInfo &info)
{
  map<string,Instruction>::iterator q;
  for(q=m.instructions.begin();q!=m.instructions.end();++q) {

    if(q->second.encoding.width != inputStr.length()) continue;

    string insOpcName = q->second.opcode.name;

    map<string,Opcode>::iterator p = m.opcodes.find(insOpcName);
    if(p==m.opcodes.end()) {
      emsg("Could not find ins opcode name in opcode map '"+insOpcName+"'");
      return false; 
    }

    string insOpcMask   = q->second.encoding.masks["opc"];
    string insMaskedOpc = q->second.encoding.paddedOpcode;

    string inputStrMasked;
    bool ok = getMaskedString(inputStr,insOpcMask,inputStrMasked);    
    if(!ok) return ok;

    if(inputStrMasked != insMaskedOpc) continue;
    
    ok =  getDisassemblyValues(m,q->second,inputStr,info);
    if(!ok) return ok;
  }

  if(info.ins.opcode.name.length() == 0) {
    emsg("Disassemble fall through for input '"+inputStr+"'");
    return false;
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::getDisassemblyValues(Machine &m,Instruction &ins,
                                          string &input,InsDisInfo &info)
{
  info.ins = ins;

  for(size_t i=0;i<ins.encoding.fields.size();++i) {

    Field f = ins.encoding.fields[i];
    if(f.name == "opc") continue;

    map<string,string>::iterator q = ins.encoding.masks.find(f.name);
    if(q == ins.encoding.masks.end()) {
      emsg("Inconsistent fields/masks entries in "+ins.opcode.name);
      return false;
    }

    string fieldMask = q->second;

    string maskedInput;
    bool ok = getMaskedString(input,fieldMask,maskedInput);
    if(!ok) return ok;

    uint32_t fieldWidth = f.posMsb-f.posLsb+1;

    string val = "";
    for(size_t j=0;j<fieldWidth;++j) val += "0";

    reverse(maskedInput.begin(),maskedInput.end());

    size_t idx = 0;    
    for(size_t j=f.posLsb;j<=f.posMsb;++j) {
      val[idx++] = maskedInput[j];
    }

    reverse(val.begin(),val.end());
    val = ::to_string(fieldWidth) + "'b" + val;

    Operand opr;
    for(size_t j=0;j<ins.operands.size();++j) {
      if(ins.operands[j].name != f.name) continue;
      opr = ins.operands[j];
      break;
    }

    if(opr.name.length() == 0) {
      emsg("Inconsistent operands/fields entries in "+ins.opcode.name);
      return false;
    }

    info.names.push_back(opr.name);
    string encRuleName = opr.encodeRuleName;

    map<string,EncodeRule>::iterator erI = m.encodeRules.find(encRuleName);

    if(erI == m.encodeRules.end()) {
      emsg("Inconsistent operands/encRule entries in "+ins.opcode.name);
      return false;
    }

    info.erules.push_back(encRuleName);

    if(   erI->second.type == EncodeRule::ERULE_UIMM
       || erI->second.type == EncodeRule::ERULE_SIMM)
    {
      string hex
        = getHexStrFromBinStr(val,erI->second.type == EncodeRule::ERULE_SIMM);
      info.values.push_back(hex);
    } else {
      map<string,string>::iterator mI;
      for(mI=erI->second.mapping.begin();mI!=erI->second.mapping.end();++mI) {
        if(mI->second == val) {
          info.values.push_back(mI->first);
          break;
        }
      }
    }
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::getMaskedString(string &input,string &mask,string &maskstr)
{
  maskstr = "";

  if(input.length() != mask.length()) {
    emsg("Mismatched lengths found in getMaskedString()");
    emsg(" input "+input+" length "+::to_string(input.length()));
    emsg(" mask  "+mask+" length "+::to_string(mask.length()));
    return false;
//    return s;
  }

  for(size_t i=0;i<mask.length();++i) {
    maskstr += '0';
    if(mask[i] == '1') maskstr[i] = input[i];
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::findMachine(System &sys,string &name,Machine *m)
{
  map<string,Machine>::iterator q = findMachine(sys,name);
  if(q!= sys.machines.end()) {
    currentMachine = q->second;
    m = &currentMachine;
    return true;
  }
  return false;
}
// -----------------------------------------------------------------
map<string,Machine>::iterator AspenApi::findMachine(System &sys,string &name)
{
  bool found = false;
  map<string,Machine>::iterator q;

  for(q=sys.machines.begin();q!=sys.machines.end();++q) {
    if(q->second.name == name) {
      found = true;
      break;
    }
  }

  if(found) return q;
  else return sys.machines.end();
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::goodBinString(std::string &s)
{
  size_t pos = s.find("'");
  if(pos == string::npos) {
    emsg("Malformed opcode value string: "+s);
    return false;
  }

  if(s[pos+1] != 'b' && s[pos+1] != 'B') {
    emsg("Incorrect data type in opcode string: "+s);
    return false;
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
std::string AspenApi::getHexStrFromBinStr(std::string &s,bool)
{
  uint32_t binWidth = getWidthFromBinString(s).first;
  string   binValue = getValueStrFromBinString(s).first;
  reverse(binValue.begin(),binValue.end());

  uint32_t v=0;
  for(size_t i=0;i<binWidth;++i) {
    v += binValue[i] == '1' ? (uint32_t(pow(2,i))) : 0; 
  }

  stringstream ss; ss<<"0x"<<hex<<v<<endl;
  return ss.str();
}
// -----------------------------------------------------------------
// FIXME: make a version that is getWidthOfOpcode(Opcode&)
// -----------------------------------------------------------------
pair<uint32_t,bool> AspenApi::getWidthFromBinString(std::string &s)
{
  pair<uint32_t,bool> p(0,false);

  if(!goodBinString(s)) {
    return p;
  }

  size_t pos = s.find("'");
  string sub = s.substr(0,pos);

  uint32_t value = (uint32_t) atoi(sub.c_str());
  p.first = value;
  p.second = true;
  return p;
}
// -----------------------------------------------------------------
// FIXME: make a version that is getValueOfOpcode(Opcode&)
// FIXME: make a version that accepts the base as an argument
// -----------------------------------------------------------------
pair<uint64_t,bool> AspenApi::getValueFromBinString(std::string &s)
{
  pair<uint64_t,bool> p(0,false);

  if(!goodBinString(s)) {
    return p;
  }

  pair<uint64_t,bool> wp = getWidthFromBinString(s);

  //this shouldnt happen due to goodBinString()
  if(!wp.second) {
    emsg("Could not extract width from bin string, '"+s+"'");
  }

  if(wp.first > 64) {
    emsg("Extended precision is required for bin string, '"+s+"'");
    return p;
  }
  
  size_t pos = s.find("'");
  string radix = s.substr(pos+1,1);
  string sub = s.substr(pos+2);

  char *ptr;
  uint64_t value = (uint64_t) strtoll(sub.c_str(), & ptr, 2);

  p.first  = value;
  p.second = true;
  return p;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
pair<string,bool> AspenApi::getValueStrFromBinString(std::string &s)
{
  pair<string,bool> p("",false);
  if(!goodBinString(s)) {
    return p;
  }

  size_t pos = s.find("'");
  string sub = s.substr(pos+2);

  p.first  = sub;
  p.second = true;
  return p;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
pair<string,bool>
AspenApi::getRegFileNameFromErule(Machine &m,string eRuleName,bool upper)
{
  string ern = eRuleName;
  transform(ern.begin(),ern.end(),ern.begin(),::toupper);

  pair<string,bool> p("",false);

  map<string,EncodeRule>::iterator q;
  q = m.encodeRules.find(ern);
  if(q == m.encodeRules.end()) return p;

  string rfName = q->second.regFileName;
  if(upper) transform(rfName.begin(),rfName.end(),rfName.begin(),::toupper);
   else     transform(rfName.begin(),rfName.end(),rfName.begin(),::tolower);

  p.first  = rfName;
  p.second = true;
  return p;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
uint32_t AspenApi::getMaxInstructionWidth(Machine &m)
{
  uint32_t maxWidth = 0;
  for(auto i : m.instructions) {
    if(i.second.encoding.width > maxWidth) maxWidth = i.second.encoding.width;
  }

  return maxWidth;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
string AspenApi::to_caps(string in)
{
  string n = in;
  for(size_t i=0;i<n.length();++i) {
    n[i] = toupper(n[i]);
  }
  return n;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
string AspenApi::padstr(string in,uint32_t length,string padchar)
{
  if(padchar.length() <=0 || in.length() >= length) return in;
  string p = string(1,padchar[0]);
  string n = in;
  while(n.length() < length) {
    n = n + p;
  }
  return n;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
string AspenApi::itostr(uint32_t i)
{
  stringstream ss; ss<<i;
  return ss.str();
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
uint32_t AspenApi::numBitsToEncode(uint32_t i)
{
  return (uint32_t) (ceil(log(i)/log(2)));
}
// --------------------------------------------------------------
// --------------------------------------------------------------
void AspenApi::sortByInsWidth(map<string,Instruction> &instructions,
                          map<uint32_t,vector<Instruction> > &sortedByWidth)
{
  map<uint32_t,vector<Instruction> >::iterator p;
  map<string,Instruction>::iterator q;
  for(q=instructions.begin();q!=instructions.end();++q) {
    Instruction ins = q->second;
    uint32_t width = ins.encoding.width;

    p = sortedByWidth.find(width);

    if(p == sortedByWidth.end()) {
      vector<Instruction> empty;
      empty.push_back(ins);
      sortedByWidth.insert(make_pair(q->second.encoding.width,empty));
    } else {
      p->second.push_back(ins);
    }
  }
}
}

