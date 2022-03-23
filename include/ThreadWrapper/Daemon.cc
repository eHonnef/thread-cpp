#ifndef DAEMON_NS_H
#define DAEMON_NS_H
#ifdef DAEMON_NS_H
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#endif

/*
 * Daemon class.
 * This is a wrapper for the std::thread object.
 * Specialize this class (check out SimplePrint.cc) and then override the process function.
 */
template <class T> class CDaemon {
public:
  /*
   * Data struct to hold the data and the info about on how to process this data (using nMessageID).
   */
  struct SData {
    int nPriority;  // Message priority
    int nMessageID; // Message id
    T Data;         // Message data
    std::chrono::time_point<std::chrono::high_resolution_clock> dtEnqueuedTime =
        std::chrono::high_resolution_clock::now(); // Message enqueue time

    SData(int p_nPriority, int p_nMessageID, T p_Data)
        : nPriority(p_nPriority), nMessageID(p_nMessageID), Data(p_Data) {}
  };

private:
  /*
   * Private class that provide the comparison function.
   * It'll order by ascending.
   */
  class CPriorityQueueComparison {
  public:
    CPriorityQueueComparison() {}
    bool operator()(const SData &lData, const SData &rData) {
      return lData.nPriority > rData.nPriority;
    }
  };

private:
  std::thread m_Thread; // Thread object.
  std::priority_queue<SData, std::vector<SData>, CPriorityQueueComparison>
      m_Queue;                             // Thread processing queue.
  std::atomic<bool> m_bIsRunning = false;  // Is this thread running?
  std::atomic<bool> m_bFinished = false;   // Did this thread finish the processing?
  std::atomic<int> m_nSleepMs = 0;         // How long this thread should sleep?
  std::atomic<bool> m_bIsSleeping = false; // Is this thread sleeping?
  std::atomic<double> m_fDelaySec; // How long took for the last message to be processed? In seconds
  std::condition_variable m_ConditionVar; // Conditional variable to notify the thread object when
                                          // there is something to process.
  mutable std::mutex m_Mutex;             // Mutex.

  /*
   * Registers the delay between enqueueing the message and the time to start processing it.
   * The delay is available at GetLastDelay
   * @see GetLastDelay
   */
  inline void RegisterDelayToProcess(const SData &Data) {
    // Calculate
    auto dtNow = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = dtNow - Data.dtEnqueuedTime;
    // Register
    m_fDelaySec.store(diff.count());
  }

protected:
  /*
   * Last message dequeue delay in seconds.
   */
  inline double GetLastDelay() const { return m_fDelaySec.load(); }

  /*
   * It'll make the calling thread to sleep nMs milliseconds.
   * Forward for the STL sleep function.
   * To correctly work in this wrapper you should call this in the context of the Thread object.
   * In other words, you should call this function somewhere inside the Execute function.
   * Preferable using (overriding) some of the virtual functions listed in the Execute function
   * comment.
   * @see Execute
   */
  inline void SleepNow(int nMs) { std::this_thread::sleep_for(std::chrono::milliseconds(nMs)); }

  /*
   * Safely dequeue a Data object so we can process it.
   * @return a Data object wrapped in the std::optional, if the queue is empty it'll return an empty
   *std::optional.
   */
  std::optional<SData> Dequeue() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Queue.empty())
      return {};
    auto rtn = m_Queue.top();
    m_Queue.pop();
    return rtn;
  }

  /*
   * Override this function to process your data inside the thread.
   * @param nMessageID The message ID, so you can control what/how to process a Data object.
   * @param data The Data object that was dequeued.
   * @see Data
   * @see dequeue()
   */
  virtual void Process(int nMessageID, const SData &Data) = 0;

  /*
   * Override this function to process something before entering the thread loop.
   * It'll be processed in the thread object context.
   */
  virtual void ProcessThreadPreamble() {}

  /*
   * Override this function to process something after the thread loop.
   * It'll be processed in the thread object context.
   * As default, we finish processing the thread's queue.
   */
  virtual void ProcessThreadEpilogue() {
    auto Data = Dequeue();
    while (Data.has_value()) {
      Process((*Data).nMessageID, (*Data));
      Data = Dequeue();
    }
  }

  /*
   * Override this function to process something before processing the queue data.
   * It'll be processed in the thread object context.
   */
  virtual void ProcessPreQueue() {}

  /*
   * Override this function to process something after processing the queue data.
   * It'll be processed in the thread object context.
   */
  virtual void ProcessAfterQueue() {}

private:
  /*
   * This is the function that the thread object will run.
   * There is no need to override/overload this function.
   * Checkout these functions:
   * @see Process
   * @see ProcessThreadPreamble
   * @see ProcessThreadEpilogue
   * @see ProcessPreQueue
   * @see ProcessAfterQueue
   */
  void Execute() {
    // Process something before entering the thread loop in this thread context.
    ProcessThreadPreamble();

    while (m_bIsRunning) {
      {
        // We wait in this context until there is something to process.
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_ConditionVar.wait(lock, [&] {
          // To process something one of those things should happen:
          // a) Queue is not empty;
          // b) We didn't call stop (to exit the loop);
          // c) The sleep function was called while this thread was idle;
          return !m_Queue.empty() || !m_bIsRunning.load() || m_nSleepMs.load() > 0;
        });
      }

      if (int nSleep = m_nSleepMs) {
        m_bIsSleeping = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(nSleep));
        m_bIsSleeping = false;
        m_nSleepMs = 0;
        continue;
      }

      // Process something before the queue
      ProcessPreQueue();

      // Process the queue
      std::optional<SData> Data = Dequeue();
      if (Data.has_value()) {
        Process((*Data).nMessageID, (*Data));
        RegisterDelayToProcess(*Data);
      }

      // Process something after the queue
      ProcessAfterQueue();
    }

    // Process something after exiting the thread loop in this thread context.
    ProcessThreadEpilogue();
    m_bFinished = true;
  }

public:
  /*
   * Constructor. The thread will start after calling Start
   * @see Start
   */
  CDaemon() = default;

  /*
   * Constructor.
   * @param bStartSuspended If set to false the thread will run after construction.
   */
  CDaemon(bool bStartSuspended) {
    if (!bStartSuspended)
      Start();
  }

  /*
   * Destructor.
   * We clear the queue.
   */
  virtual ~CDaemon() {
    if (m_bIsRunning)
      Stop(); // It'll call the join function
    else if (m_Thread.joinable())
      m_Thread.join(); // We wait for this thread to finish
  }

  /*
   * Starts the thread.
   */
  void Start() {
    if (not m_bIsRunning) {
      m_Thread = std::thread(&CDaemon<T>::Execute, this);
      m_bIsRunning = true;
    }
  }

  /*
   * Stops the thread execution.
   * It'll make the calling thread to wait this one.
   */
  void Stop() {
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      m_bIsRunning = false;
    }

    m_ConditionVar.notify_one();

    // We wait for this thread to finish processing
    if (m_Thread.joinable())
      m_Thread.join();
  }

  /*
   * Is this thread running?
   */
  inline bool IsRunning() const { return m_bIsRunning; }

  /*
   * Sleep function.
   * Suspend this thread for nMs if it isn't already suspended.
   * @param nMs sleep time in milliseconds.
   */
  void Sleep(int nMs) {
    if (not m_bIsSleeping) {
      m_nSleepMs = nMs;
      m_ConditionVar.notify_one();
    }
  }

  /*
   * Did this thread finish the processing?
   */
  bool Finished() const { return m_bFinished.load(); }

  /*
   * Enqueue a data object.
   * @param data The data object that'll be processed by this thread.
   * @see SData
   */
  void SafeAddMessage(const SData &Data) {
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      m_Queue.push(std::move(Data));
    }

    // Notify thread object that there is data to process
    m_ConditionVar.notify_one();
  }
};

#endif // DAEMON_NS_H
