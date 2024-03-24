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

from . import HIPPOCAMPI_CHOICES, THALAMI_CHOICES

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('hsvs', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Generate a 5TT image based on Hybrid Surface and Volume Segmentation (HSVS), using FreeSurfer and FSL tools')
  parser.add_argument('input', help='The input FreeSurfer subject directory')
  parser.add_argument('output', help='The output 5TT image')
  parser.add_argument('-template', help='Provide an image that will form the template for the generated 5TT image')
  parser.add_argument('-hippocampi', choices=HIPPOCAMPI_CHOICES, help='Select method to be used for hippocampi (& amygdalae) segmentation; options are: ' + ','.join(HIPPOCAMPI_CHOICES))
  parser.add_argument('-thalami', choices=THALAMI_CHOICES, help='Select method to be used for thalamic segmentation; options are: ' + ','.join(THALAMI_CHOICES))
  parser.add_argument('-white_stem', action='store_true', help='Classify the brainstem as white matter')
  parser.add_citation('Smith, R.; Skoch, A.; Bajada, C.; Caspers, S.; Connelly, A. Hybrid Surface-Volume Segmentation for improved Anatomically-Constrained Tractography. In Proc OHBM 2020')
  parser.add_citation('Fischl, B. Freesurfer. NeuroImage, 2012, 62(2), 774-781', is_external=True)
  parser.add_citation('Iglesias, J.E.; Augustinack, J.C.; Nguyen, K.; Player, C.M.; Player, A.; Wright, M.; Roy, N.; Frosch, M.P.; Mc Kee, A.C.; Wald, L.L.; Fischl, B.; and Van Leemput, K. A computational atlas of the hippocampal formation using ex vivo, ultra-high resolution MRI: Application to adaptive segmentation of in vivo MRI. NeuroImage, 2015, 115, 117-137', condition='If FreeSurfer hippocampal subfields module is utilised', is_external=True)
  parser.add_citation('Saygin, Z.M. & Kliemann, D.; Iglesias, J.E.; van der Kouwe, A.J.W.; Boyd, E.; Reuter, M.; Stevens, A.; Van Leemput, K.; Mc Kee, A.; Frosch, M.P.; Fischl, B.; Augustinack, J.C. High-resolution magnetic resonance imaging reveals nuclei of the human amygdala: manual segmentation to automatic atlas. NeuroImage, 2017, 155, 370-382', condition='If FreeSurfer hippocampal subfields module is utilised and includes amygdalae segmentation', is_external=True)
  parser.add_citation('Iglesias, J.E.; Insausti, R.; Lerma-Usabiaga, G.; Bocchetta, M.; Van Leemput, K.; Greve, D.N.; van der Kouwe, A.; ADNI; Fischl, B.; Caballero-Gaudes, C.; Paz-Alonso, P.M. A probabilistic atlas of the human thalamic nuclei combining ex vivo MRI and histology. NeuroImage, 2018, 183, 314-326', condition='If -thalami nuclei is used', is_external=True)
  parser.add_citation('Ardekani, B.; Bachman, A.H. Model-based automatic detection of the anterior and posterior commissures on MRI scans. NeuroImage, 2009, 46(3), 677-682', condition='If ACPCDetect is installed', is_external=True)
