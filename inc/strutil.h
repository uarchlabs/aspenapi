#pragma once
// ------------------------------------------------------------------
// String helpers in standard C++17
// ------------------------------------------------------------------
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace aspen
{
// ------------------------------------------------------------------
// Split s on any character in delims. A run of delimiters counts as one
// separator. A leading or trailing delimiter run yields an empty first
// or last token, and an empty input yields one empty token.
// ------------------------------------------------------------------
inline std::vector<std::string> splitAnyOf(const std::string &s,
                                           const std::string &delims)
{
  std::vector<std::string> out;
  size_t start = 0;
  while(true) {
    size_t pos = s.find_first_of(delims,start);
    if(pos == std::string::npos) {
      out.push_back(s.substr(start));
      break;
    }
    out.push_back(s.substr(start,pos-start));
    start = s.find_first_not_of(delims,pos);
    if(start == std::string::npos) {
      out.push_back("");
      break;
    }
  }
  return out;
}
// ------------------------------------------------------------------
// Join parts with sep between them.
// ------------------------------------------------------------------
inline std::string joinStrings(const std::vector<std::string> &parts,
                               const std::string &sep)
{
  std::string out;
  for(size_t i=0;i<parts.size();++i) {
    if(i) out += sep;
    out += parts[i];
  }
  return out;
}
// ------------------------------------------------------------------
// Convert a decimal string to uint32_t. The whole string must be an
// optional '+' or '-' followed by one or more digits, with no spaces.
// A leading '-' wraps modulo 2^32, so "-1" gives 4294967295. Values
// above 4294967295 and malformed strings throw a std::exception.
// ------------------------------------------------------------------
inline uint32_t toU32(const std::string &s)
{
  size_t i = 0;
  bool neg = false;
  if(i < s.size() && (s[i] == '+' || s[i] == '-')) {
    neg = (s[i] == '-');
    ++i;
  }
  if(i == s.size()) throw std::invalid_argument("toU32: '"+s+"'");
  uint64_t v = 0;
  for(;i<s.size();++i) {
    char c = s[i];
    if(c < '0' || c > '9') throw std::invalid_argument("toU32: '"+s+"'");
    v = v*10 + static_cast<uint64_t>(c-'0');
    if(v > 0xFFFFFFFFull) throw std::out_of_range("toU32: '"+s+"'");
  }
  uint32_t u = static_cast<uint32_t>(v);
  return neg ? static_cast<uint32_t>(0u - u) : u;
}

} // namespace aspen
