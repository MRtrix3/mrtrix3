def getHeaderInfo(image_path, header_item):
  import app, os, subprocess, sys
  from printMessage import printMessage
  command = 'mrinfo ' + image_path + ' -' + header_item
  if app.verbosity > 1:
    printMessage('Command: \'' + command + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None, shell=True)
  result = proc.stdout.read()
  result = result.rstrip()
  if app.verbosity > 1:
    printMessage ('Result: ' + result)
  return result

