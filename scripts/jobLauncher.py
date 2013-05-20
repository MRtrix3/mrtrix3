import subprocess
import time
import os
import sys
import multiprocessing
import progress

def is_int(s):
  try:
      int(s)
      return True
  except ValueError:
      return False

def launchJobs(jobs, logfile='None'):
  if logfile == 'None':
    cmd = ['parallel','--env', 'PATH', '--sshlogin', '2/beast,2/beauty']
  else:
    cmd = ['parallel','--env', 'PATH', '--sshlogin', '2/beast,2/beauty', '--joblog', logfile]
  proc = subprocess.Popen (cmd, stdin=subprocess.PIPE)
  alljobs = ''
  for job in jobs:
    alljobs = alljobs + str(job) + ';\n'
  proc.communicate (alljobs)

def launchLocal(jobs, maxProcessors):
  numJobs = len(jobs)
  currentJob = 0
  processes = {}
  if maxProcessors == 0:
    maxProcessors = multiprocessing.cpu_count()

  total = len(jobs)
  p = progress.ProgressMeter(total=total)
  p.update(0)
  while currentJob < numJobs:

    if len(processes.keys()) < maxProcessors:
      pid = os.fork()

      # If we have the child process then launch this job
      if pid == 0:
        if (os.system(jobs[currentJob])):
          print 'Command failed: ' +  jobs[currentJob][:-16]
        sys.exit(1)
      # We have the parent so store this id in the processes list
      else:
        for i in range(0, maxProcessors):
          if i not in processes.keys():
            processes[i] = pid
            break
        currentJob = currentJob + 1

    else:
      # can't launch anymore so we wait until a job finishes
      procToDel = []
      for i in processes.keys():
        status = os.waitpid(processes[i],os.WNOHANG)
        if status[0] != 0:
          # the process has finished so add it to the list to be deleted
          # this is needed because processes.keys() returns a reference
          procToDel.append(i)
      # delete the process(es)
      for proc in procToDel:
        del processes[proc]
        p.update(1)

      time.sleep(1)

  # wait for all of them to finish
  while len(processes.keys()) > 0:
    procToDel = []
    for i in processes.keys():
      status = os.waitpid(processes[i],os.WNOHANG)
      if status[0] != 0:
        procToDel.append(i)

    # delete the process
    for proc in procToDel:
      del processes[proc]
      p.update(1)

    time.sleep(1)