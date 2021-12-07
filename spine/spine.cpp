//
// Created by neo on 11/30/21.
//

#include "spine.h"
#include <sys/time.h>

Spine::Spine() {
  sc[0].open("can0", 1e6);
  sc[1].open("can1", 1e6);
  sc[2].open("can2", 1e6);
  sc[3].open("can3", 1e6);
  leg[0].attachMotors(&sc[0], 1, 2, 3);
  leg[1].attachMotors(&sc[1], 1, 2, 3);
  leg[2].attachMotors(&sc[2], 1, 2, 3);
  leg[3].attachMotors(&sc[0], 1, 2, 3);
  if (!lcm_.good()) {
    throw std::runtime_error("LCM is not good");
  }
  spiCommandSub_ =
      lcm_.subscribe("spi_command", &Spine::spineCommandHandler, this);
}
void Spine::spineCommandHandler(const lcm::ReceiveBuffer *rxbuf,
                                const std::string &channel,
                                const spi_command_t *msg) {
  (void)rxbuf;
  (void)channel;
  gettimeofday(&rxTime_, nullptr);
  // RF LF RH LH
  for (int i = 0; i < 4; i++) {
    // TODO Separate Communication
    // TODO Long time no receive (controller dead) should stop
    leg[i].motor[0].controlRaw(msg->q_des_abad[i], msg->qd_des_abad[i],
                               msg->tau_abad_ff[i], msg->kp_abad[i],
                               msg->kd_abad[i]);
    leg[i].motor[1].controlRaw(msg->q_des_hip[i], msg->qd_des_hip[i],
                               msg->tau_hip_ff[i], msg->kp_hip[i],
                               msg->kd_hip[i]);
    leg[i].motor[2].controlRaw(msg->q_des_knee[i], msg->qd_des_knee[i],
                               msg->tau_knee_ff[i], msg->kp_knee[i],
                               msg->kd_knee[i]);
  }
}
void Spine::loop() {
  lcm_.handleTimeout(1);
  spi_data_t spi_data{};
  spi_torque_t spi_torque{};
  for (int i = 0; i < 4; i++) {
    spi_data.q_abad[i] = (float)leg[i].motor[0].q;
    spi_data.qd_abad[i] = (float)leg[i].motor[0].qd;
    spi_data.q_hip[i] = (float)leg[i].motor[1].q;
    spi_data.qd_hip[i] = (float)leg[i].motor[1].qd;
    spi_data.q_knee[i] = (float)leg[i].motor[2].q;
    spi_data.qd_knee[i] = (float)leg[i].motor[2].qd;
    spi_torque.tau_abad[i] = (float)leg[i].motor[0].tau;
    spi_torque.tau_hip[i] = (float)leg[i].motor[1].tau;
    spi_torque.tau_knee[i] = (float)leg[i].motor[2].tau;
  }
  lcm_.publish("spi_data", &spi_data);
  lcm_.publish("spi_torque", &spi_torque);
}