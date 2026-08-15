# Changelog

All notable changes to this community fix are documented here.

## [1.0.5.0] - 2026-08-15

### Fixed
- Fire/steam/magic damage silently not applying to actors standing in fire sources with large or offset bounding boxes (e.g. the tall College of Winterhold magic pillars). The fire ticker only tested collision against the single fire object whose bounding-box *center* was nearest to the actor, instead of checking every registered fire nearby — so a closer-but-non-colliding object could be picked over the fire the actor was actually standing in. Every nearby fire is now checked.
- The College of Winterhold magic fountain/pillar (FormID `0x10D264` in `Skyrim.esm`) was never registered as a fire source at all, so it could never deal damage regardless of the bug above. It's now registered as a `Magic`-type fire.
- Restored the `NPCFireDamage` TOML toggle (`[Damage]` section, default `true`), which had been dropped in an earlier revision. The player always takes fire damage regardless of this setting; it only gates NPCs.

## [1.0.4] - 2026-08-08

### Fixed
- Crash on game load caused by `TES::ForEachReference` walking a streaming cell — replaced with `playerCell->ForEachReference`, which only iterates the current cell.
- Crash when the `Data/SKSE/Plugins/FireHurtsRE/` directory is missing.
- Crash from a malformed or missing `.toml` config on startup — the exception is now caught and logged, and the plugin continues with defaults.
- `GetBaseObject()` null dereference when checking fire type.
- Null keyword/spell crash when `FiresHurtRE.esp` forms aren't found.
- NPCs pushed into fire took no damage.

### Added
- NPC fire damage ticker (scans the current cell every second).
- Configurable damage per second per fire type via the TOML config.
- Optional fire-resistance scaling.
- Visual Studio 2026 / MSVC 14.51 build support.
