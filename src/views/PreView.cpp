#include "../Files.h"

struct PreView {
public:
	void Draw(const char* title);
	bool IsOpen() const { return open; }
	PreView() : open(false)
	{
	}
protected:
	bool open;
};

void PreView::Draw(const char* title)
{
}
