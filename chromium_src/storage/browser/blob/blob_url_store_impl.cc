// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_url_store_impl.h"

#include "base/strings/strcat.h"
#include "net/base/features.h"

#define BlobURLStoreImpl BlobURLStoreImpl_ChromiumImpl
#define BRAVE_BLOB_URL_STORE_IMPL_BLOB_URL_IS_VALID                           \
  if (!valid_origin && origin_.opaque() &&                                    \
      origin_.GetTupleOrPrecursorTupleIfOpaque().IsValid() &&                 \
      base::FeatureList::IsEnabled(                                           \
          net::features::kBravePartitionBlobStorage)) {                       \
    const auto& origin_scheme_host_port =                                     \
        origin_.GetTupleOrPrecursorTupleIfOpaque();                           \
    const url::Origin non_opaque_origin =                                     \
        url::Origin::CreateFromNormalizedTuple(                               \
            origin_scheme_host_port.scheme(), origin_scheme_host_port.host(), \
            origin_scheme_host_port.port());                                  \
    valid_origin = non_opaque_origin == url_origin;                           \
  }

#include "src/storage/browser/blob/blob_url_store_impl.cc"

#undef BRAVE_BLOB_URL_STORE_IMPL_BLOB_URL_IS_VALID
#undef BlobURLStoreImpl

namespace storage {

BlobURLStoreImpl::BlobURLStoreImpl(const url::Origin& origin,
                                   base::WeakPtr<BlobUrlRegistry> registry)
    : BlobURLStoreImpl_ChromiumImpl::BlobURLStoreImpl_ChromiumImpl(
          origin,
          std::move(registry)) {}

void BlobURLStoreImpl::Register(
    mojo::PendingRemote<blink::mojom::Blob> blob,
    const GURL& url,
    const base::UnguessableToken& unsafe_agent_cluster_id,
    const absl::optional<net::SchemefulSite>& unsafe_top_level_site,
    RegisterCallback callback) {
  BlobURLStoreImpl_ChromiumImpl::Register(
      std::move(blob), GetEphemeralOrOriginalUrl(url), unsafe_agent_cluster_id,
      unsafe_top_level_site, std::move(callback));
}

void BlobURLStoreImpl::Revoke(const GURL& url) {
  BlobURLStoreImpl_ChromiumImpl::Revoke(GetEphemeralOrOriginalUrl(url));
}

void BlobURLStoreImpl::Resolve(const GURL& url, ResolveCallback callback) {
  BlobURLStoreImpl_ChromiumImpl::Resolve(GetEphemeralOrOriginalUrl(url),
                                         std::move(callback));
}

void BlobURLStoreImpl::ResolveAsURLLoaderFactory(
    const GURL& url,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    ResolveAsURLLoaderFactoryCallback callback) {
  if (!registry_) {
    BlobURLLoaderFactory::Create(mojo::NullRemote(), url, std::move(receiver));
    std::move(callback).Run(absl::nullopt, absl::nullopt);
    return;
  }

  // Use modified URL only for accessing blob registry. Original URL is passed
  // as is into BlobURLLoaderFactory.
  const GURL& ephemeral_url = GetEphemeralOrOriginalUrl(url);
  BlobURLLoaderFactory::Create(registry_->GetBlobFromUrl(ephemeral_url), url,
                               std::move(receiver));
  std::move(callback).Run(registry_->GetUnsafeAgentClusterID(ephemeral_url),
                          registry_->GetUnsafeTopLevelSite(ephemeral_url));
}

void BlobURLStoreImpl::ResolveForNavigation(
    const GURL& url,
    mojo::PendingReceiver<blink::mojom::BlobURLToken> token,
    ResolveForNavigationCallback callback) {
  BlobURLStoreImpl_ChromiumImpl::ResolveForNavigation(
      GetEphemeralOrOriginalUrl(url), std::move(token), std::move(callback));
}

GURL BlobURLStoreImpl::GetEphemeralOrOriginalUrl(const GURL& url) const {
  if (origin_.opaque() &&
      origin_.GetTupleOrPrecursorTupleIfOpaque().IsValid() &&
      base::FeatureList::IsEnabled(net::features::kBravePartitionBlobStorage)) {
    // Use origin nonce as a partition key and append it to the URL path.
    GURL clean_url = BlobUrlUtils::ClearUrlFragment(url);
    std::string partitioned_path =
        base::StrCat({clean_url.path_piece(), "_",
                      origin_.GetNonceForEphemeralStorageKeying().ToString()});
    GURL::Replacements replacements;
    replacements.SetPathStr(partitioned_path);
    return clean_url.ReplaceComponents(replacements);
  }

  return url;
}

}  // namespace storage
