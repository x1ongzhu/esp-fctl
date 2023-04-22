# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/drew/esp/esp-idf/components/bootloader/subproject"
  "/Users/drew/Projects/esp/restful_server/build/bootloader"
  "/Users/drew/Projects/esp/restful_server/build/bootloader-prefix"
  "/Users/drew/Projects/esp/restful_server/build/bootloader-prefix/tmp"
  "/Users/drew/Projects/esp/restful_server/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/drew/Projects/esp/restful_server/build/bootloader-prefix/src"
  "/Users/drew/Projects/esp/restful_server/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/drew/Projects/esp/restful_server/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/drew/Projects/esp/restful_server/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
