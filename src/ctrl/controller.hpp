#pragma once

#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>
#include <apostellein/struct.hpp>

#include "../util/item-slot.hpp"

struct buttons;

struct controller {
public:
	static constexpr udx INVALID_SLOT = static_cast<udx>(-1);
	static constexpr udx MAXIMUM_ITEMS = 30;
	static constexpr udx MAXIMUM_FLAGS = 512 / sizeof(u64);
	static constexpr udx BITS_PER_FLAG = 64;
	static constexpr udx MAXIMUM_PROFILES = 6;
	union internals {
		bitfield_raw<u32> _raw {};
		bitfield_index<u32, 0> init;
		bitfield_index<u32, 1> quitting;
		bitfield_index<u32, 2> booting;
		bitfield_index<u32, 3> saving;
		bitfield_index<u32, 4> loading;
		bitfield_index<u32, 5> locked;
		bitfield_index<u32, 6> frozen;
		bitfield_index<u32, 7> transfering;
		bitfield_index<u32, 8> importing;
		bitfield_index<u32, 9> language;
	};
	void build();
	void clear() {
		state_._raw = {};
		state_.init = true;
		state_.locked = true;
		state_.frozen = true;
		ticks_ = 0;
		profile_ = 0;
		cursor_ = 0;
		provision_ = INVALID_SLOT;
		slots_.assign(MAXIMUM_ITEMS, item_slot{});
		flags_.assign(MAXIMUM_FLAGS, 0);
		id_ = 0;
		field_.clear();
		event_.clear();
		language_.clear();
	}
	void clear(const std::string& field) {
		this->clear();
		state_.transfering = true;
		field_ = field;
	}
	void handle();
	void read(const nlohmann::json& data);
	void write(nlohmann::json& data) const;
	void quit() {
		state_.quitting = true;
	}
	void boot() {
		state_.booting = true;
	}
	void lock() {
		state_.locked = true;
		state_.frozen = false;
	}
	void freeze() {
		state_.locked = true;
		state_.frozen = true;
	}
	void unlock() {
		if (!state_.transfering and !state_.booting and !state_.loading) {
			state_.locked = false;
			state_.frozen = false;
		}
	}
	void load() {
		state_.loading = true;
		state_.saving = false;
	}
	void save() {
		state_.saving = true;
		state_.loading = false;
	}
	void transfer(const std::string& field, i32 id) {
		state_.init = false;
		state_.transfering = true;
		state_.importing = false;
		field_ = field;
		id_ = id;
		event_.clear();
	}
	void transfer(const std::string& field, const std::string& event, i32 id) {
		state_.init = false;
		state_.transfering = true;
		state_.importing = true;
		field_ = field;
		event_ = event;
		id_ = id;
	}
	void language(const std::string& name) {
		language_ = name;
		state_.booting = true;
		state_.language = true;
	}
	std::string language() {
		state_.language = false;
		std::string result {};
		std::swap(language_, result);
		return result;
	}
	void finish() {
		state_.init = false;
		state_.booting = false;
		state_.transfering = false;
		state_.importing = false;
		event_.clear();
	}
	void close() {
		state_.init = false;
		state_.booting = false;
		state_.saving = false;
		state_.loading = false;
		id_ = 0;
	}
	void profile(udx value) {
		if (value < MAXIMUM_PROFILES) {
			profile_ = value;
		}
	}
	void cursor(udx value) {
		if (value < MAXIMUM_ITEMS) {
			cursor_ = value;
		}
	}
	void provision(udx value) {
		if (value < MAXIMUM_ITEMS) {
			provision_ = value;
		} else {
			provision_ = INVALID_SLOT;
		}
	}
	void flag_at(udx bit, bool value) {
		const auto index = bit / BITS_PER_FLAG;
		if (index < MAXIMUM_FLAGS) {
			const auto mask = 1ULL << (static_cast<u64>(bit) & (BITS_PER_FLAG - 1));
			if (value) {
				flags_[index] |= mask;
			} else {
				flags_[index] &= ~mask;
			}
		}
	}
	void flags_from(udx first, udx last, bool value) {
		if (last < first) {
			return;
		}
		for (udx it = first; it <= last; ++it) {
			this->flag_at(it, value);
		}
	}
	const internals& state() const { return state_; }
	udx profile() const { return profile_; }
	udx cursor() const { return cursor_; }
	udx provision() const { return provision_; }
	item_slot item_at(udx index) const {
		if (index < MAXIMUM_ITEMS) {
			return slots_[index];
		}
		return {};
	}
	item_slot* item_ptr(udx index) {
		if (index < MAXIMUM_ITEMS) {
			return &slots_[index];
		}
		return nullptr;
	}
	item_slot* provision_ptr() { return this->item_ptr(provision_); }
	item_slot provisioned_item() const { return this->item_at(provision_); }
	bool flag_at(udx bit) const {
		const auto index = bit / BITS_PER_FLAG;
		if (index < MAXIMUM_FLAGS) {
			const auto mask = 1ULL << (static_cast<u64>(bit) & (BITS_PER_FLAG - 1));
			return flags_[index] & mask;
		}
		return false;
	}
	bool query_flags(udx first, udx last) const {
		if (last < first) {
			return false;
		}
		for (udx it = first; it <= last; ++it) {
			if (!this->flag_at(it)) {
				return false;
			}
		}
		return true;
	}
	bool provision_valid() const { return provision_ != INVALID_SLOT; }
	bool provision_weapon() const {
		if (this->provision_valid()) {
			return slots_[provision_].weapon;
		}
		return false;
	}
	bool push_item(i32 type, i32 count) {
		if (type <= 0 or count <= 0 or count > item_slot::MAXIMUM_LIMIT) {
			return false;
		}
		for (auto&& slot : slots_) {
			if (slot.type == 0) {
				slot.count = count;
				slot.limit = item_slot::MAXIMUM_LIMIT;
				return true;
			}
		}
		return false;
	}
	bool weaponize_item(i32 type, bool weapon) {
		if (type <= 0) {
			return false;
		}
		for (auto&& slot : slots_) {
			if (slot.type == type) {
				slot.weapon = weapon;
				return true;
			}
		}
		return false;
	}
	bool limit_item(i32 type, i32 limit) {
		if (type <= 0 or limit <= 0 or limit > item_slot::MAXIMUM_LIMIT) {
			return false;
		}
		bool found = false;
		for (auto&& slot : slots_) {
			if (slot.type == type) {
				if (slot.count > limit) {
					slot.count = limit;
				}
				slot.limit = limit;
				found = true;
			}
		}
		return found;
	}
	bool add_item(i32 type, i32 count) {
		if (type <= 0 or count <= 0 or count > item_slot::MAXIMUM_LIMIT) {
			return false;
		}
		for (auto&& slot : slots_) {
			if (slot.type == 0) {
				slot.type = type;
				slot.count = count;
				slot.limit = item_slot::MAXIMUM_LIMIT;
				return true;
			} else if (slot.type == type) {
				if (slot.count + count > slot.limit) {
					const auto diff = slot.limit - (slot.count + count);
					slot.count = slot.limit;
					if (this->push_item(type, diff)) {
						return this->limit_item(type, slot.limit);
					}
				} else {
					slot.count += count;
					return true;
				}
			}
		}
		return false;
	}
	bool remove_item(i32 type) {
		if (type <= 0) {
			return false;
		}
		bool found = false;
		for (auto&& slot : slots_) {
			if (slot.type == type) {
				slot = {};
				this->shift_slots_(std::distance(&slots_[0], &slot));
				found = true;
			}
		}
		return found;
	}
	bool subtract_item(i32 type, i32 count) {
		if (type <= 0 or count <= 0) {
			return false;
		}
		bool found = false;
		for (auto&& slot : slots_) {
			if (slot.type == type) {
				if (count >= slot.count) {
					const auto diff = count - slot.count;
					count -= diff;
					slot = {};
					this->shift_slots_(std::distance(&slots_[0], &slot));
					found = true;
				} else {
					slot.count -= count;
					count = 0;
					found = true;
				}
				if (count <= 0) {
					break;
				}
			}
		}
		return found;
	}
	i32 count_items(i32 type) const {
		if (type <= 0) {
			return 0;
		}
		i32 result = 0;
		for (auto&& slot : slots_) {
			if (slot.type == type) {
				result += slot.count;
			}
		}
		return result;
	}
	i32 id() const { return id_; }
	const std::string& field() const { return field_; }
	const std::string& event() const { return event_; }
	r64 seconds() const;
private:
	void shift_slots_(udx removed);
	internals state_ {};
	i64 ticks_ {};
	udx profile_ {};
	udx cursor_ {};
	udx provision_ { INVALID_SLOT };
	std::vector<item_slot> slots_ {};
	std::vector<u64> flags_ {};
	i32 id_ {};
	std::string field_ {};
	std::string event_ {};
	std::string language_ {};
};
