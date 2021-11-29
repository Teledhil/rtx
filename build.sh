#!/bin/bash

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

usage() {

  echo "Usage: $0 [TARGET] [OPTION]"
  echo
  echo "  Targets:"
  echo
  echo "   --code           Compile the code. DEFAULT target."
  echo
  echo "  Options:"
  echo
  echo "   -h, --help       Print this message."
  echo "   -c, --clean      Clean."
  echo "   -g, --debug      Compile with debug stuff."
  echo
  echo " To disable notifications, run: touch $(git rev-parse --show-toplevel)/.nopopups"

}

check_bye() {
  return_code=$2
  if [ ! $return_code -eq 0 ]; then
    bye $1 $2 $3
  fi
}

bye() {
  local title="${PWD##*/}"
  local return_code=$2
  local subtitle="$1 (${return_code})"
  local text="$3"

  if [ ! -f $(git rev-parse --show-toplevel)/.nopopups ]; then
    if [ -x "$(command -v osascript)" ]; then
      osascript -e "display notification \"${text}\" with title \"${title}\" subtitle \"${subtitle}\" sound name \"Submarine\""
    fi
  fi

  echo "${subtitle} ${text}"

  exit ${return_code}
}

clean() {

  echo "Cleaning"

  if [ -d ${BUILD_DIR} ]; then
    echo "Removing ${BUILD_DIR}"
    rm -rf ${BUILD_DIR}
  fi

  return 0
}

setup_build_dir() {

  local result=0

  local CMAKE_FLAGS=()

  local CMAKE_GENERATOR="Ninja"
  CMAKE_FLAGS+=("-G")
  CMAKE_FLAGS+=("${CMAKE_GENERATOR}")

  if ${DEBUG_MODE}; then
    CMAKE_FLAGS+=("-D")
    CMAKE_FLAGS+=("CMAKE_BUILD_TYPE=Debug")
  else
    CMAKE_FLAGS+=("-D")
    CMAKE_FLAGS+=("CMAKE_BUILD_TYPE=Release")
  fi

  if [ ! -d ${BUILD_DIR} ]; then
    mkdir -p ${BUILD_DIR}
  fi

  cd ${BUILD_DIR}
  cmake "${CMAKE_FLAGS[@]}" ..
  result=$?

  cd - > /dev/null

  return $result
}

build_target() {

  local result=0

  if [ ! -d ${BUILD_DIR} ]; then
    setup_build_dir
    result=$?
    check_bye "Test" $result "Failed to setup build dir"
  fi

  cd ${BUILD_DIR}

  local NINJA_FLAGS=("--")
  if ${DEBUG_MODE}; then
    NINJA_FLAGS+=("-v")
  fi

  time cmake --build . --target $1 "${NINJA_FLAGS[@]}"
  result=$?

  cd - > /dev/null

  return $result
}

# Parse args
#
POSITIONAL=()

BUILD_DIR="$(git rev-parse --show-toplevel)/_build"
BUILD_PACKAGE=false
CLEAN=false
DEBUG_MODE=false

while [[ $# -gt 0 ]]
do
  key="$1"

  case $key in
    -c|--clean)
      CLEAN=true
      shift # past argument
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --code)
      shift # past argument
      ;;
    -g|--debug)
      DEBUG_MODE=true
      shift # past argument
      ;;
    *)    # unknown option
      POSITIONAL+=("$1") # save it in an array for later
      shift # past argument
      ;;
  esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

# Run
#
if $CLEAN; then
  clean
  bye "Clean" $?
fi

build_target rtx
bye "Compilation" $?
