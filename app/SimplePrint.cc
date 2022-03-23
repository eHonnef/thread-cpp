/*
 * Main file and example on how to use Daemon object.
 */

#include <algorithm>
#include <iostream>
#include <random>
#include <stdlib.h>
#include <string>

#include <ThreadWrapper/Daemon.cc>

enum MSG { MSG_01, MSG_02, MSG_03 };

/**************
 * SimplePrint class.
 * Example of usage for the Daemon object.
 ***************/
class CSimplePrint : public CDaemon<std::string> {
protected:
  void Process(int nMessageID, const SData &Data) override {
    SleepNow(1000); // Let's pretend we are processing a lot of stuff so we desync the threads
    switch (MSG(nMessageID)) {
    case MSG_01:
      std::cout << "MSG_01: " << Data.Data.c_str()
                << "; Priority: " << std::to_string(Data.nPriority) << std::endl;
      break;

    case MSG_02:
      std::cout << "MSG_02: " << Data.Data.c_str()
                << "; Priority: " << std::to_string(Data.nPriority) << std::endl;
      break;

    case MSG_03:
      std::cout << "MSG_03: " << Data.Data.c_str()
                << "; Priority: " << std::to_string(Data.nPriority) << std::endl;
      break;
    }
  }

  void ProcessThreadEpilogue() override {
    // Example of overriding but still executing the inherited function
    std::cout << "--- Processing remaining queue ---" << std::endl;
    CDaemon::ProcessThreadEpilogue();
  }

  void ProcessAfterQueue() override {
    // Let's just print the time to dequeue a message.
    std::cout << "--- Time to dequeue a message: " << GetLastDelay() << " ms" << std::endl;
  }
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
  // Create and start the thread
  CSimplePrint sSP;
  sSP.Start();

  // Couple of control variables
  int nSentMessages = 0;
  int nTotal = 0;

  // Let's generate some random stuff (10 messages total)
  while (true) {
    nTotal++;
    CSimplePrint::SData d(rand() % 10, rand() % 3,
                          "ID=" + std::to_string(nTotal) + ": " + random_string(10));
    sSP.SafeAddMessage(d);
    nSentMessages += 1;

    if (nTotal == 10) {
      std::cout << "--- Exit ---" << std::endl;
      break;
    }
  }

  // Manually sent strings to another thread.
  // Write exit to ... well... exit.
  while (true) {
    std::cout << "--- Send something more to process ---" << std::endl;
    std::string strMsg;
    std::cin >> strMsg;

    if (strMsg == "exit")
      break;

    if (strMsg == "sleep") {
      sSP.Sleep(5000);
      continue;
    }

    CSimplePrint::SData d(rand() % 10, rand() % 3, strMsg);
    sSP.SafeAddMessage(d);
  }

  // If you want to wait the thread to finish, otherwise press enter
  std::cout << "--- Press Enter to finish ---" << std::endl;
  std::cin.ignore();
  std::cin.ignore();

  // Stopping thread and joining.
  sSP.Stop();

  return 0;
}
