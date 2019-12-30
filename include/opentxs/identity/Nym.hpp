// Copyright (c) 2010-2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_IDENTITY_NYM_HPP
#define OPENTXS_IDENTITY_NYM_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/iterator/Bidirectional.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <tuple>

namespace opentxs
{
namespace identity
{
class Nym
{
public:
    using KeyTypes = std::vector<proto::AsymmetricKeyType>;
    using AuthorityKeys = std::pair<OTIdentifier, KeyTypes>;
    using NymKeys = std::pair<OTNymID, std::vector<AuthorityKeys>>;
    using Serialized = proto::Nym;
    using key_type = Identifier;
    using value_type = Authority;
    using const_iterator =
        opentxs::iterator::Bidirectional<const Nym, const value_type>;

    OPENTXS_EXPORT static const VersionNumber DefaultVersion;
    OPENTXS_EXPORT static const VersionNumber MaxVersion;

    OPENTXS_EXPORT virtual std::string Alias() const = 0;
    OPENTXS_EXPORT virtual const Serialized asPublicNym() const = 0;
    OPENTXS_EXPORT virtual const value_type& at(const key_type& id) const
        noexcept(false) = 0;
    OPENTXS_EXPORT virtual const value_type& at(const std::size_t& index) const
        noexcept(false) = 0;
    OPENTXS_EXPORT virtual const_iterator begin() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string BestEmail() const = 0;
    OPENTXS_EXPORT virtual std::string BestPhoneNumber() const = 0;
    OPENTXS_EXPORT virtual std::string BestSocialMediaProfile(
        const proto::ContactItemType type) const = 0;
    OPENTXS_EXPORT virtual const_iterator cbegin() const noexcept = 0;
    OPENTXS_EXPORT virtual const_iterator cend() const noexcept = 0;
    OPENTXS_EXPORT virtual const opentxs::ContactData& Claims() const = 0;
    OPENTXS_EXPORT virtual bool CompareID(const Nym& RHS) const = 0;
    OPENTXS_EXPORT virtual bool CompareID(const identifier::Nym& rhs) const = 0;
    OPENTXS_EXPORT virtual VersionNumber ContactCredentialVersion() const = 0;
    OPENTXS_EXPORT virtual VersionNumber ContactDataVersion() const = 0;
    OPENTXS_EXPORT virtual std::set<OTIdentifier> Contracts(
        const proto::ContactItemType currency,
        const bool onlyActive) const = 0;
    OPENTXS_EXPORT virtual std::string EmailAddresses(
        bool active = true) const = 0;
    OPENTXS_EXPORT virtual NymKeys EncryptionTargets() const noexcept = 0;
    OPENTXS_EXPORT virtual const_iterator end() const noexcept = 0;
    OPENTXS_EXPORT virtual void GetIdentifier(
        identifier::Nym& theIdentifier) const = 0;
    OPENTXS_EXPORT virtual void GetIdentifier(String& theIdentifier) const = 0;
    OPENTXS_EXPORT virtual const crypto::key::Asymmetric& GetPrivateAuthKey(
        proto::AsymmetricKeyType keytype = proto::AKEYTYPE_NULL) const = 0;
    OPENTXS_EXPORT virtual const crypto::key::Asymmetric& GetPrivateEncrKey(
        proto::AsymmetricKeyType keytype = proto::AKEYTYPE_NULL) const = 0;
    OPENTXS_EXPORT virtual const crypto::key::Asymmetric& GetPrivateSignKey(
        proto::AsymmetricKeyType keytype = proto::AKEYTYPE_NULL) const = 0;

    OPENTXS_EXPORT virtual const crypto::key::Asymmetric& GetPublicAuthKey(
        proto::AsymmetricKeyType keytype = proto::AKEYTYPE_NULL) const = 0;
    OPENTXS_EXPORT virtual const crypto::key::Asymmetric& GetPublicEncrKey(
        proto::AsymmetricKeyType keytype = proto::AKEYTYPE_NULL) const = 0;
    // OT uses the signature's metadata to narrow down its search for the
    // correct public key.
    // 'S' (signing key) or
    // 'E' (encryption key) OR
    // 'A' (authentication key)
    OPENTXS_EXPORT virtual std::int32_t GetPublicKeysBySignature(
        crypto::key::Keypair::Keys& listOutput,
        const Signature& theSignature,
        char cKeyType = '0') const = 0;
    OPENTXS_EXPORT virtual const crypto::key::Asymmetric& GetPublicSignKey(
        proto::AsymmetricKeyType keytype = proto::AKEYTYPE_NULL) const = 0;
    OPENTXS_EXPORT virtual bool HasCapability(
        const NymCapability& capability) const = 0;
    OPENTXS_EXPORT virtual const identifier::Nym& ID() const = 0;

    OPENTXS_EXPORT virtual std::string Name() const = 0;

    OPENTXS_EXPORT virtual bool Path(proto::HDPath& output) const = 0;
    OPENTXS_EXPORT virtual std::string PaymentCode() const = 0;
    OPENTXS_EXPORT virtual std::string PhoneNumbers(
        bool active = true) const = 0;
    OPENTXS_EXPORT virtual std::uint64_t Revision() const = 0;

    OPENTXS_EXPORT virtual void SerializeNymIDSource(Tag& parent) const = 0;
    OPENTXS_EXPORT virtual bool Sign(
        const ProtobufType& input,
        const proto::SignatureRole role,
        proto::Signature& signature,
        const PasswordPrompt& reason,
        const proto::HashType hash = proto::HASHTYPE_ERROR) const = 0;
    OPENTXS_EXPORT virtual std::size_t size() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string SocialMediaProfiles(
        const proto::ContactItemType type,
        bool active = true) const = 0;
    OPENTXS_EXPORT virtual const std::set<proto::ContactItemType>
    SocialMediaProfileTypes() const = 0;
    OPENTXS_EXPORT virtual const identity::Source& Source() const = 0;
    OPENTXS_EXPORT virtual std::unique_ptr<OTPassword> TransportKey(
        Data& pubkey,
        const PasswordPrompt& reason) const = 0;

    OPENTXS_EXPORT virtual bool Unlock(
        const crypto::key::Asymmetric& dhKey,
        const std::uint32_t tag,
        const proto::AsymmetricKeyType type,
        const crypto::key::Symmetric& key,
        PasswordPrompt& reason) const noexcept = 0;
    OPENTXS_EXPORT virtual bool Verify(
        const ProtobufType& input,
        proto::Signature& signature) const = 0;
    OPENTXS_EXPORT virtual bool VerifyPseudonym() const = 0;

    OPENTXS_EXPORT virtual std::string AddChildKeyCredential(
        const Identifier& strMasterID,
        const NymParameters& nymParameters,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool AddClaim(
        const Claim& claim,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool AddContract(
        const identifier::UnitDefinition& instrumentDefinitionID,
        const proto::ContactItemType currency,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active = true) = 0;
    OPENTXS_EXPORT virtual bool AddEmail(
        const std::string& value,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active) = 0;
    OPENTXS_EXPORT virtual bool AddPaymentCode(
        const opentxs::PaymentCode& code,
        const proto::ContactItemType currency,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active = true) = 0;
    OPENTXS_EXPORT virtual bool AddPhoneNumber(
        const std::string& value,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active) = 0;
    OPENTXS_EXPORT virtual bool AddPreferredOTServer(
        const Identifier& id,
        const PasswordPrompt& reason,
        const bool primary) = 0;
    OPENTXS_EXPORT virtual bool AddSocialMediaProfile(
        const std::string& value,
        const proto::ContactItemType type,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active) = 0;
    OPENTXS_EXPORT virtual bool DeleteClaim(
        const Identifier& id,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool SetCommonName(
        const std::string& name,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool SetContactData(
        const proto::ContactData& data,
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool SetScope(
        const proto::ContactItemType type,
        const std::string& name,
        const PasswordPrompt& reason,
        const bool primary) = 0;

    OPENTXS_EXPORT virtual ~Nym() = default;

protected:
    Nym() noexcept = default;

private:
    Nym(const Nym&) = delete;
    Nym(Nym&&) = delete;
    Nym& operator=(const Nym&) = delete;
    Nym& operator=(Nym&&) = delete;
};
}  // namespace identity
}  // namespace opentxs
#endif
