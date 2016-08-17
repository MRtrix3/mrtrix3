# TODO Add compatibility with Windows (config files are in different locations)
def readMRtrixConfSetting(name):
  from lib.debugMessage import debugMessage
  debug_prefix = 'Key \'' + name + '\': '

  # Function definition: looking for key in whichever of the two files is open
  def findKey(f):
    for line in f:
      line = line.split()
      if len(line) != 2: continue
      if line[0].rstrip().replace(':', '') == name:
        return line[1]
    return ''

  try:
    f = open ('.mrtrix.conf', 'r')
    value = findKey(f)
    if value:
      debugMessage(debug_prefix + 'Value ' + value + ' found in user config file')
      return value
  except IOError:
    debugMessage(debug_prefix + 'Could not scan user config file')
    pass
    
  try:
    f = open ('/etc/mrtrix.conf', 'r')
    value = findKey(f)
    if value:
      debugMessage(debug_prefix + 'Value ' + value + ' found in system config file')
      return value
  except IOError:
    debugMessage(debug_prefix + 'Could not scan system config file')
    return ''

  debugMessage(debug_prefix + 'Not found in any config file')
  return ''
