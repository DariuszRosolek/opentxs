// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_include "opentxs/network/blockchain/sync/Block.hpp"
// IWYU pragma: no_include "opentxs/network/blockchain/sync/State.hpp"

#pragma once

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <string>
#include <vector>

namespace opentxs
{
namespace network
{
namespace blockchain
{
namespace sync
{
using TypeEnum = std::uint32_t;

enum class MessageType : TypeEnum;

class Block;
class State;

using StateData = std::vector<sync::State>;
using SyncData = std::vector<Block>;
}  // namespace sync
}  // namespace blockchain
}  // namespace network

OPENTXS_EXPORT auto print(network::blockchain::sync::MessageType in) noexcept
    -> std::string;

constexpr auto value(network::blockchain::sync::MessageType type) noexcept
{
    return static_cast<network::blockchain::sync::TypeEnum>(type);
}
}  // namespace opentxs
