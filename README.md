
IoT-AppZone-C AZX Libs

## Abstract

This repository contains Telit IoT AppZone C utility libraries, built on top of the Telit M2MB API, which provide additional functionalities and 
simplified access to on-board ones, allowing for easy creation of projects based on the Telit's modems.

---

# Libraries included

Here is a list of included libraries:

Library | Version | Description
-------------------- | ------- | --------------------------------------------------
`core/azx_adc` | `v1.0.2` | Read from and write to a peripheral via ADC
`core/azx_apn` | `v1.0.2` | Automatically setting APN based on the ICCID of the SIM
`core/azx_ati` | `v1.0.2` | Sending AT commands and handling URCs
`core/azx_base64` | `v1.1.0` | Base64 utilities
`core/azx_buffer` | `v1.0.1` | Buffers data that can be retrieved later
`core/azx_connectivity` | `v1.0.2` | Establish network and data connection synchronously and provide info
`core/azx_gpio` | `v1.0.2` | Interact with the modem's GPIO pins
`core/azx_i2c` | `v1.0.1` | Communicate with peripherals over the I2C bus
`core/azx_log` | `v1.0.8` | Logging utilities to print on available output channels
`core/azx_spi` | `v1.0.1` | Communicate with peripherals connected via the SPI bus
`core/azx_string` | `v1.0.2` | String manipulation library
`core/azx_string_utils` | `v1.0.1` | String related utilities
`core/azx_tasks` | `v1.0.4` | Tasks related utilities
`core/azx_timer` | `v1.0.2` | A better way to use timers
`core/azx_uart` | `v1.0.1` | Communicate with devices via UART
`core/azx_utils` | `v1.0.2` | Various helpful utilities
`core/azx_watchdog` | `v1.0.1` | Software watchdog to detects stalling tasks
`libraries/cjson` | `v1.0.1` | Porting of cJSON library
`libraries/easy_at` | `v1.0.1` | Utility code to simplify at parser usage (custom at commands)
`libraries/eeprom_24XX256` | `v1.0.1` | Library to provide 24XX256 EEPROM communication
`libraries/ftp` | `v1.0.0` | ftp client porting in azx style
`libraries/gnu` | `v0.0.2` | gnu abstraction layer utility in azx style
`libraries/https` | `v1.0.1` | Library to provide HTTPS client functionalities
`libraries/lfs2_utils` | `v1.0.1` | Utility code to use the implementation of LFS2 wih Ram Disk and SPI Flash memories
`libraries/pdu_codec` | `v1.0.0` | Utility code to simplify parse/encode binary PDU to be used with `m2mb_sms_*` APIs
`libraries/spi_flash` | `v1.0.1` | Driver code to interface JSC SPI data flash memories
`libraries/zlib` | `v0.0.2` | zlib abstraction layer utility in azx style

## Licensing

If not explicitly stated in each subfolder LICENSE / Attribution files, the source code provided in this repository is released according to [LICENSE.txt](LICENSE.txt)

---

# Using the AZX libs

To use any of the libraries, you can just use the `import_libs.script` which contains a function
that deals with the copying of the libraries and updating of the _Makefile.in_ file.

If you wish to avoid using a script to manage this process automatically, check out the [Manually including
Libraries](#manually-including-libraries) section. 

In order to use `import_libs.script` you need to do the following:

 - set the LIBS variable to be a list of libraries you want to use (like `LIBS="azx_ati azx_timer
   azx_utils azx_tasks"`)
 - source `import_libs.script`
 - call `copy_telit_libs` passing as arguments the location where the script is installed and the
   destination folder (where you project is)

The function will copy all the specified libraries in the location and update the Makefile.in with
correct include paths and new source files. If any libraries have other dependencies, those
dependencies will be handled automatically by the script.

## Example Script

_Please note that you might need to adjust the the variables in the script to reflect your project's directory or
setup._ 

### Linux / Windows with a bash shell


__get_libs.sh__

~~~bash
#!/bin/bash

LIBS="core/azx_timer core/azx_log core/azx_tasks core/azx_utils core/azx_ati core/azx_string core/azx_i2c core/azx_gpio"

LIBS_FOLDER_NAME="azx"
DEST=`readlink -f $0 | xargs dirname`
SRC="${AZX_LIBS}/${LIBS_FOLDER_NAME}"

source "${SRC}/import_libs.script"
copy_telit_libs "${SRC}" "${DEST}"
~~~

### Windows Powershell

__get_libs.ps1__

~~~powershell
$DEST = $PSScriptRoot
$LIBS_FOLDER_NAME = "azx"
$SRC  = Resolve-Path ${ROOT}\\${LIBS_FOLDER_NAME}

$script:LIBS = "core/azx_timer core/azx_log core/azx_tasks core/azx_utils core/azx_ati core/azx_string core/azx_i2c core/azx_gpio"

. "${SRC}/import_libs.ps1"

copy_telit_libs "${SRC}" "${DEST}"

~~~

## Manually including Libraries

In case you do not want to install extra scripts in your project you can still use the libraries but need to manually
perform the steps of the above scripts.  The process is simple but you need to make sure you include and compile all
necessary files.

Steps to manually add a library in your project:

1. Create a `libs` folder so you can place the files. This is optional since you can place them anywhere you want. The
important is to link them correctly.
1. Copy all header files (`.h`) and source files (`.c`) in your project, in a folder of your choice. Normally you would
place header files in a `hdr` directory and source files in `src`.
1. Look through those files you copied for any extra dependencies that are required and copy them. You can find a list
of all dependencies for each header file in the `@dependencies` tag.
1. Repeat for recursive dependecies.
1. Modify your Makefile and add the right flags to include the folders you copied the libraries in the build and include
path. For example:
    * to add new include paths: `CPPFLAGS += -I path/to/my_hdr`
    * to compile source files from another location into your project: `OBJS += $(patsubst %.c,%.o,$(wildcard my_src/*.c))` 

---

# Folder structure

- `doc`: AZX libraries' reference documentation in HTML format
- `README.*`: Read-me files for getting started with using the AZX libraries
- `azx`: The AZX libraries
    - `core`: The AZX basic utilities (log, tasks, etc)
      - `hdr`: Header files for the common AZX libraries
      - `src`: Implementation of the common AZX libraries
    - `libraries`: Libraries that provide added functionality
      - other folders: A folder for each library
    - `examples`: example code for Doxygen documentation.
    - `import_libs.script`: Script to facilitate importing the needed libraries in a project
    - `import_libs.ps1`: Script to facilitate importing the needed libraries in a project (with PowerShell)
- `examples`: Example codes showing the libraries usage

---

# API reference

To read the full API reference, please refer to doc/html/index.html from a local clone.

