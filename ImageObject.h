#ifndef MY_IMAGE_OBJECT
#define MY_IMAGE_OBJECT

#include "GUI_Object.h"

class ImageObject : public GUI_Object {
  public:
  ImageObject(TFT_eSprite& screen, uint16_t x, uint16_t y, char* file_name, GUI_Object::DATUM datum = GUI_Object::D_TL); 

  void update_value(String value) override;
  void draw() override;

  private:
  char* m_file_name;

  

};

#endif