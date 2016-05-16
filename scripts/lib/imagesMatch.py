def imagesMatch(image_one, image_two):
  import math
  from lib.getHeaderInfo import getHeaderInfo
  # Image dimensions
  one_dim = [ int(i) for i in getHeaderInfo(image_one, 'size').split() ]
  two_dim = [ int(i) for i in getHeaderInfo(image_two, 'size').split() ]
  if not one_dim == two_dim:
    return False
  # Voxel size
  one_spacing = [ float(f) for f in getHeaderInfo(image_one, 'vox').split() ]
  two_spacing = [ float(f) for f in getHeaderInfo(image_two, 'vox').split() ]
  for one, two in zip(one_spacing, two_spacing):
    if one and two and not math.isnan(one) and not math.isnan(two):
      if (abs(two-one) / (0.5*(one+two))) > 1e-04:
        return False
  # Image transform
  one_transform = [ float(f) for f in getHeaderInfo(image_one, 'transform').replace('\n', ' ').replace(',', ' ').split() ]
  two_transform = [ float(f) for f in getHeaderInfo(image_two, 'transform').replace('\n', ' ').replace(',', ' ').split() ]
  for one, two in zip(one_transform, two_transform):
    if abs(one-two) > 1e-4:
      return False
  # Everything matches!
  return True
  
