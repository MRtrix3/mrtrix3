def getHeaderProperty(image_path, key):
  import lib.app, subprocess
  from lib.printMessage import printMessage
  command = [ 'mrinfo', image_path, '-property', key ]
  if lib.app.verbosity > 1:
    printMessage('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if lib.app.verbosity > 1:
    printMessage('Result: ' + result)
  return result

