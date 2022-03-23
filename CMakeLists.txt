cmake_minimum_required(VERSION 3.5)

project(QAInspector VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Network
    Xml
    XmlPatterns
    Widgets
)

file(GLOB_RECURSE PROJECT_SOURCES *.h *.cpp)

set(MACOSX_BUNDLE_ICON_FILE application.icns)
set(MACOSX_BUNDLE_BUNDLE_NAME "QA Inspector")
set(APP_ICON_MACOSX ${CMAKE_CURRENT_SOURCE_DIR}/application.icns)
set_source_files_properties(${APP_ICON_MACOSX} PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources")

add_executable(qainspector-cmake
    MACOSX_BUNDLE
    ${PROJECT_SOURCES}
    ${APP_ICON_MACOSX}
)

#target_link_libraries(${PROJECT_NAME} LINK_PUBLIC Qt5::QuickControls2 Qt5::Network Qt5::Quick Qt5::Core Qt5::Xml Qt5::XmlPatterns Qt5::Widgets)

target_link_libraries(qainspector-cmake
    PRIVATE
    Qt5::Core
    Qt5::Network
    Qt5::Xml
    Qt5::XmlPatterns
    Qt5::Widgets
)

set_target_properties(qainspector-cmake PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

#finalize_executable(qainspector-cmake)

