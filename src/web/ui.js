/*
 * GrWebSDR: a web SDR receiver
 *
 * Copyright (C) 2017 Ondřej Lysoněk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING).  If not,
 * see <http://www.gnu.org/licenses/>.
 */

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
	return hw_freq - converter_offset;
}

function get_receiver_freq() {
	return get_center_freq() + freq_offset;
}

function update_center_freq() {
	var freq = get_center_freq();
	var elem = document.getElementById('lbl_center_freq');
	var formatted = format_freq(freq, true);
	elem.innerHTML = 'Center frequency: ' + formatted;

	formatted = format_freq(freq, false);
	elem = document.getElementById('center_freq');
	elem.value = formatted;
}

function update_receiver_freq() {
	var freq = get_receiver_freq();
	var elem = document.getElementById('lbl_receiver_freq');
	var formatted = format_freq(freq, true);
	elem.innerHTML = 'Receiver frequency: ' + formatted;

	elem = document.getElementById('txt_receiver_freq');
	elem.value = format_freq(freq, false);
}

function update_converter_offset(offset) {
	var formatted = format_freq(offset, true);
	var lbl = document.getElementById('converter_offset');
	lbl.innerHTML = 'Up/down converter offset: ' + formatted;
	converter_offset = offset;
	update_center_freq();
	update_receiver_freq();
}

function update_source_description(description) {
	var elem = document.getElementById('source_description');
	if (description == '')
		elem.innerHTML = '';
	else
		elem.innerHTML = 'Description: ' + description;
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
	if (source.hasOwnProperty('description')) {
		update_source_description(source.description);
	}
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
	value = value.toString();
	if (value.length > 6) {
		value = value.slice(0, -6) + '&nbsp' + value.slice(-6, value.length);
		value = value.slice(0, -3) + '&nbsp' + value.slice(-3, value.length);
	} else if (value.lenth > 3) {
		value = value.slice(0, -3) + '&nbsp' + value.slice(-3, value.length);
	}

	elem.innerHTML = 'Sample rate: ' + value + ' S/s';
}

function update_privileged(msg) {
	if (msg.hasOwnProperty('privileged')) {
		var modal = document.getElementById('login_modal');
		var btn_show_login = document.getElementById('btn_show_login');
		var btn_logout = document.getElementById('btn_logout');
		if (privileged && !msg.privileged) {
			btn_logout.style.display = 'none';
			btn_show_login.style.display = 'inline';
			privileged = false;
			hide_privileged_ui();
		} else if (!privileged && msg.privileged) {
			modal.style.display = 'none';
			btn_show_login.style.display = 'none';
			btn_logout.style.display = 'inline';
			privileged = true;
			show_privileged_ui();
		} else if (!privileged && !msg.privileged) {
			alert('Login failed.');
		}
	}
}

function send_hw_freq() {
	var freq = document.getElementById('center_freq').value;
	var tmp = parse_freq(freq);
	if (!tmp.ok) {
		alert('Bad format');
		return;
	}
	freq = tmp.value;
	freq += converter_offset;
	freq_change_requested = true;
	ws.send('{"hw_freq":' + freq + '}');
}

function show_privileged_ui() {
	var elem = document.getElementById('div_privileged');
	elem.style.display = 'block';

	elem = document.getElementById('center_freq');
}

function hide_privileged_ui() {
	var elem = document.getElementById("div_privileged");
	elem.style.display = 'none';
}

function format_freq(freq, spaced) {
	var tmp = Math.abs(freq);
	var unit = '';
	if (tmp >= 1000000) {
		freq = freq / 1000000.0;
		freq = freq.toFixed(6).toString();
		if (spaced)
			    freq = freq.slice(0, -3) + '&nbsp' + freq.slice(-3, freq.length);
		unit = 'MHz';
	} else if (tmp >= 1000) {
		freq = freq / 1000.0;
		freq = freq.toFixed(3);
		unit = 'kHz';
	} else {
		unit = 'Hz';
	}
	return freq + ' ' + unit;
}

function update_hw_freq(value) {
	hw_freq = value;
	update_center_freq();
	update_receiver_freq();
}

function send_freq_offset(offset) {
	ws.send('{"freq_offset":' + offset + '}');
}

function parse_freq(str) {
	var mult = 1;
	var ret = {};
	var ix;
	ret.ok = false;
	str = str.trim().toLowerCase();
	if (str.endsWith('mhz')) {
		mult = 1000000;
		ix = str.lastIndexOf('mhz');
	} else if (str.endsWith('khz')) {
		mult = 1000;
		ix = str.lastIndexOf('khz');
	} else if (str.endsWith('hz')) {
		ix = str.lastIndexOf('hz');
	} else {
		ix = str.length;
	}
	if (ix == 0)
		return ret;
	str = str.slice(0, ix).trim();
	if (str.search("^-?[0123456789]+(\\.[0123456789]+)?$") < 0)
		return ret;
	ret.value = Math.floor(parseFloat(str) * mult);
	ret.ok = true;
	return ret;
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

function change_receiver_freq_txt() {
	var receiver_freq = document.getElementById("txt_receiver_freq").value;
	var tmp = parse_freq(receiver_freq);
	if (!tmp.ok) {
		alert('Bad format');
		return;
	}
	receiver_freq = tmp.value;
	var offset = receiver_freq - get_center_freq();
	if (!check_freq_offset(offset))
		return;
	update_freq_offset(offset);
	send_freq_offset(offset);
}

function change_freq_offset_txt() {
	var offset = document.getElementById('txt_offset').value;
	var tmp = parse_freq(offset);
	if (!tmp.ok) {
		alert('Bad format');
		return;
	}
	offset = tmp.value;
	if (!check_freq_offset(offset))
		return;
	update_freq_offset(offset);
	send_freq_offset(offset);
}

function add_to_offset(diff) {
	var offset = freq_offset;
	offset += diff;
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
	document.getElementById('txt_offset').value = format_freq(offset, false);
	update_receiver_freq();
}

function init_audio(stream_name) {
	audio = new Audio(stream_name);
	audio.play();
}

function show_login(val) {
	if (val)
		val = 'block';
	else
		val = 'none';
	document.getElementById('login_modal').style.display = val;
}

function logout() {
	ws.send('{"logout":null}');
}

function login() {
	var obj = {};
	obj.login = {};
	obj.login.user = document.getElementById('username').value;
	obj.login.pass = document.getElementById('password').value;
	document.getElementById('password').value = '';
	ws.send(JSON.stringify(obj));
}

function send_demod(val) {
	ws.send('{"demod":"' + val + '"}');
}
