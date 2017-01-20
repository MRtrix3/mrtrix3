def imagesMatch(image_one, image_two):
  import math
  from lib.debugMessage import debugMessage
  from lib.getHeaderInfo import getHeaderInfo
  debug_prefix = '\'' + image_one + '\' \'' + image_two + '\''
  # Image dimensions
  one_dim = [ int(i) for i in getHeaderInfo(image_one, 'size').split() ]
  two_dim = [ int(i) for i in getHeaderInfo(image_two, 'size').split() ]
  if not one_dim == two_dim:
    debugMessage(debug_prefix + ' dimension mismatch (' + str(one_dim) + ' ' + str(two_dim) + ')')
    return False
  # Voxel size
  one_spacing = [ float(f) for f in getHeaderInfo(image_one, 'vox').split() ]
  two_spacing = [ float(f) for f in getHeaderInfo(image_two, 'vox').split() ]
  for one, two in zip(one_spacing, two_spacing):
    if one and two and not math.isnan(one) and not math.isnan(two):
      if (abs(two-one) / (0.5*(one+two))) > 1e-04:
        debugMessage(debug_prefix + ' voxel size mismatch (' + str(one_spacing) + ' ' + str(two_spacing) + ')')
        return False
  # Image transform
  one_transform = [ float(f) for f in getHeaderInfo(image_one, 'transform').replace('\n', ' ').replace(',', ' ').split() ]
  two_transform = [ float(f) for f in getHeaderInfo(image_two, 'transform').replace('\n', ' ').replace(',', ' ').split() ]
  for one, two in zip(one_transform, two_transform):
    if abs(one-two) > 1e-4:
      debugMessage(debug_prefix + ' transform mismatch (' + str(one_transform) + ' ' + str(two_transform) + ')')
      return False
  # Everything matches!
  debugMessage(debug_prefix + ' image match')
  return True
  
