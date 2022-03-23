[![CI](https://github.com/eHonnef/thread-cpp/workflows/CI/badge.svg)](https://github.com/eHonnef/thread-cpp/actions?query=workflow:"CI")
[![License](https://img.shields.io/badge/License-MIT-blue)](#license)

# Thread wrapper class

This is a wrapper for the `std::thread` object using `std::conditional_variable` to tell the thread when there is data available to process. If the queue is empty the thread is suspended.

The functions inside [Daemon.cc](include/ThreadWrapper/Daemon.cc) are well commented (at least I think so).

## Usage example

Check out [SimplePrint.cc](app/SimplePrint.cc) for an usage example. It implements the producer/consumer problem, the "main" thread generate random strings and the thread wrapper consumes it.

- 1) We generate 10 random strings ("instantly") and print it every 1 second in the other thread context.
- 2) You can post manually strings in the thread via `std::cin`, if you don't want to, just type `exit`.
  - 2.1) If you want to suspend the thread, for 5 seconds, type `sleep`.
- 3) After typing `exit` you will `stop` the thread, it'll wait to finish the queue, then press `enter` to exit the program.
