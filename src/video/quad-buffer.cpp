#include <array>
#include <spdlog/spdlog.h>
#include <apostellein/cast.hpp>

#include "./quad-buffer.hpp"
#include "./index-buffer.hpp"
#include "./shader.hpp"
#include "./opengl.hpp"

namespace {
	constexpr udx MAXIMUM_SECTORS = 3;
}

quad_buffer::quad_buffer(const index_buffer& indices, const vertex_format& format) {
	format_ = format;
	length_ = INDICES_TO_VERTICES(indices.length());
}

struct binding_quad_buffer : public quad_buffer {
	binding_quad_buffer(const index_buffer& indices, const vertex_format& format) : quad_buffer{ indices, format } {
		// allocated up here for exception safety since
		// destructors aren't called if a constuctor throws
		staging_ = std::make_unique<char[]>(format_.size * length_);

		glCheck(glGenVertexArrays(1, &handle_));
		glCheck(glGenBuffers(1, &buffer_));

		glCheck(glBindVertexArray(handle_));
		glCheck(glBindBuffer(
			GL_ELEMENT_ARRAY_BUFFER,
			indices.name()
		));
		glCheck(glBindBuffer(
			GL_ARRAY_BUFFER,
			buffer_
		));
		if (ogl::buffer_storage_available()) {
			glCheck(glBufferStorage(
				GL_ARRAY_BUFFER,
				format_.size * length_,
				nullptr,
				GL_DYNAMIC_STORAGE_BIT
			));
		} else {
			glCheck(glBufferData(
				GL_ARRAY_BUFFER,
				format_.size * length_,
				nullptr,
				GL_DYNAMIC_DRAW
			));
		}
		format_.detail();
		glCheck(glBindVertexArray(0));
		glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		glCheck(glBindBuffer(GL_ARRAY_BUFFER, 0));
	}
	virtual ~binding_quad_buffer() {
		if (buffer_ != 0) {
			glCheck(glDeleteBuffers(1, &buffer_));
		}
		if (handle_ != 0) {
			glCheck(glDeleteVertexArrays(1, &handle_));
		}
	}
public:
	bool draw(const shader_program& program, udx count) noexcept override {
		if (count > length_) {
			spdlog::error("Cannot draw quad buffer! Reason: Too many vertices");
			return false;
		}
		if (invalidated_) {
			glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffer_));
			glCheck(glBufferSubData(
				GL_ARRAY_BUFFER, 0,
				count * format_.size,
				staging_.get()
			));
		}
		glCheck(glBindVertexArray(handle_));
		program.bind();
		glCheck(glDrawElements(
			GL_TRIANGLES,
			VERTICES_TO_INDICES<i32>(count),
			GL_UNSIGNED_SHORT,
			nullptr
		));
		invalidated_ = false;
		return true;
	}
	bool valid() const noexcept override {
		return true;
	}
protected:
	char* staging(udx index) noexcept override {
		invalidated_ = true;
		return staging_.get() + (index * format_.size);
	}
private:
	u32 handle_ {};
	u32 buffer_ {};
	bool invalidated_ {};
	std::unique_ptr<char[]> staging_ {};
};

struct binding_quad_stream : public quad_buffer {
	binding_quad_stream(const index_buffer& indices, const vertex_format& format) : quad_buffer{ indices, format } {
		glCheck(glGenVertexArrays(1, &handle_));
		glCheck(glGenBuffers(1, &buffer_));

		glCheck(glBindVertexArray(handle_));
		glCheck(glBindBuffer(
			GL_ELEMENT_ARRAY_BUFFER,
			indices.name()
		));
		glCheck(glBindBuffer(
			GL_ARRAY_BUFFER,
			buffer_
		));
		glCheck(glBufferStorage(
			GL_ARRAY_BUFFER,
			format_.size * length_ * MAXIMUM_SECTORS,
			nullptr,
			GL_MAP_WRITE_BIT |
			GL_MAP_PERSISTENT_BIT |
			GL_MAP_COHERENT_BIT
		));
		glCheck(staging_ = glMapBufferRange(
			GL_ARRAY_BUFFER, 0,
			format_.size * length_ * MAXIMUM_SECTORS,
			GL_MAP_WRITE_BIT |
			GL_MAP_PERSISTENT_BIT |
			GL_MAP_COHERENT_BIT |
			GL_MAP_UNSYNCHRONIZED_BIT
		));
		format_.detail();
		glCheck(glBindVertexArray(0));
		glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		glCheck(glBindBuffer(GL_ARRAY_BUFFER, 0));
	}
	virtual ~binding_quad_stream() {
		for (auto&& fence : fences_) {
			if (fence) {
				glCheck(glDeleteSync(fence));
			}
		}
		if (buffer_ != 0) {
			if (staging_) {
				glCheck(glBindBuffer(GL_ARRAY_BUFFER, buffer_));
				glCheck(glUnmapBuffer(GL_ARRAY_BUFFER));
			}
			glCheck(glDeleteBuffers(1, &buffer_));
		}
		if (handle_ != 0) {
			glCheck(glDeleteVertexArrays(1, &handle_));
		}
	}
public:
	bool draw(const shader_program& program, udx count) noexcept override {
		if (count > length_) {
			spdlog::error("Cannot draw quad buffer! Reason: Too many vertices");
			return false;
		}
		glCheck(glBindVertexArray(handle_));
		program.bind();
		glCheck(glDrawElementsBaseVertex(
			GL_TRIANGLES,
			VERTICES_TO_INDICES<i32>(count),
			GL_UNSIGNED_SHORT,
			nullptr,
			as<i32>(length_ * sector_)
		));
		if (fences_[sector_]) {
			glCheck(glDeleteSync(fences_[sector_]));
		}
		glCheck(fences_[sector_] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
		sector_ = (sector_ + 1) % MAXIMUM_SECTORS;
		flags_ = GL_SYNC_FLUSH_COMMANDS_BIT;
		return true;
	}
	bool valid() const noexcept override {
		return staging_ != nullptr;
	}
protected:
	char* staging(udx index) noexcept override {
		if (flags_ and fences_[sector_]) {
			u32 result = GL_UNSIGNALED;
			do {
				glCheck(result = glClientWaitSync(fences_[sector_], flags_, 1));
				flags_ = 0;
			} while (result != GL_CONDITION_SATISFIED and result != GL_ALREADY_SIGNALED);
		}
		return reinterpret_cast<char*>(staging_) +
			(sector_ * length_ * format_.size) +
			(index * format_.size);
	}
private:
	u32 handle_ {};
	u32 buffer_ {};
	void* staging_ {};
	udx sector_ {};
	u32 flags_ {};
	std::array<GLsync, MAXIMUM_SECTORS> fences_ {};
};

struct direct_quad_buffer : public quad_buffer {
	direct_quad_buffer(const index_buffer& indices, const vertex_format& format) : quad_buffer{ indices, format } {
		// allocated up here for exception safety since
		// destructors aren't called if a constuctor throws
		staging_ = std::make_unique<char[]>(format_.size * length_);

		glCheck(glCreateBuffers(1, &buffer_));
		glCheck(glNamedBufferStorage(
			buffer_,
			format_.size * length_,
			nullptr,
			GL_DYNAMIC_STORAGE_BIT
		));
		glCheck(glGenVertexArrays(1, &handle_));
		glCheck(glBindVertexArray(handle_));
		glCheck(glBindBuffer(
			GL_ELEMENT_ARRAY_BUFFER,
			indices.name()
		));
		glCheck(glBindBuffer(
			GL_ARRAY_BUFFER,
			buffer_
		));
		format_.detail();
		glCheck(glBindVertexArray(0));
		glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		glCheck(glBindBuffer(GL_ARRAY_BUFFER, 0));
	}
	virtual ~direct_quad_buffer() {
		if (buffer_ != 0) {
			glCheck(glDeleteBuffers(1, &buffer_));
		}
		if (handle_ != 0) {
			glCheck(glDeleteVertexArrays(1, &handle_));
		}
	}
public:
	bool draw(const shader_program& program, udx count) noexcept override {
		if (count > length_) {
			spdlog::error("Cannot draw quad buffer! Reason: Too many vertices");
			return false;
		}
		if (invalidated_) {
			glCheck(glNamedBufferSubData(
				buffer_, 0,
				count * format_.size,
				staging_.get()
			));
			spdlog::info("invalidated");
		}
		glCheck(glBindVertexArray(handle_));
		program.bind();
		glCheck(glDrawElements(
			GL_TRIANGLES,
			VERTICES_TO_INDICES<i32>(count),
			GL_UNSIGNED_SHORT,
			nullptr
		));
		invalidated_ = false;
		return true;
	}
	bool valid() const noexcept override {
		return true;
	}
protected:
	char* staging(udx index) noexcept override {
		invalidated_ = true;
		return staging_.get() + (index * format_.size);
	}
private:
	u32 handle_ {};
	u32 buffer_ {};
	bool invalidated_ {};
	std::unique_ptr<char[]> staging_ {};
};

struct direct_quad_stream : public quad_buffer {
	direct_quad_stream(const index_buffer& indices, const vertex_format& format) : quad_buffer{ indices, format } {
		glCheck(glCreateBuffers(1, &buffer_));
		glCheck(glNamedBufferStorage(
			buffer_,
			format_.size * length_ * MAXIMUM_SECTORS,
			nullptr,
			GL_MAP_WRITE_BIT |
			GL_MAP_PERSISTENT_BIT |
			GL_MAP_COHERENT_BIT
		));
		glCheck(staging_ = glMapNamedBufferRange(
			buffer_, 0,
			format_.size * length_ * MAXIMUM_SECTORS,
			GL_MAP_WRITE_BIT |
			GL_MAP_PERSISTENT_BIT |
			GL_MAP_COHERENT_BIT |
			GL_MAP_UNSYNCHRONIZED_BIT
		));
		glCheck(glGenVertexArrays(1, &handle_));
		glCheck(glBindVertexArray(handle_));
		glCheck(glBindBuffer(
			GL_ELEMENT_ARRAY_BUFFER,
			indices.name()
		));
		glCheck(glBindBuffer(
			GL_ARRAY_BUFFER,
			buffer_
		));
		format_.detail();
		glCheck(glBindVertexArray(0));
		glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		glCheck(glBindBuffer(GL_ARRAY_BUFFER, 0));
	}
	virtual ~direct_quad_stream() {
		for (auto&& fence : fences_) {
			if (fence) {
				glCheck(glDeleteSync(fence));
			}
		}
		if (buffer_ != 0) {
			if (staging_) {
				glCheck(glUnmapNamedBuffer(buffer_));
			}
			glCheck(glDeleteBuffers(1, &buffer_));
		}
		if (handle_ != 0) {
			glCheck(glDeleteVertexArrays(1, &handle_));
		}
	}
	bool draw(const shader_program& program, udx count) noexcept override {
		if (count > length_) {
			spdlog::error("Cannot draw quad buffer! Reason: Too many vertices");
			return false;
		}
		glCheck(glBindVertexArray(handle_));
		program.bind();
		glCheck(glDrawElementsBaseVertex(
			GL_TRIANGLES,
			VERTICES_TO_INDICES<i32>(count),
			GL_UNSIGNED_SHORT,
			nullptr,
			as<i32>(length_ * sector_)
		));
		if (fences_[sector_]) {
			glCheck(glDeleteSync(fences_[sector_]));
		}
		glCheck(fences_[sector_] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
		sector_ = (sector_ + 1) % MAXIMUM_SECTORS;
		flags_ = GL_SYNC_FLUSH_COMMANDS_BIT;
		return true;
	}
	bool valid() const noexcept override {
		return staging_ != nullptr;
	}
protected:
	char* staging(udx index) noexcept override {
		if (flags_ and fences_[sector_]) {
			u32 result = GL_UNSIGNALED;
			do {
				glCheck(result = glClientWaitSync(fences_[sector_], flags_, 1));
				flags_ = 0;
			} while (result != GL_CONDITION_SATISFIED and result != GL_ALREADY_SIGNALED);
		}
		return reinterpret_cast<char*>(staging_) +
			(sector_ * length_ * format_.size) +
			(index * format_.size);
	}
private:
	u32 handle_ {};
	u32 buffer_ {};
	void* staging_ {};
	udx sector_ {};
	u32 flags_ {};
	std::array<GLsync, MAXIMUM_SECTORS> fences_ {};
};

std::unique_ptr<quad_buffer> quad_buffer::allocate(
	const index_buffer& indices,
	const vertex_format& format,
	bool streaming
) {
	// sanity checks
	if (!indices.valid()) {
		static constexpr char message[] = "Cannot allocate quad buffer! Reason: Passed indices are invalid!";
		spdlog::critical(message);
		throw std::runtime_error(message);
	}
	if (!format.detail or format.size == 0) {
		static constexpr char message[] = "Cannot allocate quad buffer! Reason: Passed format is invalid!";
		spdlog::critical(message);
		throw std::runtime_error(message);
	}
	if (streaming and !ogl::buffer_storage_available()) {
		streaming = false;
	}

	// choose your buffer
	std::unique_ptr<quad_buffer> result {};

	if (streaming) {
		if (ogl::direct_state_available()) {
			result = std::make_unique<direct_quad_stream>(indices, format);
		} else {
			result = std::make_unique<binding_quad_stream>(indices, format);
		}
	} else {
		if (ogl::direct_state_available()) {
			result = std::make_unique<direct_quad_buffer>(indices, format);
		} else {
			result = std::make_unique<binding_quad_buffer>(indices, format);
		}
	}

	if (!result->valid()) {
		static constexpr char message[] = "Cannot allocate quad buffer! Reason: Out of memory";
		spdlog::critical(message);
		throw std::runtime_error(message);
	}
	return result;
}
