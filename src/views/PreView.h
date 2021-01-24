#pragma once
class strref;
struct PreView {
public:
	enum class Mode {
		Hidden,
		Listing,
		SourceContext
	};

	void Draw();
	bool IsOpen() const { return open; }
	void ShowListing(strref listing);
	PreView() : open(false), mode(Mode::Hidden)
	{
		listingAddrColumn = 0;
		listingCodeColumn = 40;
		currFileTopOffs = 0;
		currFileLine = 0;
		currFileNumLines = 0;
	}
protected:
	bool open;
	Mode mode;

	int listingAddrColumn;
	int listingCodeColumn;
	strref currFile;
	uint32_t currFileTopOffs;
	uint32_t currFileLine;
	uint32_t currFileNumLines;
};

