#include "counter/publisher.hpp"

#include <chrono>
#include <cstring>

#include <functional>
#include <gtest/gtest.h>
#include <mutex>
#include <thread>
#include <vector>

#include "zenoh.hxx"

namespace counter {
namespace {

using namespace zenoh;
using namespace std::chrono_literals;

// --- Test Helpers ---

// Thread-safe collector for int64 values received via a zenoh subscriber.
struct Int64Collector {
    std::vector<int64_t> values;
    std::mutex mtx;

    // Returns a callback suitable for declare_subscriber.
    auto Callback() {
        return [this](Sample& sample) {
            auto bytes = sample.get_payload().as_vector();
            if (bytes.size() == sizeof(int64_t)) {
                int64_t value;
                std::memcpy(&value, bytes.data(), sizeof(int64_t));
                std::lock_guard<std::mutex> lock(mtx);
                values.push_back(value);
            }
        };
    }

    // Polls until at least `count` values are received, or timeout elapses.
    void WaitFor(size_t count, int max_iterations = 50) {
        for (int i = 0; i < max_iterations; ++i) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (values.size() >= count)
                    return;
            }
            std::this_thread::sleep_for(100ms);
        }
    }

    // Returns current size under lock.
    size_t Size() {
        std::lock_guard<std::mutex> lock(mtx);
        return values.size();
    }
};

// Creates a zenoh Session that connects to the given port on localhost.
Session ConnectSession(int port) {
    auto config = Config::create_default();
    std::string endpoint =
        R"(["tcp/127.0.0.1:)" + std::to_string(port) + R"("])";
    config.insert_json5(Z_CONFIG_CONNECT_KEY, endpoint.c_str());
    return Session::open(std::move(config));
}

// --- Peer Direct Topology ---

TEST(IntegrationTest, PeerDirect_PublisherListenSubscriberConnect) {
    // Publisher listens on port 7485.
    ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7485"});

    // Subscriber connects to the publisher.
    auto sub_session = ConnectSession(7485);

    Int64Collector collector;
    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("demo/counter"), collector.Callback(), closures::none);

    std::this_thread::sleep_for(500ms);

    // Publish 3 messages.
    pub.Publish();
    pub.Publish();
    pub.Publish();
    collector.WaitFor(3);

    EXPECT_EQ(pub.Counter(), 3);
    std::lock_guard<std::mutex> lock(collector.mtx);
    ASSERT_EQ(collector.values.size(), 3u);
    EXPECT_EQ(collector.values[0], 1);
    EXPECT_EQ(collector.values[1], 2);
    EXPECT_EQ(collector.values[2], 3);
}

// --- Via Router Topology ---

TEST(IntegrationTest, ViaRouter_BothConnectThroughRouter) {
    // Create router session listening on port 7486.
    auto router_config = Config::create_default();
    router_config.insert_json5("mode", "\"router\"");
    router_config.insert_json5("listen/endpoints", "[\"tcp/0.0.0.0:7486\"]");
    auto router_session = Session::open(std::move(router_config));

    // Allow router to start listening.
    std::this_thread::sleep_for(1000ms);

    // Publisher connects to the router.
    ShmCounterPublisher pub("demo/counter", {"tcp/127.0.0.1:7486"});

    // Subscriber connects to the router.
    auto sub_session = ConnectSession(7486);

    Int64Collector collector;
    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("demo/counter"), collector.Callback(), closures::none);

    // Extra time for route propagation through the router.
    std::this_thread::sleep_for(500ms);

    // Publish 3 messages.
    pub.Publish();
    pub.Publish();
    pub.Publish();
    collector.WaitFor(3, 50);  // 5 second timeout for router tests.

    EXPECT_EQ(pub.Counter(), 3);
    std::lock_guard<std::mutex> lock(collector.mtx);
    ASSERT_EQ(collector.values.size(), 3u);
    EXPECT_EQ(collector.values[0], 1);
    EXPECT_EQ(collector.values[1], 2);
    EXPECT_EQ(collector.values[2], 3);
}

TEST(IntegrationTest, ViaRouter_CustomKeyExpression) {
    // Create router session listening on port 7487.
    auto router_config = Config::create_default();
    router_config.insert_json5("mode", "\"router\"");
    router_config.insert_json5("listen/endpoints", "[\"tcp/0.0.0.0:7487\"]");
    auto router_session = Session::open(std::move(router_config));

    // Allow router to start listening.
    std::this_thread::sleep_for(1000ms);

    // Publisher connects to the router with custom key.
    ShmCounterPublisher pub("test/custom", {"tcp/127.0.0.1:7487"});

    // Subscriber connects to the router.
    auto sub_session = ConnectSession(7487);

    Int64Collector collector;
    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("test/custom"), collector.Callback(), closures::none);

    // Extra time for route propagation through the router.
    std::this_thread::sleep_for(500ms);

    // Publish 2 messages.
    pub.Publish();
    pub.Publish();
    collector.WaitFor(2, 50);  // 5 second timeout for router tests.

    EXPECT_EQ(pub.Counter(), 2);
    std::lock_guard<std::mutex> lock(collector.mtx);
    ASSERT_EQ(collector.values.size(), 2u);
    EXPECT_EQ(collector.values[0], 1);
    EXPECT_EQ(collector.values[1], 2);
}

}  // namespace
}  // namespace counter
