// Copyright Sleak Software. All Rights Reserved.
//
// Tests for FTactixEventChannel — the minimal typed event bus used by
// TactixCore subsystems (no UE delegate, no per-publish heap). Verifies
// ordered delivery, unsubscribe, capacity guard, and context pass-through.

#include "Foundation/TactixEventBus.h"

#include <gtest/gtest.h>

namespace
{
    struct TargetChanged
    {
        int NewTargetId;
    };

    struct Listener
    {
        int  LastSeen{-1};
        int  CallCount{0};

        static void Handler(const TargetChanged& E, void* Ctx)
        {
            auto* Self = static_cast<Listener*>(Ctx);
            Self->LastSeen = E.NewTargetId;
            Self->CallCount += 1;
        }
    };
}

TEST(TactixEventBus, SubscribeReceivesAndPassesContext)
{
    Tactix::FTactixEventChannel<TargetChanged> Ch;
    Listener L;

    EXPECT_TRUE(Ch.Subscribe(&Listener::Handler, &L));
    EXPECT_EQ(Ch.NumSubscribers(), 1u);

    Ch.Publish({42});
    EXPECT_EQ(L.LastSeen, 42);
    EXPECT_EQ(L.CallCount, 1);

    Ch.Publish({7});
    EXPECT_EQ(L.LastSeen, 7);
    EXPECT_EQ(L.CallCount, 2);
}

TEST(TactixEventBus, MultipleSubscribersAllNotified)
{
    Tactix::FTactixEventChannel<TargetChanged> Ch;
    Listener A, B, C;

    EXPECT_TRUE(Ch.Subscribe(&Listener::Handler, &A));
    EXPECT_TRUE(Ch.Subscribe(&Listener::Handler, &B));
    EXPECT_TRUE(Ch.Subscribe(&Listener::Handler, &C));

    Ch.Publish({100});
    EXPECT_EQ(A.LastSeen, 100);
    EXPECT_EQ(B.LastSeen, 100);
    EXPECT_EQ(C.LastSeen, 100);
}

TEST(TactixEventBus, UnsubscribeStopsDelivery)
{
    Tactix::FTactixEventChannel<TargetChanged> Ch;
    Listener A, B;

    Ch.Subscribe(&Listener::Handler, &A);
    Ch.Subscribe(&Listener::Handler, &B);

    EXPECT_TRUE(Ch.Unsubscribe(&Listener::Handler, &A));
    Ch.Publish({55});
    EXPECT_EQ(A.CallCount, 0);
    EXPECT_EQ(B.LastSeen, 55);
}

TEST(TactixEventBus, UnsubscribeUnknownReturnsFalse)
{
    Tactix::FTactixEventChannel<TargetChanged> Ch;
    Listener Unrelated;
    EXPECT_FALSE(Ch.Unsubscribe(&Listener::Handler, &Unrelated));
}

TEST(TactixEventBus, SubscribeRejectsNullFnAndOverflow)
{
    Tactix::FTactixEventChannel<TargetChanged, 2> Ch;
    Listener A, B, C;

    EXPECT_FALSE(Ch.Subscribe(nullptr, &A));
    EXPECT_TRUE (Ch.Subscribe(&Listener::Handler, &A));
    EXPECT_TRUE (Ch.Subscribe(&Listener::Handler, &B));
    EXPECT_FALSE(Ch.Subscribe(&Listener::Handler, &C)); // over capacity

    EXPECT_EQ(Ch.NumSubscribers(), 2u);
    EXPECT_EQ(Ch.MaxCapacity(),    2u);
}

TEST(TactixEventBus, ClearRemovesAllSubscribers)
{
    Tactix::FTactixEventChannel<TargetChanged> Ch;
    Listener L;
    Ch.Subscribe(&Listener::Handler, &L);
    Ch.Clear();
    Ch.Publish({1});
    EXPECT_EQ(L.CallCount, 0);
}
