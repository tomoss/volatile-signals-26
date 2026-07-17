# PlatformIO pre-build script that embeds the current Git version
# into the firmware via the FIRMWARE_VERSION preprocessor macro.

Import("env")  # SCons-injected build environment for the current PlatformIO env (e.g. esp32s3)
import subprocess

try:
    version = subprocess.check_output(
        ["git", "describe", "--tags", "--always", "--dirty"],
        stderr=subprocess.DEVNULL
    ).decode().strip()
except Exception:
    version = "unknown"

env.Append(CPPDEFINES=[("FIRMWARE_VERSION", f'\\"{version}\\"')])
