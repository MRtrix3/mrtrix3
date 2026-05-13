/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifdef SIGALRM
MRTRIX_MANIP_SIGNAL(SIGALRM, "Timer expiration");
#endif
#ifdef SIGBUS
MRTRIX_MANIP_SIGNAL(SIGBUS, "Bus error: Accessing invalid address (out of storage space?)");
#endif
#ifdef SIGFPE
MRTRIX_MANIP_SIGNAL(SIGFPE, "Floating-point arithmetic exception");
#endif
#ifdef SIGHUP
MRTRIX_MANIP_SIGNAL(SIGHUP, "Disconnection of terminal");
#endif
#ifdef SIGILL // Note: Not generated under Windows
MRTRIX_MANIP_SIGNAL(SIGILL, "Illegal instruction (corrupt binary command file?)");
#endif
#ifdef SIGINT // Note: Not supported for any Win32 application
MRTRIX_MANIP_SIGNAL(SIGINT, "Program manually interrupted by terminal");
#endif
#ifdef SIGPIPE
MRTRIX_MANIP_SIGNAL(SIGPIPE, "Nothing on receiving end of pipe");
#endif
#ifdef SIGPWR
MRTRIX_MANIP_SIGNAL(SIGPWR, "Power failure restart");
#endif
#ifdef SIGQUIT
MRTRIX_MANIP_SIGNAL(SIGQUIT, "Received terminal quit signal");
#endif
#ifdef SIGSEGV
MRTRIX_MANIP_SIGNAL(SIGSEGV, "Segmentation fault: Invalid memory access");
#endif
#ifdef SIGSYS
MRTRIX_MANIP_SIGNAL(SIGSYS, "Bad system call");
#endif
#ifdef SIGTERM // Note: Not generated under Windows
MRTRIX_MANIP_SIGNAL(SIGTERM, "Terminated by kill command");
#endif
#ifdef SIGXCPU
MRTRIX_MANIP_SIGNAL(SIGXCPU, "CPU time limit exceeded");
#endif
#ifdef SIGXFSZ
MRTRIX_MANIP_SIGNAL(SIGXFSZ, "File size limit exceeded");
#endif
