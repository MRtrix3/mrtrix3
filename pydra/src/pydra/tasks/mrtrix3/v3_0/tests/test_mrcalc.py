# Auto-generated test for mrcalc

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrcalc


def test_mrcalc(tmp_path, cli_parse_only):
    task = mrcalc(
        operand=[File.sample()],
        abs=[True],
        neg=[True],
        add=[True],
        subtract=[True],
        multiply=[True],
        divide=[True],
        modulo=[True],
        min=[True],
        max=[True],
        lt=[True],
        gt=[True],
        le=[True],
        ge=[True],
        eq=[True],
        neq=[True],
        if_=[True],
        replace=[True],
        sqrt=[True],
        pow=[True],
        round=[True],
        ceil=[True],
        floor=[True],
        not_=[True],
        and_=[True],
        or_=[True],
        xor=[True],
        isnan=[True],
        isinf=[True],
        finite=[True],
        complex=[True],
        polar=[True],
        real=[True],
        imag=[True],
        phase=[True],
        conj=[True],
        proj=[True],
        exp=[True],
        log=[True],
        log10=[True],
        cos=[True],
        sin=[True],
        tan=[True],
        acos=[True],
        asin=[True],
        atan=[True],
        cosh=[True],
        sinh=[True],
        tanh=[True],
        acosh=[True],
        asinh=[True],
        atanh=[True],
        datatype="float16",
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
