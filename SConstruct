import sys
import os
import glob
import platform
import os.path as p

def shellCall(cmd):
  print(cmd)
  os.system(cmd)

def sub(src):
  return env.SConscript(p.join(src, 'SConscript'), exports=['env'], variant_dir=p.join('build', src), duplicate=0)

def buildVendor(pattern, vDir, iDir, buildFn):
  vendorDir = p.abspath(vDir)
  installDir = p.abspath(iDir)
  if (len(glob.glob(p.join(installDir, pattern))) == 0):
    if (p.exists(vendorDir) == False or len(os.listdir(vendorDir)) == 0):
      os.system('git submodule init && git submodule update')
    if (p.exists(installDir) == False):
      Mkdir(installDir)
    curDir = os.getcwd()
    os.chdir(vendorDir)
    buildFn(vendorDir, installDir)
    os.chdir(curDir)
  else:
    print('%s is up to date' % vDir)


def buildTSK(tskDir, installDir):
  print("Building sleuthkit")
  shellCall('./configure --prefix=%s --enable-static=no --enable-shared=yes --with-afflib=%s --with-libewf=%s' % (installDir, installDir, installDir))
  shellCall('make install')

def buildBoost(boostDir, installDir):
  print("Building boost")
  shellCall('./bootstrap.sh')
  shellCall('./bjam --stagedir=%s --with-program_options link=shared variant=release '
            'threading=single stage runtime-link=shared' % installDir)

def buildAfflib(affDir, installDir):
  print("building afflib")
  shellCall('./configure --prefix=%s --enable-static=no --enable-shared=yes' % installDir)
  shellCall('make install')

def buildLibewf(ewfDir, installDir):
  print("building libewf")
  shellCall('./configure --prefix=%s --enable-static=no --enable-shared=yes' % installDir)
  shellCall('make install')
            
arch = platform.platform()
print("System is %s, %s" % (arch, platform.machine()))
if (platform.machine().find('64') > -1):
  bits = '64'

env = Environment(ENV = os.environ) # brings in PATH to give ccache some help

scopeDir = 'vendors/scope'
boostDir = 'vendors/boost'
ewfDir   = 'vendors/libewf'
affDir   = 'vendors/afflib'
tskDir   = 'vendors/sleuthkit'

deps = 'deps'
depsLib = p.join(deps, 'lib')

buildVendor('lib/*aff*', affDir, deps, buildAfflib)
buildVendor('lib/*ewf*', ewfDir, deps, buildLibewf)
buildVendor('lib/*tsk*', tskDir, deps, buildTSK)
buildVendor('lib/*program_options*', boostDir, deps, buildBoost)

flags = '-O3'
ccflags = '-Wall -Wno-trigraphs -Wextra %s -isystem %s -isystem %s -isystem %s' % (flags, scopeDir, boostDir, tskDir)
env.Replace(CCFLAGS=ccflags)
env.Append(LIBPATH=[p.abspath(depsLib)])

fsrip = sub('src')
