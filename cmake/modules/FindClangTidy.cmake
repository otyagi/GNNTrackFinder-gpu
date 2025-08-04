# Defines the following variables:
#
#   ClangTidy_FOUND - Found clang-tidy
#   CLANG_TIDY_BIN - clang-tidy executable

find_program(CLANG_TIDY_BIN
  NAMES clang-tidy
        clang-tidy-16
        clang-tidy-15
        clang-tidy-14
        clang-tidy-13
        clang-tidy-12
        clang-tidy-11
)

Message("CLANG_TIDY_BIN: ${CLANG_TIDY_BIN}")

#list(APPEND required_tidy_checks
#  modernize-deprecated-headers
#  modernize-use-nullptr
#)

# Extract the list of checks from the clang-tidy configuration
# The line looks like: "Checks:          '-*,modernize-deprecated-headers'"
# Remove everything beside the part between the quotes and convert the
# string to a list
# The checks must not contain any wildcards
# IDEA: Check if one can extract the same information from clang-tidy --dump-version

file(STRINGS ${CMAKE_SOURCE_DIR}/.clang-tidy required_tidy_checks_from_file REGEX "Checks.*")

string(LENGTH ${required_tidy_checks_from_file} _length)
string(FIND ${required_tidy_checks_from_file} "'" _pos_first_quote)
math(EXPR _pos_first_quote ${_pos_first_quote}+1)
string(SUBSTRING ${required_tidy_checks_from_file} ${_pos_first_quote} ${_length} required_tidy_checks_from_file)

string(FIND ${required_tidy_checks_from_file} "'" _pos_last_quote)
string(SUBSTRING ${required_tidy_checks_from_file} 0 ${_pos_last_quote} required_tidy_checks_from_file)

string(REPLACE "," ";" required_tidy_checks ${required_tidy_checks_from_file})

message(VERBOSE "required_tidy_checks: ${required_tidy_checks}")
if (CLANG_TIDY_BIN)
  # Loop over list of required checks
  # Succeed if all required checks are supported by the clang version

  execute_process(COMMAND  ${CLANG_TIDY_BIN} -checks=-*,modernize-* --list-checks
                  OUTPUT_VARIABLE available_tidy_checks
                 )
  message(VERBOSE "Available clang-tidy checks: ${available_tidy_checks}")

  set(CLANG_TIDY_CHECKS_SUPPORTED TRUE)
  foreach(check IN LISTS required_tidy_checks)
    # Skip all checks which contain wildcards
    string(REGEX MATCH "\\*" _wildcard ${check})
    if(_wildcard)
      message("The checker doesn't support wildcards in the check")
      string(REGEX MATCH "^-\\*" _wildcard ${check})
      if(_wildcard)
        message("The check ${check} is droped.")
        continue()
      else()
        message("The check ${check} violates this. clang-tidy support is disabled.")
        set(CLANG_TIDY_CHECKS_SUPPORTED FALSE)
        break()
      endif()
    endif()
    string(FIND "${available_tidy_checks}" "${check}" check_avail)
    if(${check_avail} EQUAL -1)
      message("clang-tidy doesn't support the needed Check ${check}. clang-tidy support is disabled.")
      set(CLANG_TIDY_CHECKS_SUPPORTED FALSE)
      break()
    endif()
    message("Used check: ${check}")
  endforeach()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ClangTidy
  REQUIRED_VARS CLANG_TIDY_BIN CLANG_TIDY_CHECKS_SUPPORTED
)

if(ClangTidy_FOUND)
  message("The found clang tidy supports all requested checks.")
endif()
