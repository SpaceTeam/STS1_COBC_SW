function(find_rodos)
    set(RODOS_PACKAGE_NAME
        "rodos"
        CACHE STRING "Name of the Rodos package used when calling find_package()"
    )
    find_package(${RODOS_PACKAGE_NAME} REQUIRED ${ARGN})
    get_target_property(rodos_location rodos::rodos LOCATION)
    message("Found rodos: ${rodos_location}")
endfunction()
