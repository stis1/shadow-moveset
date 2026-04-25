cmake_minimum_required(VERSION 3.28)
project(mINI LANGUAGES CXX)

add_library(mINI INTERFACE)
target_sources(mINI
    PUBLIC FILE_SET HEADERS BASE_DIRS mINI/src/mini FILES
        mINI/src/mini/ini.h
)
