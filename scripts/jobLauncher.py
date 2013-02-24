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

class Server:
  def __init__(self, name, dataDir, sshfsDir, cpuCores, isCluster):
    self.name = name
    self.dataDir = dataDir
    self.sshfsDir = sshfsDir
    self.cpuCores = cpuCores
    self.isCluster = isCluster

def launchRemote(jobs, clusterJobs, servers):

  if len(jobs) != len(clusterJobs):
    print "The job list for the cluster and all other computers must have the same length"
    return

  processes = []
  currentJob = 0;


  while (currentJob < len(jobs)):
    for i in servers:
      if i.isCluster:
        subprocess.call(["ssh", i.name, "countJobs" + " > " + i.dataDir + "jobcount.txt"]); #qstat | grep raf022 | wc -l
      else:
        subprocess.call(["ssh", i.name, "ps aux|awk 'NR > 0 { s +=$3 }; END {print s}'" + " > " + i.dataDir + "cputime.txt"]);

    for i in servers:
      if i.isCluster:
        text_file = open(i.sshfsDir + 'jobcount.txt', "r")
        for line in text_file:
          currentNumberOfJobs = float(line.rstrip())
        text_file.close()

        if currentNumberOfJobs < i.cpuCores:
          pid = os.fork()
          # If we have the child process then launch this job
          if pid == 0:
            subprocess.call(["ssh", i.name, "dataDir=" + i.dataDir + "; " + clusterJobs[currentJob]]);
            sys.exit(1)
          # We have the parent so store this id in the processes list
          else:
            currentJob = currentJob + 1
            processes.append(pid)
            print 'Launched job number ' + str(currentJob) + '/' + str(len(jobs)) + ' on ' + i.name

      else:
        text_file = open(i.sshfsDir + 'cputime.txt', "r")
        for line in text_file:
          cpuUsage = float(line.rstrip())
        text_file.close()

        if cpuUsage < ((i.cpuCores * 100) - 85):
          pid = os.fork()
          # If we have the child process then launch this job
          if pid == 0:
            subprocess.call(["ssh", i.name, "dataDir=" + i.dataDir + "; " + jobs[currentJob]]);
            sys.exit(1)
          # We have the parent so store this id in the processes list
          else:
            currentJob = currentJob + 1
            processes.append(pid)
            print 'Launched job number ' + str(currentJob) + '/' + str(len(jobs)) + ' on ' + i.name

    time.sleep(5)

  # make sure all processes are finished
  stillRunning = True
  while stillRunning:
    stillRunning = False
    for i in range(0,len(processes)):
      if processes[i] != 0:
        status = os.waitpid(processes[i],os.WNOHANG)
        if status[0] != 0:
          processes[i] = 0
        else:
          stillRunning = True
    time.sleep(1)
  print "Finished all jobs!"

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