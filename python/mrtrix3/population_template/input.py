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

class Input: # pylint: disable=unused-variable
  """
      Class that holds input information specific to a single image (multiple contrasts)

      Attributes
      ----------
      uid: str
        unique identifier for these input image(s), does not contain spaces

      ims_path: list of str
        full path to input images, shell quoted OR paths to cached file if cache_local was called

      msk_path: str
        full path to input mask, shell quoted OR path to cached file if cache_local was called

      ims_filenames : list of str
        for each contrast the input file paths stripped of their respective directories. Used for final output only.

      msk_filename: str
        as ims_filenames

      ims_transformed: list of str
        input_transformed<contrast identifier>/<uid>.mif

      msk_transformed: list of str
        mask_transformed/<uid>.mif

      aggregation_weight: float
        weights used in image aggregation that forms the template. Has to be normalised across inputs.

      _im_directories : list of str
        full path to user-provided input directories containing the input images, one for each contrast

      _msk_directory: str
        full path to user-provided mask directory

      _local_ims: list of str
        path to cached input images

      _local_msk: str
        path to cached input mask

      Methods
      -------
      cache_local()
        copy files into folders in current working directory. modifies _local_ims and  _local_msk

      """
  def __init__(self, uid, filenames, directories, contrasts, mask_filename='', mask_directory=''):
    self.contrasts = contrasts

    self.uid = uid
    assert self.uid, "UID empty"
    assert self.uid.count(' ') == 0, 'UID "%s" contains whitespace' % self.uid

    assert len(directories) == len(filenames)
    self.ims_filenames = filenames
    self._im_directories = directories

    self.msk_filename = mask_filename
    self._msk_directory = mask_directory

    n_contrasts = len(contrasts)

    self.ims_transformed = [os.path.join('input_transformed'+contrasts[cid], uid + '.mif') for cid in range(n_contrasts)]
    self.msk_transformed = os.path.join('mask_transformed', uid + '.mif')

    self.aggregation_weight = None

    self._local_ims = []
    self._local_msk = None

  def __repr__(self, *args, **kwargs):
    text = '\nInput ['
    for key in sorted([k for k in self.__dict__ if not k.startswith('_')]):
      text += '\n\t' + str(key) + ': ' + str(self.__dict__[key])
    text += '\n]'
    return text

  def info(self):
    message = ['input: ' + self.uid]
    if self.aggregation_weight:
      message += ['agg weight: ' + self.aggregation_weight]
    for csuff, fname in zip(self.contrasts, self.ims_filenames):
      message += [((csuff + ': ') if csuff else '') + '"' + fname + '"']
    if self.msk_filename:
      message += ['mask: ' + self.msk_filename]
    return ', '.join(message)

  def cache_local(self):
    from mrtrix3 import run, path  # pylint: disable=no-name-in-module, import-outside-toplevel
    contrasts = self.contrasts
    for cid, csuff in enumerate(contrasts):
      if not os.path.isdir('input' + csuff):
        path.make_dir('input' + csuff)
      run.command('mrconvert ' + self.ims_path[cid] + ' ' + os.path.join('input' + csuff, self.uid + '.mif'))
    self._local_ims = [os.path.join('input' + csuff, self.uid + '.mif') for csuff in contrasts]
    if self.msk_filename:
      if not os.path.isdir('mask'):
        path.make_dir('mask')
      run.command('mrconvert ' + self.msk_path + ' ' + os.path.join('mask', self.uid + '.mif'))
      self._local_msk = os.path.join('mask', self.uid + '.mif')

  def get_ims_path(self, quoted=True):
    """ return path to input images """
    from mrtrix3 import path # pylint: disable=no-name-in-module, import-outside-toplevel
    # Have to import locally to avoid circular dependency
    from .utils import abspath # pylint: disable=import-outside-toplevel
    if self._local_ims:
      return self._local_ims
    return [path.from_user(abspath(d, f), quoted) for d, f in zip(self._im_directories, self.ims_filenames)]
  ims_path = property(get_ims_path)

  def get_msk_path(self, quoted=True):
    """ return path to input mask """
    from mrtrix3 import path # pylint: disable=no-name-in-module, import-outside-toplevel
    if self._local_msk:
      return self._local_msk
    return path.from_user(os.path.join(self._msk_directory, self.msk_filename), quoted) if self.msk_filename else None
  msk_path = property(get_msk_path)
