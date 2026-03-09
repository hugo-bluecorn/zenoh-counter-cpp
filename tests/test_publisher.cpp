#include <gtest/gtest.h>

#include "counter/publisher.hpp"

namespace counter {
namespace {

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
    EXPECT_NO_THROW({
        ShmCounterPublisher pub("demo/counter", {},
                                {"tcp/0.0.0.0:7450"});
    });
}

TEST(ShmCounterPublisherTest, ConnectEndpointSucceeds) {
    EXPECT_NO_THROW({
        ShmCounterPublisher pub("demo/counter",
                                {"tcp/127.0.0.1:7451"});
    });
}

}  // namespace
}  // namespace counter
