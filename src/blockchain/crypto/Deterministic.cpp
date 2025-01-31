// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                         // IWYU pragma: associated
#include "1_Internal.hpp"                       // IWYU pragma: associated
#include "blockchain/crypto/Deterministic.hpp"  // IWYU pragma: associated

#include <boost/container/vector.hpp>
#include <robin_hood.h>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "Proto.hpp"
#include "blockchain/crypto/Element.hpp"
#include "blockchain/crypto/Subaccount.hpp"
#include "internal/api/crypto/Blockchain.hpp"
#include "internal/util/LogMacros.hpp"
#include "opentxs/api/crypto/Blockchain.hpp"
#include "opentxs/api/crypto/Seed.hpp"
#include "opentxs/api/session/Crypto.hpp"
#include "opentxs/api/session/Factory.hpp"
#include "opentxs/api/session/Session.hpp"
#include "opentxs/blockchain/crypto/Account.hpp"
#include "opentxs/blockchain/crypto/Element.hpp"
#include "opentxs/blockchain/crypto/Wallet.hpp"
#include "opentxs/crypto/key/EllipticCurve.hpp"
#include "opentxs/crypto/key/asymmetric/Role.hpp"
#include "opentxs/util/Log.hpp"
#include "opentxs/util/Pimpl.hpp"
#include "serialization/protobuf/HDPath.pb.h"

namespace opentxs::blockchain::crypto::implementation
{
Deterministic::Deterministic(
    const api::Session& api,
    const crypto::Account& parent,
    const SubaccountType type,
    OTIdentifier&& id,
    const proto::HDPath path,
    ChainData&& data,
    Identifier& out) noexcept
    : Subaccount(api, parent, type, std::move(id), out)
    , path_(path)
    , data_(std::move(data))
    , generated_({{data_.internal_.type_, 0}, {data_.external_.type_, 0}})
    , used_({{data_.internal_.type_, 0}, {data_.external_.type_, 0}})
    , last_allocation_()
    , cached_key_()
{
    data_.internal_.map_.reserve(1024u);
    data_.internal_.map_.reserve(1024u);
}

Deterministic::Deterministic(
    const api::Session& api,
    const crypto::Account& parent,
    const SubaccountType type,
    const SerializedType& serialized,
    const Bip32Index internal,
    const Bip32Index external,
    ChainData&& data,
    Identifier& out) noexcept(false)
    : Subaccount(api, parent, type, serialized.common(), out)
    , path_(serialized.path())
    , data_(std::move(data))
    , generated_(
          {{data_.internal_.type_, internal},
           {data_.external_.type_, external}})
    , used_(
          {{data_.internal_.type_, serialized.internalindex()},
           {data_.external_.type_, serialized.externalindex()}})
    , last_allocation_()
    , cached_key_()
{
}

Deterministic::ChainData::ChainData(
    const api::Session& api,
    Subchain internalType,
    bool internalContact,
    Subchain externalType,
    bool externalContact) noexcept
    : internal_(api, internalType, internalContact)
    , external_(api, externalType, externalContact)
{
}

auto Deterministic::ChainData::Get(Subchain type) noexcept(false)
    -> AddressData&
{
    if (type == internal_.type_) {

        return internal_;
    } else if (type == external_.type_) {

        return external_;
    } else {

        throw std::out_of_range("Invalid subchain");
    }
}

auto Deterministic::accept(
    const rLock& lock,
    const Subchain type,
    const Identifier& contact,
    const UnallocatedCString& label,
    const Time time,
    const Bip32Index index,
    Batch& generated,
    const PasswordPrompt& reason) const noexcept -> std::optional<Bip32Index>
{
    check_lookahead(lock, type, generated, reason);
    set_metadata(lock, type, index, contact, label);
    auto& element =
        const_cast<Deterministic&>(*this).element(lock, type, index);
    element.Internal().Reserve(time);
    LogTrace()(OT_PRETTY_CLASS())("Accepted index ")(index).Flush();

    return index;
}

auto Deterministic::AllowedSubchains() const noexcept
    -> UnallocatedSet<Subchain>
{
    return {data_.internal_.type_, data_.external_.type_};
}

auto Deterministic::BalanceElement(const Subchain type, const Bip32Index index)
    const noexcept(false) -> const crypto::Element&
{
    auto lock = rLock{lock_};

    return element(lock, type, index);
}

auto Deterministic::check(
    const rLock& lock,
    const Subchain type,
    const Identifier& contact,
    const UnallocatedCString& label,
    const Time time,
    const Bip32Index candidate,
    const PasswordPrompt& reason,
    Fallback& fallback,
    std::size_t& gap,
    Batch& generated) const noexcept(false) -> std::optional<Bip32Index>
{
    const auto accept = [&](Bip32Index index) {
        return this->accept(
            lock, type, contact, label, time, index, generated, reason);
    };

    if (is_generated(lock, type, candidate)) {
        LogTrace()(OT_PRETTY_CLASS())("Examining generated index ")(candidate)
            .Flush();
        const auto& element = this->element(lock, type, candidate);
        const auto status = element.Internal().IsAvailable(contact, label);

        switch (status) {
            case Status::NeverUsed: {
                LogTrace()(OT_PRETTY_CLASS())("index ")(
                    candidate)(" was never used")
                    .Flush();

                return accept(candidate);
            }
            case Status::Reissue: {
                LogTrace()(OT_PRETTY_CLASS())("Recycling unused index ")(
                    candidate)
                    .Flush();

                return accept(candidate);
            }
            case Status::Used: {
                LogTrace()(OT_PRETTY_CLASS())("index ")(
                    candidate)(" has confirmed transactions")
                    .Flush();
                gap = 0;

                throw std::runtime_error("Not acceptable");
            }
            case Status::MetadataConflict: {
                LogTrace()(OT_PRETTY_CLASS())("index ")(
                    candidate)(" can not be used")
                    .Flush();
                ++gap;

                throw std::runtime_error("Not acceptable");
            }
            case Status::Reserved: {
                LogTrace()(OT_PRETTY_CLASS())("index ")(
                    candidate)(" is reserved")
                    .Flush();
                ++gap;

                throw std::runtime_error("Not acceptable");
            }
            case Status::StaleUnconfirmed:
            default: {
                LogTrace()(OT_PRETTY_CLASS())("saving index ")(
                    candidate)(" as a fallback")
                    .Flush();
                fallback[status].emplace(candidate);
                ++gap;

                throw std::runtime_error("Not acceptable");
            }
        }
    } else {
        LogTrace()(OT_PRETTY_CLASS())("Generating index ")(candidate).Flush();
        const auto newIndex = generate(lock, type, candidate, reason);

        OT_ASSERT(newIndex == candidate);

        generated.emplace_back(newIndex);

        return accept(candidate);
    }
}

auto Deterministic::check_activity(
    const rLock& lock,
    const UnallocatedVector<Activity>& unspent,
    UnallocatedSet<OTIdentifier>& contacts,
    const PasswordPrompt& reason) const noexcept -> bool
{
    set_deterministic_contact(contacts);

    try {
        for (const auto& [coin, key, value] : unspent) {
            const auto& [account, subchain, index] = key;

            if (const auto& data = data_.external_; data.type_ == subchain) {

                if (data.set_contact_) {
                    extract_contacts(index, data.map_, contacts);
                }
            } else if (const auto& data = data_.internal_;
                       data.type_ == subchain) {

                if (data.set_contact_) {
                    extract_contacts(index, data.map_, contacts);
                }
            } else {

                return false;
            }
        }

        return true;
    } catch (...) {

        return false;
    }
}

void Deterministic::check_lookahead(
    const rLock& lock,
    Batch& generated,
    const PasswordPrompt& reason) const noexcept(false)
{
    check_lookahead(lock, data_.internal_.type_, generated, reason);
    check_lookahead(lock, data_.external_.type_, generated, reason);
}

auto Deterministic::check_lookahead(
    const rLock& lock,
    const Subchain type,
    Batch& generated,
    const PasswordPrompt& reason) const noexcept(false) -> void
{
    auto needed = need_lookahead(lock, type);

    while (0u < needed) {
        generated.emplace_back(generate_next(lock, type, reason));
        --needed;
    }
}

auto Deterministic::confirm(
    const rLock& lock,
    const Subchain type,
    const Bip32Index index) noexcept -> void
{
    try {
        auto& used = used_.at(type);

        if (index < used) { return; }

        static const auto blank = api_.Factory().Identifier();

        for (auto i{index}; i > used; --i) {
            const auto& element = this->element(lock, type, i - 1u);

            if (Status::Used != element.Internal().IsAvailable(blank, "")) {
                return;
            }
        }

        for (auto i{index}; i < generated_.at(type); ++i) {
            const auto& element = this->element(lock, type, i);

            if (Status::Used == element.Internal().IsAvailable(blank, "")) {
                used = std::max(used, i + 1u);
            } else {

                return;
            }
        }
    } catch (...) {
    }
}

auto Deterministic::element(
    const rLock&,
    const Subchain type,
    const Bip32Index index) noexcept(false) -> crypto::Element&
{
    if (auto& data = data_.internal_; data.type_ == type) {
        return *data.map_.at(index);
    } else if (auto& data = data_.external_; data.type_ == type) {
        return *data.map_.at(index);
    } else {
        throw std::out_of_range("Invalid subchain");
    }
}

auto Deterministic::extract_contacts(
    const Bip32Index index,
    const AddressMap& map,
    UnallocatedSet<OTIdentifier>& contacts) noexcept -> void
{
    try {
        auto contact = map.at(index)->Contact();

        if (false == contact->empty()) { contacts.emplace(std::move(contact)); }
    } catch (...) {
    }
}

auto Deterministic::finish_allocation(const rLock& lock, Batch& generated)
    const noexcept -> bool
{
#if OT_BLOCKCHAIN
    if (0u < generated.size()) {
        parent_.Parent().Parent().Internal().KeyGenerated(chain_);
    }
#endif  // OT_BLOCKCHAIN

    return save(lock);
}

auto Deterministic::Floor(const Subchain type) const noexcept
    -> std::optional<Bip32Index>
{
    auto lock = rLock{lock_};

    try {

        return used_.at(type);
    } catch (...) {

        return std::nullopt;
    }
}

auto Deterministic::GenerateNext(
    const Subchain type,
    const PasswordPrompt& reason) const noexcept -> std::optional<Bip32Index>
{
    auto lock = rLock{lock_};

    if (0 == generated_.count(type)) { return {}; }

    try {
        auto generated = Batch{};
        generated.emplace_back(generate_next(lock, type, reason));

        OT_ASSERT(0u < generated.size());

        if (finish_allocation(lock, generated)) {

            return generated.front();
        } else {

            return std::nullopt;
        }
    } catch (...) {

        return std::nullopt;
    }
}

auto Deterministic::generate(
    const rLock& lock,
    const Subchain type,
    const Bip32Index desired,
    const PasswordPrompt& reason) const noexcept(false) -> Bip32Index
{
    auto& index = generated_.at(type);

    OT_ASSERT(desired == index);

    if (max_index_ <= index) { throw std::runtime_error("Account is full"); }

    auto pKey = PrivateKey(type, index, reason);

    if (false == bool(pKey)) {
        throw std::runtime_error("Failed to generate key");
    }

    const auto& key = *pKey;
    auto& addressMap = data_.Get(type).map_;
    const auto& blockchain = parent_.Parent().Parent();
    const auto [it, added] = addressMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(index),
        std::forward_as_tuple(std::make_unique<implementation::Element>(
            api_, blockchain, *this, chain_, type, index, key, get_contact())));

    if (false == added) { throw std::runtime_error("Failed to add key"); }

    return index++;
}

auto Deterministic::generate_next(
    const rLock& lock,
    const Subchain type,
    const PasswordPrompt& reason) const noexcept(false) -> Bip32Index
{
    return generate(lock, type, generated_.at(type), reason);
}

auto Deterministic::get_contact() const noexcept -> OTIdentifier
{
    static const auto blank = api_.Factory().Identifier();

    return blank;
}

auto Deterministic::init(const PasswordPrompt& reason) noexcept(false) -> void
{
    auto lock = rLock{lock_};

    if (account_already_exists(lock)) {
        throw std::runtime_error("Account already exists");
    }

    auto generated = Batch{};
    check_lookahead(lock, generated, reason);

    if (false == finish_allocation(lock, generated)) {
        throw std::runtime_error("Failed to save new account");
    }

    init();
}

auto Deterministic::Key(const Subchain type, const Bip32Index index)
    const noexcept -> ECKey
{
    try {

        return data_.Get(type).map_.at(index)->Key();
    } catch (...) {

        return nullptr;
    }
}

auto Deterministic::LastGenerated(const Subchain type) const noexcept
    -> std::optional<Bip32Index>
{
    auto lock = rLock{lock_};

    try {
        auto output = generated_.at(type);

        return (0 == output) ? std::optional<Bip32Index>{} : output - 1;
    } catch (...) {

        return {};
    }
}

auto Deterministic::mutable_element(
    const rLock& lock,
    const Subchain type,
    const Bip32Index index) noexcept(false) -> crypto::Element&
{
    return *data_.Get(type).map_.at(index);
}

auto Deterministic::need_lookahead(const rLock& lock, const Subchain type)
    const noexcept -> Bip32Index
{
    const auto capacity = (generated_.at(type) - used_.at(type));

    if (capacity >= window_) { return 0u; }

    const auto effective = [&] {
        auto& last = last_allocation_[type];

        if (false == last.has_value()) {
            last = window_;

            return window_;
        } else {
            last = std::min(max_allocation_, last.value() * 2u);

            return last.value();
        }
    }();

    return effective - capacity;
}

auto Deterministic::Reserve(
    const Subchain type,
    const PasswordPrompt& reason,
    const Identifier& contact,
    const UnallocatedCString& label,
    const Time time) const noexcept -> std::optional<Bip32Index>
{
    auto batch = Reserve(type, 1u, reason, contact, label, time);

    if (0u == batch.size()) { return std::nullopt; }

    return batch.front();
}

auto Deterministic::Reserve(
    const Subchain type,
    const std::size_t batch,
    const PasswordPrompt& reason,
    const Identifier& contact,
    const UnallocatedCString& label,
    const Time time) const noexcept -> Batch
{
    auto output = Batch{};
    output.reserve(batch);
    auto lock = rLock{lock_};
    auto gen = Batch{};

    while (output.size() < batch) {
        auto out = use_next(lock, type, reason, contact, label, time, gen);

        if (false == out.has_value()) { break; }

        output.emplace_back(out.value());
    }

    if ((0u < output.size()) && (false == finish_allocation(lock, gen))) {

        return {};
    }

    return output;
}

auto Deterministic::RootNode(const PasswordPrompt& reason) const noexcept
    -> blockchain::crypto::HDKey
{
    auto& [mutex, key] = cached_key_;
    auto lock = Lock{mutex};

    if (key) { return key; }

    auto fingerprint(path_.root());
    auto path = UnallocatedVector<Bip32Index>{};

    for (const auto& child : path_.child()) { path.emplace_back(child); }

    key = api_.Crypto().Seed().GetHDKey(
        fingerprint,
        EcdsaCurve::secp256k1,
        path,
        opentxs::crypto::key::asymmetric::Role::Sign,
        opentxs::crypto::key::EllipticCurve::MaxVersion,
        reason);

    return key;
}

auto Deterministic::ScanProgress(Subchain type) const noexcept
    -> block::Position
{
    try {
        auto lock = rLock{lock_};

        return data_.Get(type).progress_;
    } catch (const std::exception& e) {
        LogError()(OT_PRETTY_CLASS())(e.what()).Flush();

        return Subaccount::ScanProgress(type);
    }
}

auto Deterministic::serialize_deterministic(
    const rLock& lock,
    SerializedType& out) const noexcept -> void
{
    out.set_version(BlockchainDeterministicAccountDataVersion);
    serialize_common(lock, *out.mutable_common());
    *out.mutable_path() = path_;
    out.set_internalindex(used_.at(data_.internal_.type_));
    out.set_externalindex(used_.at(data_.external_.type_));
}

auto Deterministic::set_metadata(
    const rLock& lock,
    const Subchain subchain,
    const Bip32Index index,
    const Identifier& contact,
    const UnallocatedCString& label) const noexcept -> void
{
    const auto blank = api_.Factory().Identifier();

    try {
        auto& data = data_.Get(subchain);
        const auto& id = data.set_contact_ ? contact : blank.get();
        data.map_.at(index)->Internal().SetMetadata(id, label);
    } catch (...) {
    }
}

auto Deterministic::SetScanProgress(
    const block::Position& progress,
    Subchain type) noexcept -> void
{
    try {
        auto lock = rLock{lock_};

        data_.Get(type).progress_ = progress;
    } catch (const std::exception& e) {
        LogError()(OT_PRETTY_CLASS())(e.what()).Flush();
    }
}

auto Deterministic::unconfirm(
    const rLock& lock,
    const Subchain type,
    const Bip32Index index) noexcept -> void
{
    try {
        auto& used = used_.at(type);
        const auto& element = this->element(lock, type, index);

        if (0 == element.Confirmed().size()) { used = std::min(used, index); }
    } catch (...) {
    }
}

auto Deterministic::use_next(
    const rLock& lock,
    const Subchain type,
    const PasswordPrompt& reason,
    const Identifier& contact,
    const UnallocatedCString& label,
    const Time time,
    Batch& generated) const noexcept -> std::optional<Bip32Index>
{
    try {
        auto gap = std::size_t{0};
        auto candidate = used_.at(type);
        auto fallback = Fallback{};

        while (gap < window_) {
            try {

                return check(
                    lock,
                    type,
                    contact,
                    label,
                    time,
                    candidate,
                    reason,
                    fallback,
                    gap,
                    generated);
            } catch (...) {
                ++candidate;

                continue;
            }
        }

        LogTrace()(OT_PRETTY_CLASS())(
            "Gap limit reached. Searching for acceptable fallback")
            .Flush();
        const auto accept = [&](Bip32Index index) {
            return this->accept(
                lock, type, contact, label, time, index, generated, reason);
        };

        if (auto& set = fallback[Status::StaleUnconfirmed]; 0 < set.size()) {
            LogTrace()(OT_PRETTY_CLASS())(
                "Recycling index with old never-confirmed transactions")
                .Flush();

            return accept(*set.cbegin());
        }

        LogTrace()(OT_PRETTY_CLASS())(
            "No acceptable fallback discovered. Generating past the gap "
            "limit")
            .Flush();

        while (candidate < max_index_) {
            try {

                return check(
                    lock,
                    type,
                    contact,
                    label,
                    time,
                    candidate,
                    reason,
                    fallback,
                    gap,
                    generated);
            } catch (...) {
                ++candidate;

                continue;
            }
        }

        return {};
    } catch (...) {

        return {};
    }
}
}  // namespace opentxs::blockchain::crypto::implementation
