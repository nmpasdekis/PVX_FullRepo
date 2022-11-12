angular.module("app", ["ngMaterial", "ngSanitize", "ng"]).controller("ctrl", ["$scope", "$http", "$interval", function ($scope, $http, $interval) {
    $scope.temps = Array.from({ length: 17 }).map((d, c) => c + 16);
    $scope.mode = 0;
    $scope.fan = 0;
    $scope.temp = 24;
    function getState() {
        $http.post("/api/ac/state").then(r => {
            $scope.mode = r.data.mode;
            $scope.fan = r.data.fan;
            $scope.temp = r.data.temp;
        })
    }
    getState();
    let stateInt = $interval(getState, 10000);
    $scope.$on("$destroy", () => {
        debugger;
        $interval.cancel(stateInt);
    });
    
    function go(on, mode, fan, temp) {
        return $http.post("/api/ac/command", {
            on: on,
            mode: mode,
            fan: fan,
            temp: temp
        })
    }
    $scope.command = go;
}])