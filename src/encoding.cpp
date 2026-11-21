#include "field.h"
#include "encoding.h"
#include <vector>
#include <string>

using namespace std;
namespace aspen
{
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
Encoding::Encoding()
  : width(0),
    valid(false),
    fieldString(""),
    paddedOpcode(""),
    htmlTable(""),
    fields(),
    masks()
{
}
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
Encoding::Encoding(uint32_t _w,string _fs)
  : width(_w),
    valid(false),
    fieldString(_fs),
    paddedOpcode(""),
    htmlTable(""),
    fields(),
    masks()
{
//  fieldsFromStr(packString);
}
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
Encoding::Encoding(uint32_t _w,vector<Field>&_m)
  : width(_w),
    valid(false),
    fieldString(""),
    paddedOpcode(""),
    htmlTable(""),
    fields(_m),
    masks()
{
  verify();
}
// ---------------------------------------------------------------------
void Encoding::fieldsFromStr(string pack)
{
}
// ---------------------------------------------------------------------
void Encoding::verify()
{
  valid = false;
}
}

