//
// Created by neo on 2021/12/8.
//

#include "spine.h"
#include <csignal>
#include <cstdlib>
#include <unistd.h>

Spine *pSpine = nullptr;
bool needStop = false;

void exitHandler() {
  system("cansend can0 003#FF.FF.FF.FF.FF.FF.FF.FD");
  system("ifconfig can0 down");
  exit(-1);
}

void termHandler(int signum) {
  (void) signum;
  exitHandler();
}

int main(int argc, char **argv) {
  try {
    Spine spine;

    atexit(exitHandler);
    signal(SIGTERM, termHandler);
    signal(SIGINT, termHandler);

    // ignore SIGPIPE
    //signal(SIGPIPE, SIG_IGN);
    // signal(SIGBUS, crashHandler);     // 总线错误
    //signal(SIGSEGV, crashHandler);    // SIGSEGV，非法内存访问
    // signal(SIGFPE, crashHandler);       //
    // SIGFPE，数学相关的异常，如被0除，浮点溢出，等等
    signal(SIGABRT, termHandler);     // SIGABRT，由调用abort函数产生，进程非正常退出
    while (!needStop) {
      spine.loop();
    }

  } catch (...) {
    throw;
  }
  return 0;
}