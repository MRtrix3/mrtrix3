# Copyright (c) 2008-2024 the MRtrix3 contributors.
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

import os

from mrtrix3 import MRtrixError
from mrtrix3 import app



class Contrasts: # pylint: disable=unused-variable
  """
      Class that parses arguments and holds information specific to each image contrast

      Attributes
      ----------
      suff: list of str
        identifiers used for contrast-specific filenames and folders ['_c0', '_c1', ...]

      names: list of str
        derived from constrast-specific input folder

      templates_out: list of str
        full path to output templates

      templates: list of str
        holds current template names during registration

      n_volumes: list of int
        number of volumes in each contrast

      fod_reorientation: list of bool
        whether to perform FOD reorientation with mrtransform

      isfinite_count: list of str
        filenames of images holding (weighted) number of finite-valued voxels across all images

      mc_weight_<mode>: list of floats
        contrast-specific weight used during initialisation / registration

      <mode>_weight_option: list of str
        weight option to be passed to mrregister, <mode> = {'initial_alignment', 'rigid', 'affine', 'nl'}

      n_contrasts: int

      """


  def __init__(self):
    n_contrasts = len(app.ARGS.input_dir)
    self.suff = [f'_c{c}' for c in map(str, range(n_contrasts))]
    self.names = [os.path.relpath(f, os.path.commonprefix(app.ARGS.input_dir)) for f in app.ARGS.input_dir]

    self.templates_out = list(app.ARGS.template)

    self.mc_weight_initial_alignment = [None for _ in range(self.n_contrasts)]
    self.mc_weight_rigid = [None for _ in range(self.n_contrasts)]
    self.mc_weight_affine = [None for _ in range(self.n_contrasts)]
    self.mc_weight_nl = [None for _ in range(self.n_contrasts)]
    self.initial_alignment_weight_option = [None for _ in range(self.n_contrasts)]
    self.rigid_weight_option = [None for _ in range(self.n_contrasts)]
    self.affine_weight_option = [None for _ in range(self.n_contrasts)]
    self.nl_weight_option = [None for _ in range(self.n_contrasts)]

    self.isfinite_count = [f'isfinite{c}.mif' for c in self.suff]
    self.templates = [None for _ in range(self.n_contrasts)]
    self.n_volumes = [None for _ in range(self.n_contrasts)]
    self.fod_reorientation = [None for _ in range(self.n_contrasts)]


    for mode in ['initial_alignment', 'rigid', 'affine', 'nl']:
      opt = app.ARGS.__dict__.get(f'mc_weight_{mode}', None)
      if opt:
        if n_contrasts == 1:
          raise MRtrixError(f'mc_weight_{mode} requires multiple input contrasts')
        if len(opt) != n_contrasts:
          raise MRtrixError(f'mc_weight_{mode} needs to be defined for each contrast')
      else:
        opt = [1.0] * n_contrasts
      self.__dict__[f'mc_weight_{mode}'] = opt
      self.__dict__[f'{mode}_weight_option'] = f' -mc_weights {",".join(map(str, opt))}' if n_contrasts > 1 else ''

    if len(app.ARGS.template) != n_contrasts:
      raise MRtrixError(f'number of templates ({len(app.ARGS.template)}) '
                        f'does not match number of input directories ({n_contrasts})')

  @property
  def n_contrasts(self):
    return len(self.suff)

  def __repr__(self, *args, **kwargs):
    text = ''
    for cid in range(self.n_contrasts):
      text += f'\tcontrast: {self.names[cid]}, suffix: {self.suff[cid]}\n'
    return text
