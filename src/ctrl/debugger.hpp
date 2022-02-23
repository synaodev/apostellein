#pragma once

#include <apostellein/def.hpp>

enum class activity_type;
struct config_file;
struct buttons;
struct renderer;
struct runtime;

#if defined(APOSTELLEIN_IMGUI_DEBUGGER)

struct debugger {
	~debugger();
public:
	bool build(const config_file& cfg, const renderer& rdr);
	void handle(const buttons& bts, runtime& state);
	void update(i64 delta);
	void flush() const;
private:
	void ui_(runtime& state);
	void* window_ {};
	const renderer* rdr_ {};
	i64 timer_ {};
	i64 fading_ {};
	i64 frames_ {};
	bool visible_ {};
};

#else

struct debugger {
public:
	bool build(const config_file&, const renderer&) const { return true; }
	void handle(const buttons&, const runtime&) const {}
	void update(i64) const {}
	void flush() const {}
};

#endif
