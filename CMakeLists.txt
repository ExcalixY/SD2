cmake_minimum_required(VERSION 3.10)

project(SD2
  VERSION 0.01
  DESCRIPTION "Maziukas zaidimas SD2 namu darbui naudojantis mano zaidimu varikli ExcalEngine"
)

set(CMAKE_CXX_STANDARD 23)

set(OUT_DIR ${CMAKE_SOURCE_DIR}/out/${EXCAL_RELEASE_TYPE}-${CMAKE_BUILD_TYPE})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR}/archive)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR}/bin)

add_subdirectory(libs)

file(GLOB_RECURSE SRC_CORE CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/CD/*.cpp)

add_library(SD2Core STATIC ${SRC_CORE})
target_link_libraries(SD2Core PUBLIC nlohmann_json::nlohmann_json)
target_include_directories(SD2Core PUBLIC ${CMAKE_SOURCE_DIR}/include)

file(GLOB_RECURSE SRC_GAME ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(FILTER SRC_GAME EXCLUDE REGEX "/src/CD/")

# add_executable(SD2 ${SRC_GAME})
# target_link_libraries(SD2 PUBLIC ExcalCore SD2Core)
# target_include_directories(SD2 PUBLIC ${CMAKE_SOURCE_DIR}/include)
# excal_add_assets(SD2 ${CMAKE_SOURCE_DIR}/Assets)

# Every game header as its own translation unit, so clangd gets the real flags for it even before a .cpp includes it.
# Never built, it only exists to land in compile_commands.json
# file(GLOB_RECURSE HEADERS_GAME CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/include/*.hpp)
# set_source_files_properties(${HEADERS_GAME} PROPERTIES LANGUAGE CXX)
#
# add_library(SD2_Headers OBJECT EXCLUDE_FROM_ALL ${HEADERS_GAME})
# target_link_libraries(SD2_Headers PRIVATE ExcalCore SD2Core)
# target_include_directories(SD2_Headers PRIVATE ${CMAKE_SOURCE_DIR}/include)

file(GLOB SRC_TERMINAL CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tools/Terminal/*.cpp)

add_executable(SD2Terminal ${SRC_TERMINAL})
target_link_libraries(SD2Terminal PRIVATE SD2Core)
excal_add_assets(SD2Terminal ${CMAKE_SOURCE_DIR}/Assets)
