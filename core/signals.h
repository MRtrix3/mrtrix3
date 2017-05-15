/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifdef SIGALRM
    __SIGNAL (SIGALRM, "Timer expiration");
#endif
#ifdef SIGBUS
    __SIGNAL (SIGBUS, "Bus error: Accessing invalid address (out of storage space?)");
#endif
#ifdef SIGFPE
    __SIGNAL (SIGFPE, "Floating-point arithmetic exception");
#endif
#ifdef SIGHUP
    __SIGNAL (SIGHUP, "Disconnection of terminal");
#endif
#ifdef SIGILL // Note: Not generated under Windows
    __SIGNAL (SIGILL, "Illegal instruction (corrupt binary command file?)");
#endif
#ifdef SIGINT // Note: Not supported for any Win32 application
    __SIGNAL (SIGINT, "Program manually interrupted by terminal");
#endif
#ifdef SIGPIPE
    __SIGNAL (SIGPIPE, "Nothing on receiving end of pipe");
#endif
#ifdef SIGPWR
    __SIGNAL (SIGPWR, "Power failure restart");
#endif
#ifdef SIGQUIT
    __SIGNAL (SIGQUIT, "Received terminal quit signal");
#endif
#ifdef SIGSEGV
    __SIGNAL (SIGSEGV, "Segmentation fault: Invalid memory access");
#endif
#ifdef SIGSYS
    __SIGNAL (SIGSYS, "Bad system call");
#endif
#ifdef SIGTERM // Note: Not generated under Windows
    __SIGNAL (SIGTERM, "Terminated by kill command");
#endif
#ifdef SIGXCPU
    __SIGNAL (SIGXCPU, "CPU time limit exceeded");
#endif
#ifdef SIGXFSZ
    __SIGNAL (SIGXFSZ, "File size limit exceeded");
#endif

