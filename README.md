# WinFocus — Gestionnaire de fenêtres X11 avec ImGui

Interface graphique permettant de lister, renommer, ordonner et mettre au focus des fenêtres X11 filtrées par mot-clé.
Développé via Claude Code.

## Fonctionnalités

| Action | Comment |
|--------|---------|
| **Filtrage** | Saisir un mot-clé (sensible à la casse), cliquer *Rafraîchir* |
| **Auto-refresh** | Cocher *Auto* + régler l'intervalle (1–30 s) |
| **Focus** | Clic sur le bouton **Focus** de la ligne |
| **Focus cycle** | Clic molette — capturé globalement, fonctionne depuis n'importe quelle fenêtre |
| **Renommer** | Double-clic sur le nom **ou** bouton `...` — Entrée pour valider |
| **Réordonner** | Glisser-déposer via la poignée `:::` à gauche |

## Dépendances

```
sudo apt install libglfw3-dev libgl-dev libx11-dev git cmake build-essential
```

| Paquet | Rôle |
|--------|------|
| `libglfw3-dev` | Fenêtre OpenGL + callbacks souris/clavier |
| `libgl-dev` | Rendu OpenGL 3.3 |
| `libx11-dev` | Interaction X11 (focus, liste des fenêtres, grab global) |
| `cmake` ≥ 3.16 | Système de build |
| `git` | Clone automatique d'ImGui |

Dear ImGui est téléchargé automatiquement par le script de build (tag v1.90.1).

## Compilation & Lancement

```bash
chmod +x build.sh
./build.sh
./build/winfocus
```

## Architecture du projet

```
winfocus/
├── CMakeLists.txt          — Configuration CMake
├── build.sh                — Script de build (vérifie deps + compile)
├── README.md
├── imgui/                  — Sources ImGui (créé par build.sh via git clone)
└── src/
    ├── main.cpp            — UI ImGui + boucle GLFW
    ├── x11_windows.h       — Interface X11WindowManager
    └── x11_windows.cpp     — Implémentation X11 (fenêtres, focus, grab global)
```

## Notes X11

- La liste des fenêtres utilise `_NET_CLIENT_LIST` (EWMH) pour les WM modernes (GNOME, KDE, XFWM…).
  Fallback sur `XQueryTree` si non disponible.
- Le focus utilise `_NET_ACTIVE_WINDOW` (méthode recommandée) + `XRaiseWindow`/`XSetInputFocus` en fallback.
- Le clic molette est capturé globalement via `XGrabButton` sur un display secondaire, ce qui permet de
  cycler entre les fenêtres sans revenir sur WinFocus. Le clic est ensuite rejoué (`ReplayPointer`) pour
  ne pas perturber les autres applications.
- Le renommage est **local à l'application** : il ne modifie pas le titre X11 réel (`WM_NAME`).

## Raccourcis

| Action | Raccourci |
|--------|-----------|
| Fenêtre suivante | Clic molette (n'importe où sur le bureau) |
| Valider le renommage | Entrée |
| Annuler le renommage | Échap |