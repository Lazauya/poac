include_guard(GLOBAL)

message(CHECK_START "Adding Spdlog")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.9.0
)

set(SPDLOG_FMT_EXTERNAL ON)
FetchContent_MakeAvailable(spdlog)

list(APPEND POAC_DEPENDENCIES spdlog::spdlog)
message(CHECK_PASS "added")

list(POP_BACK CMAKE_MESSAGE_INDENT)
