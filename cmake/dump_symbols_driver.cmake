# Install-time / standalone worker; dumps Breakpad symbols for our binaries + CEF
# Output: <binary>.sym next to each binary in the install tree
# Inputs (via install(CODE) or -D): DS_DUMP_SYMS, DS_STRIP, DS_STRIP_ARGS, DS_CEF_SYMBOL_DIR, DS_CEF_BIN_DIR, DS_JOBS_FILE

if(NOT DS_DUMP_SYMS)
	message(WARNING "dump_syms not set; skipping symbol generation")
	return()
endif()

# Dump one binary: <source> (PDB on Windows, binary elsewhere) -> <output>.sym, then optional strip
function(dump_one source output strip)
	if(NOT EXISTS "${source}")
		message(STATUS "Symbols: source missing, skipping ${source}")
		return()
	endif()
	message(STATUS "Symbols: ${output}")
	execute_process(
		COMMAND "${DS_DUMP_SYMS}" -o "${output}" "${source}"
		RESULT_VARIABLE _rc)
	if(NOT _rc EQUAL 0)
		message(WARNING "dump_syms failed (${_rc}) for ${source}")
		return()
	endif()

	# Strip the installed binary after dumping (Linux/macOS only; never on Windows or CEF)
	if(strip AND DS_STRIP)
		separate_arguments(_strip_args NATIVE_COMMAND "${DS_STRIP_ARGS}")
		execute_process(COMMAND "${DS_STRIP}" ${_strip_args} "${source}")
	endif()
endfunction()

# Our binaries: Jobs resolved at configure time into DS_JOBS_FILE
if(DS_JOBS_FILE AND EXISTS "${DS_JOBS_FILE}")
	include("${DS_JOBS_FILE}")
endif()
foreach(_job ${DS_JOBS})
	string(REPLACE "|" ";" _parts "${_job}")
	list(GET _parts 0 _source)
	list(GET _parts 1 _output)
	list(GET _parts 2 _strip)
	dump_one("${_source}" "${_output}" "${_strip}")
endforeach()

# CEF binaries: Dump every symbol file in the CEF symbol distribution next to its binary
# Windows ships <X>.dll.pdb; the glob naturally covers only the DLLs that have symbols
# Never stripped; the Linux/macOS symbol layout differs and is NOT verified on Windows
# The glob below uses the same pattern but must be confirmed at the next Linux/macOS build
if(DS_CEF_SYMBOL_DIR AND DS_CEF_BIN_DIR)
	if(WIN32)
		# Windows: <X>.dll.pdb -> <X>.dll.sym beside <X>.dll in the dist CEF bin dir
		file(GLOB _cef_pdbs "${DS_CEF_SYMBOL_DIR}/*.pdb")
		foreach(_pdb ${_cef_pdbs})
			get_filename_component(_name "${_pdb}" NAME) # e.g. libcef.dll.pdb
			string(REGEX REPLACE "\\.pdb$" "" _binname "${_name}") # libcef.dll
			dump_one("${_pdb}" "${DS_CEF_BIN_DIR}/${_binname}.sym" 0)
		endforeach()
	else()
		# Linux/macOS CEF - VERIFY the symbol-distribution layout at the next build there
		# Linux: Same-named .so with debug info -> keep the full name
		# macOS: .dSYM bundles (directories) -> strip the .dSYM suffix
		file(GLOB _cef_syms "${DS_CEF_SYMBOL_DIR}/*")
		if(NOT _cef_syms)
			message(STATUS "Symbols: no CEF symbol distribution at ${DS_CEF_SYMBOL_DIR}; skipping CEF (verify on Linux/macOS)")
		endif()
		foreach(_sym ${_cef_syms})
			get_filename_component(_name "${_sym}" NAME)
			if(_name MATCHES "\\.dSYM$")
				string(REGEX REPLACE "\\.dSYM$" "" _binname "${_name}")
				dump_one("${_sym}" "${DS_CEF_BIN_DIR}/${_binname}.sym" 0)
			elseif(NOT IS_DIRECTORY "${_sym}")
				dump_one("${_sym}" "${DS_CEF_BIN_DIR}/${_name}.sym" 0)
			endif()
		endforeach()
	endif()
endif()
