#include "Story.hpp"

 // StartCamera, DoorCamera, FoodCamera, BedCamera, UnderBedCamera, VentCamera, BallCamera, ChestCamera, CandleCamera

Story make_story() {
	Story story;
	story.start = "START";

	story.nodes["START"] = {
		.camera = "StartCamera",
		.text = "You wake on cold stone. The cell door is locked, the air is stale"
		        "and it seems there is no one around.",
		.choices = {
			{ .text = "Search the bed", .go_to = "BED" },
			{ .text = "Examine the door", .go_to = "DOOR" },
			{ .text = "Look at the vent", .go_to = "VENT" },
		},
	};

    story.nodes["CELL"] = {
		.camera = "StartCamera",
		.text = "The cell door is locked, the air is stale"
		        "and it seems there is no one around.",
		.choices = {
			{ .text = "Search the bed", .go_to = "BED" },
			{ .text = "Examine the door", .go_to = "DOOR" },
			{ .text = "Look at the vent", .go_to = "VENT" },
		},
	};

    /******************** BED ****************/

	story.nodes["BED"] = {
		.camera = "BedCamera",
		.text = "The old wooden bed smells of rot. Does not look very comfortable to sleep in. "
                "You see two wooden stands next to the bed, one has a candle and the other has a chest on top.",
		.choices = {
            { .text = "Take a nap", .go_to = "NAP"},
			{ .text = "Examine the candle", .go_to = "CANDLE"},
            { .text = "Examine the chest", .go_to = "CHEST", .need_not = {"chest_open"}},
            { .text = "Examine the chest", .go_to = "CHESTOPEN", .need = {"chest_open"}},
            { .text = "Look under the bed", .go_to = "UNDERBED", .need_not = {"dug"}},
            { .text = "Look under the bed", .go_to = "DUG", .need = {"dug"}},
			{ .text = "Go back", .go_to = "CELL" },
		},
	};

    story.nodes["NAP"] = {
		.camera = "BedCamera",
		.text = "Suprisingly the sleep wasn't bad. You feel rested.",
		.choices = {
			{ .text = "Examine the candle", .go_to = "CANDLE"},
            { .text = "Examine the chest", .go_to = "CHEST", .need_not = {"chest_open"}},
            { .text = "Examine the chest", .go_to = "CHESTOPEN", .need = {"chest_open"}},
            { .text = "Look under the bed", .go_to = "UNDERBED", .need_not = {"dug"}},
            { .text = "Look under the bed", .go_to = "DUG", .need = {"dug"}},
			{ .text = "Go back", .go_to = "CELL" },
		},
	};

    story.nodes["UNDERBED"] = {
		.camera = "UnderBedCamera",
		.text = "You look under the bed. There seems to be some sort of dirt mound. "
                "If only you had something to dig with.",
		.choices = {
			{ .text = "Dig with the spoon", .go_to = "DUG", .need = {"has_spoon"}, .need_not = {"dug"},
                .set_flags = {"dug"}, .hide_meshes = {"FloorDirt"}},
			{ .text = "Go back", .go_to = "BED" },
		},
	};

    story.nodes["DUG"] = {
		.camera = "UnderBedCamera",
		.text = "You dig with the sturdy spoon. There seems to have been something underneath the dirt.",
		.choices = {
			{ .text = "Take the item", .go_to = "DRILL", .need = {"dug"}, .need_not = {"has_drill"}, .set_flags = {"has_drill"}, .hide_meshes = {"Drill"}},
			{ .text = "Go back", .go_to = "BED" },
		},
	};

    story.nodes["DRILL"] = {
		.camera = "UnderBedCamera",
		.text = "It seems to be half of a screwdriver. Sturdy and well made. If only you could find a handle",
		.choices = {
			{ .text = "Combine with handle", .go_to = "SCREWDRIVER", .need = {"has_drill", "has_handle"},
                .set_flags = {"has_screwdriver"}},
			{ .text = "Go back", .go_to = "BED", .need_not = {"has_handle"}},
		},
	};

    story.nodes["CANDLE"] = {
		.camera = "CandleCamera",
		.text = "It's a normal candle. Wax is melting off the sides.",
		.choices = {
			{ .text = "Fill the key mold with wax", .go_to = "KEY", .need = {"has_mold"}, .need_not = {"has_key"}, .set_flags = {"has_key"}},
			{ .text = "Go back", .go_to = "BED" },
		},
	};

    story.nodes["CHEST"] = {
		.camera = "ChestCamera",
		.text = "It's a blue chest with a lock. The lock has a four number code, if only you knew what it was.",
		.choices = {
			{ .text = "Input the code you found on the ball", .go_to = "CHESTOPEN", .need = {"has_code"}, .need_not = {"chest_open"},
                .set_flags = {"chest_open"}, .hide_meshes = {"Lock", "TopChest"}},
			{ .text = "Go back", .go_to = "BED" },
		},
	};

    story.nodes["CHESTOPEN"] = {
		.camera = "ChestCamera",
		.text = "The code worked! The chest is now unlocked, there seems to have been some item inside.",
		.choices = {
			{ .text = "Grab the item", .go_to = "HANDLE", .need = {"chest_open"}, .need_not = {"has_handle"},
                .set_flags = {"has_handle"}, .hide_meshes = {"Handle"}},
			{ .text = "Go back", .go_to = "BED" },
		},
	};

    story.nodes["HANDLE"] = {
		.camera = "ChestCamera",
		.text = "It seems to be a handle of screwdriver. If only you could find the other half.",
		.choices = {
			{ .text = "Combine with drill", .go_to = "SCREWDRIVER", .need = {"has_drill", "has_handle"},
                .set_flags = {"has_screwdriver"}},
			{ .text = "Go back", .go_to = "BED", .need_not = {"has_drill"}},
		},
	};

    story.nodes["SCREWDRIVER"] = {
		.camera = "StartCamera",
		.text = "You combine the drill and the handle. You have managed to create a flimsy screwdriver!",
		.choices = {
			{ .text = "Search the bed", .go_to = "BED" },
			{ .text = "Examine the door", .go_to = "DOOR" },
			{ .text = "Look at the vent", .go_to = "VENT" },
		},
	};

    //*********** DOOR **********/

	story.nodes["DOOR"] = {
		.camera = "DoorCamera",
		.text = "The iron door will not budge. You see a hole for a key, but where would you get a key? "
                "On the floor next to the door you see your meal for the day.",
		.choices = {
			{ .text = "Push against the door again", .go_to = "DOORTRYOPEN"},
			{ .text = "Investigate what you have for dinner", .go_to = "FOOD"},
            { .text = "Use the wax key to open the door", .go_to = "ESCAPE", .need = {"has_key"}, .hide_meshes = {"Door"}},
			{ .text = "Go back", .go_to = "CELL" },
		},
	};

    story.nodes["DOORTRYOPEN"] = {
		.camera = "DoorCamera",
		.text = "The iron door still doesn't budge. It seems you are trapped in here.",
		.choices = {
			{ .text = "Push against the door again", .go_to = "DOORTRYOPEN"},
			{ .text = "Investigate what you have for dinner", .go_to = "FOOD"},
            { .text = "Use the wax key to open the door", .go_to = "ESCAPE", .need = {"has_key"}},
			{ .text = "Go back", .go_to = "CELL" },
		},
	};

    story.nodes["FOOD"] = {
		.camera = "FoodCamera",
		.text = "Some sort of red sludge seems to be dinner. Gross. "
                "They at least gave you a spoon to eat with.",
		.choices = {
			{ .text = "Take a bit of the food", .go_to = "FOODEAT"},
			{ .text = "Take the spoon", .go_to = "TOOK_SPOON", .need_not = {"has_spoon"}, .set_flags = {"has_spoon"}, .hide_meshes = {"Spoon"}},
			{ .text = "Go back", .go_to = "DOOR" },
		},
	};

    story.nodes["FOODEAT"] = {
		.camera = "FoodCamera",
		.text = "Ugh, the food is disgusting as always. I don't know why you tried it",
		.choices = {
            { .text = "Take the spoon", .go_to = "TOOK_SPOON", .need_not = {"has_spoon"}, .set_flags = {"has_spoon"}, .hide_meshes = {"Spoon"}},
			{ .text = "Go back", .go_to = "DOOR" },
		},
	};

    story.nodes["TOOK_SPOON"] = {
		.camera = "FoodCamera",
		.text = "A bent iron spoon. Not much of a weapon, but the handle is thin and strong.",
		.choices = {
            { .text = "Take a bit of the food", .go_to = "FOODEAT"},
			{ .text = "Go back", .go_to = "DOOR" },
		},
	};

    /****************** VENT ******************/

	story.nodes["VENT"] = {
		.camera = "BallCamera",
		.text = "There is a vent and a stack of balls.",
		.choices = {
            { .text = "Take a ball", .go_to = "VENT", .need_not = {"ball1"}, .set_flags = {"ball1"}, .hide_meshes = {"Ball1"}},
            { .text = "Take a ball", .go_to = "VENT", .need = {"ball1"}, .need_not = {"ball2"}, .set_flags = {"ball2"}, .hide_meshes = {"Ball2"}},
            { .text = "Take a ball", .go_to = "VENT", .need = {"ball2"}, .need_not = {"ball3"}, .set_flags = {"ball3"}, .hide_meshes = {"Ball3"}},
            { .text = "Take a ball", .go_to = "CODE", .need = {"ball3"}, .need_not = {"ball4"}, .set_flags = {"ball4", "has_code"}, .hide_meshes = {"Ball4"}},
            { .text = "Investigate the vent", .go_to = "VENTCLOSE", .need_not = {"vent_open"}},
            { .text = "Investigate the vent", .go_to = "VENTOPEN", .need = {"vent_open"}},
			{ .text = "Go back", .go_to = "CELL" },
		},
	};

    story.nodes["CODE"] = {
		.camera = "BallCamera",
		.text = "Under the last ball you find a four digit code, 1482. Maybe this could be used for something?",
		.choices = {
            { .text = "Investigate the vent", .go_to = "VENTCLOSE" },
			{ .text = "Go back", .go_to = "CELL" },
		},
	};

    story.nodes["VENTCLOSE"] = {
		.camera = "VentCamera",
		.text = "The vent is screwed tight. You can see something behind the vent cover. "
                "If only you could somehow unscrew it.",
		.choices = {
            { .text = "Use the screwdriver to unscrew the vent cover", .go_to = "VENTOPEN", .need = {"has_screwdriver"},
                .need_not = {"vent_open"}, .set_flags = {"vent_open"}, .hide_meshes = {"VentCover"}},
			{ .text = "Go back", .go_to = "VENT" },
		},
	};

    story.nodes["VENTOPEN"] = {
		.camera = "VentCamera",
		.text = "You use the screwdriver to open the vent cover. Now you have access to the item behind.",
		.choices = {
            { .text = "Take the item", .go_to = "MOLD", .need = {"vent_open"},
                .need_not = {"has_mold"}, .set_flags = {"has_mold"}, .hide_meshes = {"KeyMold"}},
			{ .text = "Go back", .go_to = "VENT" },
		},
	};

    story.nodes["MOLD"] = {
		.camera = "VentCamera",
		.text = "You pick up the item. It seems to be a mold for some sort of key. Maybe the key to the door? "
                "What would you use to make the key however?",
		.choices = {
			{ .text = "Go back", .go_to = "VENT" },
		},
	};


    story.nodes["KEY"] = {
		.camera = "CandleCamera",
		.text = "You pour the melted wax into the mold and wait. After a while you have made a key of wax!",
		.choices = {
			{ .text = "Go back", .go_to = "BED" },
		},
	};


	//----- endings (no choices) -----

	story.nodes["ESCAPE"] = {
		.camera = "DoorCamera",
		.text = "The door opens. You walk through and into the dark corridor beyond. "
		        "You are free.",
		.choices = {},
	};

	return story;
}