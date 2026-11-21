#include "msg.h"
#include "str.h"
#include <sstream>
#include <iostream>

Msg *Msg::instance = 0;
std::unique_ptr<Msg> msg(Msg::getInstance());

using namespace std;

int main()
{
  Str str_;
  int err = 0;

  stringstream ss;
  ss <<"                  \n";

  string line;
  getline(ss,line);

  if(!str_.isEmpty(line)) {
    cout<<"-E: str_ function isEmpty() failed"<<endl;
    ++err;
  }

  string s = "                 //---------------";
 
  s = str_.removeToEndOfLine("//",s);

  if(!str_.isEmpty(s)) {
    cout<<"-E: str_ function removeToEndOfLine() failed"<<endl;
    ++err;
  }
 
  cout<<"-I: Errors "<<err<<endl;

  return err;
}
