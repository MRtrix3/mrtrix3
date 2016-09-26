def getTempFilePath(suffix):
  import os, random, string
  from lib.debugMessage          import debugMessage
  from lib.readMRtrixConfSetting import readMRtrixConfSetting
  dir_path = readMRtrixConfSetting('TmpFileDir')
  if not dir_path:
    dir_path = '.'
  prefix = readMRtrixConfSetting('TmpFilePrefix')
  if not prefix:
    prefix = os.path.basename(sys.argv[0]) + '-tmp-'
  full_path = dir_path
  if not suffix:
    suffix = 'mif'
  while os.path.exists(full_path):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
    full_path = os.path.join(dir_path, prefix + random_string + '.' + suffix)
  debugMessage(full_path)
  return full_path

