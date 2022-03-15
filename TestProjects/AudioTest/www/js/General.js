var isUnminified = /param/.test(function (param) { });
var emcEnableDebugger = true;

Array.prototype.select = Array.prototype.map;
Array.prototype.where = Array.prototype.filter;
Array.prototype.toDictionary = Array.prototype.toObject = function (fKey, fVal) {
	let obj = {};
	if(!fVal) fVal = c => c;
    this.forEach(c => {
        obj[fKey(c)] = fVal(c);
    });
    return obj;
};
Array.prototype.groupBy = function (fKey) {
    let obj = {};
    this.forEach(c => {
        let k = fKey(c);
        obj[k] = obj[k] || []
        obj[k].push(c);
    });
    return obj;
}
function toArray(obj) {
    return Object.keys(obj).map(k => ({ key: k, value: obj[k] }));
}
Array.prototype.orderBy = function (fKey) {
    return this.sort((a, b) => fKey(a) > fKey(b) ? 1 : -1);
}

Array.prototype.min = function (fnc, def) {
    fnc = fnc || (c => c);
    return this.reduce(function (acc, b) {
        let tmp = fnc(b);
        return tmp < acc ? tmp : acc;
    }, def || fnc(this[0]));
};
Array.prototype.max = function (fnc, def) {
    fnc = fnc || (c => c);
    return this.reduce(function (acc, b) {
        let tmp = fnc(b);
        return tmp > acc ? tmp : acc;
    }, def || fnc(this[0]));
};
Array.prototype.sum = function (fnc, def) {
    fnc = fnc || (c => c);
    return this.reduce((acc, b) => acc + fnc(b), def || 0);
};
Array.prototype.avg = function (fnc, def) {
    fnc = fnc || (c => c);
    return this.reduce((acc, b) => acc + fnc(b), def || 0)/this.length;
};
Array.prototype.var = function (fnc, def) {
	fnc = fnc || (c => c);
	let a = this.avg(fnc, def);
	return this.map(c => (fnc(c) - a) ** 2).avg();
};
Array.prototype.std = function (fnc, def){
	return Math.sqrt(this.var(fnc, def));
}

String.prototype.contains = String.prototype.includes;
String.prototype.replaceAll = function (search, replacement) {
	var target = this;
	return target.replace(new RegExp(search, 'g'), replacement);
};

String.prototype.myMatch = function (reg) {
	var r2 = new RegExp(reg, 'g');
	var mm = this.match(r2);
	return mm && mm.map(function (c) { return c.match(reg); });
};

Array.prototype.all = function (fnc) {
    return !this.length || !this.find(c => !fnc(c));
};

Array.prototype.any = function (fnc) {
    return this.length && !!this.find(fnc);
};

Array.prototype.count = function (fnc) {
    return this.filter(fnc).length;
};

Array.prototype.Join = function (fnc, sep) {
    return this.map(fnc).join(sep || "");
};

Array.prototype.toCSV = function(){
	let headers = {};
	this.forEach(c => {
		Object.keys(c).forEach(d => { headers[d]=1 });
	});
	headers = Object.keys(headers);
	return headers.join(";") + "\n" +
		this.map(c => headers.map(d => c[d]||"").join(";")).join("\n");
}

Array.prototype.toTable = function(cols){
	cols.forEach((c, i) => {
		if(!c) cols[i] = ()=>"";
		else{
			cols[i] = ((d, f) => { try{ return c(d,f)||"" }catch(e){ return ""; } })
		}
	});
	return this.map(l => "<tr>" + cols.map((d,i) => `<td>${d(l,i)||""}</td>`).join("") + "</tr>").join("\n");
}

String.prototype.replaceIndex = function (idx, newValue) {
    return this.substring(0, idx) + newValue + this.substring(idx + 1);
};

String.prototype.removeAccent = function () {
    let acc = { 'ά': 'α', 'έ': 'ε', 'ί': 'ι', 'ϊ': 'ι', 'ΐ': 'ι', 'ή': 'η', 'ύ': 'υ', 'ϋ': 'υ', 'ΰ': 'υ', 'ό': 'ο', 'ώ': 'ω', 'ς': 'σ' };
    return this.toLowerCase().split("").map(c => ((c in acc) ? acc[c] : c)).join("");
};

String.prototype.editDistance = function (t) {
    let s = this.removeAccent();
    t = t.removeAccent();

    let n = s.length;
    let m = t.length;
    let d = [];
    for (let i = 0; i <= n; i++) { d.push([]); for (let j = 0; j <= m; j++)d[i].push(0); }

    if (n === 0) return m;
    if (m === 0) return n;

    for (let i = 0; i <= n; d[i][0] = i++);
    for (let j = 0; j <= m; d[0][j] = j++);

    for (let i = 1; i <= n; i++) {
        for (let j = 1; j <= m; j++) {
            d[i][j] = Math.min(Math.min(d[i - 1][j] + 1, d[i][j - 1] + 1), d[i - 1][j - 1] + ((t[j - 1] === s[i - 1]) ? 0 : 1));
        }
    }
    return d[n][m];
};

String.prototype.editDistanceMatch = function (list) {
    if (list && list.length) {
        list = list.map(c => ({ str: c, edit: c.editDistance(this), edit2: c.longestCommonSubstring(this) }));
        return list.reduce((a, b) => {
            if (a.edit == b.edit) {
                return a.edit2 > b.edit2 ? a : b;
            } else {
                return a.edit < b.edit ? a : b;
            }
        }, list[0]).str;
    }
}

String.prototype.commonSubsdtringMatch = function (list) {
    if (list && list.length) {
        list = list.map(c => ({ str: c, edit: c.editDistance(this), edit2: c.longestCommonSubstring(this) }));
        return list.reduce((a, b) => {
            if (a.edit2 == b.edit2) {
                return a.edit < b.edit ? a : b;
            } else {
                return a.edit2 > b.edit2 ? a : b;
            }
        }, list[0]).str;
    }
}

String.prototype.longestCommonSubstring = function (b) {
    let a = this.removeAccent();
    b = b.removeAccent();
    let n = a.length, m = b.length;
    if (m == 0 || n == 0) return 0;
    if (m > n) {
        let tmp = a;
        a = b;
        b = tmp;
        m = n;
        n = a.length;
    }

    let d = [];
    for (let i = 0; i < 2; i++) { d.push([]); for (let j = 0; j <= m; j++) d[i].push(0); }

    let cur = 0;

    let mx = 0;
    for (let i = 0; i < n; i++) {
        let upLine = d[cur];
        let Line = d[cur ^ 1];
        Line[0] = 0;
        for (let j = 0; j < m; j++) {
            if (a[i] == b[j]) {
                mx = Math.max(mx, Line[j + 1] = upLine[j] + 1);
            } else {
                Line[j + 1] = 0;
            }
        }
        cur ^= 1;
    }
    return mx;
};

Array.prototype.walkTree = function (selectChildren, clb) {
    this.forEach(c => {
        clb(c);
        let child = selectChildren(c);
        if (child && child.length) child.walkTree(selectChildren, clb);
    });
}

Array.prototype.mapTree = function (selectChildren, clb) {
    res = [];
    this.forEach(c => {
        res.push(clb?clb(c):c);
        let child = selectChildren(c);
        if (child && child.length)
            res = res.concat(child.mapTree(selectChildren, clb));
    });
    return res;
}

forEachObject = function (obj, fnc) {
    Object.keys(obj).forEach(function (k) {
        fnc(k, obj[k]);
    });
};

function Safe(obj, args) {
	for (let i = 0; i < args.length; i++) {
		if (obj[args[i]])
			obj = obj[args[i]];
		else
			return null;
	}
	return obj;
}

function RemoveStrings(cd, ss) {
	var tmp = [];
	var idx = 0;
	var next = -1;
	while (1) {
		var q = '"';
		idx = cd.indexOf('"', next + 1), idx2 = cd.indexOf("'", next + 1);
		if (idx == -1 || (idx2 != -1 && idx2 < idx)) { idx = idx2; q = "'"; }
		if (idx == -1) break;

		next = cd.indexOf(q, idx + 1);
		while (cd[next - 1] == '\\') next = cd.indexOf(q, next + 1);
		if (next == -1) throw "Mismatched Quotes";
		ss.push(cd.substring(idx, next + 1));
		tmp.push({ from: idx, len: next - idx + 1 });
	}

	for (var i = tmp.length - 1; i >= 0; i--) {
		var t = tmp[i];
		cd = cd.split("");
		cd.splice(t.from, t.len, '""');
		cd = cd.join("");
	}
	return cd;
}

function AddString(cd, ss) {
	var sp = cd.split('""');
	cd = sp[0];
	for (var i = 1; i < sp.length; i++)
		cd += ss[i - 1] + sp[i];
	return cd;
}


function MinifyJavascript(cd) {
	var ss = [];
	cd = RemoveStrings(cd, ss);

	cd = cd.replace(/\s+/g, " ");
	cd = cd.replace(/([a-zA-Z0-9_$]) (?![a-zA-Z0-9_$])/g, "$1");
	cd = cd.replace(/([^a-zA-Z0-9_$]) (?=[a-zA-Z0-9_$])/g, "$1");
	cd = cd.replace(/([^a-zA-Z0-9_$]) (?![a-zA-Z0-9_$])/g, "$1");
	return AddString(cd, ss);
}

function MatchAll(regex, Text) {
	var ret = [];
	var m;
	while (m = regex.exec(Text))
		ret.push(m);
	return ret;
}

function GatherFunctions(Text) {
	return MatchAll(/function\s+([^\(\s]+)\(([^\)]*)\)/g, Text).map(function (c) {
		return {
			Name: c[1],
			Args: c[2] && c[2].split(",").map(function (c) { return c.trim(); })
		}
	});
}

function FormatJavascript(cd) {
	function indent(t) {
		var ret = "";
		for (var j = 0; j < t; j++) ret += "\t";
		return ret;
	}
	var ss = [];
	cd = RemoveStrings(cd, ss);
	cd = cd.replace(/\s*(\.|\(|\[)\s*/g, "$1");
	cd = cd.replace(/\s*(\+\+|--|===|!==|<==|>==|\|\||&&|==|>=|<=|=>|!=|[+-]|\*|\/|\\|%|\^|&|<|>|=)\s*/g, " $1 ");
	cd = cd.replace(/\s+(\+\+|--)/g, "$1");
	cd = cd.replace(/\s*(,|;|:|\?)\s*/g, "$1 ");
	cd = cd.replace(/\s*({)\s*/g, " $1");
	cd = cd.replace(/\s+/g, " ");

	var ind = 0;

	for (var i = 0; i < cd.length; i++) {
		var tmp = "";
		switch (cd[i]) {
			case "{": tmp = "{\n" + indent(++ind); break;
			case ";": tmp = ";\n" + indent(ind); break;
			case "}": tmp = "\n" + indent(--ind) + "}\n" + indent(ind); break;
		}
		if (tmp.length) {
			cd = cd.replaceIndex(i, tmp);
			i += tmp.length - 1;
		}
	}
	cd = cd.replace(/for\s*\(\s*([^;]*)\s*;\s*([^;]*);\s*([^\)\(]*)\)/g, "for($1; $2; $3)");

	cd = cd.replace(/\t /g, "\t");
	//	cd = cd.replace(/\s*(,|;)/g, "$1 ");
	cd = cd.replace(/\n\s*\n/g, "\n");
	cd = cd.replace(/\{\s*\}/g, "{}");
	cd = cd.replace(/\s*(;|\)|\])/g, "$1");
	cd = cd.replace(/\s+=\s+/g, " = ");
	cd = cd.replace(/\s*(,|;|:|\?)/g, "$1");

	cd = AddString(cd.trim(), ss);
	try {
		eval("(" + cd + ")");
	} catch (e) {
		throw e;
	}

	return cd;
}

function MyCtrl(t, s, n) {
	s.currentController = t;
	s.ctrlName = [n];
	if (s.$parent && s.$parent.ctrlName) {
		var p = s.$parent && s.$parent.ctrlName;
		for (var i = 0; i < p.length; i++)
			s.ctrlName.push(p[i]);
	}
}

function GetQueryParams() {
	let ret = {}
	document.location.search.slice(1).split("&").forEach(c => {
		let val = c.split("=").map(d => d.trim());
		ret[val[0]] = val[1];
	});
	return ret;
}

function GetHashParams() {
    if (!document.location.hash) return {};
    let ret = {}
    decodeURIComponent(document.location.hash).slice(1).split("&").forEach(c => {
        let val = c.split("=").map(d => d.trim());
        ret[val[0]] = val[1];
    });
    return ret;
}

function SetHashParam(name, value) {
    let p = GetHashParams();
    p[name] = value;
    window.location.hash = Object.keys(p).filter(c => p[c]!==null && p[c]!==undefined).map(c => c + "=" + p[c]).join("&");
}

function SetHashParams(params) {
    let p = {
        ...GetHashParams(),
        ...params
    }
    window.location.hash = Object.keys(p).filter(c => p[c] !== null && p[c] !== undefined).map(c => c + "=" + p[c]).join("&");
}

function Find(list, field, value) {
	return list && list.find(function (c) { return c[field] == value; });
};

function indexOf(list, field, value) {
	return list && list.findIndex(function (c) { return c[field] == value; }, list);
};

function makePager(itemCount, currentPage, indexCount, pageSize) {
    currentPage = currentPage || 0;
	let pageCount = Math.floor((itemCount + pageSize - 1) / pageSize);
	if (currentPage >= pageCount)
		currentPage = pageCount - 1;
	let start = currentPage - Math.floor((indexCount - 1) / 2);
	let end = start + indexCount - 1;
	if (start < 0) {
		start = 0;
		end = indexCount - 1;
	}
	if (end >= pageCount) {
		end = pageCount - 1;
		start = end - indexCount + 1;
		if (start < 0) start = 0;
	}

	let ret = [];
	for (let i = start; i <= end; i++)
		ret.push(i);
	return {
		Pages: ret,
		Current: currentPage,
        Count: itemCount,
        LastIndex: pageCount - 1
	};
}

function splitPages_PageSize(list, count) {
	var ret = [];
	var cur = 0;
	while (cur + count < list.length) {
		ret.push(list.slice(cur, cur + count));
		cur += count;
	}
	if (cur < list.length)
		ret.push(list.slice(cur, list.length));
	return ret;
}

function splitPages_PageCount(list, count) {
	count = Math.ceil(list.length / count);
	var ret = [];
	var cur = 0;
	while (cur + count < list.length) {
		ret.push(list.slice(cur, cur + count));
		cur += count;
	}
	if (cur < list.length)
		ret.push(list.slice(cur, list.length));
	return ret;
}

//var Base64 = { _keyStr: "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=", encode: function (e) { var t = ""; var n, r, i, s, o, u, a; var f = 0; e = Base64._utf8_encode(e); while (f < e.length) { n = e.charCodeAt(f++); r = e.charCodeAt(f++); i = e.charCodeAt(f++); s = n >> 2; o = (n & 3) << 4 | r >> 4; u = (r & 15) << 2 | i >> 6; a = i & 63; if (isNaN(r)) { u = a = 64 } else if (isNaN(i)) { a = 64 } t = t + this._keyStr.charAt(s) + this._keyStr.charAt(o) + this._keyStr.charAt(u) + this._keyStr.charAt(a) } return t }, decode: function (e) { var t = ""; var n, r, i; var s, o, u, a; var f = 0; e = e.replace(/[^A-Za-z0-9+/=]/g, ""); while (f < e.length) { s = this._keyStr.indexOf(e.charAt(f++)); o = this._keyStr.indexOf(e.charAt(f++)); u = this._keyStr.indexOf(e.charAt(f++)); a = this._keyStr.indexOf(e.charAt(f++)); n = s << 2 | o >> 4; r = (o & 15) << 4 | u >> 2; i = (u & 3) << 6 | a; t = t + String.fromCharCode(n); if (u != 64) { t = t + String.fromCharCode(r) } if (a != 64) { t = t + String.fromCharCode(i) } } t = Base64._utf8_decode(t); return t }, _utf8_encode: function (e) { e = e.replace(/rn/g, "n"); var t = ""; for (var n = 0; n < e.length; n++) { var r = e.charCodeAt(n); if (r < 128) { t += String.fromCharCode(r) } else if (r > 127 && r < 2048) { t += String.fromCharCode(r >> 6 | 192); t += String.fromCharCode(r & 63 | 128) } else { t += String.fromCharCode(r >> 12 | 224); t += String.fromCharCode(r >> 6 & 63 | 128); t += String.fromCharCode(r & 63 | 128) } } return t }, _utf8_decode: function (e) { var t = ""; var n = 0; var r = c1 = c2 = 0; while (n < e.length) { r = e.charCodeAt(n); if (r < 128) { t += String.fromCharCode(r); n++ } else if (r > 191 && r < 224) { c2 = e.charCodeAt(n + 1); t += String.fromCharCode((r & 31) << 6 | c2 & 63); n += 2 } else { c2 = e.charCodeAt(n + 1); c3 = e.charCodeAt(n + 2); t += String.fromCharCode((r & 15) << 12 | (c2 & 63) << 6 | c3 & 63); n += 3 } } return t } };
//var Base64 = {
//	encode: function (txt) { return window.btoa(txt) },
//	decode: function (b64) { return window.atob(b64) }
//}

function toLocalTime(date) {
	var newDate = new Date(date.getTime() + date.getTimezoneOffset() * 60 * 1000);
	newDate.setHours(date.getHours() - date.getTimezoneOffset() / 60);
	return newDate;
};

function ParseDate(v) {
	if (!v) return null;
	var d = /\/Date\((\d*)\)\//.exec(v);
	return new Date(d ? +d[1] : v);
};

function sysMessage(msg, log, clb) {
	$("#sysMessage").html(msg);
	$("#sysMessageWindowBg").show().delay(2000).fadeOut(500, clb);
	if (log) {
		if ((typeof log === "string") && log.toLowerCase() == "error") {
			console.log("%cError: " + msg, "color:red");
			return;
		}
		console.log(msg);
	}
};
function sysError(r) {
	sysMessage("Error:" + r.data, "error");
};

function CopyObject(from, to) {
	var k = Object.keys(from);
	for (var i = 0; i < k.length; i++) {
		to[k[i]] = from[k[i]];
	}
}

function msToTime(ms) {
	var seconds = parseInt((ms / 1000) % 60),
		minutes = parseInt((ms / (1000 * 60)) % 60),
		hours = parseInt((ms / (1000 * 60 * 60)) % 24);

	minutes = (minutes < 10) ? "0" + minutes : minutes;
	seconds = (seconds < 10) ? "0" + seconds : seconds;

	return (hours + ":" + minutes + ":" + seconds).replace(/^(0{1,2}:?)*/, "");
}

var Base64 = {
	// private property
	_keyStr: "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=",
	// public method for encoding
	encode: function (input) {
		var output = "";
		var chr1, chr2, chr3, enc1, enc2, enc3, enc4;
		var i = 0;
		input = Base64._utf8_encode(input);
		while (i < input.length) {
			chr1 = input.charCodeAt(i++);
			chr2 = input.charCodeAt(i++);
			chr3 = input.charCodeAt(i++);
			enc1 = chr1 >> 2;
			enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
			enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
			enc4 = chr3 & 63;
			if (isNaN(chr2)) {
				enc3 = enc4 = 64;
			} else if (isNaN(chr3)) {
				enc4 = 64;
			}
			output = output +
				this._keyStr.charAt(enc1) + this._keyStr.charAt(enc2) +
				this._keyStr.charAt(enc3) + this._keyStr.charAt(enc4);
		}
		return output;
	},
	// public method for decoding
	decode: function (input) {
		var output = "";
		var chr1, chr2, chr3;
		var enc1, enc2, enc3, enc4;
		var i = 0;
		input = input.replace(/[^A-Za-z0-9\+\/\=]/g, "");
		while (i < input.length) {
			enc1 = this._keyStr.indexOf(input.charAt(i++));
			enc2 = this._keyStr.indexOf(input.charAt(i++));
			enc3 = this._keyStr.indexOf(input.charAt(i++));
			enc4 = this._keyStr.indexOf(input.charAt(i++));
			chr1 = (enc1 << 2) | (enc2 >> 4);
			chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
			chr3 = ((enc3 & 3) << 6) | enc4;
			output = output + String.fromCharCode(chr1);
			if (enc3 != 64) {
				output = output + String.fromCharCode(chr2);
			}
			if (enc4 != 64) {
				output = output + String.fromCharCode(chr3);
			}
		}
		output = Base64._utf8_decode(output);
		return output;
	},
	// private method for UTF-8 encoding
	_utf8_encode: function (string) {
		string = string.replace(/\r\n/g, "\n");
		var utftext = "";
		for (var n = 0; n < string.length; n++) {
			var c = string.charCodeAt(n);
			if (c < 128) {
				utftext += String.fromCharCode(c);
			}
			else if ((c > 127) && (c < 2048)) {
				utftext += String.fromCharCode((c >> 6) | 192);
				utftext += String.fromCharCode((c & 63) | 128);
			}
			else {
				utftext += String.fromCharCode((c >> 12) | 224);
				utftext += String.fromCharCode(((c >> 6) & 63) | 128);
				utftext += String.fromCharCode((c & 63) | 128);
			}
		}
		return utftext;
	},
	// private method for UTF-8 decoding
	_utf8_decode: function (utftext) {
		var string = "";
		var i = 0;
		var c = c1 = c2 = 0;
		while (i < utftext.length) {
			c = utftext.charCodeAt(i);
			if (c < 128) {
				string += String.fromCharCode(c);
				i++;
			}
			else if ((c > 191) && (c < 224)) {
				c2 = utftext.charCodeAt(i + 1);
				string += String.fromCharCode(((c & 31) << 6) | (c2 & 63));
				i += 2;
			}
			else {
				c2 = utftext.charCodeAt(i + 1);
				c3 = utftext.charCodeAt(i + 2);
				string += String.fromCharCode(((c & 15) << 12) | ((c2 & 63) << 6) | (c3 & 63));
				i += 3;
			}
		}
		return string;
	}
}

function Download2(Data, contType, filename) {
	$("<a download='" + filename + "'>").attr("href", "data:" + contType + ";base64," + Base64.encode(Data))[0].click();
}

var cleanUp = function (a) {
	a.textContent = 'Downloaded';
	a.dataset.disabled = true;
	setTimeout(function () {
		window.URL.revokeObjectURL(a.href);
	}, 1500);
};

function Download(Data, contType, filename) {
	var URL = (window.webkitURL || window.URL);
	var bb = new Blob([Data], {
		type: contType
	});
	var $a = $("<a>");
	var a = $a[0];
	a.download = filename;
	a.href = URL.createObjectURL(bb);
	a.dataset.downloadurl = [contType, a.download, a.href].join(':');
	a.onclick = function (e) {
		if ('disabled' in this.dataset) {
			return false;
		}
		cleanUp(this);
	};
	a.click();
}

function DownloadXLS(data, filename){
	let tableToExcel = (function () {
		let template = '<html xmlns:o="urn:schemas-microsoft-com:office:office" xmlns:x="urn:schemas-microsoft-com:office:excel" xmlns="http://www.w3.org/TR/REC-html40"><head><!--[if gte mso 9]><xml><x:ExcelWorkbook><x:ExcelWorksheets><x:ExcelWorksheet><x:Name>{worksheet}</x:Name><x:WorksheetOptions><x:DisplayGridlines/></x:WorksheetOptions></x:ExcelWorksheet></x:ExcelWorksheets></x:ExcelWorkbook></xml><![endif]--><meta http-equiv="content-type" content="text/plain; charset=UTF-8"/></head><body><table><tr><td><table>{table}</table></td></tr></table></body></html>'
            , format = function (s, c) { return s.replace(/{(\w+)}/g, function (m, p) { return c[p]; }) }
		return function (table, name) {
			let ctx = { worksheet: name || 'Worksheet', table: table }
			return format(template, ctx);
		}
	})()
	Download(tableToExcel(data), "data:application/vnd.ms-excel;charset=UTF-8", filename||"report.xls");
}

function Csv2Json(Text, Headers) {
	var lines = Text.trim().split("\n").map(c => c.split("\t").map(d => d.trim()));
	var headers = lines[0];
	if (typeof Headers == "object" && Headers.length === 0)
		for (let h of headers) Headers.push(h);
	lines = lines.slice(1);
	return lines.map(c => {
		var tmp = {};
		c.forEach((d, i) => tmp[headers[i]] = d);
		return tmp;
	});
}

function isUndef(x, y) {
	if (x === undefined) return y;
	return x;
}

function Csv2Json_HeaderMap(Text, map, Headers) {
	var lines = Text.trim().split("\n").map(c => c.split("\t").map(d => d.trim()));
	var headers = lines[0];
	if (typeof Headers == "object" && Headers.length === 0)
		for (let h of headers) Headers.push(map[h]);
	lines = lines.slice(1);
	return lines.map(c => {
		var tmp = {};
		c.forEach((d, i) => tmp[map[headers[i]]] = d);
		return tmp;
	});
}

function serialize(args, fnc, p) {
	var i = 0;
	function onReturn(r) {
		i++;
		if (i < args.length)
			return ser();
		return p;
	}
	function ser() {
		return fnc(args[i], i).then(onReturn, onReturn);
	}
	return ser();
}

function serializeList(tmpData, fnc, Count) {
	function onReturn(r) {
		if (tmpData.length) {
			return ser();
		}
		return null;
	}
	function ser() {
		let dt = tmpData.slice(0, Count);
		tmpData = tmpData.slice(Count);
		return fnc(dt).then(onReturn);
	}
	return ser();
}

function serializeListAsync(tmpData, fnc, Count, Threads) {
	let count = Math.ceil(tmpData.length / Threads);

	return new Promise(function (resolve, reject) {
		let c = 0;
		for (let i = 0; i < Threads; i++) {
			let dt = tmpData.slice(0, count);
			tmpData = tmpData.slice(count);
			serializeList(dt, fnc, Count).then(function (r) {
				c++;
				if (c == Threads) resolve();
			});
		}
	});
}

function parallelize(tmpData, fnc, Count) {
	return new Promise(function (resolve, reject) {
		let c = 0;
		let Threads = 0;
		while (tmpData.length) {
			Threads++;
			let dt = tmpData.slice(0, Count);
			tmpData = tmpData.slice(Count);

			let ret = fnc(dt);

			if (ret.then) {
				ret.then(function (r) {
					c++;
					if (c == Threads) resolve();
				});
			} else {
				c++;
				if (c == Threads) resolve();
			}
		}
	});

	return helper();
}

function NPV(r, cf){ return cf.map((c, i) => (c/( (1 + r)**(i+1)))).reduce((a, c) => a + c,0); }

function GradientDescent(ErrorFunc, Model, options, min){

	let { 
		Rate, 
		RMSProp, 
		Momentum, 
		dx,
		Iterations,
		StopCondition
	} = (options || {});

	Momentum = Momentum || 0.9;
	RMSProp = RMSProp || 0.9999;
	dx = dx || 0.000001;

	StopCondition = StopCondition || 0.00001
	Iterations = Iterations || 1000

	let iRMSprop = 1 - RMSProp;
	let mom = 0;
	Gradient = 0;
	let rms = 1.00;

	let update = 0;

    
	let Error = ErrorFunc(Model);

	min = min || {}
	if(min.Error === undefined){
		min.Error = Error;
		min.Value = Model;
	}

	Rate = Rate || 0.001;
    
	function Iterate(dx){
		let nerr = ErrorFunc(Model + dx);
		let dif = Error - nerr;
		Gradient = dif / dx;

		rms = RMSProp * rms + iRMSprop * Gradient * Gradient;
		Gradient /= Math.sqrt(rms + 0.00000001);
		mom = mom * Momentum + Rate * Gradient;

		Model += mom;
        
		Error = ErrorFunc(Model);
		if(Error<min.Error) {
			min.Value = Model;
			min.Error = Error;
		}
	}

	for(let i = 0; i < Iterations && Math.abs(Error) > StopCondition; i++){
		Iterate(dx);
	}
	return Model;
}

function IRR(CF, m){
	let x = CF[0];
	CF = CF.slice(1);
	let mins = {}
	let fnc = (m => Math.abs(NPV(m, CF) + x));
	m = m || 0;
	for(let i = 0; i < 50; i++){
		m = GradientDescent(fnc, m, null, mins);
	}
	return mins.Value;
}