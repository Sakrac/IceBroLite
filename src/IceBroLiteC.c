#include "sokol/sokol_app.h"
#include "IceBroLite.h"

#include "WindowIcon.inc"

sapp_desc sokol_main(int argc, char *argv[])
{
	IBLCommandLine(argc, argv);
	IBLPreSokolSetup();

	return (sapp_desc){
		.width = sWindow_width,
		.height = sWindow_height,
		.init_cb = IBLInit,
		.frame_cb = IBLFrame,
		.cleanup_cb = IBLCleanup,
		.event_cb = IBLEvent,
		.window_title = "Ice Bro Lite",
		.enable_clipboard = true,
		.enable_dragndrop = true,
		.icon.sokol_default = false,
		.icon.images[0].width = sIcon_Width,
		.icon.images[0].height = sIcon_Height,
		.icon.images[0].pixels = {sIcon_Pixels, sizeof(sIcon_Pixels)},
	};
}
