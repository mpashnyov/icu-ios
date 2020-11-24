DEVELOPER="$(xcode-select --print-path)"
# SDK="iphoneos"
SDK="macosx"
SDKROOT=`xcrun -sdk ${SDK} --show-sdk-path`
ARCH="x86_64"
TARGET="x86_64-apple-ios13.0-macabi"
ICU_PATH="$(pwd)/icu"
ICU_FLAGS="-I$ICU_PATH/source/common/ -I$ICU_PATH/source/tools/tzcode/ "

export CXX="$DEVELOPER/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++"
export CC="$(xcrun --sdk ${SDK} -f clang) -arch ${ARCH} -isysroot ${SDK}"
export CFLAGS="-isysroot $SDKROOT -I$SDKROOT/usr/include/ -I./include/ -arch $ARCH -target $TARGET $ICU_FLAGS"
export CXXFLAGS="-stdlib=libc++ -std=c++11 -isysroot $SDKROOT -I$SDKROOT/usr/include/ -I./include/ -arch $ARCH -target $TARGET $ICU_FLAGS"
export LDFLAGS="-stdlib=libc++ -L$SDKROOT/usr/lib/ -isysroot $SDKROOT -Wl,-dead_strip -lstdc++ -arch x86_64 -target $TARGET"

mkdir -p build-x86_64_catalist && cd build-x86_64_catalist

[ -e Makefile ] && make distclean

sh $ICU_PATH/source/configure --host=x86_64-apple-darwin --enable-static --disable-shared -with-cross-build=$ICU_PATH/../build-host
# sh $ICU_PATH/source/configure --host=x86_64-apple-darwin --enable-static --disable-shared -with-cross-build=$ICU_PATH/../build-host
