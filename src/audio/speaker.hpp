#pragma once

#include <apostellein/struct.hpp>

struct noise_buffer;

struct speaker : public not_copyable {
	speaker() noexcept = default;
	speaker(speaker&& that) noexcept {
		*this = std::move(that);
	}
	speaker& operator=(speaker&& that) noexcept {
		if (this != &that) {
			ready_ = that.ready_;
			that.ready_ = false;
			handle_ = that.handle_;
			that.handle_ = 0;
			current_ = that.current_;
			that.current_ = nullptr;
		}
		return *this;
	}
	~speaker() { this->destroy(); }
public:
	void create();
	void destroy();
	bool bind(const noise_buffer* noise);
	void unbind();
	r32 volume() const;
	void volume(r32 value);
	void play();
	bool playing() const;
	void stop();
	bool stopped() const;
	void pause();
	bool paused() const;
private:
	bool matches_state_(i32 name) const;
	bool ready_ {};
	u32 handle_ {};
	const noise_buffer* current_ {};
};
