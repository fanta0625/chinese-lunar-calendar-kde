cmake_minimum_required(VERSION 3.16)

set(CHINESE_LUNAR_CALENDAR_VERSION "0.1.0" CACHE STRING "中国农历发布版本")

project(ChineseLunarCalendarForKDE VERSION ${CHINESE_LUNAR_CALENDAR_VERSION} LANGUAGES CXX)

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

find_package(Qt6 REQUIRED COMPONENTS Core Qml)
find_package(KF6 REQUIRED COMPONENTS CoreAddons)
# Debian packages CalendarEvents within KF6Declarative rather than as a
# standalone KF6CalendarEvents CMake package.
find_package(KF6Declarative REQUIRED)

add_library(chineselunarcalendarcore STATIC
    src/customevents.cpp
    src/eventcolors.cpp
    src/lunarconverter.cpp
    src/solarterms.cpp
    src/chineselunarcalendarsettings.cpp
    src/workdaydata.cpp
)

target_include_directories(chineselunarcalendarcore PUBLIC src)
target_link_libraries(chineselunarcalendarcore PUBLIC Qt6::Core)

# Plasma loads calendar event extensions as shared libraries.
set(BUILD_SHARED_LIBS ON)
# Some KF6 distributions require this for kcoreaddons_add_plugin() to place
# the uninstalled module in the build tree instead of the filesystem root.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

kcoreaddons_add_plugin(chineselunarcalendar
    SOURCES
        src/customeventsconfig.cpp
        src/customeventsconfig.h
        src/lunarcalendarplugin.cpp
        src/lunarcalendarplugin.h
    INSTALL_NAMESPACE "plasmacalendarplugins"
)

target_link_libraries(chineselunarcalendar PRIVATE
    Qt6::Qml
    KF6::CalendarEvents
    KF6::CoreAddons
    chineselunarcalendarcore
)

# Match upstream plasma-workspace convention: calendar plugin files carry no
# "lib" prefix, and the plugin id is derived from the file name.
set_target_properties(chineselunarcalendar PROPERTIES PREFIX "")

install(FILES src/eventsconfig.qml
    DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasmacalendarplugins
)

install(DIRECTORY data/
    DESTINATION ${KDE_INSTALL_DATADIR}/lunarcalendar
)

# One-time first-login hint with one-click enable. The desktop file is
# configured to point at the installed script in libexec.
configure_file(
    postinstall/chinese-lunar-calendar-postinstall-hint.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/chinese-lunar-calendar-postinstall-hint.desktop
    @ONLY
)
install(PROGRAMS postinstall/chinese-lunar-calendar-postinstall-hint
    DESTINATION ${KDE_INSTALL_LIBEXECDIR}
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/chinese-lunar-calendar-postinstall-hint.desktop
    DESTINATION ${KDE_INSTALL_SYSCONFDIR}/xdg/autostart
)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)

    add_executable(chineselunarcalendarcore_test
        tests/lunarconvertertest.cpp
    )
    target_link_libraries(chineselunarcalendarcore_test PRIVATE
        Qt6::Test
        chineselunarcalendarcore
    )
    add_test(NAME chineselunarcalendarcore_test COMMAND chineselunarcalendarcore_test)

    add_executable(customeventstest
        tests/customeventstest.cpp
    )
    target_link_libraries(customeventstest PRIVATE
        Qt6::Test
        chineselunarcalendarcore
    )
    add_test(NAME customeventstest COMMAND customeventstest)
endif()

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "chinese-lunar-calendar-kde")
set(CPACK_PACKAGE_VENDOR "Loongson")
set(CPACK_PACKAGE_CONTACT "Loongson")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Chinese lunar calendar plugin for KDE Plasma")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_REPLACES "suishi")
set(CPACK_DEBIAN_PACKAGE_BREAKS "suishi")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "plasma-workspace (>= 6.0), python3, libnotify-bin")

include(CPack)
