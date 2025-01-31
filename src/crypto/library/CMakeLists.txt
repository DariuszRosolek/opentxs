# Copyright (c) 2010-2022 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(opentxs-crypto-library OBJECT "")

add_subdirectory(openssl)

if(PACKETCRYPT_EXPORT)
  add_subdirectory(packetcrypt)
endif()

add_subdirectory(secp256k1)
add_subdirectory(sodium)

target_sources(
  opentxs-common
  PRIVATE
    "${opentxs_SOURCE_DIR}/src/internal/crypto/library/Factory.hpp"
    "${opentxs_SOURCE_DIR}/src/internal/crypto/library/Null.hpp"
    "${opentxs_SOURCE_DIR}/src/internal/crypto/library/Scrypt.hpp"
    "AsymmetricProvider.cpp"
    "AsymmetricProvider.hpp"
    "EcdsaProvider.cpp"
    "EcdsaProvider.hpp"
    "HashingProvider.cpp"
)
set(cxx-install-headers
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/library/AsymmetricProvider.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/library/EcdsaProvider.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/library/EncodingProvider.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/library/HashingProvider.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/library/SymmetricProvider.hpp"
)
target_include_directories(
  opentxs-common SYSTEM
  PRIVATE "${opentxs_SOURCE_DIR}/deps/robin-hood/src/include"
)
target_link_libraries(
  opentxs-common
  PRIVATE
    Boost::headers
    opentxs::messages
    unofficial-sodium::sodium
)
target_link_libraries(opentxs PUBLIC unofficial-sodium::sodium)
target_sources(opentxs-common PRIVATE ${cxx-install-headers})

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/crypto/library
    COMPONENT dev
  )
endif()

target_sources(
  opentxs-crypto-library
  PRIVATE
    "${opentxs_SOURCE_DIR}/src/internal/crypto/library/Pbkdf2.hpp"
    "${opentxs_SOURCE_DIR}/src/internal/crypto/library/Ripemd160.hpp"
    "trezor/hmac.c"
    "trezor/hmac.h"
    "trezor/memzero.c"
    "trezor/memzero.h"
    "trezor/options.h"
    "trezor/pbkdf2.c"
    "trezor/pbkdf2.h"
    "trezor/ripemd160.c"
    "trezor/ripemd160.h"
    "trezor/sha2.c"
    "trezor/sha2.h"
    "Pbkdf2.cpp"
    "Pbkdf2.hpp"
    "Ripemd160.cpp"
    "Ripemd160.hpp"
)
target_include_directories(
  opentxs-crypto-library SYSTEM
  PRIVATE "${opentxs_SOURCE_DIR}/deps/robin-hood/src/include"
)
target_link_libraries(
  opentxs-crypto-library
  PRIVATE
    Boost::headers
    opentxs::messages
    unofficial-sodium::sodium
)
target_link_libraries(opentxs PUBLIC unofficial-sodium::sodium)

if(${CMAKE_CXX_COMPILER_ID}
   MATCHES
   Clang
)
  set_source_files_properties(
    "Ripemd160.cpp" PROPERTIES COMPILE_FLAGS "-Wno-reserved-id-macro"
  )
  set_source_files_properties(
    "Pbkdf2.cpp" PROPERTIES COMPILE_FLAGS "-Wno-reserved-id-macro"
  )
endif()

target_sources(opentxs PRIVATE $<TARGET_OBJECTS:opentxs-crypto-library>)
add_dependencies(opentxs-crypto-library generated_code)
