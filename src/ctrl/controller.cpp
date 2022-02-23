#include <algorithm>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./controller.hpp"

namespace {
	constexpr char FIELD_ENTRY[] = "Field";
	constexpr char TICKS_ENTRY[] = "Ticks";
	constexpr char CURSOR_ENTRY[] = "Cursor";
	constexpr char PROVISION_ENTRY[] = "Provision";
	constexpr char ITEMS_ENTRY[] = "Items";
	constexpr char FLAGS_ENTRY[] = "Flags";
}

void controller::build() {
	slots_.assign(MAXIMUM_ITEMS, item_slot{});
	flags_.assign(MAXIMUM_FLAGS, 0);
	this->clear();
}

void controller::handle() {
	if (state_._raw.any()) {
		++ticks_;
	}
}

void controller::read(const nlohmann::json& data) {
	if (
		data.contains(FIELD_ENTRY) and
		data[FIELD_ENTRY].is_string()
	) {
		field_ = data[FIELD_ENTRY].get<std::string>();
		state_.transfering = true;
	}
	if (
		data.contains(TICKS_ENTRY) and
		data[TICKS_ENTRY].is_number_unsigned()
	) {
		ticks_ = data[TICKS_ENTRY].get<i64>();
	} else {
		ticks_ = 0;
	}

	if (
		data.contains(CURSOR_ENTRY) and
		data[CURSOR_ENTRY].is_number_unsigned()
	) {
		cursor_ = data[CURSOR_ENTRY].get<udx>();
	} else {
		cursor_ = 0;
	}

	if (
		data.contains(PROVISION_ENTRY) and
		data[PROVISION_ENTRY].is_number_unsigned()
	) {
		provision_ = data[PROVISION_ENTRY].get<udx>();
		if (provision_ >= MAXIMUM_ITEMS) {
			provision_ = INVALID_SLOT;
		}
	} else {
		provision_ = INVALID_SLOT;
	}

	if (
		data.contains(ITEMS_ENTRY) and
		data[ITEMS_ENTRY].is_array() and
		data[ITEMS_ENTRY].size() == MAXIMUM_ITEMS
	) {
		auto& list = data[ITEMS_ENTRY];
		for (udx x = 0; x < MAXIMUM_ITEMS; ++x) {
			if (list[x].is_array() and list[x].size() == 4) {
				if (
					list[x][0].is_number_integer() and
					list[x][1].is_number_integer() and
					list[x][2].is_number_integer() and
					list[x][3].is_boolean()

				) {
					slots_[x].type = list[x][0].get<i32>();
					slots_[x].count = list[x][1].get<i32>();
					slots_[x].limit = list[x][2].get<i32>();
					slots_[x].weapon = list[x][3].get<bool>();
				} else {
					slots_[x] = {};
				}
			} else {
				slots_[x] = {};
			}
		}
	} else {
		std::fill(slots_.begin(), slots_.end(), item_slot{});
	}

	if (
		data.contains(FLAGS_ENTRY) and
		data[FLAGS_ENTRY].is_array() and
		data[FLAGS_ENTRY].size() == MAXIMUM_FLAGS
	) {
		auto& list = data[FLAGS_ENTRY];
		for (udx idx = 0; idx < MAXIMUM_FLAGS; ++idx) {
			if (list[idx].is_number_unsigned()) {
				flags_[idx] = list[idx].get<u64>();
			} else {
				flags_[idx] = 0;
			}
		}
	} else {
		std::fill(flags_.begin(), flags_.end(), 0);
	}
}

void controller::write(nlohmann::json& data) const {
	data[FIELD_ENTRY] = field_;
	data[TICKS_ENTRY] = ticks_;
	data[CURSOR_ENTRY] = cursor_;
	if (provision_ != INVALID_SLOT) {
		data[PROVISION_ENTRY] = provision_;
	} else {
		data[PROVISION_ENTRY] = -1;
	}
	{
		auto entry = nlohmann::json::array();
		for (auto&& slot : slots_) {
			auto details = nlohmann::json::array({
				slot.type,
				slot.count,
				slot.limit,
				slot.weapon
			});
			entry.push_back(details);
		}
		data[ITEMS_ENTRY] = entry;
	}
	{
		auto entry = nlohmann::json::array();
		for (auto&& flag : flags_) {
			entry.push_back(flag);
		}
		data[FLAGS_ENTRY] = entry;
	}
}

void controller::shift_slots_(udx removed) {
	if (provision_ == removed) {
		provision_ = INVALID_SLOT;
	} else if (provision_ > removed) {
		--provision_;
	}
	const udx shifts = MAXIMUM_ITEMS - removed - 1;
	for (udx s = removed; s < shifts; ++s) {
		std::swap(slots_[s], slots_[s + 1]);
	}
	slots_[MAXIMUM_ITEMS - removed] = {};
}

r64 controller::seconds() const {
	return as<r64>(ticks_) * konst::INVERSE_TICK<r64>();
}
