function equals(a, b) {
    if (a === b) return true;
    if (typeof a != typeof b) return false;
    if (Array.isArray(a)) {
        return a.length == b.length && -1 == a.findIndex((c, i) => !equals(c, b[i]));
    }
    let ka = Object.keys(a);
    return ka.length == Object.keys(b).length && -1 == ka.findIndex(k => !equals(a[k], b[k]));
}

function deepCopy(x) {
    if (typeof x != "object") return x;
    if (Array.isArray(x)) return x.map(c => deepCopy(c));
    let ret = {};
    Object.keys(x).forEach(c => ret[c] = deepCopy(x[c]));
    return ret;
}

var lastObjects = {};
var Objects = {};
var clear = {};

function Watch(id) {
    if (!equals(lastObjects[id], Objects[id])) {
        postMessage(id);
    }
    if (!clear[id]) setTimeout(() => Watch(id), 100);
    else delete clear[id];
}

onmessage = function (e) {
    if (e.data.do === "set") {
        Objects[e.data.id] = e.data.Object;
        lastObjects[e.data.id] = deepCopy(e.data.Object);
        Watch(e.data.id);
    } else if (e.data.do === "clear") {
        clear[e.data.id] = true;
    }
}