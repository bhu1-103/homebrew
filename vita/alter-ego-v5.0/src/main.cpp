#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/audioin.h>
#include <psp2/net/net.h>
#include <psp2/sysmodule.h>
#include <psp2/io/fcntl.h>

#include <imgui_vita.h>
#include <vitaGL.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
    int _newlib_heap_size_user = 256 * 1024 * 1024;
}

#define SERVER_PORT 2012
#define CONTROL_PORT 2013
#define NET_PARAM_MEM_SIZE (1*1024*1024)
#define CONFIG_FILE "ux0:data/alter-ego/config.txt"
#define MAX_IP_LEN 32

#define SAMPLE_RATE 16000
#define GRAIN 256

const char *commands[] = {
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

const int command_count = sizeof(commands) / sizeof(commands[0]);

int read_ip_from_config(char *ip_buffer, size_t buf_size)
{
    SceUID fd = sceIoOpen(CONFIG_FILE, SCE_O_RDONLY, 0);
    if (fd < 0)
        return -1;

    int bytes_read = sceIoRead(fd, ip_buffer, buf_size - 1);
    sceIoClose(fd);

    if (bytes_read <= 0)
        return -2;

    ip_buffer[bytes_read] = '\0';

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

void send_cmd(SceNetSockaddrIn *addr, int sock, const char *cmd)
{
    sceNetSendto(sock, cmd, strlen(cmd), 0,
                 (SceNetSockaddr*)addr, sizeof(*addr));
}

int main(int, char**)
{
    // ---- Graphics ----
    vglInitExtended(0, 960, 544, 0x1800000, SCE_GXM_MULTISAMPLE_4X);

    ImGui::CreateContext();
    ImGui_ImplVitaGL_Init();
    ImGui::StyleColorsDark();
    ImGui_ImplVitaGL_TouchUsage(true);
    ImGui_ImplVitaGL_GamepadUsage(true);

    // ---- Network ----
    char SERVER_IP[MAX_IP_LEN];
    if (read_ip_from_config(SERVER_IP, sizeof(SERVER_IP)) < 0)
        return 1;

    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);

    SceNetInitParam net_init_param;
    net_init_param.memory = malloc(NET_PARAM_MEM_SIZE);
    memset(net_init_param.memory, 0, NET_PARAM_MEM_SIZE);
    net_init_param.size = NET_PARAM_MEM_SIZE;
    net_init_param.flags = 0;
    sceNetInit(&net_init_param);

    int audio_sock = sceNetSocket("audio_socket", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, 0);
    int ctrl_sock  = sceNetSocket("ctrl_socket", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, 0);

    SceNetSockaddrIn server_audio{};
    server_audio.sin_family = SCE_NET_AF_INET;
    server_audio.sin_port = sceNetHtons(SERVER_PORT);
    sceNetInetPton(SCE_NET_AF_INET, SERVER_IP, &server_audio.sin_addr);

    SceNetSockaddrIn server_ctrl{};
    server_ctrl.sin_family = SCE_NET_AF_INET;
    server_ctrl.sin_port = sceNetHtons(CONTROL_PORT);
    sceNetInetPton(SCE_NET_AF_INET, SERVER_IP, &server_ctrl.sin_addr);

    // ---- Audio ----
    int port = sceAudioInOpenPort(
        SCE_AUDIO_IN_PORT_TYPE_VOICE,
        GRAIN,
        SAMPLE_RATE,
        SCE_AUDIO_IN_PARAM_FORMAT_S16_MONO);

    short *audioIn = (short*)malloc(sizeof(short) * GRAIN);

    bool mic_enabled = true;
    int sensitivity = 3;
    int current_command = 0;
    float rms_level = 0.0f;

    while (1)
    {
        if (mic_enabled)
        {
            sceAudioInInput(port, (void*)audioIn);

            sceNetSendto(audio_sock,
                         audioIn,
                         sizeof(short) * GRAIN,
                         0,
                         (SceNetSockaddr*)&server_audio,
                         sizeof(server_audio));

            float sum = 0.0f;
            for (int i = 0; i < GRAIN; i++)
            {
                float s = audioIn[i] / 32768.0f;
                sum += s * s;
            }
            rms_level = sqrtf(sum / GRAIN);
        }
        else
        {
            rms_level = 0.0f;
        }

        // ---- UI ----
        ImGui_ImplVitaGL_NewFrame();

        ImGui::Begin("Alter Ego Control");

        ImGui::Text("Server: %s", SERVER_IP);
        ImGui::Checkbox("Mic Enabled", &mic_enabled);
        ImGui::SliderInt("Sensitivity", &sensitivity, 1, 10);

        ImGui::Separator();
        ImGui::ListBox("Commands", &current_command,
                       commands, command_count, 6);

        if (ImGui::Button("Send Command"))
        {
            send_cmd(&server_ctrl, ctrl_sock,
                     commands[current_command]);
        }

        ImGui::Separator();

        float vu = rms_level * sensitivity;
        if (vu > 1.0f) vu = 1.0f;

        ImGui::Text("Mic Level");
        ImGui::ProgressBar(vu, ImVec2(-1, 0));

        ImGui::End();

        // ---- Render ----
        glViewport(0, 0,
                   (int)ImGui::GetIO().DisplaySize.x,
                   (int)ImGui::GetIO().DisplaySize.y);

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
        vglSwapBuffers(GL_FALSE);

        sceKernelDelayThread(10000);
    }

    return 0;
}
