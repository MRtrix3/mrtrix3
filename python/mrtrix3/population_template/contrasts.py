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

import os
from mrtrix3 import MRtrixError
from mrtrix3 import app, path

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

      mc_weight_<mode>: list of str
        contrast-specific weight used during initialisation / registration

      <mode>_weight_option: list of str
        weight option to be passed to mrregister, <mode> = {'initial_alignment', 'rigid', 'affine', 'nl'}

      n_contrasts: int

      """

  def __init__(self):

    n_contrasts = len(app.ARGS.input_dir)

    self.suff = ["_c" + c for c in map(str, range(n_contrasts))]
    self.names = [os.path.relpath(f, os.path.commonprefix(app.ARGS.input_dir)) for f in app.ARGS.input_dir]

    self.templates_out = [path.from_user(t, True) for t in app.ARGS.template]

    self.mc_weight_initial_alignment = [None for _ in range(self.n_contrasts)]
    self.mc_weight_rigid = [None for _ in range(self.n_contrasts)]
    self.mc_weight_affine = [None for _ in range(self.n_contrasts)]
    self.mc_weight_nl = [None for _ in range(self.n_contrasts)]
    self.initial_alignment_weight_option = [None for _ in range(self.n_contrasts)]
    self.rigid_weight_option = [None for _ in range(self.n_contrasts)]
    self.affine_weight_option = [None for _ in range(self.n_contrasts)]
    self.nl_weight_option = [None for _ in range(self.n_contrasts)]

    self.isfinite_count = ['isfinite' + c + '.mif' for c in self.suff]
    self.templates = [None for _ in range(self.n_contrasts)]
    self.n_volumes = [None for _ in range(self.n_contrasts)]
    self.fod_reorientation = [None for _ in range(self.n_contrasts)]


    for mode in ['initial_alignment', 'rigid', 'affine', 'nl']:
      opt = app.ARGS.__dict__.get('mc_weight_' + mode, None)
      if opt:
        if n_contrasts == 1:
          raise MRtrixError('mc_weight_' + mode+' requires multiple input contrasts')
        opt = opt.split(',')
        if len(opt) != n_contrasts:
          raise MRtrixError('mc_weight_' + mode+' needs to be defined for each contrast')
      else:
        opt = ["1"] * n_contrasts
      self.__dict__['mc_weight_%s' % mode] = opt
      self.__dict__['%s_weight_option' % mode] = ' -mc_weights '+','.join(opt)+' ' if n_contrasts > 1 else ''

    if len(self.templates_out) != n_contrasts:
      raise MRtrixError('number of templates (%i) does not match number of input directories (%i)' %
                        (len(self.templates_out), n_contrasts))

  @property
  def n_contrasts(self):
    return len(self.suff)

  def __repr__(self, *args, **kwargs):
    text = ''
    for cid in range(self.n_contrasts):
      text += '\tcontrast: %s, template: %s, suffix: %s\n' % (self.names[cid], self.templates_out[cid], self.suff[cid])
    return text
