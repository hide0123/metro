from skbuild import setup

setup(
    name="metrolang",
    version="0.7.7",
    description="A Typed Script Language",
    author="mugen1234",
    url="https://github.com/mugen1234/metro",
    license="MIT",
    packages=["metrolang"],
    cmake_install_dir="metrolang/bin",
    entry_points={
        "console_scripts": [
            "metro=metrolang:metro"
        ]
    }
)
