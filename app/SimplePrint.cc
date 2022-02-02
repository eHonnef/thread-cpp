/*
 * Main file and example on how to use Daemon object.
 */

#include <algorithm>
#include <iostream>
#include <random>
#include <stdlib.h>
#include <string>

#include <Daemon.cc>

enum MSG { MSG_01, MSG_02, MSG_03 };

/**************
 * SimplePrint class.
 * Example of usage for the Daemon object.
 ***************/
class SimplePrint : public Daemon<std::string> {
protected:
  void process(int nMessageID, const Data &data) override {
    switch (MSG(nMessageID)) {
    case MSG_01:
      sleep(500); // Let's pretend we are processing a lot of stuff so we desync the threads
      std::cout << "MSG_01: " << data.TData.c_str() << std::endl;
      break;

    case MSG_02:
      sleep(1000); // Let's pretend we are processing a lot of stuff so we desync the threads
      std::cout << "MSG_02: " << data.TData.c_str() << std::endl;
      break;

    case MSG_03:
      std::cout << "MSG_03: " << data.TData.c_str() << std::endl;
      break;
    }
  }

public:
  SimplePrint() : Daemon(0, true) {}
};

/*
 * Random string generator.
 */
std::string random_string(size_t length) {
  auto randchar = []() -> char {
    const char charset[] = "0123456789"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz";
    const size_t max_index = (sizeof(charset) - 1);
    return charset[rand() % max_index];
  };
  std::string str(length, 0);
  std::generate_n(str.begin(), length, randchar);
  return str;
}

/*
 * Main program, it'll enqueue random strings on the SimplePrint thread.
 */
int main() {
  SimplePrint sSP;

  sSP.start();
  sSP.detach(); // Running as a daemon

  int nSentMessages = 0;

  while (true) {
    // Let's generate some random stuff
    SimplePrint::Data d(rand() % 10, rand() % 3, random_string(10));
    sSP.safeAddMessage(d);
    nSentMessages += 1;

    // Send a burst of 5 messages and wait for 2000ms
    if (nSentMessages >= 5) {
      nSentMessages = 0;
      std::cout << "--- [Begin] Waiting for thread ---" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
      std::cout << "--- [End] Waiting for thread ---" << std::endl;
    }
  }

  return 0;
}
