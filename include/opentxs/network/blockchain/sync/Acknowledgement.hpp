// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <string>
#include <vector>

#include "opentxs/blockchain/Types.hpp"
#include "opentxs/network/blockchain/sync/Base.hpp"
#include "opentxs/network/blockchain/sync/State.hpp"
#include "opentxs/network/blockchain/sync/Types.hpp"

namespace opentxs
{
namespace network
{
namespace blockchain
{
namespace sync
{
class State;
}  // namespace sync
}  // namespace blockchain
}  // namespace network
}  // namespace opentxs

namespace opentxs::network::blockchain::sync
{
class OPENTXS_EXPORT Acknowledgement final : public Base
{
public:
    class Imp;

    auto Endpoint() const noexcept -> const std::string&;
    auto State() const noexcept -> const StateData&;
    /// throws std::out_of_range if specified chain is not present
    auto State(opentxs::blockchain::Type chain) const noexcept(false)
        -> const sync::State&;

    OPENTXS_NO_EXPORT Acknowledgement(Imp* imp) noexcept;

    ~Acknowledgement() final;

private:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow-field"
    Imp* imp_;
#pragma GCC diagnostic pop

    Acknowledgement(const Acknowledgement&) = delete;
    Acknowledgement(Acknowledgement&&) = delete;
    auto operator=(const Acknowledgement&) -> Acknowledgement& = delete;
    auto operator=(Acknowledgement&&) -> Acknowledgement& = delete;
};
}  // namespace opentxs::network::blockchain::sync
