// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include "opentxs/core/contract/peer/PeerReply.hpp"
#include "opentxs/util/SharedPimpl.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace contract
{
namespace peer
{
namespace reply
{
class Connection;
}  // namespace reply
}  // namespace peer
}  // namespace contract

using OTConnectionReply = SharedPimpl<contract::peer::reply::Connection>;
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::contract::peer::reply
{
class OPENTXS_EXPORT Connection : virtual public peer::Reply
{
public:
    ~Connection() override = default;

protected:
    Connection() noexcept = default;

private:
    friend OTConnectionReply;

#ifndef _WIN32
    auto clone() const noexcept -> Connection* override = 0;
#endif

    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    auto operator=(const Connection&) -> Connection& = delete;
    auto operator=(Connection&&) -> Connection& = delete;
};
}  // namespace opentxs::contract::peer::reply
