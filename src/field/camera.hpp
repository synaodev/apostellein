#pragma once

#include <glm/mat4x4.hpp>
#include <apostellein/rect.hpp>

struct environment;
struct player;

struct camera {
public:
	void clear();
	void prepare();
	void handle(const player& plr, const environment& env);
	void limit(const rect& bounds);
	void focus_on(const glm::vec2& point);
	void follow(i32 id) {
		id_ = id;
	}
	void quake(r32 power, i32 ticks);
	void quake(r32 power);
	rect view() const {
		return {
			current_ - (dimensions_ / 2.0f),
			dimensions_
		};
	}
	rect view(r32 ratio) const;
	glm::mat4 matrix(r32 ratio) const;
private:
	bool cycling_ {};
	bool indefinite_ {};
	i32 id_ {};
	i32 ticks_ {};
	rect limit_ {};
	glm::vec2 previous_ {};
	glm::vec2 current_ {};
	glm::vec2 dimensions_ {};
	glm::vec2 offsets_ {};
	r32 power_ {};
	r32 tilt_ {};
};
