#include "PCH.h"
#include "IEditorWindow.h"

IEditorWindow::IEditorWindow(const std::string& name, bool defaultOpen) {
	name_ = name;
	isOpen_ = defaultOpen;
}