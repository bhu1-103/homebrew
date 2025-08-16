#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/audioin.h>
#include <psp2/ctrl.h>
#include <psp2/net/net.h>
#include <psp2/sysmodule.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#include <stdlib.h>
#include <string.h>

#include "debugScreen.h"

#define printf psvDebugScreenPrintf
#define MAX_COMMANDS 32
//#define SERVER_IP "192.168.0.104"
#define SERVER_PORT 2012
#define CONTROL_PORT 2013
#define NET_PARAM_MEM_SIZE (1*1024*1024)
#define CONFIG_FILE "ux0:data/alter-ego/config.txt"
#define MAX_IP_LEN 32

#define MAX_VU 50

typedef struct {
    uint32_t button_mask;
    const char *cmd;
} ButtonCmd;

const char *commands[MAX_COMMANDS] = {
    "CMD:toggle music",
    "CMD:previous song",
    "CMD:next song",
    "CMD:shut up",
    "CMD:today",
    "CMD:lock screen",
    "CMD:files",
    "CMD:what time is it",
    "CMD:screenshot",
    "CMD:volume up",
    "CMD:volume down",
    "CMD:open firefox",
    "WAKE",
    "CONFIRM"
};
int command_count = 14;
int current_command = 0;

ButtonCmd button_cmds[] = {
    /*{SCE_CTRL_CROSS, "CONFIRM"},*/
    {SCE_CTRL_CIRCLE, "CMD:shut up"},
    {SCE_CTRL_TRIANGLE, "WAKE"},
    {SCE_CTRL_SQUARE, "CMD:toggle music"},
    {SCE_CTRL_LEFT, "CMD:previous song"},
    {SCE_CTRL_RIGHT, "CMD:next song"},
    /*{SCE_CTRL_L, "LEFT_REL"},
    {SCE_CTRL_R, "RIGHT_REL"}, //to cycle certain commands in a string array */
};

void send_cmd(SceNetSockaddrIn *server_addr, int sock, const char *cmd) {
    sceNetSendto(sock, cmd, strlen(cmd), 0,
                 (SceNetSockaddr*)server_addr, sizeof(*server_addr));
}

int read_ip_from_config(char *ip_buffer, size_t buf_size)
{
    SceUID fd = sceIoOpen(CONFIG_FILE, SCE_O_RDONLY, 0);
    if (fd < 0) {printf("config file not found");return -1;}
    int bytes_read = sceIoRead(fd, ip_buffer, buf_size - 1);
    sceIoClose(fd);
    
    if (bytes_read <= 0){printf("failed to read ip from config"); return -2;}
    ip_buffer[bytes_read] = '\0'; //wow, okay
    for (int i = 0; i < bytes_read; i++) 
    {
        if (ip_buffer[i] == '\n' || ip_buffer[i] == '\r')
        {
            ip_buffer[i] = '\0';
            break;
        }
    }
    return 0;
}


int main(int argc, char *argv[]) {
    int sfd;
    int control_sock;
    SceNetSockaddrIn server_addr;
    SceNetSockaddrIn server_ctrl_addr;
    SceNetInitParam net_init_param;
    SceCtrlData ctrl, oldCtrl;
    memset(&oldCtrl, 0, sizeof(oldCtrl));

    psvDebugScreenInit();

    char SERVER_IP[MAX_IP_LEN];
    if (read_ip_from_config(SERVER_IP, sizeof(SERVER_IP)) < 0)
    {
        printf("create %s with ip address of laptop/pc\n", CONFIG_FILE);
        return 1;
    }
    printf("Server IP: %s\n",SERVER_IP);

    // Init networking
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    net_init_param.memory = malloc(NET_PARAM_MEM_SIZE);
    memset(net_init_param.memory, 0, NET_PARAM_MEM_SIZE);
    net_init_param.size = NET_PARAM_MEM_SIZE;
    net_init_param.flags = 0;
    sceNetInit(&net_init_param);

    // Create UDP socket
    sfd = sceNetSocket("mic_socket", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, 0);
    control_sock = sceNetSocket("ctrl_socket", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, 0);

    //audio stream socket
    server_addr.sin_family = SCE_NET_AF_INET;
    server_addr.sin_port = sceNetHtons(SERVER_PORT);
    sceNetInetPton(SCE_NET_AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    //controls socket
    server_ctrl_addr.sin_family = SCE_NET_AF_INET;
    server_ctrl_addr.sin_port = sceNetHtons(CONTROL_PORT);
    sceNetInetPton(SCE_NET_AF_INET, SERVER_IP, &server_ctrl_addr.sin_addr);

    // Open mic
    int freq = 16000;
    int grain = 256;
    int port = sceAudioInOpenPort(SCE_AUDIO_IN_PORT_TYPE_VOICE, grain, freq, SCE_AUDIO_IN_PARAM_FORMAT_S16_MONO);
    short *audioIn = (short*)malloc(sizeof(short) * grain);

    int sensitivity = 3;
    int mic_enabled = 1;  // mic enabled by default

    printf("\e[s"); // save cursor position
    
    while (1) {
        sceCtrlPeekBufferPositive(0, &ctrl, 1);

        // Toggle mic with START
        if ((ctrl.buttons & SCE_CTRL_START) && !(oldCtrl.buttons & SCE_CTRL_START)) {
            mic_enabled = !mic_enabled;
        }

        if ((ctrl.buttons & SCE_CTRL_UP) && !(oldCtrl.buttons & SCE_CTRL_UP)) sensitivity++;
        if ((ctrl.buttons & SCE_CTRL_DOWN) && !(oldCtrl.buttons & SCE_CTRL_DOWN)) sensitivity--;
        if (sensitivity < 1) sensitivity = 1;

        // Capture audio if mic is enabled
        if (mic_enabled) {
            sceAudioInInput(port, (void*)audioIn);
            sceNetSendto(sfd, (const void*)audioIn, sizeof(short) * grain, 0,
                         (SceNetSockaddr*)&server_addr, sizeof(server_addr));
        } else {
            memset(audioIn, 0, sizeof(short) * grain);  // mute
        }

        for (int i = 0; i < sizeof(button_cmds)/sizeof(button_cmds[0]); i++) {
            if ((ctrl.buttons & button_cmds[i].button_mask) && 
                !(oldCtrl.buttons & button_cmds[i].button_mask)) {
                send_cmd(&server_ctrl_addr, control_sock, button_cmds[i].cmd);
            }
        }

        //cycle all the commands
        if ((ctrl.buttons & SCE_CTRL_LTRIGGER) && !(oldCtrl.buttons & SCE_CTRL_LTRIGGER)) {  //the greatest feature of all time
            current_command--;
            if (current_command < 0) current_command = command_count - 1;
        }
        if ((ctrl.buttons & SCE_CTRL_RTRIGGER) && !(oldCtrl.buttons & SCE_CTRL_RTRIGGER)) {
            current_command++;
            if (current_command >= command_count) current_command = 0;
        }
        if ((ctrl.buttons & SCE_CTRL_CROSS) && !(oldCtrl.buttons & SCE_CTRL_CROSS)) {
            send_cmd(&server_ctrl_addr,control_sock, commands[current_command]);
        }


        // Compute simple average for VU meter
        int average = 0;
        int audioInMax = 1; // prevent div by zero
        for (int i = 0; i < grain; i++) {
            int val = abs(audioIn[i]);
            //int val = audioIn[i] < 0 ? 0 : audioIn[i];
            average += val;
            if (audioInMax < val) audioInMax = val;
        }
        average /= grain;
        average = ((average * sensitivity) * MAX_VU) / audioInMax;
        if (average > MAX_VU) average = MAX_VU;

        // Draw VU meter top-to-bottom
        printf("\e[u"); // restore cursor
        /*for (int i = MAX_VU-1; i >= 0; i--) {
            printf("\e[7G"); // col 7
            if (i >= MAX_VU - average) {
                if (i < MAX_VU / 2)
                    psvDebugScreenSetBgColor(0xFF00FF00);
                else if (i < MAX_VU*3/4)
                    psvDebugScreenSetBgColor(0xFF00FFFF);
                else
                    psvDebugScreenSetBgColor(0xFF0000FF);
            } else {
                psvDebugScreenSetBgColor(0xFF000000);
            }
            printf("           \n");
        }
        psvDebugScreenSetBgColor(0xFF000000);*/

        //printf("Streaming mic to %s:%d via UDP...\n", SERVER_IP, SERVER_PORT);
        //printf("Mic is %s | Sensitivity = %d\n\n", mic_enabled ? "    ENABLED     " : "      MUTED     ", sensitivity);
        //printf("Up/Down: VU sensitivity | START: Toggle mic | SELECT: Quit\n\n");
        printf("Selected Command: %-20s\n", commands[current_command]);
        printf("Press X to execute command");

        memcpy(&oldCtrl, &ctrl, sizeof(SceCtrlData));
        sceKernelDelayThread(10000);

        if ( ((ctrl.buttons & SCE_CTRL_SELECT) & !(oldCtrl.buttons & SCE_CTRL_SELECT)) & ((ctrl.buttons & SCE_CTRL_DOWN) & !(oldCtrl.buttons & SCE_CTRL_DOWN)) ) break;
    }

    free(audioIn);
    sceAudioInReleasePort(port);
    sceNetSocketClose(sfd);
    sceNetTerm();
    free(net_init_param.memory);
    sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);

    return 0;
}
