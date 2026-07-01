# macOS one-time setup

A few keys rely on macOS-side settings. Do these once on the Mac after flashing.

## 1. Caps Lock → switch input source (Georgian ⇄ English)

The firmware sends a normal Caps Lock on tap; macOS turns that into an input-source switch.

- **System Settings ▸ Keyboard ▸ Input Sources ▸ Edit…** — make sure both sources (e.g. Georgian
  and ABC/English) are added.
- Set the **"Use the Caps Lock key to switch to and from [non-Latin source]"** option (shown when a
  non-Latin source is present).
- Result: a quick **Caps tap** on the GMMK switches input like on the laptop; **holding Caps** is the
  `_FN` layer and never toggles caps lock.

## 2. Dictation — `Fn+F5`

The firmware emits **⌃⌥⌘D** (Control+Option+Command+D) for this key.

- **System Settings ▸ Keyboard ▸ Dictation ▸ Shortcut ▸ Customize…**, then press **⌃⌥⌘D** so the
  shortcut matches what the key sends.

## 3. Do Not Disturb / Focus — `Fn+F6`

The firmware emits **⌃⌥⌘N** (Control+Option+Command+N). macOS has no built-in global DND shortcut, so
make one:

- Open **Shortcuts.app ▸ +** new shortcut ▸ add **"Set Focus"** (Do Not Disturb, Toggle).
- In the shortcut's settings, assign the keyboard shortcut **⌃⌥⌘N**.
- (Alternative: Raycast/BetterTouchTool can bind the same combo to a DND toggle.)

## 4. Function keys — nothing to do

External keyboards send real F1–F12 by default (which is what the base layer wants). No "use F-keys
as function keys" change is needed.

---

### Linux (Fedora/GNOME) — when you set it up

- **Caps → input switch:** GNOME doesn't use Caps for this by default. Either set an input-switch
  shortcut in Settings ▸ Keyboard, or add the `grp:caps_toggle` XKB option via GNOME Tweaks.
- Screenshots, word/doc nav, Mission-Control/Spotlight, and undo/redo are already handled in firmware
  via `os_detection` (GNOME combos). No extra setup expected beyond the input switch.
