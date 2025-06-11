#ifndef MY_INPUT_CONTROLLER
#define MY_INPUT_CONTROLLER


class InputController {
  public:

  InputController();
  void init();

  bool L_pressed();
  bool R_pressed();



  private:
  int m_prev_L;
  int m_prev_R;

  unsigned long m_last_Ldebounce;
  unsigned long m_last_Rdebounce;

  const unsigned long debounce_time = 50;
  

};
#endif