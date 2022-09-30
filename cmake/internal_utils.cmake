
#application version
function(get_app_version version default_version)
    set (${version} ${default_version} PARENT_SCOPE)
    if (EXISTS ${CMAKE_SOURCE_DIR}/.git)
        find_package(Git)
        if (GIT_FOUND)
            #get the number of commits
            execute_process(COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE NUMBER_COMMITS
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE)
            set(${version} "${default_version}.${NUMBER_COMMITS}" PARENT_SCOPE)
        elseif()
            set(${version} "${default_version}.1" PARENT_SCOPE)
        endif()
    endif()
endfunction()
