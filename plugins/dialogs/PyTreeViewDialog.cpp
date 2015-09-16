#include "PyTreeViewDialog.h"
#include "PyTreeViewItem.h"
using namespace std;

#include "PyTreeViewDialog.h"
using namespace std;

static void PyTreeViewDialogDealloc (PyObject* pySelf) {
PyTreeViewDialog* self = (PyTreeViewDialog*)pySelf;
delete self->signals;
Py_TYPE(pySelf)->tp_free(pySelf);
}

static int PyTreeViewDialogInit (PyTreeViewDialog* self, PyObject* args, PyObject* kwds) {
return 0;
}

static PyMethodDef PyTreeViewDialogMethods[] = {
PyDecl("addEvent", &PyTreeViewDialog::addEvent),
PyDecl("removeEvent", &PyTreeViewDialog::removeEvent),
PyDecl("close", &PyTreeViewDialog::close),
PyDecl("focus", &PyTreeViewDialog::focus),
PyDeclEnd
};

#define Prop(x) PyAccessor(#x, &PyTreeViewDialog::get_##x, &PyTreeViewDialog::set_##x)
#define RProp(x) PyReadOnlyAccessor(#x, &PyTreeViewDialog::get_##x)
static PyGetSetDef PyTreeViewDialogAccessors[] = {
RProp(root), RProp(selection),  RProp(closed),
Prop(title), Prop(text),
PyDeclEnd
};
#undef Prop
#undef RProp

static PyTypeObject PyTreeViewDialogType = { 
    PyVarObject_HEAD_INIT(NULL, 0) 
    "qc6paddlgs.TreeViewDialog",             /* tp_name */ 
    sizeof(PyTreeViewDialog), /* tp_basicsize */ 
    0,                         /* tp_itemsize */ 
    PyTreeViewDialogDealloc,                         /* tp_dealloc */ 
    0,                         /* tp_print */ 
    0,                         /* tp_getattr */ 
    0,                         /* tp_setattr */ 
    0,                         /* tp_reserved */ 
    0,                         /* tp_repr */ 
    0,                         /* tp_as_number */ 
    0,                         /* tp_as_sequence */ 
    0,                         /* tp_as_mapping */ 
    0,                         /* tp_hash  */ 
    0,                         /* tp_call */ 
    0,                         /* tp_str */ 
    0,                         /* tp_getattro */ 
    0,                         /* tp_setattro */ 
    0,                         /* tp_as_buffer */ 
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /* tp_flags */ 
    NULL,           /* tp_doc */
    0,                         /* tp_traverse */ 
    0,                         /* tp_clear */ 
    0,                         /* tp_richcompare */ 
    0,                         /* tp_weaklistoffset */ 
    0,                         /* tp_iter */ 
    0,                         /* tp_iternext */ 
    PyTreeViewDialogMethods,             /* tp_methods */ 
NULL,             /* tp_members */ 
    PyTreeViewDialogAccessors,                         /* tp_getset */ 
    0,                         /* tp_base */ 
    0,                         /* tp_dict */ 
    0,                         /* tp_descr_get */ 
    0,                         /* tp_descr_set */ 
    0,                         /* tp_dictoffset */ 
    (initproc)PyTreeViewDialogInit,      /* tp_init */ 
    0,                         /* tp_alloc */ 
}; 

PyTreeViewDialog* PyTreeViewDialog::New (HWND hd, HWND ht) {
GIL_PROTECT
PyTreeViewDialog* t = (PyTreeViewDialog*) PyTreeViewDialogType.tp_alloc(&PyTreeViewDialogType,0);
t->hDlg = hd;
t->hTree = ht;
t->signals = new TVDSignals();
return t;
}

void PyTreeViewDialog::Delete () {
GIL_PROTECT
Py_XDECREF( (PyObject*)this );
}

bool PyRegister_TreeViewDialog (PyObject* m) {
//PyTreeViewDialogType.tp_new = (decltype(PyTreeViewDialogType.tp_new))PyTreeViewDialogNew;
if (PyType_Ready(&PyTreeViewDialogType) < 0)          return false;
Py_INCREF(&PyTreeViewDialogType); 
PyModule_AddObject(m, "TreeViewDialog", (PyObject*)&PyTreeViewDialogType); 
return true;
}

bool PyTreeViewDialog::allowEdit (HTREEITEM item) {
bool re = true;
if (!signals->onedit.empty()) re = signals->onedit((PyObject*)this, (PyObject*)PyTreeViewItem::New(hTree, item));
return re;
}

bool PyTreeViewDialog::allowEdit (HTREEITEM item, tstring& text) {

if (!signals->onedited.empty()) {
var re = signals->onedited((PyObject*)this, (PyObject*)PyTreeViewItem::New(hTree, item), text);
if (re.getType()==T_BOOL && !re) return false;
else if (re.getType()==T_STR) text = re.toTString();
}
return text.size()>0;
}

PyObject* PyTreeViewDialog::get_root () {
return (PyObject*) PyTreeViewItem::New(hTree, TVI_ROOT);
}

PyObject* PyTreeViewDialog::get_selection () {
HTREEITEM selection = (HTREEITEM)SendMessage(hTree, TVM_GETNEXTITEM, TVGN_CARET, 0);
if (selection) return (PyObject*) PyTreeViewItem::New(hTree, selection);
else { Py_RETURN_NONE; }
}

int PyTreeViewDialog::get_closed () {
return closed;
}

void PyTreeViewDialog::close () {
if (!closed) SendMessage(hDlg, WM_COMMAND, IDCANCEL, 0);
}

tstring PyTreeViewDialog::get_title () {
return GetWindowText(hDlg);
}

tstring PyTreeViewDialog::get_text  () {
return GetDlgItemText(hDlg, 1001);
}

void PyTreeViewDialog::set_title (const tstring& title) {
SetWindowText(hDlg, title);
}

void PyTreeViewDialog::set_text (const tstring& text) {
SendMessage(hDlg, WM_USER, 1001, text.c_str());
}

void PyTreeViewDialog::focus () {
if (!closed) RunSync([&]()mutable{
ShowWindow(hDlg, SW_SHOW);
SetForegroundWindow(hDlg);
});//
}

int PyTreeViewDialog::addEvent (const string& type, const PySafeObject& cb) {
connection con;
if(false){}
#define E(n) else if (type==#n) con = signals->on##n .connect(cb.asFunction<typename decltype(signals->on##n)::signature_type>());
E(action) E(select) E(expand) E(contextMenu)
E(edit) E(edited) E(close) E(focus) E(blur)
E(keyDown) E(keyUp)
#undef E
if (con.connected()) return AddSignalConnection(con);
else return 0;
}

int PyTreeViewDialog::removeEvent (const string& type, int id) {
connection con = RemoveSignalConnection(id);
bool re = con.connected();
con.disconnect();
return re;
}

static LRESULT CALLBACK TreeViewSubclassProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR subclassId, PyTreeViewDialog* dlg) {
switch(msg){
case WM_KEYDOWN: case WM_SYSKEYDOWN: {
int kc = LOWORD(wp) | GetCurrentModifiers();
if (!dlg->signals->onkeyDown((PyObject*)dlg, kc)) return true;
}break;
case WM_KEYUP: case WM_SYSKEYUP: {
int kc = LOWORD(wp) | GetCurrentModifiers();
if (!dlg->signals->onkeyUp((PyObject*)dlg, kc)) return true;
}break;
}
return DefSubclassProc(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK EditingTreeLabelProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR subclassId, HWND hTree) {
if (msg==WM_KEYDOWN) switch(LOWORD(wp)){
case VK_RETURN: SendMessage(hTree, TVM_ENDEDITLABELNOW, FALSE, 0); return true;
case VK_ESCAPE: SendMessage(hTree, TVM_ENDEDITLABELNOW, TRUE, 0); return true;
}
else if (msg==WM_GETDLGCODE) return DLGC_WANTALLKEYS;
return DefSubclassProc(hwnd, msg, wp, lp);
}

INT_PTR TreeViewDlgProc (HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp) {
switch (umsg) {
case WM_INITDIALOG : {
HWND hTree = GetDlgItem(hwnd, 1002);
TreeViewDialogInfo& tvdi = *(TreeViewDialogInfo*)(lp);
tvdi.dlg = PyTreeViewDialog::New(hwnd, hTree);
PyTreeViewDialog& dlg = *tvdi.dlg;
dlg.hTree = hTree;
Py_XINCREF(&dlg);
SetWindowLong(hwnd, DWL_USER, (LONG)tvdi.dlg);
SetWindowText(hwnd, tvdi.title);
SetDlgItemText(hwnd, 1001, tvdi.label);
SetDlgItemText(hwnd, IDOK, msg("&OK"));
SetDlgItemText(hwnd, IDCANCEL, msg("&Close"));
SetWindowSubclass(hTree, (SUBCLASSPROC)TreeViewSubclassProc, 0, (DWORD_PTR)&dlg);
SetFocus(hTree);
sp->AddModlessWindow(hwnd);
}return false;
case WM_COMMAND :
switch(LOWORD(wp)) {
case IDOK: {
if (GetFocus()!=GetDlgItem(hwnd,1002)) break;
PyTreeViewDialog& dlg = *(PyTreeViewDialog*)GetWindowLong(hwnd, DWL_USER);
bool modal = !!GetWindowLong(hwnd, GWL_USERDATA), re = modal;
if (!dlg.signals->onaction.empty()) re = dlg.signals->onaction((PyObject*)&dlg, dlg.get_selection());
if (!re) break; 
}
case IDCANCEL: {
PyTreeViewDialog& dlg = *(PyTreeViewDialog*)GetWindowLong(hwnd, DWL_USER);
bool modal = !!GetWindowLong(hwnd, GWL_USERDATA), re = true;
if (!dlg.signals->onclose.empty()) re = dlg.signals->onclose((PyObject*)&dlg);
if (!re) return true;
if (modal) EndDialog(hwnd,1);
else {
sp->RemoveModlessWindow(hwnd);
DestroyWindow(hwnd);
sp->GoToNextModlessWindow(0);
}
dlg.closed=true; 
dlg.Delete();
}break;//IDCANCEL
}break;//WM_COMMAND
case WM_NOTIFY : {
LPNMHDR nmh = (LPNMHDR)(lp);
if (nmh->idFrom==1002) {
PyTreeViewDialog& dlg = *(PyTreeViewDialog*)GetWindowLong(hwnd, DWL_USER);
switch(nmh->code){
case NM_DBLCLK:
case NM_RETURN:
SendMessage(hwnd, WM_COMMAND, IDOK, 0);
return true;
case NM_RCLICK:
if (!dlg.signals->oncontextMenu.empty()) dlg.signals->oncontextMenu((PyObject*)&dlg, dlg.get_selection());
return true;
case NM_RDBLCLK:
break;
case TVN_SELCHANGED: {
TVITEM& item = ((LPNMTREEVIEW)lp)->itemNew;
if (!dlg.signals->onselect.empty()) dlg.signals->onselect((PyObject*)&dlg, (PyObject*)PyTreeViewItem::New(nmh->hwndFrom, item.hItem));
}break;
case TVN_ITEMEXPANDING: {
if (!(((LPNMTREEVIEW)lp)->action & TVE_EXPAND)) return false;
TVITEM& item = ((LPNMTREEVIEW)lp)->itemNew;
bool re = true;
if (!dlg.signals->onexpand.empty()) re = dlg.signals->onexpand((PyObject*)&dlg, (PyObject*)PyTreeViewItem::New(nmh->hwndFrom, item.hItem));
return !re;
}break;
case TVN_DELETEITEM : {
TVITEM& item = ((LPNMTREEVIEW)lp)->itemOld;
{ GIL_PROTECT Py_XDECREF((PyObject*)item.lParam); }
}break;
case TVN_BEGINLABELEDIT: {
TVITEM& item = ((LPNMTVDISPINFO)lp)->item;
if (!dlg.allowEdit(item.hItem)) return true;
HWND hEdit = (HWND)SendMessage(nmh->hwndFrom, TVM_GETEDITCONTROL, 0, 0);
if (hEdit) SetWindowSubclass(hEdit, (SUBCLASSPROC)EditingTreeLabelProc, 0, (DWORD_PTR)nmh->hwndFrom);
return false;
}break;
case TVN_ENDLABELEDIT: {
TVITEM& item = ((LPNMTVDISPINFO)lp)->item;
if (item.pszText) {
tstring text = item.pszText;
if (!dlg.allowEdit(item.hItem, text)) return false;
item.mask = TVIF_TEXT;
item.pszText = (LPTSTR)text.c_str();
item.cchTextMax = text.size();
SendMessage(nmh->hwndFrom, TVM_SETITEM, 0, &item);
}
return true;
}break;
case TVN_KEYDOWN: {
WORD key = *(WORD*)(nmh+1);
switch(key){
case 0x5D: { 
NMHDR z = *(LPNMHDR)lp; z.code = NM_RCLICK;
SendMessage(hwnd, WM_NOTIFY, 0, &z);
}break;
case VK_F2: SendMessage(nmh->hwndFrom, TVM_EDITLABEL, 0, SendMessage(nmh->hwndFrom, TVM_GETNEXTITEM, TVGN_CARET, NULL)); break;
}return false;}
//other notifications
}}}break;
case WM_ACTIVATE: {
PyTreeViewDialog& dlg = *(PyTreeViewDialog*)GetWindowLong(hwnd, DWL_USER);
if (LOWORD(wp)) dlg.signals->onfocus((PyObject*)&dlg);
else dlg.signals->onblur((PyObject*)&dlg);
}break;
case WM_USER: SetDlgItemText(hwnd, LOWORD(wp), (LPCTSTR)lp); break;
}
return FALSE;
}

PyObject* test123 (void) {
TreeViewDialogInfo tvdi = { TEXT("Tree view dialog"), TEXT("Tree view control"), NULL };
HWND hDlg = CreateDialogParam(hinstance, IDD_TREEVIEW, sp->win, TreeViewDlgProc, &tvdi);
ShowWindow(hDlg, SW_SHOW);
return (PyObject*) tvdi.dlg;
}
