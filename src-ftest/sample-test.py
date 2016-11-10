import json
import unittest
import subprocess

''' The tests have to be called from the project root, otherwise they do not work '''

class TestJsonMethods(unittest.TestCase):

    def test_first_sample_file(self):
		filename = 'data/ref-example-1.json'
		subprocess.call("build/bin/simple-test -f {} -o result.json".format(filename), shell=True)
		with open(filename) as reference:
			with open("result.json") as generated:
				refjson = json.load(reference)
				genjson = json.load(generated)
				self.assertEqual(refjson, genjson)

    def test_second_sample_file(self):
		filename = 'data/ref-example-2.json'
		subprocess.call("build/bin/simple-test -f {} -o result.json".format(filename), shell=True)
		with open(filename) as reference:
			with open("result.json") as generated:
				refjson = json.load(reference)
				genjson = json.load(generated)
				self.assertEqual(refjson, genjson)

    def test_third_sample_file(self):
		filename = 'data/ref-example-3.json'
		subprocess.call("build/bin/simple-test -f {} -o result.json".format(filename), shell=True)
		with open(filename) as reference:
			with open("result.json") as generated:
				refjson = json.load(reference)
				genjson = json.load(generated)
				self.assertEqual(refjson, genjson)

    def test_fourth_sample_file(self):
		filename = 'data/ref-example-4.json'
		subprocess.call("build/bin/simple-test -f {} -o result.json".format(filename), shell=True)
		with open(filename) as reference:
			with open("result.json") as generated:
				refjson = json.load(reference)
				genjson = json.load(generated)
				self.assertEqual(refjson, genjson)

    def test_fifth_sample_file(self):
		filename = 'data/ref-example-5.json'
		subprocess.call("build/bin/simple-test -f {} -o result.json".format(filename), shell=True)
		with open(filename) as reference:
			with open("result.json") as generated:
				refjson = json.load(reference)
				genjson = json.load(generated)
				self.assertEqual(refjson, genjson)

    def test_first_sample_file_insitu(self):
		filename = 'data/ref-example-1.json'
		subprocess.call("build/bin/simple-test -f {} -o result.json -i 1".format(filename), shell=True)
		with open(filename) as reference:
			with open("result.json") as generated:
				refjson = json.load(reference)
				genjson = json.load(generated)
				self.assertEqual(refjson, genjson)

    def test_second_sample_file_insitu(self):
		filename = 'data/ref-example-2.json'
		subprocess.call("build/bin/simple-test -f {} -o result.json -i 1".format(filename), shell=True)
		with open(filename) as reference:
			with open("result.json") as generated:
				refjson = json.load(reference)
				genjson = json.load(generated)
				self.assertEqual(refjson, genjson)

    def test_third_sample_file_insitu(self):
		filename = 'data/ref-example-3.json'
		subprocess.call("build/bin/simple-test -f {} -o result.json -i 1".format(filename), shell=True)
		with open(filename) as reference:
			with open("result.json") as generated:
				refjson = json.load(reference)
				genjson = json.load(generated)
				self.assertEqual(refjson, genjson)

    def test_fourth_sample_file_insitu(self):
		filename = 'data/ref-example-4.json'
		subprocess.call("build/bin/simple-test -f {} -o result.json -i 1".format(filename), shell=True)
		with open(filename) as reference:
			with open("result.json") as generated:
				refjson = json.load(reference)
				genjson = json.load(generated)
				self.assertEqual(refjson, genjson)

    def test_fifth_sample_file_insitu(self):
		filename = 'data/ref-example-5.json'
		subprocess.call("build/bin/simple-test -f {} -o result.json -i 1".format(filename), shell=True)
		with open(filename) as reference:
			with open("result.json") as generated:
				refjson = json.load(reference)
				genjson = json.load(generated)
				self.assertEqual(refjson, genjson)


if __name__ == '__main__':
    unittest.main()
