// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once

namespace aspen
{

struct InsTiming
{
  InsTiming()
    : latency(0),
      throughput(0),
      loadUse(0),
      branchDelay(0)
  {}

  InsTiming(uint32_t _lat,uint32_t _thp,uint32_t _lu,uint32_t _bd)
    : latency(_lat),
      throughput(_thp),
      loadUse(_lu),
      branchDelay(_bd)
  {}

 ~InsTiming() {}

  uint32_t latency;
  uint32_t throughput;
  uint32_t loadUse;
  uint32_t branchDelay;
};

}
