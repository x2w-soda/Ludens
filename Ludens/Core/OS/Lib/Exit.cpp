#include <cstdlib>
#include "Core/OS/Include/Exit.h"
#include "Core/DSA/Include/Vector.h"

namespace LD {

	static Vector<void (*)()> sExitCallbacks;

	static void ExitProxy()
	{
		for (int i = (int)sExitCallbacks.Size() - 1; i >= 0; i--)
		{
			sExitCallbacks[i]();
		}
	}

	void Exit(int exit_code)
	{
		std::exit(exit_code);
	}

	void AtExit(void(*fn)())
	{
		if (sExitCallbacks.IsEmpty())
		{
			std::atexit(ExitProxy);
		}

		sExitCallbacks.PushBack(fn);
	}

} // namespace LD