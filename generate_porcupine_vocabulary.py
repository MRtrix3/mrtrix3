#!/usr/bin/env python

import json, os, subprocess, sys
mrtrix_bin_dir = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'bin')
data = { }
data["toolboxes"] = { }
data["toolboxes"]["name"] = "MRtrix3"
data["toolboxes"]["nodes"] = [ ]
for cmdname in sorted(os.listdir(mrtrix_bin_dir)):
  try:
    process = subprocess.Popen([ os.path.join(mrtrix_bin_dir, cmdname), '__print_usage_porcupine__' ], stdin=None, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False)
    usage = process.communicate()[0].decode('cp1252')
    if usage and not process.returncode:
      try:
        node = json.loads(usage)
        data["toolboxes"]["nodes"].append(node)
      except json.decoder.JSONDecodeError:
        sys.stderr.write('Error parsing output from \'' + cmdname + '\'' + '\n')
  except OSError:
    pass

sys.stdout.write(json.dumps(data))
