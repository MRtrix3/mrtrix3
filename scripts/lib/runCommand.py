def runCommand(cmd, verbose):
  import os, sys
  if verbose:
  	sys.stdout.write(cmd + '\n')
  	sys.stdout.flush()
# Consider changing this to a subprocess call, pipe console outputs to local variables,
#   if the command fails then provide output from the relevant function only
# Some issues with subprocess:
# * Would suppress progress bars from MRtrix commands, which may be wanted with the -verbose option
# * Ideally want to run with shell=False, for both security and compatibility; but then:
#   - can't use wildcards etc. that rely on a shell
#   - can't use piping in MRtrix
#   - would have to convert command string into a python list
#   - Technically could handle these explicitly... bit of work though.
  if (os.system(cmd)):
    sys.stderr.write('Command failed: ' + cmd + '\n')
    exit(1)

