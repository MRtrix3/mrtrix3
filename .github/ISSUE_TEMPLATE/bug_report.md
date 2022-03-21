---
name: Bug report
about: Create a report to help us improve
title: "[command]: [bug report title]"
labels: bug
assignees: ''

---

Please use this form for *bug reports only*; for installation problems, and general questions and support, please consult the [community forum](http://community.mrtrix.org/search?expanded=true).

**Describe the bug**

A clear and concise description of what the bug is; including if appropriate
how the observed behaviour differs from that expected.

**To Reproduce**

Steps to reproduce the behaviour.

If the issue is data-dependent, please consider providing a link to
(anonymised) data to assist developers in reproducing the bug.

**Platform/Environment/Version**

Please provide the following information:

-  OS: (ubuntu: `lsb_release -a`, macOS: `sw_vers -productVersion`) [e.g. Ubuntu 16.04]

-  *MRtrix3* version (example: `mrinfo -version`) [ e.g. `3.0_RC3-309-g0074bc6c` ]

---

**Advanced debugging information (if applicable)**

-  In case of a critical error such as "segmentation fault", please generate
   and report the backtrace as described [here](https://community.mrtrix.org/t/advanced-debugging-of-mrtrix3-binaries).

- For issues with `mrview`, please provide:

   -  Information about the Qt version; one of the following:

      -  `grep "Qt:" $(dirname $(which mrview))/../config`

      -  Open `mrview` --> `[i]` --> About Qt

   -  In the case of crashes or rendering issues, the information from:
      `mrview -exit -debug`.
