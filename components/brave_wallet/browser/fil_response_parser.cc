/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_response_parser.h"

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/json/rs/src/lib.rs.h"

namespace brave_wallet {

bool ParseFilGetBalance(const std::string& json, std::string* balance) {
  return brave_wallet::ParseSingleStringResult(json, balance);
}

bool ParseFilGetTransactionCount(const std::string& raw_json, uint64_t* count) {
  if (raw_json.empty())
    return false;
  std::string sanitized_json(
      json::convert_uint64_to_unicode_string("/result", raw_json.c_str()).c_str());
  if (sanitized_json.empty())
    return false;
  std::string sanitized_value;
  if (!brave_wallet::ParseSingleStringResult(sanitized_json,
                                             &sanitized_value) ||
      sanitized_value.empty())
    return false;
  return base::StringToUint64(sanitized_value, count);
}

}  // namespace brave_wallet
