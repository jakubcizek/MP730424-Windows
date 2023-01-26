## Multicomp Pro MP730424 Win32 serial logger
Simple Win32 console serial decoder of MP730424 messages in SCPI format ([datasheet](https://www.farnell.com/datasheets/3205713.pdf)). You can compile it with [MSVC build tools and extensions for VS Code](https://code.visualstudio.com/docs/cpp/config-msvc).

**How to use**:

    mp730424.exe -p port -b baudrate -s delayMs -f filename

**Example for COM8 and 115200 b/s:**

    mp730424.exe -p COM8 -b 115200 -s 1000 -f measurements.csv

With this setup, program will periodically request multimeter data every 1000 ms for current function and measurement. It will print data with HH:MM:SS.ms timestamp to stdout and measurements.csv semicolon-separated text file.

Still some small r/w sync bugs if you switch function on multimeter during recording. Do not switch functions during session ;-) 

![mp730424.exe in action](https://raw.githubusercontent.com/jakubcizek/MP730424-Windows/main/screenshot.png)

![enter image description here](https://raw.githubusercontent.com/jakubcizek/MP730424-Windows/main/multimeter.jpg)
