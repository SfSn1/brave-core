/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/base/features.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"

namespace blink {
namespace {

const SecurityOrigin* GetEphemeralOrOriginalSecurityOrigin(
    ExecutionContext* context) {
  if (base::FeatureList::IsEnabled(net::features::kBravePartitionBlobStorage)) {
    return brave::GetEphemeralOrOriginalSecurityOrigin(context);
  }

  return context->GetSecurityOrigin();
}

}  // namespace
}  // namespace blink

#include "src/third_party/blink/renderer/core/fileapi/public_url_manager.cc"
