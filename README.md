# ZYDP miniPad

![](../hardware/images/IMG_20240917_183127_444.jpg)

The ZYDP miniPad is an experimental MIDI finger drumpad project based on the Blue Pill (STM32F103C8T6) development board. The project features 12 pads and 5 pushbuttons for controlling the drumpad, together with TRS and USB MIDI output and the possibility of expanding the control using foot pedals. The pads are constructed from piezoelectric discs as the sensor. 

[PlatformIO](https://platformio.org/) is used as the development framework of this project, using the [old Arduino STM32](https://github.com/rogerclarkmelbourne/Arduino_STM32) core by rogerclarkmelbourne (named as "genericSTM32F103C8" under PlatformIO).

## Status: Alpha

This project is currently still experimental, and possibly will remain this way. Therefore, it is not encouraged to recreate this project if you want to make something that will work. However, feel free to use parts of this project in your own project. 

That being said, this project is a part of my upcoming series of MIDI musical instrument project, and therefore will likely serve as the base for my future projects.

## Objectives and Results
### Software
1. Explore the PlatformIO framework and Blue Pill development board.
1. Learn about the USB functionalities of the board and library to use them, especially MIDI.
    - [USBComposite](https://github.com/arpruss/USBComposite_stm32f1) library is used as it can create multiple types of USB devices simultaneously. This allows a separate serial interface to be created to allows control through connected device.
1. Develop potentially useful utility functions that can be reused in future project.
    - See the [lib](lib) folder for reusable functions and classes.
1. Develop a preset system for storing user configuration, and an interface to enable modification of the configuration on-the-fly through the device it is connected to. 
    - User can define their own settings across multiple banks and slots. The implementation took inspiration from memory system of commercial piano keyboards.

### Hardware
1. Improve on the overall appearance and finish of the project. Note that the construction of the hardware enclosure mainly uses mounting board as I do not have tools to deal with anything harder to process.
    - See the [hardware branch](../hardware) for images and approach taken to make the labelled faceplate from inkjet printer.
1. Build a mechanical construction that can effectively isolate the pads and prevent cross talk.
    - Failed objective and is area for future improvement.
1. Research on UART MIDI output circuit for 3.3V MCU.
    - The standard MIDI output is using 5V instead of 3.3V.
    - But because MIDI input is a current device (optocoupler) as opposed to voltage, it is possible to drive it using 3.3V level. However, the lower resistor values makes the output susceptible to short-circuit damage.
    - An open-collector output is constructed to produce 5V output from the 3.3V signal.
1. Investigate the viability of using piezoelectric disc in finger drumpad (pressure sensitive resistors are usually used instead).
    - The main reason for using piezoelectric disc is because I was not able to source pressure sensitive resistors at reasonable price for multiple pads.
    - Another reason is this project will serve as the base for actual drumpad (that is hit with a drumstick as opposed to fingers), and that commonly uses piezoelectric disc.
    - In this iteration, the design for isolating pads failed. The current design is inspired by [existing drumpad project](https://www.youtube.com/watch?v=ZOAaDEYHt2U) by other makers, the difference being this project is scaled down.
    - As not much research is done to improve the isolation, this objectives should continue to be investigated in the future iteration.

## Software setup

The following section is a documentation of all the software related setup of this project. For all the hardware related stuff, please check out the [hardware](../hardware) branch.

### Development environment

1. Install VSCode or VSCodium.
1. Install [PlatformIO extension](https://platformio.org/install/ide?install=vscode).
    - If using VSCodium, refer to [this](https://github.com/platformio/platformio-vscode-ide/issues/1802).
1. Clone this repository and open the folder in VSCode. PlatformIO should automatically setup the environment.

### Blue Pill Bootloader Installation

To allow uploading of code through the on board USB, a bootloader need to be installed on the board. The [STM32duino-bootloader](https://github.com/rogerclarkmelbourne/STM32duino-bootloader) is used for this project.

For Windows, there are plenty of resources available on how to upload the bootloader such as [this](https://circuitdigest.com/microcontroller-projects/programming-stm32f103c8-board-using-usb-port) using the ST Flash Loader tool. This section will focus on instruction for Linux environment.

1. Verify that your Blue Pill has the correct pull-up resistor of 1.5k. For more info, refer to [this](https://github.com/amitesh-singh/amitesh-singh.github.io/blob/master/_posts/2017-10-09-correcting-usbpullup-resistor.markdown).
1. Grab the correct bootloader from the [Github page](https://github.com/rogerclarkmelbourne/STM32duino-bootloader/tree/master/bootloader_only_binaries). The "generic" bootloaders are for the Blue Pill development board but there are several variants of it, where the names include the pin that is connected to the on board LED. You may need to choose a different version for your board, but mine uses the `generic_boot20_pc13.bin` bootloader.
1. Get the stm32flash program. The program is distributed on Debian 12 repository under the package `stm32flash`, to install it:
    ```
    sudo apt install stm32flash
    ```
    Alternatively, you may download the source code from [sourceforge](https://sourceforge.net/projects/stm32flash/) and compile from source. The compilation is trivial and instruction is available in the `INSTALL` file inside the tarball.
1. Set the BOOT0 jumper to 1 (programming mode). A USB to serial converter such as the FTDI FT232RL module is required. The following connection is made between the module and the board:
    - RX -> A9 (TX pin of Blue Pill)
    - TX -> A10 (RX pin of Blue Pill)
    - Vcc -> 5V
    - GND -> G
1. Connect the FTDI module to your computer. Check which serial port it is assigned to using the `dmesg` command and you will see similar output as the following (taken from [here](https://askubuntu.com/questions/1414103/ubuntu20-04-ttyusb-doesnt-show-up-when-ftdi-device-connected)):
    ```
    [1617898.963562] usb 4-2: new full-speed USB device number 9 using uhci_hcd
    [1617899.184579] usb 4-2: New USB device found, idVendor=0403, idProduct=6001, bcdDevice= 6.00
    [1617899.185006] usb 4-2: New USB device strings: Mfr=1, Product=2, SerialNumber=3
    [1617899.185469] usb 4-2: Product: TTL232R-3V3
    [1617899.186221] usb 4-2: Manufacturer: FTDI
    [1617899.186966] usb 4-2: SerialNumber: FT99IB1Q
    [1617899.194639] ftdi_sio 4-2:1.0: FTDI USB Serial Device converter detected
    [1617899.195105] usb 4-2: Detected FT232RL
    [1617899.197641] usb 4-2: FTDI USB Serial Device converter now attached to ttyUSB0
    ```
    In the above example, the serial port is assigned to `/dev/ttyUSB0`. It is likely that the serial converter will be assigned to `/dev/ttyUSBX` where `X` is some number.
1. Verify that the connection is ok by getting device information using the `stm32flash` command:
    ```
    stm32flash /dev/ttyUSB0
    ```
    Replace the serial port with your real serial port path.
1. To upload the downloaded bootloader, use the following command:
    ```
    stm32flash -v -w /path/to/downloaded/bootloader_file.bin /dev/ttyUSB0
    ```
    Again, replace the serial port with your real serial port path and the path to bootloader file.

    A similar output as the following should be displayed, indicating successful upload:
    ```
    stm32flash 0.7

    http://stm32flash.sourceforge.net/

    Using Parser    : Raw BINARY
    Size            : 22268
    Interface serial_posix: 57600 8E1
    Version         : 0x22
    Option 1        : 0x00
    Option 2        : 0x00
    Device 1D       : 0x0410 (STM32F10xxx Medium-density)
    - RAM           : Up to 20KiB (512b reserved by bootloader)
    - Flash         : Up to 128KiB (size first sector: 4x1024)
    - Option RAM    : 16b
    - System RAM    : 2KiB
    Write to memory
    Erasing memory
    Wrote and verified address 0x080056fc (100.00%) Done.
    ```

### Compilation and Uploading the Code

You should be able to use the compile and upload button at the lower left corner of the VSCode window to compile and upload to the Blue Pill.

## License

This software part of this project is licensed under the [Apache License Version 2.0](LICENSE).