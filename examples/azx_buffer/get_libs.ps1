
$DEST = $PSScriptRoot
$ROOT = Resolve-Path $DEST\\..\\..\
$LIBS_FOLDER_NAME = "azx"
$SRC  = Resolve-Path ${ROOT}\\${LIBS_FOLDER_NAME}


$script:LIBS = Get-Content -path $DEST\\azx.in


if ($args.count -gt 0)
{
    $SRC = $args[1]
}

if ($args.count -gt 1)
{
    $DEST = $args[2]
}

if ($args.count -gt 2)
{
    $LIBS_FOLDER_NAME = $args[3]
}


Write-Host "Sourcing ${SRC}/import_libs.ps1`n" 

. "${SRC}/import_libs.ps1"

Write-Host "Copy ${SRC} to ${DEST}`n"

copy_telit_libs "${SRC}" "${DEST}" "${LIBS_FOLDER_NAME}"


$script:LIBS = $null