#include "aspenapi.h"
#include "system.h"
#include "timing.h"
#include "opcode.h"
#include "nvx.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <typeinfo>
#include <algorithm>
#include <map>
#ifndef Q_MOC_RUN
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#endif
// -----------------------------------------------------------------
// -----------------------------------------------------------------
#define lexyi(s) boost::lexical_cast<uint32_t>(s)
// -----------------------------------------------------------------
// -----------------------------------------------------------------
using namespace std;
// -----------------------------------------------------------------
// -----------------------------------------------------------------
namespace aspen
{

bool AspenApi::tryStoi(string &opc,string &s,uint32_t i)
{
  try {
    i = stoi(s);
  } catch(std::exception&) {

    emsg("STOI failed for opcode '"+opc+"' for value '"+s+"'");
    return false;
  }
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::readMdl(System &s,string fileName)
{
  reportVersion();

  TR("+readMdl");
  ifstream in(fileName.c_str());
  if(!in.is_open()) {
    emsg("Could not open file : "+fileName);
    TR("-readMdl 0");
    return false;
  }

  s.clear();

  Json::Reader reader;
  bool parsingSuccessful = reader.parse( in, s.root );
  if ( !parsingSuccessful )
  {
    emsg("Failed to parse MDL");
    cout << reader.getFormattedErrorMessages();
    TR("-readMdl 1");
    return false;
  }

  if(!rdMdlFileFormat(s)) return false;
  if(!rdMdlChangeLog(s)) return false;
  if(!rdMdlUserData(s)) return false;

  if(!rdMdlSystemInfo(s)) return false;
  if(!rdMdlSystemDefines(s)) return false;
  if(!rdMdlSystemFiles(s)) return false;
  if(!rdMdlSystemHierarchy(s)) return false;
  if(!rdMdlSystemPins(s)) return false;
  if(!rdMdlSystemMemory(s)) return false;
  if(!rdMdlSystemDirectives(s)) return false;
  if(!rdMdlSystemRegisters(s)) return false;
  if(!rdMdlSystemInstances(s)) return false;

  Json::Value r = s.root["machines"];

  if(!r.isArray()) {
    emsg("Expected a JSON array for machine section.");
    TR("-readMdl 2");
    return false;
  }

  for(Json::Value::iterator q=r.begin();q!=r.end();++q) {

    Machine m;
    if(!rdMdlMachineInfo(m,q)) return false;
    if(!rdMdlMachineDefines(m,q)) return false;
    if(!rdMdlMachineActions(m,q)) return false;
    if(!rdMdlMachineEncodeRules(m,q)) return false;
    if(!rdMdlMachineInstructions(m,q)) return false;
    if(!rdMdlMachineInstructionWords(m,q)) return false;
    if(!rdMdlMachineMemory(m,q)) return false;
    if(!rdMdlMachinePins(m,q)) return false;
    if(!rdMdlMachineProperties(m,q)) return false;
    if(!rdMdlMachineOpcodes(m,q)) return false;
    if(!rdMdlMachineSyntax(m,q)) return false;
    if(!rdMdlMachinePipelines(m,q)) return false;
    if(!rdMdlMachineRegisterFiles(m,q)) return false;
    if(!rdMdlMachineNamedBits(m,q)) return false;

//    if(!rdMdlMachineCompSites(m,q)) return false;
//    if(!rdMdlMachineFunits(m,q)) return false;
//    if(!rdMdlMachineHierarchy(m,q)) return false;
//    if(!rdMdlMachineResources(m,q)) return false;

    s.machines.insert(make_pair(m.name,m));
  }

  //Every read of the mdl needs to have that ISA resolved/expanded so
  //it is done here rather than requiring a separate call
  bool ok = true;
  map<string,Machine>::iterator q;

  for(q=s.machines.begin();q!=s.machines.end();++q) {
    if(!resolveInstructionSet(s,q->second)) {
      emsg("Could not resolve ISA for "+q->first);
      ok = false;
    }
  }

  TR("-readMdl");
  return ok;  
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlFileFormat(System &s)
{
  TR("+rdMdlFileFormat");
  s.fileFormat.clear();
  s.fileFormat = FileFormat(s.root["-fileformat"]["format"].asString(),
                            s.root["-fileformat"]["version"].asString(),
                            s.root["-fileformat"]["writtenby"].asString());
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlChangeLog(System &s)
{
  TR("+rdMdlChangeLog");
  s.changeLog.clear();

  Json::Value r = s.root["-changelog"];

  if(r.isNull()) {
    wmsg("No change log detected for system: "+s.name);
    return true;
  } 

  int id = 0;
  string idx = ::to_string(id)+"_entry";
  Json::Value e = r[idx];

  while(!e.empty()) {

    ChangeLogEntry ce(id,
                      e["author"].asString(),
                      e["date"].asString(),
                      e["comment"].asString());

    s.changeLog.addEntry(ce);

    ++id; 
    idx = ::to_string(id)+"_entry";

    e = r[idx];
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlUserData(System &s)
{
  TR("+rdMdlUserData");
  Json::Value r = s.root["userdata"];

  if(r.isNull()) {
    wmsg("No change-log detected for system: "+s.name);
    return true;
  } else { 
    if(!r.isArray()) {
      emsg("Expected a JSON array for user data for system: "+s.name);
      return false;
    }
  }

  for(Json::Value::iterator q=r.begin();q!=r.end();++q) {

    Nvt nvt;
    nvt.name  = (*q).get("name","").asString();
    if(nvt.name.length() > 0) {
      nvt.value = (*q).get("value","").asString();
      nvt.type  = (*q).get("type","").asString();
      s.userData.push_back(nvt);
    }
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlSystemInfo(System &s)
{
  TR("+rdMdlSystemInfo");
  Json::Value r = s.root["system"]["-info"];

  if(r.isNull()) {
    wmsg("No system-info detected for system: "+s.name);
    return true;
  }

  s.name = r.get("-name","").asString();
  s.version = r.get("-version","").asString();

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlSystemDefines(System &s)
{
  TR("+rdMdlSystemDefines");
  Json::Value r = s.root["system"]["defines"];

  if(r.isNull()) {
    wmsg("No system-defines detected for system: "+s.name);
    return true;
  } else {
    if(!r.isArray()) {
      emsg("Expected a JSON array for defines section for system: "+s.name);
      return false;
    }
  }

  for(Json::Value::iterator q=r.begin();q!=r.end();++q) {

    Nvt nvt;
    nvt.name  = (*q).get("name","").asString();
    if(nvt.name.length() > 0) {
      nvt.value = (*q).get("value","").asString();
      nvt.type  = (*q).get("type","").asString();
      s.defines.insert(make_pair(nvt.name,nvt));
    }
  }
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlSystemFiles(System &s)
{
  TR("+rdMdlSystemFiles");
  Json::Value r = s.root["system"]["files"];

  if(r.isNull()) {
    wmsg("No system-files detected for system: "+s.name);
    return true;
  } 

  if(!r.isArray()) {
    wmsg("Expected a JSON array for system files");
  } else {
    for(Json::Value::iterator p=r.begin();p!=r.end();++p) {
      pair<string,string> pr;
      if(testJsObj(p,"name"))  pr.first = (*p).get("name","").asString();
      if(pr.first.length() > 0) {
        if(testJsObj(p,"value")) pr.second = (*p).get("value","").asString();
        s.files.insert(make_pair(pr.first,pr.second));
      } 
    }
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlSystemHierarchy(System &)
{
  TR("+rdMdlSystemHierarchy");
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlSystemPins(System &s)
{
  TR("+rdMdlSystemPins");

  Json::Value r = s.root["system"]["pins"];

  if(r.isNull()) {
    wmsg("No system-pins detected for system: "+s.name);
    return true;
  }

  if(!r.isArray()) {
    wmsg("Expected a JSON array for pins section for system: "+s.name);
  } else {

    for(Json::Value::iterator q=r.begin();q!=r.end();++q) {
      Pin p;
      if(   testJsObj(q,"name")      && testJsObj(q,"width")
         && testJsObj(q,"direction") && testJsObj(q,"trigger"))
      {
        p.name  = (*q).get("name","").asString();
        if(p.name.length() > 0) {
          p.width = lexyi((*q).get("width","").asString());
          p.direction = (*q).get("direction","").asString();
          p.trigger = (*q).get("trigger","").asString();
          s.pins.insert(make_pair(p.name,p));
        }
      }
    }

  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlSystemMemory(System &)
{
  TR("+rdMdlSystemMemory");
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlSystemDirectives(System &)
{
  TR("+rdMdlSystemDirectives");
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlSystemRegisters(System &s)
{
  TR("+rdMdlSystemRegisters");
  Json::Value root = s.root["system"]["registers"];

  if(root.isNull()) {
    wmsg("No instances found in system: "+s.name);
    return true;
  } else {

    for(Json::Value::iterator ri=root.begin();ri!=root.end();++ri) {

      Register reg;

      if(   testJsObj(ri,"-name")  && testJsObj(ri,"desc")
         && testJsObj(ri,"width") && testJsObj(ri,"address"))
      {

        reg.name  = (*ri).get("-name","").asString();

        if(reg.name.length() > 0) {
          reg.desc    =       (*ri).get("desc",   "").asString();
          reg.address = lexyi((*ri).get("address","").asString());
          reg.width   = lexyi((*ri).get("width",  "").asString());
        }

        if(testJsObj(ri,"fields")) {
          Json::Value field = (*ri).get("fields","");
          for(Json::Value::iterator fi=field.begin();fi!=field.end();++fi) {
            Field f;
            f.name     =       (*fi).get("-name",   "").asString();
            f.strValue =       (*fi).get("strvalue","").asString();
            f.msb      = lexyi((*fi).get("msb",     "").asString());
            f.lsb      = lexyi((*fi).get("lsb",     "").asString());
            f.posMsb   = lexyi((*fi).get("posMsb",  "").asString());
            f.posLsb   = lexyi((*fi).get("posLsb",  "").asString());
            f.value    = lexyi((*fi).get("value",   "").asString());
            f.props = (aspen::Field::FieldProperties)
                         lexyi((*fi).get("props","").asString());
            reg.bits.push_back(f);
          }
        }
      }
      s.registers.insert(make_pair(reg.name,reg));
    }
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlSystemInstances(System &s)
{
  TR("+rdMdlSystemInstances");
  Json::Value r = s.root["system"]["instances"];

  if(r.isNull()) {
    wmsg("No instances found in system: "+s.name);
    return true;
  } 

  vector<string> keys = r.getMemberNames();
  for(size_t i=0;i<keys.size();++i) {
    string value = r.get(keys[i],"").asString();
    s.instances.insert(make_pair(keys[i],value));
  }
 
  return true;
}
// =================================================================
// =================================================================
bool AspenApi::rdMdlMachineInfo(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachineInfo");
  m.name     = (*q).get("-name","").asString();

  if(m.name.length() == 0) {
    emsg("A name is required for all machine declarations");
    return false;
  }

  m.version  = (*q).get("-version","NONE").asString();
  m.config   = (*q).get("-config","NONE").asString();

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlMachineDefines(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachineDefines");
  Json::Value r = (*q)["defines"];

  if(r.isNull()) {
    wmsg("No machine-defines detected for machine: "+m.name);
    return true;
  } else {
    if(!r.isArray()) {
      emsg("Expected a JSON array for defines section for machine: "+m.name);
      return true;
    }
  }

  for(Json::Value::iterator q=r.begin();q!=r.end();++q) {

    Nvt nvt;
    nvt.name  = (*q).get("name","").asString();

    if(nvt.name.length() > 0) {
      nvt.value = (*q).get("value","").asString();
      nvt.type  = (*q).get("type","").asString();
      m.defines.insert(make_pair(nvt.name,nvt));
    }

  }
  TR("-rdMdlMachineDefines");
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlMachineActions(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachineActions");
  Json::Value r = (*q)["actions"];

  if(r.isNull()) {
    wmsg("No actions section found for machine: "+m.name);
    return true;
  }

  vector<string> keys = r.getMemberNames();
  for(size_t i=0;i<keys.size();++i) {

    string value = r.get(keys[i],"").asString();
    if(value.length() > 0) {
      Nvp act;
      act.name = keys[i];
      act.value = value;
      m.actions.insert(make_pair(act.name,act));
    }
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlMachineEncodeRules(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachineEncodeRules");
  Json::Value r = (*q)["encoderules"];

  if(r.isNull()) {
    wmsg("No encode rules found for machine: "+m.name);
    return true;
  } else {
    if(!r.isArray()) {
      emsg("Expected a JSON array for encode rules for machine: "+m.name);
      return false;
    }
  }

  for(Json::Value::iterator q=r.begin();q!=r.end();++q) {

    string ruleName = (*q).get("rulename","").asString();

    if(ruleName.length() == 0) {
      emsg("Unnamed encode rule detected");
      return false;
    }

    if(m.encodeRules.find(ruleName) != m.encodeRules.end()) {
      emsg("Duplicate encoding rules found : "+ruleName);
      return false;
    }

    Json::Value rmap = (*q)["map"];
//    if(!rmap.isNull() || () {

      std::map<std::string,std::string> mapping;
      vector<string> keys = rmap.getMemberNames();
      for(size_t i=0;i<keys.size();++i) {
        string value = rmap.get(keys[i],"").asString();
        if(value.length() > 0) {
          mapping.insert(make_pair(keys[i],value));
        }
      }

      string regFileName = (*q).get("regfile","").asString();
      if(regFileName.length() == 0) {
        emsg("Bad register file field in encode rule "+ruleName);
        emsg("Encode rules must specify a register file name, ");
        emsg("or one of the immediate flags, UIMM or SIMM");
        return false;
      }

      EncodeRule::eERULE_TYPE type;

           if(regFileName == "UIMM") type = EncodeRule::ERULE_UIMM;
      else if(regFileName == "SIMM") type = EncodeRule::ERULE_SIMM;
      else                           type = EncodeRule::ERULE_REG;

      uint32_t width = (*q).get("width",0).asUInt();
      if(width == 0) {
        emsg("Illegal zero width in encode rule "+ruleName);
        return false;
      }
  
      uint32_t msb = (*q).get("msb",width-1).asUInt();
      uint32_t lsb = (*q).get("lsb",0).asUInt();
  
      pair<uint32_t,uint32_t> range(msb,lsb);
  
      EncodeRule er(ruleName,type,width,range,mapping,regFileName);
  
      m.encodeRules.insert(make_pair(ruleName,er));
//    }
  }
    
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlMachineInstructions(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachineInstructions");
  Json::Value r = (*q)["instructions"];

  if(r.isNull()) {
    wmsg("No instructions detected for machine: "+m.name);
    return true;
  } else {
    if(!r.isArray()) {
      emsg("Expected a JSON array for instruction list for machine: "+m.name);
      return false;
    }
  }

  imsg("Processing instructions for "+m.name);

  uint32_t unspecId = 0;
  for(Json::Value::iterator q=r.begin();q!=r.end();++q) {
    Instruction ins;

    //Mnemonic
    ins.mnemonic.name = (*q).get("-mnemonic","").asString();
    if(ins.mnemonic.name.length() == 0) {
      wmsg("Unspecified mnemonic found, assigning unique mnemonic");
      ins.mnemonic.name = "UNSPEC_"+to_string(unspecId++);
    }
    ins.mnemonic.prefix = (*q).get("prefix","").asString();
    ins.mnemonic.suffix = (*q).get("suffix","").asString();

    //Opcode
    ins.opcode.name = (*q).get("-opc","").asString();
    if(ins.opcode.name.length() == 0) {
      wmsg("Unspecified opcode name found, assigning unique opcode name");
      ins.opcode.name = "UNSPEC_"+to_string(unspecId++);
    }

    if(0) imsg("Processing "+ins.mnemonic.name+" "+ins.opcode.name);

    size_t pos = ins.opcode.name.find_last_of("_");
    string digits = ins.opcode.name.substr(pos+1);
    uint32_t id=0;
    if(!tryStoi(ins.opcode.name,digits,id)) {
      emsg("Bad opcode ID detected, failed reading MDL");
      return false;
    } 

    //Action
    ins.action.name = (*q).get("action","").asString();
    if(ins.action.name.length() > 0) {
      m.actions.insert(make_pair(ins.action.name,ins.action));
    }

    //Encoding
    Json::Value jvEnc = (*q)["encoding"];

    Encoding enc;
    enc.fieldString = jvEnc.get("fields","").asString();

    if(enc.fieldString.length() == 0) {
      wmsg("For "+ins.opcode.name+" unspecified encoding found, "
           "assigning unique encoding ");
      enc.fieldString = "UNSPEC_"+to_string(unspecId++);
    }

    enc.width = jvEnc.get("width",0).asUInt();
    ins.encoding = enc;

    //Docs
    ins.shortDesc     = (*q).get("shortdesc","").asString();

    Json::Value jvPcode = (*q)["pseudocode"];
    for(Json::Value::iterator p=jvPcode.begin();p!=jvPcode.end();++p) {
      ins.pcode.lines.push_back((*p).asString());
    }

    ins.desc.richText = (*q).get("description","").asString();

    //Syntax
    ins.syntax.key = (*q).get("syntax","").asString();

    //State
    Json::Value use = (*q)["state"]["use"];
    for(Json::Value::iterator p=use.begin();p!=use.end();++p) {
      ins.state.use.push_back((*p).asString());
    }

    Json::Value def = (*q)["state"]["def"];
    for(Json::Value::iterator p=def.begin();p!=def.end();++p) {
      ins.state.def.push_back((*p).asString());
    }

    //F-units
    Json::Value units = (*q)["funcunits"];
    for(Json::Value::iterator p=units.begin();p!=units.end();++p) {
      ins.funcUnits.push_back((*p).asString());

//if(ins.opcode.name == "OPC_ADD_0") {
//cout<<"HERE func unit "<<(*p).asString()<<endl;
//}

    }

    //Timing
    //FIXME: these should be UInt()
    InsTiming tim;
    Json::Value jvTim  = (*q)["timing"];
    string latency     = jvTim.get("latency","").asString();
    string throughput  = jvTim.get("throughput","").asString();
    string branchDelay = jvTim.get("branchdelay","").asString();
    string loadUse     = jvTim.get("loaduse","").asString();

    if(latency.length() == 0) tim.latency = 0;
    else tim.latency = stoi(latency);

    if(throughput.length() == 0) tim.throughput = 0;
    else tim.throughput = stoi(throughput);

    if(branchDelay.length() == 0) tim.branchDelay = 0;
    else tim.branchDelay = stoi(branchDelay);

    if(loadUse.length() == 0) tim.loadUse = 0;
    else tim.loadUse = stoi(loadUse);

    ins.timing = tim;

    //Notations
    Nvt note; // n/v/t init to "" in ctor

    Json::Value jvNotes = (*q)["notations"];
    if(!jvNotes.isArray()) {
      wmsg("Expected a JSON array for notations: "+m.name);
    } else {
      for(Json::Value::iterator p=jvNotes.begin();p!=jvNotes.end();++p) {
        if(testJsObj(p,"name"))  note.name  = (*p).get("name","").asString();
        if(testJsObj(p,"value")) note.value = (*p).get("value","").asString();
        if(testJsObj(p,"type"))  note.type  = (*p).get("type","").asString();
      }
    }

    ins.notations.push_back(note);

    //Operands
    Json::Value jvOprs = (*q)["operands"];
    if(!jvOprs.isNull()) {
      if(!jvOprs.isArray()) {
        emsg("Expected a JSON array for operands of "+ins.opcode.name+
             " in machine: "+m.name);
        return false;
      }

      for(Json::Value::iterator p=jvOprs.begin();p!=jvOprs.end();++p) {

        Operand opr;
        //Name
        opr.name = (*p).get("name","").asString();
        if(opr.name.length() == 0) {
          wmsg("Ignoring operand with zero length name");
          wmsg("Opcode "+ins.opcode.name);
          continue;
        } 

        //E-rule name
        opr.encodeRuleName = (*p).get("encoderule","").asString();
        ins.operands.push_back(opr);

      }
    }

    m.instructions.insert(make_pair(ins.opcode.name,ins));
  }
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlMachineInstructionWords(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachineInstructionWords");
  Json::Value r = (*q)["instructionwords"];
  if(!r.isArray()) {
    emsg("Expected a JSON array for instruction words in machine: "+m.name);
    return false;
  }

  for(Json::Value::iterator q=r.begin();q!=r.end();++q) {

    InstructionWord iw;

    iw.name = (*q).get("name","").asString();

    if(iw.name.length() == 0) {
      wmsg("Ignoring instruction word format with zero length name for "
           +m.name);
      continue;
    }

    Json::Value f = (*q)["fields"];

    if(!f.isArray()) {
      emsg("Expected a JSON array for iword fields in machine: "
            +m.name);
      return false;
    }

    for(Json::Value::iterator p=f.begin();p!=f.end();++p) {

      Field field;
      field.props = Field::IWORD;

      field.strValue = (*p).asString();

      iw.fields.push_back(field);
    }

    m.instructionWords.push_back(iw); 
  }
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlMachineMemory(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachineMemory");
  Json::Value r = (*q)["memory"];

  if(!r.isArray()) {
    emsg("Expected a JSON array for memory in machine: "+m.name);
    return false;
  }

  for(Json::Value::iterator q=r.begin();q!=r.end();++q) {
    Memory mem;

    mem.name = (*q).get("name","").asString();
    if(mem.name.length() == 0) {
      wmsg("Ignoring zero length memory name for : "+m.name);
      continue;
    }

    mem.width  = (*q).get("width",0).asUInt();
    if(mem.width == 0) {
      emsg("Zero width memory detected, "+mem.name+" for : "+m.name);
      return false;
    }

    mem.type = (*q).get("type","data").asString();
    mem.access = (*q).get("access","rw").asString();
    mem.entries = (*q).get("entries",0).asUInt();
    mem.loadDelay = (*q).get("loaddelay",0).asUInt();
    mem.storeDelay = (*q).get("storedelay",0).asUInt();
  
    m.memories.push_back(mem); 
  }
 
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlMachineOpcodes(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachineOpcodes");

  Json::Value r = (*q)["opcodes"];

  if(r.isNull()) {
    wmsg("No opcodes section found for machine: "+m.name);
    return true;
  }

  vector<string> keys = r.getMemberNames();
  for(size_t i=0;i<keys.size();++i) {

    string value = r.get(keys[i],"").asString();
    if(value.length() > 0) {
      Opcode opc;
      opc.name = keys[i];
      opc.binString = value;

      pair<uint32_t,bool> width = getWidthFromBinString(opc.binString);
      if(!width.second) return false;

      pair<uint64_t,bool> value = getValueFromBinString(opc.binString);
      if(!value.second) return false;

      opc.width = width.second;
      opc.value = value.second;

      m.opcodes.insert(make_pair(opc.name,opc));
    }
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlMachineProperties(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachineProperties");
  Json::Value r = (*q)["properties"];

  if(!r.isArray()) {
    emsg("Expected a JSON array for properties in machine: "+m.name);
    return false;
  }

  for(Json::Value::iterator q=r.begin();q!=r.end();++q) {
    Nvp prop;
    prop.name = (*q).get("name","").asString();
    if(prop.name.length() == 0) {
      wmsg("Ignoring property with zero length name for "+m.name);
      continue;
    }

    prop.value = (*q).get("value","").asString();

    m.properties.push_back(prop);
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlMachinePins(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachinePins");
  Json::Value r = (*q)["pins"];

  if(!r.isArray()) {
    emsg("Expected a JSON array for pins in machine: "+m.name);
    return false;
  } 

  for(Json::Value::iterator q=r.begin();q!=r.end();++q) {
    Pin pin;
    pin.name = (*q).get("name","").asString();
    if(pin.name.length() == 0) {
      wmsg("Ignoring pin with zero length name for "+m.name);
      continue; 
    }

    pin.width = (*q).get("width",0).asUInt();
    if(pin.width == 0) {
      emsg("Detected pin with zero width, "+ pin.name+", for "+m.name);
      return false; 
    }

    pin.direction = (*q).get("direction","").asString();
    if(pin.direction.length() == 0) {
      emsg("Unspecified direction for pin, "+ pin.name+", for "+m.name);
      return false; 
    }

    pin.trigger = (*q).get("trigger","level").asString();
   
    m.pins.push_back(pin); 
  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlMachineSyntax(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachineSyntax");
  Json::Value r = (*q)["syntax"];

  if(r.isNull()) {
    wmsg("No syntax section found for machine: "+m.name);
    return true;
  } 

  vector<string> keys = r.getMemberNames();
  for(size_t i=0;i<keys.size();++i) {

    string value = r.get(keys[i],"").asString();
//    if(value.length() > 0) {
      Syntax syn;
      syn.key = keys[i];
      syn.shortHand = value;
      m.syntax.insert(make_pair(syn.key,syn));
//    }
  }
 
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlMachinePipelines(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachinePipeline");
  Json::Value r = (*q)["pipelines"];

  if(!r.isArray()) {
    emsg("Expected a JSON array for pipelines in machine: "+m.name);
    return false;
  }

  for(Json::Value::iterator q=r.begin();q!=r.end();++q) {

    Pipeline pipe;
    pipe.name = (*q).get("name","").asString();
    if(pipe.name.length() == 0) {
      wmsg("Ignoring pipeline with zero length name for "+m.name);
      continue;
    }

    pipe.length = (*q).get("length","").asInt();
    pipe.graph = (*q).get("graph","").asString();
    m.pipelines.push_back(pipe);

  }

  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlMachineRegisterFiles(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachineRegisterFiles");

  m.registerFiles.clear();

  Json::Value r = (*q)["registerfiles"];

  if(!r.isArray()) {
    emsg("Expected a JSON array for register files in machine: "+m.name);
    return false;
  }

  for(Json::Value::iterator q=r.begin();q!=r.end();++q) {

    RegisterFile rf;

    rf.name = (*q).get("name","").asString();

    if(rf.name.length() == 0) {
      wmsg("Ignoring register file with zero length name for "+m.name);
      continue;
    }

    rf.width = (*q).get("width",0).asUInt();
    if(rf.width == 0) {
      emsg("Register file with zero width, "+rf.name+", for "+m.name);
      return false;
    }

    rf.type = (*q).get("type","I").asString();

    string rlist = (*q).get("list","").asString(); 

    if(rlist.length() == 0) {
      emsg("Register file with no register list, "+rf.name+", for "+m.name);
      return false;
    }

    std::vector<std::string> splitVec;
    boost::split(splitVec,rlist,boost::is_any_of(","),
                 boost::token_compress_on);

    for(size_t i=0;i<splitVec.size();++i) {

      Register reg;
      reg.name = splitVec[i];

      Field field(reg.name,rf.width-1,0,0);
      reg.bits.push_back(field);

      rf.regs.push_back(reg);
    }

    m.registerFiles.push_back(rf);
  }

  TR("-rdMdlMachineRegisterFiles");
  return true;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
bool AspenApi::rdMdlMachineNamedBits(Machine &m,Json::Value::iterator q)
{
  TR("+rdMdlMachineNamedBits");
  Json::Value r = (*q)["namedbits"];

  if(r.isNull()) {
    wmsg("No named bits section found for machine: "+m.name);
    return true;
  } 

  vector<string> keys = r.getMemberNames();
  for(size_t i=0;i<keys.size();++i) {

    Json::Value elem = r[keys[i]];
    if(elem.isArray()) {
      emsg("Could not read named bits for register "+keys[i]);
      return false;
    }

    vector<string> bits = elem.getMemberNames();
    vector<Field> fvec;
    for(size_t j=0;j<bits.size();++j) {
      string bit = bits[j];
      string bitName = elem.get(bits[j],"").asString();
      Field f;
      f.name = bitName;
      f.msb  = stoi(bit);
      f.lsb  = stoi(bit);
      fvec.push_back(f);
    }
    m.namedBits.insert(make_pair(keys[i],fvec)); 
  }

  TR("-rdMdlMachineNamedBits");
  return true;
}

}
