---
name: Bug report
about: Create a report to help us improve
title: "[command]: [bug report title]"
labels: bug
assignees: ''

---

Please use this form for bug reports only, for installation problems and general questions and support please consult the [community forum](http://community.mrtrix.org/search?expanded=true).

**Describe the bug**
A clear and concise description of what the bug is.

**To Reproduce**
Steps to reproduce the behaviour:

**Platform/Environment/Version (please complete the following information):**
 - OS: (ubuntu: `lsb_release -a`, macOS: `sw_vers -productVersion`) [e.g. ubuntu 16.04]
 - MRtrix version (example: `mrinfo -version`) [ e.g. 3.0_RC3-309-g0074bc6c ]

---

**Advanced debugging information (if applicable)**
- In case of a critical error such as "segmentation fault", please report the backtrace as described [here](https://mrtrix.readthedocs.io/en/latest/troubleshooting/advanced_debugging.html).
- For issues with `mrview` please provide information about the Qt version `grep "Qt:" $(dirname $(which mrview))/../config` (or open mrinfo `--> [i] --> About Qt`) and in the case of crashes or rendering issues, the information from: `mrview -exit -debug`.
- If the issue is data-dependent, please consider providing a link to the (anonymised) data to reproduce the bug.
