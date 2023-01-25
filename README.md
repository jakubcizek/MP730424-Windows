## Multicomp Pro MP730424 Win32 serial logger
Simple Win32 console serial decoder of MP730424 messages in SCPI format ([datasheet](https://www.farnell.com/datasheets/3205713.pdf)). You can build it with MSVC build tools.

**How to use**:

    mp730424.exe -p port -s baudrate -d delayMs -f filename

**Example for COM8 and 115200 b/s:**

    mp730424.exe -p COM8 -s 115200 -d 1000 -f measurements.csv

With this setup, program will periodically query multimeter after 1000 ms for function and measurement. It will print data with HH:MM:SS.ms timestamp to stdout and measurements.csv text file.

![mp730424.exe in action](https://raw.githubusercontent.com/jakubcizek/MP730424-Windows/main/screenshot.png)

![enter image description here](https://raw.githubusercontent.com/jakubcizek/MP730424-Windows/main/multimeter.jpg)
