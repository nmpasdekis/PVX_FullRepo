var app = angular.module("app", ["ngMaterial", "ngSanitize", "ui.router","ui.ace"]);
app.config(["$stateProvider", "$urlRouterProvider", "$locationProvider", function ($stateProvider, $urlRouterProvider, $locationProvider) {
	$locationProvider.html5Mode(true);
	$stateProvider.
		state("root", {
			url: "/",
			templateUrl: '/views/layout.html',
			controller: "root",
			resolve: {
				roles: ["$http", function ($http) {
					return $http.post("/account/roles").then(r => r.data.map(c => c.Name));
				}],
				user: [function () {
					let cookie = document.cookie.match(/pvx-token=([^\&]+)/);
					if(cookie)
						return JSON.parse(Base64.decode(cookie[1].slice(44)).replace(/\0+$/, ""))
					return null;
				}],
				config: ["$http", function ($http) {
					return $http.post("/api/config").then(r => r.data);
                }]
            }
		}).
		state("root.view", {
			url: "view/{name}",
			template: function (params) {
				return "<div emc-directive='" + params.name + "'>Directive Not Found</div>"
			}
		})
}]);