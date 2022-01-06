/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_CONTRIBUTION_ROUTER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_CONTRIBUTION_ROUTER_H_

#include <string>

#include "bat/ledger/internal/contributions/contribution_data.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/future.h"

namespace ledger {

class ContributionRouter : public BATLedgerContext::Object {
 public:
  static const char kContextKey[];

  Future<bool> SendContribution(ContributionType contribution_type,
                                const std::string& publisher_id,
                                double amount);

  Future<bool> SendOrSavePendingContribution(const std::string& publisher_id,
                                             double amount);

  Future<absl::optional<double>> GetAvailableBalance();

  ContributionSource GetCurrentSource();
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_CONTRIBUTION_ROUTER_H_
