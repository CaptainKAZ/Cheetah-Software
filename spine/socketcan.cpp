//
// Created by neo on 11/30/21.
//

#include "socketcan.h"
#include <bits/unistd.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

SocketCan::~SocketCan() {
  if (this->isOpen()) {
    close();
  }
}

void SocketCan::open(const std::string &interfaceName, const uint32_t bitrate) {
  ifreq ifr{};
  sockaddr_can addr{};
  FILE *fp;
  std::string command = "ip link set " + interfaceName + " type can bitrate " +
                        std::to_string(bitrate);
  std::string ret;
  char buf;
  fp = ::popen(command.c_str(), "r");
  if (fp == nullptr) {
    throw std::runtime_error("Error setting bitrate: popen failed");
  }
  while (EOF != (buf = (char)::fgetc(fp))) {
    ret += buf;
  }
  std::cout << "[Bitrate Setting]" << ret << std::endl;
  auto retVal = ::pclose(fp);
  if (retVal == 127) {
    throw std::runtime_error("Error setting bitrate: bad command:\n" + command);
  } else if (retVal == -1) {
    throw std::runtime_error(
        "Error setting bitrate: can't get child thread status");
  } else if (WIFEXITED(retVal)) {
    std::cout << "Bitrate setting exited, status=" << WEXITSTATUS(retVal)
              << std::endl;
  } else if (WIFSIGNALED(retVal)) {
    throw std::runtime_error("Bitrate setting killed by signal " +
                             std::to_string(WTERMSIG(retVal)));
  }
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
        if (!self->rxCallbacks_.empty()) {
          for (auto &callback : self->rxCallbacks_) {
            if (callback(rxFrame, self)) {
              break;
            }
          }
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
    const std::function<bool(struct can_frame &)> &callback) {
  std::lock_guard<std::mutex> lock(rxMutex_);
  rxCallbacks_.push_back(callback);
  rxQueue_ = std::queue<struct can_frame>();
}

void SocketCan::clearRxCallback() {
  std::lock_guard<std::mutex> lock(rxMutex_);
  rxCallbacks_.clear();
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
  if (!rxCallbacks_.empty()) {
    throw std::runtime_error("Can't receive frame, callback is set");
  }
  std::lock_guard<std::mutex> lock(rxMutex_);
  if (rxQueue_.empty()) {
    memset(&frame, 0, sizeof(can_frame));
  } else {
    frame = rxQueue_.front();
    rxQueue_.pop();
  }
  return this;
}
