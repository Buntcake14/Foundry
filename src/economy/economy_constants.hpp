#pragma once

namespace economy {

constexpr inline float factories_per_state_required_city_size = 60'000.f;

namespace numerical {
namespace commodity_unit {
inline constexpr float epsilon = 0.0001f;
}
namespace employment_unit {
inline constexpr float epsilon = 0.1f;
}
}

inline constexpr float factory_closed_threshold = 0.0001f;
inline constexpr uint32_t price_history_length = 256;
inline constexpr uint32_t gdp_history_length = 128;

// descides the divisor for the army demand from reinforcements. It is set to 28 to spread out the reinforcement demand over 28 days, as reinforce ticks only happen once a month
constexpr inline float unit_reinforcement_demand_divisor = 28.0f;


inline constexpr float merchant_cut_foreign = 0.05f;
inline constexpr float merchant_cut_domestic = 0.001f;
inline constexpr float effect_of_transportation_scale = 0.0005f;
inline constexpr float trade_distance_covered_by_pair_of_workers_per_unit_of_good = 10.f;
// Huge values could cause massive spikes of demand for transportation labor
inline constexpr float invalid_trade_route_distance = 0.01f;
inline constexpr float trade_loss_per_distance_unit = 0.001f;
inline constexpr float trade_effect_of_scale_lower_bound = 0.1f;
inline constexpr float trade_base_multiplicative_decay = 0.0002f;
inline constexpr float trade_base_additive_decay = 0.1f;
inline constexpr float min_trade_expansion_multiplier = 0.15f;
inline constexpr float trade_demand_satisfaction_cutoff = 0.7f;

float constexpr inline base_qol = 0.75f;
constexpr inline uint32_t build_factory = issue_rule::pop_build_factory;
constexpr inline uint32_t expand_factory = issue_rule::pop_expand_factory;
constexpr inline uint32_t can_invest = expand_factory | build_factory;

// stockpile related things:
inline constexpr float stockpile_to_supply = 0.1f;
inline constexpr float stockpile_spoilage = 0.01f;
inline constexpr float stockpile_expected_spending_per_commodity = 1'000.f;
inline constexpr float market_savings_target = 1'000'000.f;
inline constexpr float trade_transaction_soft_limit = 1'000.f;

// base subsistence
inline constexpr float subsistence_score_life = 30.0f;
inline constexpr float subsistence_score_total = subsistence_score_life;

// vanilla-style strict needs tiering: a lower tier must be ~fully satisfied
// with money before any budget is released to the next tier up
inline constexpr float needs_tier_satisfaction_gate = 0.98f;

// vanilla-style artisan production: a province works one good at a time and
// only periodically reconsiders switching, rather than continuously blending
// fractional employment across every possible good every day
inline constexpr int32_t artisan_reroll_period_days = 30;
inline constexpr float artisan_switch_margin = 5.0f;

// vanilla-style factory hiring: a factory wants its full base_workforce * level
// directly rather than slowly walking toward it via a profit gradient; this
// damping factor is how much of the remaining gap it closes per day
inline constexpr float factory_hiring_damping = 0.2f;

// a factory the affordability clamp drives toward zero desired employment risks
// automatic deletion (economy.cpp's prune_factories fires below 50 while
// unprofitable) -- keep a minimal skeleton crew alive, safely above that
// threshold, so a temporary input-cost spike wounds a factory instead of
// permanently destroying it
inline constexpr float factory_survival_floor = 60.f;

// vanilla-style discrete RGO tiers (user-requested, not a vanilla mechanic --
// vanilla RGO size is just fixed, and Alice's original smooth investment curve
// up to rgo_potential turned out to be too easy to leave stuck or to spiral
// against, see the milestone 6 economy phase writeup): an RGO only starts
// investing toward its *next* tier once the current one is this utilized (an
// idle RGO shouldn't expand just because it theoretically could -- only one
// that's actually in hot demand), and completing a tier costs this much
// investment per worker-slot the tier is worth (commodity_get_rgo_workforce).
inline constexpr float rgo_level_up_utilization_threshold = 0.9f;
inline constexpr float rgo_level_up_cost_per_worker = 0.05f;

// move to defines later
inline constexpr float payouts_spending_multiplier = 10.f;

inline constexpr float investment_pool_investment_per_day = 0.25f;

// greed drives incomes of corresponding pops up
// while making life worse on average
// profit cuts change distribution of incomes
inline constexpr float aristocrats_greed = 0.1f;
inline constexpr float artisans_greed = 0.1f;
inline constexpr float labor_greed_life = 0.1f;
inline constexpr float capitalists_greed = 0.1f;
}
