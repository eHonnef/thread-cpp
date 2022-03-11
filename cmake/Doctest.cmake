if(ENABLE_DOCTESTS)
  include(CTest)
  add_definitions(-DENABLE_DOCTEST_IN_LIBRARY)

  FetchContent_Declare(
    DocTest
    GIT_REPOSITORY "https://github.com/onqtam/doctest"
    GIT_TAG "v2.4.8"
  )

  FetchContent_MakeAvailable(DocTest)
  include_directories(${DOCTEST_INCLUDE_DIR})
endif()
