cmake_minimum_required(VERSION 3.16)

set(SUISHI_VERSION "0.1.0" CACHE STRING "岁时发布版本")

project(suishi VERSION ${SUISHI_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(CTest)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(KF6 REQUIRED COMPONENTS CoreAddons)
# Debian packages CalendarEvents within KF6Declarative rather than as a
# standalone KF6CalendarEvents CMake package.
find_package(KF6Declarative REQUIRED)

add_library(lunarcalendarcore STATIC
    src/lunarconverter.cpp
    src/solarterms.cpp
    src/workdaydata.cpp
)

target_include_directories(lunarcalendarcore PUBLIC src)
target_link_libraries(lunarcalendarcore PUBLIC Qt6::Core)

# Plasma loads calendar event extensions as shared libraries.
set(BUILD_SHARED_LIBS ON)

kcoreaddons_add_plugin(locallunarcalendar
    SOURCES
        src/lunarcalendarplugin.cpp
        src/lunarcalendarplugin.h
    INSTALL_NAMESPACE "plasmacalendarplugins"
)

target_link_libraries(locallunarcalendar PRIVATE
    KF6::CalendarEvents
    KF6::CoreAddons
    lunarcalendarcore
)

install(DIRECTORY data/
    DESTINATION ${KDE_INSTALL_DATADIR}/lunarcalendar
)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)

    add_executable(lunarcalendarcore_test
        tests/lunarconvertertest.cpp
    )
    target_link_libraries(lunarcalendarcore_test PRIVATE
        Qt6::Test
        lunarcalendarcore
    )
    add_test(NAME lunarcalendarcore_test COMMAND lunarcalendarcore_test)
endif()

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "suishi")
set(CPACK_PACKAGE_VENDOR "Loongson")
set(CPACK_PACKAGE_CONTACT "Loongson")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Chinese lunar calendar extension for KDE Plasma")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "plasma-workspace (>= 6.0)")

include(CPack)
