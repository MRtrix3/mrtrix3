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

from . import DEFAULT_SOFTWARE
from . import SOFTWARES

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('b02template', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Register the mean b=0 image to a T2-weighted template to back-propagate a brain mask')
  parser.add_description('This script currently assumes that the template image provided via the -template option '
                         'is T2-weighted, and can therefore be trivially registered to a mean b=0 image.')
  parser.add_description('Command-line option -ants_options can be used with either the "antsquick" or "antsfull" software options. '
                         'In both cases, image dimensionality is assumed to be 3, and so this should be omitted from the user-specified options.'
                         'The input can be either a string (encased in double-quotes if more than one option is specified), or a path to a text file containing the requested options. '
                         'In the case of the "antsfull" software option, one will require the names of the fixed and moving images that are provided to the antsRegistration command: these are "template_image.nii" and "bzero.nii" respectively.')
  parser.add_citation('M. Jenkinson, C.F. Beckmann, T.E. Behrens, M.W. Woolrich, S.M. Smith. FSL. NeuroImage, 62:782-90, 2012',
                      condition='If FSL software is used for registration',
                      is_external=True)
  parser.add_citation('B. Avants, N.J. Tustison, G. Song, P.A. Cook, A. Klein, J.C. Jee. A reproducible evaluation of ANTs similarity metric performance in brain image registration. NeuroImage, 2011, 54, 2033-2044',
                      condition='If ANTs software is used for registration',
                      is_external=True)
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the "template" algorithm')
  options.add_argument('-software', choices=SOFTWARES, help='The software to use for template registration; options are: ' + ','.join(SOFTWARES) + '; default is ' + DEFAULT_SOFTWARE)
  options.add_argument('-template', metavar='TemplateImage MaskImage', nargs=2, help='Provide the template image to which the input data will be registered, and the mask to be projected to the input image. The template image should be T2-weighted.')
  ants_options = parser.add_argument_group('Options applicable when using the ANTs software for registration')
  ants_options.add_argument('-ants_options', help='Provide options to be passed to the ANTs registration command (see Description)')
  fsl_options = parser.add_argument_group('Options applicable when using the FSL software for registration')
  fsl_options.add_argument('-flirt_options', metavar='" FlirtOptions"', help='Command-line options to pass to the FSL flirt command (provide a string within quotation marks that contains at least one space, even if only passing a single command-line option to flirt)')
  fsl_options.add_argument('-fnirt_config', metavar='FILE', help='Specify a FNIRT configuration file for registration')
