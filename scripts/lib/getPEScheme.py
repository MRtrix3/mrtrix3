def getPEScheme(path):
  import lib.app, os, subprocess
  from lib.debugMessage import debugMessage
  from lib.printMessage import printMessage
  command = [ 'mrinfo', path, '-petable' ]
  if lib.app.verbosity > 1:
    printMessage('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if result:
    result = [ [ float(f) for f in line.split() ] for line in result.split('\n') ]
  if lib.app.verbosity > 1:
    if not result:
      printMessage('Result: No phase encoding table found')
    else:
      printMessage('Result: ' + str(len(result)) + ' x ' + str(len(result[0])) + ' table')
      debugMessage(str(result))
  return result

