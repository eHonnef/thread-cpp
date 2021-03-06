/*
 * Main file and example on how to use Daemon object.
 */

#include <algorithm>
#include <iostream>
#include <random>
#include <stdlib.h>
#include <string>

#include <ThreadWrapper/Daemon.cc>

#define UNUSED(x) (void)(x)

/*
 * Class to expose some private members.
 */
class CPriorityTest : public CDaemon<std::string> {
protected:
  void Process(int nMessageID, const SData &Data) override {
    // Do nothing
    UNUSED(nMessageID);
    UNUSED(Data);
  }

public:
  bool TryDequeue(SData &Data) { return CDaemon<std::string>::TryDequeue(Data); }
};

void EnqueueData(int nPriority, int nMsgID, CPriorityTest &PriorityTest) {
  CPriorityTest::SData Data(
      nPriority, 1, "Priority=" + std::to_string(nPriority) + "; MsgID=" + std::to_string(nMsgID));
  PriorityTest.SafeAddMessage(Data);
}

/*
 * Main program
 */
int main() {
  CPriorityTest objDaemon;

  // Let's enqueue some data:
  int nMsgID = 0;
  EnqueueData(20, nMsgID++, objDaemon);
  EnqueueData(40, nMsgID++, objDaemon);
  EnqueueData(4, nMsgID++, objDaemon);
  EnqueueData(3, nMsgID++, objDaemon);
  EnqueueData(0, nMsgID++, objDaemon);
  EnqueueData(10, nMsgID++, objDaemon);
  EnqueueData(1, nMsgID++, objDaemon);
  EnqueueData(0, nMsgID++, objDaemon);
  EnqueueData(5, nMsgID++, objDaemon);
  EnqueueData(50, nMsgID++, objDaemon);
  EnqueueData(50, nMsgID++, objDaemon);
  EnqueueData(1, nMsgID++, objDaemon);
  EnqueueData(1, nMsgID++, objDaemon);

  CPriorityTest::SData Data;
  while (objDaemon.TryDequeue(Data))
    std::cout << Data.Data.c_str() << std::endl;

  return 0;
}
