## Multicomp Pro MP730424 Win32 serial logger
Simple Win32 console serial decoder of MP730424 messages in SCPI format ([datasheet](https://www.farnell.com/datasheets/3205713.pdf))

**How to use**:

    mp730424.exe -p port -s baudrate -d delayMs -f filename

**Example for COM8 and 115200 b/s:**

    mp730424.exe -p COM8 -s 115200 -d 1000 -f measurements.csv

With this setup, program will query multimeter every 1000 ms for current mode and measurement. It will print data with timestamp to stdout and to measurements.csv text file.

![mp730424.exe in action](https://raw.githubusercontent.com/jakubcizek/MP730424-Windows/main/screenshot.png)

![enter image description here](https://raw.githubusercontent.com/jakubcizek/MP730424-Windows/main/multimeter.jpg)
