#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/audioin.h>
#include <psp2/ctrl.h>
#include <psp2/net/net.h>
#include <psp2/sysmodule.h>

#include <stdlib.h>
#include <string.h>

#include "debugScreen.h"

#define printf psvDebugScreenPrintf

#define SERVER_IP "192.168.0.104" //will fix later
#define SERVER_PORT 2012
#define NET_PARAM_MEM_SIZE (1*1024*1024)

#define MAX_VU 32

int main(int argc, char *argv[]) {
    int sfd;
    SceNetSockaddrIn server_addr;
    SceNetInitParam net_init_param;

    psvDebugScreenInit();
    printf("Streaming mic to %s:%d via UDP...\n", SERVER_IP, SERVER_PORT);
    printf("Up/Down: VU sensitivity | START: Toggle mic | SELECT: Quit\n\n");

    // Init networking
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    net_init_param.memory = malloc(NET_PARAM_MEM_SIZE);
    memset(net_init_param.memory, 0, NET_PARAM_MEM_SIZE);
    net_init_param.size = NET_PARAM_MEM_SIZE;
    net_init_param.flags = 0;
    sceNetInit(&net_init_param);

    // Create UDP socket
    sfd = sceNetSocket("mic_socket", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, 0);

    server_addr.sin_family = SCE_NET_AF_INET;
    server_addr.sin_port = sceNetHtons(SERVER_PORT);
    sceNetInetPton(SCE_NET_AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Open mic
    int freq = 16000;
    int grain = 256;
    int port = sceAudioInOpenPort(SCE_AUDIO_IN_PORT_TYPE_VOICE, grain, freq, SCE_AUDIO_IN_PARAM_FORMAT_S16_MONO);
    short *audioIn = (short*)malloc(sizeof(short) * grain);

    SceCtrlData ctrl, oldCtrl;
    memset(&oldCtrl, 0, sizeof(oldCtrl));

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

        // Compute simple average for VU meter
        int average = 0;
        int audioInMax = 1; // prevent div by zero
        for (int i = 0; i < grain; i++) {
            int val = audioIn[i] < 0 ? 0 : audioIn[i];
            average += val;
            if (audioInMax < val) audioInMax = val;
        }
        average /= grain;
        average = ((average * sensitivity) * MAX_VU) / audioInMax;
        if (average > MAX_VU) average = MAX_VU;

        // Draw VU meter top-to-bottom
        printf("\e[u"); // restore cursor
        for (int i = MAX_VU-1; i >= 0; i--) {
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
        psvDebugScreenSetBgColor(0xFF000000);

        printf("\nMic is %s | Sensitivity = %d\n\n", mic_enabled ? "ENABLED" : "MUTED", sensitivity);

        memcpy(&oldCtrl, &ctrl, sizeof(SceCtrlData));
        sceKernelDelayThread(10000);

        if (ctrl.buttons & SCE_CTRL_SELECT) break;
    }

    free(audioIn);
    sceAudioInReleasePort(port);
    sceNetSocketClose(sfd);
    sceNetTerm();
    free(net_init_param.memory);
    sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);

    return 0;
}
