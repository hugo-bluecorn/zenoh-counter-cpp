#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace counter {

class ShmCounterPublisher {
 public:
  explicit ShmCounterPublisher(
      const std::string& key_expr = "demo/counter",
      const std::vector<std::string>& connect_endpoints = {},
      const std::vector<std::string>& listen_endpoints = {});

  ShmCounterPublisher(const ShmCounterPublisher&) = delete;
  ShmCounterPublisher& operator=(const ShmCounterPublisher&) = delete;

  ~ShmCounterPublisher();

  void Publish();
  int64_t Counter() const;

 private:
  int64_t counter_ = 0;
};

}  // namespace counter
