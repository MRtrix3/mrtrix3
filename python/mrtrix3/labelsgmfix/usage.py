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
  cmdline.set_synopsis('In a FreeSurfer parcellation image, replace the sub-cortical grey matter structure delineations using FSL FIRST')
  cmdline.add_citation('Patenaude, B.; Smith, S. M.; Kennedy, D. N. & Jenkinson, M. A Bayesian model of shape and appearance for subcortical brain segmentation. NeuroImage, 2011, 56, 907-922', is_external=True)
  cmdline.add_citation('Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219', is_external=True)
  cmdline.add_citation('Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. The effects of SIFT on the reproducibility and biological accuracy of the structural connectome. NeuroImage, 2015, 104, 253-265')
  cmdline.add_argument('parc',   help='The input FreeSurfer parcellation image')
  cmdline.add_argument('t1',     help='The T1 image to be provided to FIRST')
  cmdline.add_argument('lut',    help='The lookup table file that the parcellated image is based on')
  cmdline.add_argument('output', help='The output parcellation image')
  cmdline.add_argument('-premasked', action='store_true', default=False, help='Indicate that brain masking has been applied to the T1 input image')
  cmdline.add_argument('-sgm_amyg_hipp', action='store_true', default=False, help='Consider the amygdalae and hippocampi as sub-cortical grey matter structures, and also replace their estimates with those from FIRST')
