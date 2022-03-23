/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

//-------------------------------------------------------------------------------
// |OperationalPatterns| is a class for handling the collection of
// operational patterns, which are are anonymous, minimal representations of how
// users engage with the browser over a collection period. A collection period
// is divided into collection slots (i.e. 30m intervals). Two timers are
// instatiated at startup:
// 1. |collection_timer_| fires every |collection_slot_size_|/2
// minutes (at most twice per collection slot) and starts the next timer.
// 2. |mock_training_timer_| fires a set number of minutes after
// the |collection_slot_periodic_timer_|. When this timer fires, a ping is sent
// to the server.
//
// For more information see
// https://github.com/brave/brave-browser/wiki/Operational-Patterns
//
//-------------------------------------------------------------------------------

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_OPERATIONAL_PATTERNS_OPERATIONAL_PATTERNS_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_OPERATIONAL_PATTERNS_OPERATIONAL_PATTERNS_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"

class PrefRegistrySimple;
class PrefService;

namespace net {
class HttpResponseHeaders;
}

namespace network {

class SharedURLLoaderFactory;
class SimpleURLLoader;
struct ResourceRequest;

}  // namespace network

namespace brave_federated {

class OperationalPatterns final {
 public:
  OperationalPatterns(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~OperationalPatterns();
  OperationalPatterns(const OperationalPatterns&) = delete;
  OperationalPatterns& operator=(const OperationalPatterns&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  bool IsRunning();

  void Start();
  void Stop();

 private:
  void LoadPrefs();
  void SavePrefs();
  void ClearPrefs();

  void OnCollectionTimerFired();
  void OnMockTrainingTimerFired();

  void MaybeResetCollectionId();
  void ResetCollectionId();

  std::string BuildCollectionPingPayload() const;
  void SendCollectionPing();
  void OnCollectionPingSent(scoped_refptr<net::HttpResponseHeaders> headers);

  std::string BuildDeletePingPayload() const;
  void SendDeletePing();
  void OnDeletePingSent(scoped_refptr<net::HttpResponseHeaders> headers);

  raw_ptr<PrefService> pref_service_ = nullptr;  // NOT OWNED
  scoped_refptr<network::SharedURLLoaderFactory>
      url_loader_factory_;  // NOT OWNED

  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  std::unique_ptr<base::RepeatingTimer> collection_timer_;
  std::unique_ptr<base::RetainingOneShotTimer> mock_training_timer_;

  bool is_running_ = false;
  base::Time collection_id_expiration_time_;
  int current_collected_slot_ = 0;
  int last_checked_slot_ = 0;
  std::string collection_id_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_OPERATIONAL_PATTERNS_OPERATIONAL_PATTERNS_H_
