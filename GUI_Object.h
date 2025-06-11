#ifndef MY_GUI_OBJECT
#define MY_GUI_OBJECT

#include <TFT_eSPI.h>

class GUI_Object {
  public:
  enum OBJECT_TYPE {
      TEXT,
      IMAGE,
      BUTTON
  };

  enum DATUM {
    D_TL, // Top left
    D_TC, // Top centre
    D_TR, // Top right
    D_ML, // Middle left
    D_MC, // Middle centre
    D_MR, // Middle right
    D_BL, // Bottom left
    D_BC, // Bottom centre
    D_BR  // Bottom right
  };

  GUI_Object(TFT_eSprite& screen, OBJECT_TYPE type, uint16_t x, uint16_t y, int width, int height, DATUM datum = DATUM::D_TL);

  OBJECT_TYPE get_type();

  int width();
  int height();
  uint16_t x();
  uint16_t y();
  DATUM datum();

  int topleft_x();
  int topleft_y();

  virtual void update_value(String value) = 0;
  virtual void draw() = 0;

  protected:
  TFT_eSprite& m_screen;
  OBJECT_TYPE m_type;
  uint16_t m_x;
  uint16_t m_y;
  int m_width;
  int m_height;
  DATUM m_datum;

};

#endif