cmake_minimum_required(VERSION 3.16)
project(MendoraQ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mendoraq_core
    src/CircularTicketQueue.cpp
    src/RepairWorkshop.cpp
)
target_include_directories(mendoraq_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
    # Native Win32 controls keep the Windows desktop app dependency-free.
    add_executable(mendoraq WIN32 src/gui_win32.cpp src/mendoraq.rc)
    target_link_libraries(mendoraq PRIVATE mendoraq_core user32 gdi32)
    target_compile_definitions(mendoraq PRIVATE _WIN32_WINNT=0x0A00 WINVER=0x0A00)
    if(MINGW)
        target_link_options(mendoraq PRIVATE -municode)
    endif()
else()
    message(STATUS "The MendoraQ desktop GUI uses Win32 and is built on Windows only.")
endif()

add_executable(queue_tests tests/queue_tests.cpp)
target_link_libraries(queue_tests PRIVATE mendoraq_core)

# Optional browser target. Configure with emcmake so the same core sources are
# compiled to WebAssembly; the native Windows target remains unchanged.
if(EMSCRIPTEN)
    add_executable(mendoraq_web src/wasm_bridge.cpp)
    target_link_libraries(mendoraq_web PRIVATE mendoraq_core)
    target_link_options(mendoraq_web PRIVATE
        --bind
        -sALLOW_MEMORY_GROWTH=1
        -sENVIRONMENT=web
        -sMODULARIZE=1
        -sEXPORT_NAME=createMendoraQModule
    )
    set_target_properties(mendoraq_web PROPERTIES
        OUTPUT_NAME "mendoraq"
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/web"
    )
endif()

if(MSVC)
    foreach(target mendoraq_core queue_tests)
        target_compile_options(${target} PRIVATE /W4)
    endforeach()
    if(TARGET mendoraq)
        target_compile_options(mendoraq PRIVATE /W4)
    endif()
else()
    foreach(target mendoraq_core queue_tests)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endforeach()
    if(TARGET mendoraq)
        target_compile_options(mendoraq PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

enable_testing()
add_test(NAME queue_tests COMMAND queue_tests)
