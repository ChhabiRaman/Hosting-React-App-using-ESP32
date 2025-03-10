# HTTP Restful API Server Example using React

This project showcases how to run a React application on an ESP32 microcontroller using the ESP-IDF framework for IoT development and React with TypeScript for the front-end. Additionally, QEMU (Quick Emulator) is used to emulate the ESP32, enabling the execution of ESP32 code with Ethernet support for networking, all without the need for physical hardware.

## Overview

This example is an adaptation of the [RESTful server example][restful-server] (originally built using the Vue framework) from the official ESP-IDF GitHub repository. In this version, I’ve used the popular React framework to achieve a similar functionality.

## Deploy Mode Types

- SPI Flash
- SD Card
- Semihost Technology

Note: Only SD Card mode is supported when using QEMU.

## Software Requirements

- [node v23 or later]
- [TailWindCSS and DaisyUI]
- [ESP-IDF]
- [Docker]: Required only when using QEMU

## How to use example

If an SD Card is used to deploy the website, adjust the pin connections shown below according to your setup:

| ESP32  | SD Card |
| ------ | ------- |
| GPIO2  | D0      |
| GPIO4  | D1      |
| GPIO12 | D2      |
| GPIO13 | D3      |
| GPIO14 | CLK     |
| GPIO15 | CMD     |

Note: Optional when using QEMU.

**Project Configuration**

- If you are not using QEMU and want to use Wi-Fi for connectivity, then open the project configuration menu and navigate to the "WiFi Configuration" option and set your WiFi SSID and Password.
- Ethernet support is available in QEMU for ESP-IDF as described [here][openeth].

To build the React application, do the following.

```sh
cd path_to_this_example/front/web-demo
npm i
npm run build
```

Note: This should generate a folder named "dist" inside the web-demo directory.

**For only those using QEMU**

To add an SD card to the setup, create an image of the SD Card and pass it to the QEMU later.

```sh
dd if=/dev/zero bs=$((1024*1024)) count=64 of=sd_image.bin  //create 64MB raw image file
mkfs.vfat sd_image.bin	                    //Format the SD card image
mkdir /MOUNT_POINT 		                    //Mount the SD card image
mount -o loop sd_image.bin /MOUNT_POINT     //Mount the SD card image
cp abc.txt /MOUNT_POINT	                    //copy files to mount point
cp -r abc /MOUNT_POINT		                //or instead copy directory
ls /MOUNT_POINT		                        //verify the mount
umount /MOUNT_POINT		                    //unmount the image
```

To generate a merged bin file, with the name result.bin in this case, run the command given below. Change the offset for the bootloader, partition table, and firmware bin file if needed.

```sh
python -m esptool --chip esp32 merge_bin --output result.bin --fill-flash-size 4MB 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/{YOUR_PROJECT_NAME}.bin --flash_mode dio --flash_freq 40m --flash_size 4MB
```

Pass the SD Card image to the QEMU and forward the connection to port 8000 on your local machine.

```sh
qemu-system-xtensa -nographic -machine esp32 -drive file=result.bin,if=mtd,format=raw -nic user,model=open_eth,id=lo0,hostfwd=tcp:127.0.0.1:8000-:80 -drive file=sd_image.bin,if=sd,format=raw
```

## Example Output

![webserver](demo.gif)

[//]: # "These are reference links used in the body of this note and get stripped out when the markdown processor does its job."
[restful-server]: https://github.com/espressif/esp-idf/blob/master/examples/protocols/http_server/restful_server/README.md
[openeth]: https://github.com/espressif/esp-toolchain-docs/blob/main/qemu/esp32/README.md
