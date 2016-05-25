def getHeaderInfo(image_path, header_item):
  import lib.app, subprocess
  from lib.printMessage import printMessage
  command = [ 'mrinfo', image_path, '-' + header_item ]
  if lib.app.verbosity > 1:
    printMessage('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if lib.app.verbosity > 1:
    printMessage('Result: ' + result)
  return result

