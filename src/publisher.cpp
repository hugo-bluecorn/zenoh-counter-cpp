#include "counter/publisher.hpp"

#include <cstring>
#include <stdexcept>
#include <string>

#include "zenoh.hxx"

using namespace zenoh;

namespace counter {
namespace {

constexpr size_t kShmPoolSize = 65536;

// Formats a list of endpoints as a JSON5 array string for zenoh config.
std::string FormatEndpointsJson(const std::vector<std::string>& endpoints) {
    std::string json = "[";
    for (size_t i = 0; i < endpoints.size(); ++i) {
        if (i > 0)
            json += ",";
        json += "\"" + endpoints[i] + "\"";
    }
    json += "]";
    return json;
}

}  // namespace

struct ShmCounterPublisher::Impl {
    Session session;
    Publisher publisher;
    PosixShmProvider shm_provider;

    Impl(Session session, Publisher publisher, PosixShmProvider shm_provider)
        : session(std::move(session)),
          publisher(std::move(publisher)),
          shm_provider(std::move(shm_provider)) {}
};

ShmCounterPublisher::ShmCounterPublisher(
    const std::string& key_expr,
    const std::vector<std::string>& connect_endpoints,
    const std::vector<std::string>& listen_endpoints)
    : counter_(0) {
    auto config = Config::create_default();

    if (!connect_endpoints.empty()) {
        config.insert_json5(Z_CONFIG_CONNECT_KEY,
                            FormatEndpointsJson(connect_endpoints).c_str());
    }

    if (!listen_endpoints.empty()) {
        config.insert_json5(Z_CONFIG_LISTEN_KEY,
                            FormatEndpointsJson(listen_endpoints).c_str());
    }

    auto session = Session::open(std::move(config));
    auto publisher = session.declare_publisher(KeyExpr(key_expr));
    PosixShmProvider shm_provider(
        MemoryLayout(kShmPoolSize, AllocAlignment({2})));

    impl_ = std::make_unique<Impl>(std::move(session), std::move(publisher),
                                   std::move(shm_provider));
}

ShmCounterPublisher::~ShmCounterPublisher() = default;

void ShmCounterPublisher::Publish() {
    ++counter_;
    auto alloc_result = impl_->shm_provider.alloc_gc_defrag_blocking(
        sizeof(int64_t), AllocAlignment({0}));
    auto buf = std::get<ZShmMut>(std::move(alloc_result));
    std::memcpy(buf.data(), &counter_, sizeof(int64_t));
    impl_->publisher.put(std::move(buf));
}

int64_t ShmCounterPublisher::Counter() const {
    return counter_;
}

}  // namespace counter
