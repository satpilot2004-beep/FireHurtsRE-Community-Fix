#include <UselessFenixUtils.h>
#include "Geom.h"
#include "FireStorage.h"
#include "Settings.h"
#include <array>


class DataStorage
{
	static constexpr auto ModName = "FiresHurtRE.esp"sv;

public:
	static auto get_f314FH_kywd_All()
	{
		static auto ans = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSKeyword>(0x803, ModName);
		return ans;
	}
	static auto get_f314FH_spel_Fire()
	{
		static auto ans = RE::TESDataHandler::GetSingleton()->LookupForm<RE::SpellItem>(0x808, ModName);
		return ans;
	}
	static auto get_f314FH_spel_Magic()
	{
		static auto ans = RE::TESDataHandler::GetSingleton()->LookupForm<RE::SpellItem>(0x809, ModName);
		return ans;
	}
	static auto get_f314FH_spel_Steam()
	{
		static auto ans = RE::TESDataHandler::GetSingleton()->LookupForm<RE::SpellItem>(0x80a, ModName);
		return ans;
	}
};

bool is_cooking(RE::Actor* a)
{
	return a->AsActorState()->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal ||
	       a->IsPlayerRef() && RE::UI::GetSingleton()->IsMenuOpen("Crafting Menu");
}

bool is_onfire(RE::Actor* a)
{
	auto kywd = DataStorage::get_f314FH_kywd_All();
	return kywd && FenixUtils::TESObjectREFR__HasEffectKeyword(a, kywd);
}

auto get_bound_vertexes(const global_bounds_t& bounds)
{
	RE::NiPoint3 n1{ bounds.Normals.entry[0][0], bounds.Normals.entry[1][0], bounds.Normals.entry[2][0] };
	RE::NiPoint3 n2{ bounds.Normals.entry[0][1], bounds.Normals.entry[1][1], bounds.Normals.entry[2][1] };
	RE::NiPoint3 n3{ bounds.Normals.entry[0][2], bounds.Normals.entry[1][2], bounds.Normals.entry[2][2] };
	return std::array<RE::NiPoint3, 8>{
		bounds.Base + n1 + n2 + n3,
		bounds.Base - n1 + n2 + n3,
		bounds.Base - n1 - n2 + n3,
		bounds.Base + n1 - n2 + n3,
		bounds.Base + n1 + n2 - n3,
		bounds.Base - n1 + n2 - n3,
		bounds.Base - n1 - n2 - n3,
		bounds.Base + n1 - n2 - n3
	};
}

template <glm::vec4 Color = Colors::RED>
void draw_bounds(const global_bounds_t& bounds, float update_period)
{
	auto verts = get_bound_vertexes(bounds);
	const int dur = static_cast<int>(update_period * 1000);
	const float wide = 5.0f;
	auto draw_ = [=](int u, int v) { draw_line<Color>(verts[u], verts[v], wide, dur); };
	draw_(0, 1); draw_(0, 3); draw_(2, 1); draw_(2, 3);
	draw_(4, 5); draw_(4, 7); draw_(6, 5); draw_(6, 7);
	draw_(0, 4); draw_(1, 5); draw_(2, 6); draw_(3, 7);
}

template <glm::vec4 Color = Colors::RED>
void draw([[maybe_unused]] RE::Actor* a, [[maybe_unused]] RE::TESObjectREFR* refr, [[maybe_unused]] float update_period)
{
#ifndef NDEBUG
	draw_line<Color>(refr->GetPosition(), a->GetPosition(), 5.0f, static_cast<int>(update_period) * 1000);
	draw_bounds<Color>(get_refr_bounds(refr), update_period);
#endif
}

class Timeouts
{
public:
	static float cooking()    { return 10.0f; }
	static float noFireNear() { return 5.0f; }
	static float inFire()     { return 1.0f; }
	static float fireNear(float dist2) { return sqrtf(dist2) / 1000.0f; }
};

enum class FireTypes { None, Fire, Steam, Magic };

FireTypes get_fire_type(RE::FormID id)
{
	if (FiresStorage::is_steam_refr(id)) return FireTypes::Steam;
	if (FiresStorage::is_magic_refr(id)) return FireTypes::Magic;
	return FireTypes::Fire;
}

RE::SpellItem* get_fireSpell(FireTypes type)
{
	switch (type) {
	case FireTypes::Steam: return DataStorage::get_f314FH_spel_Steam();
	case FireTypes::Magic: return DataStorage::get_f314FH_spel_Magic();
	default:               return DataStorage::get_f314FH_spel_Fire();
	}
}

float get_fire_damage(FireTypes type)
{
	switch (type) {
	case FireTypes::Steam: return static_cast<float>(*Settings::SteamDamage);
	case FireTypes::Magic: return static_cast<float>(*Settings::MagicDamage);
	default:               return static_cast<float>(*Settings::FireDamage);
	}
}

// Apply one tick of fire damage to an actor.
// The spell is cast for its visual/sound/keyword effects (is_onfire depends on the keyword).
// Direct health damage is then applied separately so it is fully configurable via TOML.
void apply_fire_damage(RE::Actor* actor, FireTypes type)
{
	// Cast spell for VFX, sounds and the burning keyword.
	auto spell = get_fireSpell(type);
	if (spell)
		FenixUtils::cast_spell(actor, actor, spell);

	// Apply configured damage per second directly to health.
	float dmg = get_fire_damage(type);
	if (dmg <= 0.0f)
		return;

	// Optionally scale by fire resistance (0..100 -> multiplier 1..0).
	if (*Settings::UseFireResistance) {
		float resist = actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kResistFire);
		resist = std::clamp(resist, 0.0f, 100.0f);
		dmg *= (1.0f - resist / 100.0f);
	}

	if (dmg > 0.0f)
		actor->AsActorValueOwner()->RestoreActorValue(
			RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, -dmg);
}


// ---------------------------------------------------------------------------
// Player ticker
// ---------------------------------------------------------------------------
class TickerPlayer
{
	float updateafter = 0.0f;
	float infire_time = 0.0f;

	enum class FireStates { Cooking, NoFireNear, InFire, NearFire };
	struct State { FireStates state : 3; FireTypes type : 3; };

	auto update(RE::Actor* a)
	{
		std::pair<State, float> ans;
		ans.second = -1.0f;
		ans.first.type = FireTypes::None;

		if (is_cooking(a)) { ans.first.state = FireStates::Cooking; return ans; }

		float mindist2 = 1.0E15f;
		RE::TESObjectREFR* refr = nullptr;
		RE::TESObjectREFR* collidingRefr = nullptr;

		auto* playerCell = a->GetParentCell();
		if (playerCell) {
			playerCell->ForEachReference([&](RE::TESObjectREFR& _refr) {
				if (!_refr.IsDeleted() && !_refr.IsDisabled() && FiresStorage::is_fire(_refr)) {
					float d = a->GetPosition().GetSquaredDistance(FiresStorage::get_bounds_center(&_refr));
					if (d < mindist2) { mindist2 = d; refr = &_refr; }
					// Check every nearby fire for an actual collision, not just the one whose
					// bounds center happens to be closest - large/offset bounds (e.g. tall magic
					// pillars) can have a far-away center while still containing the actor.
					if (!collidingRefr && is_collides(a, &_refr))
						collidingRefr = &_refr;
				}
				return RE::BSContainer::ForEachResult::kContinue;
			});
		}

#ifndef NDEBUG
		draw_bounds(get_npc_bounds(a), 0);
#endif

		if (collidingRefr) {
			ans.first.state = FireStates::InFire;
			auto base = collidingRefr->GetBaseObject();
			ans.first.type = base ? get_fire_type(base->GetFormID()) : FireTypes::Fire;
#ifndef NDEBUG
			draw<Colors::RED>(a, collidingRefr, get_new_updateafter(ans, a));
#endif
			return ans;
		}

		if (!refr || 10000000.0f < mindist2) { ans.first.state = FireStates::NoFireNear; return ans; }

		ans.first.state = FireStates::NearFire;
		ans.second = mindist2;
#ifndef NDEBUG
		draw<Colors::GRN>(a, refr, get_new_updateafter(ans, a));
#endif
		return ans;
	}

	float get_new_updateafter(const std::pair<State, float>& state, RE::Actor* a)
	{
		switch (state.first.state) {
		case FireStates::Cooking:    return Timeouts::cooking();
		case FireStates::InFire:     return Timeouts::inFire();
		case FireStates::NoFireNear: return Timeouts::noFireNear();
		case FireStates::NearFire:   return is_onfire(a) ? Timeouts::inFire() : Timeouts::fireNear(state.second);
		default:                     return 0.0f;
		}
	}

public:
	void tick(float delta)
	{
		updateafter -= delta;
		if (infire_time != 0.0f) infire_time += delta;

		if (updateafter < 0.0f) {
			RE::Actor* a = RE::PlayerCharacter::GetSingleton();
			auto state = update(a);
			updateafter = get_new_updateafter(state, a);

			if (state.first.state == FireStates::InFire) {
				if (infire_time == 0.0f) infire_time = delta;
				if (infire_time > *Settings::FireDelay)
					apply_fire_damage(a, state.first.type);
			} else {
				infire_time = 0.0f;
			}
		}
	}
} ticker;


// ---------------------------------------------------------------------------
// NPC ticker
// ---------------------------------------------------------------------------
class TickerNPCs
{
	float updateafter = 0.0f;
	static constexpr float UPDATE_INTERVAL = 1.0f;

public:
	void tick(float delta)
	{
		if (!*Settings::NPCFireDamage) return;

		updateafter -= delta;
		if (updateafter > 0.0f) return;
		updateafter = UPDATE_INTERVAL;

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return;

		auto* cell = player->GetParentCell();
		if (!cell) return;

		std::vector<RE::TESObjectREFR*> fires;
		std::vector<RE::Actor*>         actors;

		cell->ForEachReference([&](RE::TESObjectREFR& refr) {
			if (refr.IsDeleted() || refr.IsDisabled())
				return RE::BSContainer::ForEachResult::kContinue;

			if (FiresStorage::is_fire(refr)) {
				fires.push_back(&refr);
			} else if (auto* actor = refr.As<RE::Actor>()) {
				if (!actor->IsPlayerRef() && !actor->IsDead())
					actors.push_back(actor);
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});

		if (fires.empty() || actors.empty()) return;

		for (auto* actor : actors) {
			// Check every nearby fire for an actual collision, not just the one whose bounds
			// center happens to be closest - large/offset bounds (e.g. tall magic pillars) can
			// have a far-away center while still containing the actor.
			for (auto* fire : fires) {
				if (is_collides(actor, fire)) {
					auto base = fire->GetBaseObject();
					auto fireType = base ? get_fire_type(base->GetFormID()) : FireTypes::Fire;
					apply_fire_damage(actor, fireType);
					break;
				}
			}
		}
	}
} npc_ticker;


// ---------------------------------------------------------------------------
// Hook & plugin entry
// ---------------------------------------------------------------------------
struct PlayerCharacterHook
{
	static void thunk(RE::PlayerCharacter* a_player, float a_delta)
	{
		func(a_player, a_delta);
#ifndef NDEBUG
		DebugAPI_IMPL::DebugAPI::Update();
#endif
		ticker.tick(a_delta);
		npc_ticker.tick(a_delta);
	}
	static inline REL::Relocation<decltype(thunk)> func;

	static void Hook()
	{
		if (REL::Module::IsVR())
			stl::write_vfunc<RE::PlayerCharacter, 0xAF, PlayerCharacterHook>();
		else
			stl::write_vfunc<RE::PlayerCharacter, 0xAD, PlayerCharacterHook>();
	}
};

static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message)
{
	switch (message->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		FiresStorage::init_fires();
		try {
			Settings::load();
		} catch (const std::exception& e) {
			logger::error("Failed to load settings: {}", e.what());
		} catch (...) {
			logger::error("Failed to load settings: unknown error");
		}
		break;
	}
}

void Load()
{
	PlayerCharacterHook::Hook();
	SKSE::GetMessagingInterface()->RegisterListener("SKSE", SKSEMessageHandler);
}
