// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include <stdint.h>
#include <iostream>
#include <utility>
#include <map>
#include <string>

namespace aspen
{
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct ChangeLogEntry
{
  ChangeLogEntry(uint32_t _id,std::string _author,std::string _date,
                 std::string _comment)
    : id(_id),
      author(_author),
      date(_date),
      comment(_comment)
  {}
  // ----------------------------------------------------------------
  ~ChangeLogEntry() {}
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  uint32_t id;
  std::string author;
  std::string date;
  std::string comment;
};
// ------------------------------------------------------------------
// ------------------------------------------------------------------
struct ChangeLog
{
  // ----------------------------------------------------------------
  ChangeLog()
    : entries()
  {}
  // ----------------------------------------------------------------
 ~ChangeLog() {}
  // ----------------------------------------------------------------
  void clear() { entries.clear(); }
  // ----------------------------------------------------------------
  bool addEntry(ChangeLogEntry &e)
  {
    std::map<uint32_t,ChangeLogEntry>::iterator q;
    q = entries.find(e.id);
    if(q!=entries.end()) {
      std::cout<<"-E:ChangeLog: duplicate entry detected, "
               <<"'"<<e.id<<"' "
               <<"'"<<e.author<<"' "
               <<"'"<<e.date<<"' "
               <<"'"<<e.comment<<"'"<<std::endl;
      return false;
    }
    entries.insert(std::make_pair(e.id,e));
    return true;
  }
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  std::map<uint32_t,ChangeLogEntry> entries;
};

}
