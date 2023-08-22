/*
 * Copyright (C) 2014-2015 Apple Inc. All rights reserved.
 * Copyright (C) 2013 University of Washington. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.ProbeDetailsSidebarPanel = class ProbeDetailsSidebarPanel extends WebInspector.DetailsSidebarPanel
{
    constructor()
    {
        super("probe", WebInspector.UIString("Probes"), WebInspector.UIString("Probes"));

        WebInspector.probeManager.addEventListener(WebInspector.ProbeManager.Event.ProbeSetAdded, this._probeSetAdded, this);
        WebInspector.probeManager.addEventListener(WebInspector.ProbeManager.Event.ProbeSetRemoved, this._probeSetRemoved, this);

        this._probeSetSections = new Map;
        this._inspectedProbeSets = [];

        // Initialize sidebar sections for probe sets that already exist.
        for (var probeSet of WebInspector.probeManager.probeSets)
            this._probeSetAdded(probeSet);
    }

    // Public

    get inspectedProbeSets()
    {
        return this._inspectedProbeSets.slice();
    }

    set inspectedProbeSets(newProbeSets)
    {
        for (var probeSet of this._inspectedProbeSets) {
            var removedSection = this._probeSetSections.get(probeSet);
            this.contentElement.removeChild(removedSection.element);
        }

        this._inspectedProbeSets = newProbeSets;

        for (var probeSet of newProbeSets) {
            var shownSection = this._probeSetSections.get(probeSet);
            this.contentElement.appendChild(shownSection.element);
        }
    }

    inspect(objects)
    {
        if (!(objects instanceof Array))
            objects = [objects];

        var inspectedProbeSets = objects.filter(function(object) {
            return object instanceof WebInspector.ProbeSet;
        });

        inspectedProbeSets.sort(function sortBySourceLocation(aProbeSet, bProbeSet) {
            var aLocation = aProbeSet.breakpoint.sourceCodeLocation;
            var bLocation = bProbeSet.breakpoint.sourceCodeLocation;
            var comparisonResult = aLocation.sourceCode.displayName.localeCompare(bLocation.sourceCode.displayName);
            if (comparisonResult !== 0)
                return comparisonResult;

            comparisonResult = aLocation.displayLineNumber - bLocation.displayLineNumber;
            if (comparisonResult !== 0)
                return comparisonResult;

            return aLocation.displayColumnNumber - bLocation.displayColumnNumber;
        });

        this.inspectedProbeSets = inspectedProbeSets;

        return !!this._inspectedProbeSets.length;
    }

    // Private

    _probeSetAdded(probeSetOrEvent)
    {
        var probeSet;
        if (probeSetOrEvent instanceof WebInspector.ProbeSet)
            probeSet = probeSetOrEvent;
        else
            probeSet = probeSetOrEvent.data.probeSet;
        console.assert(!this._probeSetSections.has(probeSet), "New probe group ", probeSet, " already has its own sidebar.");

        var newSection = new WebInspector.ProbeSetDetailsSection(probeSet);
        this._probeSetSections.set(probeSet, newSection);
    }


    _probeSetRemoved(event)
    {
        var probeSet = event.data.probeSet;
        console.assert(this._probeSetSections.has(probeSet), "Removed probe group ", probeSet, " doesn't have a sidebar.");

        // First remove probe set from inspected list, then from mapping.
        var inspectedProbeSets = this.inspectedProbeSets;
        var index = inspectedProbeSets.indexOf(probeSet);
        if (index !== -1) {
            inspectedProbeSets.splice(index, 1);
            this.inspectedProbeSets = inspectedProbeSets;
        }
        var removedSection = this._probeSetSections.get(probeSet);
        this._probeSetSections.delete(probeSet);
        removedSection.closed();
    }
};
