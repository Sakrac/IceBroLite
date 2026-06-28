#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

	extern int sWindow_width, sWindow_height;

	struct sapp_event;

	void IBLCommandLine(int argc, char *argv[]);
	void IBLPreSokolSetup(void);
	void IBLInit(void);
	void IBLFrame(void);
	void IBLCleanup(void);
	void IBLEvent(const sapp_event *sokol_event);

#ifdef __cplusplus
}
#endif
