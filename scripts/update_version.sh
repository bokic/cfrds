#!/usr/bin/env bash
#
# Prebuild script to synchronize compile-time version macros in include/cfrds.h
# with $CFRDS_VERSION, script argument, or git tags.
#
# Priority:
#   1. Script argument:       ./scripts/update_version.sh 1.2.3
#   2. Environment variable:  export CFRDS_VERSION=1.2.3
#   3. Git tag:               git describe --tags --abbrev=0
#
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HEADER_FILE="${ROOT_DIR}/include/cfrds.h"

# 1. Determine version: argument > CFRDS_VERSION env > git tag
VERSION=""
if [ -n "${1:-}" ]; then
    VERSION="$1"
elif [ -n "${CFRDS_VERSION:-}" ]; then
    VERSION="${CFRDS_VERSION}"
elif [ -d "${ROOT_DIR}/.git" ] || command -v git &>/dev/null; then
    VERSION="$(git -C "${ROOT_DIR}" describe --tags --abbrev=0 2>/dev/null || true)"
fi

if [ -z "${VERSION}" ]; then
    echo "warning: no version argument, CFRDS_VERSION env var, or git tag found; leaving ${HEADER_FILE} unchanged" >&2
    exit 0
fi

# Strip leading 'v' if present (e.g. v1.2.3 -> 1.2.3)
VERSION="${VERSION#v}"

# Validate X.Y.Z (with optional pre-release tag, e.g. 1.2.3-rc1)
if ! printf '%s' "${VERSION}" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+'; then
    echo "error: version '${VERSION}' is not a valid semver (must start with X.Y.Z)" >&2
    exit 1
fi

IFS='.' read -r MAJOR MINOR REST <<< "${VERSION}"
PATCH="${REST%%-*}" # Strip pre-release suffix for integer calculation

MAJOR="${MAJOR:-0}"
MINOR="${MINOR:-0}"
PATCH="${PATCH:-0}"

VERSION_INT=$(( MAJOR * 10000 + MINOR * 100 + PATCH ))
VERSION_STR="${MAJOR}.${MINOR}.${PATCH}"

# 2. Check if update is needed (idempotent)
CURRENT_STR="$(grep -E '^#define CFRDS_VERSION ' "${HEADER_FILE}" | awk -F'"' '{print $2}' || true)"

if [ "${CURRENT_STR}" = "${VERSION_STR}" ]; then
    echo "include/cfrds.h version is already up to date (${VERSION_STR})."
    exit 0
fi

# 3. Update include/cfrds.h
sed -i -E \
  -e "s/^#define CFRDS_VERSION_MAJOR .*/#define CFRDS_VERSION_MAJOR ${MAJOR}/" \
  -e "s/^#define CFRDS_VERSION_MINOR .*/#define CFRDS_VERSION_MINOR ${MINOR}/" \
  -e "s/^#define CFRDS_VERSION_PATCH .*/#define CFRDS_VERSION_PATCH ${PATCH}/" \
  -e "s/^#define CFRDS_VERSION_INT .*/#define CFRDS_VERSION_INT ${VERSION_INT}/" \
  -e "s/^#define CFRDS_VERSION .*/#define CFRDS_VERSION \"${VERSION_STR}\"/" \
  "${HEADER_FILE}"

echo "Updated ${HEADER_FILE} to version ${VERSION_STR} (INT: ${VERSION_INT})"
