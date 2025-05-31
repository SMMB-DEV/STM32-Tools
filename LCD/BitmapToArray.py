import os
import imageio.v3 as iio
import numpy as np
import struct
from itertools import batched
import argparse


def check_input(input: str) -> str:
	if os.path.isdir(input) or os.path.isfile(input):
		return input
	
	raise ValueError()

def check_output(output: str) -> str:
	if os.path.isdir(output) or os.path.splitext(output)[1]:
		return output
	
	raise ValueError()

parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('-i', metavar='', required=True, default=argparse.SUPPRESS, type=check_input, help="Input file")
parser.add_argument('-o', metavar='', required=False, default=argparse.SUPPRESS, type=check_output, help="Output file")
parser.add_argument('-th', metavar='', default=np.uint8(128), type=np.uint8, help="Threshold for pixel brightness")
parser.add_argument('--max-vals-per-line', metavar='', default=16, type=int, help="Maximum number of hex values in each line in the output file")

args = parser.parse_args()

files = {}

if os.path.isfile(args.i):
	if not hasattr(args, "o"):
		args.o = os.path.splitext(args.i)[0] + "-array.txt"
	elif not os.path.splitext(args.o)[1]:	# is a dir
		args.o = os.path.join(args.o, os.path.splitext(os.path.basename(args.i))[0] + ".txt")
	
	files[os.path.normpath(args.i)] = os.path.normpath(args.o)
	
else:	# input is a path
	if not hasattr(args, "o"):
		args.o = args.i
	elif not os.path.isdir(args.o):
		raise ValueError()
	
	args.i = os.path.normpath(args.i)
	args.o = os.path.normpath(args.o)
	for file in os.listdir(args.i):
		files[os.path.join(args.i, file)] = os.path.join(args.o, os.path.splitext(file)[0] + ".txt")


for input, output in files.items():
	print(f"{input} -> {output}")
	
	image = iio.imread(input, mode='L')
	assert image.dtype == np.uint8
	y, x = image.shape
	
	# iio.imwrite(os.path.splitext(input)[0] + "-new.bmp", image)
	
	file_name = os.path.splitext(os.path.split(output)[1])[0]
	
	with open(output, 'w') as output_file:
		output_file.write(f"inline constexpr uint8_t BMP_{file_name.replace(' ', '_').upper()}[] =\n{{")
		size = struct.pack("<HH", x, y)
		
		output_file.write("\n\t")
		for byte in size:
			output_file.write(f"{byte}, ")
		
		for oct, line in enumerate(range(0, y, 8)):
			
			for batch in batched(range(x), args.max_vals_per_line):
				output_file.write("\n\t")
				for col in batch:
					bits: list[bool] = [image[line + i][col] >= args.th if line + i < len(image) else False for i in range(8)]
					byte = sum(map(lambda x: x[1] << x[0], enumerate(bits)))
					output_file.write(f"0x{byte:02X}, ")
			
			output_file.write("\n")
		
		output_file.write("};")

print("Done!")
