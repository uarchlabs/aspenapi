// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#include "aspenapi.h"
#include "system.h"
#include <string>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <set>
#include <map>
#include <vector>
#ifndef Q_MOC_RUN
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#endif
#define lexyui(s) boost::lexical_cast<uint32_t>(s)
// -----------------------------------------------------------------
//using namespace aspen;
using namespace std;
// -----------------------------------------------------------------
namespace aspen {

const string AspenApi::API_VERSION = "0.0.2";
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::resolveInstructionSet(System &s,Machine &m)
{
 //TR("+resolveInstructionSet");
//  if(1) imsg("Resolving ISA for "+m.name);

  map<string,Instruction>::iterator q;
  for(q=m.instructions.begin();q!=m.instructions.end();++q) {
    //Expand the syntax
    if(!expandSyntax(m,q->second)) return false;
    //Set value fields in the opcode class
    if(!updateOpcodeValue(m,q->second)) return false;
    //Update the encoding fields
    if(!updateEncoding(s,m,q->second)) return false;
    //use the results of updateEncoding() to create disassembly field masks
    //split out from updateEncoding() due to length
    if(!updateFieldMasks(m,q->second)) return false;
  }

  //discover and assign the instruction formats, tag each
  //instruction with it's format name and assign an ins id
  //Moved to the API because builder and aspengui both need
  //the info.
  if(!updateInstructionFormats(m)) return false;

  //sort the register vectors in each register file
  //split out from updateEncoding() due to length
  if(!updateRegisterFiles(m)) return false;

  //Populate the register fields that contain named bits
  if(!updateNamedBits(m)) return false;

  //fill out the widths of each register based on
  //the maximum extent of the msb in the bits section
  if(!updateRegisters(m)) return false;

  //Populate the imapping map which use the inverse indexing of
  //mapping and converts the bin strings to uint32_t
  //numerically
  if(!updateEncodeRules(m)) return false;

  if(!mdlCleanUp(m)) return false;

 //TR("-resolveInstructionSet");
  return true;
}
// -----------------------------------------------------------------
bool AspenApi::updateInstructionFormats(Machine &m)
{
  m.instructionFormats.clear();
  map<uint32_t,vector<aspen::Instruction> > sortedByWidth;
  sortByInsWidth(m.instructions,sortedByWidth);

  map<uint32_t,map<string,string> > unique; //<size,<field string,name> >

  string alpha = "ABCDEFGHIJKLMNOPQRSTWXYZ";

  map<uint32_t,vector<aspen::Instruction> >::iterator q;
  for(q=sortedByWidth.begin();q!=sortedByWidth.end();++q) {

    int wrapCount = 0;
    map<string,string> uniq;
    int idx = 0;
    for(size_t i=0;i<q->second.size();++i) {

      string fieldString = q->second[i].encoding.fieldString;
      if(uniq.find(fieldString) == uniq.end()) {

        int pos = idx%alpha.length();
        string formatName = ""; 
        formatName += alpha[pos];
        formatName += (wrapCount == 0) ? "" : ::to_string(wrapCount);
        formatName += "_" + ::to_string(q->first);
        uniq.insert(make_pair(fieldString,formatName));
        if(idx != 0 && pos==0) ++wrapCount;
        ++idx;
      }
    }
    m.instructionFormats.insert(make_pair(q->first,uniq));
  }

  uint32_t id=0;
  map<string,Instruction>::iterator p;
  map<uint32_t,map<string,string> >::iterator x;
  for(p=m.instructions.begin();p!=m.instructions.end();++p) {
    //Find the map for this ins' size
    uint32_t size = p->second.encoding.width;
    x = m.instructionFormats.find(size);
    if(x==m.instructionFormats.end()) {
      emsg("Could not find instruction '"+p->second.opcode.name+"' "
           "of width "+ ::to_string(size) + "in instruction formats");
      return false;
    }
    //find the format name for this ins' of this size
    string fieldString = p->second.encoding.fieldString;
    map<string,string>::iterator r = x->second.find(fieldString);

    if(r==x->second.end()) {
      emsg("Could not match field string '"+fieldString+"' "
           "for opcode '"+p->second.opcode.name);
      return false;
    }

    p->second.formatName = r->second;
    p->second.id = id;
    ++id; 
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::updateEncodeRules(Machine &m)
{
  map<string,EncodeRule>::iterator p;
  for(p=m.encodeRules.begin();p!=m.encodeRules.end();++p) {

    p->second.imapping.clear();

    map<string,string>::iterator q;
    for(q=p->second.mapping.begin();q!=p->second.mapping.end();++q) {

      pair<uint64_t,bool> idx = getValueFromBinString(q->second);

      if(!idx.second) {
        emsg("Could not convert mapping value for encode rule "
             +p->second.name+", value was "+q->second);
        return false;
      }

      p->second.imapping.insert(make_pair((uint32_t)idx.first,q->first));
    }
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::updateRegisters(Machine &m)
{
  //for each register file
  for(size_t i=0;i<m.registerFiles.size();++i) {
    //for each register in the file
    for(size_t j=0;j<m.registerFiles[i].regs.size();++j) {
      uint32_t msb=0;
      //for each field in the register in the file
      for(size_t k=0;k<m.registerFiles[i].regs[j].bits.size();++k) {
        if(m.registerFiles[i].regs[j].bits[k].msb > msb) {
          msb = m.registerFiles[i].regs[j].bits[k].msb;
        }
      }
      m.registerFiles[i].regs[j].width = msb+1;
    }
  }
  return true;
}
// -----------------------------------------------------------------
// remove unused opcodes and (eventually) other duplicated or 
// redundant information
// -----------------------------------------------------------------
bool AspenApi::updateRegisterFiles(Machine &m)
{
  if(1) imsg("Updating register files for "+m.name);

  for(size_t i=0;i<m.registerFiles.size();++i) {
    ::sort(m.registerFiles[i].regs.begin(),
           m.registerFiles[i].regs.end(),aspen::sort_register_names());
  }
  return true;
}
// -----------------------------------------------------------------
// remove unused opcodes and (eventually) other duplicated or 
// redundant information
// -----------------------------------------------------------------
bool AspenApi::mdlCleanUp(Machine &m)
{
  std::map<std::string,Opcode> newOpcodes;
  map<string,Instruction>::iterator q;
  map<string,Opcode>::iterator p;

  for(q=m.instructions.begin();q!=m.instructions.end();++q) {
    p = m.opcodes.find(q->second.opcode.name);
    if(p == m.opcodes.end()) {
      emsg("Inconsistent opcode map for opcode "+q->second.opcode.name);
      return false;
    }
    newOpcodes.insert(make_pair(p->first,p->second));
  }

  m.opcodes = newOpcodes; 
  return true; 
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::updateNamedBits(Machine &m)
{
  map<string,vector<Field> >::iterator q;
  map<string,Register>::iterator p;

  for(q=m.namedBits.begin();q!=m.namedBits.end();++q) {
    string regName = q->first;

    
    bool foundIt = false;
    for(size_t i=0;i<m.registerFiles.size();++i) {
      for(size_t j=0;j<m.registerFiles[i].regs.size();++j) {
        if(m.registerFiles[i].regs[j].name == regName) {
          foundIt = true;
          m.registerFiles[i].regs[j].bits.clear();
           m.registerFiles[i].regs[j].bits = q->second;
          break;
        }
      }
    }

    if(foundIt == false) {
      emsg("Could not find matching register '"+regName+" for named bit");
      return false;
    }

  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::updateFieldMasks(Machine &m,Instruction &ins)
{

  ins.encoding.masks.clear();

  string blankMask;
  for(size_t j=0;j<ins.encoding.width;++j) blankMask += "0";
  string paddedOpcode = blankMask;

  //Pad out the opcode
  for(size_t i=0;i<ins.encoding.fields.size();++i) {

    Field f = ins.encoding.fields[i];
    if(f.name != "opc") continue;

    map<string,Opcode>::iterator t = m.opcodes.find(ins.opcode.name);

    if(t == m.opcodes.end()) {
      emsg("Could not find opcode name to create padded opc");
      return false;
    }

    pair<string,bool> value = getValueStrFromBinString(t->second.binString);
    if(!value.second) {
      emsg("Could not get value string "
           "from binString in updateFieldMasks()");
      return false;
    } 

    string opcValue = value.first;
    reverse(opcValue.begin(),opcValue.end());

    //merge the field
    for(uint32_t j=f.posLsb;j<=f.posMsb;++j) {
      if(j < paddedOpcode.length()) {
        paddedOpcode[j] = opcValue[j];
      } else {
        emsg("In forming padded opcode the index exceeded the encoding "
             "length.");
        emsg("Opcode was '"+ins.opcode.name+"'");
        return false;
      }
    }
  }

  reverse(paddedOpcode.begin(),paddedOpcode.end());
  ins.encoding.paddedOpcode = paddedOpcode;

  //Form the mask for the operands
  map<string,string>::iterator p;
  map<string,string> operandMap;

  for(size_t i=0;i<ins.encoding.fields.size();++i) {

    Field f = ins.encoding.fields[i];
    if(f.name == "opc") continue;

    string operandMask = blankMask;

    for(uint32_t j=f.posLsb;j<=f.posMsb;++j) {
      if(j < operandMask.length()) {
        operandMask[j] = '1';
      } else {
        emsg("In forming operand mask the index exceeded the encoding "
             "length.");
        emsg("Opcode was '"+ins.opcode.name+"'");
        return false;
      }
    }

    p = operandMap.find(f.name);

    //if it already exists, merge it
    if(p != operandMap.end()) {
      for(size_t j=0;j<operandMask.length();++j) {
        if(operandMask[j] == '1') p->second[j] = '1';
      }
    } else {
      operandMap.insert(make_pair(f.name,operandMask));
    }

  }

  ins.encoding.masks.clear();
  for(p=operandMap.begin();p!=operandMap.end();++p) {
    string mask = p->second;
    reverse(mask.begin(),mask.end());
    ins.encoding.masks.insert(make_pair(p->first,mask));
  }

  return true;
}
//// -----------------------------------------------------------------
//// -----------------------------------------------------------------
//bool AspenApi::updateFieldMasks(Machine &m,Instruction &ins)
//{
//  set<string> names;
//  //get a unique set of field names
//  for(size_t i=0;i<ins.encoding.fields.size();++i) {
//    Field f = ins.encoding.fields[i];
//    names.insert(f.name);
//  }
//
//  struct OprInfo {
//    vector<uint32_t> oprLsb;
//    vector<uint32_t> oprMsb;
//  };
//
//  ins.encoding.masks.clear();
//
//  string blankMask;
//  for(size_t j=0;j<ins.encoding.width;++j) blankMask += "0";
//
//  map<string,OprInfo>::iterator p;
//  map<string,OprInfo> imap;
//
//  set<string>::iterator q;
//  for(q=names.begin();q!=names.end();++q) {
//
//    string searchName = *q;
//
//    for(size_t i=0;i<ins.encoding.fields.size();++i) {
//      Field f = ins.encoding.fields[i];
//
//      if(f.name == searchName) {
//
//        if(imap.find(searchName) != imap.end()) {
//          emsg("Split operand field detected, "
//               "this is not currently implemented");
//          emsg("Instruction      : "+ins.opcode.name);
//          emsg("Operand name was : "+searchName);
//          emsg("Field size was   : "+::to_string(ins.encoding.fields.size()));
//          for(size_t j=0;j<ins.encoding.fields.size();++j) {
//            Field g = ins.encoding.fields[j];
//            g.info("");
//          }
//
//          return false;
//        }
//
//        OprInfo oprInfo;
//        oprInfo.oprLsb.push_back(f.posLsb);
//        oprInfo.oprMsb.push_back(f.posMsb);
//        imap.insert(make_pair(searchName,oprInfo));
//
//        if(f.name == "opc") {
//          map<string,Opcode>::iterator t = m.opcodes.find(ins.opcode.name);
//          if(t == m.opcodes.end()) {
//            emsg("Could not find opcode name to create padded opc");
//            return false;
//          }
//          pair<string,bool> value
//            = getValueStrFromBinString(t->second.binString);
//          if(!value.second) {
//            emsg("Could not get value string "
//                 "from binString in updateFieldMasks()");
//            return false;
//          }
//
//          for(size_t j=0;j<oprInfo.oprLsb.size();++j) {
//            for(size_t i=0;i<oprInfo.oprLsb[j];++i) {
//              value.first += "0";          
//            }
//          }
//
//          for(size_t j=0;j<oprInfo.oprMsb.size();++j) {
//            for(size_t i=oprInfo.oprMsb[j]+1;i<ins.encoding.width;++i) {
//              value.first = "0" + value.first;
//            }
//          }
//          ins.encoding.paddedOpcode = value.first;
//        }
//      }
//    }
//
//    for(p=imap.begin();p!=imap.end();++p) {
//      string mask = blankMask;
//      for(size_t k=p->second.oprLsb[0];k<=p->second.oprMsb[0];++k) mask[k] = '1';
//      reverse(mask.begin(),mask.end());
//      ins.encoding.masks.insert(make_pair(p->first,mask));
//    }
//
//  }
//
//  return true;
//}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::updateEncoding(System &s,Machine &m,Instruction &ins)
{
  string opcodeName = ins.opcode.name;

// //TR("+updateEncoding "+opcodeName);

  map<string,Opcode>::iterator q = m.opcodes.find(opcodeName);

  if(q == m.opcodes.end()) {
    emsg("Opcode '"+opcodeName+"' not found");
    //TR("-updateEncoding 0");
    return false;
  }

  vector<string> splitVec;
  boost::split(splitVec,ins.encoding.fieldString,
               boost::is_any_of(","),
               boost::token_compress_on);

  uint32_t encPos = ins.encoding.width-1;
  for(size_t i=0;i<splitVec.size();++i) {
    string sv = splitVec[i];

    size_t pos1 = sv.find("[");
    size_t pos2 = sv.find("]");
    size_t pos3 = sv.find(":");

    if(   (pos1 != string::npos && pos2 == string::npos) 
       || (pos1 == string::npos && pos2 != string::npos))
    {
      emsg("Malformed field string '"+sv+"'");
      emsg("Opcode was "+ins.opcode.name);
      //TR("-updateEncoding 1");
      return false;
    }

    Field f;

    if(pos1 == string::npos && pos2 == string::npos) {

      map<string,Nvt>::iterator p = s.defines.find(sv);

      if(p == s.defines.end()) {

        emsg("Could not locate define '"+sv+"' in field string");
        emsg("Field string was '"+ins.encoding.fieldString+"'");
        emsg("Opcode was "+ins.opcode.name);
        //TR("-updateEncoding 2");
        return false;

      } else if(pos3 != string::npos) {

        emsg("Malformed define in field string '"+sv+"'");
        emsg("Unexpected separator ':'");
        emsg("Opcode was "+ins.opcode.name);
        //TR("-updateEncoding 3");
        return false;

      } else {

//FIXME - add support for defines
//        f = expandDefine(s,sv,p->second);
        f.strValue = p->first;
        ins.encoding.fields.push_back(f);
      }
      continue;
    }

    try {
      f.name = sv.substr(0,pos1);
    } catch(std::exception&) {
      emsg("Error processing field name in '"+sv+"'");
      emsg("Substring range was 0-"+::to_string(pos1));
      emsg("Opcode was "+ins.opcode.name);
      //TR("-updateEncoding 4");
      return false;
    }

    string msb = sv.substr(pos1+1,pos3-pos1-1);
    string lsb = sv.substr(pos3+1,pos2-pos3-1);
    uint32_t msbI,lsbI;

    try {
      msbI = stoi(msb);
      lsbI = stoi(lsb);
    } catch (std::exception&) {
      emsg("Could not determine range of field '"+f.name+"'");
      emsg("Opcode was "+ins.opcode.name);
      //TR("-updateEncoding 5");
      return false;
    }

    f.msb = msbI;
    f.lsb = lsbI;

    f.posMsb = encPos;
    encPos  -= f.msb-f.lsb;
    f.posLsb = encPos;
    --encPos;

    if(f.posMsb < 0 || f.posLsb < 0 
       || (f.posMsb == 0 && f.posLsb == 0))
    {
      emsg("Malformed field msb/lsb values for "+ins.opcode.name);
      //TR("-updateEncoding 6");
      return false;
    }

    //opc is a keyword in the field string
    //so are the numbered operands, s1, s2
    if(f.name == "opc") {

      f.props = Field::OPCODE;

      string binStr = q->second.binString;
      pair<string,bool> valStr = getValueStrFromBinString(binStr);

      if(valStr.second == false) {
        emsg("Could not determine value of opcode field in "+ins.opcode.name);
        //TR("-updateEncoding 7");
        return false;
      }

      
      size_t beg  = valStr.first.length()-f.msb-1;
      size_t size = f.msb-f.lsb+1;
      try {
        f.strValue  = valStr.first.substr(beg,size);
      } catch(std::exception&) {
        emsg("Failed to extract the value from substring '");
        emsg("Substring was '"+valStr.first+"'");
        if((f.msb > valStr.first.length())) {
          emsg("A negative field length was calculated");
          emsg("Substring length was '"+::to_string(valStr.first.length())+"'");
          emsg("MSB was '"+::to_string(f.msb)+"'");
        }
        emsg("Opcode was "+ins.opcode.name);
        return false;
      }

    } else { //Since operand fields do not have a fixed value the 
             //documentation repeats the operand name
      f.props = Field::OPERAND;
      f.strValue = f.name;
    }

    ins.encoding.fields.push_back(f);

  }

  //TR("-updateEncoding");
  return true;
}
// -----------------------------------------------------------------
bool AspenApi::updateOpcodeValue(Machine &m,Instruction &ins)
{
  //TR("+updateOpcodeValue "+ins.mnemonic.name);
  string opcodeName = ins.opcode.name;
  map<string,Opcode>::iterator q;

  q = m.opcodes.find(opcodeName);

  if(q == m.opcodes.end()) {
    emsg("Opcode '"+opcodeName+"' not found in dictionary");
    return false;
  }

  string binString = q->second.binString;

  if(!goodBinString(binString)) {
    emsg("Invalid opcode binString '"+binString+"'");
    emsg("Opcode : "+ins.opcode.name);
    //TR("-updateOpcodeValue 0");
    return false;
  }

  pair<uint32_t,bool> width = getWidthFromBinString(binString);
  if(!width.second) {
    emsg("Invalid width in binString '"+binString+"'");
    emsg("Opcode : "+ins.opcode.name);
    //TR("-updateOpcodeValue 2");
    return false;
  }

  q->second.width = width.first;

  pair<uint64_t,bool> value = getValueFromBinString(binString);
  if(!value.second) {
    emsg("Invalid value returned for binString '"+binString+"'");
    emsg("Opcode : "+ins.opcode.name);
    //TR("-updateOpcodeValue 3");
    return false;
  }

  q->second.value = value.first;

  ins.opcode = q->second;

  //TR("-updateOpcodeValue");
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::expandSyntax(Machine &m,Instruction &ins)
{
//bool debug = false;
//if(ins.opcode.name == "OPC_ADD_0") debug = true;

  string key = ins.syntax.key;
  //TR("+expandSyntax "+ins.mnemonic.name+" '"+key+"'");
  map<string,Syntax>::iterator q = m.syntax.find(key);

  if(q==m.syntax.end()) {
    emsg("Syntax expansion failed.");
    emsg("Syntax name "+key+" not found.");
    emsg("Opcode: "+ins.opcode.name);
    return false;
  }

  string textNames = q->second.shortHand;

  //Expand the mnemonic
  size_t pos1 = textNames.find("%M");
  if(pos1 != string::npos) {
    try {
      textNames.replace(pos1,2,ins.mnemonic.name);
    } catch(std::exception&) {
      emsg("Syntax expansion failed.");
      emsg("Could not find mnemonic anchor.");
      emsg("Opcode: "+ins.opcode.name);
      //TR("-expandSyntax x");
      return false;
    }
  }

  //Check for an explicit unit syntax
  bool explicitUnit;
  pos1 = textNames.find("%U ");

  if(pos1 != string::npos) {
    if(ins.funcUnits.size() == 0) {
      emsg("Syntax expansion failed.");
      emsg("Explicit unit specified but no unit assigned to the instruction");
      emsg("Opcode: "+ins.opcode.name);
      //TR("-expandSyntax x");
      return false;  
    }
    explicitUnit = true;
  } else {

    explicitUnit = false;

   //Remove any optional unit, lower case U means unit in syntax is optional
    pos1 = textNames.find("%u ");
    if(pos1 != string::npos) {
      try {
        textNames.erase(pos1,3);
      } catch(std::exception&) {
        emsg("Syntax expansion failed.");
        emsg("Optional unit  anchor processing failed.");
        //TR("-expandSyntax 1");
        return false;
      }
    }
  }

  //names will be in the form ADD (.U)  x1,  x2,  x3
  //types will be in the form ADD (.U) GPR, GPR, GPR
  string textTypes = textNames;

  if(explicitUnit) {
    pos1 = textNames.find("%U ");
    try {
      string units = "(" + boost::join(ins.funcUnits,",") + ")";
      textNames.replace(pos1,2,units);
      textTypes.replace(pos1,2,units);
    } catch(std::exception&) {
      emsg("Syntax expansion failed.");
      emsg("Could not replace unit anchor with unit list.");
      emsg("Opcode: "+ins.opcode.name);
      //TR("-expandSyntax 0");
      return false;
    }
  }

  //For all the remaining %xxx values replace them with a constructed
  //operand name
  size_t pos2;
  for(size_t i=0;i<ins.operands.size();++i) {

    pos1 = textNames.find("%");
    pos2 = textNames.find("%",pos1+1);

    if(pos1 == string::npos || pos2 == string::npos) {
      emsg("Mismatch syntax string and operand set");
      emsg("Syntax "+textNames);
      emsg("Types  "+textTypes);
      emsg("Opcode: "+ins.opcode.name);
      //TR("-expandSyntax 2");
      return false;
    }

    textNames.replace(pos1,pos2-pos1+1,ins.operands[i].name);

    pos1 = textTypes.find("%");
    pos2 = textTypes.find("%",pos1+1);

    if(pos1 == string::npos || pos2 == string::npos) {
      emsg("Mismatch syntax string and operand set");
      emsg("Syntax "+textTypes);
      emsg("Opcode: "+ins.opcode.name);
      //TR("-expandSyntax 3");
      return false;
    }

    string erName = ins.operands[i].encodeRuleName;
    transform(erName.begin(), erName.end(), erName.begin(), ::tolower);
    erName[0] = toupper(erName[0]);

    textTypes.replace(pos1,pos2-pos1+1,erName);
  }

  vector<string> namesvec;
  boost::split(namesvec,textNames,
               boost::is_any_of(" "),
               boost::token_compress_on);

  vector<string> typesvec;
  boost::split(typesvec,textTypes,
               boost::is_any_of(" "),
               boost::token_compress_on);

  if(namesvec.size() > 1) textNames = formSyntaxString(namesvec,explicitUnit);
  if(typesvec.size() > 1) textTypes = formSyntaxString(typesvec,explicitUnit);

  ins.syntax.expandedNames = textNames;
  ins.syntax.expandedTypes = textTypes;

//if(debug) {
//cout<<"HERE "<<textNames<<endl;
//cout<<"HERE "<<textTypes<<endl;
//}

 //TR("-expandSyntax");
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
string AspenApi::formSyntaxString(vector<string> &vec,bool explicitUnit)
{
  string out = "";

  for(size_t j=0;j<vec.size();++j) {
    bool no = (j==0) || (j==1) || (j==2 && explicitUnit);
    string comma = no ? "" : ",";
    out += comma+" "+vec[j];
  }

  return out;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::info(ApiClass c,Machine &m)
{
  map<string,Instruction>::iterator q;
  if(c == ApiClass::INSTRUCTIONS) {
    for(q=m.instructions.begin();q!=m.instructions.end();++q) {
      cout<<"-I:INFO: mnemonic "<<q->second.mnemonic.name<<endl;
      cout<<"-I:INFO: opc name "<<q->second.opcode.name<<endl;
      cout<<"-I:INFO: opcode   "<<hex<<q->second.opcode.value<<endl;
    }
    cout<<"-I:INFO: Total instructions "<<dec<<m.instructions.size()<<endl;
  }

  if(c == ApiClass::OPCNAMES_SHDESC) {
    for(q=m.instructions.begin();q!=m.instructions.end();++q) {
      cout<<"-I:INFO: OPCNAME/SHDESC "<<setw(16)<<q->second.opcode.name<<" ";
      cout<<q->second.shortDesc<<endl;
    }
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::standardizeOperandNames(Instruction &ins)
{
  uint32_t dcnt = 0;
  uint32_t ucnt = 0;
  std::string c;
  
  for(size_t i=0;i<ins.operands.size();++i) {
    if(!ins.operands[i].use && !ins.operands[i].def) {
      ins.operands[i].name = "x" + std::to_string(i);
      ++dcnt;
      ++ucnt;
    } else if(!ins.operands[i].use &&  ins.operands[i].def) {
      ins.operands[i].name = "d" + std::to_string(i);
      ++dcnt;
    } else if( ins.operands[i].use && !ins.operands[i].def) {
      ins.operands[i].name = "s" + std::to_string(i);
      ++ucnt;
    } else if( ins.operands[i].use &&  ins.operands[i].def) {
      ins.operands[i].name = "sd" + std::to_string(i);
      ++dcnt;
      ++ucnt;
    }
  }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void AspenApi::reportVersion()
{
  if(showbanner) {
    cout<<endl;
    cout<<"  AspenApi -- AspenTrail Machine Description API"<<endl;
    cout<<"  Version: "<<API_VERSION<<endl;
    cout<<"  Author : Jeff Nye, jeff.nye@intel.com"<<endl;
    cout<<endl;
  }
}

}
