#pragma once

#include <utility>
#include <glm/vec2.hpp>
#include <apostellein/struct.hpp>

struct frame_buffer : public not_copyable {
	frame_buffer() noexcept = default;
	frame_buffer(frame_buffer&& that) noexcept {
		*this = std::move(that);
	}
	frame_buffer& operator=(frame_buffer&& that) noexcept {
		if (this != &that) {
			handle_ = that.handle_;
			that.handle_ = 0;
			buffer_ = that.buffer_;
			that.buffer_ = 0;
			dimensions_ = that.dimensions_;
			that.dimensions_ = {};
			binding_ = that.binding_;
			that.binding_ = 0;
		}
		return *this;
	}
	~frame_buffer() { this->destroy(); }
public:
	bool create(const glm::ivec2& dimensions, i32 binding);
	void destroy();
	template<typename F>
	void execute(const F& function, const chroma& color) {
		static_assert(std::is_function<F>::value);
		assert(handle_);
		this->start_(color);
		std::invoke(function);
		this->finish_();
	}
	template<typename F>
	void execute(const F& function) {
		this->execute(function, chroma::TRANSLUCENT());
	}
	bool blit(frame_buffer& target) const;
	bool blit() const;
	bool valid() const { return handle_ != 0; }
	i32 binding() const { return binding_; }
private:
	void start_(const chroma& color);
	void finish_();
	u32 handle_ {};
	u32 buffer_ {};
	glm::ivec2 dimensions_ {};
	i32 binding_ {};
};
