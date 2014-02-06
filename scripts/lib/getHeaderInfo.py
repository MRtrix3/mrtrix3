def getHeaderInfo(image_path, header_item, verbose):
  import os, subprocess, sys
  from printMessage import printMessage
  command = 'mrinfo ' + image_path + ' -' + header_item
  if verbose:
    printMessage('Command: \'' + command + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None, shell=True)
  result = proc.stdout.read()
  result = result.rstrip()
  if verbose:
    printMessage ('Result: ' + result)
  return result

