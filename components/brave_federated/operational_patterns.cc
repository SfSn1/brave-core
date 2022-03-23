/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "brave/components/brave_federated/operational_patterns.h"

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_federated/operational_patterns_util.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace {

static constexpr char federatedLearningUrl[] = "https://fl.brave.com/";

constexpr char kLastCheckedSlotPrefName[] = "brave.federated.last_checked_slot";
constexpr char kCollectionIdPrefName[] = "brave.federated.collection_id";
constexpr char kCollectionIdExpirationPrefName[] =
    "brave.federated.collection_id_expiration";

constexpr int kLastCheckedSlotInitialValue = -1;

constexpr int kMinutesBeforeRetry = 5;

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("operational_pattern", R"(
        semantics {
          sender: "Operational Patterns"
          description:
            "Report of anonymized engagement statistics. For more info see "
            "https://github.com/brave/brave-browser/wiki/Operational-Patterns"
          trigger:
            "Reports are automatically generated on startup and at intervals "
            "while Brave is running."
          data:
            "Anonymized and encrypted engagement data."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This service is enabled only when P3A is enabled."
          policy_exception_justification:
            "Not implemented."
        }
    )");
}

}  // anonymous namespace

namespace brave_federated {

OperationalPatterns::OperationalPatterns(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : pref_service_(pref_service), url_loader_factory_(url_loader_factory) {
  DCHECK(pref_service_);
  DCHECK(url_loader_factory_);
}

OperationalPatterns::~OperationalPatterns() {}

void OperationalPatterns::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kLastCheckedSlotPrefName,
                                kLastCheckedSlotInitialValue);
  registry->RegisterStringPref(kCollectionIdPrefName, {});
  registry->RegisterTimePref(kCollectionIdExpirationPrefName, base::Time());
}

bool OperationalPatterns::IsRunning() {
  return is_running_;
}

void OperationalPatterns::Start() {
  VLOG(1) << "Starting operational patterns";
  is_running_ = true;

  DCHECK(!mock_training_timer_);
  DCHECK(!collection_timer_);

  LoadPrefs();
  MaybeResetCollectionId();

  mock_training_timer_ = std::make_unique<base::RetainingOneShotTimer>();
  mock_training_timer_->Start(
      FROM_HERE,
      base::Seconds(brave_federated::features::
                        GetSimulateLocalTrainingStepDurationValue() *
                    60),
      this, &OperationalPatterns::OnMockTrainingTimerFired);

  collection_timer_ = std::make_unique<base::RepeatingTimer>();
  collection_timer_->Start(
      FROM_HERE,
      base::Seconds(brave_federated::features::GetCollectionSlotSizeValue() *
                    60 / 2),
      this, &OperationalPatterns::OnCollectionTimerFired);
}

void OperationalPatterns::Stop() {
  VLOG(1) << "Stopping operational patterns";
  is_running_ = false;

  mock_training_timer_.reset();
  collection_timer_.reset();

  SendDeletePing();
}

///////////////////////////////////////////////////////////////////////////////

void OperationalPatterns::LoadPrefs() {
  last_checked_slot_ = pref_service_->GetInteger(kLastCheckedSlotPrefName);
  collection_id_ = pref_service_->GetString(kCollectionIdPrefName);
  collection_id_expiration_time_ =
      pref_service_->GetTime(kCollectionIdExpirationPrefName);
}

void OperationalPatterns::SavePrefs() {
  pref_service_->SetInteger(kLastCheckedSlotPrefName, last_checked_slot_);
  pref_service_->SetString(kCollectionIdPrefName, collection_id_);
  pref_service_->SetTime(kCollectionIdExpirationPrefName,
                         collection_id_expiration_time_);
}

void OperationalPatterns::ClearPrefs() {
  pref_service_->ClearPref(kLastCheckedSlotPrefName);
  pref_service_->ClearPref(kCollectionIdPrefName);
  pref_service_->ClearPref(kCollectionIdExpirationPrefName);
}

void OperationalPatterns::OnCollectionTimerFired() {
  mock_training_timer_->Reset();
}

void OperationalPatterns::OnMockTrainingTimerFired() {
  SendCollectionPing();
}

void OperationalPatterns::MaybeResetCollectionId() {
  const base::Time now = base::Time::Now();
  if (collection_id_.empty() || (!collection_id_expiration_time_.is_null() &&
                                 now > collection_id_expiration_time_)) {
    ResetCollectionId();
  }
}

void OperationalPatterns::ResetCollectionId() {
  VLOG(1) << "Resetting collection ID";

  const base::Time now = base::Time::Now();
  collection_id_ =
      base::ToUpperASCII(base::UnguessableToken::Create().ToString());
  collection_id_expiration_time_ =
      now + base::Seconds(brave_federated::features::GetCollectionIdLifetime() *
                          24 * 60 * 60);
  SavePrefs();
}

// Collection Ping ------------------------------------------------------------

std::string OperationalPatterns::BuildCollectionPingPayload() const {
  base::Value root(base::Value::Type::DICTIONARY);

  root.SetKey("collection_id", base::Value(collection_id_));
  root.SetKey("platform", base::Value(brave_stats::GetPlatformIdentifier()));
  root.SetKey("collection_slot", base::Value(current_collected_slot_));
  root.SetKey("wiki-link", base::Value("https://github.com/brave/brave-browser/"
                                       "wiki/Operational-Patterns"));

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
}

void OperationalPatterns::SendCollectionPing() {
  VLOG(1) << "Sending collection ping";

  current_collected_slot_ = GetCurrentCollectionSlot();
  if (current_collected_slot_ == last_checked_slot_) {
    return;
  }

  MaybeResetCollectionId();

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(federatedLearningUrl);
  resource_request->headers.SetHeader("X-Brave-FL-Operational-Patterns", "?1");
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->method = "POST";

  VLOG(2) << resource_request->method << " " << resource_request->url;

  const std::string payload = BuildCollectionPingPayload();
  VLOG(2) << "Payload " << payload;

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(payload, "application/json");
  url_loader_->DownloadHeadersOnly(
      url_loader_factory_.get(),
      base::BindOnce(&OperationalPatterns::OnCollectionPingSent,
                     base::Unretained(this)));
}

void OperationalPatterns::OnCollectionPingSent(
    scoped_refptr<net::HttpResponseHeaders> headers) {
  if (!headers) {
    VLOG(1) << "Failed to upload collection ping";
    return;
  }

  int response_code = headers->response_code();
  if (response_code == net::HTTP_OK) {
    VLOG(1) << "Successfully uploaded collection ping";

    last_checked_slot_ = current_collected_slot_;
    SavePrefs();
  } else {
    VLOG(1) << "Failed to send collection ping with HTTP " << response_code;
  }
}

// Delete Ping ---------------------------------------------------------------

std::string OperationalPatterns::BuildDeletePingPayload() const {
  base::Value root(base::Value::Type::DICTIONARY);

  root.SetKey("collection_id", base::Value(collection_id_));
  root.SetKey("wiki-link", base::Value("https://github.com/brave/brave-browser/"
                                       "wiki/Operational-Patterns"));

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
}

void OperationalPatterns::SendDeletePing() {
  VLOG(1) << "Sending delete ping";

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(federatedLearningUrl);
  resource_request->headers.SetHeader("X-Brave-FL-Operational-Patterns", "?1");
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->method = "DELETE";

  VLOG(2) << resource_request->method << " " << resource_request->url;

  const std::string payload = BuildDeletePingPayload();
  VLOG(2) << "Payload " << payload;

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(payload, "application/json");
  url_loader_->DownloadHeadersOnly(
      url_loader_factory_.get(),
      base::BindOnce(&OperationalPatterns::OnDeletePingSent,
                     base::Unretained(this)));
}

void OperationalPatterns::OnDeletePingSent(
    scoped_refptr<net::HttpResponseHeaders> headers) {
  if (!headers) {
    VLOG(1) << "Failed to send delete";
  }

  int response_code = headers->response_code();
  if (response_code == net::HTTP_OK) {
    VLOG(1) << "Successfully sent delete";

    ClearPrefs();
  } else {
    VLOG(1) << "Failed to send delete with HTTP " << response_code;

    auto retry_timer = std::make_unique<base::RetainingOneShotTimer>();
    retry_timer->Start(FROM_HERE, base::Seconds(kMinutesBeforeRetry * 60), this,
                       &OperationalPatterns::SendDeletePing);

    VLOG(1) << "Retry in " << kMinutesBeforeRetry << " minutes";
    // TODO(Moritz Haller): Delete retry timer in case collection ping was sent
  }
}

}  // namespace brave_federated
