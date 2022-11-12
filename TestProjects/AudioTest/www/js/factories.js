let modalCache = {};
app.factory('modalFactory', ["$http", "$mdDialog", function ($http, $mdDialog) {
    function show(options) {
        if (options.controller.includes("data")) {
            if (!options.locals) options.locals = { data: {} };
            else if (!options.locals.data) options.locals.data = {};
        }
        return $mdDialog.show({ ...modalCache[name], ...options });
    }
    return function (name, options, data, debug) {
        options = options || {};
        if (data) {
            options.locals = options.locals || {};
            options.locals.data = data;
        }
        if (modalCache[name] && !debug) {
            return show({ ...modalCache[name], ...options });
        }
        return $http.post("/api/view/GetCustomView", { Name: name }).then(function (r) {
            modalCache[name] = {
                template: r.data.Html,
                controller: eval(r.data.Controller)
            }
            return show({ ...modalCache[name], ...options });
        });
    }
}]);
app.factory('modalFactory_dbg', ["$http", "$mdDialog", function ($http, $mdDialog) {
    function show(options) {
        if (options.controller.includes("data")) {
            if (!options.locals) options.locals = { data: {} };
            else if (!options.locals.data) options.locals.data = {};
        }
        return $mdDialog.show({ ...modalCache[name], ...options });
    }
    return function (name, options, data) {
        options = options || {};
        if (data) {
            options.locals = options.locals || {};
            options.locals.data = data;
        }
        return $http.post("/api/view/GetCustomView", { Name: name }).then(function (r) {
            modalCache[name] = {
                template: r.data.Html,
                controller: eval(r.data.Controller)
            }
            return show({ ...modalCache[name], ...options });
        });
    }
}]);
app.factory('modal2Factory', ["$http", "$mdDialog", function ($http, $mdDialog) {
    function show(options) {
        return $mdDialog.show(options);
    }
    return function (name, options, data) {
        options = options || {};
        if (data) {
            options.locals = {
                ...(options.locals || {}),
                ...data
            }
        }
        if (modalCache[name]) {
            return show({ ...modalCache[name], ...options });
        }
        return $http.post("/api/view/GetCustomView", { Name: name }).then(function (r) {
            modalCache[name] = {
                template: r.data.Html,
                controller: eval(r.data.Controller)
            }
            return show({ ...modalCache[name], ...options });
        });
    }
}]);
app.factory('modal2Factory_dbg', ["$http", "$mdDialog", function ($http, $mdDialog) {
    function show(options) {
        return $mdDialog.show(options);
    }
    return function (name, options, data) {
        options = options || {};
        if (data) {
            options.locals = {
                ...(options.locals || {}),
                ...data
            }
        }
        return $http.post("/api/view/GetCustomView", { Name: name }).then(function (r) {
            modalCache[name] = {
                template: r.data.Html,
                controller: eval(r.data.Controller)
            }
            return show({ ...modalCache[name], ...options });
        });
    }
}]);
app.factory('modaldbgFactory', ["$http", "$mdDialog", function ($http, $mdDialog) {
    function show(options) {
        if (options.controller.includes("data")) {
            if (!options.locals) options.locals = { data: {} };
            else if (!options.locals.data) options.locals.data = {};
        }
        return $mdDialog.show({ ...modalCache[name], ...options });
    }
    return function (name, options, data, debug) {
        options = options || {};
        if (data) {
            options.locals = options.locals || {};
            options.locals.data = data;
        }
        return $http.post("/api/view/GetCustomView", { Name: name }).then(function (r) {
            modalCache[name] = {
                template: r.data.Html,
                controller: eval(r.data.Controller)
            }
            return show({ ...modalCache[name], ...options });
        });
    }
}]);
app.factory("$hash", ["$location", function ($location) {
    function f(obj) {
        let ret = UriToObject($location.hash());
        if (obj) {
            Object.keys(obj).forEach(c => {
                if (obj[c] === undefined) delete ret[c];
                else ret[c] = obj[c];
            });
            $location.hash(Object.keys(ret).map(c => {
                return c + "=" + (Array.isArray(ret[c]) ? ret[c].join(",") : ret[c]);
            }).join("&"));
        }
        return ret;
    }
    return f;
}]);

app.factory("webGL2", ["$timeout", "$q", function ($timeout, $q) {
    return function (selector) {
        function init(r) {
            let canvas = document.querySelector(selector);
            if (canvas) {
                r(getWebGL2Context(canvas));
                return;
            }
            $timeout(() => init(r));
        }
        return $q(init);
    }
}]);