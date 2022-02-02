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
template <class T> class Daemon {
public:
  /*
   * Data struct to hold the data and the info about on how to process this data (using nMessageID).
   */
  struct Data {
    int nPriority; //@todo: priority queue
    int nMessageID;
    T TData;
    Data(int pnPriority, int pnMessageID, T pTData)
        : nPriority(pnPriority), nMessageID(pnMessageID), TData(pTData) {}
  };

private:
  std::thread m_Thread;                     // Thread object.
  std::queue<Data> m_Queue;                 // Thread processing queue.
  std::atomic<bool> m_bIsRunning = false;   // Is this thread running?
  std::atomic<bool> m_bIsSuspended = false; // Is this thread suspended?
  int m_nRateMs = 10;                       // The thread processing rate, default set for 10 ms.
  mutable std::mutex m_Mutex;               // Mutex.

  /*
   * Safely dequeue a Data object so we can process it.
   * @return a Data object wrapped in the std::optional, if the queue is empty it'll return an empty
   *std::optional.
   */
  std::optional<Data> dequeue() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Queue.empty())
      return {};
    auto rtn = m_Queue.front();
    m_Queue.pop();
    return rtn;
  }

protected:
  /*
   * Sleep function.
   * @param nMs sleep time in milliseconds.
   */
  void sleep(int nMs) { std::this_thread::sleep_for(std::chrono::milliseconds(nMs)); }

  /*
   * Override this function to process your data inside the thread.
   * @param nMessageID The message ID, so you can control what/how to process a Data object.
   * @param data The Data object that was dequeued.
   * @see Data
   * @see dequeue()
   */
  virtual void process(int nMessageID, const Data &data) = 0;

  /*
   * This is the function that the thread object will run.
   */
  void execute() {
    while (m_bIsRunning) {
      if (m_bIsSuspended) {
        sleep(50); // If it is suspended we wait a little.
        continue;
      }

      std::optional<Data> data = dequeue();
      if (not data.has_value())
        continue;
      process((*data).nMessageID, (*data));

      sleep(m_nRateMs);
    }
  }

public:
  /*
   * Default constructor
   */
  Daemon() = default;
  /*
   * Constructor.
   * @param nThreadRate set the thread processing rate.
   * @param bStartSuspended default as true. If set to false the thread will run after construction.
   */
  Daemon(int nThreadRate, bool bStartSuspended = true) {
    m_nRateMs = nThreadRate;
    if (!bStartSuspended)
      start();
  }

  /*
   * Destructor.
   * We clear the queue.
   */
  virtual ~Daemon() {
    while (!m_Queue.empty())
      m_Queue.pop();
  }

  /*
   * Starts the thread.
   */
  void start() {
    if (not m_bIsRunning) {
      m_Thread = std::thread(&Daemon<T>::execute, this);
      m_bIsRunning = true;
    }
  }

  /*
   * Stop the thread.
   */
  void stop() { m_bIsRunning = false; }

  /*
   * Is this thread running?
   */
  inline bool isRunning() const { return m_bIsRunning; }

  /*
   * Is this thread suspended?
   */
  bool isSuspended() const { return m_bIsSuspended; }

  /*
   * Suspends the thread.
   */
  void suspend() { m_bIsSuspended = true; }

  /*
   * Resume the thread processing.
   */
  void resume() { m_bIsSuspended = false; }

  /*
   * Is this thread joinable?
   */
  bool joinable() const { return m_Thread.joinable(); }

  /*
   * Make another thread wait for this one.
   */
  void join() { m_Thread.join(); }

  /*
   * Detach this thread, so it'll run as a daemon.
   * Be careful, other threads will not wait for this one.
   */
  void detach() { m_Thread.detach(); }

  /*
   * Enqueue a data object.
   * @param data The data object that'll be processed by this thread.
   * @see Data
   */
  void safeAddMessage(const Data &data) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Queue.push(std::move(data));
  }
};

#endif // DAEMON_NS_H
