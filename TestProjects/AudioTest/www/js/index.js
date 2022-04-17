app.controller("root", ["$scope", "$rootScope", "$http", "$mdToast", "$mdDialog", "$state", "roles", "modalFactory", "user", "config", function ($scope, $rootScope, $http, $mdToast, $mdDialog, $state, roles, modalFactory, user, config) {
     console.log("root Controller");
   /*window.pvxWebSockets.connect().then(ConnectionId => {
        window.pvxWebSockets.Client.SetTime = (tm) => {
            $rootScope.CurTime = tm.cur;
        }
    });*/
    $scope.exit = function () { $http.post("/api/exit"); };
    $scope.menu = { collapsed: true }
    $scope.Roles = roles;
    $rootScope.Config = config;
    $rootScope.User = user;
    $rootScope.IsInRole = function (r) {
        if (!r) return true;
        if (!$rootScope.User) return false;
        if (Array.isArray(r)) return r.length == 0 || !!r.find(c => $rootScope.User.Roles.find(d => d == c));
        return $rootScope.User.Roles.find(d => d == r);
    }
    if ($state.is("root")) {
        let x = config.navMenu.find(c => $rootScope.IsInRole(c.Roles)).sref.match(/([^\(]*)(\([^\)]*\))?/);
        $state.go(x[1], eval(x[2]||"({})"));
    }

    $scope.$mdDialog = $mdDialog;

    $scope.showLogin = function () {
        modalFactory("LoginPanel")
    }
    $scope.logOff = function () {
        $http.post("/account/logoff").then(() => {
            $rootScope.User = null;
            window.location.replace("/");
        });
    }

    $rootScope.showMessage = function (message, delay) {
        $mdToast.show({
            hideDelay: delay || 3000,
            position: 'top left',
            template: `<md-toast role="alert"><div>${message}</div></md-toast>`
        });
    }
    $rootScope.DefaultMessage = function (prom) {
        prom.then(function () {
            $rootScope.showMessage("OK");
        }, function () {
            $rootScope.showMessage("Fail");
        });
    }
    $rootScope.md_jsonEditor = function (data) {
        return $mdDialog.show({
            templateUrl: '/modals/jsoneditor.html',
            clickOutsideToClose: true,
            controller: ["$scope", "$mdDialog", "text", function ($scope, $mdDialog, text) {
                $scope.$mdDialog = $mdDialog;
                $scope.ref = { text: text };
                $scope.format = function () { $scope.ref.text = JSON.stringify(angular.fromJson($scope.ref.text), null, "\t"); };
                $scope.ok = function () { $mdDialog.hide(JSON.parse($scope.ref.text)); }
            }],
            locals: {
                text: JSON.stringify(data, null, '\t')
            }
        });
    };
}])