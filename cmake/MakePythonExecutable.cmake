# Creates a short Python executable that is used to run a Python command from the terminal.
# Inputs:
#   - CMDNAME: Name of the command
#   - OUTPUT_DIR: Directory in which to create the executable
#   - PACKAGE: Python package providing the command (default "mrtrix3"); for an external
#       MRtrix3 project this is the project's own package name, e.g. "myproject".
#   - SRCDIR: Directory containing the command source (default: current working directory);
#       used only to detect the standalone-file vs. sub-directory command layout.
#   - EXTERNAL_COMMANDS: (external projects only) semicolon-separated list of the project's own
#       command names. When provided, the launcher scopes command resolution to the project's
#       own executables: it exports MRTRIX_EXECUTABLES_PATH (so run.command() resolves invoked
#       MRtrix3/project commands from the project's own executables directory rather than $PATH)
#       and MRTRIX_EXTRA_COMMANDS (so the project's own commands are treated as version-matched).
#       This is what guarantees that invoking the project does not shadow a separate MRtrix3
#       installation on the user's $PATH.

if(NOT DEFINED PACKAGE)
    set(PACKAGE "mrtrix3")
endif()
if(NOT DEFINED SRCDIR)
    set(SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}")
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

# For an external project, scope command resolution to the project's own executables.
# os.environ.setdefault() ensures the outermost project command wins and nested invocations inherit.
if(DEFINED EXTERNAL_COMMANDS)
    string(REPLACE ";" "," EXTERNAL_COMMANDS_CSV "${EXTERNAL_COMMANDS}")
    string(APPEND BINPATH_CONTENTS
        "os.environ.setdefault('MRTRIX_EXECUTABLES_PATH', _self_dir)\n"
        "os.environ.setdefault('MRTRIX_EXTRA_COMMANDS', '${EXTERNAL_COMMANDS_CSV}')\n"
    )
endif()

# Locate the installation's Python library directory by walking up from this executable until a
# directory containing the '${PACKAGE}' package is found. This makes the launcher work whether it
# resides in '<prefix>/bin' (sibling 'lib') or a nested location such as
# '<prefix>/libexec/<project>/bin' (lib several levels up), and identically in the build tree.
string(APPEND BINPATH_CONTENTS
    "_probe = _self_dir\n"
    "_lib_dir = None\n"
    "for _ in range(8):\n"
    "    _candidate = os.path.join(_probe, 'lib')\n"
    "    if os.path.isdir(os.path.join(_candidate, '${PACKAGE}')):\n"
    "        _lib_dir = _candidate\n"
    "        break\n"
    "    _parent = os.path.dirname(_probe)\n"
    "    if _parent == _probe:\n"
    "        break\n"
    "    _probe = _parent\n"
    "if _lib_dir is None:\n"
    "    sys.stderr.write('ERROR: unable to locate ${PACKAGE} Python library relative to ' + _self_dir + '\\n')\n"
    "    sys.exit(1)\n"
    "sys.path.insert(0, _lib_dir)\n"
    "from ${PACKAGE}.app import _execute\n"
    "\n"
)

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
