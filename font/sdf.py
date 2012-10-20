#!/usr/bin/python

import sys
import math
import struct

def build_pairs(d):
	d_sq = d ** 2

	pairs = [
		(x, y) 
		for x in range(-d, d) 
		for y in range(-d, d)
		if x ** 2 + y ** 2 <= d_sq
		]
	pairs.sort(key=lambda p: p[0] ** 2 + p[1] ** 2)

	return pairs[1:]

def extract_char(input, output, char_size, num_chars, i, j):
	line_width = char_size[0] * num_chars[0]
	input.seek(line_width * char_size[1] * j + char_size[0] * i)

	for x in range(char_size[1]):
		data = input.read(char_size[0])
		output.write(data)
		input.seek(line_width - char_size[0], 1)

def get_signed_distance(data, x_range, y_range, pairs, clamp, x, y):
	if x in x_range and y in y_range:
		cd = data[y][x]
	else:
		cd = '\x00'

	sign = 2 * struct.unpack('B', cd)[0] - 1

	for pair in pairs:
		dx = x + pair[0]
		dy = y + pair[1]

		if dx not in x_range or dy not in y_range:
			continue

		if data[dy][dx] != cd:
			return sign * math.sqrt(pair[0] ** 2 + pair[1] ** 2)

	return sign * clamp

def calculate_field(input, output, char_size, num_chars, clamp, scale, padding):
	pairs = build_pairs(clamp)
	x_starts = [char_size[0] * i for i in range(num_chars[0])]
	x_ranges = [range(start, start + char_size[0]) for start in x_starts]
	y_range = range(char_size[1])

	for j in range(num_chars[1]):
		sys.stderr.write('Char line %d: ' % j)

		input_data = []
		for y in range(char_size[1]):
			line = input.read(char_size[0] * num_chars[0])
			input_data.append(line)

		for y in range(-padding[1], char_size[1] + padding[1], scale):
			sys.stderr.write('.')
			sys.stderr.flush()

			for i in range(num_chars[0]):
				for x in range(-padding[0] + x_starts[i], x_starts[i] + char_size[0] + padding[0], scale):
					d = get_signed_distance(input_data, x_ranges[i], y_range, pairs, clamp, x, y)
					d = ((d / (2.0 * clamp)) + 0.5) * 255
					output.write(struct.pack('B', d))

		sys.stderr.write('\n')


# with open('bitmap', 'rb') as input:
# 	with open('char', 'wb') as output:
# 		extract_char(input, output, (100, 200), (16, 16), 10, 1)

# with open('char', 'rb') as input:
# 	with open('char_sdf', 'wb') as output:
# 		calculate_field(input, output, (100, 200), (1, 1), 2, 8, (14, 28))

if __name__ == '__main__':
	if len(sys.argv) != 9:
		sys.stderr.write('Usage: sdf.py [char_size_x] [char_size_y] [num_chars_x] [num_chars_y] [clamp] [scale] [padding_x] [padding_y]\n')
		exit(1)

	char_size = (int(sys.argv[1]), int(sys.argv[2]))
	num_chars = (int(sys.argv[3]), int(sys.argv[4]))
	clamp = int(sys.argv[5])
	scale = int(sys.argv[6])
	padding = (int(sys.argv[7]), int(sys.argv[8]))

	calculate_field(sys.stdin, sys.stdout, char_size, num_chars, clamp, scale, padding)
