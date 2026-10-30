from Version import Version

import Crypto.Cipher.AES as AES
from Crypto.Hash import SHA512
from Crypto.Protocol.KDF import HKDF
import crc
import argparse

import os, time, subprocess, shutil, glob
from gettext import gettext
from inspect import signature


class CustomHelpFormatter(argparse.HelpFormatter):
	def _get_help_string(self, action):
		help = action.help
		if help is None:
			help = ''
		
		if ('%(default)' not in help
			and action.default is not argparse.SUPPRESS
			and action.default is not None
			and not action.required):
			defaulting_nargs = (argparse.OPTIONAL, argparse.ZERO_OR_MORE)
			if action.option_strings or action.nargs in defaulting_nargs:
				if isinstance(action.default, str):
					help += gettext(' (default: "%(default)s")')
				else:
					help += gettext(' (default: %(default)s)')
		
		return help

calculator = crc.Calculator(crc.Configuration(width = 32, polynomial = 0x93A409EB), True)

def print_file_size(file_size: int, max_size: int | None) -> None:
	if max_size is None or max_size <= 0:
		print(f"File size: {file_size} B - {file_size / 1024 :.1f} KiB")
	else:
		print(f"File size: {file_size} B - {file_size / 1024 :.1f} KiB / {max_size / 1024 :.1f} KiB ({100 * file_size / max_size :.1f}%)")

def convert_to_binary(args: argparse.Namespace):
	result = subprocess.run(f'srec_cat "{args.hex_file}" -Intel -offset -{args.input_offset:#X} -o "{args.bin_file}" -Binary')
	if result.returncode != 0:
		print(f"Error converting hex file to binary: {result.stderr}")
		exit(result.returncode)
	
	with open(args.bin_file, "br+") as file:
		content = file.read()
		file_crc = calculator.checksum(content)
		file.write(file_crc.to_bytes(4, 'big'))
		file_size = file.tell()

	print(f' \nCreated binary file "{args.bin_file}" from hex file "{args.hex_file}" (offset = 0x{args.input_offset:08X}).')
	print(f'CRC: 0x{file_crc:08X}')
	print_file_size(file_size, args.max_size)

def encrypt(args: argparse.Namespace, ver: Version):
	KEY_LEN = 32
	TAG_LEN = 16
	NONCE_LEN = 12	# https://crypto.stackexchange.com/questions/41601/aes-gcm-recommended-iv-size-why-12-bytes
	
	nonce = os.urandom(NONCE_LEN)
	key = HKDF(args.master_key, KEY_LEN, args.salt, SHA512, 1, nonce)
	assert isinstance(key, bytes)
	
	shutil.copy(args.bin_file, args.enc_file)
	
	with open(args.enc_file, 'br+') as file:
		content = file.read()[:-4]		# remove CRC
		cipher = AES.new(key = key, mode=AES.MODE_GCM, nonce=nonce, mac_len=TAG_LEN)

		header, footer = None, None
		
		if args.header_file:
			with open(args.header_file, 'br') as f:
				header = f.read()
				cipher.update(header)
		
		if args.footer_file:
			with open(args.footer_file, 'br') as f:
				footer = f.read()
				cipher.update(footer)
		
		ciphertext, tag = cipher.encrypt_and_digest(content)
		
		file.seek(0)
		if header:
			file.write(header)
		file.write(ciphertext)
		file.write(tag)
		file.write(nonce)
		if footer:
			file.write(footer)
		file_size = file.tell()
		
		print(f' \nCreated encrypted file "{args.enc_file}" with added metadata.')
		print("Key:", key.hex(' ').upper())
		print("Tag:", tag.hex(' ').upper())
		print("Nonce:", nonce.hex(' ').upper())
		print_file_size(file_size, args.max_size)

def check_single_file(f: str) -> str:
	files = glob.glob(f)
	if len(files) == 0:
		print(f'No matching files found for "{f}"!')
		exit(1)
	elif len(files) > 1:
		print(f'Multiple matching files found for "{f}"!')
		exit(1)
	
	return files[0]

def main():
	parser = argparse.ArgumentParser(formatter_class=CustomHelpFormatter, add_help=False)
	parser.add_argument('-h', '--help', action='help', default=argparse.SUPPRESS, help="Show this help message and exit.")

	parser.add_argument('-i', '--input', default='*/*.hex', type=check_single_file, dest='hex_file',
	                    help="The input hex file's name. Could be a glob pattern. Must match exactly one file.")
	parser.add_argument('-o', '--output', type=str, dest='bin_file',
	                    help="The output binary file's name. Uses the input file name by default. Overwrites existing files.")
	parser.add_argument('--offset', default='0x08000000', type=lambda x: int(x, base=0), dest='input_offset', help="The offset for the hex file.")
	parser.add_argument('--max-size', type=lambda x: int(x, base=0),
	                    help="The maximum size for the output files. Only used for calculating the percentage of the storage used.")

	version_group = parser.add_argument_group('Versioning', "Options for handling versioning. A STM32T::Version instance must be defined in code for these options to work.")
	version_group.add_argument('--add-version', action=argparse.BooleanOptionalAction, default=True, help="Whether or not the version should be added to output file names.")
	version_group.add_argument('--axf-file', default='*/*.axf', type=check_single_file,
	                           help="The AXF file's name for versioning. Could be a glob pattern. Must match exactly one file.")
	version_group.add_argument('--version-name', default=signature(Version.get_from_file).parameters['version_name'].default, type=str,
	                           help="The name of the version symbol in the AXF file.")

	enc_group = parser.add_argument_group('Encryption',
	    """The encryption algorithm is AES-256-GCM. The encryption key is derived using HKDF with SHA512. The header and footer will be used as AAD.
		Encrypted file format: [Header] + Ciphertext + Tag + Nonce + [Footer]""")
	enc_group.add_argument('--encrypt', action=argparse.BooleanOptionalAction, default=False, help="Whether or not an encrypted version should be created.")
	enc_group.add_argument('--key', type=lambda x: bytes.fromhex(x), dest='master_key',
	                       help='The master key used for HKDF to generate the encryption key in hexadecimal format. Example: "EF3B"')
	enc_group.add_argument('--salt', default="", type=lambda x: bytes.fromhex(x),
	                       help="The salt used for HKDF to generate the encryption key in hexadecimal format.")
	enc_group.add_argument('--enc-file', type=str, help="The encrypted file's name. Uses the input file name by default. Overwrites existing files.")
	enc_group.add_argument('--header', type=check_single_file, dest='header_file',
	                    help="An optional file containing metadata that will be prepended to the binary data unencrypted. Could be a glob pattern.")
	enc_group.add_argument('--footer', type=check_single_file, dest='footer_file',
	                    help="An optional file containing metadata that will be appended to the binary data unencrypted. Could be a glob pattern.")
	
	
	args = parser.parse_args()
	
	if args.bin_file is None:
		split = os.path.splitext(args.hex_file)
		args.bin_file = split[0] + ".bin"

	ver = None
	if args.add_version:
		ver = Version.get_from_file(args.axf_file, args.version_name)
		if not ver:
			exit(1)
		
		split = os.path.splitext(args.bin_file)
		args.bin_file = split[0] + f" v{ver}" + split[1]
	
	convert_to_binary(args)
	
	if args.encrypt:
		if args.master_key is None:
			parser.error("--key must be provided when --encrypt is specified!")
		
		if ver is None:
			parser.error("The version is required for creating encrypted files.")

		if args.enc_file is None:
			split = os.path.splitext(args.bin_file)
			args.enc_file = split[0] + ".enc"

	assert ver is not None
	encrypt(args, ver)

	print(f" \nProcessing time: {time.process_time() * 1000 :.0f} ms")


if __name__ == "__main__":
	main()
