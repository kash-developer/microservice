project(baresip)

cmake_minimum_required(VERSION 3.4.1)

set(distribution_DIR ${CMAKE_SOURCE_DIR}/../../../../distribution)

add_library(lib_crypto STATIC IMPORTED)
set_target_properties(lib_crypto PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/openssl/lib/${ANDROID_ABI}/libcrypto.a)

add_library(lib_ssl STATIC IMPORTED)
set_target_properties(lib_ssl PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/openssl/lib/${ANDROID_ABI}/libssl.a)

add_library(lib_re STATIC IMPORTED)
set_target_properties(lib_re PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/re/lib/${ANDROID_ABI}/libre.a)

set(re_DIR ${distribution_DIR}/re/cmake)
find_package(re CONFIG REQUIRED HINTS ${distribution_DIR}/re/cmake)
add_definitions(${RE_DEFINITIONS})

add_library(lib_opus STATIC IMPORTED)
set_target_properties(lib_opus PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/opus/lib/${ANDROID_ABI}/libopus.a)

add_library(lib_spandsp STATIC IMPORTED)
set_target_properties(lib_spandsp PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/spandsp/lib/${ANDROID_ABI}/libspandsp.a)

add_library(lib_g722_1 STATIC IMPORTED)
set_target_properties(lib_g722_1 PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/g7221/lib/${ANDROID_ABI}/libg722_1.a)

add_library(lib_g729 STATIC IMPORTED)
set_target_properties(lib_g729 PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/g729/lib/${ANDROID_ABI}/libbcg729.a)

add_library(lib_gsm STATIC IMPORTED)
set_target_properties(lib_gsm PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gsm/lib/${ANDROID_ABI}/libgsm.a)

add_library(lib_amrnb STATIC IMPORTED)
set_target_properties(lib_amrnb PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/amr/lib/${ANDROID_ABI}/libamrnb.a)

add_library(lib_amrwb STATIC IMPORTED)
set_target_properties(lib_amrwb PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/amr/lib/${ANDROID_ABI}/libamrwb.a)

add_library(lib_amrwbenc STATIC IMPORTED)
set_target_properties(lib_amrwbenc PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/amr/lib/${ANDROID_ABI}/libamrwbenc.a)

add_library(lib_webrtc STATIC IMPORTED)
set_target_properties(lib_webrtc PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/webrtc/lib/${ANDROID_ABI}/libwebrtc.a)

add_library(lib_zrtpcppcore STATIC IMPORTED)
set_target_properties(lib_zrtpcppcore PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gzrtp/lib/${ANDROID_ABI}/libzrtpcppcore.a)

add_library(lib_sndfile STATIC IMPORTED)
set_target_properties(lib_sndfile PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/sndfile/lib/${ANDROID_ABI}/libsndfile.a)

add_library(lib_baresip STATIC IMPORTED)
set_target_properties(lib_baresip PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/baresip/lib/${ANDROID_ABI}/libbaresip.a)



set(
        MICRO_SOURCE
        microdeviceservice.cpp
        jsoncpp.cpp
        trace.cpp
        tools.cpp
        http_message.cpp
        http_request.cpp
        http_response.cpp
        http_server.cpp
        httpu_message.cpp
        httpu_server.cpp
        boiler_device.cpp
        breaker_device.cpp
        controller_device.cpp
        curtain_device.cpp
        doorlock_device.cpp
        gasvalve_device.cpp
        home_device_lib.cpp
        forwarder_lib.cpp
        light_device.cpp
        phone_device.cpp
        powergate_device.cpp
        prevent_crime_ext_device.cpp
        remote_inspector_device.cpp
        serial_device.cpp
        system_aircon_device.cpp
        temperature_controller_device.cpp
        vantilator_device.cpp
)

add_library( # Sets the name of the library.
        microdeviceservice

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${MICRO_SOURCE}
        )
target_include_directories(microdeviceservice PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        )
target_link_libraries(microdeviceservice
        log
        )

add_library(baresip SHARED baresip.c)


target_include_directories(baresip PRIVATE
        ${distribution_DIR}/openssl/include
        ${distribution_DIR}/re/include
        ${distribution_DIR}/baresip/include
        ${distribution_DIR}/microdeviceservice/include
        ${CMAKE_SOURCE_DIR}
        )

add_definitions(-DHAVE_PTHREAD)

target_link_libraries(baresip
        android
        OpenSLES
        lib_baresip
        lib_re
        lib_ssl
        lib_crypto
        lib_opus
        lib_spandsp
        lib_g722_1
        lib_g729
        lib_gsm
        lib_amrnb
        lib_amrwb
        lib_amrwbenc
        lib_webrtc
        lib_zrtpcppcore
        lib_sndfile
        z
        log
        )