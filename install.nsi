;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"

;--------------------------------
;General

  ;Name and file
  Name "SIS"
  OutFile "build\sis-setup.exe"
  Unicode True

  ;Default installation folder
  InstallDir "$LOCALAPPDATA\SIS"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\SIS" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel user

;--------------------------------
;Variables

  Var StartMenuFolder

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "LICENSE"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY

  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\SIS"
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "SIS"
  ;!define MUI_STARTMENUPAGE_DEFAULTFOLDER "SIS"
  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "SIS" SecSIS

  SetOutPath "$INSTDIR"
  
  File "LICENSE"
  SetOutPath "$INSTDIR\bin"
  File "build\sis.exe"
  File "build\sisui.exe"
  CreateShortCut "$DESKTOP\SIS.lnk" "$INSTDIR\bin\sisui.exe" \
  "" "$INSTDIR\assets\sis.ico" 0 SW_SHOWNORMAL \
  "" "SIS"
  File "build\SDL2.dll"
  File "build\glew32.dll"
  File "build\libgcc_s_seh-1.dll"
  File "build\libstdc++-6.dll"
  File "build\libwinpthread-1.dll"
  SetOutPath "$INSTDIR\assets"
  File "assets\blender_icons16.png"
  File "assets\DejaVuSans.ttf"
  File "assets\sis.ico"
  SetOutPath "$INSTDIR\depthmaps"
  File "depthmaps\begger.png"
  File "depthmaps\billiards.png"
  File "depthmaps\bunny.png"
  File "depthmaps\circles.png"
  File "depthmaps\elephant1.png"
  File "depthmaps\elephant2.png"
  File "depthmaps\flowers.png"
  File "depthmaps\geometries.png"
  File "depthmaps\helmet.jpg"
  File "depthmaps\oval.png"
  File "depthmaps\roses.png"
  File "depthmaps\snowman.jpg"
  SetOutPath "$INSTDIR\textures"
  File "textures\beans.png"
  File "textures\clover.png"
  File "textures\cork.png"
  File "textures\fire.jpg"
  File "textures\texpic01.png"
  File "textures\texpic02.png"
  
  ;Store installation folder
  WriteRegStr HKCU "Software\SIS" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  ;Create shortcuts
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  SetOutPath "$INSTDIR"
  CreateShortcut "$SMPROGRAMS\$StartMenuFolder\SIS.lnk" "$INSTDIR\bin\sisui.exe" \
  "" "$INSTDIR\assets\sis.ico" 0 SW_SHOWNORMAL \
  "" "SIS"
  CreateShortcut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecSIS ${LANG_ENGLISH} "SIS stereogram generator"

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecSIS} $(DESC_SecSIS)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  Delete "$INSTDIR\LICENSE"
  Delete "$INSTDIR\bin\sis.exe"
  Delete "$INSTDIR\bin\sisui.exe"
  Delete "$INSTDIR\bin\libgcc_s_seh-1.dll"
  Delete "$INSTDIR\bin\libwinpthread-1.dll"
  Delete "$INSTDIR\assets\blender_icons16.png"
  Delete "$INSTDIR\assets\DejaVuSans.ttf"
  Delete "$INSTDIR\assets\sis.ico"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$INSTDIR\depthmaps\begger.png"
  Delete "$INSTDIR\depthmaps\billiards.png"
  Delete "$INSTDIR\depthmaps\bunny.png"
  Delete "$INSTDIR\depthmaps\circles.png"
  Delete "$INSTDIR\depthmaps\elephant1.png"
  Delete "$INSTDIR\depthmaps\elephant2.png"
  Delete "$INSTDIR\depthmaps\flowers.png"
  Delete "$INSTDIR\depthmaps\geometries.png"
  Delete "$INSTDIR\depthmaps\helmet.jpg"
  Delete "$INSTDIR\depthmaps\oval.png"
  Delete "$INSTDIR\depthmaps\roses.png"
  Delete "$INSTDIR\depthmaps\snowman.jpg"
  Delete "$INSTDIR\textures\beans.png"
  Delete "$INSTDIR\textures\clover.png"
  Delete "$INSTDIR\textures\cork.png"
  Delete "$INSTDIR\textures\fire.jpg"
  Delete "$INSTDIR\textures\texpic01.png"
  Delete "$INSTDIR\textures\texpic02.png"
  Delete "$DESKTOP\SIS.lnk"
  RMDir "$INSTDIR\bin"
  RMDir "$INSTDIR\assets"
  RMDir "$INSTDIR\depthmaps"
  RMDir "$INSTDIR\textures"
  RMDir "$INSTDIR"

  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  Delete "$SMPROGRAMS\$StartMenuFolder\SIS.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"

  DeleteRegKey /ifempty HKCU "Software\SIS"

SectionEnd