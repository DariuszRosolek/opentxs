// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                    // IWYU pragma: associated
#include "1_Internal.hpp"                  // IWYU pragma: associated
#include "internal/network/asio/HTTP.hpp"  // IWYU pragma: associated

#include <chrono>
#include <exception>
#include <future>
#include <utility>

#include "api/network/asio/Context.hpp"
#include "network/asio/WebRequest.tpp"

namespace opentxs::network::asio
{
HTTP::HTTP(
    const std::string_view hostname,
    const std::string_view file,
    api::network::asio::Context& asio,
    Finish&& cb,
    allocator_type alloc) noexcept
    : WebRequest(hostname, file, asio, std::move(cb), std::move(alloc))
    , stream_(std::nullopt)
{
}

auto HTTP::get_stream() noexcept(false) -> Stream&
{
    if (false == stream_.has_value()) {
        auto& stream = stream_.emplace(asio_.get());
        static constexpr auto timeout = std::chrono::seconds{10};
        stream.expires_after(timeout);
    }

    return stream_.value();
}

auto HTTP::Start() noexcept -> void
{
    try {
        resolve("http");
    } catch (...) {
        promise_.set_exception(std::current_exception());

        return;
    }
}

HTTP::~HTTP() = default;
}  // namespace opentxs::network::asio
