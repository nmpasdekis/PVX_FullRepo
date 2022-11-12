(function VanillaEvents(list) {
	list.forEach(event => {
		let dirName = "em" + event[0].toUpperCase() + event.substring(1).toLowerCase();
		app.directive(dirName, ['$parse', '$rootScope', function ($parse, $rootScope) {
			return {
				restrict: 'A',
				compile: function ($element, attr) {
					var fn = $parse("(" + attr[dirName] + ")", null, true);

					return function ngDragEventHandler(scope, element) {
						element.on(event, function (event) {
							var callback = function () {
								event.setData = function (data) {
									event.originalEvent.dataTransfer.setData("text", JSON.stringify(data))
								}
								event.getData = function () {
									return JSON.parse(event.originalEvent.dataTransfer.getData("text"));
								}
								fn(scope, { $event: event });
							};

							scope.$apply(callback);
						});
					};
				}
			};
		}])
	})
})(["dragenter", "dragleave", "dragover", "drop", "dragstart", "dragstop"]);

app.directive('ngMousewheel', function () {
	return function (scope, element, attrs) {
		element.bind("DOMMouseScroll mousewheel onmousewheel", function (event) {
			scope.$apply(function () {
				scope.$event = event || window.event;
				scope.$event.wheelDelta = scope.$event.originalEvent.wheelDelta;
				scope.$eval(attrs.ngMousewheel);
				delete scope.$event;
			});
		});
	};
});

app.directive("emcDirective", ['$http', '$injector', '$compile', "$q", function ($http, $injector, $compile, $q) {
	let cacheViews = {};
	return {
		template: `<ng-transclude ng-if='!View'></ng-transclude>`,
		scope: {
			emcDirective: "@"
		},
		transclude: true,
		link: function ($scope, elem, attr) {
			let lastName;
			let lastScope = null;

			function Process(r) {
				lastName = $scope.emcDirective;
				$scope.View = r.Html;
				lastScope = $scope.$parent.$new();
				EvaluateController(r.Controller, lastScope);
				if ($scope.View) {
					elem.html($scope.View);
					$compile(elem.contents())(lastScope);
				}
			}
			function IsDebug() {
				return (attr.debug !== undefined) && (attr.debug || attr.debug === "");
			}

			function Init() {
				if (lastScope) lastScope.$destroy();
				if (cacheViews[$scope.emcDirective] && !IsDebug())
					Process(cacheViews[$scope.emcDirective]);
				else $http.post("/api/view/GetCustomView", { Name: $scope.emcDirective }).then(function (r) {
					cacheViews[$scope.emcDirective] = r.data;
					Process(r.data);
				});
			}
			let w = $scope.$watch("emcDirective", function (n, o) {
				if (n && n != o && n != lastName) Init();
			});
			$scope.$on("$destroy", function () { w(); });

			Init();

			function EvaluateController(Controller, scope) {
				if (Controller) {
					try {
						let args = [];
						let ctrl = eval(`(function DebugFunction(){try{return ${Controller};}catch(e){debugger;throw e;}})()`);
						let promices = [];
						let promiceIndex = [];
						for (let i = 0; i < ctrl.length - 1; i++) {
							let injection = ctrl[i].replace(/_cfg$/, IsDebug() ? "_dbg" : "");
							if (injection == "$scope") args.push(scope);
							else if (injection == "data") args.push(attr.data ? scope.$eval(attr.data) : {});
							else {
								let inj = $injector.get(injection);
								if (inj && inj.then) {
									promices.push(inj);
									promiceIndex.push(i);
								}
								args.push(inj);
							}
						}
						if (promices.length) {
							$q.all(promices).then(function (r) {
								r.forEach((x, j) => {
									args[promiceIndex[j]] = x;
								});
								ctrl[ctrl.length - 1].apply(null, args);
							});
						} else {
							ctrl[ctrl.length - 1].apply(null, args);
						}
					}
					catch (e) {
						console.log(e.message);
						throw e;
					}
				}
			}
		}
	};
}]);

app.directive("emcCode", [function(){
	return{
		template: (elem, attr) => `<div style="position:relative;height:100%">
	<div layout-fill ng-model="${attr.ngModel}" class="editor" ui-ace="{ mode: '${attr.mode}', theme: 'monokai', useSoftTabs: true, tabSize: 4 }"
		style="font-family: monospace; white-space:pre; position:absolute; top:0;bottom:0;left:0;right:0;"></div>
</div>`
	}
}]);

app.directive("emCode", [function () {
	return {
		template: (elem, attr) => `<div ${Object.keys(attr).filter(c => !c.indexOf("ng")).map(c => {
			return c.replace(/[A-Z]/g, c => `-${c.toLowerCase()}`) + `="${attr[c]}"`;
		}).join(" ")} class="editor" ui-ace="{ mode: '${attr.mode}', theme: 'monokai', useSoftTabs: true, tabSize: 4 }"
		style="font-family: monospace; white-space:pre;"></div>`
	}
}]);