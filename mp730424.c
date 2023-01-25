
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>

HANDLE port;
BOOL loop = TRUE;
FILE *fo;

void writeAndReadUntilEOL(HANDLE *port, char *request, size_t request_size, char *response);

void onSignal(int signal)
{
    if (signal == SIGINT)
    {
        loop = FALSE;
    }
}

int main(int argc, char *argv[])
{
    char COM_NAME[20] = "COM1";
    DWORD COM_SPEED = 115200;
    DWORD delayMs = 500;
    char fileName[50] = "zaznam.csv";
    BOOL writing = FALSE;

    signal(SIGINT, onSignal);

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
            sprintf(COM_NAME, "\\\\.\\%s", argv[i + 1]);
        if (strcmp(argv[i], "-s") == 0)
            COM_SPEED = atoll(argv[i + 1]);
        if (strcmp(argv[i], "-d") == 0)
            delayMs = atoll(argv[i + 1]);
        if (strcmp(argv[i], "-f") == 0)
        {
            strcpy(fileName, argv[i + 1]);
            writing = TRUE;
        }
    }

    port = CreateFileA(COM_NAME,                     // port name
                       GENERIC_READ | GENERIC_WRITE, // Read/Write
                       0,                            // No Sharing
                       NULL,                         // No Security
                       OPEN_EXISTING,                // Open existing port only
                       0,                            // Non Overlapped I/O
                       NULL);                        // Null for Comm Devices

    if (port == INVALID_HANDLE_VALUE)
        printf("Nelze otevrit port %s\r\n", COM_NAME);
    else
    {
        FlushFileBuffers(port);

        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = 0;
        timeouts.ReadTotalTimeoutConstant = 100;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 100;
        timeouts.WriteTotalTimeoutMultiplier = 0;
        SetCommTimeouts(port, &timeouts);

        DCB state = {0};
        state.DCBlength = sizeof(DCB);
        state.BaudRate = COM_SPEED;
        state.ByteSize = 8;
        state.Parity = NOPARITY;
        state.StopBits = ONESTOPBIT;
        SetCommState(port, &state);

        char response[512];

        writeAndReadUntilEOL(&port, "*IDN?\n", 6, response);
        printf("Detekce multimetru: %s\r\n", response);

        if(writing){
            fo = fopen(fileName, "w");
        }

        while (loop)
        {
            SYSTEMTIME t;
            GetLocalTime(&t);
            char dt[13];
            sprintf(dt, "%02d:%02d:%02d.%03d", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
            writeAndReadUntilEOL(&port, "FUNC?\n", 6, response);
            printf("%s\t %s\t", dt, response);
            if(writing) fprintf(fo, "%s;%s;", dt, response);
            writeAndReadUntilEOL(&port, "MEAS?\n", 6, response);
            double number = atof(response);
            printf("%f\r\n", number);
            if(writing) fprintf(fo,"%f\n", number);
            Sleep(delayMs);
        }

        if(writing) fclose(fo);
    }

    CloseHandle(port);

    return 0;
}

void writeAndReadUntilEOL(HANDLE *port, char *request, size_t request_size, char *response)
{
    int sent = 0;
    WriteFile(*port, request, request_size, &sent, NULL);
    if (sent == request_size)
    {
        size_t pos = 0;
        while (1)
        {
            char buffer[2];
            int received;
            ReadFile(*port, buffer, 1, &received, NULL);
            if (received == 1)
            {
                if ((int)buffer[0] != 10)
                {
                    if ((int)buffer[0] != 13)
                        response[pos++] = buffer[0];
                }
                else
                {
                    response[pos] = '\0';
                    break;
                }
            }
        }
    }
}