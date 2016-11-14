try:
    import unittest2 as unittest
except ImportError:
    import unittest

import json
import subprocess
import os

''' The tests have to be called from the project root, otherwise they do not work '''

def get_exec_path():
    return os.path.abspath(os.path.dirname(os.path.realpath(__file__)))

class TestJsonMethods(unittest.TestCase):

    BINARY_NAME="simple-processor"
    DATA_DIR="data"

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

    @classmethod
    def lookup_dir_path(self, dirname, dir=get_exec_path()):
        for i in range(0,1):
            dir = os.path.dirname(dir)
            for (dirpath, dirnames, filenames) in os.walk(dir):
                if dirname in dirnames:
                    return os.path.join(dirpath, dirname)
        return None


    def test_first_sample_file(self):
        data_dir = self.lookup_dir_path(self.DATA_DIR)
        binary_path = self.lookup_bin_path(self.BINARY_NAME)
        filename = os.path.join(data_dir, 'ref-example-1.json')
        subprocess.call("{} -f {} -o result.json".format(binary_path, filename), shell=True)
        with open(filename) as reference:
            with open("result.json") as generated:
                refjson = json.load(reference)
                genjson = json.load(generated)
                self.assertEqual(refjson, genjson)

    def test_second_sample_file(self):
        data_dir = self.lookup_dir_path(self.DATA_DIR)
        binary_path = self.lookup_bin_path(self.BINARY_NAME)
        filename = os.path.join(data_dir, 'ref-example-2.json')
        subprocess.call("{} -f {} -o result.json".format(binary_path, filename), shell=True)
        with open(filename) as reference:
            with open("result.json") as generated:
                refjson = json.load(reference)
                genjson = json.load(generated)
                self.assertEqual(refjson, genjson)

    def test_third_sample_file(self):
        data_dir = self.lookup_dir_path(self.DATA_DIR)
        binary_path = self.lookup_bin_path(self.BINARY_NAME)
        filename = os.path.join(data_dir, 'ref-example-3.json')
        subprocess.call("{} -f {} -o result.json".format(binary_path, filename), shell=True)
        with open(filename) as reference:
            with open("result.json") as generated:
                refjson = json.load(reference)
                genjson = json.load(generated)
                self.assertEqual(refjson, genjson)

    def test_fourth_sample_file(self):
        data_dir = self.lookup_dir_path(self.DATA_DIR)
        binary_path = self.lookup_bin_path(self.BINARY_NAME)
        filename = os.path.join(data_dir, 'ref-example-4.json')
        subprocess.call("{} -f {} -o result.json".format(binary_path, filename), shell=True)
        with open(filename) as reference:
            with open("result.json") as generated:
                refjson = json.load(reference)
                genjson = json.load(generated)
                self.assertEqual(refjson, genjson)

    def test_fifth_sample_file(self):
        data_dir = self.lookup_dir_path(self.DATA_DIR)
        binary_path = self.lookup_bin_path(self.BINARY_NAME)
        filename = os.path.join(data_dir, 'ref-example-5.json')
        subprocess.call("{} -f {} -o result.json".format(binary_path, filename), shell=True)
        with open(filename) as reference:
            with open("result.json") as generated:
                refjson = json.load(reference)
                genjson = json.load(generated)
                self.assertEqual(refjson, genjson)

    def test_first_sample_file_insitu(self):
        data_dir = self.lookup_dir_path(self.DATA_DIR)
        binary_path = self.lookup_bin_path(self.BINARY_NAME)
        filename = os.path.join(data_dir, 'ref-example-1.json')
        subprocess.call("{} -f {} -o result.json -i 1".format(binary_path, filename), shell=True)
        with open(filename) as reference:
            with open("result.json") as generated:
                refjson = json.load(reference)
                genjson = json.load(generated)
                self.assertEqual(refjson, genjson)

    def test_second_sample_file_insitu(self):
        data_dir = self.lookup_dir_path(self.DATA_DIR)
        binary_path = self.lookup_bin_path(self.BINARY_NAME)
        filename = os.path.join(data_dir, 'ref-example-2.json')
        subprocess.call("{} -f {} -o result.json -i 1".format(binary_path, filename), shell=True)
        with open(filename) as reference:
            with open("result.json") as generated:
                refjson = json.load(reference)
                genjson = json.load(generated)
                self.assertEqual(refjson, genjson)

    def test_third_sample_file_insitu(self):
        data_dir = self.lookup_dir_path(self.DATA_DIR)
        binary_path = self.lookup_bin_path(self.BINARY_NAME)
        filename = os.path.join(data_dir, 'ref-example-3.json')
        subprocess.call("{} -f {} -o result.json -i 1".format(binary_path, filename), shell=True)
        with open(filename) as reference:
            with open("result.json") as generated:
                refjson = json.load(reference)
                genjson = json.load(generated)
                self.assertEqual(refjson, genjson)

    def test_fourth_sample_file_insitu(self):
        data_dir = self.lookup_dir_path(self.DATA_DIR)
        binary_path = self.lookup_bin_path(self.BINARY_NAME)
        filename = os.path.join(data_dir, 'ref-example-4.json')
        subprocess.call("{} -f {} -o result.json -i 1".format(binary_path, filename), shell=True)
        with open(filename) as reference:
            with open("result.json") as generated:
                refjson = json.load(reference)
                genjson = json.load(generated)
                self.assertEqual(refjson, genjson)

    def test_fifth_sample_file_insitu(self):
        data_dir = self.lookup_dir_path(self.DATA_DIR)
        binary_path = self.lookup_bin_path(self.BINARY_NAME)
        filename = os.path.join(data_dir, 'ref-example-5.json')
        subprocess.call("{} -f {} -o result.json -i 1".format(binary_path, filename), shell=True)
        with open(filename) as reference:
            with open("result.json") as generated:
                refjson = json.load(reference)
                genjson = json.load(generated)
                self.assertEqual(refjson, genjson)
                
if __name__ == '__main__':
    unittest.main()
