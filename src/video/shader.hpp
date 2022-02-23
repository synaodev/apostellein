#pragma once

#include <string>
#include <apostellein/def.hpp>

#include "./vertex.hpp"

struct const_buffer;
struct frame_buffer;

enum class shader_stage {
	vertex,
	pixel,
	compute
};

struct shader_object : public not_copyable {
	shader_object() noexcept = default;
	shader_object(shader_object&& that) noexcept {
		*this = std::move(that);
	}
	shader_object& operator=(shader_object&& that) noexcept {
		if (this != &that) {
			handle_ = that.handle_;
			that.handle_ = 0;
			stage_ = that.stage_;
			that.stage_ = shader_stage::vertex;
		}
		return *this;
	}
	~shader_object() { this->destroy(); }
public:
	bool create(const std::string& source, shader_stage stage);
	void destroy();
	bool matches(shader_stage stage) const { return stage_ == stage; }
private:
	friend struct shader_program;
	u32 handle_ {};
	shader_stage stage_ { shader_stage::vertex };
};

struct shader_program : public not_copyable {
	shader_program() noexcept = default;
	shader_program(shader_program&& that) noexcept {
		*this = std::move(that);
	}
	shader_program& operator=(shader_program&& that) noexcept {
		if (this != &that) {
			handle_ = that.handle_;
			that.handle_ = 0;
			format_ = that.format_;
			that.format_ = {};
		}
		return *this;
	}
	~shader_program() { this->destroy(); }
public:
	bool create(const shader_object& obj);
	bool create(const shader_object& vtx, const shader_object& pix);
	void destroy();
	void buffer(const std::string& name, const const_buffer& data) const;
	void sampler(const std::string& name, const frame_buffer& data) const;
	void sampler(const std::string& name, i32 value) const;
	void bind() const;
	const vertex_format& format() const { return format_; }
private:
	static u32 current_;
	u32 handle_ {};
	vertex_format format_ {};
};
