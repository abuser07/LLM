cmake_minimum_required(VERSION 3.16)

project(llm_studio VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Necessário para o mecanismo de signals/slots e Q_OBJECT
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_standard_project_setup()

add_executable(llm_studio
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(llm_studio PRIVATE
    Qt6::Widgets
    Qt6::Network
)

# No Windows: não abrir uma janela de console junto com a GUI.
if(WIN32)
    set_target_properties(llm_studio PROPERTIES WIN32_EXECUTABLE ON)
endif()

if(MSVC)
    target_compile_options(llm_studio PRIVATE /W4 /utf-8)
else()
    target_compile_options(llm_studio PRIVATE -Wall -Wextra)
endif()
