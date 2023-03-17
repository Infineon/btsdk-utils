DfuMeta
=======

Overview
========
DfuMeta is a tool to generate metadata and manifest file for device firmware update. It generates firmware ID based on user input, then compute the hash of firmware ID and firmware image, sign it with a private key (using Elliptic Curve Digital Signature Algorithm), and save the signed firmare image, then create a manifest JSON file that contain the firmware image file and metadata file names and firmware ID.

Command Usage
=============
DfuMeta cid=0x0131 pid=0x3016 vid=0x0002 version=1.1 -k key.pri.bin -i fw.ota.bin -m metadata -o manifest.json

Parameter        |  Description
----------------------------------------------------------------------------------------------------------
cid=0x0131       |  Company ID
pid=0x3016       |  Product ID
vid=0x0002       |  Hardware version ID
version=1.1      |  Software version number defined in makefile as APP_VERSION_MAJOR and APP_VERSION_MINOR
-k key.pri.bin   |  Private key file
-i fw.ota.bin    |  Input firmware image file
-m metadata      |  Output metadata file
-o manifest.json |  Output manifest JSON file

Build Instructions
==================
- Build DfuMeta for Linux
  In Linux command line, change to DfuMeta/bin/Linux64 folder and run following script:
    ./build_linux.sh

- Build DfuMeta for Mac OS
  In Mac OS terminal, change to DfuMeta/bin/OSX folder and run following script:
    ./build_macos.sh

- Build DfuMeta.exe for Windows
  In Windows command line, change to DfuMeta/bin/Windows folder and run following batch file:
    build_win.bat
