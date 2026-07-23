## firmware for adcs cloudview 2 board
developed using the pico sdk, using the raspberry pi pico vs code extension.

The point of the code is to take code from the three sensors and send it as a string to the OBC. You
could encode it if you want, as it's more efficient, but both the ADCS and OBC are overkill when it
comes to compute so there's no need. The code is fairly well commented so you can look through it and
understand what I'm doing most of the time...

The pressure sensor is the most simple, it's a registor sensor so you read data from the registors
and store them into a buffer. This data then needs to be calibrated and converted. The driver for the
bmp280 is based off an example written by raspberry pi (link is in the c file) and is pretty
reliable.

The GPS is a bit more complex, but mostly because it's driver runs on a seperate core. It's because
the GPS itself should run on a slower baud rate because it leads to more accurate GPS data. However,
the slow baud rate jams the core running main, so the rest of it can't run. Eric wrote a small part
at the top of main.c which puts the processing of the GPS into the second core.

The IMU is the most complex, it runs on sh2 which is some proprietary sensor fusion firmware that the
IMU runs on its own on-board processor, which you have to work with. Most of the bno085 driver is
mandatory stuff that interfaces with sh2, I based it off the repo linked at the top of the driver,
but the best way to understand it is to read the source code (https://github.com/ceva-dsp/sh2) and
write functions based on what sh2 is asking for (I do not do this unless I have to). There is also
documentation that ceva (the guys who made the bno085) wrote, I have read this but near the start of
development when I was clueless, so it wasn't very helpful (https://www.ceva-ip.com/wp-content/uploads/SH-2-SHTP-Reference-Manual.pdf). The nice thing is that the sh2 driver I wrote is
usually not the problem, but instead something else thing I did, so hopefully this won't cause you
too much pain.
You can build and flash to the board using the raspberry pi pico extension in vscode.
Currently, the branch called archive-adcs contains the working code for cv2. I'm currently working on
making the code more readable and efficient by moving things from main into their own c files. This
is because main should mostly be calling functions from other files, rather than doing the work
itself. It's like an outline of the entire program.

I've found ometimes the IMU takes awhile to give you data, especially if you don't use the board for awhile
