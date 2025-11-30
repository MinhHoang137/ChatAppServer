cmake_minimum_required(VERSION 3.16)

project(Server VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appServer
    main.cpp
)

qt_add_qml_module(appServer
    URI Server
    QML_FILES
        Main.qml
    SOURCES server.h server.cpp
    SOURCES authentication.cpp
    SOURCES authentication.h
)

target_link_libraries(appServer PRIVATE Qt6::Quick Qt6::Core Qt6::Widgets Qt6::Network Qt6::Sql Ws2_32)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appServer PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appServer
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appServer
    PRIVATE Qt6::Quick
)
target_link_libraries(appServer PRIVATE Qt6::Core Qt6::Widgets)

include(GNUInstallDirs)
install(TARGETS appServer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
