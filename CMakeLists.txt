cmake_minimum_required(VERSION 3.21.0)
project(DhTas CXX)

add_subdirectory(sumhook)

add_executable(dhtas
    injector/main.cpp
    injector/inject_dll.cpp
)
target_include_directories(dhtas PRIVATE common)
target_compile_features(dhtas PRIVATE cxx_std_20)
target_link_libraries(dhtas PRIVATE shlwapi)

add_library(dhtashook SHARED
    hook/dllmain.cpp
    hook/initguid.cpp
    hook/pytas.cpp
    hook/steam.cpp
    hook/window.cpp
)

target_include_directories(dhtashook PRIVATE common)
target_compile_features(dhtashook PRIVATE cxx_std_20)
target_link_libraries(dhtashook PRIVATE sumhook uuid)

find_package(Python3 REQUIRED COMPONENTS Development.Embed)

target_link_libraries(dhtashook PRIVATE Python3::Python)

install(TARGETS
    dhtas dhtashook
    RUNTIME DESTINATION .
)

install(DIRECTORY
    datafiles/documents12
    datafiles/documents14
    DESTINATION datafiles
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    FILES_MATCHING
    PATTERN Logs EXCLUDE
    PATTERN DishonoredEngine.ini
    PATTERN DishonoredInput.ini
)

install(DIRECTORY
    datafiles/scripts
    DESTINATION datafiles
    PATTERN __pycache__ EXCLUDE
)

install(DIRECTORY
    datafiles/userdata
    DESTINATION datafiles
)

install(DIRECTORY
    docs
    DESTINATION .
    FILES_MATCHING PATTERN "*.pdf"
)

install(DIRECTORY
    typings
    DESTINATION .
)

install(FILES
    README.md
    LICENSE
    DESTINATION .
)

install(IMPORTED_RUNTIME_ARTIFACTS
    Python3::Python
    RUNTIME DESTINATION .
)

install(DIRECTORY
    "${Python3_RUNTIME_LIBRARY_DIRS}/DLLs"
    "${Python3_RUNTIME_LIBRARY_DIRS}/Lib"
    DESTINATION .
    PATTERN "__pycache__" EXCLUDE
)

install(FILES
    "${Python3_RUNTIME_LIBRARY_DIRS}/NEWS.txt"
    "${Python3_RUNTIME_LIBRARY_DIRS}/LICENSE.txt"
    DESTINATION python_docs
)
