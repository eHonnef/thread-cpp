# --------------------------------------------------------------------------------
#                                      FetchExternal <Change>
# --------------------------------------------------------------------------------
# Import all external libraries using FetchContent.
#
# External dependency example.
# If your project doesn't have an external dependency, remove the line below.
# --------------------------------------------------------------------------------
# FetchContent_Declare(fmtlib
#   GIT_REPOSITORY "https://github.com/fmtlib/fmt"
#   GIT_TAG "8.0.1"
#   GIT_SHALLOW TRUE
# )
# set(FMT_INSTALL ON)
# --------------------------------------------------------------------------------
# FetchContent_MakeAvailable(fmtlib)
list(APPEND lst_external)
