#!/usr/bin/env python

# Copyright (c) 2011-2012 James Deery
# Released under the MIT license <http://opensource.org/licenses/MIT>.
# See COPYING for details.

import sys, os

def read_chunks(filename, size):
	with open(filename, 'rb') as f:
		while True:
			chunk = f.read(size)
			if chunk:
				yield chunk
			else:
				break


def compile_resources(output_filename, input_filenames, output_name=None):
	if output_name == None:
		output_name = os.path.basename(output_filename)[:-2]

	with open(output_filename, 'w') as output_file:
		output_file.write('#include <stddef.h>\n')
		output_file.write('#include "%s.h"\n' % output_name)
		output_file.write('\n')

		for input_filename in input_filenames:
			input_name = os.path.basename(input_filename).replace('.', '_')
			output_file.write('const char %s_%s[] = {\n' % (output_name, input_name))

			size = 0
			for chunk in read_chunks(input_filename, 16):
				size += len(chunk)

				output_file.write('\t')
				output_file.write(', '.join([hex(ord(c)) for c in chunk]))
				output_file.write(',\n')

			output_file.write('\t0x0};\n')
			output_file.write('const size_t %s_%s_size = %d;\n\n' % (output_name, input_name, size))


def scons_builder(target, source, env):
	compile_resources(str(target[0]), [str(s) for s in source])


def add_scons_builder(Builder, env):
	env.Append(BUILDERS={'Resources': Builder(action=scons_builder, suffix='.c')})


def run_xcode_rule():
	num_inputs = int(os.environ['SCRIPT_INPUT_FILE_COUNT'])
	input_filenames = [os.environ['SCRIPT_INPUT_FILE_' + str(i)] for i in range(num_inputs)]
	output_filename = os.environ['SCRIPT_OUTPUT_FILE_0']
	output_name = os.path.basename(output_filename)[:-10]
	compile_resources(output_filename, input_filenames, output_name)
	print()


if __name__ == '__main__':
	if len(sys.argv) < 2:
		print('Usage: compile_resources.py [output_file] [input_files...]')
	else:
		compile_resources(sys.argv[1], sys.argv[2:])
