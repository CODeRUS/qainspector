cmake_minimum_required(VERSION 3.5)

project(QAInspector VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Network
    Xml
    XmlPatterns
    Widgets
)

file(GLOB_RECURSE PROJECT_SOURCES *.h *.hpp *.cppc)

set(MACOSX_BUNDLE_ICON_FILE application.icns)
set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
set(APP_ICON_MACOSX application.icns)
set_source_files_properties(${APP_ICON_MACOSX} PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources")

add_executable(${PROJECT_NAME}
    MACOSX_BUNDLE
    ${PROJECT_SOURCES}
    ${APP_ICON_MACOSX}
)

#target_link_libraries(${PROJECT_NAME} LINK_PUBLIC Qt5::QuickControls2 Qt5::Network Qt5::Quick Qt5::Core Qt5::Xml Qt5::XmlPatterns Qt5::Widgets)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt5::Core
    Qt5::Network
    Qt5::Xml
    Qt5::XmlPatterns
    Qt5::Widgets
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER qa.engine.inspector
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

#finalize_executable(${PROJECT_NAME})

