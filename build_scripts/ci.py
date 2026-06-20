from frameworks_ci.__main__ import main
from frameworks_ci.target_info import TargetInfo
import sys
import platform

sys.argv.append('--enable-sanitizers')

# only check style on Mac.
if platform.system() == "Darwin":
    sys.argv.append('--check-style')

main(
    targets=["izo_build/ni-random.gyp:ni-randomUnitTests"],
)
