#pragma once

#include <array>
#include <memory>
#include <apostellein/rect.hpp>

#include "./priority-type.hpp"
#include "../video/quad-buffer.hpp"
#include "../video/blending-type.hpp"

struct material;
struct mirror_type;

enum class pipeline_type : udx {
	blank,
	sprite,
	glyph,
	light
};

struct display_list : public not_copyable {
	display_list() noexcept = default;
	display_list(
		priority_type priority,
		blending_type blending,
		pipeline_type pipeline,
		std::unique_ptr<quad_buffer> quads
	) : priority_{ priority },
		blending_{ blending },
		pipeline_{ pipeline },
		quads_{ std::move(quads) } {}
	display_list(display_list&& that) noexcept {
		*this = std::move(that);
	}
	display_list& operator=(display_list&& that) noexcept {
		if (this != &that) {
			visible_ = that.visible_;
			that.visible_ = false;
			length_ = that.length_;
			that.length_ = 0;
			stored_ = that.stored_;
			that.stored_ = 0;
			quads_ = std::move(that.quads_);
			priority_ = that.priority_;
			that.priority_ = priority_type::automatic;
			blending_ = that.blending_;
			that.blending_ = blending_type::alpha;
			pipeline_ = that.pipeline_;
			that.pipeline_ = pipeline_type::sprite;
		}
		return *this;
	}
	~display_list() = default;
public:
	static constexpr udx QUAD = 4;
	void batch_blank(const rect& raster,const chroma& color);
	void batch_sprite(
		const glm::vec2& position,
		const glm::vec2& raster,
		const rect& uvs,
		const material& texture
	);
	void batch_sprite(
		const rect& raster,
		const rect& uvs,
		const material& texture
	) {
		this->batch_sprite(
			raster.position(),
			raster.dimensions(),
			uvs,
			texture
		);
	}
	void batch_sprite(
		const std::array<glm::vec2, 4>& raster,
		const rect& uvs,
		const material& texture,
		const mirror_type& mirror,
		const chroma& color
	);
	void batch_parallax(
		const rect& view,
		const glm::vec2& shift,
		const glm::vec2& raster,
		const material& texture
	);
	template<typename V>
	void upload(const std::vector<V>& vertices, udx count) {
		static_assert(std::is_base_of<vtx_type, V>::value);
		this->batch_begin_(count);
		if (stored_ > 0) {
			quads_->copy<V>(vertices, length_, count);
		}
		this->batch_end_();
	}
	template<typename V>
	void upload(const std::vector<V>& vertices) {
		this->upload<V>(vertices, vertices.size());
	}
	void skip(udx count);
	void flush(const shader_program& program);
	bool visible() const {
		if (length_ > 0) {
			return true;
		}
		return visible_;
	}
	bool matches(priority_type priority, blending_type blending, pipeline_type pipeline) const {
		return (
			priority_ == priority and
			blending_ == blending and
			pipeline_ == pipeline
		);
	}
	bool operator<(const display_list& that) const {
		if (priority_ == that.priority_) {
			if (blending_ == that.blending_) {
				return pipeline_ < that.pipeline_;
			}
			return blending_ < that.blending_;
		}
		return priority_ < that.priority_;
	}
	const priority_type& priority() const { return priority_; }
	const blending_type& blending() const { return blending_; }
	const pipeline_type& pipeline() const { return pipeline_; }
private:
	void batch_begin_(udx count);
	void batch_end_();
	bool visible_ {};
	udx length_ {};
	udx stored_ {};
	std::unique_ptr<quad_buffer> quads_ {};
	priority_type priority_ { priority_type::automatic };
	blending_type blending_ { blending_type::alpha };
	pipeline_type pipeline_ { pipeline_type::sprite };
};
