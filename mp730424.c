
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <winuser.h>
#pragma comment(lib, "user32.lib")

HANDLE port;                            // Serial COM port
BOOL loop = TRUE;                       // Serial reader loop
FILE *fo;                               // CSV file writer
BOOL fileLogging = FALSE;               // Logging to CSV file disabled by default
HWND requestTimer;                      // Multimeter requesting timer
UINT_PTR timerId = 0;                   // Multimeter requesting timer ID
char fileName[50] = "measurements.csv"; // Default logging filename
char portName[20] = "COM1";             // Default COM port
char portPath[100] = "\\.\\COM1";       // Default COM port path
DWORD portSpeed = 115200;               // Default COM baudrate
DWORD timerDelayMs = 500;               // Default multimeter querying delay

void writeAndReadUntilEOL(HANDLE *port, char *request, DWORD request_size, char *response);    // Write to Serial and wait to response until \n
void onSignal(int signal);                                                                      // Signal handler
int main(int argc, char *argv[]);                                                               // App entry

// Signal handler
void onSignal(int signal)
{
    if (signal == SIGINT)
    {
        printf("\r\n");
        printf("Closing timer... ");
        if (KillTimer(requestTimer, timerId))
        {
            printf("OK\r\n");
        }
        else
        {
            printf("ERROR\r\n");
        }
        if (fileLogging)
        {
            printf("Closing %s... ", fileName);
            if (fclose(fo) == 0)
            {
                printf("OK\r\n");
            }
            else
            {
                printf("ERROR\r\n");
            }
        }
        if (port != NULL)
        {
            printf("Closing %s.... ", portName);
            if (CloseHandle(port))
            {
                printf("OK\r\n");
            }
            else
            {
                printf("ERROR\r\n");
            }
        }

        printf("Bye bye!\r\n");
        exit(0);
    }
}

// Write query to Serial COM and wait for response until \n, or timeout
// Reading from serial buffer char by char
void requestData(HANDLE *port, char *request, DWORD request_size, char *response)
{
    DWORD sent = 0;
    WriteFile(*port, request, request_size, &sent, NULL);
    if (sent == request_size)
    {
        size_t pos = 0;
        while (1) // Infinite loop, breaking manually
        {
            char buffer[2];
            DWORD received = 0;
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
            else // If we did not received just 1 char, let's break while loop
                break;
        }
    }
}

void CALLBACK requestLoop(HWND hwnd, UINT msg, UINT timer, DWORD time)
{
    // Get current time in hh:mm:ss.ms
    SYSTEMTIME t;
    GetLocalTime(&t);
    char dt[13];
    sprintf(dt, "%02d:%02d:%02d.%03d", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

    // Ask multimeter for its current function via "FUNC?\n" query
    char response[50];
    requestData(&port, "FUNC?\n", 6, response);
    printf("%s\t %s\t", dt, response);
    if (fileLogging)
        fprintf(fo, "%s;%s;", dt, response);

    // Ask multimeter for its current measurement via "MEAS?\n" query
    requestData(&port, "MEAS?\n", 6, response);
    double number = atof(response);
    printf("%f\r\n", number);
    if (fileLogging)
        fprintf(fo, "%f\n", number);
}

// App entry
int main(int argc, char *argv[])
{
    MSG msg;

    signal(SIGINT, onSignal); // SIGINT registration

    /**
     * Stupid argv parser
     * -p port, COM1, COM2, ...
     * -b baudrate, 9600, 115200, ...
     * -s multimeter querying speed in ms
     * -f CSV filename for logging
     */
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
        {
            sprintf(portPath, "\\\\.\\%s", argv[i + 1]);
            strcpy(portName, argv[i + 1]);
        }
        if (strcmp(argv[i], "-b") == 0)
            portSpeed = atoll(argv[i + 1]);
        if (strcmp(argv[i], "-s") == 0)
            timerDelayMs = atoll(argv[i + 1]);
        if (strcmp(argv[i], "-f") == 0)
        {
            strcpy(fileName, argv[i + 1]);
            fileLogging = TRUE;
        }
    }

    // Openning COM port
    printf("Openning port %s... ", portName);
    port = CreateFileA(portPath,                     // port path
                       GENERIC_READ | GENERIC_WRITE, // Read/Write
                       0,                            // No Sharing
                       NULL,                         // No Security
                       OPEN_EXISTING,                // Open existing port only
                       0,                            // Non Overlapped I/O
                       NULL);                        // Null for Comm Devices

    if (port == INVALID_HANDLE_VALUE)
        printf("ERROR\r\n");
    else
    {
        printf("OK\r\n");
        printf("Flushing port buffers... ", portName);
        if (FlushFileBuffers(port))
        {
            printf("OK\r\n");
        }
        else
        {
            printf("ERROR\r\n");
        }

        // Port timeouts in ms
        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = 0;
        timeouts.ReadTotalTimeoutConstant = 100;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 100;
        timeouts.WriteTotalTimeoutMultiplier = 0;
        printf("Configuring port timers... ");
        if (SetCommTimeouts(port, &timeouts))
        {
            printf("OK\r\n");
        }
        else
        {
            printf("ERROR\r\n");
        }

        // Serial setup
        DCB state = {0};
        state.DCBlength = sizeof(DCB);
        state.BaudRate = portSpeed;
        state.ByteSize = 8;
        state.Parity = NOPARITY;
        state.StopBits = ONESTOPBIT;
        printf("Configuring serial connection %d-8-N-1... ", portSpeed);
        if (SetCommState(port, &state))
        {
            printf("OK\r\n");
        }
        else
        {
            printf("ERROR\r\n");
        }

        // Requesting multimeter for its ID via "*IDN?\n" query
        char response[100]; // Response buffer
        printf("Requesting name of connected device: ");
        requestData(&port, "*IDN?\n", 6, response);
        if (strlen(response) > 0)
        {
            printf("%s\r\n", response);
        }
        else
        {
            printf("Unknown device\r\n");
        }

        // Open file for CSV logging
        if (fileLogging)
        {
            printf("Openning %s for logging... ", fileName);
            fo = fopen(fileName, "w");
            if (fo != NULL)
            {
                printf("OK\r\n");
            }
            else
            {
                printf("ERROR\r\n");
                fileLogging = FALSE;
            }
        }

        printf("\r\n");

        timerId = SetTimer(requestTimer, 1, timerDelayMs, (TIMERPROC)&requestLoop);

        // Messaging loop
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}
