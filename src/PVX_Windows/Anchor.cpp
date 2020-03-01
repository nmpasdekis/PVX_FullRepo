#include <PVX_Window.h>

namespace PVX::Windows {

	struct PrivateData {
		int Point;
		RECT rc;
	};

	Anchor::Anchor(HWND Main) {
		SetMain(Main);
	}
	Anchor::Anchor() {}
	Anchor::~Anchor() {
		for (auto & a : Anchors)
			delete ((PrivateData*)a.second);
	}
	void Anchor::SetMain(HWND main) {
		Main = main;
		LONG_PTR d = GetWindowLongPtr(Main, DWLP_DLGPROC)&(~0x0fff);
		LONG_PTR x = ((LONG_PTR)DefWindowProc)&(~0x0fff);
		IsDialog = (d != x);

		GetClientRect(main, &MainRect);
	}
	void Anchor::Set(int Point, HWND control) {
		WINDOWPLACEMENT wp;
		GetWindowPlacement(control, &wp);
		PrivateData * pd;
		auto an = Anchors.find(control);
		if (Point != TopLeft) {
			if (an == Anchors.end()) {
				pd = new PrivateData();
				Anchors[control] = pd;
			} else
				pd = (PrivateData*)Anchors[control];
			pd->Point = Point;
			pd->rc = wp.rcNormalPosition;
		} else if (an != Anchors.end()) {
			pd = (PrivateData*)Anchors[control];
			delete pd;
			Anchors.erase(control);
		}
	}
	void Anchor::Set(int Point, int ctrlId) {
		if (IsDialog)
			Set(Point, GetDlgItem(Main, ctrlId));
		else
			Set(Point, GetDlgItem(Main, ctrlId));
	}
	void Anchor::Set(int Point, const std::vector<HWND> ctrlIds) {
		Set(Point, ctrlIds.data(), (int)ctrlIds.size());
	}
	void Anchor::Set(int Point, const int ctrlIds[], int count) {
		for (int i = 0; i < count; i++)
			Set(Point, ctrlIds[i]);
	}
	void Anchor::Set(int Point, const std::vector<int> ctrlIds) {
		Set(Point, ctrlIds.data(), (int)ctrlIds.size());
	}
	void Anchor::Set(int Point, const HWND ctrlIds[], int count) {
		for (int i = 0; i < count; i++)
			Set(Point, ctrlIds[i]);
	}
	void Anchor::Apply() {
		WINDOWPLACEMENT wp;
		GetClientRect(Main, &wp.rcNormalPosition);
		for (auto &[hWnd, Point] : Anchors) {
			WINDOWPLACEMENT wp2 = {};
			GetWindowPlacement(hWnd, &wp2);
			PrivateData & pd = *(PrivateData*)Point;
			if (!(pd.Point & Top))
				wp2.rcNormalPosition.top = (wp.rcNormalPosition.bottom - wp.rcNormalPosition.top) - (MainRect.bottom - pd.rc.top);
			if (!(pd.Point & Left))
				wp2.rcNormalPosition.left = (wp.rcNormalPosition.right - wp.rcNormalPosition.left) - (MainRect.right - pd.rc.left);
			if (pd.Point & Bottom)
				wp2.rcNormalPosition.bottom = (wp.rcNormalPosition.bottom - wp.rcNormalPosition.top) - (MainRect.bottom - pd.rc.bottom);
			if (pd.Point & Right)
				wp2.rcNormalPosition.right = (wp.rcNormalPosition.right - wp.rcNormalPosition.left) - (MainRect.right - pd.rc.right);
			SetWindowPlacement(hWnd, &wp2);
		}
	}
}