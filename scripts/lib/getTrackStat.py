def getTrackStat(track_path, statistic):
  import lib.app, os, subprocess, sys
  from lib.printMessage import printMessage
  command = 'tckstats ' + track_path + ' -output ' + statistic + ' -ignorezero' + lib.app.mrtrixQuiet
  if lib.app.verbosity > 1:
    printMessage('Command: \'' + command + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None, shell=True)
  result = proc.stdout.read()
  result = result.rstrip().decode('utf-8')
  if lib.app.verbosity > 1:
    printMessage('Result: ' + result)
  return float(result)

