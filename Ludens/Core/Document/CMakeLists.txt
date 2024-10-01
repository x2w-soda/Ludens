set(MODULE_INCLUDE
	Include/Document.h
	Include/DocumentItem.h
	Include/DocumentLink.h
	Include/DocumentCompiler.h
)

set(MODULE_LIB
	Lib/Document.cpp
	Lib/DocumentItem.cpp
	Lib/DocumentLink.cpp
	Lib/DocumentCompiler.cpp
)

add_library(LDDocument
	"${MODULE_INCLUDE}"
	"${MODULE_LIB}"
)