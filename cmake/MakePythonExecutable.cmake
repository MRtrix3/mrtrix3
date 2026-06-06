# Creates a short Python executable that is used to run a Python command from the terminal.
# Inputs:
#   - CMDNAME: Name of the command
#   - OUTPUT_DIR: Directory in which to create the executable
#   - PACKAGE: Python package providing the command (default "mrtrix3"); for an external
#       MRtrix3 project this is the project's own package name, e.g. "myproject".
#   - SRCDIR: Directory containing the command source (default: current working directory);
#       used only to detect the standalone-file vs. sub-directory command layout.
#   - MRTRIX_LIB_RELPATH: forward-slash relative path from the launcher's directory to the directory
#       containing the mrtrix3 Python package's parent ('lib'). Default "../lib" (the self-contained
#       MRtrix3 layout). An external project nesting MRtrix3 passes e.g. "../mrtrix3/lib".
#   - PROJECT_LIB_RELPATH: (external projects) relative path from the launcher's directory to the
#       directory containing the project's own Python package. Default "../lib".
#   - EXTERNAL_COMMANDS: (external projects only) semicolon-separated list of the project's own
#       command names. When provided, the launcher runs in external-project mode: it adds both the
#       project and mrtrix3 library directories to sys.path, and exports MRTRIX_EXTRA_COMMANDS and
#       MRTRIX_EXTRA_EXECUTABLES_PATH so that run.command() resolves the project's own commands as
#       version-matched (in addition to MRtrix3's, which resolve via the nested mrtrix3 package).

if(NOT DEFINED PACKAGE)
    set(PACKAGE "mrtrix3")
endif()
if(NOT DEFINED SRCDIR)
    set(SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
if(NOT DEFINED MRTRIX_LIB_RELPATH)
    set(MRTRIX_LIB_RELPATH "../lib")
endif()
if(NOT DEFINED PROJECT_LIB_RELPATH)
    set(PROJECT_LIB_RELPATH "../lib")
endif()

set(BINPATH_CONTENTS
    "#!/usr/bin/env python3\n"
    "# -*- coding: utf-8 -*-\n"
    "\n"
    "import importlib\n"
    "import os\n"
    "import sys\n"
    "\n"
    "_self_dir = os.path.dirname(os.path.realpath(__file__))\n"
)

if(DEFINED EXTERNAL_COMMANDS)
    # External-project mode. MRtrix3 commands are resolved by the (nested) mrtrix3 package's baked
    # executables path; the project's own commands are added via the env vars below. setdefault()
    # ensures the outermost project command wins and nested invocations inherit.
    string(REPLACE ";" "," EXTERNAL_COMMANDS_CSV "${EXTERNAL_COMMANDS}")
    string(APPEND BINPATH_CONTENTS
        "os.environ.setdefault('MRTRIX_EXTRA_COMMANDS', '${EXTERNAL_COMMANDS_CSV}')\n"
        "os.environ.setdefault('MRTRIX_EXTRA_EXECUTABLES_PATH', _self_dir)\n"
        "sys.path.insert(0, os.path.normpath(os.path.join(_self_dir, '${MRTRIX_LIB_RELPATH}')))\n"
        "sys.path.insert(0, os.path.normpath(os.path.join(_self_dir, '${PROJECT_LIB_RELPATH}')))\n"
        "from ${PACKAGE}.app import _execute\n"
        "\n"
    )
else()
    string(APPEND BINPATH_CONTENTS
        "sys.path.insert(0, os.path.normpath(os.path.join(_self_dir, '${MRTRIX_LIB_RELPATH}')))\n"
        "from ${PACKAGE}.app import _execute\n"
        "\n"
    )
endif()

# Two possible interfaces:
#   1. Standalone file residing in commands/
#   2. File stored in location commands/<cmdname>/<cmdname>.py, which will contain usage() and execute() functions
if(EXISTS "${SRCDIR}/${CMDNAME}/__init__.py")
    if(EXISTS "${SRCDIR}/${CMDNAME}/${CMDNAME}.py")
        string(APPEND BINPATH_CONTENTS
            "module = importlib.import_module('.${CMDNAME}', '${PACKAGE}.commands.${CMDNAME}')\n"
        )
    else()
        message(FATAL_ERROR "Malformed filesystem structure for Python command ${CMDNAME}")
    endif()
elseif(EXISTS "${SRCDIR}/${CMDNAME}.py")
    string(APPEND BINPATH_CONTENTS
        "module = importlib.import_module('.${CMDNAME}', '${PACKAGE}.commands')\n"
    )
else()
    message(FATAL_ERROR "Malformed filesystem structure for Python command ${CMDNAME}")
endif()
string(APPEND BINPATH_CONTENTS
    "_execute(module.usage, module.execute)\n"
)


file(WRITE ${OUTPUT_DIR}/${CMDNAME} ${BINPATH_CONTENTS})
file(CHMOD ${OUTPUT_DIR}/${CMDNAME} FILE_PERMISSIONS
    OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)
