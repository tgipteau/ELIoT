const room_selection_div = document.querySelector("#room_selection");
const combos_div = document.querySelector("#combos_div");
const card = document.getElementById("card");
const mkcmb_div = document.querySelector("#mkcmb_div");
const room_divs = [];
const room_selectors = []; // même indice que room_divs

async function sendCmd(url) {
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
	const response = await fetch("/get_json_dat", {method:"GET"});
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
	mkcmb_div.replaceChildren();
	room_divs.length = 0;                   // vide les tableaux en conservant la référence
	room_selectors.length = 0;
	combos_div.length = 0;
}



// MAIN
// Populate page based on json data sent by the server
function run(dat) {

	clear_ui();
	const rooms = dat.rooms;
	const combos = dat.combos;





	// CREATE ROOM DIVS
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


		// CREATE DEVICES BUTTONS
		for (const appareil of room.appareils) {
			const url = `/cmd/${appareil.url}/${appareil.actions[0]}`;
			const button = document.createElement("button");
			button.textContent = appareil.nom;
			button.classList.add("device_cmd_btn")
			button.addEventListener("click", ()=>{
				sendCmd(url);
			});
			room_div.append(button);
		}
		card.append(room_div);
	});




	// CREATE COMBOS DIV
	for (const combo of combos) {
		const button = document.createElement("button");
		button.textContent = combo.nom;
		button.classList.add("combo_btn");
		button.addEventListener("click", () => {
			for (const cmd of combo.urls) {
				sendCmd(`/cmd/${cmd}`);
			}
		});
		combos_div.append(button);
	}


	// MAKE COMBO DIV

	mkcmb_selectors = [];
	selected_actions_div = document.createElement("div");
	selected_actions_title = document.createElement("p");
	selected_actions_title.textContent = "Selectionné";
	selected_actions_div.append(selected_actions_title);
	mkcmb_div.append(selected_actions_div);

	function show_selected_actions() {
		selected_actions_div.replaceChildren;
		for (const selector in mkcmb_selectors) {
			const selected_p = document.createElement("p");
			selected_p.textContent = selector.value;
			selected_actions_div.append(selected_p);
		}
	}

	for (const room of rooms) {
		mkcmb_room_div = document.createElement("div");
		mkcmb_room_div.classList.add("mkcmb_room_div");
		mkcmb_div.append(mkcmb_room_div);

		for (const device of room.appareils) {
			mkcmb_device_div = document.createElement("div");
			mkcmb_device_div.classList.add("mkcmb_device_div")
			mkcmb_room_div.append(mkcmb_device_div);

			mkcmb_device_p = document.createElement("p");
			mkcmb_device_p.textContent = device.nom;
			mkcmb_sel_action = document.createElement("select");
			const no_opt = document.createElement("option");
			no_opt.value = "";
			no_opt.textContent = "";
			mkcmb_selectors.push(mkcmb_sel_action);
			mkcmb_sel_action.append(no_opt);
			mkcmb_device_div.append(mkcmb_device_p);
			mkcmb_device_div.append(mkcmb_sel_action);

			mkcmb_sel_action.addEventListener("change", show_selected_actions);


			for (const action of device.actions) {
				const mkcmb_action = document.createElement("option");
				mkcmb_action.value = device.url + "/" + action;
				mkcmb_action.textContent = action;
				mkcmb_sel_action.append(mkcmb_action);

			}


		}

	}



	// show first room only on startup
	show_room(0);

}

// main call to script
get_dat_and_run()
