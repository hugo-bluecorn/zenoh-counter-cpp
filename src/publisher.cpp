#include "counter/publisher.hpp"

#include <stdexcept>

namespace counter {

ShmCounterPublisher::ShmCounterPublisher(
    const std::string& /*key_expr*/,
    const std::vector<std::string>& /*connect_endpoints*/,
    const std::vector<std::string>& /*listen_endpoints*/) {
    throw std::runtime_error("Not implemented");
}

ShmCounterPublisher::~ShmCounterPublisher() = default;

void ShmCounterPublisher::Publish() {
    throw std::runtime_error("Not implemented");
}

int64_t ShmCounterPublisher::Counter() const { return counter_; }

}  // namespace counter
