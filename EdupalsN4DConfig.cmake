

find_library(EDUPALS_N4D_LIB "edupals-n4d")
set(EDUPALS_N4D_INCLUDE_DIRS "/usr/include/edupals/n4d/")
set(EDUPALS_N4D_LIBRARIES ${EDUPALS_N4D_LIB})

add_library(Edupals::N4D SHARED IMPORTED)
set_target_properties(Edupals::N4D PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${EDUPALS_N4D_INCLUDE_DIRS}
    INTERFACE_LINK_LIBRARIES "Edupals::N4D"
    IMPORTED_LOCATION ${EDUPALS_N4D_LIBRARIES}
)