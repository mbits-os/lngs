from distutils.core import setup, Extension

mod = Extension('strings',
                sources=['src/py-strings.c'],
                libraries=['strings'])

setup(name = 'Strings',
      version = '0.2',
      description = 'Locale python bindings',
      ext_modules = [mod])

