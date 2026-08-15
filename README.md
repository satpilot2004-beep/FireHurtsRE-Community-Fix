# FireHurtsRE — Community Fix

A community-maintained fix and enhancement of [FireHurtsNG](https://www.nexusmods.com/skyrimspecialedition/mods/?) by **fenix31415** and **doodlum**.

Makes fire, steam, and magical flames actually dangerous — players and NPCs take damage when standing in or being pushed into fire sources.

---

## What was fixed

The original plugin had been abandoned for years and caused frequent crashes on modern setups, and several fire sources (including magical ones like the College of Winterhold pillars) dealt no damage at all due to a collision-detection bug. See [CHANGELOG.md](CHANGELOG.md) for the full history.

---

## New features

- **NPC fire damage** — NPCs and followers pushed into fire now take damage, not just the player. Toggle independently via `NPCFireDamage` in the TOML config.
- **Configurable damage per second** — independently tune damage for regular fire, steam, and magical flames via the TOML config file.
- **Fire resistance scaling** — damage can optionally respect the actor's fire resistance stat.
- **Visual Studio 2026 / MSVC 14.51 support** — updated build system to compile on current toolchains.

---

## Requirements

- [Skyrim Special Edition](https://store.steampowered.com/app/489830) — version 1.6.x (AE) or 1.5.97 (SE)
- [SKSE64](https://skse.silverlock.org/)
- [FiresHurtRE.esp](?) — the original ESP is still required (provides spells, keywords, and form data)

---

## Installation

Use a mod manager (MO2 or Vortex) or install manually:

```
Data/
  SKSE/
    Plugins/
      FireHurtsRE.dll
      FiresHurtRE.toml        ← copy from this release, edit to taste
      FireHurtsRE/
        EmbersHD.json         ← fire source definitions
        EmbersXD.json
```

The original `FiresHurtRE.esp` must also be active in your load order.

---

## Configuration

Edit `Data/SKSE/Plugins/FiresHurtRE.toml` to your liking:

```toml
[General]
# Seconds the actor must be inside fire before damage starts. 0.0 = instant.
Delay = 0.0

[Damage]
# Health damage applied per second for each fire type.
# Set to 0.0 to rely entirely on the spell's own magnitude.
FireDamagePerSecond  = 5.0   # campfires, torches, hearths
SteamDamagePerSecond = 3.0   # geothermal vents
MagicDamagePerSecond = 8.0   # Dwemer fire, magical flames (e.g. College of Winterhold pillars)

# Scale damage by the actor's fire resistance stat (0-100%).
UseFireResistance = true

# Whether NPCs (non-player actors) take fire damage. The player is always
# affected regardless of this setting.
NPCFireDamage = true
```

Changes take effect on the next game load (no mid-session hot reload).

---

## Compatibility

- Compatible with **Embers XD**, **EmbersHD**, and other fire retexture mods — support is driven by the JSON files in `FireHurtsRE/`, not the textures.
- To add support for a custom fire mod, add a JSON entry to `FireHurtsRE/` following the existing format.
- Not compatible with other plugins that modify the same fire damage spells or hook `PlayerCharacter::Update` in conflicting ways.

---

## Building from source

Requirements: Visual Studio 2022 (or later) with the matching MSVC/CMake toolset, CMake 3.21+, Git, vcpkg.

```bash
git clone --recurse-submodules https://github.com/satpilot2004-beep/FireHurtsRE-Community-Fix.git
cd FireHurtsRE-Community-Fix
setx VCPKG_ROOT C:\vcpkg   # if not already set
"Build Release.bat"
```

The output DLL will be at `build/skyrim/Release/FireHurtsRE.dll`.

---

## Credits

- **doodlum** — original FireHurtsNG plugin and ESP
- **fenix31415** — FenixUtils / UselessFenixUtils, original mod concept
- **CharmedBaryon** — CommonLibSSE-NG
- **Ryan McKenzie** — AutoTOML
- **satpilot2004-beep** — community fix maintainer
- **Claude (Anthropic)** — debugged and fixed the fire/steam/magic damage collision bug, restored the NPC damage toggle, and helped maintain this fork

---

## License

Apache-2.0 — same as the original source. See [LICENSE](LICENSE).
