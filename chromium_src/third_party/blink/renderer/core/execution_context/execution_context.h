/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EXECUTION_CONTEXT_EXECUTION_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EXECUTION_CONTEXT_EXECUTION_CONTEXT_H_

#include "src/third_party/blink/renderer/core/execution_context/execution_context.h"

#include <random>

#include "base/callback.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"

namespace blink {
class WebContentSettingsClient;
class WebSecurityOrigin;
}  // namespace blink

namespace brave {

typedef base::RepeatingCallback<float(float, size_t)> AudioFarblingCallback;

CORE_EXPORT blink::WebContentSettingsClient* GetContentSettingsClientFor(
    blink::ExecutionContext* context);
CORE_EXPORT blink::WebSecurityOrigin GetEphemeralOrOriginalSecurityOrigin(
    blink::ExecutionContext* context);
CORE_EXPORT BraveFarblingLevel
GetBraveFarblingLevelFor(blink::ExecutionContext* context,
                         BraveFarblingLevel default_value);
CORE_EXPORT bool AllowFingerprinting(blink::ExecutionContext* context);

class CORE_EXPORT BraveSessionCache final
    : public blink::GarbageCollected<BraveSessionCache>,
      public blink::Supplement<blink::ExecutionContext> {
 public:
  static const char kSupplementName[];

  explicit BraveSessionCache(blink::ExecutionContext&);
  virtual ~BraveSessionCache() = default;

  static BraveSessionCache& From(blink::ExecutionContext&);

  AudioFarblingCallback GetAudioFarblingCallback(
      blink::WebContentSettingsClient* settings);
  void PerturbPixels(blink::WebContentSettingsClient* settings,
                     const unsigned char* data,
                     size_t size);
  WTF::String GenerateRandomString(std::string seed, wtf_size_t length);
  WTF::String FarbledUserAgent(WTF::String real_user_agent);
  std::mt19937_64 MakePseudoRandomGenerator();

 private:
  bool farbling_enabled_;
  uint64_t session_key_;
  uint8_t domain_key_[32];

  void PerturbPixelsInternal(const unsigned char* data, size_t size);
};

}  // namespace brave

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EXECUTION_CONTEXT_EXECUTION_CONTEXT_H_
