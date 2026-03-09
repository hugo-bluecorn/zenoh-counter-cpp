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

    // Polls until at least `count` values are received, or 3 seconds elapse.
    void WaitFor(size_t count) {
        for (int i = 0; i < 30; ++i) {
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

// --- Construction Tests (Slice 2) ---

TEST(ShmCounterPublisherTest, DefaultConstructionSucceeds) {
    EXPECT_NO_THROW({
        ShmCounterPublisher pub;
        EXPECT_EQ(pub.Counter(), 0);
    });
}

TEST(ShmCounterPublisherTest, CustomKeyExpressionSucceeds) {
    EXPECT_NO_THROW({
        ShmCounterPublisher pub("demo/test/counter");
        EXPECT_EQ(pub.Counter(), 0);
    });
}

TEST(ShmCounterPublisherTest, ListenEndpointSucceeds) {
    EXPECT_NO_THROW(
        { ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7450"}); });
}

TEST(ShmCounterPublisherTest, ConnectEndpointSucceeds) {
    EXPECT_NO_THROW(
        { ShmCounterPublisher pub("demo/counter", {"tcp/127.0.0.1:7451"}); });
}

// --- Publish Tests (Slice 3) ---

TEST(ShmCounterPublisherTest, PublishIncrementsCounterAndSendsInt64) {
    ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7452"});
    auto sub_session = ConnectSession(7452);

    Int64Collector collector;
    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("demo/counter"), collector.Callback(), closures::none);

    std::this_thread::sleep_for(500ms);
    pub.Publish();
    collector.WaitFor(1);

    EXPECT_EQ(pub.Counter(), 1);
    std::lock_guard<std::mutex> lock(collector.mtx);
    ASSERT_EQ(collector.values.size(), 1u);
    EXPECT_EQ(collector.values[0], 1);
}

TEST(ShmCounterPublisherTest, MultiplePublishesSendIncrementingSequence) {
    ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7453"});
    auto sub_session = ConnectSession(7453);

    Int64Collector collector;
    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("demo/counter"), collector.Callback(), closures::none);

    std::this_thread::sleep_for(500ms);
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

TEST(ShmCounterPublisherTest, PayloadIsExactly8Bytes) {
    ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7454"});
    auto sub_session = ConnectSession(7454);

    std::vector<size_t> payload_sizes;
    std::mutex mtx;

    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("demo/counter"),
        [&payload_sizes, &mtx](Sample& sample) {
            auto bytes = sample.get_payload().as_vector();
            std::lock_guard<std::mutex> lock(mtx);
            payload_sizes.push_back(bytes.size());
        },
        closures::none);

    std::this_thread::sleep_for(500ms);
    pub.Publish();

    for (int i = 0; i < 30; ++i) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (payload_sizes.size() >= 1)
                break;
        }
        std::this_thread::sleep_for(100ms);
    }

    std::lock_guard<std::mutex> lock(mtx);
    ASSERT_EQ(payload_sizes.size(), 1u);
    EXPECT_EQ(payload_sizes[0], sizeof(int64_t));
}

// --- Integration Tests (Slice 4) ---

TEST(ShmCounterPublisherIntegrationTest,
     PublisherOnListenSessionDeliversToSubscriberOnConnectSession) {
    ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7460"});
    auto sub_session = ConnectSession(7460);

    Int64Collector collector;
    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("demo/counter"), collector.Callback(), closures::none);

    std::this_thread::sleep_for(500ms);
    pub.Publish();
    collector.WaitFor(1);

    EXPECT_EQ(pub.Counter(), 1);
    std::lock_guard<std::mutex> lock(collector.mtx);
    ASSERT_EQ(collector.values.size(), 1u);
    EXPECT_EQ(collector.values[0], pub.Counter());
}

TEST(ShmCounterPublisherIntegrationTest,
     MultiplePublishesReceivedAcrossSessions) {
    ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7461"});
    auto sub_session = ConnectSession(7461);

    Int64Collector collector;
    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("demo/counter"), collector.Callback(), closures::none);

    std::this_thread::sleep_for(500ms);
    for (int i = 0; i < 5; ++i) {
        pub.Publish();
    }
    collector.WaitFor(5);

    EXPECT_EQ(pub.Counter(), 5);
    std::lock_guard<std::mutex> lock(collector.mtx);
    ASSERT_EQ(collector.values.size(), 5u);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(collector.values[i], i + 1);
    }
}

TEST(ShmCounterPublisherIntegrationTest,
     SubscriberReceivesCorrectByteEncodingCrossSession) {
    ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7462"});
    auto sub_session = ConnectSession(7462);

    std::vector<std::vector<uint8_t>> raw_payloads;
    Int64Collector collector;

    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("demo/counter"),
        [&raw_payloads, &collector](Sample& sample) {
            auto bytes = sample.get_payload().as_vector();
            if (bytes.size() == sizeof(int64_t)) {
                int64_t value;
                std::memcpy(&value, bytes.data(), sizeof(int64_t));
                std::lock_guard<std::mutex> lock(collector.mtx);
                raw_payloads.push_back(bytes);
                collector.values.push_back(value);
            }
        },
        closures::none);

    std::this_thread::sleep_for(500ms);

    // Publish 100 times to reach a large counter value.
    for (int i = 0; i < 100; ++i) {
        pub.Publish();
    }
    collector.WaitFor(100);

    EXPECT_EQ(pub.Counter(), 100);

    std::lock_guard<std::mutex> lock(collector.mtx);
    ASSERT_GE(collector.values.size(), 1u);

    // Verify the last received payload is exactly 8 bytes and decodes
    // correctly.
    auto& last_raw = raw_payloads.back();
    EXPECT_EQ(last_raw.size(), sizeof(int64_t));

    int64_t last_value = collector.values.back();
    EXPECT_EQ(last_value, static_cast<int64_t>(collector.values.size()));
}

}  // namespace
}  // namespace counter
