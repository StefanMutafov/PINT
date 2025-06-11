#include "ButtonObject.h"

ButtonObject::ButtonObject(TFT_eSprite& screen, std::function<void()> on_trigger, String button_text, uint8_t font, uint16_t x, uint16_t y, int width, int height, GUI_Object::DATUM datum, uint16_t text_colour, uint16_t box_colour, uint16_t box_selected_colour, uint16_t box_border_colour) 
  : GUI_Object{ screen, GUI_Object::BUTTON, x, y, width, height, datum}
  , trigger{ on_trigger }
  , m_text{ button_text }
  , m_font{ font }
  , m_text_colour{ text_colour }
  , m_box_colour{ box_colour }
  , m_selected_colour{ box_selected_colour }
  , m_border_colour{ box_border_colour }
  , m_selected{ false }
{

}

void ButtonObject::update_value(String value)
{
  m_text = value;
}

void ButtonObject::select()
{
  m_selected = true;
}
void ButtonObject::deselect() 
{
  m_selected = false;
}

void ButtonObject::draw()
{
  m_screen.fillRect(topleft_x(), topleft_y(), m_width, m_height, m_selected ? m_selected_colour : m_box_colour);
  m_screen.drawRect(topleft_x(), topleft_y(), m_width, m_height, m_border_colour);
  m_screen.drawRect(topleft_x() + 1, topleft_y() + 1, m_width - 2, m_height - 2, m_border_colour);

  m_screen.setTextDatum(MC_DATUM);
  m_screen.setTextColor(m_text_colour);
  m_screen.drawString(m_text, topleft_x() + m_width/2, topleft_y() + m_height/2, m_font);
  
}
