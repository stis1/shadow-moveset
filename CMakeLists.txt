cmake_minimum_required(VERSION 3.28)
project(shadow-moveset VERSION 0.0.1 
                 DESCRIPTION "Shadow Moveset"
                 LANGUAGES CXX)

set(PROJECT_AUTHOR "stis")
set(PROJECT_LONG_DESCRIPTION "Editing Shadow Moveset")
set(PROJECT_COPYRIGHT "Copyright (c) 2025 stis - Licensed under GPL3")
set(PROJECT_AUTHOR_URL "https://github.com/stis1")
set(PROJECT_UPDATE_SERVER "https://raw.githubusercontent.com/stis1/mod_update/main/shadow-moveset/")
option(INSTALL_TO_UPDATE_SERVER "Install to update server" OFF)
set(UPDATE_SERVER_PATH "" CACHE PATH "Path to update server")
string(TIMESTAMP PROJECT_BUILD_DATE "%d.%m.%Y")

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>")

set(GAME_FOLDER "C:\\Program Files (x86)\\Steam\\steamapps\\common\\SONIC_X_SHADOW_GENERATIONS" CACHE STRING "The location of your game folder.")
cmake_path(SET GAME_FOLDER_NORMALIZED NORMALIZE ${GAME_FOLDER})
cmake_path(APPEND GAME_FOLDER_NORMALIZED "SONIC_X_SHADOW_GENERATIONS.exe" OUTPUT_VARIABLE SHADOW_GENERATIONS_GAME_EXECUTABLE)

if(DEFINED ENV{MILLER_SDK})
    message("MILLER_SDK environment variable found. Using this version instead of the local submodule.")
    cmake_path(SET MILLER_SDK_PATH NORMALIZE $ENV{MILLER_SDK})
else()
    set(MILLER_SDK_PATH miller-sdk)
endif()

if(INSTALL_TO_UPDATE_SERVER)
    cmake_path(SET UPDATE_SERVER_PATH_NORMALIZED NORMALIZE ${UPDATE_SERVER_PATH})
    message("Install to update server enabled. Update server path is ${UPDATE_SERVER_PATH}")
endif()

add_subdirectory(vendor EXCLUDE_FROM_ALL)
add_subdirectory(${MILLER_SDK_PATH} miller-sdk EXCLUDE_FROM_ALL)

add_library(mod SHARED)
target_compile_features(mod PRIVATE cxx_std_20)
target_compile_options(mod PRIVATE /permissive-)
target_compile_definitions(mod
    PRIVATE
        PROJECT_BINARY_FILENAME=$<TARGET_FILE_NAME:mod>
        PROJECT_NAME="${PROJECT_NAME}"
        PROJECT_AUTHOR="${PROJECT_AUTHOR}"
        PROJECT_DESCRIPTION="${PROJECT_DESCRIPTION}"
        PROJECT_LONG_DESCRIPTION="${PROJECT_LONG_DESCRIPTION}"
        PROJECT_COPYRIGHT="${PROJECT_COPYRIGHT}"
        PROJECT_AUTHOR_URL="${PROJECT_AUTHOR_URL}"
        PROJECT_UPDATE_SERVER="${PROJECT_UPDATE_SERVER}"
        PROJECT_BUILD_DATE="${PROJECT_BUILD_DATE}"
        PROJECT_VERSION=${PROJECT_VERSION}
        PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)
set_target_properties(mod
    PROPERTIES
        OUTPUT_NAME ${PROJECT_NAME}
        VS_DEBUGGER_COMMAND ${SHADOW_GENERATIONS_GAME_EXECUTABLE}
        VS_DEBUGGER_WORKING_DIRECTORY ${GAME_FOLDER_NORMALIZED}
)

target_link_libraries(mod PRIVATE miller-sdk detours)

add_subdirectory(src)
configure_file(mod.ini.in mod.ini)
configure_file(mod_version.ini.in mod_version.ini)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT mod)

cmake_path(APPEND GAME_FOLDER_NORMALIZED "mods_shadow" OUTPUT_VARIABLE MODS_FOLDER)
cmake_path(APPEND MODS_FOLDER "${PROJECT_NAME}" OUTPUT_VARIABLE MOD_FOLDER)

install(TARGETS mod RUNTIME DESTINATION "${MOD_FOLDER}")
install(DIRECTORY resources/ DESTINATION "${MOD_FOLDER}")
install(FILES LICENSE.txt CHANGELOG.md "${CMAKE_CURRENT_BINARY_DIR}/mod.ini" DESTINATION "${MOD_FOLDER}")

install(CODE "
    file(MAKE_DIRECTORY \"${CMAKE_CURRENT_BINARY_DIR}/archives\")
    execute_process(
        COMMAND \$\{CMAKE_COMMAND} \"-E\" \"tar\" \"cvf\" \"${CMAKE_CURRENT_BINARY_DIR}/archives/${PROJECT_NAME}-v${PROJECT_VERSION}.zip\" \"--format=zip\" \"${PROJECT_NAME}\"
        WORKING_DIRECTORY \"${MODS_FOLDER}\"
    )
")

if(DEFINED UPDATE_SERVER_PATH_NORMALIZED)
    cmake_path(APPEND UPDATE_SERVER_PATH_NORMALIZED "shadow-generations/${PROJECT_NAME}" OUTPUT_VARIABLE UPDATE_SERVER_MOD_FOLDER)

    install(TARGETS mod RUNTIME DESTINATION ${UPDATE_SERVER_MOD_FOLDER})
    install(DIRECTORY resources/ DESTINATION ${UPDATE_SERVER_MOD_FOLDER})
    install(FILES LICENSE.txt CHANGELOG.md "${CMAKE_CURRENT_BINARY_DIR}/mod.ini" "${CMAKE_CURRENT_BINARY_DIR}/mod_version.ini" DESTINATION ${UPDATE_SERVER_MOD_FOLDER})

    install(CODE "
        file(GLOB_RECURSE UPDATE_SERVER_FILES LIST_DIRECTORIES false RELATIVE \"${UPDATE_SERVER_MOD_FOLDER}\" \"${UPDATE_SERVER_MOD_FOLDER}/*\")
        list(REMOVE_ITEM UPDATE_SERVER_FILES \"mod_files.txt\" \"mod_version.ini\")
        list(TRANSFORM UPDATE_SERVER_FILES PREPEND \"add \")
        list(JOIN UPDATE_SERVER_FILES \"\\n\" UPDATE_SERVER_MOD_FILES_CONTENT)
        file(WRITE \"${UPDATE_SERVER_MOD_FOLDER}/mod_files.txt\" \"\${UPDATE_SERVER_MOD_FILES_CONTENT}\\n\")
    ")
endif()
