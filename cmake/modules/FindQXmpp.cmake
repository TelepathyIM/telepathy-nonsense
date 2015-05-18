find_package(PkgConfig)
pkg_check_modules(PC_QXMPP QUIET qxmpp)

find_path(QXMPP_INCLUDE_DIR QXmppConfiguration.h HINTS ${PC_QXMPP_INCLUDEDIR} ${PC_QXMPP_INCLUDE_DIRS})
find_library(QXMPP_LIBRARY NAMES qxmpp HINTS ${PC_QXMPP_LIBDIR} ${PC_QXMPP_LIBRARY_DIRS})

set(QXMPP_LIBRARIES ${QXMPP_LIBRARY})
set(QXMPP_INCLUDE_DIRS ${QXMPP_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QXMPP DEFAULT_MSG QXMPP_LIBRARY QXMPP_INCLUDE_DIR)
mark_as_advanced(QXMPP_INCLUDE_DIR QXMPP_LIBRARY)

