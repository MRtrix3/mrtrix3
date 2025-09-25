# Creates a short Python executable that is used to run a Python command from the terminal.
# Inputs:
#   - CMDNAME: Name of the command
#   - OUTPUT_DIR: Directory in which to create the executable

set(BINPATH_CONTENTS
    "#!/usr/bin/env python3\n"
    "# -*- coding: utf-8 -*-\n"
    "\n"
    "import importlib\n"
    "import os\n"
    "import sys\n"
    "\n"
    "mrtrix_lib_path = os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, 'lib'))\n"
    "sys.path.insert(0, mrtrix_lib_path)\n"
    "from mrtrix3.app import _execute\n"
    "\n"
)

# Two possible interfaces:
#   1. Standalone file residing in commands/
#   2. File stored in location commands/<cmdname>/<cmdname>.py, which will contain usage() and execute() functions
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${CMDNAME}/__init__.py")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${CMDNAME}/${CMDNAME}.py")
        string(APPEND BINPATH_CONTENTS
            "module = importlib.import_module('.${CMDNAME}', 'mrtrix3.commands.${CMDNAME}')\n"
        )
    else()
        message(FATAL_ERROR "Malformed filesystem structure for Python command ${CMDNAME}")
    endif()
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${CMDNAME}.py")
    string(APPEND BINPATH_CONTENTS
        "module = importlib.import_module('.${CMDNAME}', 'mrtrix3.commands')\n"
    )
else()
    message(FATAL_ERROR "Malformed filesystem structure for Python command ${CMDNAME}")
endif()
string(APPEND BINPATH_CONTENTS
    "_execute(module.usage, module.execute)\n"
)


if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
    file(WRITE ${OUTPUT_DIR}/${CMDNAME} ${BINPATH_CONTENTS})
    file(CHMOD ${OUTPUT_DIR}/${CMDNAME} FILE_PERMISSIONS
        OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
    )
else()
    set(tmp_output_file "${CMAKE_CURRENT_BINARY_DIR}/tmp_${CMDNAME}")
    file(WRITE ${tmp_output_file} ${BINPATH_CONTENTS})
    file(COPY ${tmp_output_file} DESTINATION ${OUTPUT_DIR}
        FILE_PERMISSIONS
            OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
    )
    file(RENAME ${OUTPUT_DIR}/tmp_${CMDNAME} ${OUTPUT_DIR}/${CMDNAME})
    file(REMOVE ${tmp_output_file})
endif()
