#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/audioin.h>
#include <psp2/net/net.h>
#include <psp2/sysmodule.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h> 
#include <psp2/motion.h>
#include <psp2/rtc.h>
#include <psp2/ctrl.h>  
#include <psp2/power.h> 

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
#define CONFIG_PATH "ux0:data/alter-ego/config.txt"
#define CONFIG_DIR  "ux0:data/alter-ego"

#define SAMPLE_RATE 16000
#define GRAIN 256
#define EQ_BANDS 32
#define PI 3.1415926535f

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

float lerpf(float a, float b, float t) { return a + (b - a) * t; }
float clampf(float v, float mn, float mx) { return (v < mn) ? mn : (v > mx) ? mx : v; }

float random_at(int i) {
    return (float)(((unsigned int)i * 1234567) % 1000) / 1000.0f;
}

void send_cmd(SceNetSockaddrIn* addr, int sock, const char* cmd) {
    sceNetSendto(sock, cmd, strlen(cmd), 0, (SceNetSockaddr*)addr, sizeof(*addr));
}

// Config Management
void LoadConfig(int* ip, float* hue, int* sens) {
    FILE* f = fopen(CONFIG_PATH, "r");
    if (f) {
        // Line 1: IP
        fscanf(f, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
        // Line 2: Hue Offset (Float)
        if(fscanf(f, "%f", hue) != 1) *hue = 0.0f;
        // Line 3: Sensitivity (Int)
        if(fscanf(f, "%d", sens) != 1) *sens = 4;
        
        fclose(f);
    }
}

void SaveConfig(int* ip, float hue, int sens) {
    sceIoMkdir(CONFIG_DIR, 0777); 
    FILE* f = fopen(CONFIG_PATH, "w");
    if (f) {
        fprintf(f, "%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
        fprintf(f, "%.3f\n", hue);
        fprintf(f, "%d\n", sens);
        fclose(f);
    }
}


struct Theme {
    ImVec4 accent_primary;
    ImVec4 accent_secondary;
    ImVec4 bg_color;
    ImVec4 panel_bg;
} g_theme;

void UpdateTheme(float hue_shift) {
    float h1 = fmodf(0.5f + hue_shift, 1.0f); 
    float h2 = fmodf(0.9f + hue_shift, 1.0f);
    
    ImGui::ColorConvertHSVtoRGB(h1, 1.0f, 1.0f, g_theme.accent_primary.x, g_theme.accent_primary.y, g_theme.accent_primary.z);
    g_theme.accent_primary.w = 1.0f;

    ImGui::ColorConvertHSVtoRGB(h2, 1.0f, 1.0f, g_theme.accent_secondary.x, g_theme.accent_secondary.y, g_theme.accent_secondary.z);
    g_theme.accent_secondary.w = 1.0f;
    
    g_theme.bg_color = ImVec4(0.08f, 0.08f, 0.12f, 1.0f);
    g_theme.panel_bg = ImVec4(0.12f, 0.12f, 0.18f, 0.95f); 
}

void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.Colors[ImGuiCol_WindowBg] = g_theme.bg_color;
    style.Colors[ImGuiCol_ChildBg]  = g_theme.panel_bg; 
    style.Colors[ImGuiCol_Border]   = g_theme.accent_primary;
    style.Colors[ImGuiCol_Text]     = ImVec4(0.9f, 0.9f, 0.95f, 1.0f);
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

void DrawArcReactor(float* values, int count, float radius, ImVec2 center) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    
    draw->AddCircle(center, radius, ImGui::GetColorU32(g_theme.accent_primary), 64, 2.0f);
    draw->AddCircle(center, radius * 0.3f, ImGui::GetColorU32(g_theme.accent_secondary), 32, 2.0f);

    for (int i = 0; i < count; i++) {
        float val = clampf(values[i], 0.0f, 1.0f);
        float angle_step = (2.0f * PI) / count;
        float angle = i * angle_step - (PI / 2.0f);
        
        float r_inner = radius * 0.35f;
        float r_outer = r_inner + (val * (radius * 0.6f));
        
        ImVec2 p1 = ImVec2(center.x + cosf(angle) * r_inner, center.y + sinf(angle) * r_inner);
        ImVec2 p2 = ImVec2(center.x + cosf(angle) * r_outer, center.y + sinf(angle) * r_outer);
        
        ImU32 col = (val > 0.8f) ? ImGui::GetColorU32(g_theme.accent_secondary) : 
                                   ImGui::GetColorU32(g_theme.accent_primary);
        draw->AddLine(p1, p2, col, 3.0f);
    }
}

bool VectorButton(const char* id, int shape_type, float size) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    
    bool pressed = ImGui::InvisibleButton(id, ImVec2(size, size));
    
    ImU32 col = ImGui::IsItemHovered() ? ImGui::GetColorU32(g_theme.accent_secondary) : ImGui::GetColorU32(g_theme.accent_primary);
    if (ImGui::IsItemActive()) col = 0xFFFFFFFF;

    float pad = size * 0.25f;
    draw->AddRect(p, ImVec2(p.x + size, p.y + size), col, 8.0f, 0, 2.0f);

    if (shape_type == 0) { // PREV
        float mid_y = p.y + size/2;
        float l = p.x + pad;
        float r = p.x + size - pad;
        float mid_x = p.x + size/2;
        draw->AddTriangleFilled(ImVec2(mid_x, p.y+pad), ImVec2(mid_x, p.y+size-pad), ImVec2(l, mid_y), col);
        draw->AddTriangleFilled(ImVec2(r, p.y+pad), ImVec2(r, p.y+size-pad), ImVec2(mid_x, mid_y), col);
    } 
    else if (shape_type == 1) { // PLAY
        draw->AddTriangleFilled(ImVec2(p.x+pad, p.y+pad), ImVec2(p.x+pad, p.y+size-pad), ImVec2(p.x+size-pad, p.y+size/2), col);
    }
    else if (shape_type == 2) { // NEXT
        float mid_y = p.y + size/2;
        float l = p.x + pad;
        float r = p.x + size - pad;
        float mid_x = p.x + size/2;
        draw->AddTriangleFilled(ImVec2(l, p.y+pad), ImVec2(l, p.y+size-pad), ImVec2(mid_x, mid_y), col);
        draw->AddTriangleFilled(ImVec2(mid_x, p.y+pad), ImVec2(mid_x, p.y+size-pad), ImVec2(r, mid_y), col);
    }
    return pressed;
}

void DrawBackgroundWindow(float time, float yaw_offset, float pitch_offset, float audio_energy, float warp_speed) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowBgAlpha(0.0f); 
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoScrollbar | 
                             ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoInputs | 
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("Background", NULL, flags);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 size = io.DisplaySize;
    
    ImU32 col_bg = 0xFF100510; 
    draw_list->AddRectFilled(ImVec2(0,0), size, col_bg);

    // Horizon moves with Pitch
    float horizon_base = size.y * 0.3f;
    float horizon = horizon_base + (pitch_offset * 150.0f); 
    horizon = clampf(horizon, size.y * 0.1f, size.y * 0.8f);

    ImVec2 sun_pos = ImVec2(size.x * 0.5f, horizon);

    // --- STARS ---
    for(int i = 0; i < 60; i++) {
        float sx = random_at(i * 12) * size.x;
        float sy = random_at(i * 34) * size.y; 

        sx = fmodf(sx - (yaw_offset * 100.0f), size.x);
        if(sx < 0) sx += size.x;
        
        sy = fmodf(sy + (pitch_offset * 50.0f), size.y);
        if(sy < 0) sy += size.y;

        float dx = sx - sun_pos.x;
        float dy = sy - sun_pos.y;
        float dist = sqrtf(dx*dx + dy*dy);
        if(dist < 5.0f) dist = 5.0f; 

        float dir_x = dx / dist;
        float dir_y = dy / dist;

        float length = 1.5f;
        if(warp_speed > 1.0f) length = 5.0f + (warp_speed * 15.0f); 
        if(warp_speed < 0.1f) length = 1.0f;

        ImVec2 p1 = ImVec2(sx, sy);
        ImVec2 p2 = ImVec2(sx + (dir_x * length), sy + (dir_y * length));

        draw_list->AddLine(p1, p2, ImGui::GetColorU32(g_theme.accent_secondary), 1.5f);
    }

    // --- SUN ---
    float sun_radius = 80.0f;
    ImU32 sun_yellow = IM_COL32(255, 220, 0, 255);
    ImU32 sun_red    = IM_COL32(255, 0, 80, 255);
    ImU32 sun_violet = IM_COL32(140, 0, 255, 255);

    for (float y = -sun_radius; y < sun_radius; y += 1.0f) {
        float y_norm = y + sun_radius; 
        if ((int)y_norm % 8 >= 6) continue;

        float width = sqrtf(sun_radius*sun_radius - y*y);
        float t = (y + sun_radius) / (sun_radius * 2.0f); 
        ImU32 line_color = (t < 0.5f) ? LerpColor(sun_yellow, sun_red, t * 2.0f) : LerpColor(sun_red, sun_violet, (t - 0.5f) * 2.0f);

        draw_list->AddLine(
            ImVec2(sun_pos.x - width, sun_pos.y + y - 20.0f), 
            ImVec2(sun_pos.x + width, sun_pos.y + y - 20.0f), 
            line_color
        );
    }

    // --- GRID ---
    float spacing = 120.0f;
    ImU32 grid_col = ImGui::GetColorU32(g_theme.accent_primary);
    int alpha = 40 + (int)(clampf(audio_energy, 0, 1) * 150);
    grid_col = (grid_col & 0x00FFFFFF) | (alpha << 24);

    for(int i = -120; i < 120; i++) {
        float x_base = (i * spacing) - (yaw_offset * 300.0f); 
        
        while(x_base < -size.x * 2) x_base += (240*spacing);
        while(x_base > size.x * 3)  x_base -= (240*spacing);
        
        if(x_base > -3000.0f && x_base < size.x + 3000.0f) {
            draw_list->AddLine(
                ImVec2(size.x/2 + (x_base - size.x/2) * 0.2f, horizon), 
                ImVec2(x_base, size.y), 
                grid_col, 2.0f
            );
        }
    }
    
    float offset = fmodf(time * 20.0f * warp_speed, 20.0f);
    for(float y = horizon; y < size.y; y += (y - horizon) * 0.05f + 3.0f + offset * 0.05f) {
        if(y > size.y) break;
        draw_list->AddLine(ImVec2(0, y), ImVec2(size.x, y), grid_col, 2.0f);
    }

    ImGui::End();
}

void DrawIPOctetCombo(const char* label_id, int* value) {
    char preview_val[8];
    sprintf(preview_val, "%d", *value);
    
    ImGui::PushItemWidth(60);
    if (ImGui::BeginCombo(label_id, preview_val, ImGuiComboFlags_HeightLarge)) { 
        for (int i = 0; i <= 255; i++) {
            bool is_selected = (*value == i);
            char item_label[8];
            sprintf(item_label, "%d", i);
            
            if (ImGui::Selectable(item_label, is_selected)) {
                *value = i;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
}

int main(int, char**) {
    vglInitExtended(0, 960, 544, 0x1800000, SCE_GXM_MULTISAMPLE_4X);
    ImGui::CreateContext();
    ImGui_ImplVitaGL_Init();
    ImGui_ImplVitaGL_TouchUsage(true);
    ImGui_ImplVitaGL_GamepadUsage(true); 

    SetupStyle();
    
    sceMotionStartSampling();
    sceMotionMagnetometerOn();
    sceMotionSetTiltCorrection(1);
    
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
    int sensitivity = 4;
    float hue_offset = 0.0f;
    
    LoadConfig(ip_parts, &hue_offset, &sensitivity); 

    char server_ip[MAX_IP_LEN]; 
    SceNetSockaddrIn server_audio{}, server_ctrl{};

    short* audioIn = (short*)malloc(sizeof(short)*GRAIN);
    int port = sceAudioInOpenPort(SCE_AUDIO_IN_PORT_TYPE_VOICE, GRAIN, SAMPLE_RATE, SCE_AUDIO_IN_PARAM_FORMAT_S16_MONO);
    
    bool mic_enabled = true;
    float eq_values[EQ_BANDS] = {0};
    
    float anim_time = 0.0f;
    float avg_energy = 0.0f;
    
    float smooth_yaw = 0.0f;
    float smooth_pitch = 0.0f;

    float yaw_ref = 0.0f;
    float pitch_ref = 0.0f;
    bool request_calibration = false;

    SceCtrlData ctrl_data;
    float warp_speed = 1.0f;

    ImGuiIO& io = ImGui::GetIO();

    while (1) {
        sceCtrlPeekBufferPositive(0, &ctrl_data, 1);
        
        float ry = (ctrl_data.ry - 128.0f) / 128.0f; 
        if (fabsf(ry) > 0.1f) {
            warp_speed = lerpf(warp_speed, 1.0f - (ry * 2.0f), 0.1f);
            if(warp_speed < 0.0f) warp_speed = 0.0f;
        } else {
            warp_speed = lerpf(warp_speed, 1.0f, 0.05f);
        }

        float rx = (ctrl_data.rx - 128.0f) / 128.0f;
        if (fabsf(rx) > 0.1f) {
            hue_offset += (rx * 0.01f);
            if(hue_offset > 1.0f) hue_offset -= 1.0f;
            if(hue_offset < 0.0f) hue_offset += 1.0f;
        }

        anim_time += (1.0f / 60.0f) * warp_speed;
        UpdateTheme(hue_offset);
        SetupStyle(); 

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

        float frame_energy = 0.0f;
        for (int b = 0; b < EQ_BANDS; b++) {
            int start = (GRAIN/EQ_BANDS)*b;
            int end   = start + (GRAIN/EQ_BANDS);
            float sum = 0.0f;
            for (int i = start; i < end; i++) sum += fabsf(audioIn[i] / 32768.0f);
            float level = clampf(sum * sensitivity, 0, 1);
            eq_values[b] = lerpf(eq_values[b], level, 0.2f);
            frame_energy += level;
        }
        avg_energy = lerpf(avg_energy, frame_energy / EQ_BANDS, 0.1f);

        // GYRO CALCULATION & CALIBRATION
        SceMotionState motion_state;
        sceMotionGetState(&motion_state);
        
        float current_raw_yaw = atan2f(motion_state.nedMatrix.y.x, motion_state.nedMatrix.x.x);
        float current_raw_pitch = atan2f(motion_state.nedMatrix.z.y, motion_state.nedMatrix.z.z);

        if (request_calibration) {
            yaw_ref = current_raw_yaw;
            pitch_ref = current_raw_pitch;
            request_calibration = false;
        }

        float display_yaw = current_raw_yaw - yaw_ref;
        float display_pitch = current_raw_pitch - pitch_ref;

        smooth_yaw = lerpf(smooth_yaw, display_yaw, 0.1f);
        smooth_pitch = lerpf(smooth_pitch, display_pitch, 0.1f);

        ImGui_ImplVitaGL_NewFrame();
        
        DrawBackgroundWindow(anim_time, smooth_yaw, smooth_pitch, avg_energy, warp_speed);

        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGuiWindowFlags main_flags = ImGuiWindowFlags_NoTitleBar | 
                                      ImGuiWindowFlags_NoResize | 
                                      ImGuiWindowFlags_NoMove | 
                                      ImGuiWindowFlags_NoScrollbar;

        ImGui::Begin("MainLayout", NULL, main_flags);

        float parallax_x = clampf(smooth_yaw * -20.0f, -60.0f, 60.0f);
        float parallax_y = clampf(smooth_pitch * 30.0f, -40.0f, 40.0f);
        
        float margin = 80.0f;
        float half_w = (io.DisplaySize.x - (margin * 2)) / 2;

        // LEFT PANEL
        ImGui::SetCursorPos(ImVec2(margin + parallax_x, 30.0f + parallax_y));
        ImGui::BeginChild("LeftPanel", ImVec2(half_w, 480), true);
        {
            ImGui::TextColored(g_theme.accent_secondary, "ALTER EGO v4.0");
            ImGui::Separator();
            
            ImGui::Dummy(ImVec2(0, 20));
            ImVec2 center_screen = ImGui::GetCursorScreenPos();
            center_screen.x += half_w * 0.5f;
            center_screen.y += 120.0f;
            
            DrawArcReactor(eq_values, EQ_BANDS, 90.0f, center_screen);
            ImGui::Dummy(ImVec2(0, 240)); 

            ImGui::PushItemWidth(-1);
            if (ImGui::BeginCombo("##Cmds", "Execute Command...")) {
                for (int i=0;i<command_count;i++) {
                    if (ImGui::Selectable(commands[i])) send_cmd(&server_ctrl, ctrl_sock, commands[i]);
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            
            ImGui::Dummy(ImVec2(0,10));
            
            if (ImGui::CollapsingHeader("SYSTEM CONFIG")) {
                ImGui::Indent();
                ImGui::Text("Server IP:");
                DrawIPOctetCombo("##1", &ip_parts[0]); ImGui::SameLine(); 
                DrawIPOctetCombo("##2", &ip_parts[1]); ImGui::SameLine(); 
                DrawIPOctetCombo("##3", &ip_parts[2]); ImGui::SameLine(); 
                DrawIPOctetCombo("##4", &ip_parts[3]);
                
                ImGui::Dummy(ImVec2(0,5));
                if (ImGui::Button("SAVE CONFIG", ImVec2(-1, 0))) {
                    SaveConfig(ip_parts, hue_offset, sensitivity);
                }

                ImGui::Dummy(ImVec2(0,5));
                if (ImGui::Button("RECALIBRATE GYRO", ImVec2(-1, 0))) {
                    request_calibration = true;
                }

                ImGui::Dummy(ImVec2(0,5));
                ImGui::Text("Audio Gain:");
                ImGui::SliderInt("##sens", &sensitivity, 1, 10);
                ImGui::Unindent();
            }

            ImGui::Dummy(ImVec2(0,10));
            ImGui::Checkbox("TX AUDIO", &mic_enabled);
            if(mic_enabled && (int)(anim_time * 4) % 2 == 0) {
                ImGui::SameLine(); ImGui::TextColored(ImVec4(1,0,0,1), " [LIVE]");
            }
        }
        ImGui::EndChild();

        // RIGHT PANEL
        ImGui::SetCursorPos(ImVec2(margin + half_w + 20 + parallax_x, 30.0f + parallax_y));
        ImGui::BeginChild("RightPanel", ImVec2(half_w, 480), true);
        {
            int batt_percent = scePowerGetBatteryLifePercent();
            int wifi_bars = 3 + (rand() % 2); 
            
            ImGui::BeginGroup();
            {
                ImGui::TextColored(g_theme.accent_primary, "BATT: %d%%", batt_percent);
                ImGui::SameLine(half_w - 120);
                ImGui::TextColored(g_theme.accent_primary, "WIFI: ");
                for(int w=0; w<4; w++) {
                    ImGui::SameLine();
                    ImGui::TextColored(w < wifi_bars ? g_theme.accent_secondary : ImVec4(0.3f,0.3f,0.3f,1), "|");
                }
            }
            ImGui::EndGroup();
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 20));

            SceDateTime time;
            sceRtcGetCurrentClockLocalTime(&time);
            char timeStr[16];
            sprintf(timeStr, "%02d:%02d", sceRtcGetHour(&time), sceRtcGetMinute(&time));
            
            ImGui::SetWindowFontScale(5.0f);
            ImGui::SetCursorPosX((half_w - ImGui::CalcTextSize(timeStr).x) * 0.5f);
            ImGui::TextColored(ImVec4(1,1,1,1), "%s", timeStr);
            ImGui::SetWindowFontScale(1.0f);

            ImGui::Dummy(ImVec2(0, 40));

            float btn_size = 70.0f;
            float spacing = (half_w - (btn_size * 3)) / 4.0f;
            
            ImGui::SetCursorPosX(spacing);
            if (VectorButton("Prev", 0, btn_size)) send_cmd(&server_ctrl, ctrl_sock, COMMAND_PREV);
            
            ImGui::SameLine(0, spacing);
            if (VectorButton("Play", 1, btn_size)) send_cmd(&server_ctrl, ctrl_sock, COMMAND_TOGGLE);
            
            ImGui::SameLine(0, spacing);
            if (VectorButton("Next", 2, btn_size)) send_cmd(&server_ctrl, ctrl_sock, COMMAND_NEXT);
            
            ImGui::Dummy(ImVec2(0, 40));
            ImGui::TextDisabled("STATUS:");
            ImGui::Text("Warp: %.1fx  |  Hue: %.2f", warp_speed, hue_offset);
            ImGui::Text("Link: %s", server_ip);
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
