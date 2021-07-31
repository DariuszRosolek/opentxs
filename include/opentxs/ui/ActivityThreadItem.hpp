// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_ACTIVITYTHREADITEM_HPP
#define OPENTXS_UI_ACTIVITYTHREADITEM_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <chrono>
#include <cstdint>
#include <string>

#include "ListRow.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"

#ifdef SWIG
// clang-format off
%extend opentxs::ui::ActivityThreadItem {
    int Timestamp() const noexcept
    {
        return Clock::to_time_t($self->Timestamp());
    }
}
%ignore opentxs::ui::ActivityThreadItem::Timestamp;
%template(OTUIActivityThreadItem) opentxs::SharedPimpl<opentxs::ui::ActivityThreadItem>;
%rename(UIActivityThreadItem) opentxs::ui::ActivityThreadItem;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class ActivityThreadItem;
}  // namespace ui

using OTUIActivityThreadItem = SharedPimpl<ui::ActivityThreadItem>;
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class OPENTXS_EXPORT ActivityThreadItem : virtual public ListRow
{
public:
    virtual opentxs::Amount Amount() const noexcept = 0;
    virtual bool Deposit() const noexcept = 0;
    virtual std::string DisplayAmount() const noexcept = 0;
    virtual std::string From() const noexcept = 0;
    virtual bool Loading() const noexcept = 0;
    virtual bool MarkRead() const noexcept = 0;
    virtual std::string Memo() const noexcept = 0;
    virtual bool Outgoing() const noexcept = 0;
    virtual bool Pending() const noexcept = 0;
    virtual std::string Text() const noexcept = 0;
    virtual Time Timestamp() const noexcept = 0;
    virtual StorageBox Type() const noexcept = 0;

    ~ActivityThreadItem() override = default;

protected:
    ActivityThreadItem() noexcept = default;

private:
    ActivityThreadItem(const ActivityThreadItem&) = delete;
    ActivityThreadItem(ActivityThreadItem&&) = delete;
    ActivityThreadItem& operator=(const ActivityThreadItem&) = delete;
    ActivityThreadItem& operator=(ActivityThreadItem&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif
