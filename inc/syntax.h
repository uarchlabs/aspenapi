#pragma once
#include <string>
#include <iostream>

namespace aspen
{

struct Syntax
{
  // -----------------------------------------------------------------
  // -----------------------------------------------------------------
  Syntax(std::string _k="",std::string _sh="",
         std::string _et="",std::string _en="")
    : key(_k),
      shortHand(_sh),
      expandedTypes(_et),
      expandedNames(_en)
  {}

  // -----------------------------------------------------------------
  // -----------------------------------------------------------------
 ~Syntax() {}

  void info() {
    std::cout<<"key:"<<key
             <<" short:"<<shortHand
             <<" expType:"<<expandedTypes
             <<" expNames:"<<expandedNames
             <<std::endl;
  }
  // -----------------------------------------------------------------
  // -----------------------------------------------------------------
  std::string key;       //use to look up short hand in dictionary
  std::string shortHand; //only needed by dictionary entries

  std::string expandedTypes;  //only needed by instruction entries
  std::string expandedNames;  //only needed by instruction entries
};

}
