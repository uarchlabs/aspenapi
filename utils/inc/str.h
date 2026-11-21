// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include <string>
#include <algorithm>
#include <cctype>
#include <functional>
#include <locale>
using namespace std; //FIXME

struct Str
{
   Str() {}
  ~Str() {}

//  std::string trimleft(std::string);
//  std::string trimright(std::string);
//  std::string trim(std::string);
//  std::string rmvWS(std::string);
//  std::string removeRedundant(std::string);
//  std::string removeToEndOfLine(std::string,std::string);
//  std::string replaceString(std::string,std::string,std::string);
//  std::string replaceRange(std::string,std::string,std::string,std::string);
//
  static bool spaces(char lhs, char rhs)
    { return (lhs == rhs) && (lhs == ' '); }

  static bool isEmpty(std::string s)
  {
    if(s.length() == 0) return true;
    return s.find_first_not_of("\t\n ") == std::string::npos;
  }

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
static string trimleft(string s)
{
  string r = s;
  r.erase(r.begin(), find_if(r.begin(), r.end(),
          not1(ptr_fun<int, int>(isspace))));
  return r;
}
// ------------------------------------------------------------------------
static string trimright(string s)
{
  string r = s;
  r.erase(find_if(r.rbegin(), r.rend(),
          not1(ptr_fun<int,int>(isspace))).base(), r.end());
  return r;
}
// ------------------------------------------------------------------------
static string trim(string s)
  { return trimleft(trimright(s)); }
// ------------------------------------------------------------------------
static string rmvWS(string s)
{
  string r = s;
  r = removeRedundant(r);
  r = trim(r);
  return r;
}
// ------------------------------------------------------------------------
static string removeRedundant(string s)
{
  string r = s;
  string::iterator end = unique(r.begin(),r.end(),spaces);
  r.erase(end, r.end());
  return r;
}
// ------------------------------------------------------------------------
static string removeToEndOfLine(string pattern,string s)
{
  string r = s;
  size_t pos = r.find(pattern);
  if(pos == string::npos) return r;

  r = r.erase(pos);
  return r;
}
// ------------------------------------------------------------------------
static string replaceRange(string s,string start,string end,string rpl)
{
  string r = s;
  size_t pos1 = r.find(start);
  if(pos1 == string::npos) return r;
  size_t pos2 = r.find(end);
  if(pos2 == string::npos) return r;

  r = r.replace(pos1,pos2-pos1+1,rpl);
  return r;
}
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
static string replaceString(string input,string tag,string repl)
{
  if(input.length() == 0) return input;
  if(tag.length() == 0)   return input;

  size_t tlen = tag.length();

  string target = input;
  size_t pos = target.find(tag);
  while(pos != string::npos) {
    target.replace(pos,tlen,repl);
    pos = target.find(tag);
  }
  return target;
}

static string toLower(string input)
{
  string ret = input;
  std::transform(ret.begin(), ret.end(), ret.begin(),
    [](unsigned char c){ return std::tolower(c); });
  return ret;
}

static string toUpper(string input)
{
  string ret = input;
  std::transform(ret.begin(), ret.end(), ret.begin(),
    [](unsigned char c){ return std::toupper(c); });
  return ret;
}


};
