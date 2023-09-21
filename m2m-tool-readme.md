# Telit m2m shell tool

`m2m.exe` (or `m2m` for Linux machines) tool allows to manage a Telit device programmatically, mostly through wrapped AT commands.
A non comprehensive list of functionalities include:

- list files in module filesystem
- set the APN for a context ID
- remove files
- copy files
- install application binaries
- read files content
- dump module file on PC
- manage GPIO status
- manage AppZone engine
- Shutdown/reboot the device
- Send AT commands


## Additional info

By default, the tool configuration is stored in
`<user>/.config/m2m/CONFIG.ini/`



An example configuration could be:

```ini 
[Device]
#port=/dev/ttyS42
port=COM33
[I2C]
sda=6
scl=5
```


**Device** section allows to set the COM port
**I2C** section allows to configure sda and scl pins to be used in I2C communication


For further details, execute the `m2m` tool with no parameters


## Common operations

Below some examples of common operations. The examples explicitly define the device `-D COM33`, but if a CONFIG.ini is stored, it can be omitted.

### Install AppZone app

```
m2m.exe -D COM33 install ./myapp.bin
```

### Upload a generic file from PC to module

```
m2m.exe -D COM33 copy ./file.txt /mod/file.txt
```

### Download a generic file the module to the PC

```
m2m.exe -D COM33 dump /mod/file.txt ./file.txt 
```


Note: file operations will report the progress status if `-p` flag is used
