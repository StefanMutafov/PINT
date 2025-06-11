#include "GUI_Object.h"

GUI_Object::GUI_Object(TFT_eSprite& screen, OBJECT_TYPE type, uint16_t x, uint16_t y, int width, int height, DATUM datum)
  : m_type{ type }
  , m_x{ x }
  , m_y{ y }
  , m_width{ width }
  , m_height{ height }
  , m_screen{ screen }
  , m_datum{ datum }
  {
    
  }

GUI_Object::OBJECT_TYPE GUI_Object::get_type()
{
  return m_type;
}

int GUI_Object::width()
{
  return m_width;
}

int GUI_Object::height()
{
  return m_height;
}

uint16_t GUI_Object::x()
{
  return m_x;
}

uint16_t GUI_Object::y()
{
  return m_y;
}

GUI_Object::DATUM GUI_Object::datum()
{
  return m_datum;
}

int GUI_Object::topleft_x()
{
  switch (m_datum)
  {
    case DATUM::D_TL:
    case DATUM::D_ML:
    case DATUM::D_BL:
      return m_x;
    case DATUM::D_TC:
    case DATUM::D_MC:
    case DATUM::D_BC:
      return m_x - m_width/2;
    case DATUM::D_TR:
    case DATUM::D_MR:
    case DATUM::D_BR:
      return m_x - m_width;
    default:
      return m_x;
  }
}

int GUI_Object::topleft_y()
{
  switch (m_datum)
  {
    case DATUM::D_TL:
    case DATUM::D_TC:
    case DATUM::D_TR:
      return m_y;
    case DATUM::D_ML:
    case DATUM::D_MC:
    case DATUM::D_MR:
      return m_y - m_height/2;
    case DATUM::D_BL:
    case DATUM::D_BC:
    case DATUM::D_BR:
      return m_y - m_height;
    default:
      return m_y;
  }
}