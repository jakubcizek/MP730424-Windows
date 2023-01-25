
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>

HANDLE port;        // Serial COM port
BOOL loop = TRUE;   // Serial reader loop
FILE *fo;           // CSV file writer

void writeAndReadUntilEOL(HANDLE *port, char *request, size_t request_size, char *response);    // Write to Serial and wait to response until \n
void onSignal(int signal);                                                                      // Signal handler
int main(int argc, char *argv[]);                                                               // App entry

// Signal handler
void onSignal(int signal)
{
    if (signal == SIGINT)
    {
        loop = FALSE;
    }
}

// Write query to Serial COM and wait for response until \n, or timeout
// Reading from serial buffer char by char
void writeAndReadUntilEOL(HANDLE *port, char *request, DWORD request_size, char *response)
{
    LPDWORD sent = 0;
    WriteFile(*port, request, request_size, &sent, NULL);
    if (sent == request_size)
    {
        size_t pos = 0;
        while (1) // Infinite loop, breaking manually
        {
            char buffer[2];
            LPDWORD received = 0;
            ReadFile(*port, buffer, 1, &received, NULL);
            if (received == 1)
            {
                if ((int)buffer[0] != 10) // If not LF (\n)
                {
                    if ((int)buffer[0] != 13) // If not CR (\r)
                        response[pos++] = buffer[0];
                }
                else
                {
                    response[pos] = '\0'; // Close string and break while loop
                    break;
                }
            }
            else    // If we did not received just 1 char, let's break while loop
                break;
        }
    }
}

// App entry
int main(int argc, char *argv[])
{
    char portName[20] = "COM1";             // Default COM port
    DWORD portSpeed = 115200;               // Default COM baudrate
    DWORD delayMs = 500;                    // Default multimeter querying delay
    char fileName[50] = "measurements.csv"; // Default filename
    BOOL writing = FALSE;                   // Logging to CSV file disabled by default

    signal(SIGINT, onSignal);               // SIGINT registration

    /**
     * Stupid argv parser
     * -p port, COM1, COM2, ...
     * -s baudrate, 9600, 115200, ...
     * -d multimeter querying delay in ms
     * -f CSV filename for logging
     */
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
            sprintf(portName, "\\\\.\\%s", argv[i + 1]);
        if (strcmp(argv[i], "-s") == 0)
            portSpeed = atoll(argv[i + 1]);
        if (strcmp(argv[i], "-d") == 0)
            delayMs = atoll(argv[i + 1]);
        if (strcmp(argv[i], "-f") == 0)
        {
            strcpy(fileName, argv[i + 1]);
            writing = TRUE;
        }
    }

    // Openning COM port
    port = CreateFileA(portName,                     // port name
                       GENERIC_READ | GENERIC_WRITE, // Read/Write
                       0,                            // No Sharing
                       NULL,                         // No Security
                       OPEN_EXISTING,                // Open existing port only
                       0,                            // Non Overlapped I/O
                       NULL);                        // Null for Comm Devices

    if (port == INVALID_HANDLE_VALUE)
        printf("Can not open %s\r\n", portName);
    else
    {
        FlushFileBuffers(port); // Flushing port buffers

        // Port timeouts in ms
        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = 0;
        timeouts.ReadTotalTimeoutConstant = 100;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 100;
        timeouts.WriteTotalTimeoutMultiplier = 0;
        SetCommTimeouts(port, &timeouts);

        // Serial setup
        DCB state = {0};
        state.DCBlength = sizeof(DCB);
        state.BaudRate = portSpeed;
        state.ByteSize = 8;
        state.Parity = NOPARITY;
        state.StopBits = ONESTOPBIT;
        SetCommState(port, &state);

        char response[100]; // Response buffer
        
        // Ask multimeter for its ID via "*IDN?\n" query
        writeAndReadUntilEOL(&port, "*IDN?\n", 6, response);
        printf("%s\r\n", response);

        // Open file for CSV logging
        if (writing)
        {
            fo = fopen(fileName, "w");
        }

        // Querying loop
        while (loop)
        {
            // Get current time in hh:mm:ss.ms
            SYSTEMTIME t;
            GetLocalTime(&t);
            char dt[13];
            sprintf(dt, "%02d:%02d:%02d.%03d", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

            // Ask multimeter for its current function via "FUNC?\n" query
            writeAndReadUntilEOL(&port, "FUNC?\n", 6, response);
            printf("%s\t %s\t", dt, response);
            if (writing)
                fprintf(fo, "%s;%s;", dt, response);

            // Ask multimeter for its current measurement via "MEAS?\n" query
            writeAndReadUntilEOL(&port, "MEAS?\n", 6, response);
            double number = atof(response);
            printf("%f\r\n", number);
            if (writing)
                fprintf(fo, "%f\n", number);
            
            Sleep(delayMs); // Next query delay
        }

        if (writing)
            fclose(fo);
    }

    CloseHandle(port);  // Close COM port

    return 0; // Bye bye!
}