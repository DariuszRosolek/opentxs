# Copyright (c) 2010-2022 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

target_sources(
  opentxs-common
  PRIVATE
    "${opentxs_SOURCE_DIR}/src/internal/interface/ui/UI.hpp"
    "Combined.hpp"
    "Items.hpp"
    "List.hpp"
    "Row.hpp"
    "RowType.hpp"
    "Sort.cpp"
    "Widget.cpp"
    "Widget.hpp"
)
set(cxx-install-headers
    "${opentxs_SOURCE_DIR}/include/opentxs/interface/ui/List.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/interface/ui/ListRow.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/interface/ui/Widget.hpp"
)
target_sources(opentxs-common PRIVATE ${cxx-install-headers})

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/interface/ui
    COMPONENT dev
  )
endif()
