#pragma once

#include <array>
#include <string>
#include <vector>
#include <glm/fwd.hpp>
#include <apostellein/rect.hpp>
#include <apostellein/struct.hpp>

struct mirror_type;
struct material;
struct renderer;

struct animation_frame {
	constexpr animation_frame() noexcept = default;
	constexpr animation_frame(
		const glm::vec2& _position,
		const glm::vec2& _origin
	) noexcept : position{ _position }, origin{ _origin } {}

	glm::vec2 position {};
	glm::vec2 origin {};
};

struct animation_raster {
	animation_raster() noexcept = default;
	animation_raster(
		const glm::vec2& position,
		const glm::vec2& dimensions
	) noexcept;
	animation_raster(
		const rect& quad,
		r32 angle,
		const glm::vec2& about
	) noexcept;

	rect bounds {};
	std::array<glm::vec2, 4> points {};
};

struct animation_sequence : public not_copyable {
	animation_sequence() noexcept = default;
	animation_sequence(const glm::vec2& dimensions, i64 delay, udx count, bool repeating, bool reflecting) noexcept :
		dimensions_{ dimensions },
		delay_{ delay },
		count_{ count },
		repeating_{ repeating },
		reflecting_{ reflecting } {}
	animation_sequence(animation_sequence&& that) noexcept {
		*this = std::move(that);
	}
	animation_sequence& operator=(animation_sequence&& that) noexcept {
		if (this != &that) {
			frames_ = std::move(that.frames_);
			that.frames_.clear();
			action_points_ = std::move(that.action_points_);
			that.action_points_.clear();
			dimensions_ = that.dimensions_;
			that.dimensions_ = {};
			delay_ = that.delay_;
			that.delay_ = 0;
			count_ = that.count_;
			that.count_ = 0;
			repeating_ = that.repeating_;
			that.repeating_ = true;
			reflecting_ = that.reflecting_;
			that.reflecting_ = false;
		}
		return *this;
	}
	~animation_sequence() = default;
public:
	void append(const glm::vec2& action_point);
	void append(const glm::vec2& start, const glm::vec4& points);
	void update(i64 delta, i64& timer, udx& frame) const;
	void update(i64 delta, bool& invalidated, i64& timer, udx& frame) const;
	const animation_frame& frame_with(udx frame, udx variation) const;
	rect quad_with(udx frame, udx variation) const;
	rect unsafe_quad_with(udx idx) const;
	animation_raster raster_with(
		udx frame,
		udx variation,
		const mirror_type& mirror,
		const glm::vec2& scale,
		const glm::vec2& position
	) const;
	animation_raster raster_with(
		udx frame,
		udx variation,
		const mirror_type& mirror,
		const glm::vec2& scale,
		r32 angle,
		const glm::vec2& pivot,
		const glm::vec2& position
	) const;
	glm::vec2 dimensions() const { return dimensions_; }
	glm::vec2 origin_with(udx frame, udx variation, const mirror_type& mirror) const;
	glm::vec2 unsafe_origin_with(udx idx, const mirror_type& mirror) const;
	glm::vec2 action_point_with(udx variation, const mirror_type& mirror) const;
	bool finished(udx frame, i64 timer) const;
private:
	std::vector<animation_frame> frames_ {};
	std::vector<glm::vec2> action_points_ {};
	glm::vec2 dimensions_ {};
	i64 delay_ {};
	udx count_ {};
	bool repeating_ { true };
	bool reflecting_ {};
};

struct animation_group : public not_copyable {
	animation_group() noexcept = default;
	animation_group(animation_group&& that) noexcept {
		*this = std::move(that);
	}
	animation_group& operator=(animation_group&& that) noexcept {
		if (this != &that) {
			sequences_ = std::move(that.sequences_);
			that.sequences_.clear();
			texture_ = that.texture_;
			that.texture_ = nullptr;
		}
		return *this;
	}
	~animation_group() = default;
public:
	void update(i64 delta, udx state, i64& timer, udx& frame) const;
	void update(i64 delta, bool& invalidated, udx state, i64& timer, udx& frame) const;
	void render(
		udx state,
		udx frame,
		udx variation,
		const mirror_type& mirror,
		const chroma& color,
		const glm::vec2& scale,
		r32 angle,
		const glm::vec2& pivot,
		const glm::vec2& position,
		const rect& view,
		renderer& rdr
	) const;
	bool visible(
		udx state,
		udx frame,
		udx variation,
		const mirror_type& mirror,
		const glm::vec2& scale,
		r32 angle,
		const glm::vec2& pivot,
		const glm::vec2& position,
		const rect& view
	) const;
	void render(
		udx state,
		udx frame,
		udx variation,
		const mirror_type& mirror,
		const chroma& color,
		const glm::vec2& scale,
		const glm::vec2& position,
		const rect& view,
		renderer& rdr
	) const;
	bool visible(
		udx state,
		udx frame,
		udx variation,
		const mirror_type& mirror,
		const glm::vec2& scale,
		const glm::vec2& position,
		const rect& view
	) const;
	void render(
		bool& invalidated,
		udx state,
		udx frame,
		udx variation,
		const glm::vec2& position,
		renderer& rdr
	) const;
	void load(const std::string& path);
	bool finished(udx state, udx frame, i64 timer) const;
	bool ready() const { return !sequences_.empty(); }
	glm::vec2 origin(udx state, udx frame, udx variation, const mirror_type& mirror) const;
	glm::vec2 action_point(udx state, udx variation, const mirror_type& mirror) const;
private:
	std::vector<animation_sequence> sequences_ {};
	const material* texture_ {};
};
