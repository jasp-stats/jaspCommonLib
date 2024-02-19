# This will build the Common library
#
# On Linux,
#   - We need to link to the `librt.so`, and for some reason, CMake cannot find it
#     easily, so, Libraries module tries to find it and provides it to Common here.
#   - Since we are not using Conan, I link to the libjsoncpp, using the PkgConfig variables
#
#
list(APPEND CMAKE_MESSAGE_CONTEXT jaspCommon)

message(CHECK_START "Checking for CRYPT_KEY")
set(CRYPT_KEY
    ""
    CACHE STRING "")

if(CRYPT_KEY STREQUAL "")
  # Let's see if the user has set something in the environment
  set(CRYPT_KEY $ENV{CRYPTKEY})

  if(CRYPT_KEY STREQUAL "")

    message(CHECK_PASS "set to default.")

    set(CRYPT_KEY "0x0c2ad4a4acb9f023")

  else()

    message(CHECK_PASS "found in environment.")

  endif()

  message(STATUS "  ${CRYPT_KEY}")

else()
  message(CHECK_PASS "set by user in cmake config.")
endif()

set(JASP_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(JASP_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(JASP_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(JASP_VERSION_TWEAK ${PROJECT_VERSION_TWEAK})

set(JASP_VERSION ${CMAKE_PROJECT_VERSION})
set(JASP_SHORT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

message(STATUS "Version: ${CMAKE_PROJECT_VERSION}")

configure_file(${CMAKE_CURRENT_LIST_DIR}/appinfo.cpp.in
               ${CMAKE_CURRENT_LIST_DIR}/appinfo.cpp)
message(STATUS "appinfo.cpp is successfully generated...")

configure_file(${CMAKE_CURRENT_LIST_DIR}/QtUtils/appdirs.h.in
               ${CMAKE_CURRENT_LIST_DIR}/QtUtils//appdirs.h)
message(STATUS "appdirs.h is successfully generated...")

file(GLOB HEADER_FILES "${CMAKE_CURRENT_LIST_DIR}/*.h" "${CMAKE_CURRENT_LIST_DIR}/json/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/*.cpp"  "${CMAKE_CURRENT_LIST_DIR}/json/*.cpp")
file(GLOB QT_UTILS_HEADER_FILES "${CMAKE_CURRENT_LIST_DIR}/QtUtils/*.h")
file(GLOB QT_UTILS_SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/QtUtils/*.cpp")

add_library(Common SHARED ${SOURCE_FILES} ${HEADER_FILES})
add_library(CommonQt SHARED ${SOURCE_FILES} ${HEADER_FILES} ${QT_UTILS_HEADER_FILES} ${QT_UTILS_SOURCE_FILES})

if(LINUX)
  target_link_libraries(Common PUBLIC ${_LIB_RT})
  target_link_options(Common PUBLIC -lrt)
  target_link_libraries(CommonQt PUBLIC ${_LIB_RT})
  target_link_options(CommonQt PUBLIC -lrt)
endif()

target_include_directories(
    Common PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${R_INCLUDE_PATH}
        ${R_HOME_PATH}/include
        ${RCPP_PATH}/include
)

target_include_directories(
    CommonQt PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${R_INCLUDE_PATH}
        ${R_HOME_PATH}/include
        ${RCPP_PATH}/include
)

target_link_libraries(
    Common PUBLIC
        #Boost::system
        #Boost::date_time
        #Boost::timer
        #Boost::chrono
        $<$<BOOL:${WINDOWS}>:ntdll>
)

target_link_libraries(
    CommonQt PUBLIC
        #Boost::system
        #Boost::date_time
        #Boost::timer
        #Boost::chrono
        $<$<BOOL:${WINDOWS}>:ntdll>
        Qt::Core
        Qt::Qml
        Qt::Quick
)

target_compile_definitions(
    Common PUBLIC
        $<$<BOOL:${JASP_TIMER_USED}>:PROFILE_JASP>
        JSONCPP_NO_LOCALE_SUPPORT
)

target_compile_definitions(
    CommonQt PUBLIC
        $<$<BOOL:${JASP_USES_QT_HERE}>:JASP_USES_QT_HERE>
        $<$<BOOL:${JASP_TIMER_USED}>:PROFILE_JASP>
        JSONCPP_NO_LOCALE_SUPPORT
)

if(WINDOWS)
  target_compile_definitions(
    Common PUBLIC
        NOMINMAX
        WIN32_LEAN_AND_MEAN
  )
  target_compile_definitions(
    CommonQt PUBLIC
        NOMINMAX
        WIN32_LEAN_AND_MEAN
  )
endif()

if(IWYU_EXECUTABLE AND RUN_IWYU)
  set_target_properties(Common PROPERTIES CXX_INCLUDE_WHAT_YOU_USE ${IWYU_EXECUTABLE})
  set_target_properties(CommonQt PROPERTIES CXX_INCLUDE_WHAT_YOU_USE ${IWYU_EXECUTABLE})
endif()

list(POP_BACK CMAKE_MESSAGE_CONTEXT)
