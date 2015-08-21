def getHeaderInfo(image_path, header_item):
  import lib.app, os, subprocess, sys
  from lib.printMessage import printMessage
  command = 'mrinfo ' + image_path + ' -' + header_item
  if lib.app.verbosity > 1:
    printMessage('Command: \'' + command + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None, shell=True)
  result = proc.stdout.read()
  result = result.rstrip().decode('utf-8')
  if lib.app.verbosity > 1:
    printMessage('Result: ' + result)
  return result

