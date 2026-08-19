#!/usr/bin/env bash
#
# Release build helper. Syncs the Cargo.toml package version from the most
# recent git tag (mirroring the C build, which derives its version from
# `git describe --tags --abbrev=0`), then runs a release build.
#
# The version only needs to be X.Y.Z; tags that don't match are rejected.

set -euo pipefail

cd "$(dirname "$0")"

VERSION="$(git describe --tags --abbrev=0 2>/dev/null || true)"

if [ -n "$VERSION" ]; then
    if ! printf '%s' "$VERSION" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
        echo "error: git tag '$VERSION' is not a valid X.Y.Z version" >&2
        exit 1
    fi
    # Idempotent: only touch Cargo.toml when the version actually differs.
    if ! grep -qE "^version = \"$VERSION\"$" Cargo.toml; then
        sed -i -E "s/^version = \".*\"/version = \"$VERSION\"/" Cargo.toml
        echo "Cargo.toml: version updated to $VERSION"
    fi
else
    echo "warning: no git tag found; keeping Cargo.toml version unchanged" >&2
fi

exec cargo build --release "$@"
