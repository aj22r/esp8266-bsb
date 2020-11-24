//Copyright (C) 2016 <>< Charles Lohr, see LICENSE file for more info.
//
//This particular file may be licensed under the MIT/x11, New BSD or ColorChord Licenses.

var bsbValues = {
	0x15000242: {id: "bsb_outside_temp", type: 't'},
	0x2d05051e: {id: "bsb_room_temp", type: 't'},
	0x2d050593: {id: "bsb_area1_temp", type: 't'},
	0x2e050593: {id: "bsb_area2_temp", type: 't'},
	0x05050d2a: {id: "bsb_generator", type: 'p'},
	0x050512f4: {id: "bsb_elec", type: 'p'},

	0x5905052d: {id: "bsb_pealevool1", type: 't'},
	0x22050518: {id: "bsb_pealevool2", type: 't'},
	0x59050537: {id: "bsb_tagasivool", type: 't'},
	0x3105052f: {id: "bsb_water_temp", type: 't'},
	0x59050767: {id: "bsb_pealevoolu_setpoint", type: 't'},
	0x22050667: {id: "bsb_2_pealevoolu_setpoint", type: 't'},
}

function initExample() {
	var infoItm = `
		<div class=info>
		Toas: <label id=bsb_room_temp>?</label>&deg;<br>
		Ala 1: <label id=bsb_area1_temp>?</label>&deg;<br>
		Ala 2: <label id=bsb_area2_temp>?</label>&deg;<br>
		Soojuspump: <label id=bsb_generator>?</label>%<br>
		Elekt. lisaküte 9kW: <label id=bsb_elec>?</label>%<br>
		<hr>
		Pealevool 1: <label id=bsb_pealevool1>?</label>&deg;<br>
		Pealevool 2: <label id=bsb_pealevool2>?</label>&deg;<br>
		Tagasivool: <label id=bsb_tagasivool>?</label>&deg;<br>
		STV: <label id=bsb_water_temp>?</label>&deg;<br>
		Õues: <label id=bsb_outside_temp>?</label>&deg;<br>
		SP pealevoolu seadepunkt: <label id=bsb_pealevoolu_setpoint>?</label>&deg;<br>
		Ala 2 pealevoolu seadepunkt: <label id=bsb_2_pealevoolu_setpoint>?</label>&deg;<br>
		</div>
		<br>
	`
	$('#MainMenu').before( infoItm );

	var menItm = `
		<tr><td width=1>
		<input type=submit onclick="ShowHideEvent( 'BSB' ); StartBSBTicker();" value="BSB"></td><td>
		<div id=BSB class="collapsible">
		<table width=100% border=1><tr><td>
		<input type=submit onclick="$('#bsb_terminal').val('')" value="Clear">
		Autoscroll: <input type=checkbox id=bsb_autoscroll checked><br>
		<textarea id=bsb_terminal readonly rows=20 cols=80></textarea><br>
		Source: <input type=text id=bsb_src><br>
		Destination: <input type=text id=bsb_dest><br>
		Type: <input type=text id=bsb_type><br>
		Command: <input type=text id=bsb_cmd><br>
		Payload: <input type=text id=bsb_payload><br>
		<input type=submit value=\"Send message\" onclick=\"SendBSBMessage()\">
		</td></tr></table>
		</div>
		</td></tr>
	`;
	$('#MainMenu > tbody:last').after( menItm );
	
	setInterval(UpdateBSBValues, 2000);

	UpdateBSBValues();
}

window.addEventListener("load", initExample, false);

function UpdateBSBValues() {
	for(var cmd in bsbValues) {
		QueueOperation("CR" + cmd.toString(), function(req, msgAsText, msg) {
			var cmd = bsbValues[parseInt(req.request.slice(2))];
			var val = "";
			switch(cmd.type) {
				case 't':
					if(msg.length == 3)
						val = (((msg[1] << 8) | msg[2]) / 64).toFixed(1);
					else
						return;
					break;
				case 'p':
					if(msg.length == 2)
						val = String(msg[1]);
					else
						return;
					break;
			}
			document.getElementById(cmd.id).innerText = val;
		});
	}
}

// https://stackoverflow.com/questions/40031688/javascript-arraybuffer-to-hex/50767210
function buf2hex(buffer) { // buffer is an ArrayBuffer
	return Array.prototype.map.call(new Uint8Array(buffer), x => ('00' + x.toString(16)).slice(-2)).join('');
}

function OnBSBMessage(req, msgAsStr, msg) {
	if(msg.length == 0) return;
	//console.log(msgAsStr, msg);
	$("#bsb_terminal").val($("#bsb_terminal").val() + "\n" + BSBMessageToString(msg));
	// make bsb_terminal auto scroll to bottom
	if(document.getElementById('bsb_autoscroll').checked)
		$('#bsb_terminal').scrollTop($('#bsb_terminal')[0].scrollHeight);

	BSBTicker();
}

function BSBTicker() {
	if(!IsTabOpen('BSB')) {
		return;
	}

	QueueOperation("CB", OnBSBMessage);

	setTimeout(BSBTicker, 200);
}

function StartBSBTicker() {
	BSBTicker();
}

function SendBSBMessage() {
	var st = "CS";
	st += $("#bsb_src").val();
	st += "\t" + $("#bsb_dest").val();
	st += "\t" + $("#bsb_type").val();
	st += "\t" + parseInt($("#bsb_cmd").val(), 16);
	st += "\t" + Math.floor($("#bsb_payload").val().length / 2);
	st += "\t" + $("#bsb_payload").val();
	QueueOperation( st );
}

var tgTypeDict = {
	0x01: "TYPE_QINF",  // request info telegram
	0x02: "TYPE_INF",   // send info telegram
	0x03: "TYPE_SET",   // set parameter
	0x04: "TYPE_ACK",   // acknowledge set parameter
	0x05: "TYPE_NACK",  // do not acknowledge set parameter
	0x06: "TYPE_QUR",   // query parameter
	0x07: "TYPE_ANS",   // answer query
	0x08: "TYPE_ERR",   // error
	0x0F: "TYPE_QRV",   // query  reset value
	0x10: "TYPE_ARV",   // answer reset value
	0x12: "TYPE_IQ1",   // internal query type 1 (still undecoded)
	0x13: "TYPE_IA1",   // internal answer type 1 (still undecoded)
	0x14: "TYPE_IQ2",   // internal query type 2 (still undecoded)
	0x15: "TYPE_IA2"    // internal answer type 2 (still undecoded)
}

var tgAddrDict = {
	0x00: "ADDR_HEIZ",
	0x03: "ADDR_EM1",
	0x04: "ADDR_EM2",
	0x06: "ADDR_RGT1",
	0x07: "ADDR_RGT2",
	0x08: "ADDR_CNTR",
	0x0A: "ADDR_DISP",
	0x0B: "ADDR_SRVC",
	0x31: "ADDR_OZW",
	0x32: "ADDR_FE",
	0x36: "ADDR_RC",
	0x42: "ADDR_LAN",
	0x7F: "ADDR_ALL"
}

var bruteforceValues = [];
var bruteforceAddress = 0x05050100;
var bruteforceRunning = false;

function BSBMessageToString(msg) {
	var src = msg[1] & 0x7F;
	var dst = msg[2];
	var msgLen = msg[3];
	var msgType = msg[4];

	var cmd = 0;
	if(msgType == 3 || msgType == 6) { // QUERY and SET: byte 5 and 6 are in reversed order
		cmd = (msg[6]<<24) | (msg[5]<<16) | (msg[7]<<8) | msg[8];
	} else {
		cmd = (msg[5]<<24) | (msg[6]<<16) | (msg[7]<<8) | msg[8];
	}

	var dataLen = 0;
	if(msgType < 0x12) {
		dataLen = msgLen - 11; // get packet length, then subtract
	} else {
		dataLen = msgLen - 7;  // for yet unknow telegram types 0x12 to 0x15
	}
	var payload = msg.slice(9, 9 + dataLen)

	if(bruteforceRunning && cmd == bruteforceAddress && msgType != 6) {
		bruteforceValues[bruteforceAddress] = payload;
		bruteforceAddress++;
		//Bruteforce();
	}
	
	var str = "";
	str += String(tgAddrDict[src]) + " (" + String(src) + ") -> " + String(tgAddrDict[dst]) + " (" + String(dst) + ")";
	str += " | ";
	str += "Type: " + String(tgTypeDict[msgType]) + " (" + String(msgType) + "), cmd: " + cmd.toString(16);
	str += ", payload: " + buf2hex(payload);
	if(payload.length == 3) {
		var value = (payload[1] << 8) | payload[2];
		str += " (" + String(value) + "/" + String(value / 64) + ")";
	}

	return str;
}

function Bruteforce() {
	bruteforceRunning = true;
	$("#bsb_src").val("66");
	$("#bsb_dest").val("0");
	$("#bsb_type").val("6");
	$("#bsb_cmd").val(bruteforceAddress.toString(16));
	$("#bsb_payload").val("");
	SendBSBMessage();

	setTimeout(Bruteforce, 500);
}