from elftools.elf.elffile import ELFFile
from elftools.elf import sections
from dataclasses import dataclass
from enum import Enum


class PR(Enum):
	ALPHA = 32
	ALPHA1 = 33
	ALPHA2 = 34
	ALPHA3 = 35
	ALPHA4 = 36
	ALPHA5 = 37
	ALPHA6 = 38
	ALPHA7 = 39
	ALPHA8 = 40
	ALPHA9 = 41
	ALPHA10 = 42
	ALPHA11 = 43
	ALPHA12 = 44
	ALPHA13 = 45
	ALPHA14 = 46
	ALPHA15 = 47
	ALPHA16 = 48
	ALPHA17 = 49
	ALPHA18 = 50
	ALPHA19 = 51
	ALPHA20 = 52
	ALPHA21 = 53
	ALPHA22 = 54
	ALPHA23 = 55
	ALPHA24 = 56
	ALPHA25 = 57
	ALPHA26 = 58
	ALPHA27 = 59
	ALPHA28 = 60
	ALPHA29 = 61
	ALPHA30 = 62
	ALPHA31 = 63
	
	BETA = 64
	BETA1 = 65
	BETA2 = 66
	BETA3 = 67
	BETA4 = 68
	BETA5 = 69
	BETA6 = 70
	BETA7 = 71
	BETA8 = 72
	BETA9 = 73
	BETA10 = 74
	BETA11 = 75
	BETA12 = 76
	BETA13 = 77
	BETA14 = 78
	BETA15 = 79
	BETA16 = 80
	BETA17 = 81
	BETA18 = 82
	BETA19 = 83
	BETA20 = 84
	BETA21 = 85
	BETA22 = 86
	BETA23 = 87
	BETA24 = 88
	BETA25 = 89
	BETA26 = 90
	BETA27 = 91
	BETA28 = 92
	BETA29 = 93
	BETA30 = 94
	BETA31 = 95

	RC = 96
	RC1 = 97
	RC2 = 98
	RC3 = 99
	RC4 = 100
	RC5 = 101
	RC6 = 102
	RC7 = 103
	RC8 = 104
	RC9 = 105
	RC10 = 106
	RC11 = 107
	RC12 = 108
	RC13 = 109
	RC14 = 110
	RC15 = 111
	RC16 = 112
	RC17 = 113
	RC18 = 114
	RC19 = 115
	RC20 = 116
	RC21 = 117
	RC22 = 118
	RC23 = 119
	RC24 = 120
	RC25 = 121
	RC26 = 122
	RC27 = 123
	RC28 = 124
	RC29 = 125
	RC30 = 126
	RC31 = 127

	NORMAL = 255

@dataclass(init=False)
class Version:
	major: int
	minor: int
	patch: int
	pr: PR

	def __init__(self, major: int, minor: int, patch: int, pr: int):
		assert 0 <= major <= 255 and 0 <= minor <= 255 and 0 <= patch <= 255 and 0 <= pr <= 255, "Version numbers must be in the range [0, 255]"
		
		self.major = major
		self.minor = minor
		self.patch = patch
		self.pr = PR(pr)

	def __str__(self) -> str:
		if self.pr == PR.NORMAL:
			return f"{self.major}.{self.minor}.{self.patch}"
		
		return f"{self.major}.{self.minor}.{self.patch}-{self.pr.name.lower()}"

	def value(self) -> int:
		return (self.major << 24) | (self.minor << 16) | (self.patch << 8) | self.pr.value

def get_version(axf_file: str, version_name: str = '_ZL7VERSION'):
	""" https://github.com/eliben/pyelftools/issues/227#issuecomment-495184612
	"""
	with open(axf_file, "rb") as f:
		elf = ELFFile(f)
		section: sections.SymbolTableSection = elf.get_section_by_name(".symtab")
		if not section:
			print(".symtab section not found in axf file!")
			exit()
		
		symbols = section.get_symbol_by_name(version_name)
		if not symbols:
			print("Version not found! Candidate symbol names:")
			
			for symbol in section.iter_symbols():
				if symbol.name.lower().endswith('version'):
					print(symbol.name)
			
			exit(1)
		elif len(symbols) > 1:
			print(f'Multiple "{version_name}" symbols found!')
			exit(1)

		symbol = symbols[0]

		file_offset = None
		for seg in elf.iter_segments():
			if seg.header['p_type'] != 'PT_LOAD':
				continue
			
			if symbol['st_value'] >= seg['p_vaddr'] and symbol['st_value'] < seg['p_vaddr'] + seg['p_filesz']:
				file_offset = symbol['st_value'] - seg['p_vaddr'] + seg['p_offset']
				break

		if not file_offset:
			print('Error getting file offset from ELF data')
			exit(1)
		
		elf.stream.seek(file_offset)
		content = elf.stream.read(4)
		ver = Version(content[3], content[2], content[1], content[0])
		return ver
