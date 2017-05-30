find_package(KDE4 REQUIRED)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include(KDE4Defaults)
include(FindPkgConfig)
include(MacroLibrary)

pkg_check_modules(CCS REQUIRED libcompizconfig)

QT4_ADD_DBUS_INTERFACE( kconfig4_kwin_SRCS org.kde.KWin.xml kwin_interface )


link_directories(${CCS_LIBRARY_DIRS})
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CCS_INCLUDE_DIRS})

set(kconfig4_LIB_SRCS kconfig_backend.cpp ${kconfig4_kwin_SRCS})

kde4_add_library(kconfig4 SHARED ${kconfig4_LIB_SRCS})

target_link_libraries(kconfig4 ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${CCS_LIBRARIES} X11)

install(TARGETS kconfig4 DESTINATION ${CCS_LIBDIR}/compizconfig/backends)
