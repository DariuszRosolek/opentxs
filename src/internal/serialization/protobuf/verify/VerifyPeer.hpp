// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "internal/serialization/protobuf/Basic.hpp"
#include "opentxs/Version.hpp"

namespace opentxs::proto
{
auto PeerObjectAllowedNym() noexcept -> const VersionMap&;
auto PeerObjectAllowedPeerReply() noexcept -> const VersionMap&;
auto PeerObjectAllowedPeerRequest() noexcept -> const VersionMap&;
auto PeerObjectAllowedPurse() noexcept -> const VersionMap&;
auto PeerReplyAllowedBailment() noexcept -> const VersionMap&;
auto PeerReplyAllowedConnectionInfo() noexcept -> const VersionMap&;
auto PeerReplyAllowedNotice() noexcept -> const VersionMap&;
auto PeerReplyAllowedOutBailment() noexcept -> const VersionMap&;
auto PeerReplyAllowedSignature() noexcept -> const VersionMap&;
auto PeerRequestAllowedBailment() noexcept -> const VersionMap&;
auto PeerRequestAllowedConnectionInfo() noexcept -> const VersionMap&;
auto PeerRequestAllowedFaucet() noexcept -> const VersionMap&;
auto PeerRequestAllowedOutBailment() noexcept -> const VersionMap&;
auto PeerRequestAllowedPendingBailment() noexcept -> const VersionMap&;
auto PeerRequestAllowedSignature() noexcept -> const VersionMap&;
auto PeerRequestAllowedStoreSecret() noexcept -> const VersionMap&;
auto PeerRequestAllowedVerificationOffer() noexcept -> const VersionMap&;
}  // namespace opentxs::proto
