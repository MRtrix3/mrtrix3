def getImageStat(image_path, statistic, mask_path = ''):
  import lib.app, os, subprocess, sys
  from lib.printMessage import printMessage
  command = 'mrstats ' + image_path + ' -output ' + statistic
  if mask_path:
    command += ' -mask ' + mask_path
  if lib.app.verbosity > 1:
    printMessage('Command: \'' + command + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None, shell=True)
  result = proc.stdout.read()
  result = result.rstrip().decode('utf-8')
  if lib.app.verbosity > 1:
    printMessage('Result: ' + result)
  return result

