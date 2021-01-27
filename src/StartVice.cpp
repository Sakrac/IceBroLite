#include <stdio.h>
#include <stdlib.h>
#include "struse/struse.h"
#include "Files.h"
#include "FileDialog.h"
#include "views/FilesView.h"
#include "views/Views.h"

bool LoadViceEXE();
void WaitForViceEXEPath()
{
	if (LoadViceEXEPathReady()) {
		LoadViceEXE();
	}
}

#ifdef _WIN32
#include <windows.h>

// If CreateProcess succeeds, it returns a PROCESS_INFORMATION 
// structure containing handlesand identifiers for the new processand
// its primary thread.The threadand process handles are created with
// full access rights, although access can be restricted if you specify
// security descriptors.When you no longer need these handles,
// close them by using the CloseHandle function.
// You can also create a process using the CreateProcessAsUser or
// CreateProcessWithLogonW function.This allows you to specify the
// security context of the user account in which the process will execute.


bool LoadViceEXE()
{
	char* viceEXEPath = GetViceEXEPath();
	if (viceEXEPath == nullptr) {
		return false;
	}
	STARTUPINFO si = {};
	PROCESS_INFORMATION pi = {};

	si.cb = sizeof(si);

	// Start the child process. 
	if (!CreateProcess(NULL,   // No module name (use command line)
					   viceEXEPath,        // Command line
					   NULL,           // Process handle not inheritable
					   NULL,           // Thread handle not inheritable
					   FALSE,          // Set handle inheritance to FALSE
					   0,              // No creation flags
					   NULL,           // Use parent's environment block
					   NULL,           // Use parent's starting directory 
					   &si,            // Pointer to STARTUPINFO structure
					   &pi)           // Pointer to PROCESS_INFORMATION structure
		) {
		//printf("CreateProcess failed (%d).\n", GetLastError());
		return false;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return true;
}

#elif defined(__linux__)


// The standard UNIX way is to use fork(2) followed by an exec(3)
// call(there's a whole family of them—choose whichever suits
// your needs the best).

#include <unistd.h>

bool LoadVIceEXE()
{
	char* viceEXEPath = GetViceEXEPath();
	if (viceEXEPath == nullptr) {
		return false;
	}
	pid_t     pid;

	printf("before fork\n");
	if ((pid = fork()) < 0) {

		//It may fail -- super rare
		perror("Fork failed");

	} else if (pid > 0) {
		//If it returns a positive number, you're in the parent process and pid holds the pid of the child

		printf("Mah kid's pid is %d\n", pid);
		printf("Mine's %d\n", getpid());

	} else {
		//If it returns zero, you're in the child process

		//you can do some preparatory work here (e.g., close filedescriptors)

		printf("I'm the child and my pid is %d\n", getpid());

		//exec will replace the process image with that of echo (wherever in the PATH environment variable it is be found (using the execlP version here)
		execlp(viceEXEPath, "echo", "hello world from echo; I'm now the child because I've replaced the original child because it called an exec function", (char*)NULL);

		printf("This won't run because now we're running the process image of the echo binary. Not this.");
	}

	return EXIT_SUCCESS;
}

#endif
