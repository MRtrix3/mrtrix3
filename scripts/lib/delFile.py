def delFile(path):
  import lib.app, os
  from lib.printMessage import printMessage
  if not lib.app.cleanup:
    return
  if lib.app.verbosity > 1:
    printMessage('Deleting file: ' + path)
  try:
    os.remove(path)
  except OSError:
    pass


def delFolder(path):
  import lib.app, os, shutil
  from lib.printMessage import printMessage
  if not lib.app.cleanup:
    return
  if lib.app.verbosity > 1:
    printMessage('Deleting folder: ' + path)
  if os.path.exists(path):
    shutil.rmtree(path)
