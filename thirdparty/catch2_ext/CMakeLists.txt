include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_SHALLOW    TRUE
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.6.0)
FetchContent_MakeAvailable(Catch2)

# For catch 2 only
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/contrib)
# For catch 3 only
# list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)