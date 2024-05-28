# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.

project("microdeviceservice")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.


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

add_compile_definitions(__ANDROID__)

add_library( # Sets the name of the library.
     microdeviceservice

     # Sets the library as a shared library.
     SHARED

     # Provides a relative path to your source file(s).
     #microdeviceservice.cpp )
     ${MICRO_SOURCE} )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
      log-lib

      # Specifies the name of the NDK library that
      # you want CMake to locate.
      log )


# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
               microdeviceservice

               # Links the target library to the log library
               # included in the NDK.
               ${log-lib}
)

target_include_directories(
    microdeviceservice
    PRIVATE ./include
    PRIVATE .
)