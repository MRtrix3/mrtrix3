import subprocess
import time
import os
import sys

def is_int(s):
  try:
      int(s)
      return True
  except ValueError:
      return False

def launchJobs(jobs, logfile='None'):
  if logfile == 'None':
    cmd = ['parallel','--env', 'PATH', '--sshlogin', '2/beast,2/beauty', '--wd', os.getcwd()]
  else:
    cmd = ['parallel','--env', 'PATH', '--sshlogin', '2/beast,2/beauty', '--wd', os.getcwd(), '--joblog', logfile]
  proc = subprocess.Popen (cmd, stdin=subprocess.PIPE)
  proc.communicate (jobs)
