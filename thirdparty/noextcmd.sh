# Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#!/bin/bash

echo "You are attempting to execute from within the MRtrix3 container" \
     "an MRtrix3 Python script that itself invokes a command" \
     "from another neuroimaging software package." \
     "This external neuroimaging software package is however not bundled" \
     " within the official MRtrix3 container." \
     "If you require this specific functionality," \
     "you may consider one of the following options:"
echo "- Configuring for running MRtrix3 a native environment" \
     "that incorporates not only MRtrix3 but also other neuroimaging packages."
echo "- Utilising the alternative container \"mrtrix3_with3p\"," \
     "within which is installed both MRtrix3 and the third-party neuroimaging software package " \
     "from which the required command originates: " \
     "https://github.com/MRtrix3/mrtrix3-with3p"
echo "- Utilising any other alternative container environment" \
     "that encompasses multiple nr=euroimaging software packages" \
     "including both MRtrix3 and the required third-party dependency."
echo "For more information please see:"
echo "https://mrtrix.readthedocs.io/en/latest/installation/third_party_software.html"

exit 1
