def runFunction(fn, *args):

  import lib.app, os, sys

  fnstring = fn.__module__ + '.' + fn.__name__ + '(' + ', '.join(args) + ')'

  if lib.app.lastFile:
    # Check to see if the last file produced in the previous script execution is
    #   intended to be produced by this command; if it is, this will be the last
    #   command that gets skipped by the -continue option
    # It's possible that the file might be defined in a '--option=XXX' style argument
    #  It's also possible that the filename in the command string has the file extension omitted
    for entry in args:
      if entry.startswith('--') and '=' in entry:
        totest = entry.split('=')[1]
      else:
        totest = entry
      filetotest = [ lib.app.lastFile, os.path.splitext(lib.app.lastFile)[0] ]
      if totest in filetotest:
        debugMessage('Detected last file \'' + lib.app.lastFile + '\' in function \'' + fnstring + '\'; this is the last runCommand() / runFunction() call that will be skipped')
        lib.app.lastFile = ''
        break
    if lib.app.verbosity:
      sys.stderr.write(lib.app.colourConsole + 'Skipping function:' + lib.app.colourClear + ' ' + fnstring + '\n')
    return

  if lib.app.verbosity:
    sys.stderr.write(lib.app.colourConsole + 'Function:' + lib.app.colourClear + ' ' + fnstring + '\n')

  # Now we need to actually execute the requested function
  result = fn(*args)

  # Only now do we append to the script log, since the function has completed successfully
  if lib.app.tempDir:
    with open(os.path.join(lib.app.tempDir, 'log.txt'), 'a') as outfile:
      outfile.write(fnstring + '\n')

  return result

