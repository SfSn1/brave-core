/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/operational_patterns_util.h"

#include "base/time/time.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests
// -filter=BraveFederatedOperationalPatternsUtilTest*

namespace brave_federated {

class BraveFederatedOperationalPatternsUtilTest : public testing::Test {
 public:
  BraveFederatedOperationalPatternsUtilTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(BraveFederatedOperationalPatternsUtilTest,
       GetCurrentCollectionSlotAtTheBeginningOfTheMonth) {
  // Arrange
  const base::Time::Exploded exploded_time = {
      /* year */ 2022,
      /* month */ 1,
      /* day_of_week */ 6,
      /* day_of_month */ 1,
      /* hour */ 0,
      /* minute */ 0,
      /* second */ 0,
      /* millisecond */ 0,
  };
  base::Time time;
  ASSERT_TRUE(base::Time::FromLocalExploded(exploded_time, &time));

  const base::TimeDelta time_delta = time - base::Time::Now();
  task_environment_.FastForwardBy(time_delta);

  // Act
  const int current_collection_slot = GetCurrentCollectionSlot();

  // Assert
  const int expected_collection_slot = 0;
  EXPECT_EQ(expected_collection_slot, current_collection_slot);
}

TEST_F(BraveFederatedOperationalPatternsUtilTest,
       GetCurrentCollectionSlotAtTheEndOfTheMonth) {
  // Arrange
  const base::Time::Exploded exploded_time = {
      /* year */ 2022,
      /* month */ 1,
      /* day_of_week */ 1,
      /* day_of_month */ 31,
      /* hour */ 23,
      /* minute */ 59,
      /* second */ 59,
      /* millisecond */ 0,
  };
  base::Time time;
  ASSERT_TRUE(base::Time::FromLocalExploded(exploded_time, &time));

  const base::TimeDelta time_delta = time - base::Time::Now();
  task_environment_.FastForwardBy(time_delta);

  // Act
  const int current_collection_slot = GetCurrentCollectionSlot();

  // Assert
  const int expected_collection_slot = 1487;
  EXPECT_EQ(expected_collection_slot, current_collection_slot);
}

}  // namespace brave_federated
