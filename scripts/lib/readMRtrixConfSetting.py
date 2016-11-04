# TODO Add compatibility with Windows (config files are in different locations)
def readMRtrixConfSetting(name):

  # Function definition: looking for key in whichever of the two files is open
  def findKey(f):
    for line in f:
      line = line.split()
      if len(line) != 2: continue
      if line[0].rstrip().replace(':', '') == name:
        return line[1]
    return ''

  try:
    import os
    f = open (os.path.join(os.path.expanduser("~"),'.mrtrix.conf'), 'r')
    value = findKey(f)
    if value:
      return value
  except IOError:
    pass
    
  try:
    f = open ('/etc/mrtrix.conf', 'r')
  except IOError:
    return ''
  return findKey(f)

