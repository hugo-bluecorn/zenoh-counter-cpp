#include "counter/publisher.hpp"

#include <stdexcept>
#include <string>

#include "zenoh.hxx"

using namespace zenoh;

namespace counter {

struct ShmCounterPublisher::Impl {
    Session session;
    Publisher publisher;
    PosixShmProvider shm_provider;

    Impl(Session session, Publisher publisher,
         PosixShmProvider shm_provider)
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
        std::string json = "[";
        for (size_t i = 0; i < connect_endpoints.size(); ++i) {
            if (i > 0) json += ",";
            json += "\"" + connect_endpoints[i] + "\"";
        }
        json += "]";
        config.insert_json5(Z_CONFIG_CONNECT_KEY, json.c_str());
    }

    if (!listen_endpoints.empty()) {
        std::string json = "[";
        for (size_t i = 0; i < listen_endpoints.size(); ++i) {
            if (i > 0) json += ",";
            json += "\"" + listen_endpoints[i] + "\"";
        }
        json += "]";
        config.insert_json5(Z_CONFIG_LISTEN_KEY, json.c_str());
    }

    auto session = Session::open(std::move(config));
    auto publisher = session.declare_publisher(KeyExpr(key_expr));
    PosixShmProvider shm_provider(MemoryLayout(65536, AllocAlignment({2})));

    impl_ = std::make_unique<Impl>(std::move(session), std::move(publisher),
                                   std::move(shm_provider));
}

ShmCounterPublisher::~ShmCounterPublisher() = default;

void ShmCounterPublisher::Publish() {
    throw std::runtime_error("Not implemented");
}

int64_t ShmCounterPublisher::Counter() const { return counter_; }

}  // namespace counter
