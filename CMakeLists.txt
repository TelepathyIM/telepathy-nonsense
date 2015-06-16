project(telepathy-nonsense)

cmake_minimum_required(VERSION 2.8)

set (telepathy_nonsense_VERSION_MAJOR 0)
set (telepathy_nonsense_VERSION_MINOR 1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
add_definitions(-DQT_NO_CAST_FROM_ASCII)

if (USE_QT4)
    find_package(TelepathyQt4 0.9.6 REQUIRED)
    find_package(TelepathyQt4Service 0.9.6 REQUIRED)
    find_package(QXmpp 0.8.1 REQUIRED)
    find_package(Qt4 4.8.0 REQUIRED QtCore QtNetwork)
else()
    find_package(TelepathyQt5 0.9.6 REQUIRED)
    find_package(TelepathyQt5Service 0.9.6 REQUIRED)
    find_package(QXmpp 0.8.1 REQUIRED)
    find_package(Qt5 REQUIRED COMPONENTS Core DBus Xml Network)
endif()

include(GNUInstallDirs)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(nonsense_SOURCES
    main.cc
    common.cc
    connection.cc
    filetransferchannel.cc
    protocol.cc
    textchannel.cc
    uniquehandlemap.cc
)

add_executable(telepathy-nonsense ${nonsense_SOURCES})

set_target_properties(telepathy-nonsense PROPERTIES AUTOMOC TRUE)

if (USE_QT4)
    target_include_directories(telepathy-nonsense PRIVATE
        ${QT_INCLUDES}
        ${TELEPATHY_QT4_INCLUDE_DIR}
        ${QXMPP_INCLUDE_DIR}
    )
    target_link_libraries(telepathy-nonsense
        ${QT_QTCORE_LIBRARY}
        ${QT_QTDBUS_LIBRARY}
        ${QT_QTNETWORK_LIBRARY}
        ${QT_QTGUI_LIBRARY}
        ${TELEPATHY_QT4_LIBRARIES}
        ${TELEPATHY_QT4_SERVICE_LIBRARIES}
        ${QXMPP_LIBRARIES}
    )
else()
    target_include_directories(telepathy-nonsense PRIVATE
        ${TELEPATHY_QT5_INCLUDE_DIR}
        ${QXMPP_INCLUDE_DIR}
    )
    target_link_libraries(telepathy-nonsense
        Qt5::Core
        Qt5::DBus
        Qt5::Network
        ${TELEPATHY_QT5_LIBRARIES}
        ${TELEPATHY_QT5_SERVICE_LIBRARIES}
        ${QXMPP_LIBRARIES}
    )
endif()

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
