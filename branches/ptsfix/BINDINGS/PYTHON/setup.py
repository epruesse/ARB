#!/usr/bin/env python
"""
setup.py file for ARB bindings
"""

from distutils.core import setup, Extension
from os import environ

ARB_module = Extension('_ARB',
                       sources=['ARB.i'],
                       swig_opts=['-c++'],
                       include_dirs=['../../INCLUDE'],
                       libraries=['ARBDB','CORE'],
#                       extra_objects=['../ARB_oolayer.o'],
                       library_dirs=['../../lib']
                      )

setup (name = 'ARB',
       version = '0.1',
       author      = "epruesse",
       description = """Module to access ARB databases""",
       ext_modules = [ARB_module],
       py_modules = ["ARB"],
       )
