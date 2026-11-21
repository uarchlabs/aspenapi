#include "str.h"
#include <algorithm>
#include <cctype>
#include <functional>
#include <locale>
#include <string>

using namespace std;
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
string Str::trimleft(string s)
{
  string r = s;
  r.erase(r.begin(), find_if(r.begin(), r.end(),
          not1(ptr_fun<int, int>(isspace))));
  return r;
}
// ------------------------------------------------------------------------
string Str::trimright(string s)
{
  string r = s;
  r.erase(find_if(r.rbegin(), r.rend(),
          not1(ptr_fun<int,int>(isspace))).base(), r.end());
  return r;
}
// ------------------------------------------------------------------------
string Str::trim(string s)
  { return trimleft(trimright(s)); }
// ------------------------------------------------------------------------
string Str::rmvWS(string s)
{
  string r = s;
  r = removeRedundant(r);
  r = trim(r);
  return r;
}
// ------------------------------------------------------------------------
string Str::removeRedundant(string s)
{
  string r = s;
  string::iterator end = unique(r.begin(),r.end(),spaces);
  r.erase(end, r.end());
  return r;
}
// ------------------------------------------------------------------------
string Str::removeToEndOfLine(string pattern,string s)
{
  string r = s;
  size_t pos = r.find(pattern);
  if(pos == string::npos) return r;

  r = r.erase(pos);
  return r;
}
// ------------------------------------------------------------------------
string Str::replaceRange(string s,string start,string end,string rpl)
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
string Str::replaceString(string input,string tag,string repl)
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
