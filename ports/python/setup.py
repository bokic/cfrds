import os
import subprocess
from setuptools import setup

here = os.path.abspath(os.path.dirname(__file__))


def _get_version():
    try:
        # e.g. "v1.0.6-3-gabcdef" or "v1.0.6-0-gabcdef"
        desc = subprocess.check_output(
            ["git", "describe", "--tags", "--long"],
            cwd=here,
            stderr=subprocess.DEVNULL,
        ).decode().strip()
        parts = desc.rsplit("-", 2)          # [tag, commits_ahead, ghash]
        tag = parts[0].lstrip("v")
        commits_ahead = int(parts[1])
        ghash = parts[2]                     # e.g. "gabcdef"
        dirty = subprocess.check_output(
            ["git", "status", "--porcelain"],
            cwd=here,
            stderr=subprocess.DEVNULL,
        ).decode().strip()
        version = tag
        if commits_ahead:
            version += f".post{commits_ahead}+{ghash}"
        if dirty:
            version += "+dirty" if not commits_ahead else ".dirty"
        return version
    except Exception:
        return "0.0.0"


with open(os.path.join(here, "README.md"), encoding="utf-8") as f:
    long_description = f.read()

setup(
    name="cfrds",
    version=_get_version(),
    description="Python interface for ColdFusion RDS service.",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="Boris Barbulovski",
    author_email="bbarbulovski@gmail.com",
    url="https://github.com/bbarbulovski/cfrds",
    py_modules=["cfrds"],
    python_requires=">=3.8",
)
