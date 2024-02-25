#include <inttypes.h>
#include "../struse/struse.h"
#include "Views.h"
#include "../Config.h"
#include "ConsoleView.h"
#include <stdio.h>
#include "../ViceInterface.h"
#include "../Expressions.h"
#include "../6510.h"
#include "../platform.h"
#include "../Commands.h"
#include "../FileDialog.h"

static const strref command_separator(" $.");

static const char* aViceCmds[] = {
	"bank", "backtrace", "bt", "cpu",
	"cpuhistory", "chis", "dump", "export", "exp",
	"goto", "g", "io", "next", "n",
	"registers", "r", "reset", "return", "ret",
	"screen", "sc", "step", "z", "stopwatch", "sw",
	"undump",

	"add_label", "al", "delete_label", "dl", "load_labels", "ll",
	"save_labels", "sl", "show_labels", "shl", "clear_labels", "cl",

	">", "a", "compare", "c", "disass", "d", "fill", "f", "hunt", "h",
	"i", "ii", "mem", "m", "memchar", "mc", "memmapsave", "mmsave",
	"memmapshow", "mmsh", "memmamzap", "mmzap", "memsprite", "ms", "move", "t",

	"break", "bk", "command", "condition", "cond", "delete", "del", "disable", "dis",
	"enable", "en", "ignore", "until", "un", "watch", "w", "trace", "tr",

	"device", "dev", "exit", "x", "quit", "radix", "rad", "sidefx", "sfx",

	"@", "attach", "autostart",
	"autoload", "bload", "bl", "block_read", "br",
	"bsave", "bs", "block_write", "bw", "cd",
	"detach", "dir", "ls", "list",
	"load", "l", "pwd", "save", "s",

	"~", "cartfreeze", "help", "?",
	"keybuf", "playback", "pb", "print", "p",
	"record", "rec", "resourceget", "resget", "resourceset", "resset",
	"load_resources", "resload", "save_resources", "ressave", "stop",
	"screenshot", "scrsh", "tapectrl"
};

// same order as strings above
enum ViceCommands {
	VICE_BANK,
	VICE_BACKTRACE,
	VICE_BT,
	VICE_CPU,
	VICE_CPUHISTORY,
	VICE_CHIS,
	VICE_DUMP,
	VICE_EXPORT,
	VICE_EXP,
	VICE_GOTO,
	VICE_G,
	VICE_IO,
	VICE_NEXT,
	VICE_N,
	VICE_REGISTERS,
	VICE_R,
	VICE_RESET,
	VICE_RETURN,
	VICE_RET,
	VICE_SCREEN,
	VICE_SC,
	VICE_STEP,
	VICE_Z,
	VICE_STOPWATCH,
	VICE_SW,
	VICE_UNDUMP,

	VICE_ADD_LABEL,
	VICE_AL,
	VICE_DELETE_LABEL,
	VICE_DL,
	VICE_LOAD_LABELS,
	VICE_LL,
	VICE_SAVE_LABELS,
	VICE_SL,
	VICE_SHOW_LABELS,
	VICE_SHL,
	VICE_CLEAR_LABELS,
	VICE_CL,

	VICE_SETMEM,
	VICE_A,
	VICE_COMPARE,
	VICE_C,
	VICE_DISASS,
	VICE_D,
	VICE_FILL,
	VICE_F,
	VICE_HUNT,
	VICE_H,
	VICE_I,
	VICE_II,
	VICE_MEM,
	VICE_M,
	VICE_MEMCHAR,
	VICE_MC,
	VICE_MEMMAPSAVE,
	VICE_MMSAVE,
	VICE_MEMMAPSHOW,
	VICE_MMSH,
	VICE_MEMMAMZAP,
	VICE_MMZAP,
	VICE_MEMSPRITE,
	VICE_MS,
	VICE_MOVE,
	VICE_T,

	VICE_BREAK,
	VICE_BK,
	VICE_COMMAND,
	VICE_CONDITION,
	VICE_COND,
	VICE_DELETE,
	VICE_DEL,
	VICE_DISABLE,
	VICE_DIS,
	VICE_ENABLE,
	VICE_EN,
	VICE_IGNORE,
	VICE_UNTIL,
	VICE_UN,
	VICE_WATCH,
	VICE_W,
	VICE_TRACE,
	VICE_TR,

	VICE_DEVICE,
	VICE_DEV,
	VICE_EXIT,
	VICE_X,
	VICE_QUIT,
	VICE_RADIX,
	VICE_RAD,
	VICE_SIDEFX,
	VICE_SFX,

	VICE_DISKCMD,
	VICE_ATTACH,
	VICE_AUTOSTART,
	VICE_AUTOLOAD,
	VICE_BLOAD,
	VICE_BL,
	VICE_BLOCK_READ,
	VICE_BR,
	VICE_BSAVE,
	VICE_BS,
	VICE_BLOCK_WRITE,
	VICE_BW,
	VICE_CD,
	VICE_DETACH,
	VICE_DIR,
	VICE_LS,
	VICE_LIST,
	VICE_LOAD,
	VICE_L,
	VICE_PWD,
	VICE_SAVE,
	VICE_S,

	VICE_NUMBER,
	VICE_CARTFREEZE,
	VICE_HELP,
	VICE_HELP2,
	VICE_KEYBUF,
	VICE_PLAYBACK,
	VICE_PB,
	VICE_PRINT,
	VICE_P,
	VICE_RECORD,
	VICE_REC,
	VICE_RESOURCEGET,
	VICE_RESGET,
	VICE_RESOURCESET,
	VICE_RESSET,
	VICE_LOAD_RESOURCES,
	VICE_RESLOAD,
	VICE_SAVE_RESOURCES,
	VICE_RESSAVE,
	VICE_STOP,
	VICE_SCREENSHOT,
	VICE_SCRSH,
	VICE_TAPECTRL
};

#define nViceCmds ( sizeof( aViceCmds ) / sizeof( aViceCmds[ 0 ] ) )

// calculated on startup
static uint32_t aViceCmdHash[nViceCmds] = {};

static IBMutex logSafe_mutex;// = IBMutex_Clear;

IceConsole::IceConsole() : open(true)
{
	ClearLog();
	memset(InputBuf, 0, sizeof(InputBuf));
	HistoryPos = -1;
	for (size_t c = 0; c < nViceCmds; ++c) {
		aViceCmdHash[c] = strref(aViceCmds[c]).fnv1a_lower();
	}
	IBMutexInit(&logSafe_mutex, "Vice connect mutex");
	Init();
}


IceConsole::~IceConsole()
{
	ClearLog();
	IBMutexDestroy(&logSafe_mutex);
	for (int i = 0; i < History.Size; i++)
		free(History[i]);
	History.clear();
}

void IceConsole::Init()
{
	ClearLog();
	memset(InputBuf, 0, sizeof(InputBuf));
	HistoryPos = -1;
	for (size_t c = 0; c < nViceCmds; ++c) {
		aViceCmdHash[c] = strref(aViceCmds[c]).fnv1a_lower();
	}
	ViceAddLogger(LogCB, this);
	AddLog("Enter 'cmd' for console commands");
	AddLog("Enter 'help' for VICE commands when connected");
}

void IceConsole::WriteConfig(UserData& config)
{
	config.AddValue(strref("open"), config.OnOff(open));
}

void IceConsole::ReadConfig(strref config)
{
	ConfigParse conf(config);
	while (!conf.Empty()) {
		strref name, value;
		ConfigParseType type = conf.Next(&name, &value);
		if (name.same_str("open") && type == ConfigParseType::CPT_Value) {
			open = !value.same_str("Off");
		}
	}
}

void  IceConsole::ClearLog()
{
	for (int i = 0; i < Items.Size; i++)
		free(Items[i]);
	Items.clear();
	ScrollToBottom = true;
}

void IceConsole::AddLog(const char* fmt, ...)// IM_FMTARGS(2)
{
	// FIXME-OPT
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
	buf[IM_ARRAYSIZE(buf) - 1] = 0;
	va_end(args);
	Items.push_back(Strdup(buf));
	ScrollToBottom = true;
}

void IceConsole::AddLog(strref line)
{
	strown<1024> line2(line);
	Items.push_back(Strdup(line2.c_str()));
}

void IceConsole::AddLogSafe(strref line)
{
	char* copy = (char*)malloc(line.get_len() + 1);
	if (copy) {
		memcpy(copy, line.get(), line.get_len());
		copy[line.get_len()] = 0;

		IBMutexLock(&logSafe_mutex);
		safeItems.push_back(copy);
		IBMutexRelease(&logSafe_mutex);
	}
}

void IceConsole::FlushLogSafe()
{
	IBMutexLock(&logSafe_mutex);
	if (safeItems.size()) { ScrollToBottom = true; }
	for (int i = 0, n = safeItems.size(); i < n; ++i) {
		Items.push_back(safeItems[i]);
	}
	safeItems.clear();
	IBMutexRelease(&logSafe_mutex);
}

void IceConsole::Draw()
{
	if (!open) { return; }
	ImGui::SetNextWindowPos(ImVec2(600, 130), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowDockID(2, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Vice Monitor", &open)) {
		ImGui::End();
		return;
	}

	// As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar. So e.g. IsItemHovered() will return true when hovering the title bar.
	// Here we create a context menu only available from the title bar.
	if (ImGui::BeginPopupContextItem()) {
		if (ImGui::MenuItem("Close Console"))
			open = false;
		ImGui::EndPopup();
	}

	if (ImGui::SmallButton("Clear")) { ClearLog(); } ImGui::SameLine();
	bool copy_to_clipboard = ImGui::SmallButton("Copy"); ImGui::SameLine();
	if (ImGui::SmallButton("Scroll to bottom")) ScrollToBottom = true;
	//static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

	ImGui::Separator();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	static ImGuiTextFilter filter;
	filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
	ImGui::PopStyleVar();
	ImGui::Separator();

	const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(); // 1 separator, 1 input text
	ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText
	if (ImGui::BeginPopupContextWindow()) {
		if (ImGui::Selectable("Clear")) ClearLog();
		ImGui::EndPopup();
	}

	// check for new log entries added outside of drawing
	FlushLogSafe();

	// Display every line as a separate entry so we can change their color or add custom widgets. If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
	// NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping to only process visible items.
	// You can seek and display only the lines that are visible using the ImGuiListClipper helper, if your elements are evenly spaced and you have cheap random access to the elements.
	// To use the clipper we could replace the 'for (int i = 0; i < Items.Size; i++)' loop with:
	//     ImGuiListClipper clipper(Items.Size);
	//     while (clipper.Step())
	//         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
	// However, note that you can not use this code as is if a filter is active because it breaks the 'cheap random-access' property. We would need random-access on the post-filtered list.
	// A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices that passed the filtering test, recomputing this array when user changes the filter,
	// and appending newly elements as they are inserted. This is left as a task to the user until we can manage to improve this example code!
	// If your items are of variable size you may want to implement code similar to what ImGuiListClipper does. Or split your data into fixed height items to allow random-seeking into your list.
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
	if (copy_to_clipboard)
		ImGui::LogToClipboard();
	ImVec4 col_default_text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
	// don't show more than 4096
	int first_show = Items.Size < 1024 ? 0 : (Items.Size - 1024);

	for (int i = first_show; i < Items.Size; i++) {
		const char* item = Items[i];
		if (!filter.PassFilter(item))
			continue;
		ImVec4 col = col_default_text;
		if (strstr(item, "[error]")) col = ImColor(1.0f, 0.4f, 0.4f, 1.0f);
		else if (strncmp(item, "# ", 2) == 0) col = ImColor(1.0f, 0.78f, 0.58f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, col);
		ImGui::TextUnformatted(item);
		ImGui::PopStyleColor();
	}
	if (copy_to_clipboard)
		ImGui::LogFinish();
	if (ScrollToBottom)
		ImGui::SetScrollHereY(1.0f);
	ScrollToBottom = false;
	ImGui::PopStyleVar();
	ImGui::EndChild();
	ImGui::Separator();

	// Command-line
	bool reclaim_focus = false;
	if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this)) {
		char* s = InputBuf;
		Strtrim(s);
		if (s[0]) {
			ExecCommand(s);
		}
		s[0] = 0;
		reclaim_focus = true;
	}

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (reclaim_focus) {
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
	}

	ImGui::End();
}
void SendViceMonitorLine(const char* message, int size);

void IceConsole::ExecCommand(const char* command_line)
{
	AddLog("# %s\n", command_line);

	// Insert into history. First find match and delete it so it can be pushed to the back. This isn't trying to be smart or optimal.
	HistoryPos = -1;
	for (int i = History.Size - 1; i >= 0; i--) {
		if (Stricmp(History[i], command_line) == 0) {
			free(History[i]);
			History.erase(History.begin() + i);
			break;
		}
	}
	History.push_back(Strdup(command_line));

	strref line(command_line), param = line;
	strref cmd = param.split_token_any(command_separator);
	if (!cmd) { cmd = param; param.clear(); }
	uint32_t cmdHash = cmd.fnv1a_lower();

	for (size_t c = 0; c < nViceCmds; ++c) {
		if (cmdHash == aViceCmdHash[c]) {
			// forward command to vice
			strown<512> msg(command_line);
			msg.append(0x0a);
			SendViceMonitorLine(msg.get(), msg.get_len());
//			if (ViceConnected()) {
//				ViceSend(line.get(), line.get_len());
//			} else {
//				AddLog("Vice is not connected\n");
//			}
			return;
		}
	}

	param.trim_whitespace();
	// command is not a vice command, execute here
	if (cmd.same_str("connect") || cmd.same_str("cnct")) {
		// attempt vice connection
		/*if( param )*/
		{
			strref address = param.split_token(':');
			strown<32> addrchr(address);
			int port = 6510;
			if (!addrchr) { addrchr.copy("127.0.0.1"); }
			if (param) { port = (int)param.atoi(); }
			addrchr.c_str();
			ViceConnect(addrchr.charstr(), port);
		}
	} else if (cmd.same_str("pause")) {
		ViceBreak();
	} else if (cmd.same_str("eval")) {
		uint8_t rpn[512];
		bool mem = param.get_first() == '*';
		if (mem) { ++param; }
		/*uint32_t rpnLen =*/ BuildExpression(param.get(), rpn, sizeof(rpn));
		int value = EvalExpression(rpn);
		if (mem) {
			CPU6510* cpu = GetCurrCPU();
			strown<128> memStr;
			memStr.append("= ").append_num(value, 4, 16);
			for (int b = 0; b < 32; ++b) { memStr.append(' ').append_num(cpu->GetByte(uint16_t(value + b)), 2, 16); }
			AddLog("%s", memStr.c_str());
		} else {
			AddLog("= $%x", value);
		}
	} else if (cmd.same_str("font")) {
		SelectFont((int)param.atoi());
	} else if (cmd.same_str("hist") || cmd.same_str("history")) {
		int first = History.Size - 10;
		for (int i = first > 0 ? first : 0; i < History.Size; i++)
			AddLog("%3d: %s\n", i, History[i]);
	} else if (cmd.same_str("clear")) {
		ClearLog();
	} else if (cmd.same_str("cwd")) {
		char dir[512];
		if (GetCWD(dir, sizeof(dir))) {
			AddLog("\"%s\"\n", dir);
		}
	} else if (cmd.same_str("poke")) {
		if (!ViceConnected()) { AddLog("VICE Not Connected Error"); }
		else { CommandPoke(param); }
	} else if (cmd.same_str("remember")) {
		if (!ViceConnected()) { AddLog("VICE Not Connected Error"); }
		else { CommandRemember(param); }
	} else if (cmd.same_str("forget")) {
		CommandForget();
	} else if (cmd.same_str("match")) {
		if (!ViceConnected()) { AddLog("VICE Not Connected Error"); }
		else { CommandMatch(param, (int)ImGui::GetWindowSize().x / (int)ImGui::GetFont()->GetCharAdvance('D')); }
	} else if (cmd.same_str("gfxsave")) {
		if (!ViceConnected()) { AddLog("VICE Not Connected Error"); }
		else {
			strref arg = param.valid() ? param : strref("gfxsave");
			const char* mode = CommandGfxSave(arg);
			AddLog("Exported " STRREF_FMT ".txt, " STRREF_FMT ".scr (screen),\n"
				STRREF_FMT ".col (color mem) and " STRREF_FMT ".chr (characters)\nfor graphics mode %s\n",
				STRREF_ARG(arg), STRREF_ARG(arg), STRREF_ARG(arg), STRREF_ARG(arg), mode);
		}
	} else if (cmd.same_str("commands") || cmd.same_str("cmd")) {
		if (param.same_str("remember")) {
			AddLog("remember command:");
			AddLog("  remember <byte>[-<byte>] [<addr> <addr>]");
			AddLog(" Clears the matches and stores a new set");
			AddLog(" of matches in the memory range.");
			AddLog(" Identical to match <byte> <addr> C[lear]");
		} else if(param.same_str("forget")) {
			AddLog("forget command:");
			AddLog("  forget");
			AddLog(" Clears the match buffer, same as match C[lear]");
		} else if(param.same_str("match")) {
			AddLog("match command:");
			AddLog("  match <byte>[-<byte>] [<addr> <addr>] C[lear] F[ilter] T[race] W[atch]");
			AddLog(" Match will compare the byte range within");
			AddLog(" the address range to a stored list of");
			AddLog(" previous matched addresses.");
			AddLog(" byte range can be prefixed with '!' for inverted range.");
			AddLog("  If the list had previous matches it will");
			AddLog(" print out the addresses that current match.");
			AddLog(" Controls: (must be after byte/address range)");
			AddLog("  * C[lear]: clear any current results first");
			AddLog("  * F[ilter]: remove all non-matching results for another run");
			AddLog("  * T[race]: add a Trace store for the matching results");
			AddLog("  * W[atch]: add a Watch store for the matching results");
		} else if(param.same_str("poke")) {
			AddLog("poke command:");
			AddLog("  poke <addr>,<byte>");
			AddLog(" Store a byte in a specific memory location, VICE");
			AddLog(" does not need to be in a break state for this command.");
			AddLog(" addr and byte can expressions so if you have loaded");
			AddLog(" relevant symbols you can f.e. poke VICREGS+32, 3+2");
		} else {
			AddLog("Vice Console IceBro Commands");
			AddLog(" connect/cnct [<ip>:<port>] - connect to a remote host, default to 127.0.0.1:6510;");
			AddLog(" pause; font <size:0-6>; eval <exp>; history/hist;");
			AddLog(" clear, cwd, poke; remember; forget; match; gfxsave");
			AddLog(" type cmd <command> for more information on some commands.");
		}
	}
}

int IceConsole::TextEditCallbackStub(ImGuiInputTextCallbackData* data)
{
	IceConsole* console = (IceConsole*)data->UserData;
	return console->TextEditCallback(data);
}

int IceConsole::TextEditCallback(ImGuiInputTextCallbackData* data)
{
	//AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
	switch (data->EventFlag) {
		case ImGuiInputTextFlags_CallbackCompletion:
		{
			break;
		}
		case ImGuiInputTextFlags_CallbackHistory:
		{
			// Example of HISTORY
			const int prev_history_pos = HistoryPos;
			if (data->EventKey == ImGuiKey_UpArrow) {
				if (HistoryPos == -1)
					HistoryPos = History.Size - 1;
				else if (HistoryPos > 0)
					HistoryPos--;
			} else if (data->EventKey == ImGuiKey_DownArrow) {
				if (HistoryPos != -1)
					if (++HistoryPos >= History.Size)
						HistoryPos = -1;
			}

			// A better implementation would preserve the data on the current input line along with cursor position.
			if (prev_history_pos != HistoryPos) {
				const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
				data->DeleteChars(0, data->BufTextLen);
				data->InsertChars(0, history_str);
			}
		}
	}
	return 0;
}

void IceConsole::LogCB(void* user, const char *text, size_t len)
{
	if (IceConsole* iceCon = (IceConsole*)user) {
		iceCon->AddLogSafe(strref(text, (strl_t)len));
	}
}


