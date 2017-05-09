#!/usr/bin/env python

import os
import subprocess
import errno

def mkdir(path):
  try:
    os.makedirs(path)
  except OSError as exc:  # Python >2.5
    if exc.errno == errno.EEXIST and os.path.isdir(path):
      pass
    else:
      raise

sge_default_options = {'-q':'rhe5b', '-j':'y', '-o':'sge'}

class CommandQueue(object):
  def __init__(self, location='local'):
    self.commands = []
    self.location = location

  def run (self, location='local', **kwargs):
    if self.location == 'local':
      self.run_local (**kwargs)
    elif self.location == 'sge':
      self.run_sge (**kwargs)

  def add (self, cmd):
    self.commands.append(subprocess.list2cmdline(cmd))

  def run_local (self, **kwargs):
    for key in kwargs:
      print "another keyword arg: %s: %s" % (key, kwargs[key])
    for cmd in self.commands:
      print(cmd)


  def run_sge (self, jobname='mrtrix.job', sge_options=sge_default_options, nthreads=1, prefix='#$ '):
    """ write a file 'mrtrix.jobs' where each line contrains a command to execute on a node
    and a bash script to launch those commands. The script sets SGE variables

    #! /bin/bash
    # define interpreting shell for this job to be the Bash shell
    #$ -S /bin/bash
    # execute the job for the current working directory
    #$ -cwd
    # merge the standard error stream into the standard output stream
    #$ -j y
    #$ -q rhe5b
    #$ -t 1-3
    TASK=`sed -n ${SGE_TASK_ID}p sge/jobs`
    eval $TASK
    """
    mkdir('sge')
    with open(os.path.join('sge','jobs'),'w') as commandfile:
      commandfile.writelines('\n'.join(self.commands))

    with open(jobname,'w') as jobfile:
      jobfile.write('#!/bin/env bash\n')
      jobfile.write(prefix+'-S /bin/bash\n')
      jobfile.write(prefix+'-cwd\n')
      for key, val in sge_options.items():
        jobfile.write(prefix+key+' '+val+'\n')
      # Tell the SGE that this is an array job, with "tasks" to be numbered 1 to ...
      jobfile.write(prefix+'-t 1:'+str(len(self.commands))+'\n')
      # choose the line to be executed on the node
      jobfile.write('TASK=`sed -n ${SGE_TASK_ID}p '+os.path.join('sge','jobs')+'`\n')
      jobfile.write('eval $TASK\n\n')

    subprocess.check_call(['qsub', '-sync', 'y', '-v', 'MRTRIX_NTHREADS='+str(nthreads), jobname])
    self.commands = []
    # env=dict(os.environ, MRTRIX_NTHREADS=str(nthreads)) # does not get propagated to the node
    # http://community.mrtrix.org/t/5ttgen/133/27
    # unset SGE_ROOT and FSLPARALLEL?

if __name__ == '__main__':

  commands=CommandQueue('sge')
  commands.add(['echo','MRTRIX_NTHREADS=$MRTRIX_NTHREADS','>','out'])
  commands.add(['env','>','env.out'])
  commands.run(sge_options=sge_default_options)


#!/bin/sh
#PBS -l nodes=1:ppn=1,walltime=5:00
#PBS -N arraytest
# #$ -l nodes=1:ppn=1,walltime=01:00:00


#!/bin/bash
#PBS -J 1-10
#PBS -j oe
#PBS -l walltime=01:00:00
#PBS -l slots=2
#PBS -l mem=2gb
#PBS -l ncpus=2
#PBS -l nodes=1:ppn=1

# set TASK=`sed -n ${SGE_TASK_ID}p /home/k1465906/calibration_nan.cmds8`
# eval $TASK

