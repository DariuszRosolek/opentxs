// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                   // IWYU pragma: associated
#include "1_Internal.hpp"                 // IWYU pragma: associated
#include "network/p2p/server/Server.hpp"  // IWYU pragma: associated

#include <atomic>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "internal/api/network/Blockchain.hpp"
#include "internal/api/session/Endpoints.hpp"
#include "internal/network/p2p/Factory.hpp"
#include "internal/network/zeromq/Batch.hpp"
#include "internal/network/zeromq/Context.hpp"
#include "internal/network/zeromq/Thread.hpp"
#include "internal/network/zeromq/Types.hpp"
#include "internal/network/zeromq/socket/Raw.hpp"
#include "internal/util/LogMacros.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/network/Blockchain.hpp"
#include "opentxs/api/network/Network.hpp"
#include "opentxs/api/session/Endpoints.hpp"
#include "opentxs/api/session/Factory.hpp"
#include "opentxs/api/session/Session.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/network/p2p/Acknowledgement.hpp"
#include "opentxs/network/p2p/Base.hpp"
#include "opentxs/network/p2p/MessageType.hpp"
#include "opentxs/network/p2p/Request.hpp"
#include "opentxs/network/p2p/State.hpp"
#include "opentxs/network/p2p/Types.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/network/zeromq/ZeroMQ.hpp"
#include "opentxs/network/zeromq/message/FrameSection.hpp"
#include "opentxs/network/zeromq/message/Message.hpp"
#include "opentxs/network/zeromq/socket/SocketType.hpp"
#include "opentxs/util/Container.hpp"
#include "opentxs/util/Log.hpp"

namespace opentxs::network::p2p
{
Server::Imp::Imp(const api::Session& api, const zeromq::Context& zmq) noexcept
    : api_(api)
    , zmq_(zmq)
    , batch_(zmq_.Internal().MakeBatch([&] {
        auto out = UnallocatedVector<zeromq::socket::Type>{};
        out.emplace_back(zeromq::socket::Type::Router);
        out.emplace_back(zeromq::socket::Type::Publish);
        out.emplace_back(zeromq::socket::Type::Pair);
        const auto chains = opentxs::blockchain::DefinedChains().size();
        out.insert(out.end(), chains, zeromq::socket::Type::Pair);

        return out;
    }()))
    , external_callback_(
          batch_.listen_callbacks_.emplace_back(zeromq::ListenCallback::Factory(
              [this](auto&& msg) { process_external(std::move(msg)); })))
    , internal_callback_(
          batch_.listen_callbacks_.emplace_back(zeromq::ListenCallback::Factory(
              [this](auto&& msg) { process_internal(std::move(msg)); })))
    , sync_(batch_.sockets_.at(0))
    , update_(batch_.sockets_.at(1))
    , wallet_(batch_.sockets_.at(2))
    , map_lock_()
    , map_([&] {
        auto output = Map{};

        OT_ASSERT(
            batch_.sockets_.size() ==
            opentxs::blockchain::DefinedChains().size() + 3u);

        auto it = std::next(batch_.sockets_.begin(), 2);

        for (const auto chain : opentxs::blockchain::DefinedChains()) {
            auto& [endpoint, enabled, socket] =
                output
                    .emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple(chain),
                        std::forward_as_tuple(
                            zeromq::MakeArbitraryInproc(), false, *(++it)))
                    .first->second;
            socket.get().Bind(endpoint.c_str());
            LogTrace()(OT_PRETTY_CLASS())("socket ")(socket.get().ID())(
                " for ")(DisplayString(chain))(" bound to ")(endpoint)
                .Flush();
        }

        return output;
    }())
    , thread_(zmq_.Internal().Start(
          batch_.id_,
          [&] {
              auto out = zeromq::StartArgs{};
              out.reserve(map_.size() + 1u);
              out.emplace_back(
                  sync_.ID(), &sync_, [&cb = external_callback_](auto&& m) {
                      cb.Process(std::move(m));
                  });
              out.emplace_back(
                  wallet_.ID(), &wallet_, [&socket = sync_](auto&& m) {
                      socket.Send(std::move(m));
                  });

              for (auto& [chain, data] : map_) {
                  auto& [endpoint, enabled, wrapper] = data;
                  auto& socket = wrapper.get();

                  out.emplace_back(
                      socket.ID(),
                      &socket,
                      [&cb = internal_callback_](auto&& m) {
                          cb.Process(std::move(m));
                      });
              }

              return out;
          }()))
    , sync_endpoint_()
    , sync_public_endpoint_()
    , update_endpoint_()
    , update_public_endpoint_()
    , started_(false)
    , running_(true)
    , gate_()
{
}

auto Server::Imp::process_external(zeromq::Message&& incoming) noexcept -> void
{
#if OT_BLOCKCHAIN
    try {
        const auto base = api_.Factory().BlockchainSyncMessage(incoming);

        if (!base) {
            throw std::runtime_error{"failed to instantiate message"};
        }

        using Type = opentxs::network::p2p::MessageType;
        const auto type = base->Type();

        switch (type) {
            case Type::query:
            case Type::sync_request: {
                process_sync(std::move(incoming), *base);
            } break;
            case Type::publish_contract:
            case Type::contract_query: {
                wallet_.Send(std::move(incoming));
            } break;
            default: {
                throw std::runtime_error{
                    UnallocatedCString{"Unsupported message type "} +
                    opentxs::print(type)};
            }
        }
    } catch (const std::exception& e) {
        LogError()(OT_PRETTY_CLASS())(e.what()).Flush();
    }
#endif  // OT_BLOCKCHAIN
}

auto Server::Imp::process_internal(zeromq::Message&& incoming) noexcept -> void
{
    const auto hSize = incoming.Header().size();
    const auto bSize = incoming.Body().size();

    if ((0u == hSize) && (0u == bSize)) { return; }

    if (0u < hSize) {
        LogTrace()(OT_PRETTY_CLASS())("transmitting sync reply").Flush();
        sync_.Send(std::move(incoming));
    } else {
        LogTrace()(OT_PRETTY_CLASS())("broadcasting push notification").Flush();
        update_.Send(std::move(incoming));
    }
}

auto Server::Imp::process_sync(
    zeromq::Message&& incoming,
    const p2p::Base& base) noexcept -> void
{
    try {
        {
            const auto ack = [&] {
                auto lock = Lock{map_lock_};

                return factory::BlockchainSyncAcknowledgement(
                    api_.Network().Blockchain().Internal().Hello(),
                    update_public_endpoint_);
            }();
            auto msg = zeromq::reply_to_message(incoming);

            if (ack.Serialize(msg)) { sync_.Send(std::move(msg)); }
        }

        namespace bcsync = opentxs::network::p2p;
        const auto type = base.Type();

        if (bcsync::MessageType::sync_request == type) {
            const auto& request = base.asRequest();

            for (const auto& state : request.State()) {
                try {
                    auto lock = Lock{map_lock_};
                    const auto& [endpoint, enabled, socket] =
                        map_.at(state.Chain());

                    if (enabled) { socket.get().Send(std::move(incoming)); }
                } catch (...) {
                }
            }
        }
    } catch (const std::exception& e) {
        LogError()(OT_PRETTY_CLASS())(e.what()).Flush();
    }
}

Server::Imp::~Imp()
{
    gate_.shutdown();

    if (auto running = running_.exchange(false); running) {
        batch_.ClearCallbacks();
    }

    zmq_.Internal().Stop(batch_.id_);
}
}  // namespace opentxs::network::p2p

namespace opentxs::network::p2p
{
Server::Server(const api::Session& api, const zeromq::Context& zmq) noexcept
    : imp_(std::make_unique<Imp>(api, zmq).release())
{
}

auto Server::Disable(const Chain chain) noexcept -> void
{
    try {
        auto lock = Lock{imp_->map_lock_};
        std::get<1>(imp_->map_.at(chain)) = false;
    } catch (...) {
    }
}

auto Server::Enable(const Chain chain) noexcept -> void
{
    try {
        auto lock = Lock{imp_->map_lock_};
        std::get<1>(imp_->map_.at(chain)) = true;
    } catch (...) {
    }
}

auto Server::Endpoint(const Chain chain) const noexcept -> std::string_view
{
    try {

        return std::get<0>(imp_->map_.at(chain));
    } catch (...) {

        return {};
    }
}

auto Server::Start(
    const std::string_view sync,
    const std::string_view publicSync,
    const std::string_view update,
    const std::string_view publicUpdate) noexcept -> bool
{
    if (sync.empty() || update.empty()) {
        LogError()(OT_PRETTY_CLASS())("Invalid endpoint").Flush();

        return false;
    }

    const auto shutdown = imp_->gate_.get();

    if (shutdown) { return false; }

    if (auto started = imp_->started_.exchange(true); started) {
        LogError()(OT_PRETTY_CLASS())("Already running").Flush();

        return false;
    } else {
        auto lock = Lock{imp_->map_lock_};
        imp_->sync_endpoint_ = sync;
        imp_->sync_public_endpoint_ = publicSync;
        imp_->update_endpoint_ = update;
        imp_->update_public_endpoint_ = publicUpdate;
    }

    imp_->thread_->Modify(imp_->sync_.ID(), [this](auto& socket) {
        const auto& endpointPublic = imp_->sync_public_endpoint_;
        const auto& endpointLocal = imp_->sync_endpoint_;
        const auto& endpointPublish = imp_->update_endpoint_;
        const auto walletEndpoint =
            CString{imp_->api_.Endpoints().Internal().P2PWallet()};

        if (socket.SetRoutingID(endpointPublic)) {
            LogTrace()("Sync socket identity set to public endpoint: ")(
                endpointPublic)
                .Flush();
        } else {
            LogError()(OT_PRETTY_CLASS())("failed to set sync socket identity")
                .Flush();
        }

        if (socket.Bind(endpointLocal.c_str())) {
            LogConsole()("Blockchain sync server listener bound to ")(
                endpointLocal)
                .Flush();
        } else {
            LogError()(OT_PRETTY_CLASS())("failed to bind sync endpoint to ")(
                endpointLocal)
                .Flush();
        }

        if (imp_->update_.Bind(endpointPublish.c_str())) {
            LogConsole()("Blockchain sync publisher listener bound to ")(
                endpointPublish)
                .Flush();
        } else {
            LogError()(OT_PRETTY_CLASS())("failed to bind sync publisher to ")(
                endpointPublish)
                .Flush();
        }

        if (imp_->wallet_.Connect(walletEndpoint.c_str())) {
            LogTrace()("Wallet socket connected to ")(walletEndpoint).Flush();
        } else {
            LogError()(OT_PRETTY_CLASS())(
                "failed to connect wallet socket to ")(walletEndpoint)
                .Flush();
        }
    });

    return true;
}

Server::~Server()
{
    if (nullptr != imp_) {
        delete imp_;
        imp_ = nullptr;
    }
}
}  // namespace opentxs::network::p2p
