% Example `azx_buffer utility`

# Build

## Linux / Windows with a bash shell

- Run `get_libs.sh` from the current directory

## Windows Powershell

- Run `get_libs.ps1` from the current directory

- Create an empty project in AZ IDE for the required family (e.g. ME910C1)
- Copy all folders and required files from the current directory to project's root folder:
  - Makefile.in
- Compile the project

# Deploy

To deploy,
- Remove all files from `/mod`: `m2m rm /mod/*`
- Install the test app: `m2m install *.bin`
- Run it: `m2m AT+M2M=4,10`
