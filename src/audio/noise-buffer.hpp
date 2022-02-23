#pragma once

#include <string>
#include <apostellein/struct.hpp>

struct speaker;

struct noise_buffer : public not_copyable {
	noise_buffer() noexcept = default;
	noise_buffer(noise_buffer&& that) noexcept {
		*this = std::move(that);
	}
	noise_buffer& operator=(noise_buffer&& that) noexcept {
		if (this != &that) {
			ready_ = that.ready_;
			that.ready_ = false;
			handle_ = that.handle_;
			that.handle_ = 0;
		}
		return *this;
	}
	~noise_buffer() { this->destroy(); }
public:
	void load(const std::string& path);
	void destroy();
	bool valid() const { return ready_; }
private:
	friend struct speaker;
	bool ready_ {};
	u32 handle_ {};
};
