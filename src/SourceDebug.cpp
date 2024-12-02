#include "../struse/struse.h"
#include "../struse/xml.h"
#include "Sym.h"
#include "Files.h"
#include <stdlib.h>
#include <vector>
#include <assert.h>
#include "ViceInterface.h"
#include "Breakpoints.h"
#include "SourceDebug.h"
#include "platform.h"

// Format:
//	parse as XML
//	Sources => list of files with indices
//	Segments => segment names, addresses & line numbers
//	Labels => segment, name, address

struct SourceDebugLine {
	const char* line;
	uint8_t len;		// no purpose in showing >255 chars
	uint8_t spaces;		// for easy white space scaling
	uint8_t block;		// not quite sure how blocks are useful but..
};

struct SourceDebugSegment {
	enum { MAX_SEG_NAME_LEN = 64 };
	uint16_t addrFirst, addrLast;
	SourceDebugLine* lines;
	strref* blockNames;	// indexed by lines->block
	strref name;
};

struct SourceDebug {
	std::vector<SourceDebugSegment> segments; // contains blocks which contains lines
	std::vector<void*> files; // segments reference strings in these files directly
};

SourceDebug* sSourceDebug = nullptr;

char* sListing = nullptr;
size_t sListingSize = 0;
static IBMutex sSrcDbgMutex;


strref GetSourceAt(uint16_t addr, int &spaces)
{
	if (sSourceDebug) {
		IBMutexLock(&sSrcDbgMutex);
		for (size_t s = 0, n = sSourceDebug->segments.size(); s < n; ++s) {
			const SourceDebugSegment& seg = sSourceDebug->segments[s];
			if (seg.addrFirst <= addr && seg.addrLast >= addr) {
				if (IsSectionVisible(seg.name.fnv1a_64())) {
					const SourceDebugLine& line = seg.lines[addr - seg.addrFirst];
					if (line.line) {
						spaces = line.spaces;
						IBMutexRelease(&sSrcDbgMutex);
						return strref(line.line, (strl_t)line.len);
					}
				}
			}
		}
		IBMutexRelease(&sSrcDbgMutex);
	}
	return strref();
}

// clean just the source debug references not the original files
void ClearSourceDebugMap()
{
	if (SourceDebug* dbg = sSourceDebug) {
		IBMutexLock(&sSrcDbgMutex);
		while (!dbg->segments.empty()) {
			SourceDebugSegment& seg = dbg->segments.back();
			free(seg.lines);
			free(seg.blockNames);
			dbg->segments.pop_back();
		}
		while (!dbg->files.empty()) {
			void* file = dbg->files.back();
			if (file != sListing) {
				free(file);
			}
			dbg->files.pop_back();
		}
		free(dbg);
		sSourceDebug = nullptr;
		IBMutexRelease(&sSrcDbgMutex);
	}
}

void ClearSourceDebug()
{
	IBMutexLock(&sSrcDbgMutex);
	if (SourceDebug* dbg = sSourceDebug) {
		sSourceDebug = nullptr;
		while (dbg->segments.size()) {
			SourceDebugSegment& seg = dbg->segments[dbg->segments.size() - 1];
			free(seg.lines);
			free(seg.blockNames);
			dbg->segments.pop_back();
		}
		while (dbg->files.size()) {
			void* file = dbg->files[dbg->files.size() - 1];
			if (file != sListing) {
				free(dbg->files[dbg->files.size() - 1]);
			}
			dbg->files.pop_back();
		}
		free(dbg);
	}
	IBMutexRelease(&sSrcDbgMutex);
	if (sListing) {
		free(sListing);
		sListing = nullptr;
		sListingSize = 0;
	}
}

void InitSourceDebug()
{
	IBMutexInit(&sSrcDbgMutex, "Source Debug");
}

void ShutdownSourceDebug()
{
	ClearSourceDebug();

	IBMutexDestroy(&sSrcDbgMutex);
}

// These structs are for parsing the XML, gets converted to a SourceDebug when all is available

struct ParseDebugSource {
	void* file;
	size_t size;
	// temp line index -> buffer offset
	std::vector<uint32_t> lineOffsets;
	ParseDebugSource() : file(nullptr), size(0) {}
};

struct ParseDebugLine {
	strref line;
	uint16_t first, last;
};

struct ParseDebugBlock {
	strref name;
	std::vector<ParseDebugLine> lines;
};

struct ParseDebugSegment {
	strref name;
	std::vector<ParseDebugBlock*> blocks;
};

struct ParseDebugText {
	strref path; // the path from the filename with the trailing slash, or empty
	strref segment;
	std::vector<ParseDebugSource*> files;
	std::vector<ParseDebugSegment*> segments;
};

bool C64DbgXMLCB(void* user, strref tag_or_data, const strref* tag_stack, int size_stack, XML_TYPE type)
{
	ParseDebugText* parse = (ParseDebugText*)user;

	if (type == XML_TYPE::XML_TYPE_TEXT && size_stack) {
		if (tag_stack->get_word().same_str("Sources")) {
			// TODO consider reading in the order attribute.. hopefully it is just for info.
			while (strref line = tag_or_data.line()) {
				strref idstr = line.split_token_trim(',');
				uint32_t id = (uint32_t)idstr.atoi();
				strown<PATH_MAX_LEN> file;
				if (line.find(':') < 0) { file.append(parse->path); }
				file.append(line);
				while (parse->files.size() <= id) { parse->files.push_back(nullptr); }
				ParseDebugSource* source = new ParseDebugSource();
				parse->files[id] = source;
				source->file = LoadBinary(file.c_str(), source->size);
				if (source->file) {
					const char* start = (const char*)source->file;
					strref read(start, (strl_t)source->size);
					source->lineOffsets.reserve(read.count_lines());
					while (read) {
						strref num_line = read.next_line();
						source->lineOffsets.push_back((uint32_t)(num_line.get() - start));
					}
				}
			}
		} else if (tag_stack->get_word().same_str("Block")) {
			ParseDebugSegment* seg = nullptr;
			for (size_t s = 0; s < parse->segments.size(); ++s) {
				if (parse->segments[s]->name.same_str(parse->segment)) {
					seg = parse->segments[s]; break;
				}
			}
			if (!seg) {
				seg = new ParseDebugSegment;
				seg->name = parse->segment;
				parse->segments.push_back(seg);
			}
			strref blockName = XMLFindAttr(tag_or_data, strref("name"));
			ParseDebugBlock* block = nullptr;
			for (size_t b = 0; b < seg->blocks.size(); ++b) {
				if (seg->blocks[b]->name.same_str_case(blockName)) {
					block = seg->blocks[b]; break;
				}
			}
			if (!block) {
				block = new ParseDebugBlock;
				block->name = blockName;
				seg->blocks.push_back(block);
			}
			while (strref line = tag_or_data.line()) {
				//"START,END,FILE_IDX,LINE1,COL1,LINE2,COL2"
				strref start = line.split_token_trim(',');
				strref last = line.split_token_trim(',');
				strref file = line.split_token_trim(',');
				strref row = line.split_token_trim(','); // line is now col, last line, last col
				strref col1 = line.split_token_trim(',');

				if (start && last && file && row) {
					if (start.get_first() == '$') { ++start; }
					if (last.get_first() == '$') { ++last; }
					size_t file_num = (size_t)file.atoui();
					if (file_num < parse->files.size()) {
						ParseDebugSource* source = parse->files[file_num];
						size_t row_num = (size_t)row.atoui();
						if (row_num && source->file && row_num <= source->lineOffsets.size()) {
							strref srcTxt = strref((const char*)source->file, strl_t(source->size));
							strl_t c1 = (strl_t)col1.atoui();
							if (c1) { c1--; }
							srcTxt += source->lineOffsets[row_num - 1];	// debug lines start at 1!
							srcTxt += c1;
							strref srcLine = srcTxt.get_line();
							if (srcLine) {
								ParseDebugLine dbgLine = { srcLine, (uint16_t)start.ahextoui(), (uint16_t)last.ahextoui() };
								block->lines.push_back(dbgLine);
							}
						}
					}
				}
			}
		} else if (tag_stack->get_word().same_str("Labels")) {
			tag_or_data.trim_whitespace();
			if (tag_or_data) {
				//ViceSetUpdateSymbols(false);
				ClearSymbols();
				while (strref label = tag_or_data.line()) {
					strref seg = label.split_token_trim(',');
					strref addr = label.split_token_trim(',');
					label = label.split_token_trim(',');
					if (addr.get_first() == '$') { ++addr; }
					if (label) {
						AddSymbol((uint16_t)addr.ahextoui(),
								  label.get(), label.get_len(),
								  seg.get(), seg.get_len());
					}
				}
				FilterSectionSymbols();
			}
		} else if (tag_stack->get_word().same_str("Breakpoints")) {
			//<Breakpoints values="SEGMENT,ADDRESS,ARGUMENT">
			tag_or_data.trim_whitespace();
			if (tag_or_data) { RemoveAllBreakpoints(); }
			while (strref bkpt = tag_or_data.line()) {
				/*strref seg =*/ bkpt.split_token_trim(',');
				strref addr = bkpt.split_token_trim(',');
				/*strref cond =*/ bkpt.split_token_trim(',');
				if (addr.get_first() == '$') { ++addr; }
				ViceAddBreakpoint((uint16_t)addr.ahextoui());
				// TODO: Also send condition for breakpoint void ViceSetCondition(int checkPoint, strref condition)
			}

		}
	} else if (type == XML_TYPE::XML_TYPE_TAG_OPEN) {
		if (tag_or_data.get_word().same_str("Segment")) {
			parse->segment = XMLFindAttr(tag_or_data, strref("name"));
		}
	}
	return true;
}

bool ReadC64DbgSrc(const char* filename)
{
	ParseDebugText parse;
	parse.path = strref(filename).before_last('/', '\\');
	parse.segment.clear(); // just in case there are blocks without segments I guess
	if (parse.path.get_len()) { parse.path = strref(parse.path.get(), parse.path.get_len() + 1); }
	size_t size;
	bool success = false;
	if (void* voidbuf = LoadBinary(filename, size)) {
		ClearSourceDebug();
		ClearSymbols();
		IBMutexLock(&sSrcDbgMutex);
		if (ParseXML(strref((const char*)voidbuf, (strl_t)size), C64DbgXMLCB, &parse)) {
			SourceDebug* dbg = new SourceDebug;
			sSourceDebug = dbg;

			// remember the file pointers for later cleanup
			dbg->files.reserve(parse.files.size());
			for (size_t f = 0; f < parse.files.size(); ++f) {
				dbg->files.push_back(parse.files[f]->file);
			}

			// segments depend on if they have data or not, could be empty.
			for (size_t s = 0; s < parse.segments.size(); ++s) {
				ParseDebugSegment* seg = parse.segments[s];

				// determine address range of segment
				uint16_t addrFirst = 0xffff, addrLast = 0x0000;
				for (size_t b = 0; b < seg->blocks.size(); ++b) {
					ParseDebugBlock* blk = seg->blocks[b];
					for (size_t l = 0; l < blk->lines.size(); ++l) {
						ParseDebugLine* lin = &blk->lines[l];
						if (addrFirst > lin->first) { addrFirst = lin->first; }
						if (addrLast < lin->last) { addrLast = lin->last; }
					}
				}

				// segment not empty -> add it!
				if (addrFirst <= addrLast) {
					dbg->segments.push_back(SourceDebugSegment());
					SourceDebugSegment* segSrc = &dbg->segments[dbg->segments.size() - 1];
					segSrc->addrFirst = addrFirst;
					segSrc->addrLast = addrLast;
					segSrc->lines = (SourceDebugLine*)calloc((size_t)addrLast + 1 - (size_t)addrFirst, sizeof(SourceDebugLine));
					segSrc->blockNames = (strref*)calloc(seg->blocks.size(), sizeof(strref));
					segSrc->name = seg->name;
					for (size_t b = 0; b < seg->blocks.size(); ++b) {
						ParseDebugBlock* blk = seg->blocks[b];
						// copy block name
						segSrc->blockNames[b] = blk->name;
						// fill in addresses with line info
						for (size_t l = 0; l < blk->lines.size(); ++l) {
							ParseDebugLine* lin = &blk->lines[l];
							uint16_t ft = lin->first, lt = lin->last;
							if (ft <= lt) {
								for (uint16_t a = ft; a <= lt; ++a) {
									assert(a <= addrLast);
									SourceDebugLine* ln = segSrc->lines + (a-addrFirst);
									ln->block = (uint8_t)b;
									strref lineStr = lin->line;
									uint8_t spaces = 0;
									while (lineStr.get_first() <= 0x20 && spaces < 255) {
										if (lineStr.get_first() == '\t') { spaces += 4; }
										else { ++spaces; }
										++lineStr;
									}
									ln->spaces = spaces;
									ln->line = lineStr.get();
									ln->len = lineStr.get_len() < 256 ? lineStr.get_len() : 255;
								}
							}
						}
					}
				}
			}
			success = true;
		}
		// clear up ParseDebugText
		while (parse.files.size()) {
			delete parse.files[parse.files.size() - 1];
			parse.files.pop_back();
		}
		while (parse.segments.size()) {
			ParseDebugSegment* segment = parse.segments[parse.segments.size() - 1];
			while (segment->blocks.size()) {
				delete segment->blocks[segment->blocks.size() - 1];
				segment->blocks.pop_back();
			}
			parse.segments.pop_back();
		}
		IBMutexRelease(&sSrcDbgMutex);
	}
	return success;
}

bool ReadListingFile(const char* filename)
{
	size_t listSize;
	if (uint8_t* listingFile = LoadBinary(filename, listSize)) {
		if (sListing) {
			ClearSourceDebug();
			free(sListing);
		}
		sListing = (char*)listingFile;
		sListingSize = listSize;
		return true;
	}
	return false;
}

strref GetListingFile()
{
	return strref(sListing, (strl_t)sListingSize);
}

static bool CheckAddrLine(strref& line, uint16_t &addr_ret)
{
	strref chk = line;
	if (chk[0] == '$') {++chk;}
	strref hexStr = chk.split(4);
	for (int i = 0; i < 4; ++i) {
		if (!strref::is_hex(hexStr[i])) {
			return false;
		}
	}
	line = chk;
	addr_ret = (uint16_t)hexStr.ahextoui();
	return true;
}

void ListingToSrcDebug(int column)
{
	ClearSourceDebugMap();

	if (!sSourceDebug) {
		sSourceDebug = new SourceDebug;
	}
	if (sSourceDebug->files.size() == 0)  {
		sSourceDebug->files.push_back(sListing);
	}

	strref list = GetListingFile();
	SourceDebugSegment seg;

	std::vector<ParseDebugLine> parseLines;

	while (strref line = list.line()) {
		uint16_t addr;
		strref left = line.split(column);
		if (CheckAddrLine(left, addr)) {
			if (strref("org").is_prefix_word(line)) { continue; }	// not useful info
			ParseDebugLine pdl = { line, addr, addr };
			parseLines.push_back(pdl);
		}
	}

	uint16_t addrFirst = 0xffff, addrLast = 0x0000;
	for (size_t l = 0; l < parseLines.size(); ++l) {
		ParseDebugLine* lin = &parseLines[l];
		if (addrFirst > lin->first) { addrFirst = lin->first; }
		if (addrLast < lin->last) { addrLast = lin->last; }
	}

	// segment not empty -> add it!
	if (addrFirst <= addrLast) {
		sSourceDebug->segments.push_back(SourceDebugSegment());
		SourceDebugSegment* segSrc = &sSourceDebug->segments[sSourceDebug->segments.size() - 1];
		segSrc->addrFirst = addrFirst;
		segSrc->addrLast = addrLast;
		segSrc->lines = (SourceDebugLine*)calloc(size_t(addrLast) + 1 - size_t(addrFirst), sizeof(SourceDebugLine));
		segSrc->blockNames = (strref*)calloc(1, sizeof(strref));
		segSrc->name = "Listing";
		if (segSrc->blockNames) { segSrc->blockNames[0] = "Listing"; }
			// fill in addresses with line info
		for (size_t l = 0; l < parseLines.size(); ++l) {
			ParseDebugLine* lin = &parseLines[l];
			uint16_t ft = lin->first, lt = lin->last;
			if (ft <= lt) {
				for (uint16_t a = ft; a <= lt; ++a) {
					SourceDebugLine* ln = segSrc->lines + (a - addrFirst);
					if (ln->block) { ln->block = 0; }
					strref lineStr = lin->line;
					uint8_t spaces = 0;
					while (lineStr.get_first() <= 0x20 && spaces < 255) {
						if (lineStr.get_first() == '\t') { spaces += 4; } else { ++spaces; }
						++lineStr;
					}
					ln->spaces = spaces;
					ln->line = lineStr.get();
					ln->len = lineStr.get_len() < 256 ? lineStr.get_len() : 255;
				}
			}
		}
	}
}
