NDK_TOOLCHAIN_VERSION := clang
APP_PLATFORM := android-14
# APP_STL has to match CMAKE_ANDROID_STL_TYPE
APP_STL := c++_static
APP_ABI := armeabi-v7a
APP_MODULES := sfml-activity-d sfml-example
APP_OPTIM := release
APP_CFLAG := -g -ggdb -O0
