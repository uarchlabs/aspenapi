// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <memory>

struct AMsg
{
  AMsg(std::string _who="",int _verbose=3)
    : w(_who+": "), verbose(_verbose)
  {}

 ~AMsg() {}

  void setWho(std::string _w) {  w = _w+": "; }

  void mmsg(std::string p,std::string m)
    { std::cout<<p<<w<<m<<std::endl; }
  
  void dmsg(std::string m="",int v=4)
    { if(v<=verbose) mmsg("-D: ",m); }

  void emsg(std::string m="",int v=1)
    { if(v<=verbose) mmsg("-E: ",m); }

  void imsg(std::string m="",int v=3)
    { if(v<=verbose) mmsg("-I: ",m); }

  void wmsg(std::string m="",int v=2)
    { if(v<=verbose) mmsg("-W: ",m); }

  void mmsg(std::ostream& o,std::string p,std::string m)
    { o<<p<<w<<m<<std::endl; }

  // ----------------------------------------------------------------
  //dmsg should be v level 4
  void dmsg(std::ostream& o,std::string m="",int v=4)
    { (void)v; mmsg(o,"-D: ",m); }

  void emsg(std::ostream& o,std::string m="",int v=1)
    { (void)v; mmsg(o,"-E: ",m); }
  void imsg(std::ostream& o,std::string m="",int v=3)
    { (void)v; mmsg(o,"-I: ",m); }
  void wmsg(std::ostream& o,std::string m="",int v=2)
    { (void)v; mmsg(o,"-W: ",m); }

  void  msg(std::string m)
    { std::cout<<m<<std::endl;  }

  // ----------------------------------------------------------------
  std::string tq(std::string s) { return "'"+s+"'"; }
  std::string w;
  // ----------------------------------------------------------------
  // verbose 0 - silent
  //         1 - errors
  //         2 - errors,warnings
  //         3 - errors,warnings,info
  //         >= 4 - errors,warnings,info,debug4
  //              - debug messages can be a various levels, debugN
  // ----------------------------------------------------------------
  int  verbose{3};

 private:
  // ------------------------------------------------------------------- 
  //  AMsg(std::string _who="",int _verbose=3)
  //    : w(_who+": "),
  //      verbose(_verbose)
  //  {}
  // ------------------------------------------------------------------- 
//  Msg(const Msg&) = delete; //copy
//  Msg(Msg&&) = delete; //move
//  Msg& operator=(const Msg&) = delete; //assign

};
//#pragma once
//#include <string>
//
//struct Msg
//{
//  Msg()
//    : debug(false),
//      prefix(""),
//      quiet(false),
//      silent(false),
//      hideErrors(false)
//  {}
//
//  ~Msg() {}
//
//   void msg(std::string);
//   void msg(std::string,std::string);
//
//   void dmsg(std::string);
//   void emsg(std::string);
//   void imsg(std::string);
//   void wmsg(std::string);
//
//   bool debug;
//   std::string prefix;
//
//   bool quiet;  //no info msgs
//   bool silent; //no warn or info msgs
//   bool hideErrors;
//};
