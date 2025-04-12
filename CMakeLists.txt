# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gpsdriver)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/nmea.c)
target_sources(app PRIVATE src/gps.c)
target_sources(app PRIVATE src/shellnmea.c)
