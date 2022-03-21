/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/shared_future.h"

#include <vector>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ledger {

class SharedFutureTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
};

TEST_F(SharedFutureTest, MultipleListeners) {
  std::vector<int> values;
  auto push_value = [&values](const int& value) { values.push_back(value); };

  SharedFuture<int> tee(MakeReadyFuture(42));

  tee.Then(base::BindLambdaForTesting(push_value));
  tee.Then(base::BindLambdaForTesting(push_value));
  tee.Then(base::BindLambdaForTesting(push_value));

  EXPECT_TRUE(values.empty());
  task_environment_.RunUntilIdle();
  ASSERT_EQ(values.size(), 3ul);
  EXPECT_EQ(values[0], 42);

  tee.Then(base::BindLambdaForTesting(push_value));
  ASSERT_EQ(values.size(), 3ul);
  task_environment_.RunUntilIdle();
  ASSERT_EQ(values.size(), 4ul);
}

TEST_F(SharedFutureTest, Copyable) {
  int value = 0;
  auto set_value = [&value](const int& v) { value = v; };

  SharedFuture<int> tee1(MakeReadyFuture(42));
  SharedFuture<int> tee2 = tee1;

  tee2.Then(base::BindLambdaForTesting(set_value));

  task_environment_.RunUntilIdle();
  EXPECT_EQ(value, 42);
}

TEST_F(SharedFutureTest, ValueSentAfterTeeIsDestroyed) {
  int value = 0;
  auto set_value = [&value](const int& v) { value = v; };

  Promise<int> promise;

  {
    SharedFuture<int> tee(promise.GetFuture());
    tee.Then(base::BindLambdaForTesting(set_value));
  }

  promise.SetValue(42);

  task_environment_.RunUntilIdle();
  EXPECT_EQ(value, 42);
}

}  // namespace ledger
