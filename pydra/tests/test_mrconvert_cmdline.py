from pydra.tasks.mrtrix3.latest.mrconvert import mrconvert
from fileformats.medimage import NiftiGz

task = mrconvert()
task.inputs.input = NiftiGz.mock()
task.inputs.export_grad_fsl = ("/out/path.v", "/out/path.b")
task.cmdline
