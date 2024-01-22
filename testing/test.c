#include <stdio.h>
#include <string.h>

struct mydstat_info
{
    unsigned long recv;
    unsigned long send;
};

int get_net_info(struct mydstat_info *mydstat)
{
    FILE *file;
    char buffer[1024];

    // Opening file
    file = fopen("/proc/net/dev", "r");
    if (file == NULL)
    {
        printf("Error when opening /proc/net/dev file\n");
        return -1;
    }

    unsigned long recvTotal = 0;
    unsigned long sendTotal = 0;

    // Reading file content
    while (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        if (strstr(buffer, "lo:") == NULL) // Skip loopback interface
        {
            char interface[256];
            unsigned long recv, send;
            sscanf(buffer, "%[^:]: %lu %*u %*u %*u %*u %*u %*u %*u %lu %*u %*u %*u %*u %*u %*u %*u", interface, &recv, &send);

            recvTotal += recv;
            sendTotal += send;
        }
    }

    // Update mydstat with network information
    mydstat->recv = recvTotal;
    mydstat->send = sendTotal;

    // Close file
    fclose(file);
    return 1;
}

int main()
{
    struct mydstat_info mydstat;
    int result = get_net_info(&mydstat);
    if (result == 1)
    {
        printf("Network Usage:\n");
        printf("Received: %lu\n", mydstat.recv);
        printf("Sent: %lu\n", mydstat.send);
    }

    return 0;
}