var app = angular.module("app", ["ngMaterial", "ngSanitize", "ui.router","ui.ace", "ng"]);
app.config(["$stateProvider", "$urlRouterProvider", "$locationProvider", "$mdThemingProvider", function ($stateProvider, $urlRouterProvider, $locationProvider, $mdThemingProvider) {
	$locationProvider.html5Mode(true);
	$stateProvider.
		state("root", {
			url: "/",
			templateUrl: '/views/layout.html',
			controller: "root",
			resolve: {
				roles: ["$http", "$rootScope", ($http, $rootScope) => $http.post("/account/roles").then(r => ($rootScope.Roles = r.data))],
				user: [function () {
					let cookie = document.cookie.match(/pvx-token=([^\&]+)/);
					if(cookie) return JSON.parse(Base64.decode(cookie[1].slice(44)).replace(/\0+$/, ""))
					return null;
				}],
				config: ["$http", $http => $http.post("/api/config").then(r => r.data)]
            }
		}).
		state("root.view", {
			url: "view/{name}",
			template: params => `<div style="height:calc(100vh - 36px - 16px)" emc-directive="${params.name}">Directive Not Found</div>`
		});
	$mdThemingProvider.definePalette('veryperi', {
		'50': 'ededf5',
		'100': 'd1d1e6',
		'200': 'b3b3d5',
		'300': '9495c4',
		'400': '7d7eb8',
		'500': '6667ab',
		'600': '5e5fa4',
		'700': '53549a',
		'800': '494a91',
		'900': '383980',
		'A100': 'cfd0ff',
		'A200': '9c9eff',
		'A400': '696bff',
		'A700': '5052ff',
		'contrastDefaultColor': 'dark',
		'contrastDarkColors': [
			'50',
			'100',
			'200',
			'300',
			'400',
			'A100',
			'A200'
		],
		'contrastLightColors': [
			'500',
			'600',
			'700',
			'800',
			'900',
			'A400',
			'A700'
		]
	});

	$mdThemingProvider.theme('default')
		.primaryPalette('veryperi', {
			'default': '700'
		})
		.accentPalette('grey', {
			'default': '600'
		});
}]);
app.run(["$rootScope", "$hash", function ($rootScope, $hash) {
	window.addEventListener('hashchange', event => {
		$rootScope.$broadcast("$hashchange", $hash())
	});
}])
