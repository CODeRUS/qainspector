cmake_minimum_required(VERSION 3.5)

project(qainspector-cmake VERSION 0.1 LANGUAGES CXX)

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

add_executable(qainspector-cmake
    ${PROJECT_SOURCES})

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

