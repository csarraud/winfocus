#pragma once
#include <string>
#include <vector>
#include <X11/Xlib.h>

struct ManagedWindow {
    Window xid;
    std::string real_title;
    std::string display_name;
    bool name_edited;

    ManagedWindow(Window id, const std::string& title)
        : xid(id), real_title(title), display_name(title), name_edited(false) {}
};

class X11WindowManager {
public:
    X11WindowManager();
    ~X11WindowManager();

    // Rafraîchit la liste des fenêtres dont le titre contient `keyword`
    void refresh(const std::string& keyword);

    // Donne le focus à la fenêtre par index dans la liste courante
    void focus_window(int index);

    // Déplace une fenêtre en tête de liste (raise)
    void raise_window(int index);

    // Passe à la fenêtre suivante dans la liste (focus cycle)
    void focus_next();

    // Renomme l'affichage d'une fenêtre (ne modifie pas X11)
    void rename_window(int index, const std::string& new_name);

    // Déplace une entrée dans la liste (réordonnancement)
    void move_window(int from, int to);

    // Sondage non-bloquant : retourne true si un clic molette global a été détecté.
    // À appeler à chaque frame depuis la boucle principale.
    bool poll_global_middle_click();

    const std::vector<ManagedWindow>& windows() const { return windows_; }
    int current_focus_index() const { return current_focus_; }

    bool is_valid() const { return display_ != nullptr; }
    std::string last_error() const { return last_error_; }

private:
    // Display principal (refresh, focus)
    Display* display_;
    Window   root_;

    // Display secondaire dédié au grab global de Button2
    Display* grab_display_;
    Window   grab_root_;

    std::vector<ManagedWindow> windows_;
    int current_focus_;
    std::string last_error_;

    std::string get_window_title(Window w);
    bool window_is_visible(Window w);

    void setup_global_grab();
    void teardown_global_grab();
};