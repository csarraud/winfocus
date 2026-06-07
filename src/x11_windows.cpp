#include "x11_windows.h"
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <algorithm>
#include <cstring>
#include <iostream>

X11WindowManager::X11WindowManager()
    : display_(nullptr), root_(0),
      grab_display_(nullptr), grab_root_(0),
      current_focus_(-1)
{
    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        last_error_ = "Impossible d'ouvrir le display X11. "
                      "Vérifiez que $DISPLAY est défini.";
        return;
    }
    root_ = DefaultRootWindow(display_);
    setup_global_grab();
}

X11WindowManager::~X11WindowManager()
{
    teardown_global_grab();
    if (display_) {
        XCloseDisplay(display_);
        display_ = nullptr;
    }
}

std::string X11WindowManager::get_window_title(Window w)
{
    Atom net_wm_name = XInternAtom(display_, "_NET_WM_NAME", False);
    Atom utf8_string  = XInternAtom(display_, "UTF8_STRING",  False);

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char* prop = nullptr;

    int ret = XGetWindowProperty(
        display_, w, net_wm_name,
        0, 1024, False, utf8_string,
        &actual_type, &actual_format,
        &nitems, &bytes_after, &prop);

    if (ret == Success && prop && nitems > 0) {
        std::string title(reinterpret_cast<char*>(prop));
        XFree(prop);
        return title;
    }
    if (prop) XFree(prop);

    char* name = nullptr;
    if (XFetchName(display_, w, &name) && name) {
        std::string title(name);
        XFree(name);
        return title;
    }

    return "(sans titre)";
}

bool X11WindowManager::window_is_visible(Window w)
{
    XWindowAttributes attrs;
    if (XGetWindowAttributes(display_, w, &attrs) == 0)
        return false;
    return attrs.map_state == IsViewable;
}

void X11WindowManager::refresh(const std::string& keyword)
{
    if (!display_) return;

    Atom net_client_list = XInternAtom(display_, "_NET_CLIENT_LIST", False);
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char* prop = nullptr;

    int ret = XGetWindowProperty(
        display_, root_, net_client_list,
        0, 1024, False, XA_WINDOW,
        &actual_type, &actual_format,
        &nitems, &bytes_after, &prop);

    std::vector<Window> all_windows;

    if (ret == Success && prop) {
        Window* wins = reinterpret_cast<Window*>(prop);
        for (unsigned long i = 0; i < nitems; ++i)
            all_windows.push_back(wins[i]);
        XFree(prop);
    } else {
        Window parent;
        Window* children = nullptr;
        unsigned int nchildren;
        if (XQueryTree(display_, root_, &root_, &parent, &children, &nchildren)) {
            for (unsigned int i = 0; i < nchildren; ++i)
                all_windows.push_back(children[i]);
            if (children) XFree(children);
        }
    }

    std::vector<ManagedWindow> old_windows = windows_;
    windows_.clear();

    for (Window w : all_windows) {
        if (!window_is_visible(w)) continue;

        std::string title = get_window_title(w);
        if (title.empty()) continue;

        if (!keyword.empty()) {
            if (title.find(keyword) == std::string::npos)
                continue;
        }

        ManagedWindow mw(w, title);
        for (const auto& old : old_windows) {
            if (old.xid == w && old.name_edited) {
                mw.display_name = old.display_name;
                mw.name_edited   = true;
                break;
            }
        }
        windows_.push_back(mw);
    }

    if (current_focus_ >= static_cast<int>(windows_.size()))
        current_focus_ = windows_.empty() ? -1 : 0;
}

void X11WindowManager::focus_window(int index)
{
    if (!display_ || index < 0 || index >= static_cast<int>(windows_.size()))
        return;

    Window w = windows_[index].xid;
    current_focus_ = index;

    Atom net_active = XInternAtom(display_, "_NET_ACTIVE_WINDOW", False);
    if (net_active != None) {
        XEvent ev;
        memset(&ev, 0, sizeof(ev));
        ev.type                 = ClientMessage;
        ev.xclient.window       = w;
        ev.xclient.message_type = net_active;
        ev.xclient.format       = 32;
        ev.xclient.data.l[0]    = 2;
        ev.xclient.data.l[1]    = CurrentTime;
        ev.xclient.data.l[2]    = 0;
        XSendEvent(display_, root_, False,
                   SubstructureNotifyMask | SubstructureRedirectMask, &ev);
    }

    XRaiseWindow(display_, w);
    XSetInputFocus(display_, w, RevertToPointerRoot, CurrentTime);
    XFlush(display_);
}

void X11WindowManager::raise_window(int index)
{
    if (!display_ || index < 0 || index >= static_cast<int>(windows_.size()))
        return;
    XRaiseWindow(display_, windows_[index].xid);
    XFlush(display_);
}

void X11WindowManager::focus_next()
{
    if (windows_.empty()) return;
    int next = (current_focus_ + 1) % static_cast<int>(windows_.size());
    focus_window(next);
}

void X11WindowManager::rename_window(int index, const std::string& new_name)
{
    if (index < 0 || index >= static_cast<int>(windows_.size())) return;
    windows_[index].display_name = new_name;
    windows_[index].name_edited  = true;
}

void X11WindowManager::move_window(int from, int to)
{
    if (from < 0 || to < 0) return;
    int n = static_cast<int>(windows_.size());
    if (from >= n || to >= n || from == to) return;

    ManagedWindow mw = windows_[from];
    windows_.erase(windows_.begin() + from);
    windows_.insert(windows_.begin() + to, mw);

    if (current_focus_ == from) {
        current_focus_ = to;
    } else if (from < current_focus_ && to >= current_focus_) {
        current_focus_--;
    } else if (from > current_focus_ && to <= current_focus_) {
        current_focus_++;
    }
}

void X11WindowManager::setup_global_grab()
{
    grab_display_ = XOpenDisplay(nullptr);
    if (!grab_display_) return;

    grab_root_ = DefaultRootWindow(grab_display_);

    XGrabButton(
        grab_display_,
        Button2,
        AnyModifier,
        grab_root_,
        False,
        ButtonPressMask,
        GrabModeSync,   // pointeur sync : freezé jusqu'à XAllowEvents
        GrabModeAsync,  // clavier async
        None, None);

    XFlush(grab_display_);
}

void X11WindowManager::teardown_global_grab()
{
    if (!grab_display_) return;
    XUngrabButton(grab_display_, Button2, AnyModifier, grab_root_);
    XCloseDisplay(grab_display_);
    grab_display_ = nullptr;
}

bool X11WindowManager::poll_global_middle_click()
{
    if (!grab_display_) return false;

    XEvent ev;
    if (XCheckMaskEvent(grab_display_, ButtonPressMask, &ev)) {
        if (ev.type == ButtonPress && ev.xbutton.button == Button2) {
            int click_x = ev.xbutton.x_root;
            int click_y = ev.xbutton.y_root;

            // Vérifie si le clic tombe dans la géométrie d'au moins une
            // fenêtre de la liste. On utilise display_ (connexion principale)
            // pour XGetWindowAttributes + XTranslateCoordinates car c'est
            // elle qui connaît les XIDs de _NET_CLIENT_LIST.
            bool in_managed = false;
            for (const auto& mw : windows_) {
                XWindowAttributes attrs;
                if (!XGetWindowAttributes(display_, mw.xid, &attrs))
                    continue;

                // Traduit l'origine de la fenêtre en coordonnées root
                int win_x, win_y;
                Window child;
                XTranslateCoordinates(display_, mw.xid, root_,
                                      0, 0, &win_x, &win_y, &child);

                if (click_x >= win_x && click_x < win_x + attrs.width &&
                    click_y >= win_y && click_y < win_y + attrs.height) {
                    in_managed = true;
                    break;
                }
            }

            XAllowEvents(grab_display_, ReplayPointer, ev.xbutton.time);
            XFlush(grab_display_);
            return in_managed;
        }
        XAllowEvents(grab_display_, ReplayPointer, CurrentTime);
        XFlush(grab_display_);
    }
    return false;
}