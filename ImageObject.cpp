#include "ImageObject.h"
#include <TJpg_Decoder.h>

ImageObject::ImageObject(TFT_eSprite& screen, uint16_t x, uint16_t y, char* file_name, GUI_Object::DATUM datum) 
  : GUI_Object{ screen, GUI_Object::IMAGE, x, y, 0, 0, datum}
  , m_file_name { file_name }
{
  TJpgDec.getSdJpgSize((short unsigned int*)&m_width, (short unsigned int*)&m_height, m_file_name);
}

void ImageObject::update_value(String value){

}

void ImageObject::draw()
{
  TJpgDec.drawSdJpg(topleft_x(), topleft_y(), m_file_name);
}

