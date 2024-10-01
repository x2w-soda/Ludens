#pragma once

namespace LD {

	// program termination, invokes all registered callbacks
	void Exit(int exit_code);

	// guarentees registering any number of callbacks, which are called
	// back to back in reverse order upon program termination
	void AtExit(void (*fn)());

} // namespace LD