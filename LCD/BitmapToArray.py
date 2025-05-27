import os
import imageio.v3 as iio
import numpy as np
import struct
from itertools import batched
import argparse


def check_input(file: str) -> str:
	if os.path.isfile(file):
		return file
	
	raise ValueError()

parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('-i', metavar='', required=True, default=argparse.SUPPRESS, type=check_input, help="Input file")
parser.add_argument('-o', metavar='', required=False, default=argparse.SUPPRESS, type=str, help="Output file")
parser.add_argument('-th', metavar='', default=np.uint8(128), type=np.uint8, help="Threshold for pixel brightness")
parser.add_argument('--max-vals-per-line', metavar='', default=16, type=int, help="Maximum number of hex values in each line in the output file")

args = parser.parse_args()

if not hasattr(args, "o"):
	args.o = os.path.splitext(args.i)[0] + "-array.txt"
elif not os.path.splitext(args.o)[1]:	# is a dir
	args.o = os.path.join(args.o, os.path.splitext(os.path.basename(args.i))[0] + ".txt")

print("Input:", args.i)
print("Output:", args.o)


image = iio.imread(args.i, mode='L')
assert image.dtype == np.uint8

# iio.imwrite(os.path.splitext(args.i)[0] + "-new.bmp", image)

y, x = image.shape
print("x:", x, "y:", y, sep=', ')

file_name = os.path.splitext(os.path.split(args.o)[1])[0]

with open(args.o, 'w') as output:
	output.write(f"inline constexpr uint8_t BMP_{file_name.replace(' ', '_').upper()}[] =\n{{")
	size = struct.pack("<HH", x, y)
	
	output.write("\n\t")
	for byte in size:
		output.write(f"{byte}, ")
	
	for oct, line in enumerate(range(0, y, 8)):
		
		for batch in batched(range(x), args.max_vals_per_line):
			output.write("\n\t")
			for col in batch:
				bits: list[bool] = [image[line + i][col] >= args.th if line + i < len(image) else False for i in range(8)]
				byte = sum(map(lambda x: x[1] << x[0], enumerate(bits)))
				output.write(f"0x{byte:02X}, ")
			
	
	output.write("\n};")

print("Done!")
