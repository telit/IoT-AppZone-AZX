# Script that helps with importing the libraries needed into a project simply
# define a $script:LIBS variable like below and source this script in a powershell:
#   $script:LIBS="core/azx_ati core/azx_timer core/azx_utils core/azx_tasks"
#   $LIBS_FOLDER_NAME = "azx"
#   $SRC  = <path-to-libs>/${LIBS_FOLDER_NAME}
#   DEST="<path-to-project>"
#   . "${SRC}/import_libs.ps1"
#   copy_telit_libs "${SRC}" "${DEST}"
# 
# Note: Please make sure you don't place anything by yourself in the libs folder
# of your project as this will be removed
#
#
# The script will copy all the needed libraries and also update Makefile.in in
# the root of the project if needed
#


if ( $LIBS_FOLDER_NAME -eq "" -or $LIBS_FOLDER_NAME -eq $null)
{
  $LIBS_FOLDER_NAME="azx"
}

$script:INSTALLED_LIBS = @()

function _add_if_missing
{
  $local:DEST=$args[0]
  $local:TEXT=$args[1]
  if ( $( Select-String -Quiet -SimpleMatch "$local:TEXT" -Path "$local:DEST" ) -eq "" )
  {
    #Write-Host "not found, add"
    Add-Content -Path "$local:DEST" -Value "`n$local:TEXT" -Encoding utf8 -NoNewline
  }
  
}

function get_dependencies
{
  $local:INFO_FILE=$args[0]

  Select-String "@dependencies (.*)$" -Path "$local:INFO_FILE" | ForEach-Object{$_.Matches.Groups[1].Value}
}


function is_installed
{
  $local:NAME=$args[0]
  ForEach ($N in $script:INSTALLED_LIBS)
  {
    if ($N -eq $local:NAME)
    {
        return $true
    }
  }
  return $false
}


function install_lib
{
  $local:LIBNAME=$args[0]
  $local:NAME="$local:LIBNAME"
  $local:DEST_LIBS_NAME=$args[1]
  
  if (is_installed $local:NAME)
  {
    #Write-Host "Already installed" -ForegroundColor DarkYellow
    return 
  }

  Write-Host "Installing <$local:NAME>" -ForegroundColor Green
  $local:DEPENDS=""
  if ( "$local:NAME".StartsWith("core/"))
  {
    $local:NAME="$local:NAME" -replace "core/", ""
    if( Test-Path -Path $SRC/core/hdr/${local:NAME}.h -PathType Leaf )
    {
        Copy-Item -Path "${SRC}/core/hdr/${local:NAME}.h" -Destination "${DEST}/${local:DEST_LIBS_NAME}/hdr/"
        #add COPYING attribute file for a specific core lib, if existing
        $local:ATTRIB_FILE="COPYING-" + $( ${local:NAME} -replace "azx_", "" )

        if (Test-Path -Path "${SRC}/core/hdr/${local:ATTRIB_FILE}" -PathType Leaf )
        {
             Copy-Item -Path "${SRC}/core/hdr/${local:ATTRIB_FILE}" -Destination "${DEST}/${local:DEST_LIBS_NAME}/hdr/"
        }

        if (Test-Path -Path "${SRC}/core/src/${local:NAME}.c" -PathType Leaf )
        {
             Copy-Item -Path "${SRC}/core/src/${local:NAME}.c" -Destination "${DEST}/${local:DEST_LIBS_NAME}/src/"
        }

        _add_if_missing "${DEST}/Makefile.in" "CPPFLAGS += -I ${local:DEST_LIBS_NAME}/hdr"
        _add_if_missing "${DEST}/Makefile.in" "OBJS += `$`(patsubst %.c,%.o,`$`(wildcard ${local:DEST_LIBS_NAME}/src/*.c`)`)"

        $local:DEPENDS=$(get_dependencies "${SRC}/core/hdr/${local:NAME}.h").split(" ",[System.StringSplitOptions]::RemoveEmptyEntries)
    }

  }

  if ( "$local:NAME".StartsWith("libraries/"))
  {
    $local:NAME = "$local:NAME" -replace "libraries/", ""
    if( Test-Path -Path ${SRC}\libraries\${local:NAME} -PathType Container )
    {
        
        Copy-Item -Path "${SRC}\libraries\${local:NAME}" -Recurse -Destination "${DEST}/${local:DEST_LIBS_NAME}/"
        _add_if_missing "${DEST}/Makefile.in" "CPPFLAGS += -I ${local:DEST_LIBS_NAME}/${local:NAME}"
        _add_if_missing "${DEST}/Makefile.in" "OBJS += `$`(patsubst %.c,%.o,`$`(wildcard ${local:DEST_LIBS_NAME}/${local:NAME}/*.c`)`)"

        
        if (Test-Path -Path "${SRC}\libraries\${local:NAME}\Makefile.in.in" )
        {
            $local:MAKEFILE_TMP = $( $(Get-Content -path ${SRC}/libraries/${local:NAME}/Makefile.in.in) -replace "BASE_LIB_PATH", "${local:DEST_LIBS_NAME}/${local:NAME}" )
            _add_if_missing "${DEST}/Makefile.in" "${local:MAKEFILE_TMP}"
            
            Remove-Item -Path "${DEST}/${local:DEST_LIBS_NAME}/${local:NAME}/Makefile.in.in"
        }
        
        $local:DEPENDS=$(get_dependencies "${SRC}/libraries/${local:NAME}/readme").split(" ",[System.StringSplitOptions]::RemoveEmptyEntries)
    }
  }
  
  $script:INSTALLED_LIBS = $script:INSTALLED_LIBS + $local:LIBNAME
  ForEach ($D in $local:DEPENDS)
  {
    install_lib ${D} ${local:DEST_LIBS_NAME}
  }
  
}


function copy_telit_libs {
  $local:SRC=$args[0]
  $local:DEST=$args[1]
  
  $local:DEST_LIBS_NAME="azx"
  
  if ($args.count -gt 2)
  {
    $local:DEST_LIBS_NAME=$args[2]
  }
  
  Write-Host "Copying libraries from `"${local:SRC}`" to `"${local:DEST}/${local:DEST_LIBS_NAME}`""
  
  Remove-Item -LiteralPath "${local:DEST}/${local:DEST_LIBS_NAME}" -Force -Recurse -ErrorAction SilentlyContinue

  if ( $script:LIBS -ne "" -and $script:LIBS -ne $null)
  {
    New-Item -Path "${local:DEST}/${local:DEST_LIBS_NAME}/" -Name "src" -ItemType "directory" | Out-Null
    New-Item -Path "${local:DEST}/${local:DEST_LIBS_NAME}/" -Name "hdr" -ItemType "directory" | Out-Null

    forEach ($LIB in $script:LIBS.split(" ",[System.StringSplitOptions]::RemoveEmptyEntries))
    {
       install_lib ${LIB} ${local:DEST_LIBS_NAME}
    }
  }
  else
  {
    Write-Host "No libraries to be installed"
  }
}