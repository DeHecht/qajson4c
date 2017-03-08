try:
    import unittest2 as unittest
except ImportError:
    import unittest

import json
import subprocess
import os
from nose_parameterized import parameterized, param

def get_exec_path():
    return os.path.abspath(os.path.dirname(os.path.realpath(__file__)))

def lookup_dir_path(dirname, dir=get_exec_path()):
    for i in range(0,1):
        dir = os.path.dirname(dir)
        for (dirpath, dirnames, filenames) in os.walk(dir):
            if dirname in dirnames:
                return os.path.join(dirpath, dirname)
    return None

def list_files():
    paths = []
    ''' Generate the tests, based on the json files '''
    data_dir = lookup_dir_path("data")
    for (dirpath, dirnames, filenames) in os.walk(data_dir):
        for file in filenames:
            path = os.path.join(data_dir, file)
            paths.append(path)

    return paths

''' The tests have to be called from the project root, otherwise they do not work '''

class TestJsonMethods(unittest.TestCase):

    BINARY_NAME="simple-processor"

    @classmethod
    def lookup_bin_path(self, binname):
        dir = get_exec_path()
        for i in range(0,1):
            dir = os.path.dirname(dir)
            for (dirpath, dirnames, filenames) in os.walk(dir):
                if binname in filenames:
                    test_bin = os.path.join(dirpath, binname)
                    with open(os.devnull, 'w') as devnull:
                        rc = subprocess.call(test_bin+" --version", shell=True, stdout=devnull, stderr=devnull)
                        if rc == 0:
                            return test_bin
        return None
    
    @parameterized.expand(list_files())
    def test_insitu(self, filename):
        binary_path = self.lookup_bin_path(self.BINARY_NAME)
        subprocess.call("{} -f {} -o result.json -i 1".format(binary_path, filename), shell=True)
        with open(filename) as reference:
            with open("result.json") as generated:
                refjson = json.load(reference)
                genjson = json.load(generated)
                self.assertEqual(refjson, genjson)

    @parameterized.expand(list_files())
    def test_normal(self, filename):
        binary_path = self.lookup_bin_path(self.BINARY_NAME)
        subprocess.call("{} -f {} -o result.json".format(binary_path, filename), shell=True)
        with open(filename) as reference:
            with open("result.json") as generated:
                refjson = json.load(reference)
                genjson = json.load(generated)
                self.assertEqual(refjson, genjson)

if __name__ == '__main__':
    unittest.main()
