#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/audioin.h>
#include <psp2/net/net.h>
#include <psp2/sysmodule.h>
#include <psp2/io/fcntl.h>
#include <psp2/motion.h>
#include <psp2/rtc.h>

#include <imgui_vita.h>
#include <vitaGL.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <cstdio>

extern "C" {
    int _newlib_heap_size_user = 256 * 1024 * 1024;
}

#define SERVER_PORT 2012
#define CONTROL_PORT 2013
#define NET_PARAM_MEM_SIZE (1*1024*1024)
#define MAX_IP_LEN 32

#define SAMPLE_RATE 16000
#define GRAIN 256
#define EQ_BANDS 32
#define PI 3.1415926535f

// ===== Commands =====
const char* COMMAND_TOGGLE  = "CMD:toggle music";
const char* COMMAND_PREV    = "CMD:previous song";
const char* COMMAND_NEXT    = "CMD:next song";

const char *commands[] = {
    COMMAND_TOGGLE, COMMAND_PREV, COMMAND_NEXT,
    "CMD:shut up", "CMD:today", "CMD:lock screen",
    "CMD:files", "CMD:what time is it", "CMD:screenshot",
    "CMD:volume up", "CMD:volume down", "CMD:open firefox",
    "WAKE", "CONFIRM"
};
const int command_count = sizeof(commands)/sizeof(commands[0]);

// ===== Utils =====
float lerpf(float a, float b, float t) { return a + (b - a) * t; }
float clampf(float v, float mn, float mx) { return (v < mn) ? mn : (v > mx) ? mx : v; }

void send_cmd(SceNetSockaddrIn* addr, int sock, const char* cmd) {
    sceNetSendto(sock, cmd, strlen(cmd), 0, (SceNetSockaddr*)addr, sizeof(*addr));
}

// ===== Styling & Graphics Helpers =====

void SetupCyberpunkStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.WindowRounding = 12.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.FramePadding = ImVec2(10, 8);
    style.ItemSpacing = ImVec2(10, 10);

    ImVec4 bg_dark      = ImVec4(0.08f, 0.08f, 0.12f, 0.94f);
    ImVec4 accent_cyan  = ImVec4(0.00f, 0.95f, 1.00f, 1.00f);
    ImVec4 accent_pink  = ImVec4(1.00f, 0.00f, 0.55f, 1.00f);
    ImVec4 panel_bg     = ImVec4(0.15f, 0.15f, 0.22f, 0.85f);
    
    style.Colors[ImGuiCol_WindowBg]         = bg_dark;
    style.Colors[ImGuiCol_ChildBg]          = panel_bg;
    style.Colors[ImGuiCol_Border]           = accent_cyan;
    style.Colors[ImGuiCol_FrameBg]          = ImVec4(0.2f, 0.2f, 0.3f, 0.5f);
    style.Colors[ImGuiCol_FrameBgHovered]   = ImVec4(0.3f, 0.3f, 0.4f, 0.6f);
    style.Colors[ImGuiCol_FrameBgActive]    = ImVec4(0.4f, 0.4f, 0.5f, 0.6f);
    style.Colors[ImGuiCol_TitleBg]          = bg_dark;
    style.Colors[ImGuiCol_TitleBgActive]    = bg_dark;
    style.Colors[ImGuiCol_Button]           = ImVec4(0.2f, 0.2f, 0.3f, 0.6f);
    style.Colors[ImGuiCol_ButtonActive]     = accent_pink;
    style.Colors[ImGuiCol_Text]             = ImVec4(0.9f, 0.9f, 0.95f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]    = accent_cyan;
    style.Colors[ImGuiCol_SliderGrab]       = accent_pink;
    style.Colors[ImGuiCol_SliderGrabActive] = accent_cyan;
}

void CenterText(const char* text) {
    ImVec2 textSize = ImGui::CalcTextSize(text);
    float windowWidth = ImGui::GetWindowSize().x;
    float textPosX = (windowWidth - textSize.x) * 0.5f;
    if(textPosX > 0.0f) ImGui::SetCursorPosX(textPosX);
}

ImU32 LerpColor(ImU32 c1, ImU32 c2, float t) {
    int r1 = (c1 >> 0) & 0xFF; int g1 = (c1 >> 8) & 0xFF; int b1 = (c1 >> 16) & 0xFF; int a1 = (c1 >> 24) & 0xFF;
    int r2 = (c2 >> 0) & 0xFF; int g2 = (c2 >> 8) & 0xFF; int b2 = (c2 >> 16) & 0xFF; int a2 = (c2 >> 24) & 0xFF;
    
    int r = r1 + (int)((r2 - r1) * t);
    int g = g1 + (int)((g2 - g1) * t);
    int b = b1 + (int)((b2 - b1) * t);
    int a = a1 + (int)((a2 - a1) * t);
    
    return IM_COL32(r, g, b, a);
}

void DrawBackgroundWindow(float time, float yaw_offset) {
    ImGuiIO& io = ImGui::GetIO();
    
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoScrollbar | 
                             ImGuiWindowFlags_NoInputs | 
                             ImGuiWindowFlags_NoSavedSettings | 
                             ImGuiWindowFlags_NoFocusOnAppearing | 
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::Begin("Background", NULL, flags);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 size = io.DisplaySize;
    
    ImU32 col_top = IM_COL32(20, 10, 30, 255);
    ImU32 col_bot = IM_COL32(40, 20, 60, 255);

    draw_list->AddRectFilledMultiColor(ImVec2(0,0), size, col_top, col_top, col_bot, col_bot);

    float perspective = size.y * 0.4f;
    float horizon = size.y * 0.3f;

    // SUN
    ImVec2 sun_pos = ImVec2(size.x * 0.5f, horizon - 50.0f);
    float sun_radius = 80.0f;
    ImU32 sun_yellow = IM_COL32(255, 220, 0, 255);
    ImU32 sun_red    = IM_COL32(255, 0, 80, 255);
    ImU32 sun_violet = IM_COL32(140, 0, 255, 255);

    for (float y = -sun_radius; y < sun_radius; y += 1.0f) {
        float y_norm = y + sun_radius; 
        if ((int)y_norm % 8 >= 6) continue; 

        float width = sqrtf(sun_radius*sun_radius - y*y);
        float t = (y + sun_radius) / (sun_radius * 2.0f); 
        ImU32 line_color;
        
        if (t < 0.5f) {
            line_color = LerpColor(sun_yellow, sun_red, t * 2.0f);
        } else {
            line_color = LerpColor(sun_red, sun_violet, (t - 0.5f) * 2.0f);
        }

        draw_list->AddLine(ImVec2(sun_pos.x - width, sun_pos.y + y), ImVec2(sun_pos.x + width, sun_pos.y + y), line_color);
    }
    
    // GRID SETTINGS
    float spacing = 40.0f;  // Wider spacing for distinct grid cells
    float thickness = 4.0f; // BOLD lines (This is what you wanted!)
    int line_count = 120;   // Enough lines to cover the wrap-around
    float total_grid_width = line_count * spacing;
    
    for(int i = -60; i < 60; i++) {
        float x_base = (i * spacing) - (yaw_offset * 300.0f); 
        while(x_base < -size.x) x_base += total_grid_width;
        while(x_base > size.x + total_grid_width) x_base -= total_grid_width;
        
        if(x_base > -100.0f && x_base < size.x + 100.0f) {
            draw_list->AddLine(
                ImVec2(size.x/2 + (x_base - size.x/2) * 0.2f, horizon),
                ImVec2(x_base, size.y),
                IM_COL32(0, 240, 255, 60), // Increased alpha slightly for boldness
                thickness // CHANGED: 4.0f Thickness
            );
        }
    }
    
    // HORIZONTAL LINES
    float speed = 20.0f;
    float offset = fmodf(time * speed, 20.0f);
    for(float y = horizon; y < size.y; y += (y - horizon) * 0.05f + 3.0f + offset * 0.05f) {
        if(y > size.y) break;
        draw_list->AddLine(
            ImVec2(0, y), 
            ImVec2(size.x, y), 
            IM_COL32(0, 240, 255, 60), 
            2.0f // Horizontal lines kept slightly thinner for perspective
        );
    }

    // FOG
    draw_list->AddRectFilledMultiColor(
        ImVec2(0, horizon), ImVec2(size.x, horizon + 150.0f),
        col_bot, col_bot, col_bot & 0x00FFFFFF, col_bot & 0x00FFFFFF
    );

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void DrawIPOctet(const char* label_id, int* value) {
    ImGui::PushID(label_id);
    ImGui::BeginGroup();
    
    // Increased button size for easier touch
    if (ImGui::Button("-", ImVec2(40, 30))) {
        if (*value > 0) (*value)--;
    }
    
    ImGui::SameLine();
    
    char buff[8];
    sprintf(buff, "%3d", *value);
    
    float current_y = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(current_y + 5.0f); 
    ImGui::Text("%s", buff);
    ImGui::SetCursorPosY(current_y); 
    
    ImGui::SameLine();
    
    if (ImGui::Button("+", ImVec2(40, 30))) {
        if (*value < 255) (*value)++;
    }
    
    ImGui::EndGroup();
    ImGui::PopID();
}

void DrawEqualizer(float* values, int count, float w, float h) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    
    float bar_w = (w / count) - 2.0f;
    
    for (int i = 0; i < count; i++) {
        float val = clampf(values[i], 0.0f, 1.0f);
        float bar_h = val * h;
        
        ImVec2 p_min = ImVec2(pos.x + i * (bar_w + 2.0f), pos.y + h - bar_h);
        ImVec2 p_max = ImVec2(pos.x + i * (bar_w + 2.0f) + bar_w, pos.y + h);
        
        ImU32 col = (val > 0.8f) ? IM_COL32(255, 50, 50, 255) :
                    (val > 0.5f) ? IM_COL32(255, 200, 0, 255) :
                                   IM_COL32(0, 255, 200, 200);

        draw->AddRectFilled(p_min, p_max, col, 2.0f);
    }
    ImGui::Dummy(ImVec2(w, h));
}

// ===== Main =====

int main(int, char**) {
    vglInitExtended(0, 960, 544, 0x1800000, SCE_GXM_MULTISAMPLE_4X);
    ImGui::CreateContext();
    ImGui_ImplVitaGL_Init();
    ImGui_ImplVitaGL_TouchUsage(true);
    ImGui_ImplVitaGL_GamepadUsage(true);

    SetupCyberpunkStyle();

    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.0f;

    sceMotionStartSampling();
    sceMotionMagnetometerOn();
    sceMotionSetTiltCorrection(1);
    
    float smooth_yaw = 0.0f;
    float target_yaw = 0.0f;

    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    SceNetInitParam net_init_param;
    net_init_param.memory = malloc(NET_PARAM_MEM_SIZE);
    memset(net_init_param.memory, 0, NET_PARAM_MEM_SIZE);
    net_init_param.size = NET_PARAM_MEM_SIZE;
    net_init_param.flags = 0;
    sceNetInit(&net_init_param);

    int audio_sock = sceNetSocket("audio_socket", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, 0);
    int ctrl_sock  = sceNetSocket("ctrl_socket",  SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, 0);

    int ip_parts[4] = {192, 168, 0, 7};
    char server_ip[MAX_IP_LEN]; 

    SceNetSockaddrIn server_audio{}, server_ctrl{};

    short* audioIn = (short*)malloc(sizeof(short)*GRAIN);
    int port = sceAudioInOpenPort(SCE_AUDIO_IN_PORT_TYPE_VOICE, GRAIN, SAMPLE_RATE, SCE_AUDIO_IN_PARAM_FORMAT_S16_MONO);
    
    bool mic_enabled = true;
    int sensitivity = 4;
    float eq_values[EQ_BANDS] = {0};
    float anim_time = 0.0f;
    
    while (1) {
        anim_time += 1.0f / 60.0f;
        
        sprintf(server_ip, "%d.%d.%d.%d", ip_parts[0], ip_parts[1], ip_parts[2], ip_parts[3]);

        server_audio.sin_family = SCE_NET_AF_INET;
        server_audio.sin_port   = sceNetHtons(SERVER_PORT);
        sceNetInetPton(SCE_NET_AF_INET, server_ip, &server_audio.sin_addr);

        server_ctrl.sin_family  = SCE_NET_AF_INET;
        server_ctrl.sin_port    = sceNetHtons(CONTROL_PORT);
        sceNetInetPton(SCE_NET_AF_INET, server_ip, &server_ctrl.sin_addr);

        if (mic_enabled) {
            sceAudioInInput(port, audioIn);
            sceNetSendto(audio_sock, audioIn, sizeof(short)*GRAIN, 0, (SceNetSockaddr*)&server_audio, sizeof(server_audio));
        }

        for (int b = 0; b < EQ_BANDS; b++) {
            int start = (GRAIN/EQ_BANDS)*b;
            int end   = start + (GRAIN/EQ_BANDS);
            float sum = 0.0f;
            for (int i = start; i < end; i++) sum += fabsf(audioIn[i] / 32768.0f);
            float level = clampf(sum * sensitivity, 0, 1);
            eq_values[b] = lerpf(eq_values[b], level, 0.2f);
        }

        SceMotionState state;
        sceMotionGetState(&state);
        float raw_yaw = atan2f(state.nedMatrix.y.x, state.nedMatrix.x.x);
        
        float step_size = PI / 4.0f;
        target_yaw = roundf(raw_yaw / step_size) * step_size;
        smooth_yaw = lerpf(smooth_yaw, target_yaw, 0.1f);

        float breathe = (sinf(anim_time * 5.0f) * 0.5f) + 0.5f; 
        ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.95f, 1.0f, 0.4f + (breathe * 0.4f));

        ImGui_ImplVitaGL_NewFrame();
        
        DrawBackgroundWindow(anim_time, smooth_yaw);

        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::SetNextWindowBgAlpha(0.0f); 

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;

        ImGui::Begin("MainLayout", NULL, flags);

        float parallax_x = smooth_yaw * -15.0f; 
        float margin_x = 100.0f; 
        float gap_middle = 50.0f;
        float half_w = (io.DisplaySize.x - (margin_x * 2) - gap_middle) * 0.5f;
        float full_h = io.DisplaySize.y - 60.0f; 

        ImGui::SetCursorPos(ImVec2(margin_x + parallax_x, 30.0f));
        ImGui::BeginChild("LeftPanel", ImVec2(half_w, full_h), true);
        {
            ImGui::SetWindowFontScale(1.2f);
            ImGui::TextColored(ImVec4(0,1,1,1), "ALTER EGO v4.0");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushItemWidth(-1);
            if (ImGui::BeginCombo("##Cmds", "Execute Command...")) {
                for (int i=0;i<command_count;i++) {
                    if (ImGui::Selectable(commands[i])) send_cmd(&server_ctrl, ctrl_sock, commands[i]);
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGui::TextDisabled("INPUT SPECTRUM");
            DrawEqualizer(eq_values, EQ_BANDS, half_w - 30, 200);

            ImGui::Spacing();
            
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Text("GAIN"); ImGui::SameLine();
            ImGui::SliderInt("##sens", &sensitivity, 1, 10);
            
            ImGui::Checkbox("TRANSMIT AUDIO", &mic_enabled);
            if(mic_enabled) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1,0,0,1), " [ON AIR]");
            }
        }
        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(margin_x + half_w + gap_middle + parallax_x, 30.0f));
        ImGui::BeginChild("RightPanel", ImVec2(half_w, full_h), true);
        {
            SceDateTime time;
            sceRtcGetCurrentClockLocalTime(&time);
            
            ImGui::SetCursorPosY(20);
            
            char timeStr[16];
            sprintf(timeStr, "%02d:%02d", sceRtcGetHour(&time), sceRtcGetMinute(&time));
            
            ImGui::SetWindowFontScale(5.0f); 
            CenterText(timeStr);
            ImGui::TextColored(ImVec4(1,1,1,1), "%s", timeStr);
            
            ImGui::SetWindowFontScale(1.2f);
            char dateStr[32];
            sprintf(dateStr, "%04d.%02d.%02d", sceRtcGetYear(&time), sceRtcGetMonth(&time), sceRtcGetDay(&time));
            CenterText(dateStr);
            ImGui::TextColored(ImVec4(0.7f,0.7f,0.8f,1), "%s", dateStr);
            
            ImGui::Dummy(ImVec2(0, 40));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 40));

            float btn_size = 80.0f;
            float spacing = (half_w - 30 - (btn_size * 3)) / 2;
            if(spacing < 5.0f) spacing = 5.0f; 
            
            ImGui::Indent(10);
            
            if (ImGui::Button("<<", ImVec2(btn_size, btn_size))) 
                send_cmd(&server_ctrl, ctrl_sock, COMMAND_PREV);
            
            ImGui::SameLine(0, spacing);
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.6f, 0.6f, 0.5f));
            if (ImGui::Button("|| >", ImVec2(btn_size, btn_size))) 
                send_cmd(&server_ctrl, ctrl_sock, COMMAND_TOGGLE);
            ImGui::PopStyleColor();

            ImGui::SameLine(0, spacing);
            
            if (ImGui::Button(">>", ImVec2(btn_size, btn_size))) 
                send_cmd(&server_ctrl, ctrl_sock, COMMAND_NEXT);
            
            ImGui::Unindent();

            ImGui::Dummy(ImVec2(0, 30));
            
            ImGui::TextDisabled("TARGET SERVER:");
            
            // --- 2 ROWS LAYOUT FOR IP ---
            DrawIPOctet("##ip1", &ip_parts[0]); 
            ImGui::SameLine(); 
            ImGui::Text("."); 
            ImGui::SameLine();
            DrawIPOctet("##ip2", &ip_parts[1]); 
            ImGui::SameLine();
            ImGui::Text("."); 
            
            ImGui::Dummy(ImVec2(0, 5)); // Spacing Row

            DrawIPOctet("##ip3", &ip_parts[2]); 
            ImGui::SameLine(); 
            ImGui::Text("."); 
            ImGui::SameLine();
            DrawIPOctet("##ip4", &ip_parts[3]);
            
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::TextDisabled("LINK: %s", server_ip);
        }
        ImGui::EndChild();

        ImGui::End();

        glViewport(0,0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        ImGui::Render();
        ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
        vglSwapBuffers(GL_FALSE);
    }

    return 0;
}
