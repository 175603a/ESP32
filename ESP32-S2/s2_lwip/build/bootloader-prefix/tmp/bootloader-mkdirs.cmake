# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Espressif/frameworks/esp-idf-v5.0.1/components/bootloader/subproject"
  "D:/51-STM-ESP-GD/ESP/ESP32-S2/s2_lwip/build/bootloader"
  "D:/51-STM-ESP-GD/ESP/ESP32-S2/s2_lwip/build/bootloader-prefix"
  "D:/51-STM-ESP-GD/ESP/ESP32-S2/s2_lwip/build/bootloader-prefix/tmp"
  "D:/51-STM-ESP-GD/ESP/ESP32-S2/s2_lwip/build/bootloader-prefix/src/bootloader-stamp"
  "D:/51-STM-ESP-GD/ESP/ESP32-S2/s2_lwip/build/bootloader-prefix/src"
  "D:/51-STM-ESP-GD/ESP/ESP32-S2/s2_lwip/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/51-STM-ESP-GD/ESP/ESP32-S2/s2_lwip/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/51-STM-ESP-GD/ESP/ESP32-S2/s2_lwip/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
