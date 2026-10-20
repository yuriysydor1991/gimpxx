python3 = import('python').find_installation()

cmd_py = '''
import glob

sources = glob.glob('@0@/src/**/*.cpp', recursive=True)
sources += glob.glob('@0@/src/**/*.h', recursive=True)

# The original GIMP tree keeps its own code style and checks.
for i in sources:
  if not i.startswith('@0@/src/gimp/'):
    print(i)
'''.format(meson.project_source_root())

all_CPP_sources = run_command(
  python3, 
  '-c', 
  cmd_py,
  check: true
).stdout().strip().split('\n')
