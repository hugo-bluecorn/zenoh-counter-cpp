#include "counter/publisher.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>

#include <functional>
#include <gtest/gtest.h>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "zenoh.hxx"

namespace counter {
namespace {

using namespace zenoh;
using namespace std::chrono_literals;

// --- Test Helpers (same patterns as test_publisher.cpp) ---

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

// --- Test 1: Subscriber Receives and Decodes Single Int64 ---

TEST(SubscriberDecodeTest, ReceivesAndDecodesSingleInt64) {
    ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7480"});
    auto sub_session = ConnectSession(7480);

    Int64Collector collector;
    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("demo/counter"), collector.Callback(), closures::none);

    std::this_thread::sleep_for(500ms);
    pub.Publish();
    collector.WaitFor(1);

    std::lock_guard<std::mutex> lock(collector.mtx);
    ASSERT_EQ(collector.values.size(), 1u);
    EXPECT_EQ(collector.values[0], 1);
}

// --- Test 2: Subscriber Receives Incrementing Sequence ---

TEST(SubscriberDecodeTest, ReceivesIncrementingSequence) {
    ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7481"});
    auto sub_session = ConnectSession(7481);

    Int64Collector collector;
    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("demo/counter"), collector.Callback(), closures::none);

    std::this_thread::sleep_for(500ms);
    for (int i = 0; i < 5; ++i) {
        pub.Publish();
    }
    collector.WaitFor(5);

    std::lock_guard<std::mutex> lock(collector.mtx);
    ASSERT_EQ(collector.values.size(), 5u);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(collector.values[i], i + 1);
    }
}

// --- Test 3: Count Flag Causes Exit After N Messages ---

TEST(SubscriberDecodeTest, CountFlagExitsAfterNMessages) {
    // Start publisher listening on port 7482
    ShmCounterPublisher pub("demo/counter", {}, {"tcp/0.0.0.0:7482"});

    // Allow publisher session to settle
    std::this_thread::sleep_for(500ms);

    // Launch counter_sub as subprocess with -n 3
    // Publish in a background thread while counter_sub is running
    std::thread pub_thread([&pub]() {
        // Give counter_sub time to start and connect
        std::this_thread::sleep_for(1000ms);
        for (int i = 0; i < 5; ++i) {
            pub.Publish();
            std::this_thread::sleep_for(200ms);
        }
    });

    std::string cmd = std::string("timeout 10 ") + COUNTER_SUB_BIN +
                       " -e tcp/127.0.0.1:7482 -n 3 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    ASSERT_NE(pipe, nullptr);

    std::vector<std::string> lines;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        lines.emplace_back(buffer);
    }
    int status = pclose(pipe);

    pub_thread.join();

    // counter_sub should exit with code 0 (timeout returns 124 on timeout)
    EXPECT_EQ(WEXITSTATUS(status), 0);

    // Count lines matching the "[N] Received on 'demo/counter'" pattern
    int received_count = 0;
    for (const auto& line : lines) {
        if (line.find("Received on '") != std::string::npos) {
            ++received_count;
        }
    }
    EXPECT_EQ(received_count, 3);
}

// --- Test 4: Subscriber Handles Non-8-Byte Payload Gracefully ---

TEST(SubscriberDecodeTest, HandlesNon8BytePayloadGracefully) {
    // Create a plain zenoh session that listens (not ShmCounterPublisher)
    auto config = Config::create_default();
    config.insert_json5(Z_CONFIG_LISTEN_KEY, R"(["tcp/0.0.0.0:7483"])");
    auto pub_session = Session::open(std::move(config));
    auto publisher = pub_session.declare_publisher(KeyExpr("demo/counter"));

    auto sub_session = ConnectSession(7483);

    Int64Collector collector;
    auto subscriber = sub_session.declare_subscriber(
        KeyExpr("demo/counter"), collector.Callback(), closures::none);

    std::this_thread::sleep_for(500ms);

    // Send a 4-byte payload (malformed -- not 8 bytes)
    std::vector<uint8_t> short_payload = {0x01, 0x02, 0x03, 0x04};
    publisher.put(Bytes(short_payload));

    // Wait a bit to ensure the message would have been processed
    std::this_thread::sleep_for(500ms);

    // Collector should have received 0 values (4-byte payload is skipped)
    EXPECT_EQ(collector.Size(), 0u);
}

}  // namespace
}  // namespace counter
