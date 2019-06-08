#!/usr/bin/env python

import json, os, subprocess, sys
mrtrix_bin_dir = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'bin')
data = { }
data["toolboxes"] = [ ]
data["toolboxes"].append({ })
data["toolboxes"][0]["name"] = "MRtrix3"
data["toolboxes"][0]["nodes"] = [ ]
for cmdname in sorted(os.listdir(mrtrix_bin_dir)):
  if cmdname != '__pycache__':
    try:
      process = subprocess.Popen([ os.path.join(mrtrix_bin_dir, cmdname), '__print_usage_porcupine__' ], stdin=None, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False)
      usage = process.communicate()[0].decode('cp1252')
      if usage and not process.returncode:
        try:
          node = json.loads(usage)
          data["toolboxes"][0]["nodes"].append(node)
        except json.decoder.JSONDecodeError as e:
          sys.stderr.write('Error parsing output from command "' + cmdname + '":\n')
          sys.stderr.write(str(e) + '\n')
      else:
        sys.stderr.write('Command "' + cmdname + '" did not provide usage information\n')
    except OSError as e:
      sys.stderr.write('Error attempting to print usage for command "' + cmdname + '":\n')
      sys.stderr.write(str(e) + '\n')

sys.stdout.write(json.dumps(data))
