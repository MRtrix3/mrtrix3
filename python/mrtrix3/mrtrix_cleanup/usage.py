# Copyright (c) 2008-2023 the MRtrix3 contributors.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Covered Software is provided under this License on an "as is"
# basis, without warranty of any kind, either expressed, implied, or
# statutory, including, without limitation, warranties that the
# Covered Software is free of defects, merchantable, fit for a
# particular purpose or non-infringing.
# See the Mozilla Public License v. 2.0 for more details.
#
# For more details, see http://www.mrtrix.org/.

def usage(cmdline): #pylint: disable=unused-variable
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Clean up residual temporary files & scratch directories from MRtrix3 commands')
  cmdline.add_description('This script will search the file system at the specified location (and in sub-directories thereof) for any temporary files or directories that have been left behind by failed or terminated MRtrix3 commands, and attempt to delete them.')
  cmdline.add_description('Note that the script\'s search for temporary items will not extend beyond the user-specified filesystem location. This means that any built-in or user-specified default location for MRtrix3 piped data and scripts will not be automatically searched. Cleanup of such locations should instead be performed explicitly: e.g. "mrtrix_cleanup /tmp/" to remove residual piped images from /tmp/.')
  cmdline.add_description('This script should not be run while other MRtrix3 commands are being executed: it may delete temporary items during operation that may lead to unexpected behaviour.')
  cmdline.add_argument('path', help='Path from which to commence filesystem search')
  cmdline.add_argument('-test', action='store_true', help='Run script in test mode: will list identified files / directories, but not attempt to delete them')
  cmdline.add_argument('-failed', metavar='file', nargs=1, help='Write list of items that the script failed to delete to a text file')
  cmdline.flag_mutually_exclusive_options([ 'test', 'failed' ])
