def delTempDir(temp_dir, verbose):
  import shutil
  from printMessage import printMessage
  if verbose:
    printMessage('Deleting temporary directory ' + temp_dir)
  shutil.rmtree(temp_dir)

