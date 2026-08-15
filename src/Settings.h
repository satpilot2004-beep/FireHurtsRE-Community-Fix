#pragma once
#include "PCH.h"
#include <AutoTOML.hpp>

struct Settings
{
private:
	using iSetting = AutoTOML::iSetting;
	using bSetting = AutoTOML::bSetting;
	using fSetting = AutoTOML::fSetting;
	using sSetting = AutoTOML::sSetting;

public:
	static void load()
	{
		try {
			const auto table = toml::parse_file("Data/SKSE/Plugins/FiresHurtRE.toml"s);
			for (const auto& setting : AutoTOML::ISetting::get_settings()) {
				setting->load(table);
			}
		} catch (const toml::parse_error& e) {
			std::ostringstream ss;
			ss
				<< "Error parsing file '" << *e.source().path << "':\n"
				<< '\t' << e.description() << '\n'
				<< "\t\t(" << e.source().begin << ')';
			logger::error("{}", ss.str());
			throw std::runtime_error("failed to load settings"s);
		}
	}

	// How long (seconds) the player/NPC must be inside a fire before damage starts.
	static inline fSetting FireDelay{ "General"s, "Delay"s, 0.0f };

	// Direct health damage applied per second while inside a fire source.
	// This is on top of whatever the spell effect itself does.
	// Set to 0.0 to rely entirely on the spell's own magnitude.
	static inline fSetting FireDamage{ "Damage"s, "FireDamagePerSecond"s, 5.0f };
	static inline fSetting SteamDamage{ "Damage"s, "SteamDamagePerSecond"s, 3.0f };
	static inline fSetting MagicDamage{ "Damage"s, "MagicDamagePerSecond"s, 8.0f };

	// Whether damage should respect actor magic resistance (fire/frost/shock resist).
	// When true, damage is reduced by the actor's fire resistance percentage.
	static inline bSetting UseFireResistance{ "Damage"s, "UseFireResistance"s, true };
};
