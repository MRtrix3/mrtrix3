# Find libzip library and headers, creating the libzip::zip imported target.

find_path(LibZip_INCLUDE_DIR
    NAMES zip.h
    DOC "libzip include directory"
)

find_library(LibZip_LIBRARY
    NAMES zip
    DOC "libzip library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibZip
    REQUIRED_VARS LibZip_LIBRARY LibZip_INCLUDE_DIR
)

if(LibZip_FOUND AND NOT TARGET libzip::zip)
    add_library(libzip::zip UNKNOWN IMPORTED)
    set_target_properties(libzip::zip PROPERTIES
        IMPORTED_LOCATION "${LibZip_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LibZip_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(LibZip_INCLUDE_DIR LibZip_LIBRARY)
