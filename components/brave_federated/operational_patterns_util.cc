/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/operational_patterns_util.h"

#include "base/time/time.h"
#include "brave/components/brave_federated/features.h"

namespace brave_federated {

namespace {

const int kMinutesInHour = 60;
const int kHoursInDay = 24;

}  // namespace

int GetCurrentCollectionSlot() {
  base::Time::Exploded now;
  base::Time::Now().LocalExplode(&now);

  const int month_to_date_in_minutes =
      (now.day_of_month - 1) * kHoursInDay * kMinutesInHour;
  const int minutes_today = now.hour * kMinutesInHour + now.minute;
  const int slot_size_in_minutes =
      brave_federated::features::GetCollectionSlotSizeValue();
  return (month_to_date_in_minutes + minutes_today) / slot_size_in_minutes;
}

}  // namespace brave_federated
