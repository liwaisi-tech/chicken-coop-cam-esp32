# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/btatacc/esp/esp-idf/components/bootloader/subproject"
  "/Users/btatacc/Repository/esp32/esp32CAM/chicken-coop-cam-esp32/build/bootloader"
  "/Users/btatacc/Repository/esp32/esp32CAM/chicken-coop-cam-esp32/build/bootloader-prefix"
  "/Users/btatacc/Repository/esp32/esp32CAM/chicken-coop-cam-esp32/build/bootloader-prefix/tmp"
  "/Users/btatacc/Repository/esp32/esp32CAM/chicken-coop-cam-esp32/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/btatacc/Repository/esp32/esp32CAM/chicken-coop-cam-esp32/build/bootloader-prefix/src"
  "/Users/btatacc/Repository/esp32/esp32CAM/chicken-coop-cam-esp32/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/btatacc/Repository/esp32/esp32CAM/chicken-coop-cam-esp32/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/btatacc/Repository/esp32/esp32CAM/chicken-coop-cam-esp32/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
