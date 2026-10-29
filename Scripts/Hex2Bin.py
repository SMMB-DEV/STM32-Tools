from html import parser

from Version import Version

import Crypto.Cipher.AES as AES
from Crypto.Hash import SHA512
from Crypto.Protocol.KDF import HKDF
from elftools.elf.elffile import ELFFile
import crc
import argparse

import os, time, subprocess, shutil, glob
from gettext import gettext


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

def get_version_offset(args: argparse.Namespace) -> int:
	with open(args.axf_file, "rb") as f:
		elf = ELFFile(f)
		section = elf.get_section_by_name(".symtab")
		
		for symbol in section.iter_symbols():
			if symbol.name == args.version_name:
				ver_offset = symbol["st_value"] - args.input_offset
				ver_size = symbol["st_size"]
				if ver_size != 4:
					print(f"Unexpected version size {ver_size} bytes!")
					exit(1)
				
				return ver_offset
		else:
			print("Version not found! Candidate symbol names:")
			
			for symbol in section.iter_symbols():
				if symbol.name.lower().endswith('version'):
					print(symbol.name)
			
			exit(1)

def make_metadata(ver: Version) -> bytes:
	"""
	Make metadata for the firmware file.
	File version 0 format:
	- data: n bytes, encrypted firmware data
	- tag: 16 bytes, authentication tag for AES-GCM
	- nonce: 12 bytes, random nonce for AES-GCM
	- metadata: 8 bytes, also used as additional authenticated data (AAD) for AES-GCM
		- fw_ver: 4 bytes, firmware version
		- reserved: 3 bytes, reserved (0x00, 0x00, 0x00)
		- file_ver: 1 byte, file version (0x00)
	"""
	return ver.value().to_bytes(4, 'little') + b"\x00\x00\x00\x00"

def print_file_size(file_size: int, max_size: int | None) -> None:
	if max_size is None or max_size <= 0:
		print(f"File size: {file_size} B - {file_size / 1024 :.1f} KiB")
	else:
		print(f"File size: {file_size} B - {file_size / 1024 :.1f} KiB / {max_size / 1024 :.1f} KiB ({100 * file_size / max_size :.1f}%)")

def convert_to_binary(args: argparse.Namespace) -> Version | None:
	result = subprocess.run(f"srec_cat {args.hex_file} -Intel -offset -{args.input_offset:#X} -o {args.bin_file} -Binary")
	if result.returncode != 0:
		print(f"Error converting hex file to binary: {result.stderr}")
		exit(result.returncode)
	
	with open(args.bin_file, "br+") as file:
		content = file.read()
		file_crc = calculator.checksum(content)
		file.write(file_crc.to_bytes(4, 'big'))
		file_size = file.tell()

		if args.add_version:
			ver_offset = get_version_offset(args)
			ver = Version(content[ver_offset + 3], content[ver_offset + 2], content[ver_offset + 1], content[ver_offset])

	if args.add_version:
		split = os.path.splitext(args.bin_file)
		new_file = split[0] + f" v{ver}" + split[1]
		os.replace(args.bin_file, new_file)
		args.bin_file = new_file

	print(f' \nCreated binary file "{args.bin_file}" from hex file "{args.hex_file}" (offset = 0x{args.input_offset:08X}).')
	print(f'CRC: 0x{file_crc:08X}')
	print_file_size(file_size, args.max_size)

	if args.add_version:
		return ver

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
		metadata = make_metadata(ver)
		ciphertext, tag = AES.new(key = key, mode=AES.MODE_GCM, nonce=nonce, mac_len=TAG_LEN).update(metadata).encrypt_and_digest(content)
		
		file.seek(0)
		file.write(ciphertext)
		file.write(tag)
		file.write(nonce)
		file.write(metadata)
		file_size = file.tell()
		
		print(f' \nCreated encrypted file "{args.enc_file}" with added metadata.')
		print("Key:", key.hex(' ').upper())
		print("Tag:", tag.hex(' ').upper())
		print("Nonce:", nonce.hex(' ').upper())
		print_file_size(file_size, args.max_size)

def check_single_file(f: str) -> str:
	files = glob.glob(f)
	if len(files) == 0:
		print("No hex files found in the current directory.")
		exit(1)
	elif len(files) > 1:
		print("Multiple hex files found in the current directory. Please specify one using the -i option.")
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
	version_group.add_argument('--version-name', default='_ZL7VERSION', type=str, help="The name of the version symbol in the AXF file.")

	enc_group = parser.add_argument_group('Encryption', "The encryption algorithm is AES-256-GCM. The encryption key is derived using HKDF with SHA512.")
	enc_group.add_argument('--encrypt', action=argparse.BooleanOptionalAction, default=False, help="Whether or not an encrypted version should be created.")
	enc_group.add_argument('--key', type=lambda x: bytes.fromhex(x), dest='master_key',
	                       help='The master key used for HKDF to generate the encryption key in hexadecimal format. Example: "EF3B"')
	enc_group.add_argument('--salt', default="", type=lambda x: bytes.fromhex(x),
	                       help="The salt used for HKDF to generate the encryption key in hexadecimal format.")
	enc_group.add_argument('--enc-file', type=str, help="The encrypted file's name. Uses the input file name by default. Overwrites existing files.")
	
	
	args = parser.parse_args()
	
	if args.bin_file is None:
		split = os.path.splitext(args.hex_file)
		args.bin_file = split[0] + ".bin"
	
	ver = convert_to_binary(args)
	
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
