var hw_freq = null;
var privileged = false;
var freq_change_requested = false;
var sample_rate = null;
var stream_name = null;
var audio = null;
var converter_offset = 0;
var freq_offset = 0;

var ws_url;
if (window.location.protocol == 'https:')
	ws_url = 'wss://' + window.location.host;
else
	ws_url = 'ws://' + window.location.host;
var ws = null;
setup_websocket();

function setup_websocket() {
	ws = new WebSocket(ws_url, 'websocket');
	ws.onerror = function (event) {
		alert('WebSocket error');
	};
	ws.onmessage = function(event) {
		var msg = JSON.parse(event.data);
		if (msg.hasOwnProperty('stream_name')) {
			stream_name = msg.stream_name;
			update_freq_offset(0);
		}
		if (msg.hasOwnProperty('sources')) {
			update_sources(msg.sources);
		}
		if (msg.hasOwnProperty('supported_demods')) {
			update_demods(msg.supported_demods);
		}
		if (msg.hasOwnProperty('current_source')) {
			if (audio == null)
				init_audio(stream_name);
			source_changed(msg.current_source);
		}
		if (msg.hasOwnProperty('hw_freq')) {
			var value = msg.hw_freq;
			var changed = hw_freq != value;
			update_hw_freq(value);
			if (!freq_change_requested && changed) {
				alert('HW frequency changed.');
				freq_change_requested = false;
			}
		}
		if (msg.hasOwnProperty('demod')) {
			update_demod_name(msg.demod);
		}
		if (msg.hasOwnProperty('freq_offset')) {
			update_freq_offset(msg.freq_offset);
		}
		update_privileged(msg);
		update_num_clients(msg);
	};
}

function update_num_clients(msg) {
	if (msg.hasOwnProperty('num_clients')) {
		var elem = document.getElementById('num_clients');
		elem.innerHTML = 'Number of clients: ' + msg.num_clients;
	}
}

function update_sources(sources) {
	var sel = document.getElementById('select_source');
	sel.innerHTML = '';
	if (sources.length == 0) {
		alert('No sources available');
		return;
	}
	for (i = 0; i < sources.length; ++i) {
		var opt = document.createElement('option');
		opt.value = i;
		opt.innerHTML = sources[i];
		sel.appendChild(opt);
	}
	update_source(0);
	send_source(0);
}

function update_demods(demods) {
	var sel = document.getElementById('select_demod');
	sel.innerHTML = '';
	if (demods.length == 0) {
		alert('No supported demodulations.');
		return;
	}
	for (i = 0; i < demods.length; ++i) {
		var opt = document.createElement('option');
		opt.value = demods[i];
		opt.innerHTML = demods[i];
		sel.appendChild(opt);
	}
	update_demod_name(demods[0]);
	send_demod(demods[0]);
}

function update_source(ix) {
	var sel = document.getElementById('select_source');
	sel.value = ix;
}

function update_demod_name(demod) {
	var sel = document.getElementById('select_demod');
	sel.value = demod;
}

function get_center_freq() {
	return hw_freq + converter_offset;
}

function get_receiver_freq() {
	return get_center_freq() + freq_offset;
}

function update_center_freq() {
	var freq = get_center_freq();
	var elem = document.getElementById('lbl_center_freq');
	var formatted = format_freq(freq);
	elem.innerHTML = 'Center frequency: ' + formatted;

	elem = document.getElementById('txt_center_freq');
	if (elem != null)
		elem.value = freq;
}

function update_receiver_freq() {
	var freq = get_receiver_freq();
	var elem = document.getElementById('lbl_receiver_freq');
	var formatted = format_freq(freq);
	elem.innerHTML = 'Receiver frequency: ' + formatted;

	elem = document.getElementById('txt_receiver_freq');
	elem.value = freq;
}

function update_converter_offset(offset) {
	var formatted = format_freq(offset);
	var lbl = document.getElementById('converter_offset');
	lbl.innerHTML = 'Up/down converter offset: ' + formatted;
	converter_offset = offset;
	update_center_freq();
	update_receiver_freq();
}

function source_changed(source) {
	if (!source.hasOwnProperty('source_ix')
			|| !source.hasOwnProperty('sample_rate')
			|| !source.hasOwnProperty('hw_freq')
			|| !source.hasOwnProperty('converter_offset')) {
		alert('Error: incomplete source info sent.');
		return;
	}
	update_converter_offset(source.converter_offset);
	update_source(source.source_ix);
	update_hw_freq(source.hw_freq);
	update_sample_rate(source.sample_rate);
}

function send_source(ix) {
	ws.send('{"source": ' + ix + '}');
}

function update_sample_rate(value) {
	var elem = document.getElementById('freq_offset');
	sample_rate = value;
	elem.min = -sample_rate/2;
	elem.max = sample_rate/2;

	elem = document.getElementById('sample_rate');
	elem.innerHTML = 'Source sample rate: ' + value;
}

function update_privileged(msg) {
	if (msg.hasOwnProperty('privileged')) {
		var btn = document.getElementById('btn_login');
		var user = document.getElementById('username');
		var pass = document.getElementById('password');
		if (privileged && !msg.privileged) {
			privileged = false;
			btn.value = 'Login';
			user.style.display = 'inline';
			pass.style.display = 'inline';
			removePrivilegedUI();
		} else if (!privileged && msg.privileged) {
			privileged = true;
			btn.value = 'Logout';
			user.style.display = 'none';
			user.value = '';
			pass.style.display = 'none';
			addPrivilegedUI();
		} else if (!privileged && !msg.privileged) {
			alert('Login failed.');
		}
	}
}

function addPrivilegedUI() {
	var form = document.createElement("form");
	form.id = "form_center_freq";
	form.action = "#";
	form.onsubmit = function() {
		var freq = parseInt(document.getElementById('center_freq').value);
		freq -= converter_offset;
		freq_change_requested = true;
		ws.send('{"hw_freq":' + freq + '}');
	};

	var e = document.createElement("input");
	e.type = "text";
	e.id = "center_freq";
	e.value = get_center_freq();
	form.appendChild(e);

	e = document.createElement("input");
	e.type = "submit";
	e.value = "Change center frequency";
	form.appendChild(e);

	document.body.appendChild(form);
}

function removePrivilegedUI() {
	var e = document.getElementById("form_center_freq");
	e.outerHTML = "";
	delete e;
}

function format_freq(freq) {
	tmp = Math.abs(freq);
	if (tmp >= 1000000)
		return (freq / 1000000.0) + ' MHz';
	else if (tmp >= 1000)
		return (freq / 1000.0) + ' kHz';
	else
		return freq + ' Hz';
}

function update_hw_freq(value) {
	hw_freq = value;
	update_center_freq();
	update_receiver_freq();
}

function send_freq_offset(offset) {
	ws.send('{"freq_offset":' + offset + '}');
}

function change_freq_offset_range() {
	var offset = parseInt(document.getElementById("freq_offset").value);
	if (!check_freq_offset(offset))
		return;
	update_freq_offset(offset);
	// No send here. For performance reasons, the offset gets sent
	// only after the mouse button is released (onchange event
	// of the range element)
}

function change_freq_offset_txt() {
	var receiver_freq = document.getElementById("txt_receiver_freq").value;
	var offset = receiver_freq - get_center_freq();
	if (!check_freq_offset(offset))
		return;
	update_freq_offset(offset);
	send_freq_offset(offset);
}

function check_freq_offset(offset) {
	if (sample_rate == null)
		return false;
	if (offset < -sample_rate / 2 || offset > sample_rate / 2) {
		alert("Frequency out of range");
		return false;
	}
	return true;
}

function update_freq_offset(offset) {
	freq_offset = offset;
	document.getElementById("freq_offset").value = offset;
	update_receiver_freq();
}

function init_audio(stream_name) {
	audio = new Audio(stream_name);
	audio.play();
}

function toggle_login() {
	if (privileged) {
		ws.send('{"logout":null}');
	} else {
		var obj = {};
		obj.login = {};
		obj.login.user = document.getElementById('username').value;
		obj.login.pass = document.getElementById('password').value;
		document.getElementById('password').value = '';
		ws.send(JSON.stringify(obj));
	}
}

function send_demod(val) {
	ws.send('{"demod":"' + val + '"}');
}
