all: WTFHeaderDetection.h
    @type NUL > "%ConfigurationBuildDir%\buildfailed"
    @perl build-generated-files.pl "%ConfigurationBuildDir%" "$(WEBKIT_LIBRARIES)" "$(DEBUGSUFFIX)" "%PlatformArchitecture%"
!IF "$(OFFICIAL_BUILD)"!="1"
    -@python2.7 work-around-vs-dependency-tracking-bugs.py
!ENDIF
    @copy-files.cmd

    -@del "%ConfigurationBuildDir%\buildfailed"

clean:
    -@del "%ConfigurationBuildDir%\buildfailed"
    -@del "%ConfigurationBuildDir%\include\private\wtf\WTFHeaderDetection.h"
    @copy-files.cmd clean

# Header detection
WTFHeaderDetection.h: WTFGenerated.make
    -@mkdir "%ConfigurationBuildDir%\include\private\wtf"
    -@<<testOSXLevel.cmd
@echo off
IF EXIST "%ConfigurationBuildDir%\include\private\wtf\$@" GOTO DONE
echo /* No Legible Output Support Found */  > "%ConfigurationBuildDir%\include\private\wtf\$@"
IF EXIST "$(WEBKIT_LIBRARIES)/include/AVFoundationCF/AVCFPlayerItemLegibleOutput.h" (echo #define __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ 1090 > "%ConfigurationBuildDir%\include\private\wtf\$@")
:DONE
<<

