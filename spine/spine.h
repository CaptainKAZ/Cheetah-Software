//
// Created by neo on 11/30/21.
//
#pragma once
#include "../lcm-types/cpp/spi_command_t.hpp"
#include "../lcm-types/cpp/spi_data_t.hpp"
#include "../lcm-types/cpp/spi_torque_t.hpp"
#include "lcm/lcm-cpp.hpp"
#include "motor.h"
#include "socketcan.h"

class Leg {
public:
  AK10_9Motor motor[3]; // abad hip knee

  void attachMotors(SocketCan *sc, uint8_t abadId, uint8_t hipId,
                    uint8_t kneeId) {
    sc_ = sc;
    motor[0].attach(abadId, sc_);
    motor[1].attach(hipId, sc_);
    motor[2].attach(kneeId, sc_);
  }

  void control() {
    motor[0].control();
    motor[1].control();
    motor[2].control();
  }

private:
  SocketCan *sc_ = nullptr;
};

class Spine {
public:
  Spine();
  ~Spine() {
    // for safety
    for (int i = 0; i < 3; i++) {
      for (auto &l : leg) {
        l.motor[0].stop();
        l.motor[1].stop();
        l.motor[2].stop();
      }
    }
  }
  Leg leg[4];
  void spineCommandHandler(const lcm::ReceiveBuffer *rxbuf,
                           const std::string &channel,
                           const spi_command_t *msg);
  void loop();
  SocketCan sc[4]; // RF LF RH LH

private:
  lcm::LCM lcm_;
  lcm::Subscription *spiCommandSub_;
  timeval rxTime_{};
};
