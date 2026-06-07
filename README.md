# WinFocus — Gestionnaire de fenêtres X11 avec ImGui

Interface graphique permettant de lister, renommer, ordonner et mettre au focus des fenêtres X11 filtrées par mot-clé.

## Fonctionnalités

| Action | Comment |
|--------|---------|
| **Filtrage** | Saisir un mot-clé (sensible à la casse), cliquer *Rafraîchir* |
| **Auto-refresh** | Cocher *Auto* + régler l'intervalle (1–30 s) |
| **Focus** | Clic sur le bouton **Focus** de la ligne |
| **Focus cycle** | Clic molette depuis une fenêtre de la liste — cycle vers la suivante |
| **Renommer** | Double-clic sur le nom **ou** bouton `...` — Entrée pour valider |
| **Réordonner** | Glisser-déposer via la poignée `:::` à gauche |

## Comportement du clic molette

Le clic molette est capturé globalement via `XGrabButton` sur toute la surface de l'écran. Il ne déclenche le cycle de fenêtres que si le clic est effectué **dans la géométrie d'une des fenêtres de la liste** (détection par `XGetWindowAttributes` + `XTranslateCoordinates`).

Si le clic est fait en dehors de toute fenêtre gérée, il est retransmis normalement à l'application cible (`XAllowEvents(ReplayPointer)`) — le comportement natif du clic molette (fermer un onglet, coller le presse-papier, scroller, etc.) est préservé.

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

- La liste des fenêtres utilise `_NET_CLIENT_LIST` (EWMH) pour les WM modernes (GNOME, KDE, XFWM…). Fallback sur `XQueryTree` si non disponible.
- Le focus utilise `_NET_ACTIVE_WINDOW` (méthode recommandée) + `XRaiseWindow`/`XSetInputFocus` en fallback.
- Le grab global utilise `GrabModeSync` pour le pointeur : cela freeze le pointeur jusqu'à `XAllowEvents`, ce qui garantit que `ReplayPointer` retransmet le clic exactement comme s'il n'avait jamais été intercepté.
- Le renommage est **local à l'application** : il ne modifie pas le titre X11 réel (`WM_NAME`).

## Raccourcis

| Action | Raccourci |
|--------|-----------|
| Fenêtre suivante | Clic molette depuis une fenêtre de la liste |
| Valider le renommage | Entrée |
| Annuler le renommage | Échap |