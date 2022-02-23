#pragma once

#include "./text.hpp"

namespace gui {
	struct title : public not_copyable {
	public:
		using callback = const bitmap_font*(*)(udx);
		void build(
			const glm::vec2& position,
			const glm::vec2& origin,
			const chroma& back_color,
			const chroma& front_color,
			const bitmap_font* font,
			gui::title::callback recalibrate
		);
		void clear();
		void invalidate() const {
			front_text_.invalidate();
			back_text_.invalidate();
			for (auto&& msg : messages_) {
				msg.invalidate();
			}
		}
		void fix(const bitmap_font* font);
		void update(i64 delta) {
			if (timer_ > 0) {
				if (timer_ -= delta; timer_ <= 0) {
					this->clear();
				}
			}
		}
		void render(renderer& rdr) const;
		template<typename T>
		void display(const std::basic_string<T>& words) {
			timer_ = title::default_timer_();
			back_text_.replace(words);
			const rect bounds = back_text_.bounds();
			back_text_.origin(bounds.w / 2.0f, 0.0f);
			front_text_.origin(bounds.w / 2.0f + 1.0f, 1.0f);
			front_text_.replace(words);
		}
		void push_message(
			bool centered,
			const glm::vec2& position,
			udx index,
			const bitmap_font* font,
			const std::string& words
		);
		void forward_message(
			bool centered,
			const glm::vec2& position,
			udx index,
			const bitmap_font* font,
			std::u32string&& words
		);
		bool displaying() const { return timer_ > 0; }
		bool visible() const { return timer_ > 0 or messages_.empty(); }
	private:
		static i64 default_timer_();
		i64 timer_ {};
		gui::text back_text_ {};
		gui::text front_text_ {};
		std::vector<gui::text> messages_ {};
		std::vector<udx> indices_ {};
		const bitmap_font*(*recalibrate_)(udx);
	};
}
