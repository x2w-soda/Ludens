#!/bin/python

import os
import sys
import subprocess

def get_platform():
	if os.name == 'nt' or os.name == 'Windows':
		print("= Platform windows, {0}".format(sys.version))
		return {
			'anchor_file' : 'Ludens.sln',
			'executable_fmt' : 'Ludens/Core/{Module}/{Build}/LD{Module}Tests.exe'
		}

	sys.exit("failed to identify platform")


def get_test_dir(platform):
	build_dir_candidates = [
		'./',
		'../Build',
		'../build',
		'./Build',
		'./build',
	]

	for build_dir in build_dir_candidates:
		if os.path.exists(build_dir + '/' + platform['anchor_file']):
			print("= Found build directory [{0}]".format(build_dir))
			return build_dir

	sys.exit("failed to find build directory to run tests")

def run_tests(platform, test_dir, build):
	modules = [
		'Math',
		'DSA',
		'OS',
		'Serialize',
		'RenderBase',
	]

	for i in range(len(modules)):
		print("= Module Test {0}/{1}".format(i+1, len(modules)))

		module = modules[i]
		executable = platform['executable_fmt'].format(Module = module, Build = build)
		executable = test_dir + '/' + executable
		print("= {0}".format(executable))

		subprocess.call(executable)

		if not os.path.exists(executable):
			print("= Not found, skipping")
			continue


def main():
	print('= Ludens Test Driver')
	platform = get_platform()
	test_dir = get_test_dir(platform)
	run_tests(platform, test_dir, 'Debug')


if __name__ == '__main__':
	main()
