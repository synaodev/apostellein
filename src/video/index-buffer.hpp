#pragma once

#include <utility>
#include <apostellein/struct.hpp>

struct index_buffer : public not_copyable {
	index_buffer() noexcept = default;
	index_buffer(index_buffer&& that) noexcept {
		*this = std::move(that);
	}
	index_buffer& operator=(index_buffer&& that) noexcept {
		if (this != &that) {
			handle_ = that.handle_;
			that.handle_ = 0;
			length_ = that.length_;
			that.length_ = 0;
		}
		return *this;
	}
	~index_buffer() { this->destroy(); }
public:
	bool quads(udx count);
	void destroy();
	bool valid() const { return handle_ != 0; }
	u32 name() const { return handle_; }
	udx length() const { return length_; }
private:
	u32 handle_ {};
	udx length_ {};
};
