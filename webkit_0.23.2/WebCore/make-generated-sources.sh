#!/bin/sh
export SOURCE_ROOT=$PWD
export SRCROOT=$PWD
export WebCore=$PWD
export InspectorScripts=$PWD/../JavaScriptCore/inspector/scripts
export WebReplayScripts=$PWD/../JavaScriptCore/replay/scripts

mkdir -p DerivedSources/WebCore &&
make -C DerivedSources/WebCore -f ../../DerivedSources.make $@
