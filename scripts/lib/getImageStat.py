def getImageStat(image_path, statistic, mask_path = ''):
  import lib.app, subprocess
  from lib.debugMessage import debugMessage
  from lib.printMessage import printMessage
  command = [ 'mrstats', image_path, '-output', statistic ]
  if mask_path:
    command.extend([ '-mask', mask_path ])
  if lib.app.verbosity > 1:
    printMessage('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if lib.app.verbosity > 1:
    printMessage('Result: ' + result)
  return result

