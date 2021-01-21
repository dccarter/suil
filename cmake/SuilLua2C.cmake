#! _LuaScript2C : This function takes a Lua script, and compiles
#  it to a Lua loadable object code
#
# \arg:name the name of the script
# \param:SCRIPT path to the lua script to convert to C (required)
# \param:TARGET the target used  to use in the variables sources name (required)
# \param:VARIABLE the variable used in the generated source code (default: \arg name)
# \param:OUTDIR the output directory for the script(default: CMAKE_CURRENT_BINARY_DIR)
# \param:BINARY path to luac binary (default: system luac)
#
function(_LuaScript name)
    set(kvargs TARGET SCRIPT VARIABLE OUTDIR BINARY)
    cmake_parse_arguments(LUA2C "" "${kvargs}" "" ${ARGN})
    if (NOT LUA2C_SCRIPT)
        message(FATAL_ERROR "suil_lua2c requires a script to convert to C file")
    endif()

    if (NOT LUA2C_TARGET)
        message(FATAL_ERROR "suil_lua2c requires a TARGET name to use in file")
    endif()

    set(SUIL_LUAC luac)
    if (LUA2C_BINARY)
        set(SUIL_LUAC ${LUA2C_BINARY})
    endif()

    if (NOT LUA2C_VARIABLE)
        set(LUA2C_VARIABLE ${name})
    endif()

    if (NOT LUA2C_OUTDIR)
        set(${name}_C_FILE ${CMAKE_CURRENT_BINARY_DIR}/${name}.c)
    else()
        set(${name}_C_FILE ${LUA2C_OUTDIR}/${name}.c)
    endif()

    message(STATUS "Add lua script ${LUA2C_SCRIPT} for compilation")
    # compile the script
    add_custom_command(
            COMMAND ${SUIL_LUAC} -o ${name}.out ${LUA2C_SCRIPT}
            OUTPUT  ${name}.out
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${LUA2C_SCRIPT})
    # dump lua object to hex file
    add_custom_command(
            COMMAND hexdump -v -e '/1 \"0x%02x, \"' ${name}.out > ${${name}_C_FILE}.hex
            OUTPUT ${${name}_C_FILE}.hex
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${name}.out)

    # convert to c file
    add_custom_command(
            COMMAND echo const char ${LUA2C_VARIABLE}[] = { `cat ${${name}_C_FILE}.hex` 0x00 } \"\\;\" const unsigned long long ${LUA2C_VARIABLE}_size = `stat -c%s ${name}.out` \"\\;\" > ${${name}_C_FILE}
            OUTPUT ${${name}_C_FILE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${${name}_C_FILE}.hex)
    set(${LUA2C_TARGET}_LUA2C_SOURCES ${${LUA2C_TARGET}_LUA2C_SOURCES} ${${name}_C_FILE} PARENT_SCOPE)
endfunction()

#! SuilLua2C : This function takes a collection of Lua scripts
#  and generates C source files for building the scripts
#
# \arg:name the name of the target that will be used for generating the scripts
# \group:SCRIPTS path to the lua scripts to convert to C sources (at least 1 required)
# \param:OUTDIR the output directory for the scripts(default: CMAKE_CURRENT_BINARY_DIR/${name})
# \param:BINARY path to luac binary (default: system luac)
#
function(SuilLua2C name)
    set(kvargs OUTDIR BINARY)
    set(kvvargs SCRIPTS)
    cmake_parse_arguments(SUIL_LUA2C "" "${kvargs}" "${kvvargs}" ${ARGN})

    if (NOT SUIL_LUA2C_SCRIPTS)
        message(FATAL_ERROR "SuilLua2C function requires at least 1 script")
    endif()

    # Default to installed luac binary
    if (NOT SUIL_LUA2C_BINARY)
        set(SUIL_LUA2C_BINARY luac)
    endif()

    # Default to CMAKE_CURRENT_BINARY_DIR/${name}
    if (NOT SUIL_LUA2C_OUTDIR)
        set(SUIL_LUA2C_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/.generated/${name}")
        file(MAKE_DIRECTORY ${SUIL_LUA2C_OUTDIR})
    endif()

    set(_SCRIPTS_DEFS  ${SUIL_LUA2C_OUTDIR}/defs.h)
    set(_SCRIPTS_DECLS ${SUIL_LUA2C_OUTDIR}/decls.h)

    file(WRITE ${_SCRIPTS_DECLS} "extern \"C\" {\n")
    file(WRITE ${_SCRIPTS_DEFS} "\n")

    set(${name}_LUA2C_SOURCES)
    foreach(SCRIPT ${SUIL_LUA2C_SCRIPTS})
        string(REGEX REPLACE "[/-]" "_" tmp_SCRIPT_NAME ${SCRIPT})
        get_filename_component(SCRIPT_NAME ${tmp_SCRIPT_NAME} NAME_WE)
        # extern script parameters into a header file that can be included when
        # building scripting code in C/C++
        file(APPEND ${_SCRIPTS_DECLS} "extern const char ${SCRIPT_NAME}[];\n")
        file(APPEND ${_SCRIPTS_DECLS} "extern size_t ${SCRIPT_NAME}_size;\n")

        # declare a script entry in a header file that can be used when building scripts list
        file(APPEND ${_SCRIPTS_DEFS} "{\"${SCRIPT_NAME}\", ${SCRIPT_NAME}_size, ${SCRIPT_NAME} },\n")

        # compile the script and generate C code
        _LuaScript(${SCRIPT_NAME}
                TARGET ${name}
                BINARY ${SUIL_LUA2C_BINARY}
                SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT}
                OUTDIR ${SUIL_LUA2C_OUTDIR})
    endforeach()

    # terminate header file
    file(APPEND ${_SCRIPTS_DECLS} "};\n")
    file(APPEND ${_SCRIPTS_DEFS} "{nullptr, 0, nullptr },\n")

    set(${name}_LUA2C_SOURCES ${${name}_LUA2C_SOURCES} PARENT_SCOPE)
endfunction()