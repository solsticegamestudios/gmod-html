set(TARGET html_awesomium)
set(SOURCES
	CGMHTML.cpp
	IGMHTML.h
	interface.h)

add_library(gmhtml SHARED ${SOURCES})
#add_dependencies(gmhtml html)
#target_link_libraries(gmhtml html)

set_target_properties(gmhtml PROPERTIES PREFIX "")

if(OS_WINDOWS)
	install(TARGETS gmhtml DESTINATION "${INSTALL_OUT_DIR}/GarrysMod/garrysmod/bin")
endif()
