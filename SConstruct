import os
import platform
import os.path as p

def shellCall(cmd):
  print(cmd)
  os.system(cmd)

def sub(src):
  vars = ['env', 'boostType', 'optLibs']
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

boostType = ARGUMENTS.get('boostType', '')

env = Environment(ENV = os.environ) # brings in PATH to give ccache some help

conf = Configure(env)
if (not (conf.CheckCXXHeader('boost/shared_ptr.hpp')
   and conf.CheckLib('boost_program_options' + boostType)
   and conf.CheckLib('tsk3'))):
   print('Configure check failed. fsrip needs Boost and The Sleuthkit.')
   Exit(1)

optLibs = checkLibs(conf, ['afflib', 'libewf'])

ccflags = '-Wall -Wno-trigraphs -Wextra -O3 -std=c++0x -Wnon-virtual-dtor'
env.Replace(CCFLAGS=ccflags)

fsrip = sub('src')
