#include "counter/publisher.hpp"

#include <chrono>
#include <cstring>

#include <gtest/gtest.h>
#include <mutex>
#include <thread>
#include <vector>

#include "zenoh.hxx"

namespace counter {
namespace {

using namespace zenoh;
using namespace std::chrono_literals;

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
    // Publisher listens on a unique port.
    ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7452"});

    // Subscriber connects to the publisher.
    auto config = Config::create_default();
    config.insert_json5(Z_CONFIG_CONNECT_KEY, R"(["tcp/127.0.0.1:7452"])");
    auto sub_session = Session::open(std::move(config));

    std::vector<int64_t> received;
    std::mutex mtx;

    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("demo/counter"),
        [&received, &mtx](Sample& sample) {
            auto bytes = sample.get_payload().as_vector();
            if (bytes.size() == sizeof(int64_t)) {
                int64_t value;
                std::memcpy(&value, bytes.data(), sizeof(int64_t));
                std::lock_guard<std::mutex> lock(mtx);
                received.push_back(value);
            }
        },
        closures::none);

    // Wait for session discovery.
    std::this_thread::sleep_for(500ms);

    pub.Publish();

    // Wait for message delivery (poll up to 3 seconds).
    for (int i = 0; i < 30; ++i) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (received.size() >= 1)
                break;
        }
        std::this_thread::sleep_for(100ms);
    }

    EXPECT_EQ(pub.Counter(), 1);

    std::lock_guard<std::mutex> lock(mtx);
    ASSERT_EQ(received.size(), 1u);
    EXPECT_EQ(received[0], 1);
}

TEST(ShmCounterPublisherTest, MultiplePublishesSendIncrementingSequence) {
    ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7453"});

    auto config = Config::create_default();
    config.insert_json5(Z_CONFIG_CONNECT_KEY, R"(["tcp/127.0.0.1:7453"])");
    auto sub_session = Session::open(std::move(config));

    std::vector<int64_t> received;
    std::mutex mtx;

    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("demo/counter"),
        [&received, &mtx](Sample& sample) {
            auto bytes = sample.get_payload().as_vector();
            if (bytes.size() == sizeof(int64_t)) {
                int64_t value;
                std::memcpy(&value, bytes.data(), sizeof(int64_t));
                std::lock_guard<std::mutex> lock(mtx);
                received.push_back(value);
            }
        },
        closures::none);

    std::this_thread::sleep_for(500ms);

    pub.Publish();
    pub.Publish();
    pub.Publish();

    for (int i = 0; i < 30; ++i) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (received.size() >= 3)
                break;
        }
        std::this_thread::sleep_for(100ms);
    }

    EXPECT_EQ(pub.Counter(), 3);

    std::lock_guard<std::mutex> lock(mtx);
    ASSERT_EQ(received.size(), 3u);
    EXPECT_EQ(received[0], 1);
    EXPECT_EQ(received[1], 2);
    EXPECT_EQ(received[2], 3);
}

TEST(ShmCounterPublisherTest, PayloadIsExactly8Bytes) {
    ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7454"});

    auto config = Config::create_default();
    config.insert_json5(Z_CONFIG_CONNECT_KEY, R"(["tcp/127.0.0.1:7454"])");
    auto sub_session = Session::open(std::move(config));

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

}  // namespace
}  // namespace counter
