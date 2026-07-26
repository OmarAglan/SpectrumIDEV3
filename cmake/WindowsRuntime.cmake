# Put the compiler runtime selected at configure time beside each native
# Windows executable. The application directory is searched before PATH, so a
# foreign MinGW installation can no longer supply incompatible DLLs.
function(qalam_harden_windows_runtime target_name)
    if(NOT WIN32 OR NOT MINGW)
        return()
    endif()

    get_filename_component(qalam_compiler_runtime_dir
        "${CMAKE_CXX_COMPILER}" DIRECTORY)

    set(qalam_compiler_runtime_dlls)
    foreach(runtime_name
            libgcc_s_seh-1.dll
            libgcc_s_dw2-1.dll
            libstdc++-6.dll
            libwinpthread-1.dll)
        set(runtime_path
            "${qalam_compiler_runtime_dir}/${runtime_name}")
        if(EXISTS "${runtime_path}")
            list(APPEND qalam_compiler_runtime_dlls "${runtime_path}")
        endif()
    endforeach()

    if(NOT qalam_compiler_runtime_dlls)
        message(FATAL_ERROR
            "No MinGW runtime DLLs were found beside "
            "${CMAKE_CXX_COMPILER}. The selected Windows toolchain is incomplete.")
    endif()

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${qalam_compiler_runtime_dlls}
            "$<TARGET_FILE_DIR:${target_name}>"
        COMMENT "Deploying the selected MinGW runtime for ${target_name}"
        VERBATIM
        COMMAND_EXPAND_LISTS
    )
endfunction()
