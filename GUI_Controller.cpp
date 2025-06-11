#include <memory>
#include "GUI_Controller.h"
#include "GUI_Object.h"
#include "TextObject.h"
#include "ImageObject.h"
#include "logo.h"
#include "InputController.h"

extern volatile bool g_sendInitiated; //Flag for sending files through BLE

extern bool inSession;

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite screen = TFT_eSprite(&tft);

bool workout_active = false;

GUI_Controller::GUI_Controller()
    : objects {}
    , buttons {}
    , current_selected {NONE}
{
  
}

void GUI_Controller::init()
{
  tft_setup();
  tjpg_setup();
  screen.createSprite(WIDTH, HEIGHT);
  //TODO: placeholder, need to draw the time variable
  objects[CLOCK] = std::unique_ptr<GUI_Object>(new TextObject(screen, "00:00", 4, WIDTH, 0, GUI_Object::D_TR));
  objects[HEART_SYMBOL] = std::unique_ptr<GUI_Object>(new ImageObject(screen, WIDTH/2, HEIGHT/3, "/heart96.jpg", GUI_Object::D_MC));
  //TODO: placeholder, need to draw the heartrate variable
  objects[HEARTRATE] = std::unique_ptr<GUI_Object>(new TextObject(screen, "76", 6, objects[HEART_SYMBOL]->x(), objects[HEART_SYMBOL]->y(), objects[HEART_SYMBOL]->datum()));
  objects[BPM] = std::unique_ptr<GUI_Object>(new TextObject(screen, "bpm", 2, 8 * WIDTH/10, objects[HEART_SYMBOL]->topleft_y() + objects[HEART_SYMBOL]->height(), GUI_Object::D_BR));
  objects[O2_SYMBOL] = std::unique_ptr<GUI_Object>(new ImageObject(screen, WIDTH/4, HEIGHT/2 + HEIGHT/2/4, "/o2_32.jpg", GUI_Object::D_MC));
  objects[O2_VALUE] = std::unique_ptr<GUI_Object>(new TextObject(screen, "80%", 4, objects[O2_SYMBOL]->x() + WIDTH/4, objects[O2_SYMBOL]->y(), GUI_Object::D_ML));
  objects[STEP_SYMBOL] = std::unique_ptr<GUI_Object>(new ImageObject(screen, WIDTH/4, HEIGHT/2 + HEIGHT/4, "/footprint32.jpg", GUI_Object::D_MC));
  objects[STEP_VALUE] = std::unique_ptr<GUI_Object>(new TextObject(screen, "1125", 4, objects[STEP_SYMBOL]->x() + WIDTH/4, objects[STEP_SYMBOL]->y(), GUI_Object::D_ML));
  buttons[NONE] = std::unique_ptr<ButtonObject>(nullptr);

  buttons[START_WORKOUT] = std::unique_ptr<ButtonObject>(new ButtonObject(screen, &start_workout_trigger, "Start Workout", 2, WIDTH/2, HEIGHT/2+(HEIGHT/2 * 3) / 4 + 10, screen.textWidth("Start Workout", 2) + 10, screen.fontHeight(2) + 10, GUI_Object::D_MC, TFT_BLACK, 0xAD90, 0xF7EF, TFT_DARKGREEN));
}

/*
void GUI_Controller::handle_input()
{
  static unsigned int idle = 0;
  static BUTTON current_selected = NONE;
  unsigned int current_time = millis();
  bool L_trigger = input.L_pressed();
  bool R_trigger = input.R_pressed();
  if (L_trigger)
  {
    idle = millis();
    select_button();
  }
  else if (R_trigger)
  {
    idle = millis();
    execute_button();
  }
  if (current_time - idle > 5000)
  {
    go_idle();
  }
}
*/

void GUI_Controller::select_button()
{
  if (current_selected == NONE)
  {
    current_selected = next_button(current_selected);
    buttons[current_selected]->select();
  }
  else
  {
    buttons[current_selected]->deselect();
    current_selected = next_button(current_selected);
    if (current_selected == NONE) current_selected = next_button(current_selected);
    buttons[current_selected]->select();
  }
}

void GUI_Controller::execute_button()
{
  if (current_selected == NONE)
  {
    current_selected = next_button(current_selected);
    Serial.println("current selected was none is " + String(current_selected));
    buttons[current_selected]->select();
  }
  else
    {
      buttons[current_selected]->trigger();

      if(current_selected == START_WORKOUT)
      {
        if (workout_active)
        {
          buttons[current_selected].reset(new ButtonObject(screen, &start_workout_trigger, "Stop Workout", 2, buttons[START_WORKOUT]->x(), buttons[START_WORKOUT]->y(), buttons[START_WORKOUT]->width(), buttons[START_WORKOUT]->height(), buttons[START_WORKOUT]->datum(), TFT_BLACK, 0xFA90, TFT_RED,/*0xF8FF, 0xF8DD,*/ TFT_MAROON));
        }
        else{
          buttons[current_selected].reset(new ButtonObject(screen, &start_workout_trigger, "Start Workout", 2, buttons[START_WORKOUT]->x(), buttons[START_WORKOUT]->y(), buttons[START_WORKOUT]->width(), buttons[START_WORKOUT]->height(), buttons[START_WORKOUT]->datum(), TFT_BLACK, 0xAD90, 0xF7EF, TFT_DARKGREEN));
        }
      }
    }
}

void GUI_Controller::go_idle(){
  if (current_selected != NONE)
    {
      buttons[current_selected]->deselect();
      current_selected = NONE;
    }
}

void GUI_Controller::update_values(String time, int32_t heartrate, int32_t oxygen, int steps)
{
  objects[CLOCK]->update_value(time);
  objects[HEARTRATE]->update_value(String(heartrate));
  objects[O2_VALUE]->update_value(String(oxygen) + "%");
  objects[STEP_VALUE]->update_value(String(steps));
}

void GUI_Controller::draw()
{
  screen.fillSprite(TFT_BLACK);
  for (auto it = objects.begin(); it != objects.end(); it++) {
    std::get<std::unique_ptr<GUI_Object>>(*it)->draw();
  }

  for (auto it = buttons.begin(); it != buttons.end(); it++) {
    if(std::get<std::unique_ptr<ButtonObject>>(*it)) std::get<std::unique_ptr<ButtonObject>>(*it)->draw();
  }
  screen.pushSprite(0,0);
}

void GUI_Controller::draw_startup()
{
  // draw the startup screen
  ImageObject startup_logo = ImageObject(screen, WIDTH/2, HEIGHT/2, "/logo130.jpg", GUI_Object::D_MC);
  startup_logo.draw();
  screen.pushSprite(0,0);
}

void GUI_Controller::tft_setup() {
  tft.begin();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
}

void GUI_Controller::tjpg_setup() {
  TJpgDec.setJpgScale(1); // don't scale the jpeg image
  TJpgDec.setSwapBytes(true); // the byte order can be swapped (needed for images)
  TJpgDec.setCallback(tft_output); // set the rendering function above
}


bool GUI_Controller::tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if ( y >= screen.height() ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  screen.pushImage(x, y, w, h, bitmap);

  // Return 1 to decode next block
  return 1;
}

void GUI_Controller::start_workout_trigger()
{
  if (inSession) {
    inSession = false;
    Serial.printf("Session ended!");
    g_sendInitiated = true;
  } 
  else {
    inSession = true;
    Serial.println("Session started!");
  }
}

GUI_Controller::BUTTON GUI_Controller::next_button(BUTTON current)
{
  switch (current)
  {
    case NONE:
    return START_WORKOUT;
    case START_WORKOUT:
    return NONE;
  }
}
GUI_Controller::BUTTON GUI_Controller::prev_button(BUTTON current)
{
  switch (current)
  {
    case NONE:
    return START_WORKOUT;
    case START_WORKOUT:
    return NONE;
  }
}