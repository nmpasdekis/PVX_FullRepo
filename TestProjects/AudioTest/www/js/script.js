app.controller("root", ["$scope", "$http", "$q", function ($scope, $http, $q) {
    $q.all([$http.post("/api/device/output/names"), $http.post("/api/device/input/names")]).then(r => {
        $scope.Devices = r[0].data;
        $scope.CaptureDevices = r[1].data;
    });
    $scope.Start = () => {
        $http.post("/api/play");
    }
    $scope.Stop = () => {
        $http.post("/api/stop");
    }
}]);