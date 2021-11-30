//
// Created by neo on 11/30/21.
//

#include "socketcan.h"
#include <bits/unistd.h>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

SocketCan::~SocketCan() {
  if (this->isOpen()) {
    close();
  }
}

void SocketCan::open(const std::string &interfaceName) {
  ifreq ifr{};
  sockaddr_can addr{};
  if ((sfd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW) < 0)) {
    throw std::runtime_error("Can't open socket");
  }
  strncpy(ifr.ifr_name, interfaceName.c_str(), IFNAMSIZ);
  if (ioctl(sfd_, SIOCGIFINDEX, &ifr) < 0) {
    throw std::runtime_error("Can't get interface index");
  }
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  if (bind(sfd_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    throw std::runtime_error("Can't bind socket");
  }
  std::cout << "Successfully open " << interfaceName << " at index "
            << ifr.ifr_ifindex << std::endl;
  pthread_attr_t canAttr;
  struct sched_param canSchedParam = {.sched_priority =
                                          sched_get_priority_max(SCHED_FIFO)};
  if (pthread_attr_init(&canAttr)) {
    throw std::runtime_error("Can't init thread attribute");
  }
  if (pthread_attr_setschedpolicy(&canAttr, SCHED_FIFO)) {
    throw std::runtime_error("Can't set thread policy");
  }
  if (pthread_attr_setinheritsched(&canAttr, PTHREAD_EXPLICIT_SCHED)) {
    throw std::runtime_error("Can't set thread inherit");
  }
  if (pthread_attr_setschedparam(&canAttr, &canSchedParam)) {
    throw std::runtime_error("Can't set thread priority");
  }

  if (pthread_create(&pRxThread_, &canAttr, &SocketCan::rxThread_, this)) {
    throw std::runtime_error("Can't create thread");
  }
}

bool SocketCan::isOpen() const { return sfd_ >= 0; }

[[noreturn]] void *SocketCan::rxThread_(void *argv) {
  auto *self = static_cast<SocketCan *>(argv);
  struct timeval timeout {};
  fd_set descriptors;
  struct can_frame rxFrame {};
  for (;;) {
    timeout.tv_sec = 1;
    FD_ZERO(&descriptors);
    FD_SET(self->sfd_, &descriptors);
    if (select(self->sfd_ + 1, &descriptors, nullptr, nullptr, &timeout)) {
      if (::read(self->sfd_, &rxFrame, sizeof(struct can_frame))) {
        if (self->rxCallback_ != nullptr) {
          self->rxCallback_(rxFrame);
        } else {
          std::lock_guard<std::mutex> lock(self->rxMutex_);
          self->rxQueue_.push(rxFrame);
        }
      } else {
        continue;
      }
    }
    pthread_testcancel();
  }
}

void SocketCan::bindRxCallback(
    std::function<void(struct can_frame &)> callback) {
  std::lock_guard<std::mutex> lock(rxMutex_);
  rxCallback_ = std::move(callback);
  rxQueue_ = std::queue<struct can_frame>();
}

void SocketCan::close() {
  if (isOpen()) {
    pthread_cancel(pRxThread_);
    pthread_join(pRxThread_, nullptr);
    ::close(sfd_);
    sfd_ = -1;
  }
}

const SocketCan *SocketCan::operator<<(struct can_frame &frame) {
  if (!isOpen()) {
    throw std::runtime_error("Can't send frame, socket is not open");
  }
  std::lock_guard<std::mutex> lock(txMutex_);
  if (::write(sfd_, &frame, sizeof(struct can_frame)) < 0) {
    throw std::runtime_error("Can't send frame");
  }
  return this;
}
const SocketCan *SocketCan::operator>>(struct can_frame &frame) {
  if (!isOpen()) {
    throw std::runtime_error("Can't receive frame, socket is not open");
  }
  if (rxCallback_ != nullptr) {
    throw std::runtime_error("Can't receive frame, callback is set");
  }
  std::lock_guard<std::mutex> lock(rxMutex_);
  if (rxQueue_.empty()) {
    memset(&frame,0,sizeof(can_frame));
  } else {
    frame = rxQueue_.front();
    rxQueue_.pop();
  }
  return this;
}