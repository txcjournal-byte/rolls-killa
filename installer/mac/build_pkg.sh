#!/bin/bash
# Builds the macOS installer: VST3 + AU into /Library/Audio/Plug-Ins, Standalone into /Applications.
# Signs + notarizes when the signing identities / credentials are present in the environment
# (see setup_signing.sh); otherwise the pkg is unsigned.
# Usage: installer/mac/build_pkg.sh <version>
set -euo pipefail
source "$(dirname "$0")/notarize.sh"
trap 'echo "::error title=$(basename "$0")::line $LINENO failed: $BASH_COMMAND"' ERR

VERSION="${1:-0.1.0}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ART="$ROOT/build/HatGoblin_artefacts/Release"
STAGE="$ROOT/build/pkgroot"
OUT="$ROOT/build/installer"

rm -rf "$STAGE" && mkdir -p "$STAGE/Library/Audio/Plug-Ins/VST3" "$STAGE/Library/Audio/Plug-Ins/Components" "$STAGE/Applications" "$OUT"
cp -R "$ART/VST3/HAT GOBLIN.vst3" "$STAGE/Library/Audio/Plug-Ins/VST3/"
cp -R "$ART/AU/HAT GOBLIN.component" "$STAGE/Library/Audio/Plug-Ins/Components/"
cp -R "$ART/Standalone/HAT GOBLIN.app" "$STAGE/Applications/"

if [ -n "${MAC_APP_SIGN_ID:-}" ]; then
    find "$STAGE" -maxdepth 6 \( -name "*.vst3" -o -name "*.component" -o -name "*.app" \) -type d -prune -print0 \
        | while IFS= read -r -d '' b; do
            codesign --force --deep --options runtime --timestamp --sign "$MAC_APP_SIGN_ID" "$b"
            codesign --verify --strict --verbose=2 "$b"
        done
else
    # no Developer ID: ad-hoc sign so Apple Silicon loads the bundles
    find "$STAGE" -maxdepth 6 \( -name "*.vst3" -o -name "*.component" -o -name "*.app" \) -type d -prune -print0 \
        | while IFS= read -r -d '' b; do codesign --force --deep --sign - "$b"; done
fi

pkgbuild --root "$STAGE" --identifier com.trapwin.hatgoblin --version "$VERSION" --install-location / "$OUT/hatgoblin-component.pkg"

cat > "$OUT/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>HAT GOBLIN by TrapWin $VERSION</title>
    <license file="EULA.txt"/>
    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <choices-outline><line choice="default"/></choices-outline>
    <choice id="default"><pkg-ref id="com.trapwin.hatgoblin"/></choice>
    <pkg-ref id="com.trapwin.hatgoblin" version="$VERSION">hatgoblin-component.pkg</pkg-ref>
</installer-gui-script>
XML

mkdir -p "$OUT/resources" && cp "$ROOT/installer/EULA.txt" "$OUT/resources/"
PKG="$OUT/HAT-GOBLIN-$VERSION-macOS.pkg"
if [ -n "${MAC_INST_SIGN_ID:-}" ]; then
    productbuild --distribution "$OUT/distribution.xml" --resources "$OUT/resources" --package-path "$OUT" --sign "$MAC_INST_SIGN_ID" "$PKG"
else
    productbuild --distribution "$OUT/distribution.xml" --resources "$OUT/resources" --package-path "$OUT" "$PKG"
fi
rm -f "$OUT/hatgoblin-component.pkg"

if [ -n "${MAC_INST_SIGN_ID:-}" ] && { [ -n "${NOTARY_KEY_PATH:-}" ] || [ -n "${NOTARY_APPLE_ID:-}" ]; }; then
    notarize "$PKG"
    xcrun stapler staple "$PKG"
fi
echo "Created $PKG"
