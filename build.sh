#!/usr/bin/env bash
# Fast macOS/Linux build entry point for the SDL3/Clay UI reference app.
# Mirrors build.ps1's Windows behavior.
#
# Usage:
#   ./build.sh                    # build Debug hello; configure only if needed
#   ./build.sh --run              # build then run hello
#   ./build.sh --tests            # build all then run ctest
#   ./build.sh --target shooter_ui_tests
#   ./build.sh --config Release
#   ./build.sh --clean
#   ./build.sh --configure

set -euo pipefail

CONFIG="Debug"
TARGET="hello"
TARGET_EXPLICIT=0
BUILD_DIR=""
CLEAN=0
FORCE_CONFIGURE=0
RUN=0
TESTS=0
RUN_ARGS=()

usage() {
    cat <<'EOF'
Usage:
  ./build.sh                    build Debug hello; configure only if needed
  ./build.sh --run              build then run hello
  ./build.sh --tests            build all then run ctest
  ./build.sh --target <name>    build one target, e.g. shooter_ui_tests
  ./build.sh --config Release   use cmake-build-release
  ./build.sh --clean            remove CMakeCache.txt + CMakeFiles first, then configure
  ./build.sh --configure        force CMake configure, then build
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --config|-Config)      CONFIG="$2"; shift 2 ;;
        --target|-Target)      TARGET="$2"; TARGET_EXPLICIT=1; shift 2 ;;
        --build-dir|-BuildDir) BUILD_DIR="$2"; shift 2 ;;
        --clean|-Clean)        CLEAN=1; shift ;;
        --configure|-Configure) FORCE_CONFIGURE=1; shift ;;
        --run|-Run)            RUN=1; shift ;;
        --tests|-Tests)        TESTS=1; shift ;;
        -h|--help|-Help)       usage; exit 0 ;;
        --)                    shift; RUN_ARGS+=("$@"); break ;;
        *)                     RUN_ARGS+=("$1"); shift ;;
    esac
done

case "$CONFIG" in
    Debug|Release|RelWithDebInfo) ;;
    *) echo "build.sh: invalid --config '$CONFIG'" >&2; exit 1 ;;
esac

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -z "$BUILD_DIR" ]]; then
    SUFFIX="$(echo "$CONFIG" | tr '[:upper:]' '[:lower:]')"
    BUILD_DIR="$REPO_DIR/cmake-build-$SUFFIX"
elif [[ "$BUILD_DIR" != /* ]]; then
    BUILD_DIR="$REPO_DIR/$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

info() { printf '\033[36mbuild.sh: %s\033[0m\n' "$*"; }
fail() { printf '\033[31mbuild.sh: %s\033[0m\n' "$*" >&2; exit 1; }

BUILD_TARGET="$TARGET"
if [[ $TESTS -eq 1 && $TARGET_EXPLICIT -eq 0 ]]; then
    BUILD_TARGET="all"
fi

printf '\033[90mbuild.sh: build dir = %s\033[0m\n' "$BUILD_DIR"
printf '\033[90mbuild.sh: target    = %s\033[0m\n' "$BUILD_TARGET"
printf '\033[90mbuild.sh: config    = %s\033[0m\n' "$CONFIG"

if [[ $CLEAN -eq 1 ]]; then
    info "cleaning cache in $BUILD_DIR"
    rm -f  "$BUILD_DIR/CMakeCache.txt"
    rm -rf "$BUILD_DIR/CMakeFiles"
fi

LOCK_FILE="$BUILD_DIR/.ui-build.lock"
LOCK_DIR="${LOCK_FILE}.d"
cleanup_lock() { rm -rf "$LOCK_DIR" 2>/dev/null || true; }
trap cleanup_lock EXIT

if command -v flock >/dev/null 2>&1; then
    exec 9>"$LOCK_FILE"
    if ! flock -n 9; then
        owner_pid="$(cat "$LOCK_FILE" 2>/dev/null || true)"
        fail "another build holds the lock${owner_pid:+ (PID $owner_pid)} on $BUILD_DIR."
    fi
    echo "$$" >&9
else
    if ! mkdir "$LOCK_DIR" 2>/dev/null; then
        owner_pid="$(cat "$LOCK_DIR/pid" 2>/dev/null || true)"
        if [[ -n "$owner_pid" ]] && kill -0 "$owner_pid" 2>/dev/null; then
            fail "another build holds the lock (PID $owner_pid) on $BUILD_DIR."
        fi
        info "reclaiming stale lock in $BUILD_DIR"
        rm -rf "$LOCK_DIR"
        mkdir "$LOCK_DIR" || fail "could not create lock $LOCK_DIR"
    fi
    echo "$$" >"$LOCK_DIR/pid"
fi

GENERATOR=""
if command -v ninja >/dev/null 2>&1; then
    GENERATOR="Ninja"
fi

CACHE_FILE="$BUILD_DIR/CMakeCache.txt"
NINJA_FILE="$BUILD_DIR/build.ninja"
MAKE_FILE="$BUILD_DIR/Makefile"

needs_configure=0
if [[ $FORCE_CONFIGURE -eq 1 || $CLEAN -eq 1 ]]; then
    needs_configure=1
elif [[ ! -f "$CACHE_FILE" ]]; then
    needs_configure=1
elif [[ "$GENERATOR" == "Ninja" && ! -f "$NINJA_FILE" ]]; then
    needs_configure=1
elif [[ "$GENERATOR" != "Ninja" && ! -f "$MAKE_FILE" ]]; then
    needs_configure=1
fi

if [[ $needs_configure -eq 1 ]]; then
    info "configuring"
    if [[ -n "$GENERATOR" ]]; then
        cmake -S "$REPO_DIR" -B "$BUILD_DIR" -G "$GENERATOR" -DCMAKE_BUILD_TYPE="$CONFIG"
    else
        cmake -S "$REPO_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CONFIG"
    fi
else
    info "configure skipped; using existing build tree"
fi

JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
cmake --build "$BUILD_DIR" --target "$BUILD_TARGET" --config "$CONFIG" -j "$JOBS"

if [[ $TESTS -eq 1 ]]; then
    info "running tests"
    ctest --test-dir "$BUILD_DIR" --output-on-failure -C "$CONFIG"
fi

printf '\033[32mbuild.sh: OK (%s/%s)\033[0m\n' "$CONFIG" "$BUILD_TARGET"

if [[ $RUN -eq 1 ]]; then
    HELLO="$BUILD_DIR/hello"
    [[ -x "$HELLO" ]] || fail "cannot run: $HELLO does not exist."
    info "running $HELLO"
    exec "$HELLO" "${RUN_ARGS[@]}"
fi
