_data = { 'SIGALRM': 'Timer expiration',
          'SIGBUS' : 'Bus error: Accessing invalid address (out of storage space?)',
          'SIGFPE' : 'Floating-point arithmetic exception',
          'SIGHUP' : 'Disconnection of terminal',
          'SIGILL' : 'Illegal instruction (corrupt binary command file?)',
          'SIGINT' : 'Program manually interrupted by terminal',
          'SIGPIPE': 'Nothing on receiving end of pipe',
          'SIGPWR' : 'Power failure restart',
          'SIGQUIT': 'Received terminal quit signal',
          'SIGSEGV': 'Segmentation fault: Invalid memory reference',
          'SIGSYS' : 'Bad system call',
          'SIGTERM': 'Terminated by kill command',
          'SIGXCPU': 'CPU time limit exceeded',
          'SIGXFSZ': 'File size limit exceeded' }
           # Can't be handled; see https://bugs.python.org/issue9524
           # 'CTRL_C_EVENT': 'Terminated by user Ctrl-C input',
           # 'CTRL_BREAK_EVENT': Terminated by user Ctrl-Break input'



def initialise():
  import signal
  from lib.debugMessage import debugMessage
  global _data
  
  successful = [ ]
  for s in _data:
    if hasattr(signal, s):
      signal.signal(getattr(signal, s), _handler)
      successful.append(s)
  debugMessage('Signal handler capturing ' + str(len(successful)) + ' signals: ' + str(successful))



def _handler(signum, frame):
  import lib.app, os, signal, sys
  from lib.runCommand import _processes
  global _data
  # First, kill any child processes
  for p in _processes:
    p.terminate()
  _processes = [ ]
  msg = '[SYSTEM FATAL CODE: '
  signal_found = False
  for (key, value) in _data.items():
    if hasattr(signal, key) and signum == getattr(signal, key):
      msg += key + ' (' + str(int(getattr(signal, key))) + ')] ' + value
      signal_found = True
      break
  if not signal_found:
    msg += '?] Unknown system signal'
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + lib.app.colourError + msg + lib.app.colourClear + '\n')
  lib.app.complete()
  exit(signum)


