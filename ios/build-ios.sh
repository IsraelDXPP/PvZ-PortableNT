#!/bin/bash
# Build PvZ-Portable for iOS (supports arm64 and 32-bit armv7 for iOS 9+ / iPad Mini 1)
# Usage: ./ios/build-ios.sh [Debug|Release] [armv7|arm64] [9.0|15.0]

set -euo pipefail

BUILD_TYPE="${1:-Release}"
ARCH="${2:-armv7}"
DEPLOY_TARGET="${3:-9.0}"

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-ios-$ARCH"

echo "=== PvZ-Portable iOS Build ($BUILD_TYPE / $ARCH / iOS $DEPLOY_TARGET) ==="

if [ -z "${VCPKG_ROOT:-}" ]; then
    echo "Error: VCPKG_ROOT is not set. Install vcpkg and set VCPKG_ROOT."
    exit 1
fi

IOS_SDK=$(xcrun --sdk iphoneos --show-sdk-path 2>/dev/null || true)
if [ -z "$IOS_SDK" ]; then
    echo "Error: iOS SDK not found. Install Xcode and run: xcode-select --install"
    exit 1
fi

TRIPLET="${ARCH}-ios"

mkdir -p "$BUILD_DIR"

# Build game (vcpkg handles all dependencies via manifest mode)
echo "--- Building PvZ-Portable ---"
cmake -B "$BUILD_DIR/game" -S "$PROJECT_ROOT" \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
    -DVCPKG_OVERLAY_TRIPLETS="$PROJECT_ROOT/ios/triplets" \
    -DVCPKG_OVERLAY_PORTS="$PROJECT_ROOT/ios/ports" \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DIOS_DEPLOYMENT_TARGET="$DEPLOY_TARGET" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOY_TARGET" \
    -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -G Xcode

cmake --build "$BUILD_DIR/game" --config "$BUILD_TYPE" -- \
    -sdk iphoneos \
    CODE_SIGN_IDENTITY="-" \
    CODE_SIGNING_ALLOWED=NO

# Create unsigned IPA
APP_PATH=$(find "$BUILD_DIR/game" -name "pvz-portable.app" -path "*${BUILD_TYPE}*" | head -1)
if [ -z "$APP_PATH" ]; then
    echo "Warning: .app bundle not found, skipping IPA creation"
else
    IPA_DIR="$BUILD_DIR/ipa"
    mkdir -p "$IPA_DIR/Payload"
    cp -R "$APP_PATH" "$IPA_DIR/Payload/"
    cd "$IPA_DIR"
    zip -r -y "$BUILD_DIR/pvz-portable-ios-$ARCH.ipa" Payload/
    rm -rf "$IPA_DIR"
    echo "IPA created: $BUILD_DIR/pvz-portable-ios-$ARCH.ipa"
fi

echo "=== iOS Build Complete ($ARCH) ==="
