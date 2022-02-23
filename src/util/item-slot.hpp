#pragma once

#include <apostellein/def.hpp>

struct item_slot {
	i32 type {};
	i32 count {};
	i32 limit {};
	bool weapon {};
public:
	static constexpr i32 MAXIMUM_LIMIT = 9999;
	bool operator==(const item_slot& that) const {
		return (
			this->type == that.type and
			this->count == that.count and
			this->limit == that.limit and
			this->weapon == that.weapon
		);
	}
	bool operator!=(const item_slot& that) const {
		return !(*this == that);
	}
};
