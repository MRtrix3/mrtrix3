def delFile(path):
  import lib.app, os
  from lib.printMessage import printMessage
  if not lib.app.cleanup:
    return
  printMessage('Deleting file: ' + path)
  os.remove(path)

