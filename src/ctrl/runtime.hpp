#pragma once

#include "./debugger.hpp"
#include "./controller.hpp"
#include "./kernel.hpp"
#include "../menu/overlay.hpp"
#include "../menu/headsup.hpp"
#include "../menu/dialogue.hpp"
#include "../menu/inventory.hpp"
#include "../field/camera.hpp"
#include "../field/player.hpp"
#include "../field/environment.hpp"
#include "../x2d/tile-map.hpp"

enum class activity_type;

struct runtime {
public:
	bool build(const config_file& cfg, const renderer& rdr);
	void handle(udx ticks, activity_type& aty, buttons& bts);
	void update(i64 delta);
	void render(r32 ratio, renderer& rdr) const;
private:
	void boot_();
	bool save_();
	bool load_();
	bool transfer_();
	controller ctl_ {};
	kernel knl_ {};
	overlay ovl_ {};
	headsup hud_ {};
	dialogue dlg_ {};
	inventory ivt_ {};
	camera cam_ {};
	player plr_ {};
	environment env_ {};
	tile_map map_ {};
	debugger dbr_ {};
	friend struct debugger;
};
