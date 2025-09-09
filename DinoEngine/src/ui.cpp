#include "pch.h"

#include "ui.h"

void UITextButton::set_text(const char *text) {
  console_log("Settings text to %s", text);
}

void UITextButton::set_on_click(button_callback_t callback) {
  console_log("Settings onCLick");
}

void UIMenu::open() {
}

void UIMenu::close() {}