# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wake_on_lan)

FILE(GLOB app_sources src/*.c)
FILE(GLOB font_sources src/fonts/*.c)

target_sources(app PRIVATE ${app_sources} ${font_sources})

zephyr_include_directories(inc)
zephyr_include_directories(src/fonts)
