//
// Created by neo on 11/30/21.
//
/**
 * @brief: a socketcan wrapper
 */
#pragma once
#include <linux/can.h>
#include <net/if.h>
#include <functional>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <string>



class SocketCan {
public:
  SocketCan() = default;
  ~SocketCan();
  void open(const std::string &interfaceName);
  bool isOpen() const;
  void bindRxCallback(std::function<void(struct can_frame &)> callback);
  void close();
  const SocketCan *operator>>(struct can_frame &frame) ;
  const SocketCan *operator<<(struct can_frame &frame) ;

private:
  int sfd_ = -1;
  std::function<void(struct can_frame &)> rxCallback_ = nullptr;
  [[noreturn]]static void* rxThread_(void *argv);
  std::queue<struct can_frame> rxQueue_;
  std::mutex rxMutex_;
  std::mutex txMutex_;
  pthread_t pRxThread_= 0;
};


