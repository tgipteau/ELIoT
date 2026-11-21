const room_selection_div = document.querySelector("#room_selection");
const combos_div = document.querySelector("#combos_div");
const card = document.getElementById("card");
const room_divs = [];
const room_selectors = []; // même indice que room_divs

async function writeButton(url) {
	try {
		const req = fetch(url, {
			method: "POST",
			body: "Corps",
		});
		const response = await req;
		const texte = await response.text();
		console.log(texte);
	} catch (error) {
		console.log(error);
	}
}


// Reload data on reopening
document.addEventListener("visibilitychange", () => {
  if (!document.hidden) get_dat_and_run();
});

// Get data and run populating script
async function get_dat_and_run() {
	const response = await fetch("/data_json", {method:"GET"});
	const dat = await response.json();
	console.log(dat);
	run(dat)

}

// Show active tab and hide others
function show_room(idx){

	// show room div
	room_divs.forEach((room, r_idx) => {
		room.classList.toggle("room_div_active", r_idx == idx);
	})

	// update room sel buttons
	room_selectors.forEach((selector, s_idx) => {
		selector.classList.toggle("room_sel_btn_active", s_idx == idx);
	})

}

function clear_ui() {
	card.replaceChildren();                 // supprime tous les room_div
	room_selection_div.replaceChildren();   // supprime tous les boutons de sélection
	combos_div.replaceChildren();
	room_divs.length = 0;                   // vide les tableaux en conservant la référence
	room_selectors.length = 0;
	combos_div.length = 0;
}

// Populate page based on json data sent by the server
function run(dat) {

	clear_ui();
	const rooms = dat.rooms;
	const combos = dat.combos;


	rooms.forEach((room, r_idx) => {

		// create room div
		const room_div = document.createElement("div");
		room_div.className = "room_div";
		room_div.id = room.nom;
		room_divs.push(room_div);

		//create room selector
		const room_selector = document.createElement("button");
		room_selector.textContent = room.nom;
		room_selector.className = "room_sel_btn";
		room_selection_div.append(room_selector);
		room_selectors.push(room_selector);
		room_selector.addEventListener("click", () => show_room(r_idx));

		// create room buttons
		for (const appareil of room.appareils) {
			const url = `/cmd/${appareil.url}/${appareil.actions[0]}`;
			const button = document.createElement("button");
			button.textContent = appareil.texte;
			button.classList.add("device_cmd_btn")
			button.addEventListener("click", ()=>{
				writeButton(url);
			});
			room_div.append(button);
		}
		card.append(room_div);
	});

	// create combos div
	for (const combo of combos) {
		const button = document.createElement("button");
		button.textContent = combo.texte;
		button.classList.add("combo_btn");
		button.addEventListener("click", () => {
			for (const cmd of combo.urls) {
				url = `/cmd/${cmd}`;
				writeButton(url);
			}
		});
		combos_div.append(button);
	}

	// show first room only on startup
	show_room(0);

}

// main call to script
get_dat_and_run()
