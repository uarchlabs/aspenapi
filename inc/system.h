#pragma once
#include "json/json.h"
#include "machine.h"
#include "fileformat.h"
#include "changelog.h"
#include "nvx.h"
#include "pin.h"
#include "memory.h"
#include "directive.h"
#include "register.h"
#include <vector>
#include <map>
#include <string>

namespace aspen
{

struct System
{
  System()
    : name(""),
      version(""),
      fileFormat(),
      changeLog(),
      userData(),
      defines(),
      files(),
      hierarchy(),
      pins(),
      memories(),
      directives(),
      registers(),
      machines(),
      instances(),
      root()
  {}

 ~System() {}

  void clear()
  { 
    name = "";
    version = "";
    fileFormat.clear();
    changeLog.clear();
    userData.clear();
    defines.clear();
    files.clear();
    hierarchy.clear();
    pins.clear();
    memories.clear();
    directives.clear();
    registers.clear();
    machines.clear();
    instances.clear();
    root.clear();
  }

  std::string name;
  std::string version;
  FileFormat  fileFormat;
  ChangeLog   changeLog;
  std::vector<Nvt> userData;
  std::map<std::string,Nvt> defines;
  std::map<std::string,std::string> files;
  //hierarchy is a placeholder, will be boost graph object
  std::vector<uint32_t> hierarchy;
  std::map<std::string,Pin> pins;
  std::map<std::string,Memory> memories;
  std::map<std::string,Directive> directives;
  std::map<std::string,Register> registers;
  std::map<std::string,Machine> machines;
  std::map<std::string,std::string> instances;

  Json::Value root;
};

}
