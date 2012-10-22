import os
import platform
import os.path as p

def shellCall(cmd):
  print(cmd)
  os.system(cmd)

def sub(src):
  vars = ['env', 'optLibs']
  return env.SConscript(p.join(src, 'SConscript'), exports=vars, variant_dir=p.join('build', src), duplicate=0)

def checkLibs(conf, libs):
  ret = []
  for l in libs:
    if (conf.CheckLib(l)):
      ret.append(l)
    else:
      print(str(l) + ' could not be found, so fsrip will not support its functionality')
  return ret

arch = platform.platform()
print("System is %s, %s" % (arch, platform.machine()))
if (platform.machine().find('64') > -1):
  bits = '64'

vars = Variables('build_variables.py')
vars.AddVariables(
  ('boostType', 'Suffix to add to Boost libraries to enable finding them', ''),
  ('CC', 'set the name of the C compiler to use (scons finds default)', ''),
  ('CXX', 'set the name of the C++ compiler to use (scons finds default)', ''),
  ('CXXFLAGS', 'add flags for the C++ compiler to CXXFLAGS', ''),
  ('LINKFLAGS', 'add flags for the linker to LINKFLAGS', '')
)

env = Environment(ENV = os.environ, variables = vars) # brings in PATH to give ccache some help
print("CC = %s, CXX = %s" % (env['CC'], env['CXX']))

conf = Configure(env)
if (not (conf.CheckLib('boost_program_options' + env['boostType'])
   and conf.CheckLib('tsk3'))):
   print('Configure check failed. fsrip needs Boost and The Sleuthkit.')
   Exit(1)

optLibs = checkLibs(conf, ['afflib', 'libewf'])

ccflags = '-Wall -Wno-trigraphs -Wextra -O3 -std=c++11 -Wnon-virtual-dtor'
env.Replace(CCFLAGS=ccflags)

vars.Save('build_variables.py', env)

Help(vars.GenerateHelpText(env))

fsrip = sub('src')
