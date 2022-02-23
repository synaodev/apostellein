#pragma once

#include <utility>
#include <apostellein/struct.hpp>

struct const_buffer : public not_copyable {
	const_buffer() noexcept = default;
	const_buffer(const_buffer&& that) noexcept {
		*this = std::move(that);
	}
	const_buffer& operator=(const_buffer&& that) noexcept {
		if (this != &that) {
			handle_ = that.handle_;
			that.handle_ = 0;
			binding_ = that.binding_;
			that.binding_ = 0;
		}
		return *this;
	}
	virtual ~const_buffer() { this->destroy(); }
public:
	bool create(u32 binding);
	void destroy();
	bool valid() const { return handle_ != 0; }
	u32 binding() const { return binding_; }
	virtual udx length() const = 0;
protected:
	u32 handle_ {};
	u32 binding_ {};
};
