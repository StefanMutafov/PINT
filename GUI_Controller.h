#ifndef MY_GUI_CONTROLLER
#define MY_GUI_CONTROLLER

#include <TFT_eSPI.h>
#include <map>
#include "GUI_Object.h"
#include "ButtonObject.h"
#include <TJpg_Decoder.h>

extern TFT_eSPI tft;
extern TFT_eSprite screen;

class GUI_Controller {
  public:

  enum SCREEN_OBJECT {
    HEART_SYMBOL,
    HEARTRATE,
    BPM,
    CLOCK,
    O2_SYMBOL,
    O2_VALUE,
    STEP_SYMBOL,
    STEP_VALUE
  };

  enum BUTTON {
    NONE,
    START_WORKOUT
  };

  GUI_Controller();
  void init();

  const int HEIGHT = 240;
  const int WIDTH = 135;

  void handle_input();
  void execute_button();
  void select_button();
  void go_idle();
  void update_values(String time, int32_t heartrate, int32_t oxygen, int steps);
  
  void draw();
  void draw_startup();

  static bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);
  static void start_workout_trigger();

  private:
  std::map<SCREEN_OBJECT, std::unique_ptr<GUI_Object>> objects;
  std::map<BUTTON, std::unique_ptr<ButtonObject>> buttons;

  BUTTON current_selected;
  

  void tft_setup();
  void tjpg_setup();

  BUTTON next_button(BUTTON current);
  BUTTON prev_button(BUTTON current);
  
  

};

#endif