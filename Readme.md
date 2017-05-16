# RN2483 Firmware Updater

Note: to be able to compile this application you need to add the right board file to your Arduino IDE.

Go to File, Preferences and set the following URL for the additional board files:

http://downloads.sodaq.net/package_sodaq_samd_index.json

## Hex Image Selection

In HexFileImage.h you can set which hex file image should be included in
the firmware, to be used for updating the module:
````C
//#define HEXFILE_RN2483_101
//#define HEXFILE_RN2483_103
//#define HEXFILE_RN2903AU_097rc7
//#define HEXFILE_RN2903_098
````

You have to uncomment one of these lines to select the required firmware.

## Other Firmware

You can include any other firmware hex file by opening the hex file in the
text editor of your choice (which should support columns) and adding a
double quote at the beginning of each line and a double quote and a comma
at the end of the line like this:
```
...
:10030000D7EF01F0FFFFFFFF5A82FACF2AF0FBCFB1
:100310002BF0D9CF2CF0DACF2DF0F3CF2EF0F4CF95
...
```
**becomes**
```
...
":10030000D7EF01F0FFFFFFFF5A82FACF2AF0FBCFB1",
":100310002BF0D9CF2CF0DACF2DF0F3CF2EF0F4CF95",
...
```

Then you can copy the result into a block like this:

```C
const char* const TheHexFileNameHere[] = {
  ...
  ":10030000D7EF01F0FFFFFFFF5A82FACF2AF0FBCFB1",
  ":100310002BF0D9CF2CF0DACF2DF0F3CF2EF0F4CF95",
  ...
}

```

and append the code into HexFileImage.h.

##  Step-by-step

After compiling the source code and uploading it to the board you will be able to start the process using a serial terminal.

Just open the Arduino Serial Monitor (at 115200 baud) and you will see this:

```
** SODAQ Firmware Updater **
Version 1.4

Press:
- 'b' to enable bootloader mode
- 'd' to enable debug
.......
```

You have 5 seconds to press any of the shown keys to enable the shown functionality (optional).

Then, the hex file image will be verified while showing the progress:

```
** SODAQ Firmware Updater **
Version 1.4

Press:
 - 'b' to enable bootloader mode
 - 'd' to enable debug
....................

* Starting HEX File Image Verification...
 0% |||||||||||| 25% |||||||||||| 50% |||||||||||| 75% |||||||||||| 100% 
HEX File Image Verification Successful!
```

At this point you should normally see the following message, allowing you to start the update process:

```
* The module is in Application mode: 
RN2483 1.0.1 Dec 15 2015 09:38:09

Ready to start firmware update...
Firmware Image: RN2483_101

Please press 'c' to continue...
```

The update will begin and a similar progress bar as above will be shown. Once the update is complete you can power-cycle the module to boot the new firmware!

## In case something goes wrong
In case there is something wrong after the module's application has been erased you can force the updater to communicate directly with the module's bootloader by pressing 'b' during the 5-seconds boot delay.

## License

Copyright (c) 2017, SODAQ
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

