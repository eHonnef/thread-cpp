#ifndef DAEMON_NS_H
#define DAEMON_NS_H
#ifdef DAEMON_NS_H
#include <atomic>
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
    int nPriority;
    int nMessageID;
    T Data;
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
      m_Queue;                              // Thread processing queue.
  std::atomic<bool> m_bIsRunning = false;   // Is this thread running?
  std::atomic<bool> m_bIsSuspended = false; // Is this thread suspended?
  int m_nRateMs = 10;         // The thread processing rate in milliseconds, default set for 10 ms.
  mutable std::mutex m_Mutex; // Mutex.

protected:
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
   * Override this function to process something before dequeuing something.
   * It'll run even if the queue is empty.
   */
  virtual void ProcessPreamble() {}

  /*
   * This is the function that the thread object will run.
   */
  void Execute() {
    while (m_bIsRunning) {
      if (m_bIsSuspended) {
        Sleep(50); // If it is suspended we wait a little.
        continue;
      }

      ProcessPreamble();

      std::optional<SData> Data = Dequeue();
      if (not Data.has_value())
        continue;
      Process((*Data).nMessageID, (*Data));

      Sleep(m_nRateMs);
    }
  }

public:
  /*
   * Default constructor
   */
  CDaemon() = default;

  /*
   * Constructor.
   * @param nThreadRate set the thread processing rate, in milliseconds.
   * @param bStartSuspended default as true. If set to false the thread will run after construction.
   */
  CDaemon(int nThreadRateMs, bool bStartSuspended = true) {
    m_nRateMs = nThreadRateMs;
    if (!bStartSuspended)
      Start();
  }

  /*
   * Destructor.
   * We clear the queue.
   */
  virtual ~CDaemon() {
    while (!m_Queue.empty())
      m_Queue.pop();
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
   * Stop the thread.
   */
  void Stop() { m_bIsRunning = false; }

  /*
   * Is this thread running?
   */
  inline bool IsRunning() const { return m_bIsRunning; }

  /*
   * Is this thread suspended?
   */
  bool IsSuspended() const { return m_bIsSuspended; }

  /*
   * Suspends the thread.
   */
  void Suspend() { m_bIsSuspended = true; }

  /*
   * Resume the thread processing.
   */
  void Resume() { m_bIsSuspended = false; }

  /*
   * Is this thread joinable?
   */
  bool Joinable() const { return m_Thread.joinable(); }

  /*
   * Make another thread wait for this one.
   */
  void Join() { m_Thread.join(); }

  /*
   * Detach this thread, so it'll run as a daemon.
   * Be careful, other threads will not wait for this one.
   */
  void Detach() { m_Thread.detach(); }

  /*
   * Sleep function.
   * @param nMs sleep time in milliseconds.
   */
  void Sleep(int nMs) { std::this_thread::sleep_for(std::chrono::milliseconds(nMs)); }

  /*
   * Enqueue a data object.
   * @param data The data object that'll be processed by this thread.
   * @see SData
   */
  void SafeAddMessage(const SData &Data) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Queue.push(std::move(Data));
  }
};

#endif // DAEMON_NS_H
