#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "x11_windows.h"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <string>

// ─────────────────────────────────────────────────────────
//  Couleurs / style
// ─────────────────────────────────────────────────────────
static const ImVec4 COL_BG        = {0.10f, 0.11f, 0.14f, 1.0f};
static const ImVec4 COL_PANEL     = {0.14f, 0.15f, 0.19f, 1.0f};
static const ImVec4 COL_ACCENT    = {0.27f, 0.62f, 0.96f, 1.0f};
static const ImVec4 COL_FOCUS     = {0.20f, 0.70f, 0.52f, 1.0f};
static const ImVec4 COL_HOVER     = {0.22f, 0.24f, 0.30f, 1.0f};
static const ImVec4 COL_DRAG      = {0.40f, 0.55f, 0.90f, 0.35f};
static const ImVec4 COL_TEXT      = {0.90f, 0.92f, 0.96f, 1.0f};
static const ImVec4 COL_TEXT_DIM  = {0.55f, 0.58f, 0.65f, 1.0f};
static const ImVec4 COL_WARN      = {0.95f, 0.60f, 0.20f, 1.0f};

static void apply_style()
{
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding    = 8.0f;
    s.FrameRounding     = 5.0f;
    s.PopupRounding     = 5.0f;
    s.ScrollbarRounding = 5.0f;
    s.GrabRounding      = 4.0f;
    s.FramePadding      = {8, 5};
    s.ItemSpacing       = {8, 6};
    s.WindowPadding     = {12, 12};
    s.ScrollbarSize     = 10.0f;
    s.GrabMinSize       = 10.0f;
    s.WindowBorderSize  = 1.0f;
    s.FrameBorderSize   = 0.0f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]             = COL_BG;
    c[ImGuiCol_ChildBg]              = COL_PANEL;
    c[ImGuiCol_PopupBg]              = COL_PANEL;
    c[ImGuiCol_Border]               = {0.25f, 0.27f, 0.33f, 0.8f};
    c[ImGuiCol_FrameBg]              = {0.18f, 0.20f, 0.25f, 1.0f};
    c[ImGuiCol_FrameBgHovered]       = COL_HOVER;
    c[ImGuiCol_FrameBgActive]        = {0.25f, 0.28f, 0.35f, 1.0f};
    c[ImGuiCol_TitleBg]              = {0.08f, 0.09f, 0.12f, 1.0f};
    c[ImGuiCol_TitleBgActive]        = {0.10f, 0.12f, 0.16f, 1.0f};
    c[ImGuiCol_MenuBarBg]            = COL_PANEL;
    c[ImGuiCol_ScrollbarBg]          = {0.08f, 0.09f, 0.12f, 0.5f};
    c[ImGuiCol_ScrollbarGrab]        = {0.30f, 0.32f, 0.40f, 1.0f};
    c[ImGuiCol_ScrollbarGrabHovered] = {0.40f, 0.43f, 0.52f, 1.0f};
    c[ImGuiCol_ScrollbarGrabActive]  = COL_ACCENT;
    c[ImGuiCol_CheckMark]            = COL_ACCENT;
    c[ImGuiCol_SliderGrab]           = COL_ACCENT;
    c[ImGuiCol_SliderGrabActive]     = {0.40f, 0.75f, 1.00f, 1.0f};
    c[ImGuiCol_Button]               = {0.20f, 0.22f, 0.28f, 1.0f};
    c[ImGuiCol_ButtonHovered]        = {0.25f, 0.45f, 0.75f, 1.0f};
    c[ImGuiCol_ButtonActive]         = COL_ACCENT;
    c[ImGuiCol_Header]               = {0.20f, 0.38f, 0.65f, 0.6f};
    c[ImGuiCol_HeaderHovered]        = {0.24f, 0.50f, 0.80f, 0.7f};
    c[ImGuiCol_HeaderActive]         = COL_ACCENT;
    c[ImGuiCol_Separator]            = {0.22f, 0.24f, 0.30f, 1.0f};
    c[ImGuiCol_DragDropTarget]       = COL_DRAG;
    c[ImGuiCol_Text]                 = COL_TEXT;
    c[ImGuiCol_TextDisabled]         = COL_TEXT_DIM;
    c[ImGuiCol_Tab]                  = {0.14f, 0.15f, 0.19f, 1.0f};
    c[ImGuiCol_TabHovered]           = {0.22f, 0.40f, 0.68f, 1.0f};
    c[ImGuiCol_TabActive]            = {0.18f, 0.35f, 0.60f, 1.0f};
}

// ─────────────────────────────────────────────────────────
//  Variables globales de l'interface
// ─────────────────────────────────────────────────────────
static char  g_keyword[256]       = "";
static bool  g_auto_refresh       = true;
static float g_refresh_interval   = 1.0f;
static float g_refresh_timer      = 0.0f;
static int   g_drag_src           = -1;
static int   g_rename_idx         = -1;
static char  g_rename_buf[256]    = "";
static bool  g_rename_focus_set   = false;

static X11WindowManager* g_wm = nullptr;

static void mouse_button_callback(GLFWwindow* /*win*/, int button, int action, int /*mods*/)
{
    if (!g_wm) return;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
        g_wm->focus_next();
}

// ─────────────────────────────────────────────────────────
//  Rendu principal
// ─────────────────────────────────────────────────────────
static void render_ui(X11WindowManager& wm, float dt)
{
    if (g_auto_refresh) {
        g_refresh_timer += dt;
        if (g_refresh_timer >= g_refresh_interval) {
            g_refresh_timer = 0.0f;
            wm.refresh(std::string(g_keyword));
        }
    }

    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##main", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoScrollbar|
        ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        ImGui::TextColored(COL_ACCENT, "WinFocus");
        ImGui::SameLine();
        ImGui::TextColored(COL_TEXT_DIM, "│");
        ImGui::SameLine();
        ImGui::TextColored(COL_TEXT_DIM, "Gestionnaire de fenêtres X11");
        ImGui::EndMenuBar();
    }

    // ── Barre de recherche ────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.12f,0.13f,0.17f,1.0f});
    ImGui::BeginChild("##toolbar", {0, 52}, false);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);

    ImGui::SetNextItemWidth(220);
    ImGui::InputTextWithHint("##kw", "Mot-clé de filtre…", g_keyword, sizeof(g_keyword));
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4{0.20f,0.45f,0.75f,1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.28f,0.58f,0.90f,1.0f});
    if (ImGui::Button("  ↺  Rafraîchir  ")) {
        wm.refresh(std::string(g_keyword));
        g_refresh_timer = 0.0f;
    }
    ImGui::PopStyleColor(2);
    ImGui::SameLine();

    ImGui::Checkbox("Auto", &g_auto_refresh);
    if (g_auto_refresh) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::SliderFloat("##interval", &g_refresh_interval, 1.0f, 30.0f, "%.0fs");
    }

    ImGui::SameLine();
    ImGui::TextColored(COL_TEXT_DIM, "  %d fenêtre(s) trouvée(s)", (int)wm.windows().size());

    if (!wm.is_valid()) {
        ImGui::SameLine();
        ImGui::TextColored(COL_WARN, "  ⚠ X11: %s", wm.last_error().c_str());
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    // ── Liste des fenêtres ────────────────────────────────
    ImGui::BeginChild("##winlist", {0, 0}, false);

    const auto& wins = wm.windows();

    if (wins.empty()) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 40);
        float w = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((w - 280.0f) * 0.5f);
        ImGui::TextColored(COL_TEXT_DIM, "Aucune fenêtre — tapez un mot-clé et cliquez Rafraîchir.");
    }

    int swap_from = -1, swap_to = -1;
    int n = (int)wins.size();

    for (int i = 0; i < n; ++i) {
        const auto& mw = wins[i];
        bool is_focus  = (i == wm.current_focus_index());

        ImGui::PushID(i);

        const float HANDLE_W    = 28.0f;
        const float NUM_W       = 28.0f;
        const float BTN_FOCUS_W = 76.0f;
        const float BTN_EDIT_W  = 26.0f;
        const float GAP         = 6.0f;
        const float ROW_H       = 48.0f;
        const float BTN_H       = 28.0f;

        ImVec2 row_pos = ImGui::GetCursorScreenPos();
        float  row_w   = ImGui::GetContentRegionAvail().x;

        float btn_area = BTN_FOCUS_W + GAP + BTN_EDIT_W;
        float name_x   = row_pos.x + HANDLE_W + NUM_W;
        float name_w   = row_w - HANDLE_W - NUM_W - btn_area - GAP * 3;
        if (name_w < 60.0f) name_w = 60.0f;
        float btn_y    = row_pos.y + (ROW_H - BTN_H) * 0.5f;
        float btn_x    = row_pos.x + row_w - btn_area - GAP;

        if (is_focus) {
            ImGui::GetWindowDrawList()->AddRectFilled(
                row_pos, {row_pos.x + row_w, row_pos.y + ROW_H},
                IM_COL32(36, 90, 70, 80), 6.0f);
            ImGui::GetWindowDrawList()->AddRect(
                row_pos, {row_pos.x + row_w, row_pos.y + ROW_H},
                IM_COL32(52, 178, 120, 160), 6.0f, 0, 1.5f);
        }

        // ── 1. Drag handle ────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {1,1,1,0.07f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {1,1,1,0.12f});
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {4, (ROW_H - 16) * 0.5f});
        ImGui::Button(":::", {HANDLE_W, ROW_H});
        if (ImGui::IsItemActive()) g_drag_src = i;
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload("WIN_IDX", &i, sizeof(i));
            ImGui::TextColored(COL_ACCENT, "  %s", mw.display_name.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        // ── 2. Zone drop ──────────────────────────────────
        ImGui::SetCursorScreenPos({row_pos.x + HANDLE_W, row_pos.y});
        ImGui::InvisibleButton("##drop_zone", {name_x - row_pos.x - HANDLE_W + name_w, ROW_H});
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("WIN_IDX")) {
                swap_from = *(const int*)payload->Data;
                swap_to   = i;
            }
            ImGui::EndDragDropTarget();
        }

        // ── 3. Numéro ─────────────────────────────────────
        ImGui::SetCursorScreenPos({row_pos.x + HANDLE_W + 2, btn_y + 5});
        ImGui::TextColored(is_focus ? COL_FOCUS : COL_TEXT_DIM, "%2d", i + 1);

        // ── 4. Nom / champ d'édition ──────────────────────
        ImGui::SetCursorScreenPos({name_x, btn_y});
        if (g_rename_idx == i) {
            if (!g_rename_focus_set) {
                ImGui::SetKeyboardFocusHere();
                g_rename_focus_set = true;
            }
            ImGui::SetNextItemWidth(name_w);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.20f, 0.30f, 0.50f, 1.0f});
            bool confirmed = ImGui::InputText("##rename", g_rename_buf, sizeof(g_rename_buf),
                ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopStyleColor();
            if (confirmed || ImGui::IsItemDeactivated()) {
                if (strlen(g_rename_buf) > 0)
                    wm.rename_window(i, std::string(g_rename_buf));
                g_rename_idx       = -1;
                g_rename_focus_set = false;
            }
        } else {
            ImGui::InvisibleButton("##name_btn", {name_w, BTN_H});
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                g_rename_idx = i;
                strncpy(g_rename_buf, wins[i].display_name.c_str(), sizeof(g_rename_buf)-1);
                g_rename_buf[sizeof(g_rename_buf)-1] = '\0';
                g_rename_focus_set = false;
            }
            ImU32 name_col = is_focus ? IM_COL32(82,200,160,255) : IM_COL32(220,224,235,255);
            ImGui::GetWindowDrawList()->AddText({name_x, btn_y + 6}, name_col,
                wins[i].display_name.c_str());
            if (wins[i].name_edited) {
                ImVec2 ts = ImGui::CalcTextSize(wins[i].display_name.c_str());
                ImGui::GetWindowDrawList()->AddText(
                    {name_x + ts.x + 6, btn_y + 6},
                    IM_COL32(120, 125, 140, 200), "(modifié)");
            }
        }

        // ── 5. Bouton Focus ───────────────────────────────
        ImGui::SetCursorScreenPos({btn_x, btn_y});
        if (is_focus) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.20f, 0.60f, 0.45f, 1.0f});
            ImGui::Button("Focus", {BTN_FOCUS_W, BTN_H});
            ImGui::PopStyleColor();
        } else {
            if (ImGui::Button("  Focus  ", {BTN_FOCUS_W, BTN_H}))
                wm.focus_window(i);
        }

        // ── 6. Bouton Renommer ────────────────────────────
        ImGui::SetCursorScreenPos({btn_x + BTN_FOCUS_W + GAP, btn_y});
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.18f, 0.20f, 0.28f, 1.0f});
        if (ImGui::Button("...", {BTN_EDIT_W, BTN_H})) {
            g_rename_idx = i;
            strncpy(g_rename_buf, wins[i].display_name.c_str(), sizeof(g_rename_buf)-1);
            g_rename_buf[sizeof(g_rename_buf)-1] = '\0';
            g_rename_focus_set = false;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Double-clic sur le nom pour renommer");
        ImGui::PopStyleColor();

        ImGui::SetCursorScreenPos({row_pos.x, row_pos.y + ROW_H});
        ImGui::Separator();

        ImGui::PopID();
    }

    if (swap_from >= 0 && swap_to >= 0 && swap_from != swap_to)
        wm.move_window(swap_from, swap_to);

    ImGui::EndChild();
    ImGui::End();
}

// ─────────────────────────────────────────────────────────
//  Main
// ─────────────────────────────────────────────────────────
int main()
{
    if (!glfwInit()) {
        fprintf(stderr, "Erreur: impossible d'initialiser GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(680, 540, "WinFocus — Gestionnaire de fenêtres", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "Erreur: impossible de créer la fenêtre GLFW\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    X11WindowManager wm;
    g_wm = &wm;
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImFontConfig fc;
    fc.OversampleH = 2;
    fc.OversampleV = 2;
    ImFont* font = io.Fonts->AddFontFromFileTTF(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 15.0f, &fc);
    if (!font)
        io.Fonts->AddFontDefault();

    apply_style();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    wm.refresh(std::string(g_keyword));

    double last_time = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double now = glfwGetTime();
        float  dt  = (float)(now - last_time);
        last_time  = now;

        // Clic molette global : cycle uniquement si le clic est dans une fenêtre gérée
        if (wm.poll_global_middle_click())
            wm.focus_next();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        render_ui(wm, dt);

        ImGui::Render();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(COL_BG.x, COL_BG.y, COL_BG.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}