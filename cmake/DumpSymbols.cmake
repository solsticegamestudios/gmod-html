# One cross-platform Breakpad symbol pipeline:
#  dump_symbols_register(): Each shipped target records a job
#  dump_symbols_finalize(): Adds an install() hook + a standalone "symbols" target
# Both run dump_symbols_driver.cmake against the installed dist tree
# Output (all platforms): <installed binary filename>.sym next to the binary

# Capture driver path here (module scope); CMAKE_CURRENT_LIST_DIR inside a function is the caller's dir
set(_DUMP_SYMBOLS_DRIVER "${CMAKE_CURRENT_LIST_DIR}/dump_symbols_driver.cmake")

# Accumulate one job per shipped target into a global property
function(dump_symbols_register)
	set(options STRIP)
	set(oneValueArgs TARGET DESTINATION BINARY)
	cmake_parse_arguments(DSR "${options}" "${oneValueArgs}" "" ${ARGN})

	if(NOT DSR_TARGET)
		message(FATAL_ERROR "dump_symbols_register: TARGET is required")
	endif()

	# Installed binary path: Explicit BINARY override (macOS bundles) or DESTINATION + filename
	if(DSR_BINARY)
		set(_installed "${DSR_BINARY}")
	elseif(DSR_DESTINATION)
		set(_installed "${DSR_DESTINATION}/$<TARGET_FILE_NAME:${DSR_TARGET}>")
	else()
		message(FATAL_ERROR "dump_symbols_register: DESTINATION or BINARY is required")
	endif()

	# Symbol source: PDB on Windows, the installed binary itself elsewhere
	if(OS_WINDOWS)
		set(_source "$<TARGET_PDB_FILE:${DSR_TARGET}>")
	else()
		set(_source "${_installed}")
	endif()

	# Record encoded as source|output|strip (1/0); split by the driver
	if(DSR_STRIP)
		set(_strip 1)
	else()
		set(_strip 0)
	endif()
	set_property(GLOBAL APPEND PROPERTY DUMP_SYMBOLS_JOBS "${_source}|${_installed}.sym|${_strip}")
endfunction()

# Add the install hook and the standalone "symbols" target; call once after all targets exist
# Args: CEF_SYMBOL_DIR (the CEF symbol distribution) and CEF_BIN_DIR (dist dir holding the CEF binaries)
function(dump_symbols_finalize)
	set(oneValueArgs CEF_SYMBOL_DIR CEF_BIN_DIR)
	cmake_parse_arguments(DSF "" "${oneValueArgs}" "" ${ARGN})

	set(_driver "${_DUMP_SYMBOLS_DRIVER}")

	# Graceful no-op if dump_syms was not found: Warn at install and from the target
	if(NOT DUMP_SYMS)
		message(WARNING "dump_syms not found; symbol generation disabled (set -DDUMP_SYMS=<path> to enable)")
		install(CODE "message(WARNING \"dump_syms not found; skipping symbol generation\")")
		add_custom_target(symbols
			COMMAND ${CMAKE_COMMAND} -E echo "dump_syms not found; nothing to do"
			VERBATIM)
		return()
	endif()

	get_property(_jobs GLOBAL PROPERTY DUMP_SYMBOLS_JOBS)

	# Resolve generator expressions (PDB paths, $<CONFIG>) into a per-config data file the driver includes
	# Bracket args keep Windows backslash paths literal when the driver re-parses this file
	set(_lines "set(DS_JOBS)\n")
	foreach(_job ${_jobs})
		string(APPEND _lines "list(APPEND DS_JOBS [==[${_job}]==])\n")
	endforeach()
	set(_jobs_file "${CMAKE_BINARY_DIR}/dump_symbols_jobs_$<CONFIG>.cmake")
	file(GENERATE OUTPUT "${_jobs_file}" CONTENT "${_lines}")

	# INSTALL hook: Set the driver inputs, then run it as the final install step
	# install(CODE) and install(SCRIPT) share one script context, so these vars reach the driver
	# $<CONFIG> in the jobs-file path is evaluated by install(CODE)
	install(CODE "set(DS_DUMP_SYMS [[${DUMP_SYMS}]])")
	install(CODE "set(DS_STRIP [[${STRIP}]])")
	install(CODE "set(DS_STRIP_ARGS [[${STRIP_ARGS}]])")
	install(CODE "set(DS_CEF_SYMBOL_DIR [[${DSF_CEF_SYMBOL_DIR}]])")
	install(CODE "set(DS_CEF_BIN_DIR [[${DSF_CEF_BIN_DIR}]])")
	install(CODE "set(DS_JOBS_FILE [[${_jobs_file}]])")
	install(SCRIPT "${_driver}")

	# Standalone target: Regenerate syms against the install tree without a full rebuild
	add_custom_target(symbols
		COMMAND ${CMAKE_COMMAND}
			"-DDS_DUMP_SYMS=${DUMP_SYMS}"
			"-DDS_STRIP=${STRIP}"
			"-DDS_STRIP_ARGS=${STRIP_ARGS}"
			"-DDS_CEF_SYMBOL_DIR=${DSF_CEF_SYMBOL_DIR}"
			"-DDS_CEF_BIN_DIR=${DSF_CEF_BIN_DIR}"
			"-DDS_JOBS_FILE=${_jobs_file}"
			-P "${_driver}"
		VERBATIM
		COMMENT "Dumping Breakpad symbols against the install tree")
endfunction()
