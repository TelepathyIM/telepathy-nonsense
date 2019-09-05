project(telepathy-nonsense)

cmake_minimum_required(VERSION 3.3)

set (telepathy_nonsense_VERSION_MAJOR 0)
set (telepathy_nonsense_VERSION_MINOR 1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
add_definitions(-DQT_NO_CAST_FROM_ASCII)

find_package(TelepathyQt5 0.9.6 REQUIRED)
find_package(TelepathyQt5Service 0.9.6 REQUIRED)
find_package(QXmpp 1.0.0 REQUIRED CONFIG)
find_package(Qt5 REQUIRED COMPONENTS Core DBus Xml Network)

include(GNUInstallDirs)
include(FeatureSummary)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(nonsense_SOURCES
    main.cc
    common.cc
    connection.cc
    debug.cc
    filetransferchannel.cc
    protocol.cc
    textchannel.cc
    muctextchannel.cc
    uniquehandlemap.cc
)

add_executable(telepathy-nonsense ${nonsense_SOURCES})

set_target_properties(telepathy-nonsense PROPERTIES AUTOMOC TRUE)

target_include_directories(telepathy-nonsense PRIVATE
    ${TELEPATHY_QT5_INCLUDE_DIR}
)
target_link_libraries(telepathy-nonsense
    Qt5::Core
    Qt5::DBus
    Qt5::Network
    QXmpp::QXmpp
    ${TELEPATHY_QT5_LIBRARIES}
    ${TELEPATHY_QT5_SERVICE_LIBRARIES}
)

configure_file(nonsense.service.in org.freedesktop.Telepathy.ConnectionManager.nonsense.service)
configure_file(telepathy-nonsense-config.h.in telepathy-nonsense-config.h)

install(
    TARGETS telepathy-nonsense
    DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}
)

install(
    FILES nonsense.manager
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/telepathy/managers
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.ConnectionManager.nonsense.service
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/dbus-1/services
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
