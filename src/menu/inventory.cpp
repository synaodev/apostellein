#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./inventory.hpp"
#include "./overlay.hpp"
#include "./headsup.hpp"
#include "./dialogue.hpp"
#include "../ctrl/kernel.hpp"
#include "../ctrl/controller.hpp"
#include "../hw/audio.hpp"
#include "../hw/vfs.hpp"
#include "../util/buttons.hpp"
#include "../util/id-table.hpp"
#include "../x2d/renderer.hpp"

namespace {
	constexpr glm::vec2 FIRST_ELEMENT_POSITION { 2.0f, 2.0f };
	constexpr glm::vec2 ELEMENT_SPACING { 49.0f, 21.0f };
	constexpr glm::vec2 ELEMENT_COUNTER_OFFSET { 38.0f, 10.0f };
	constexpr rect ELEMENT_COUNTER_RASTER { 56.0f, 9.0f, 8.0f, 9.0f };
	constexpr rect DEFAULT_CURSOR_RASTER { 0.0f, 0.0f, 32.0f, 16.0f };
	constexpr chroma CURSOR_COLOR { 0x00U, 0x00U, 0xFFU, 0x4FU }; // Blue
	constexpr chroma PROVISION_COLOR { 0xFFU, 0x00U, 0x00U, 0x4FU }; // Red

	template<typename T>
	constexpr T MAXIMUM_ROWS() { return as<T>(6); }
	template<typename T>
	constexpr T MAXIMUM_COLUMNS() { return as<T>(5); }
	template<typename T>
	constexpr T MODULO_POINT() { return as<T>(6); }

	rect index_to_raster_(udx index) {
		if (index >= controller::MAXIMUM_ITEMS) {
			return DEFAULT_CURSOR_RASTER;
		}
		return {
			FIRST_ELEMENT_POSITION.x + (
				as<r32>(index % MODULO_POINT<udx>()) *
				ELEMENT_SPACING.x
			),
			FIRST_ELEMENT_POSITION.y + (
				as<r32>(index / MODULO_POINT<udx>()) *
				ELEMENT_SPACING.y
			),
			DEFAULT_CURSOR_RASTER.w,
			DEFAULT_CURSOR_RASTER.h
		};
	}
	glm::ivec2 index_to_vec_(udx index) {
		if (index >= controller::MAXIMUM_ITEMS) {
			return {};
		}
		return {
			as<i32>(index % MODULO_POINT<udx>()),
			as<i32>(index / MODULO_POINT<udx>())
		};
	}
	rect vec_to_raster_(const glm::ivec2& vec) {
		if (
			vec.x < 0 or
			vec.y < 0 or
			vec.x >= MAXIMUM_ROWS<i32>() or
			vec.y >= MAXIMUM_COLUMNS<i32>()
		) {
			return DEFAULT_CURSOR_RASTER;
		}
		return {
			FIRST_ELEMENT_POSITION.x + (
				as<r32>(vec.x) *
				ELEMENT_SPACING.x
			),
			FIRST_ELEMENT_POSITION.y + (
				as<r32>(vec.y) *
				ELEMENT_SPACING.y
			),
			DEFAULT_CURSOR_RASTER.w,
			DEFAULT_CURSOR_RASTER.h
		};
	}
	udx vec_to_index(const glm::ivec2& vec) {
		if (
			vec.x < 0 or
			vec.y < 0 or
			vec.x >= MAXIMUM_ROWS<i32>() or
			vec.y >= MAXIMUM_COLUMNS<i32>()
		) {
			return controller::INVALID_SLOT;
		}
		return (
			as<udx>(vec.x) +
			as<udx>(vec.y) *
			MAXIMUM_ROWS<udx>()
		);
	}
}

void inventory::build() {
	this->clear();

	auto items_file = vfs::find_animation(anim::Items);
	auto amount_texture = vfs::find_material(img::Heads);

	for (auto&& elm : elements_) {
		elm.build(
			std::distance(elements_.data(), &elm),
			MAXIMUM_ROWS<udx>(),
			FIRST_ELEMENT_POSITION,
			ELEMENT_SPACING,
			items_file, true,
			ELEMENT_COUNTER_OFFSET,
			ELEMENT_COUNTER_RASTER,
			amount_texture
		);
	}
}

void inventory::clear() {
	invalidated_ = true;
	active_ = false;
	provision_ = controller::INVALID_SLOT;
	cursor_raster_ = DEFAULT_CURSOR_RASTER;
	provision_raster_ = DEFAULT_CURSOR_RASTER;
	if (elements_.empty()) {
		elements_.resize(controller::MAXIMUM_ITEMS);
	}
}

void inventory::handle(
	const buttons& bts,
	controller& ctl,
	kernel& knl,
	const overlay& ovl,
	headsup& hud,
	const dialogue& dlg
) {
	if (ovl.empty()) {
		if (!active_) {
			if (!knl.running() and bts.pressed.inventory) {
				invalidated_ = true;
				active_ = true;
				ctl.freeze();
				cursor_raster_ = index_to_raster_(ctl.cursor());
				if (ctl.provision_valid()) {
					provision_ = ctl.provision();
					provision_raster_ = index_to_raster_(provision_);
				}
				for (udx idx = 0; idx < controller::MAXIMUM_ITEMS; ++idx) {
					elements_[idx].set(ctl.item_at(idx));
				}
				hud.clear_title();
				audio::play(sfx::Inven, 0);
			}
		} else if (bts.pressed.inventory) {
			if (!knl.running()) {
				invalidated_ = true;
				active_ = false;
				ctl.unlock();
				audio::play(sfx::Inven, 0);
			}
		} else {
			auto cursor = ctl.cursor();
			auto cursor_2D = index_to_vec_(cursor);
			if (!dlg.textbox_open()) {
				if (bts.pressed.confirm and !knl.running()) {
					// description event
					invalidated_ = true;
					const auto item = ctl.item_at(cursor);
					knl.run_inventory(as<u32>(item.type));
				} else if (bts.pressed.provision) {
					// provision item
					invalidated_ = true;
					provision_ = vec_to_index(cursor_2D);
					ctl.provision(provision_);
					audio::play(sfx::TitleBeg, 0);
				} else if (bts.pressed.cancel and provision_ != controller::INVALID_SLOT) {
					// de-provision item
					invalidated_ = true;
					provision_ = controller::INVALID_SLOT;
					ctl.provision(provision_);
					audio::play(sfx::TitleBeg, 0);
				} else if (bts.pressed.right) {
					// scrolling
					if (cursor_2D.x < MAXIMUM_ROWS<i32>() - 1) {
						invalidated_ = true;
						++cursor_2D.x;
						audio::play(sfx::Select, 0);
					}
				} else if (bts.pressed.left) {
					// scrolling
					if (cursor_2D.x > 0) {
						invalidated_ = true;
						--cursor_2D.x;
						audio::play(sfx::Select, 0);
					}
				} else if (bts.pressed.up) {
					// scrolling
					if (cursor_2D.y > 0) {
						invalidated_ = true;
						--cursor_2D.y;
						audio::play(sfx::Select, 0);
					}
				} else if (bts.pressed.down) {
					// scrolling
					if (cursor_2D.y < MAXIMUM_COLUMNS<i32>() - 1) {
						invalidated_ = true;
						++cursor_2D.y;
						audio::play(sfx::Select, 0);
					}
				}
			}
			ctl.cursor(vec_to_index(cursor_2D));
			if (invalidated_) {
				cursor_raster_ = vec_to_raster_(cursor_2D);
				if (provision_ != controller::INVALID_SLOT) {
					provision_raster_ = index_to_raster_(provision_);
				}
			}
		}
	}
}

void inventory::render(renderer& rdr) const {
	if (active_) {
		for (auto&& elm : elements_) {
			elm.render(rdr);
		}
		auto& list = rdr.query(
			priority_type::deferred,
			blending_type::alpha,
			pipeline_type::blank
		);
		if (invalidated_) {
			invalidated_ = false;
			list.batch_blank(
				konst::WINDOW_DIMENSIONS<r32>(),
				chroma::TRANSLUCENT()
			);
			list.batch_blank(
				cursor_raster_,
				CURSOR_COLOR
			);
			if (provision_ != controller::INVALID_SLOT) {
				list.batch_blank(
					provision_raster_,
					PROVISION_COLOR
				);
			}
		} else {
			const auto count = provision_ == controller::INVALID_SLOT ?
				display_list::QUAD * 2 :
				display_list::QUAD * 3;
			list.skip(count);
		}
	}
}
