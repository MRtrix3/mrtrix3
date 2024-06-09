# Creates within the bin/ sub-directory of the project build directory
#   a short Python executable that is used to run a Python command from the terminal
# Receives name of the command as ${CMDNAME}; output build directory as ${BUILDDIR}
set(BINPATH "${BUILDDIR}/temporary/python/${CMDNAME}")
file(WRITE ${BINPATH} "#!/usr/bin/python3\n")
file(APPEND ${BINPATH} "# -*- coding: utf-8 -*-\n")
file(APPEND ${BINPATH} "\n")
file(APPEND ${BINPATH} "import importlib\n")
file(APPEND ${BINPATH} "import os\n")
file(APPEND ${BINPATH} "import sys\n")
file(APPEND ${BINPATH} "\n")
file(APPEND ${BINPATH} "mrtrix_lib_path = os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, 'lib'))\n")
file(APPEND ${BINPATH} "sys.path.insert(0, mrtrix_lib_path)\n")
file(APPEND ${BINPATH} "from mrtrix3.app import _execute\n")
# Three possible interfaces:
#   1. Standalone file residing in commands/
#   2. File stored in location commands/<cmdname>/<cmdname>.py, which will contain usage() and execute() functions
#   3. Two files stored at commands/<cmdname>/usage.py and commands/<cmdname>/execute.py, defining the two corresponding functions
# TODO Port population_template to type 3; both for readability and to ensure that it works
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${CMDNAME}/__init__.py")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${CMDNAME}/usage.py" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${CMDNAME}/execute.py")
        file(APPEND ${BINPATH} "module_usage = importlib.import_module('.usage', 'mrtrix3.commands.${CMDNAME}')\n")
        file(APPEND ${BINPATH} "module_execute = importlib.import_module('.execute', 'mrtrix3.commands.${CMDNAME}')\n")
        file(APPEND ${BINPATH} "_execute(module_usage.usage, module_execute.execute)\n")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${CMDNAME}/${CMDNAME}.py")
        file(APPEND ${BINPATH} "module = importlib.import_module('.${CMDNAME}', 'mrtrix3.commands.${CMDNAME}')\n")
        file(APPEND ${BINPATH} "_execute(module.usage, module.execute)\n")
    else()
        message(FATAL_ERROR "Malformed filesystem structure for Python command ${CMDNAME}")
    endif()
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${CMDNAME}.py")
    file(APPEND ${BINPATH} "module = importlib.import_module('.${CMDNAME}', 'mrtrix3.commands')\n")
    file(APPEND ${BINPATH} "_execute(module.usage, module.execute)\n")
else()
    message(FATAL_ERROR "Malformed filesystem structure for Python command ${CMDNAME}")
endif()
file(COPY ${BINPATH} DESTINATION ${BUILDDIR}/bin
    FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)

