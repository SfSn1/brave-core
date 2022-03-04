/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_nonce_tracker.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/fil_tx_meta.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/nonce_tracker.h"
#include "brave/components/brave_wallet/common/fil_address.h"

namespace brave_wallet {

FilNonceTracker::FilNonceTracker(TxStateManager* tx_state_manager,
                                 JsonRpcService* json_rpc_service)
    : NonceTracker(tx_state_manager, json_rpc_service), weak_factory_(this) {}

FilNonceTracker::~FilNonceTracker() = default;

void FilNonceTracker::GetNextNonce(const std::string& from,
                                   GetNextNonceCallback callback) {
  GetJsonRpcService()->GetFilTransactionCount(
      from,
      base::BindOnce(&FilNonceTracker::OnFilGetNetworkNonce,
                     weak_factory_.GetWeakPtr(), from, std::move(callback)));
}

uint256_t FilNonceTracker::GetHighestLocallyConfirmed(
    const std::vector<std::unique_ptr<TxMeta>>& metas) {
  uint64_t highest = 0;
  for (auto& meta : metas) {
    auto* fil_meta = static_cast<FilTxMeta*>(meta.get());
    DCHECK(
        fil_meta->tx()->nonce());  // Not supposed to happen for a confirmed tx.
    highest = std::max(highest, fil_meta->tx()->nonce().value() + (uint64_t)1);
  }
  return static_cast<uint256_t>(highest);
}

uint256_t FilNonceTracker::GetHighestContinuousFrom(
    const std::vector<std::unique_ptr<TxMeta>>& metas,
    uint256_t start) {
  bool valid_range = start <= static_cast<uint256_t>(UINT64_MAX);
  DCHECK(valid_range);
  uint64_t highest = start;
  for (auto& meta : metas) {
    auto* fil_meta = static_cast<FilTxMeta*>(meta.get());
    DCHECK(
        fil_meta->tx()->nonce());  // Not supposed to happen for a submitted tx.
    if (fil_meta->tx()->nonce().value() == highest)
      highest++;
  }
  return static_cast<uint256_t>(highest);
}

void FilNonceTracker::OnFilGetNetworkNonce(const std::string& from,
                                           GetNextNonceCallback callback,
                                           uint256_t result,
                                           mojom::FilecoinProviderError error,
                                           const std::string& error_message) {
  if (error != mojom::FilecoinProviderError::kSuccess) {
    std::move(callback).Run(false, result);
    return;
  }
  auto nonce = GetFinalNonce(from, result);
  std::move(callback).Run(nonce.has_value(), nonce.has_value() ? *nonce : 0);
}

}  // namespace brave_wallet
