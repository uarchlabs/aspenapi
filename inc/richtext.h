#pragma once
#include <string>

namespace aspen
{

struct RichText
{
  RichText(std::string _s)
    : richText(_s)
  {}

  RichText() 
    : richText("")
  {} 

 ~RichText() {}

  //FIXME: for now
  std::string rich2PlainText() { return richText; }
  std::string richText;
};

}
