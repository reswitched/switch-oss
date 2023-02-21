# Copyright (C) 2006, 2007, 2008, 2009, 2011, 2013, 2015 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
# 3.  Neither the name of Apple Inc. ("Apple") nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

VPATH = \
    $(JavaScriptCore) \
    $(JavaScriptCore)/parser \
    $(JavaScriptCore)/runtime \
	$(JavaScriptCore)/interpreter \
	$(JavaScriptCore)/jit \
	$(JavaScriptCore)/builtins \
#

.PHONY : all
all : \
    ArrayConstructor.lut.h \
    ArrayIteratorPrototype.lut.h \
    BooleanPrototype.lut.h \
    DateConstructor.lut.h \
    DatePrototype.lut.h \
    ErrorPrototype.lut.h \
    JSDataViewPrototype.lut.h \
    JSONObject.lut.h \
    JSGlobalObject.lut.h \
    JSPromisePrototype.lut.h \
    JSPromiseConstructor.lut.h \
    KeywordLookup.h \
    Lexer.lut.h \
    NumberConstructor.lut.h \
    NumberPrototype.lut.h \
    ObjectConstructor.lut.h \
    RegExpConstructor.lut.h \
    RegExpPrototype.lut.h \
    RegExpJitTables.h \
    StringConstructor.lut.h \
    StringIteratorPrototype.lut.h \
    SymbolConstructor.lut.h \
    SymbolPrototype.lut.h \
    udis86_itab.h \
    Bytecodes.h \
    InitBytecodes.asm \
    JSCBuiltins \
#

# builtin functions
.PHONY: JSCBuiltins

PYTHON = python
PERL = perl

DELETE = rm -f
# --------

JSCBuiltins: $(JavaScriptCore)/generate-js-builtins JSCBuiltins.h JSCBuiltins.cpp
JSCBuiltins.h: $(JavaScriptCore)/generate-js-builtins $(JavaScriptCore)/builtins
	$(PYTHON) $(JavaScriptCore)/generate-js-builtins --input-directory $(JavaScriptCore)/builtins --output $@
																				 
JSCBuiltins.cpp: JSCBuiltins.h

# lookup tables for classes

%.lut.h: create_hash_table %.cpp
	$(PERL) $^ -i > $@
Lexer.lut.h: create_hash_table Keywords.table
	$(PERL) $^ > $@

# character tables for Yarr

RegExpJitTables.h: create_regex_tables
	$(PYTHON) $^ > $@

KeywordLookup.h: KeywordLookupGenerator.py Keywords.table
	$(PYTHON) $^ > $@

# udis86 instruction tables

udis86_itab.h: $(JavaScriptCore)/disassembler/udis86/itab.py $(JavaScriptCore)/disassembler/udis86/optable.xml
	$(PYTHON) $(JavaScriptCore)/disassembler/udis86/itab.py $(JavaScriptCore)/disassembler/udis86/optable.xml

# Bytecode files

Bytecodes.h: $(JavaScriptCore)/generate-bytecode-files $(JavaScriptCore)/bytecode/BytecodeList.json
	$(PYTHON) $(JavaScriptCore)/generate-bytecode-files --bytecodes_h Bytecodes.h $(JavaScriptCore)/bytecode/BytecodeList.json

InitBytecodes.asm: $(JavaScriptCore)/generate-bytecode-files $(JavaScriptCore)/bytecode/BytecodeList.json
	$(PYTHON) $(JavaScriptCore)/generate-bytecode-files --init_bytecodes_asm InitBytecodes.asm $(JavaScriptCore)/bytecode/BytecodeList.json

# Inspector interfaces

INSPECTOR_DOMAINS = \
    $(JavaScriptCore)/inspector/protocol/ApplicationCache.json \
    $(JavaScriptCore)/inspector/protocol/CSS.json \
    $(JavaScriptCore)/inspector/protocol/Console.json \
    $(JavaScriptCore)/inspector/protocol/DOM.json \
    $(JavaScriptCore)/inspector/protocol/DOMDebugger.json \
    $(JavaScriptCore)/inspector/protocol/DOMStorage.json \
    $(JavaScriptCore)/inspector/protocol/Database.json \
    $(JavaScriptCore)/inspector/protocol/Debugger.json \
    $(JavaScriptCore)/inspector/protocol/GenericTypes.json \
    $(JavaScriptCore)/inspector/protocol/Inspector.json \
    $(JavaScriptCore)/inspector/protocol/LayerTree.json \
    $(JavaScriptCore)/inspector/protocol/Network.json \
    $(JavaScriptCore)/inspector/protocol/OverlayTypes.json \
    $(JavaScriptCore)/inspector/protocol/Page.json \
    $(JavaScriptCore)/inspector/protocol/Runtime.json \
    $(JavaScriptCore)/inspector/protocol/Timeline.json \
    $(JavaScriptCore)/inspector/protocol/Worker.json \
#

ifeq ($(findstring ENABLE_INDEXED_DATABASE,$(FEATURE_DEFINES)), ENABLE_INDEXED_DATABASE)
    INSPECTOR_DOMAINS := $(INSPECTOR_DOMAINS) $(JavaScriptCore)/inspector/protocol/IndexedDB.json
endif

ifeq ($(findstring ENABLE_WEB_REPLAY,$(FEATURE_DEFINES)), ENABLE_WEB_REPLAY)
    INSPECTOR_DOMAINS := $(INSPECTOR_DOMAINS) $(JavaScriptCore)/inspector/protocol/Replay.json
endif

INSPECTOR_GENERATOR_SCRIPTS = \
	$(JavaScriptCore)/inspector/scripts/codegen/__init__.py \
	$(JavaScriptCore)/inspector/scripts/codegen/cpp_generator_templates.py \
	$(JavaScriptCore)/inspector/scripts/codegen/cpp_generator.py \
	$(JavaScriptCore)/inspector/scripts/codegen/generate_cpp_backend_dispatcher_header.py \
	$(JavaScriptCore)/inspector/scripts/codegen/generate_cpp_backend_dispatcher_implementation.py \
	$(JavaScriptCore)/inspector/scripts/codegen/generate_cpp_frontend_dispatcher_header.py \
	$(JavaScriptCore)/inspector/scripts/codegen/generate_cpp_frontend_dispatcher_implementation.py \
	$(JavaScriptCore)/inspector/scripts/codegen/generate_cpp_protocol_types_header.py \
	$(JavaScriptCore)/inspector/scripts/codegen/generate_cpp_protocol_types_implementation.py \
	$(JavaScriptCore)/inspector/scripts/codegen/generate_js_backend_commands.py \
	$(JavaScriptCore)/inspector/scripts/codegen/generator_templates.py \
	$(JavaScriptCore)/inspector/scripts/codegen/generator.py \
	$(JavaScriptCore)/inspector/scripts/codegen/models.py \
	$(JavaScriptCore)/inspector/scripts/generate-combined-inspector-json.py \
	$(JavaScriptCore)/inspector/scripts/generate-inspector-protocol-bindings.py \
#

all : \
    CombinedDomains.json \
    InspectorFrontendDispatchers.h \
    InjectedScriptSource.h \
#

# The combined JSON file depends on the actual set of domains and their file contents, so that
# adding, modifying, or removing domains will trigger regeneration of inspector files.

.PHONY: force
EnabledInspectorDomains : $(JavaScriptCore)/UpdateContents.py force
	$(PYTHON) $(JavaScriptCore)/UpdateContents.py '$(INSPECTOR_DOMAINS)' $@

CombinedDomains.json : inspector/scripts/generate-combined-inspector-json.py $(INSPECTOR_DOMAINS) EnabledInspectorDomains
	$(PYTHON) $(JavaScriptCore)/inspector/scripts/generate-combined-inspector-json.py $(INSPECTOR_DOMAINS) > ./CombinedDomains.json

# Inspector Backend Dispatchers, Frontend Dispatchers, Type Builders
InspectorFrontendDispatchers.h : CombinedDomains.json $(INSPECTOR_GENERATOR_SCRIPTS)
	$(PYTHON) $(JavaScriptCore)/inspector/scripts/generate-inspector-protocol-bindings.py --framework JavaScriptCore --outputDir . ./CombinedDomains.json

InjectedScriptSource.h : inspector/InjectedScriptSource.js $(JavaScriptCore)/inspector/scripts/jsmin.py $(JavaScriptCore)/inspector/scripts/xxd.pl
	echo "//# sourceURL=__WebInspectorInjectedScript__" > ./InjectedScriptSource.min.js
	$(PYTHON) $(JavaScriptCore)/inspector/scripts/jsmin.py < $(JavaScriptCore)/inspector/InjectedScriptSource.js >> ./InjectedScriptSource.min.js
	$(PERL) $(JavaScriptCore)/inspector/scripts/xxd.pl InjectedScriptSource_js ./InjectedScriptSource.min.js InjectedScriptSource.h
	$(DELETE) InjectedScriptSource.min.js

# Web Replay inputs generator

INPUT_GENERATOR_SCRIPTS = \
    $(JavaScriptCore)/replay/scripts/CodeGeneratorReplayInputs.py \
    $(JavaScriptCore)/replay/scripts/CodeGeneratorReplayInputsTemplates.py \
#

INPUT_GENERATOR_SPECIFICATIONS = \
    $(JavaScriptCore)/replay/JSInputs.json \
#

all : JSReplayInputs.h

JSReplayInputs.h : $(INPUT_GENERATOR_SPECIFICATIONS) $(INPUT_GENERATOR_SCRIPTS)
	$(PYTHON) $(JavaScriptCore)/replay/scripts/CodeGeneratorReplayInputs.py --outputDir . --framework JavaScriptCore $(INPUT_GENERATOR_SPECIFICATIONS)
