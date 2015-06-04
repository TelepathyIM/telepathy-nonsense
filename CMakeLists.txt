project(telepathy-qxmpp)

cmake_minimum_required(VERSION 2.8)

set (telepathy_qxmpp_VERSION_MAJOR 0)
set (telepathy_qxmpp_VERSION_MINOR 1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

find_package(TelepathyQt5 0.9.5 REQUIRED)
find_package(TelepathyQt5Service 0.9.5 REQUIRED)

find_package(QXmpp 0.8.1 REQUIRED)

find_package(Qt5 REQUIRED COMPONENTS Core DBus Xml Network)

include(GNUInstallDirs)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(qxmpp_SOURCES
    main.cc
    common.cc
    common.hh
    connection.cc
    connection.hh
    protocol.cc
    protocol.hh
    textchannel.cc
    textchannel.hh
    uniquehandlemap.cc
    uniquehandlemap.hh
)

add_executable(telepathy-qxmpp ${qxmpp_SOURCES})

set_target_properties(telepathy-qxmpp PROPERTIES AUTOMOC TRUE)

target_include_directories(telepathy-qxmpp PRIVATE
    ${TELEPATHY_QT5_INCLUDE_DIR}
    ${QXMPP_INCLUDE_DIR})

target_link_libraries(telepathy-qxmpp
    Qt5::Core
    Qt5::DBus
    Qt5::Network
    ${TELEPATHY_QT5_LIBRARIES}
    ${TELEPATHY_QT5_SERVICE_LIBRARIES}
    ${QXMPP_LIBRARIES})

configure_file(qxmpp.service.in org.freedesktop.Telepathy.ConnectionManager.qxmpp.service)
configure_file(telepathy-qxmpp-config.h.in telepathy-qxmpp-config.h)

install(
    TARGETS telepathy-qxmpp
    DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}
)

install(
    FILES qxmpp.manager
    DESTINATION share/telepathy/managers
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.ConnectionManager.qxmpp.service
    DESTINATION share/dbus-1/services
)

