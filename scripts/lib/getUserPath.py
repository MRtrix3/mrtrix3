def getUserPath(filename, is_command):
  import lib.app, os
  # Places quotation marks around the full path, to handle cases where the user-specified path / file name contains spaces
  # However, this should only occur in situations where the output is being interpreted as part of a full command string;
  #   if the expected output is a stand-alone string, the quotation marks should be omitted
  wrapper=''
  if is_command and (filename.count(' ') or lib.app.workingDir(' ')):
    wrapper='\"'
  return wrapper + os.path.abspath(os.path.join(lib.app.workingDir, filename)) + wrapper

