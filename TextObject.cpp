#include "TextObject.h"

TextObject::TextObject(TFT_eSprite& screen, String content, uint8_t font, uint16_t x, uint16_t y, GUI_Object::DATUM datum, uint8_t size, uint16_t fgcolour, uint16_t bgcolour) 
  : GUI_Object{ screen, GUI_Object::TEXT, x, y, screen.textWidth(content, font) * size, screen.fontHeight(font) * size, datum}
  , m_content{ content }
  , m_font{ font }
  , m_size{ size }
  , m_fgcolour{ fgcolour }
  , m_bgcolour{ bgcolour }
{

}

void TextObject::update_value(String value)
{
  m_content = value;
  m_width = m_screen.textWidth(m_content, m_font) * m_size;
  
}

void TextObject::draw()
{
  m_screen.setTextDatum(TL_DATUM);
  m_screen.setTextSize(m_size);
  if (m_bgcolour == TFT_TRANSPARENT)
  {
    m_screen.setTextColor(m_fgcolour);
  }
  else 
  {
    m_screen.setTextColor(m_fgcolour, m_bgcolour);
  }
  m_screen.drawString(m_content, topleft_x(), topleft_y(), m_font);
}

