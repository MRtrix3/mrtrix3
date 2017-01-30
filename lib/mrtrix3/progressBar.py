import os, sys
from mrtrix3 import app, message



class progressBar:

  def _update(self):
    sys.stderr.write('\r' + message.colourConsole + os.path.basename(sys.argv[0]) + ': ' + message.colourClear + '[{0:>3}%] '.format(int(round(100.0*self.counter/self.target))) + self.message + '...' + message.clearLine + self.newline)
    sys.stderr.flush()

  def __init__(self, msg, target):
    self.counter = 0
    self.message = msg
    self.newline = '\n' if app.verbosity > 1 else '' # If any more than default verbosity, may still get details printed in between progress updates
    self.orig_verbosity = app.verbosity
    self.target = target
    app.verbosity = app.verbosity - 1 if app.verbosity else 0
    self._update()

  def increment(self, msg=''):
    if msg:
      self.message = msg
    self.counter += 1
    self._update()

  def done(self):
    self.counter = self.target
    sys.stderr.write('\r' + message.colourConsole + os.path.basename(sys.argv[0]) + ': ' + message.colourClear + '[100%] ' + self.message + message.clearLine + '\n')
    sys.stderr.flush()
    app.verbosity = self.orig_verbosity
    
