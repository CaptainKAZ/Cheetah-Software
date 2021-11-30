//
// Created by neo on 11/30/21.
//
#pragma once
#include "socketcan.h"
#include "lcm/lcm.h"
class spine {
public:
  spine()=default;
private:
  SocketCan sc_[4];
};

