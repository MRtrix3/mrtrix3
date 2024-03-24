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

import math, os, re, shutil, sys
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run, utils
from . import IMAGEEXT
from .input import Input

def abspath(arg, *args): # pylint: disable=unused-variable
  return os.path.abspath(os.path.join(arg, *args))


def relpath(arg, *args): # pylint: disable=unused-variable
  return os.path.relpath(os.path.join(arg, *args), app.WORKING_DIR)


def copy(src, dst, follow_symlinks=True): # pylint: disable=unused-variable
  """Copy data but do not set mode bits. Return the file's destination.

  mimics shutil.copy but without setting mode bits as shutil.copymode can fail on exotic mounts
  (observed on cifs with file_mode=0777).
  """
  if os.path.isdir(dst):
    dst = os.path.join(dst, os.path.basename(src))
  if sys.version_info[0] > 2:
    shutil.copyfile(src, dst, follow_symlinks=follow_symlinks)   # pylint: disable=unexpected-keyword-arg
  else:
    shutil.copyfile(src, dst)
  return dst


def check_linear_transformation(transformation, cmd, max_scaling=0.5, max_shear=0.2, max_rot=None, pause_on_warn=True): # pylint: disable=unused-variable
  if max_rot is None:
    max_rot = 2 * math.pi

  good = True
  run.command('transformcalc ' + transformation + ' decompose ' + transformation + 'decomp')
  if not os.path.isfile(transformation + 'decomp'):  # does not exist if run with -continue option
    app.console(transformation + 'decomp not found. skipping check')
    return True
  data = utils.load_keyval(transformation + 'decomp')
  run.function(os.remove, transformation + 'decomp')
  scaling = [float(value) for value in data['scaling']]
  if any(a < 0 for a in scaling) or any(a > (1 + max_scaling) for a in scaling) or any(
      a < (1 - max_scaling) for a in scaling):
    app.warn("large scaling (" + str(scaling) + ") in " + transformation)
    good = False
  shear = [float(value) for value in data['shear']]
  if any(abs(a) > max_shear for a in shear):
    app.warn("large shear (" + str(shear) + ") in " + transformation)
    good = False
  rot_angle = float(data['angle_axis'][0])
  if abs(rot_angle) > max_rot:
    app.warn("large rotation (" + str(rot_angle) + ") in " + transformation)
    good = False

  if not good:
    newcmd = []
    what = ''
    init_rotation_found = False
    skip = 0
    for element in cmd.split():
      if skip:
        skip -= 1
        continue
      if '_init_rotation' in element:
        init_rotation_found = True
      if '_init_matrix' in element:
        skip = 1
        continue
      if 'affine_scale' in element:
        assert what != 'rigid'
        what = 'affine'
      elif 'rigid_scale' in element:
        assert what != 'affine'
        what = 'rigid'
      newcmd.append(element)
    newcmd = " ".join(newcmd)
    if not init_rotation_found:
      app.console("replacing the transformation obtained with:")
      app.console(cmd)
      if what:
        newcmd += ' -' + what + '_init_translation mass -' + what + '_init_rotation search'
      app.console("by the one obtained with:")
      app.console(newcmd)
      run.command(newcmd, force=True)
      return check_linear_transformation(transformation, newcmd, max_scaling, max_shear, max_rot, pause_on_warn=pause_on_warn)
    if pause_on_warn:
      app.warn("you might want to manually repeat mrregister with different parameters and overwrite the transformation file: \n%s" % transformation)
      app.console('The command that failed the test was: \n' + cmd)
      app.console('Working directory: \n' + os.getcwd())
      input("press enter to continue population_template")
  return good


def aggregate(inputs, output, contrast_idx, mode, force=True): # pylint: disable=unused-variable

  images = [inp.ims_transformed[contrast_idx] for inp in inputs]
  if mode == 'mean':
    run.command(['mrmath', images, 'mean', '-keep_unary_axes', output], force=force)
  elif mode == 'median':
    run.command(['mrmath', images, 'median', '-keep_unary_axes', output], force=force)
  elif mode == 'weighted_mean':
    weights = [inp.aggregation_weight for inp in inputs]
    assert not any(w is None for w in weights), weights
    wsum = sum(float(w) for w in weights)
    cmd = ['mrcalc']
    if wsum <= 0:
      raise MRtrixError("the sum of aggregetion weights has to be positive")
    for weight, imagepath in zip(weights, images):
      if float(weight) != 0:
        cmd += [imagepath, weight, '-mult'] + (['-add'] if len(cmd) > 1 else [])
    cmd += ['%.16f' % wsum, '-div', output]
    run.command(cmd, force=force)
  else:
    raise MRtrixError("aggregation mode %s not understood" % mode)


def inplace_nan_mask(images, masks): # pylint: disable=unused-variable
  assert len(images) == len(masks), (len(images), len(masks))
  for imagepath, mask in zip(images, masks):
    target_dir = os.path.split(image)[0]
    masked = os.path.join(target_dir, '__' + os.path.split(imagepath)[1])
    run.command("mrcalc " + mask + " " + imagepath + " nan -if " + masked, force=True)
    run.function(shutil.move, masked, imagepath)


def calculate_isfinite(inputs, contrasts): # pylint: disable=unused-variable
  agg_weights = [float(inp.aggregation_weight) for inp in inputs if inp.aggregation_weight is not None]
  for cid in range(contrasts.n_contrasts):
    for inp in inputs:
      if contrasts.n_volumes[cid] > 0:
        cmd = 'mrconvert ' + inp.ims_transformed[cid] + ' -coord 3 0 - | mrcalc - -finite'
      else:
        cmd = 'mrcalc ' + inp.ims_transformed[cid] + ' -finite'
      if inp.aggregation_weight:
        cmd += ' %s -mult ' % inp.aggregation_weight
      cmd += ' isfinite%s/%s.mif' % (contrasts.suff[cid], inp.uid)
      run.command(cmd, force=True)
  for cid in range(contrasts.n_contrasts):
    cmd = ['mrmath', path.all_in_dir('isfinite%s' % contrasts.suff[cid]), 'sum']
    if agg_weights:
      agg_weight_norm = str(float(len(agg_weights)) / sum(agg_weights))
      cmd += ['-', '|', 'mrcalc', '-', agg_weight_norm, '-mult']
    run.command(cmd + [contrasts.isfinite_count[cid]], force=True)


def get_common_postfix(file_list): # pylint: disable=unused-variable
  return os.path.commonprefix([i[::-1] for i in file_list])[::-1]


def get_common_prefix(file_list): # pylint: disable=unused-variable
  return os.path.commonprefix(file_list)



def parse_input_files(in_files, mask_files, contrasts, f_agg_weight=None, whitespace_repl='_'): # pylint: disable=unused-variable
  """
    matches input images across contrasts and pair them with masks.
    extracts unique identifiers from mask and image filenames by stripping common pre and postfix (per contrast and for masks)
    unique identifiers contain ASCII letters, numbers and '_' but no whitespace which is replaced by whitespace_repl

    in_files: list of lists
      the inner list holds filenames specific to a contrast

    mask_files:
      can be empty

    returns list of Input

    checks: 3d_nonunity
    TODO check if no common grid & trafo across contrasts (only relevant for robust init?)

  """
  contrasts = contrasts.suff
  inputs = []
  def paths_to_file_uids(paths, prefix, postfix):
    """ strip pre and postfix from filename, replace whitespace characters """
    uid_path = {}
    uids = []
    for filepath in paths:
      uid = re.sub(re.escape(postfix)+'$', '', re.sub('^'+re.escape(prefix), '', os.path.split(filepath)[1]))
      uid = re.sub(r'\s+', whitespace_repl, uid)
      if not uid:
        raise MRtrixError('No uniquely identifiable part of filename "' + filepath + '" '
                          'after prefix and postfix substitution '
                          'with prefix "' + prefix + '" and postfix "' + postfix + '"')
      app.debug('UID mapping: "' + filepath + '" --> "' + uid + '"')
      if uid in uid_path:
        raise MRtrixError('unique file identifier is not unique: "' + uid + '" mapped to "' + filepath + '" and "' + uid_path[uid] +'"')
      uid_path[uid] = filepath
      uids.append(uid)
    return uids

  # mask uids
  mask_uids = []
  if mask_files:
    mask_common_postfix = get_common_postfix(mask_files)
    if not mask_common_postfix:
      raise MRtrixError('mask filenames do not have a common postfix')
    mask_common_prefix = get_common_prefix([os.path.split(m)[1] for m in mask_files])
    mask_uids = paths_to_file_uids(mask_files, mask_common_prefix, mask_common_postfix)
    if app.VERBOSITY > 1:
      app.console('mask uids:' + str(mask_uids))

  # images uids
  common_postfix = [get_common_postfix(files) for files in in_files]
  common_prefix = [get_common_prefix(files) for files in in_files]
  # xcontrast_xsubject_pre_postfix: prefix and postfix of the common part across contrasts and subjects,
  # without image extensions and leading or trailing '_' or '-'
  xcontrast_xsubject_pre_postfix = [get_common_postfix(common_prefix).lstrip('_-'),
                                    get_common_prefix([re.sub('.('+'|'.join(IMAGEEXT)+')(.gz)?$', '', pfix).rstrip('_-') for pfix in common_postfix])]
  if app.VERBOSITY > 1:
    app.console("common_postfix: " + str(common_postfix))
    app.console("common_prefix: " + str(common_prefix))
    app.console("xcontrast_xsubject_pre_postfix: " + str(xcontrast_xsubject_pre_postfix))
  for ipostfix, postfix in enumerate(common_postfix):
    if not postfix:
      raise MRtrixError('image filenames do not have a common postfix:\n' + '\n'.join(in_files[ipostfix]))

  c_uids = []
  for cid, files in enumerate(in_files):
    c_uids.append(paths_to_file_uids(files, common_prefix[cid], common_postfix[cid]))

  if app.VERBOSITY > 1:
    app.console('uids by contrast:' + str(c_uids))

  # join images and masks
  for ifile, fname in enumerate(in_files[0]):
    uid = c_uids[0][ifile]
    fnames = [fname]
    dirs = [abspath(path.from_user(app.ARGS.input_dir[0], False))]
    if len(contrasts) > 1:
      for cid in range(1, len(contrasts)):
        dirs.append(abspath(path.from_user(app.ARGS.input_dir[cid], False)))
        image.check_3d_nonunity(os.path.join(dirs[cid], in_files[cid][ifile]))
        if uid != c_uids[cid][ifile]:
          raise MRtrixError('no matching image was found for image %s and contrasts %s and %s.' % (fname, dirs[0], dirs[cid]))
        fnames.append(in_files[cid][ifile])

    if mask_files:
      if uid not in mask_uids:
        raise MRtrixError('no matching mask image was found for input image ' + fname + ' with uid "'+uid+'". '
                          'Mask uid candidates: ' + ', '.join(['"%s"' % m for m in mask_uids]))
      index = mask_uids.index(uid)
      # uid, filenames, directories, contrasts, mask_filename = '', mask_directory = '', agg_weight = None
      inputs.append(Input(uid, fnames, dirs, contrasts,
                          mask_filename=mask_files[index], mask_directory=abspath(path.from_user(app.ARGS.mask_dir, False))))
    else:
      inputs.append(Input(uid, fnames, dirs, contrasts))

  # parse aggregation weights and match to inputs
  if f_agg_weight:
    import csv  # pylint: disable=import-outside-toplevel
    try:
      with open(f_agg_weight, 'r', encoding='utf-8') as fweights:
        agg_weights = dict((row[0].lstrip().rstrip(), row[1]) for row in csv.reader(fweights, delimiter=',', quotechar='#'))
    except UnicodeDecodeError:
      with open(f_agg_weight, 'r', encoding='utf-8') as fweights:
        reader = csv.reader(fweights.read().decode('utf-8', errors='replace'), delimiter=',', quotechar='#')
        agg_weights = dict((row[0].lstrip().rstrip(), row[1]) for row in reader)
    pref = '^' + re.escape(get_common_prefix(list(agg_weights.keys())))
    suff = re.escape(get_common_postfix(list(agg_weights.keys()))) + '$'
    for key in agg_weights.keys():
      agg_weights[re.sub(suff, '', re.sub(pref, '', key))] = agg_weights.pop(key).strip()

    for inp in inputs:
      if inp.uid not in agg_weights:
        raise MRtrixError('aggregation weight not found for %s' % inp.uid)
      inp.aggregation_weight = agg_weights[inp.uid]
    app.console('Using aggregation weights ' + f_agg_weight)
    weights = [float(inp.aggregation_weight) for inp in inputs if inp.aggregation_weight is not None]
    if sum(weights) <= 0:
      raise MRtrixError('Sum of aggregation weights is not positive: ' + str(weights))
    if any(w < 0 for w in weights):
      app.warn('Negative aggregation weights: ' + str(weights))

  return inputs, xcontrast_xsubject_pre_postfix
