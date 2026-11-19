# epoll-ratelimiter

A lightweight C++ network rate limiter built on Linux `epoll`, featuring non-blocking TCP connections, a custom hash map, and lazy TTL expiration.

## Features

- **Epoll-based server:** Handles multiple connections using a non-blocking event loop.
- **Token Bucket:** Enforces per-client request limits with token replenishment.
- **Custom Hash Map:** FNV-1a hashing with separate chaining for client tracking.
- **Min-Heap TTL:** Lazy expiration using an array-backed binary min-heap.

## Architecture

```text
TCP Connections
      |
   epoll Loop
      |
  Rate Limiter
   /        \
Hash Map   Min-Heap
   |          |
Client     Expiration
 State      Tracking
```

## Tech Stack

- C++
- Linux `epoll`
- TCP sockets
- FNV-1a hashing
- Custom Hash Map & Min-Heap

## Build & Run

```bash
g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o rate-limiter
./rate-limiter
```

## Project Structure

```text
rate-limiter/
├── hashmap.hpp
├── MinHeap.hpp
├── RateLimiter.hpp
├── server.hpp
└── main.cpp
```

**Concepts:** Event-driven networking, non-blocking I/O, rate limiting, hashing, heap-based expiration, and Linux systems programming.
