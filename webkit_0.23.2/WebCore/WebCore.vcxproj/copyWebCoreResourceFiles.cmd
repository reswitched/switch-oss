echo Copying Resource Files...
@mkdir 2>NUL "%CONFIGURATIONBUILDDIR%\bin%PlatformArchitecture%\WebKit.resources\en.lproj"
@xcopy /y /d /s /exclude:xcopy.excludes "%ProjectDir%..\English.lproj\Localizable.strings" "%CONFIGURATIONBUILDDIR%\bin%PlatformArchitecture%\WebKit.resources\en.lproj" >nul
@xcopy /y /d /s /exclude:xcopy.excludes "%ProjectDir%..\English.lproj\mediaControlsLocalizedStrings.js" "%CONFIGURATIONBUILDDIR%\bin%PlatformArchitecture%\WebKit.resources\en.lproj" >nul
@xcopy /y /d /s "%ProjectDir%..\Modules\mediacontrols\mediaControlsApple.css" "%CONFIGURATIONBUILDDIR%\bin%PlatformArchitecture%\WebKit.resources" >nul
@xcopy /y /d /s "%ProjectDir%..\Modules\mediacontrols\mediaControlsApple.js" "%CONFIGURATIONBUILDDIR%\bin%PlatformArchitecture%\WebKit.resources" >nul
