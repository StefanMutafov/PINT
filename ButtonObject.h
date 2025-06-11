#ifndef MY_BUTTON_OBJECT
#define MY_BUTTON_OBJECT

#include "GUI_Object.h"
#include <functional>

class ButtonObject : public GUI_Object {
  public:
  ButtonObject(TFT_eSprite& screen, std::function<void()> on_trigger, String button_text, uint8_t font, uint16_t x, uint16_t y, int width, int height, GUI_Object::DATUM datum = GUI_Object::D_TL, uint16_t text_colour = TFT_BLACK, uint16_t box_colour = TFT_WHITE, uint16_t box_selected_colour = TFT_SILVER, uint16_t box_border_colour = TFT_BLUE);

  
  void update_value(String value) override;
  void draw() override;

  void select();
  void deselect();
  std::function<void()> trigger;

  private:
  String m_text;
  uint8_t m_font;
  uint16_t m_text_colour;
  uint16_t m_box_colour;
  uint16_t m_selected_colour;
  uint16_t m_border_colour;
  bool m_selected;

};

#endif