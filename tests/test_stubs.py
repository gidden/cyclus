#!/usr/bin/env python

import os
import subprocess
import shutil
import tempfile

from contextlib import contextmanager

@contextmanager
def tmpdir():
    d = tempfile.mkdtemp(dir=os.path.dirname(os.path.realpath(__file__)))
    yield d
    shutil.rmtree(d)

@contextmanager
def tmpenviron(**mapping):
    environ = dict(os.environ)
    os.environ.update(mapping)
    yield
    os.environ.clear()
    os.environ.update(environ)

def test_stubs():
    flavors = ['facility', 'inst', 'region']
    pth = 'foo'
    with tmpdir() as d:
        src = os.path.join(d, pth)
        bld = os.path.join(d, 'bar')
        inst = os.path.join(d, 'baz')
        
        stub_cmd = 'cycstub --type {0} {1}:{1}:{2}'
        inst_cmd = 'python install.py --build_dir {0} --prefix {1}'
        tst_cmd = './bin/{}_unit_tests'
        
        os.mkdir(src)
        for flav in flavors:
            # generate stub
            cmd = stub_cmd.format(flav, pth, 'tmp' + flav.capitalize())
            print(cmd)
            subprocess.call(cmd.split(), shell=(os.name=='nt'), cwd=src)
        # build/install stub
        cmd = inst_cmd.format(bld, inst)
        print(cmd)
        subprocess.call(cmd.split(), shell=(os.name=='nt'), cwd=src)
        # run unit tests for stub
        cmd = tst_cmd.format(pth)
        print(cmd)
        subprocess.call(cmd.split(), shell=(os.name=='nt'), cwd=inst)
        
        # run cyclus
        run_cmd = 'cyclus {}'
        pth = os.path.join(inst, 'lib', 'cyclus')
        with tmpenviron(CYCLUS_PATH=pth):
            for flav in flavors:
                fname = 'example_tmp_{}.xml'.format(flav)
                cmd = run_cmd.format(fname)
                print(cmd)
                subprocess.call(cmd.split(), shell=(os.name=='nt'), cwd=src)

if __name__ == '__main__':
    test_stubs()
    
    
    
