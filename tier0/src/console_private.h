#pragma once

void console_create();
void console_free();

// Seperate so that memory leak checking doesn't freak out
void console_free_filebuffer();