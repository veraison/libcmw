cmake_minimum_required(VERSION 3.16)
project(libcmw VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LIBCMW_BUILD_TESTS "Build libcmw tests" ON)

include(FetchContent)

FetchContent_Declare(
    qcbor
    GIT_REPOSITORY https://github.com/laurencelundblade/QCBOR.git
    GIT_TAG v1.6.1
)
set(BUILD_QCBOR_TEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(qcbor)

FetchContent_Declare(
    cjson
    GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
    GIT_TAG v1.7.18
)
FetchContent_GetProperties(cjson)
if(NOT cjson_POPULATED)
    FetchContent_Populate(cjson)
    add_library(cjson STATIC ${cjson_SOURCE_DIR}/cJSON.c)
    target_include_directories(cjson PUBLIC ${cjson_SOURCE_DIR})
endif()

add_library(cmw
    src/cbor_internal.cpp
    src/cfmap.cpp
    src/cmw.cpp
    src/collection.cpp
    src/indicator.cpp
    src/monad.cpp
    src/qcbor_util.cpp
    src/sniff.cpp
    src/tn.cpp
    src/type.cpp
    src/util.cpp
)

target_include_directories(cmw
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${cjson_SOURCE_DIR}
)

target_link_libraries(cmw
    PUBLIC
        qcbor
    PRIVATE
        cjson
)

target_compile_features(cmw PUBLIC cxx_std_17)

if(LIBCMW_BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
endif()

install(TARGETS cmw EXPORT cmwTargets)
install(DIRECTORY include/ DESTINATION include)
