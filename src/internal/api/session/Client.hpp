// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "internal/api/session/Session.hpp"
#include "opentxs/api/session/Client.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace identifier
{
class Nym;
class Notary;
}  // namespace identifier

namespace otx
{
namespace client
{
class Pair;
class ServerAction;
}  // namespace client
}  // namespace otx

class OT_API;
class OTAPI_Exec;
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::api::session::internal
{
class Client : virtual public session::Client, virtual public Session
{
public:
    virtual auto Exec(const UnallocatedCString& wallet = "") const
        -> const OTAPI_Exec& = 0;
    auto InternalClient() const noexcept -> const internal::Client& final
    {
        return *this;
    }
    using Session::Lock;
    virtual auto Lock(
        const identifier::Nym& nymID,
        const identifier::Notary& serverID) const -> std::recursive_mutex& = 0;
    virtual auto OTAPI(const UnallocatedCString& wallet = "") const
        -> const OT_API& = 0;
    virtual auto Pair() const -> const otx::client::Pair& = 0;
    virtual auto ServerAction() const -> const otx::client::ServerAction& = 0;

    virtual auto Init() -> void = 0;
    auto InternalClient() noexcept -> internal::Client& final { return *this; }

    ~Client() override = default;
};
}  // namespace opentxs::api::session::internal
