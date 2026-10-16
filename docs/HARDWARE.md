# Hardware — GMMK Pro build knowledgebase

Everything physical about the board: parts, specs, the mods done to it, spare keycaps, and the
purchase log. Reference for future repairs, keycap swaps, and firmware decisions.

---

## The board

|                 |                                                                                                            |
| --------------- | ---------------------------------------------------------------------------------------------------------- |
| Model           | **Glorious GMMK Pro**, 75% **ANSI**, white aluminium case                                                  |
| Bought as       | "Glorious GMMK PRO 75% Barebone White"                                                                     |
| Layout          | US ANSI, 75% (F-row + arrows + right nav column + rotary knob)                                             |
| MCU             | **STM32F303** — ARM Cortex-M4, ChibiOS                                                                     |
| Bootloader      | **stm32-dfu** (built-in ROM DFU). Flash with `qmk flash` / `dfu-util`.                                     |
| Firmware        | **QMK** (this repo). Was VIA/QMK/Glorious-Core capable from factory.                                       |
| Hotswap sockets | 5-pin, **south-facing** PCB — takes 3- and 5-pin MX switches                                               |
| RGB             | Per-key **south-facing** RGB (16.8M colors) + left/right **side diffuser strips** with soft-glow diffusers |
| Daughterboard   | USB-C port sits on a separate small PCB, wired to the main PCB                                             |
| Polling         | up to 1000 Hz (QMK default)                                                                                |
| Connection      | Detachable USB-C                                                                                           |

**The single most important hardware fact for firmware:** the keycaps are **PBT, non-shine-through**,
and the LEDs are **south-facing**. So per-key RGB shows only as a soft **edge/underglow** around each
cap — legends never light. The **side strips are the dominant visual and the natural place to encode
information** (layer, knob mode, caps lock). This is why the RGB design leans on the side strips.

---

## Switches — Kailh BOX Silent Brown

Tactile + silent. Bought as a 90-pack (perfect for the GMMK Pro) from SwitchCaptain (authentic Kailh,
2021 tooling, "Silent Edition").

| Spec                 | Value                                                         |
| -------------------- | ------------------------------------------------------------- |
| Type                 | Tactile mechanical, silent                                    |
| Travel               | 3.6 mm total                                                  |
| Actuation force      | 45 g                                                          |
| Peak (tactile) force | 65 g                                                          |
| Bottom-out           | 60 g                                                          |
| Durability           | 80 M cycles                                                   |
| Pins                 | 3-pin, plate-mounted, hotswap-compatible (no clipping needed) |
| LED                  | SMD + through-hole compatible (full RGB)                      |
| Contact              | Normally open, plate mount                                    |

Feel: a gentle bump with a muted, quiet sound — chosen for nighttime / quiet-room use.

---

## Modifications done to the board

| Mod             | Detail                                                                                                                                                                         |
| --------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Plate swap**  | Stock aluminium plate → **Polycarbonate** (Glorious GMMK PRO Switch Plate, US/ANSI). Softer, more flex, deeper/quieter typing sound. Stock alu plate is kept in the parts box. |
| **Stabilizers** | Upgraded to **premium stabs** (the stock ones were found in the parts box, confirming the installed set is the upgrade). **Lubed** with G-Lube.                                |
| **O-rings**     | Installed on the keycaps (leftover o-rings confirmed in the parts box). Softens bottom-out.                                                                                    |
| **Knob**        | Stock knob → **gold** Glorious rotary knob.                                                                                                                                    |
| **Keycaps**     | Stock → PBT 9009-style set (below).                                                                                                                                            |
| **Cable**       | Stock → coiled USB-C (below).                                                                                                                                                  |

---

## Keycaps — PBT 9009 (installed)

- **Profile:** Cherry (sculpted, row-specific — R1–R4 differ).
- **Material:** PBT, **non-shine-through** (dye-sub/doubleshot legends).
- **Colorway:** 9009 — cream/beige alphas, greige modifiers, with **sage green** and **salmon/pink**
  accents. Installed accents (per photos): salmon **Esc** and **Enter**, sage **spacebar**.
- **Likely source:** KBDfans (PBTfans 9009). Not 100% certain after time; it is definitely a PBT
  9009 Cherry-profile set. (Akko 9009 and a Drop 9009 were also bought at some point but are **not**
  the set installed.)

### Spare keycaps on hand (parts trays)

Useful for future relabels — **remember Cherry profile is row-specific**, so a spare only fits its
matching row.

- **Modifiers (bottom row R4):** `Alt`, `Code` ×2 (the retro ⌘/meta legend), `Menu`, `Ctrl`, `Fn` ×2
- **Nav / editing:** `Home`, `End`, `Pg Up`, `Pg Dn`, `Delete`, `Insert`, `Ins`, `Scroll Lock`,
  `Pause`, `Num Lock`
- **Numpad legends (dual):** `7/Home`, `8/↑`, `9/PgUp`, `4/←`, `5`, `6/→`, `1/End`, `2/↓`, `3/PgDn`,
  `0/Ins`, `./Del`, `/`, `*`, `−`, `+`
- **Arrows:** dedicated `←↑↓→`
- **Accent caps:** green `Insert`, sage **spacebar**, salmon `7/Home`, salmon shift, enter `⤶`

The installed caps already match the keycodes; no relabel is planned.

---

## Cable

Coiled **USB-C** aviator-style cable from **KBDfans**, sage/green to match the 9009 accents.

---

## Tools & accessories owned

For maintenance / future switch or stab work:

- Glorious **Ergonomic Switch Puller**
- Glorious **Switch Opener**
- Glorious **Lube Brush**
- **G-Lube** (switch/stab lubricant)

---

## Purchase log (for provenance)

**Glorious (gloriousgaming.com):**

- GMMK PRO 75% Barebone White — $100.00
- GMMK PRO Switch Plate, US (ANSI) / Polycarbonate — $19.99
- GMMK PRO Rotary Knob, Gold — $9.99
- Ergonomic Switch Puller — $7.99
- Switch Opener — $9.99
- Lube Brush — $4.99
- G-Lube — $9.99

**KBDfans (kbdfans.com):**

- PBT 9009 keycaps (PBTfans 9009, Cherry profile)
- Coiled USB-C cable

**SwitchCaptain:**

- Kailh BOX Silent Brown ×90

**Bought but NOT installed:** Akko 9009, Drop (drop.com) 9009 keycaps.

---

## Kept in the parts box

Stock aluminium plate · stock stabilizers · leftover o-rings · misc GMMK accessories. (Useful if
ever reverting a mod or rebuilding.)
