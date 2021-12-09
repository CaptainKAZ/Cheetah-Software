//
// Created by neo on 2021/12/8.
//

#include "spine.h"
#include <csignal>
#include <cstdlib>
#include <unistd.h>

Spine *pSpine = nullptr;
bool needStop = false;

void termHandler(int signum) {
  (void)signum;
  try {
    for (auto &i : pSpine->leg) {
      i.motor[0].stop();
      i.motor[1].stop();
      i.motor[2].stop();
    }
    needStop = true;
    sleep(1);
    delete pSpine;
  } catch (...) {
    exit(-1);
  }
}

void exitHandler() {
  try {
    for (auto &i : pSpine->leg) {
      i.motor[0].stop();
      i.motor[1].stop();
      i.motor[2].stop();
    }
    needStop = true;
    delete pSpine;
  } catch (...) {
    exit(-1);
  }
}

int main(int argc, char **argv) {
  try {
    pSpine = new Spine();
  } catch (...) {
    throw;
  }

  atexit(exitHandler);
  signal(SIGTERM, termHandler);
  signal(SIGINT, termHandler);

  // ignore SIGPIPE
  signal(SIGPIPE, SIG_IGN);
  // signal(SIGBUS, crashHandler);     // 总线错误
  // signal(SIGSEGV, crashHandler);    // SIGSEGV，非法内存访问
  // signal(SIGFPE, crashHandler);       //
  // SIGFPE，数学相关的异常，如被0除，浮点溢出，等等 signal(SIGABRT,
  // crashHandler);     // SIGABRT，由调用abort函数产生，进程非正常退出
  while (!needStop) {
    pSpine->loop();
  }
  return 0;
}