#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "struse/struse.h"
#include "6510.h"
#include <string.h>
#include <malloc.h>
#include "HashTable.h"
#include "Files.h"

struct SymEntry {
	int16_t count;
};

struct SymList {
	enum {
		MIN_LIST = 7,
		GROW_LIST = 8
	};
	size_t capacity;
	const char* names[ 1 ];
};

union SymRef {
	const char* unique;
	SymList* multi;
};

static SymEntry* sLabelCount = nullptr;
static SymRef* sLabelEntries = nullptr;
static std::vector<uint16_t> sortedSymAddrs;
static HashTable<uint64_t, uint32_t> sReverseLookup;

void ResetSymbols()
{
	sReverseLookup.Clear();
	sortedSymAddrs.clear();
	if( sLabelCount ) {
		for( size_t adr = 0; adr < 0x10000; ++adr ) {
			if( sLabelCount[ adr ].count == 1 ) {
				if( sLabelEntries[ adr ].unique ) {
					free( (void*)sLabelEntries[ adr ].unique );
					sLabelEntries[ adr ].unique = nullptr;
				}
			}
			else if( sLabelCount[ adr ].count ) {
				if( const char** ppStr = sLabelEntries[ adr ].multi->names ) {
					for( size_t i = 0, n = sLabelCount[ adr ].count; i < n; ++i ) {
						if( *ppStr ) { free( (void*)*ppStr ); }
						++ppStr;
					}
					if (sLabelEntries && sLabelEntries[adr].multi) {
						free(sLabelEntries[adr].multi);
					}
				}
			}
		}
		free( sLabelCount );
		sLabelCount = nullptr;
		free( sLabelEntries );
		sLabelEntries = nullptr;
	}
}

size_t GetLabelSlot(uint16_t addr)
{
	size_t lb = 0, ub = sortedSymAddrs.size();

	while ((ub-lb)>1) {
		size_t cb = (ub + lb) >> 1;
		uint16_t addr_cmp = sortedSymAddrs[cb];
		if (addr == addr_cmp) {
			return cb;
		} else if (addr > addr_cmp) {
			lb = cb;
		} else {
			ub = cb;
		}
	}
	return lb;
}

const char* NearestLabel(uint16_t addr, uint16_t& offs)
{
	size_t i = GetLabelSlot(addr);
	if (i < sortedSymAddrs.size() && addr >= sortedSymAddrs[i]) {
		uint16_t prevAddr = sortedSymAddrs[i];
		offs = addr - prevAddr;
		if (sLabelEntries[prevAddr].unique) {
			return sLabelEntries[prevAddr].unique;
		} else if (SymList* syms = sLabelEntries[prevAddr].multi) {
			return sLabelCount[prevAddr].count ? syms->names[0] : nullptr;
		} else {
			return nullptr;
		}
	}
	offs = addr;
	return nullptr;
}


void AddSymbol( uint16_t address, const char *name, size_t chars )
{
	// make sure label array exists
	if( !sLabelCount ) {
		sLabelCount = (SymEntry*)calloc( 1, sizeof( SymEntry ) * 0x10000 );
		sLabelEntries = (SymRef*)calloc( 1, sizeof( SymRef ) * 0x10000 );
	}
	if (sLabelEntries == nullptr || sLabelCount == nullptr) { return; }

	// check for dupes
	strref lbl( name, (strl_t)chars );
	if( sLabelCount[ address ].count ) {
		if( sLabelCount[ address ].count == 1 ) {
			if( lbl.same_str_case( sLabelEntries[ address ].unique ) ) { return; }
		} else {
			if( const char** ppStr = sLabelEntries[ address ].multi->names ) {
				for( size_t i = 0, n = sLabelCount[ address ].count; i < n; ++i ) {
					if( lbl.same_str( *ppStr ) ) { return; }
					++ppStr;
				}
			}
		}
	}

	char* copy = (char*)malloc(chars + 1);
	if (copy == nullptr) { return; }

	uint64_t hash = lbl.fnv1a_64();
	if( !sReverseLookup.Exists( hash ) ) {
		sReverseLookup.Insert( hash, address);
	}

	// not a dupe
	size_t slot = GetLabelSlot(address);
	if (slot < sortedSymAddrs.size()) {
		if (address < sortedSymAddrs[slot]) {
			sortedSymAddrs.insert(sortedSymAddrs.begin() + slot, address);
		} else if (address > sortedSymAddrs[slot]) {
			sortedSymAddrs.insert(sortedSymAddrs.begin() + slot+1, address);
		}
	} else {
		sortedSymAddrs.push_back(address);
	}


	memcpy( copy, name, chars );
	copy[ chars ] = 0;
	if( !sLabelCount[ address ].count ) {
		sLabelCount[ address ].count = 1;
		sLabelEntries[ address ].unique = copy;
	} else if( sLabelCount[ address ].count == 1 ) {
		const char* prev = sLabelEntries[ address ].unique;
		sLabelEntries[ address ].multi = (SymList*)malloc( sizeof( SymList ) + sizeof( const char* ) * (SymList::MIN_LIST - 1) );
		sLabelEntries[ address ].multi->capacity = SymList::MIN_LIST;
		sLabelCount[ address ].count = 2;
		sLabelEntries[ address ].multi->names[ 0 ] = prev;
		sLabelEntries[ address ].multi->names[ 1 ] = copy;
	} else {
		SymList* entries = sLabelEntries[ address ].multi;
		if( entries->capacity == (size_t)sLabelCount[ address ].count ) {
			sLabelEntries[ address ].multi = (SymList*)malloc(
				sizeof( SymList ) + sizeof( const char* ) * (entries->capacity + SymList::GROW_LIST - 1) );
			sLabelEntries[ address ].multi->capacity = entries->capacity + SymList::GROW_LIST;
			memcpy( sLabelEntries[ address ].multi->names, entries->names, sizeof( const char* ) * sLabelCount[ address ].count );
			free( entries );
			entries = sLabelEntries[ address ].multi;
		}
		entries->names[ sLabelCount[ address ].count++ ] = copy;
	}
}

void ShutdownSymbols()
{
	ResetSymbols();
}

const char* GetSymbol(uint16_t address)
{
	if( sLabelCount ) {
		if( !sLabelCount[ address ].count ) { return nullptr; }
		if( sLabelCount[ address ].count == 1 ) { return sLabelEntries[ address ].unique; }
		return sLabelEntries[ address ].multi->names[ 0 ];
	}
	return nullptr;
}

static uint64_t fnv1a_64(const char *string, size_t length, uint64_t seed = 14695981039346656037ULL)
{
	uint64_t hash = seed;
	if (string) {
		size_t left = length;
		while (left--)
			hash = (*string++ ^ hash) * 1099511628211;
	}
	return hash;
}

bool GetAddress( const char *name, size_t chars, uint16_t &addr )
{
	uint64_t key = fnv1a_64( name, chars );
	if( uint32_t* value = sReverseLookup.Value( key ) ) {
		addr = (uint16_t)*value;
		return true;
	}
	return false;
}

void ReadViceCommandFile(const char *symFile)
{
	ResetSymbols();
	FILE* f;
	if (fopen_s(&f, symFile, "rb") == 0 && f) {
		fseek(f, 0, SEEK_END);
		uint32_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (void *voidbuf = malloc(size)) {
			fread(voidbuf, size, 1, f);

			//char *strcurr = nullptr;
			//char *strend = nullptr;
			//char **strtable = nullptr;
			//uint16_t *addresses = nullptr;
			//size_t strcurr_left = 0;

			//size_t numChars = 0;
			for (int pass = 0; pass<2; pass++) {
				//uint32_t size_tmp = size;
				strref file((const char*)voidbuf, size);
				//				const char *buf = (const char*)voidbuf;
				//uint32_t numLabels = 0;

				while (strref line = file.line()) {
					if (strref command = line.get_word()) {
						uint32_t addr;
						line += command.get_len();
						line.trim_whitespace();
						if (command.same_str("break") || command.same_str("bk")) {
							if (pass) {
								if (line.get_first() == '$') { ++line; }
								// TODO: Set breakpoint in VICE
								//SetPCBreakpoint((uint16_t)(line + 1).ahextoui());
							}
						} else if (command.same_str("al") || command.same_str("add_label")) {
							if (line.has_prefix("c:")) { line += 2; }
							line.skip_whitespace();
							if (line.get_first() == '$') { ++line; }
							addr = (uint16_t)line.ahextoui_skip();
							line.skip_whitespace();
							if (addr < 0x10000) {
								AddSymbol(addr, line.get(), line.get_len());
							}
						}
					}
				}
			}
			free(voidbuf);
		}
		fclose(f);
	}
}

bool ReadSymbols(const char *filename)
{
	ResetSymbols();
	FILE *f;
	if (fopen_s(&f, filename, "rb") == 0) {
		fseek(f, 0, SEEK_END);
		uint32_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (void *voidbuf = malloc(size)) {
			fread(voidbuf, size, 1, f);
			strref file((const char*)voidbuf, size);
			while (file) {
				if (strref line = file.line()) {
					line.skip_whitespace();
					if (line.grab_prefix(".label")) {
						line.skip_whitespace();
						strref label = line.split_label();
						line.skip_whitespace();
						if (line.grab_char('=')) {
							line.skip_whitespace();
							if (line.grab_char('$')) {
								size_t addr = line.ahextoui();
								if (label.same_str("debugbreak")) {
								// TODO: Set breakpoint in VICE
									//SetViceBP((uint16_t)addr, (uint16_t)addr, -1, true, VBP_Break, false);
								} else {
									AddSymbol((uint16_t)addr, label.get(), label.get_len());
								}
							}
						}
					}
				}
			}
			free(voidbuf);
		}
		fclose(f);
		return true;
	}
	return false;
}

void ReadSymbolsForBinary(const char *binname)
{
	strref origname = strref(binname).before_last('.');
	if (!origname) { origname = strref(binname); }

	strown<PATH_MAX_LEN> symFile(origname);
	symFile.append(".sym");

	if (ReadSymbols(symFile.c_str())) { return; }

	symFile.copy(origname);
	symFile.append(".vs");

	ReadViceCommandFile(symFile.c_str());
}

