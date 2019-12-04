
find_package(EdupalsBase REQUIRED)
find_package(PkgConfig REQUIRED)

#pkg-config
pkg_check_modules(CURL REQUIRED libcurl)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${EDUPALS_BASE_INCLUDE_DIRS})

add_library(edupals-n4d SHARED n4d.cpp)
target_link_libraries(edupals-n4d ${EDUPALS_BASE_LIBRARIES} ${CURL_LIBRARIES})
set_target_properties(edupals-n4d PROPERTIES SOVERSION 1 VERSION "1.2.1")

install(TARGETS edupals-n4d
    LIBRARY DESTINATION "lib"
)

install(FILES "${PROJECT_SOURCE_DIR}/EdupalsN4DConfig.cmake"
    DESTINATION "lib/cmake/EdupalsN4D"
)

#testing application
add_executable(testing testing.cpp)
target_link_libraries(testing edupals-n4d)
