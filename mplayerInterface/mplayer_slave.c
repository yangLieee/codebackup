#include <stdio.h>
#include <libavformat/avformat.h>

#define PIPESIZE 64*1024

static int fd_fifo = -1;
static const char *pipeName = "mplayerPipe";

void mplayer_demo_main_thread(void)
{
    // Pre Prepare
    int pos = 0;
    char buffer[100] = {0};
    char out_buffer[1024] = {0};

    // Step1: Create file
    fd_fifo = open(pipeName,O_WRONLY);

    // Step2: Exec mplayer command
    // mplayer -vf rotate=2 -salve -input file=testVideo.mp4

    // Step3: Create pipe and fork sub-process
    FILE *fp = popen(buffer, "r");

    while(1) {
        // Step4: read fp->pipe context to out_buffer
        char *result = fgets(out_buffer, sizeof(out_buffer), fp);
        if(result == NULL) {
            // Must stop. Please exit
            break;
        }

        // Step5: Compare pipe context
        if ((pos = strstr(out_buffer, "ANS_LENGTH=")) != NULL) {
            int timeLength = atof(pos+11);
        }
        if ((pos = strstr(out_buffer, "ANS_TIME_POSITION=")) != NULL) {
            int timePos = atof(pos+18);
        }
        if ((pos = strstr(out_buffer, "ANS_PERCENT_POSITION=")) != NULL) {
            int percentPos = atoi(pos+21);
        }
    }
    pclose(fp);
}

int player_stop()
{
    char buffer[100] = {0};
    sprintf(buffer,"stop\n");
    int ret = write(fd_fifo, buffer ,strlen(buffer));
    if(ret != strlen(buffer)) {
        //Write Error
    }
    return 0;
}

int player_quit()
{
    char buffer[100] = {0};
    sprintf(buffer,"quit\n");
    int ret = write(fd_fifo, buffer ,strlen(buffer));
    if(ret != strlen(buffer)) {
        //Write Error
    }
    return 0;
}

int player_pause()
{
    char buffer[100] = {0};
    sprintf(buffer,"pause\n");
    int ret = write(fd_fifo, buffer ,strlen(buffer));
    if(ret != strlen(buffer)) {
        //Write Error
    }
    return 0;
}

int player_get_percentPos()
{
    char buffer[100] = {0};
    sprintf(buffer,"get_percent_pos\n");
    int ret = write(fd_fifo, buffer ,strlen(buffer));
    if(ret != strlen(buffer)) {
        //Write Error
    }
    return 0;
}

int player_get_timePos()
{
    char buffer[100] = {0};
    sprintf(buffer,"get_time_pos\n");
    int ret = write(fd_fifo, buffer ,strlen(buffer));
    if(ret != strlen(buffer)) {
        //Write Error
    }
    return 0;
}

int player_get_timeLength()
{
    char buffer[100] = {0};
    sprintf(buffer,"get_time_length\n");
    int ret = write(fd_fifo, buffer ,strlen(buffer));
    if(ret != strlen(buffer)) {
        //Write Error
    }
    return 0;
}

int player_seek(float value)
{
    char buffer[100] = {0};
    sprintf(buffer,"seek %f2\n", value);
    int ret = write(fd_fifo, buffer ,strlen(buffer));
    if(ret != strlen(buffer)) {
        //Write Error
    }
    return 0;
}
