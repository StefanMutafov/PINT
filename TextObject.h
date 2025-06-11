#ifndef MY_TEXT_OBJECT
#define MY_TEXT_OBJECT

#include "GUI_Object.h"

class TextObject : public GUI_Object {
  public:
  TextObject(TFT_eSprite& screen, String content, uint8_t font, uint16_t x, uint16_t y, GUI_Object::DATUM datum = GUI_Object::D_TL, uint8_t size = 1, uint16_t fgcolour = TFT_WHITE, uint16_t bgcolour = TFT_TRANSPARENT);

  void update_value(String value) override;
  void draw() override;

  private:
  String m_content;
  uint8_t m_font;
  uint8_t m_size;
  uint16_t m_fgcolour;
  uint16_t m_bgcolour;
  

};

#endif